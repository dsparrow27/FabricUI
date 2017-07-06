//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "RTValFCurveViewItem.h"
#include <FabricUI/ValueEditor/QVariantRTVal.h>
#include <FabricUI/FCurveEditor/FCurveEditor.h>
#include <FabricUI/FCurveEditor/Models/AnimXKL/RTValAnimXFCurveModel.h>

#include <FabricUI/ModelItems/DFGModelItemMetadata.h>

#include <FabricUI/Commands/CommandManager.h>
#include <FabricUI/Commands/KLCommandRegistry.h> // HACK: remove

#include <assert.h>
#include <QDebug>

using namespace FabricUI::ValueEditor;
using namespace FabricUI::FCurveEditor;

class RTValFCurveViewItem::RTValAnimXFCurveDFGController : public RTValAnimXFCurveConstModel
{
  std::string m_bindingId, m_dfgPortPath;

public:
  void setPath( const char* bindingId, const char* dfgPortPath )
  {
    m_bindingId = bindingId;
    m_dfgPortPath = dfgPortPath;
  }

  void setHandle( size_t i, Handle h ) FTL_OVERRIDE
  {
    FabricUI::Commands::CommandManager* manager = FabricUI::Commands::CommandManager::getCommandManager();
    static_cast<FabricUI::Commands::KLCommandRegistry*>( FabricUI::Commands::KLCommandRegistry::getCommandRegistry() )->synchronizeKL(); // HACK : remove
    QMap<QString, QString> args;
    args["target"] = "<" + QString::fromUtf8( m_bindingId.data() ) + "." + QString::fromUtf8( m_dfgPortPath.data() ) + ">";
    args["index"] = QString::number( i );
    args["time"] = QString::number( h.pos.x() );
    args["value"] = QString::number( h.pos.y() );
    args["tanInX"] = QString::number( h.tanIn.x() );
    args["tanInY"] = QString::number( h.tanIn.y() );
    args["tanOutX"] = QString::number( h.tanOut.x() );
    args["tanOutY"] = QString::number( h.tanOut.y() );
    manager->createCommand( "SetHandle", args );
  }

  void addHandle() FTL_OVERRIDE
  {
    qDebug() << "RTValAnimXFCurveDFGController::addHandle " << m_bindingId.data() << "; " << m_dfgPortPath.data();
    // TODO
  }

  void deleteHandle( size_t i ) FTL_OVERRIDE
  {
    qDebug() << "RTValAnimXFCurveDFGController::deleteHandle " << m_bindingId.data() << "; " << m_dfgPortPath.data() << " : " << i;
    // TODO
  }
};

RTValFCurveViewItem::RTValFCurveViewItem(
  QString const &name,
  QVariant const &value,
  ItemMetadata* metadata
) : BaseViewItem( name, metadata )
  , m_model( new RTValAnimXFCurveDFGController() )
  , m_editor( new FabricUI::FCurveEditor::FCurveEditor() )
{
  m_editor->setModel( m_model );
  this->onModelValueChanged( value );

  m_editor->setFixedSize( 300, 300 ); // HACK

  //connect( m_model, SIGNAL( handleAdded() ), this, SLOT( onViewValueChanged() ) );
  //connect( m_model, SIGNAL( handleMoved( size_t ) ), this, SLOT( onViewValueChanged() ) );
  //connect( m_editor, SIGNAL( interactionBegin() ), this, SIGNAL( interactionBegin() ) );
  //connect( m_editor, SIGNAL( interactionEnd() ), this, SLOT( emitInteractionEnd() ) );

  const char* bindingId = metadata->getString( FabricUI::ModelItems::DFGModelItemMetadata::VEDFGBindingIdKey.data() );
  const char* portPath = metadata->getString( FabricUI::ModelItems::DFGModelItemMetadata::VEDFGPortPathKey.data() );
  m_model->setPath( bindingId, portPath );
}

RTValFCurveViewItem::~RTValFCurveViewItem()
{
  //delete m_editor; // HACK;TODO : delete and fix crash when closing Canvas
  delete m_model;
}

void RTValFCurveViewItem::onViewValueChanged()
{
  //emit this->viewValueChanged( QVariant::fromValue<FabricCore::RTVal>( m_model->value() ) );
}

QWidget* RTValFCurveViewItem::getWidget()
{
  return m_editor;
}

void RTValFCurveViewItem::onModelValueChanged( QVariant const & v )
{
  FabricCore::RTVal rtval = v.value<FabricCore::RTVal>();
  m_model->setValue( rtval );
}

BaseViewItem * RTValFCurveViewItem::CreateItem(
  QString const& name,
  QVariant const& value,
  ItemMetadata* metaData
)
{
  if( isRTVal( value ) )
  {
    const FabricCore::RTVal& val = value.value<FabricCore::RTVal>();
    if( val.isValid() )
    {
      const char* rtype = val.getTypeNameCStr();
      if( strcmp( rtype, "AnimX::AnimCurve" ) == 0 )
        return new RTValFCurveViewItem( name, value, metaData );
    }
  }
  return NULL;
}

const int RTValFCurveViewItem::Priority = 3;
