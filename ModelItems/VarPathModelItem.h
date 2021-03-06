//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#ifndef FABRICUI_MODELITEMS_VARPATHMODELITEM_H
#define FABRICUI_MODELITEMS_VARPATHMODELITEM_H

#include <FabricUI/ValueEditor/BaseModelItem.h>
#include <FabricUI/ValueEditor/QVariantRTVal.h>
#include <FTL/StrRef.h>

namespace FabricUI {

namespace DFG {
class DFGUICmdHandler;
} // namespace DFG

namespace ModelItems {

class VarPathItemMetadata;

//////////////////////////////////////////////////////////////////////////
// Basic ModelItem for accessing ports
class VarPathModelItem : public FabricUI::ValueEditor::BaseModelItem
{
protected:

  DFG::DFGUICmdHandler *m_dfgUICmdHandler;
  FabricCore::DFGBinding m_binding;
  std::string m_execPath;
  FabricCore::DFGExec m_exec;
  std::string m_refName;

  VarPathItemMetadata *m_metadata;

public:

  VarPathModelItem(
    DFG::DFGUICmdHandler *dfgUICmdHandler,
    FabricCore::DFGBinding binding,
    FTL::StrRef execPath,
    FabricCore::DFGExec exec,
    FTL::StrRef refName
    );
  ~VarPathModelItem();

  /////////////////////////////////////////////////////////////////////////
  // Name
  /////////////////////////////////////////////////////////////////////////

  virtual FTL::CStrRef getName() /*override*/;

  virtual bool canRename() /*override*/;

  virtual void rename( FTL::CStrRef newName ) /*override*/;
  
  virtual void onRenamed(
    FTL::CStrRef oldName,
    FTL::CStrRef newName
    ) /*override*/;

  /////////////////////////////////////////////////////////////////////////
  // Others
  /////////////////////////////////////////////////////////////////////////

  virtual QVariant getValue() /*override*/;

  virtual void setValue(
    QVariant value,
    bool commit,
    QVariant valueAtInteractionBegin
    ) /*override*/;

  virtual bool hasDefault() /*override*/;
  virtual void resetToDefault() /*override*/;

  virtual FabricUI::ValueEditor::ItemMetadata* getMetadata();

protected:

  virtual void setMetadataImp(
    const char* key,
    const char* value,
    bool canUndo
    ) /*override*/;
};

} // namespace ModelItems
} // namespace FabricUI

#endif // FABRICUI_MODELITEMS_VARPATHMODELITEM_H
