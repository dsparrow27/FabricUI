//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include <string>
#include "KLCommand.h"
#include "KLCommandManager.h"
#include "KLScriptableCommand.h"
  
using namespace FabricUI;
using namespace Commands;
using namespace FabricCore;

inline bool isKLCommand(
  Command *cmd)
{
  KLCommand *klCmd = dynamic_cast<KLCommand *>(cmd);
  KLScriptableCommand *klScriptCmd = dynamic_cast<KLScriptableCommand *>(cmd);
  return (klCmd || klScriptCmd);
}

KLCommandManager::KLCommandManager(
  Client client) 
  : RTValCommandManager(client)
  , m_klCmdUndoStackCount(0)
{
  try 
  {
    m_klCmdManager = RTVal::Create(
      m_client, 
      "CommandManager", 
      0, 
      0);

    m_klCmdManager = m_klCmdManager.callMethod(
      "CommandManager", 
      "getCommandManager", 
      0, 
      0);
  }

  catch(Exception &e)
  {
    printf(
      "KLCommandManager::KLCommandManager: exception: %s\n", 
      e.getDesc_cstr());
  }
}

KLCommandManager::~KLCommandManager() 
{
}

void KLCommandManager::undoCommand() 
{
  if(m_undoStack.size() == 0)
  {
    printf("KLCommandManager::undoCommand: nothing to undo");
    return;
  }

  CommandManager::undoCommand();
  StackedCommand stackedCmd = m_redoStack[m_redoStack.size() - 1];
  m_klCmdUndoStackCount -= isKLCommand(stackedCmd.topLevelCmd) ? 1 : 0;
}

void KLCommandManager::redoCommand() 
{
  if(m_redoStack.size() == 0)
  {
    printf("KLCommandManager::redoCommand: nothing to redo");
    return;
  }

  CommandManager::redoCommand();
  StackedCommand stackedCmd = m_undoStack[m_undoStack.size() - 1];
  m_klCmdUndoStackCount += isKLCommand(stackedCmd.topLevelCmd) ? 1 : 0;
}

void KLCommandManager::clear() 
{
  try 
  {
    m_klCmdManager.callMethod(
      "", 
      "clear", 
      0, 
      0);
  }

  catch(Exception &e)
  {
    printf(
      "KLCommandManager::clear: exception: %s\n", 
      e.getDesc_cstr());
  }
  
  m_klCmdUndoStackCount = 0;
  CommandManager::clear();
}

RTVal KLCommandManager::getKLCommandManager()
{
  return m_klCmdManager;
}

QString KLCommandManager::getContent()
{
  QString res = CommandManager::getContent();

  try 
  {
    res += "\n" +
      QString(
        m_klCmdManager.callMethod(
          "String", 
          "getContent", 
          0, 
          0).getStringCString()
      );
  }

  catch(Exception &e)
  {
    printf(
      "KLCommandManager::getContent: exception: %s\n", 
      e.getDesc_cstr());
  }

  return res;
}

void KLCommandManager::synchronizeKL() 
{
  // Get the number of KL commands in the KL manager undo stack
  unsigned int klCmdCount = 0;
  try
  {
    klCmdCount = m_klCmdManager.callMethod(
      "SInt32", 
      "getStackIndex", 
      0, 
      0).getSInt32();

    if(klCmdCount > 0)
      klCmdCount ++;

    // !! Problem, KL and C++ command managers are out of synch
    if(m_klCmdUndoStackCount > klCmdCount)
      throw(
        QString(
          "KLCommandManager::synchronizeKL: the KL and C++ command managers are out of synch"
        ).toUtf8().constData() 
      );

    // Synchronize our stack with KL, two scenarios: 
    // 1. A KL command is created in KL : we construct the
    //    KLCommand and KLScriptableCommand wrappers.
    // 2. A C++/Python command is asked to be created from KL.
    unsigned int klCmdUndoStackCount = m_klCmdUndoStackCount;

    for(unsigned int i=klCmdUndoStackCount; i<klCmdCount; ++i)
    {
      RTVal cmdIndex = RTVal::ConstructUInt32(
        m_client, 
        i);
      
      // Gets the KL command from the KL manager. 
      RTVal klCmd = m_klCmdManager.callMethod(
        "Command", 
        "getCommandAtIndex", 
        1, 
        &cmdIndex);

      // Check if it's an AppCommand.
      // Construct C++ commands from KL
      RTVal appCmd = RTVal::Construct(
        m_client,
        "AppCommand", 
        1, 
        &klCmd);

      if(appCmd.isValid() && !appCmd.isNullObject())
      {     
        // Gets the command name.
        QString cmdName = appCmd.callMethod(
          "String",
          "getName",
          0, 
          0).getStringCString();
 
        RTVal argNameList = appCmd.callMethod(
          "String[]", 
          "getArgNameList", 
          0, 
          0);

        QMap< QString, RTVal > args;
        for(unsigned int j=0; j<argNameList.getArraySize(); ++j)
        {
          RTVal argNameVal = argNameList.getArrayElement(j);
          args[argNameVal.getStringCString()] = appCmd.callMethod(
            "RTVal", 
            "getArg", 
            1, 
            &argNameVal);
        }

        // Remove the KL command
        m_klCmdManager.callMethod(
          "Boolean",
          "removeCommandAtIndex",
          1,
          &cmdIndex);

        // decrement
        i--; klCmdCount--;

        // Create and execute the C++ command.
        createRTValCommand(cmdName, args);
      }

      // KL commands have actually been 
      // created, create the C++ wrappers.
      else
      {
        RTVal scriptCmd = RTVal::Construct(
          m_client,
          "ScriptableCommand", 
          1, 
          &klCmd);

        if(scriptCmd.isValid() && !scriptCmd.isNullObject())
          pushTopCommand( 
            new KLScriptableCommand(scriptCmd), 
            true);

        else
          pushTopCommand( 
            new KLCommand(klCmd), 
            true);

        m_klCmdUndoStackCount ++;
      }
    } 
  }

  catch(Exception &e)
  {
    printf(
      "KLCommandManager::synchronizeKL: exception: %s\n", 
      e.getDesc_cstr());
  }
}

void KLCommandManager::clearRedoStack() 
{
  try 
  {
    m_klCmdManager.callMethod(
      "", 
      "clearRedoStack", 
      0, 
      0);
      
    CommandManager::clearRedoStack();
  }

  catch(Exception &e)
  {
    printf(
      "KLCommandManager::clearRedoStack: exception: %s\n", 
      e.getDesc_cstr());
  }
}