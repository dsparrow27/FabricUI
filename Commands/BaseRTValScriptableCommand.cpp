//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "CommandManager.h"
#include "BaseRTValScriptableCommand.h"
#include <FabricUI/Util/RTValUtil.h>

using namespace FabricUI;
using namespace Commands;
using namespace FabricCore;

BaseRTValScriptableCommand::BaseRTValScriptableCommand() 
  : BaseScriptableCommand()
{
}

BaseRTValScriptableCommand::~BaseRTValScriptableCommand() 
{
}

void BaseRTValScriptableCommand::declareArg(
  const QString &key, 
  const QString &type,
  bool optional, 
  const QString &defaultValue) 
{
  BaseScriptableCommand::declareArg(
    key,
    optional,
    defaultValue);

  setArgType(key, type);
}

void BaseRTValScriptableCommand::declareArg(
  const QString &key, 
  const QString &type,
  bool optional, 
  RTVal defaultValue) 
{
  QString defaultValueStr = Util::RTValUtil::rtValToJSON(
    defaultValue
  );

  BaseScriptableCommand::declareArg(
    key,
    optional,
    defaultValueStr);

  setArgType(key, type);
}
 
QString BaseRTValScriptableCommand::getArgType(
  const QString &key)
{
  if(m_argSpecs.count(key) == 0) 
    throw(
      std::string(
        "BaseRTValScriptableCommand::getArgType, error no arg named " +
        std::string(key.toUtf8().constData())
      )
    );

  return m_argSpecs[key].type;
}

void BaseRTValScriptableCommand::setArgType(
  const QString &key, 
  const QString &type) 
{
  if(m_argSpecs.count(key) == 0) 
    throw(
      std::string(
        "BaseRTValScriptableCommand::setArgType, error no arg named " + 
        std::string(key.toUtf8().constData())
      )
    );

  m_argSpecs[key].type = type;
}

RTVal BaseRTValScriptableCommand::getArgAsRTVal(
  const QString &key)
{
  RTVal res;

  try
  {
    if(m_args.count(key) > 0)
      res = Util::RTValUtil::jsonToRTVal(
        Commands::CommandManager::GetCommandManager()->getFabricClient(),
        getArg(key),
        m_argSpecs[key].type);
  }

  catch(Exception &e)
  {
    printf(
      "BaseRTValScriptableCommand::getArgAsRTVal: exception: %s\n", 
      e.getDesc_cstr());
  }

  return res;
}

QMap<QString, RTVal> BaseRTValScriptableCommand::getArgsAsRTVal()
{
  QMap<QString, RTVal> res;
  
  QMapIterator<QString, QString> argsIt(m_args);
  while (argsIt.hasNext()) 
  {
    argsIt.next();
    QString argName = argsIt.key();
    res[argName] = getArgAsRTVal(argName);
  }

  return res;
}
