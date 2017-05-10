//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//
 
#include <FabricCore.h>
#include "../OptionsEditorHelpers.h"
#include "CloseKLOptionsTargetEditorCommand.h"
#include <FabricUI/Commands/CommandException.h>
#include <FabricUI/Commands/KLCommandManager.h>

using namespace FabricUI;
using namespace Commands;
using namespace FabricCore;
using namespace OptionsEditor;

CloseKLOptionsTargetEditorCommand::CloseKLOptionsTargetEditorCommand() 
  : BaseRTValScriptableCommand()
{
  try
  {
    KLCommandManager *manager = dynamic_cast<KLCommandManager *>(
      CommandManager::GetCommandManager());
    
    declareRTValArg(
      "editorID",
      "String",
      false);

    declareRTValArg(
      "failSilently",
      "Boolean",
      false,
      RTVal::ConstructBoolean(
        manager->getClient(), 
        false)
      );
  }

  catch(Exception &e)
  {
    CommandException::PrintOrThrow(
      "CloseKLOptionsTargetEditorCommand::CloseKLOptionsTargetEditorCommand",
      "",  
      e.getDesc_cstr(),
      PRINT | THROW);
  }

  catch(CommandException &e) 
  {
    CommandException::PrintOrThrow(
      "CloseKLOptionsTargetEditorCommand::CloseKLOptionsTargetEditorCommand",
      "",
      e.what(),
      PRINT | THROW);
  }
};

CloseKLOptionsTargetEditorCommand::~CloseKLOptionsTargetEditorCommand()
{
}

bool CloseKLOptionsTargetEditorCommand::canUndo() {
  return false;
}

bool CloseKLOptionsTargetEditorCommand::doIt() 
{ 
  bool res = false;

  try
  {
    bool failSilently = getRTValArg("failSilently").getBoolean();
    QString editorID = getRTValArg("editorID").getStringCString();

    QWidget *dock = GetOptionsEditorDock(editorID);

    if(dock == 0)
      res = failSilently;

    else
    {
      dock->close();
      res = true;
    }
  }

  catch(Exception &e)
  {
    CommandException::PrintOrThrow(
      "CloseKLOptionsTargetEditorCommand::doIt",
      "",  
      e.getDesc_cstr(),
      PRINT | THROW);
  }

  catch(CommandException &e) 
  {
    CommandException::PrintOrThrow(
      "CloseKLOptionsTargetEditorCommand::doIt",
      "",
      e.what(),
      PRINT | THROW);
  }

  return res;
}

QString CloseKLOptionsTargetEditorCommand::getHelp()
{
  QMap<QString, QString> argsHelp;

  argsHelp["editorID"] = "Qt objectName of the option editor / ID of the KL option in the OptionsTargetRegistry";
  argsHelp["failSilently"] = "If false, throws an error if the widget has not been closed";

  return createHelpFromArgs(
    "Close a Qt editor to edit a KL OptionsTarget",
    argsHelp);
}

QString CloseKLOptionsTargetEditorCommand::getHistoryDesc()
{
  QMap<QString, QString> argsDesc;

  argsDesc["editorID"] = getRTValArg(
    "editorID").getStringCString();

  argsDesc["failSilently"] = QString::number(
    getRTValArg("failSilently").getBoolean()
    );

  return createHistoryDescFromArgs(
    argsDesc);
}