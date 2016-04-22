/*
 *  Copyright 2010-2016 Fabric Software Inc. All rights reserved.
 */

#include "SHBaseTreeView.h"
#include <QtCore/QUrl>
#include <QtCore/QMimeData>
#include <QtCore/QAbstractItemModel>
#include <QtGui/QDrag>
#include <QtGui/QTreeWidgetItem>
#include <FabricUI/SceneHub/TreeView/SHTreeModel.h>

using namespace FabricUI;
using namespace FabricUI::SceneHub;


SHTreeView_ViewIndexTarget::SHTreeView_ViewIndexTarget(SHBaseTreeView *view, QModelIndex const &index, QObject *parent)
  : QObject( parent )
  , m_view( view )
  , m_index( index )
{}

void SHTreeView_ViewIndexTarget::expandRecursively( QModelIndex const &index ) {
  SHTreeModel *model = const_cast<SHTreeModel *>(
    static_cast<SHTreeModel const *>( index.model() )
    );
  SHTreeModel::SceneHierarchyChangedBlocker blocker( model );

  m_view->expand( index );
  int rows = model->rowCount( index );
  for ( int row = 0; row < rows; ++row ) {
    QModelIndex childIndex = model->index( row, 0, index );
    expandRecursively( childIndex );
  }
}

void SHTreeView_ViewIndexTarget::loadRecursively() {
  SHTreeModel *model = const_cast<SHTreeModel *>(
    static_cast<SHTreeModel const *>( m_index.model() )
    );
  SHTreeModel::SceneHierarchyChangedBlocker blocker( model );

  SHTreeItem *item = static_cast<SHTreeItem *>( m_index.internalPointer() );
  item->loadRecursively();
}

void SHTreeView_ViewIndexTarget::setVisibility( bool visible, unsigned char propagationType ) {
  SHTreeItem *item = static_cast<SHTreeItem *>( m_index.internalPointer() );
  FabricCore::RTVal sgObjectVal = item->getSGObject();
  if( sgObjectVal.isValid() ) {

    SHTreeModel *model = const_cast<SHTreeModel *>(
      static_cast<SHTreeModel const *>( m_index.model() )
      );

    try {
      FabricCore::RTVal args[2];
      args[0] = FabricCore::RTVal::ConstructBoolean( m_view->m_client, visible );
      args[1] = FabricCore::RTVal::ConstructUInt8( m_view->m_client, propagationType );
      sgObjectVal.callMethod( "", "setVisibility", 2, args );
    }
    catch( FabricCore::Exception e ) {
      printf( "SHTreeView_ViewIndexTarget::setVisibility: Error: %s\n", e.getDesc_cstr() );
    }
    model->emitSceneChanged();
  }
}


// *********
SHBaseTreeView::SHBaseTreeView(FabricCore::Client &client, QWidget *parent) 
  : QTreeView( parent ) 
{
  m_client = client;
  this->setAcceptDrops(true);
}

void SHBaseTreeView::setSelectedObjects( FabricCore::RTVal selectedSGObjectArray ) {
  SHTreeModel* treeModel = (SHTreeModel*)model();
  try 
  {
    std::vector< QModelIndex > selectedIndices;

    unsigned int count = selectedSGObjectArray.getArraySize();
    for(unsigned int i = 0; i < count; ++i) {
      FabricCore::RTVal sgObject = selectedSGObjectArray.getArrayElement( i );
      std::vector< QModelIndex > indices = treeModel->getIndicesFromSGObject( sgObject );
      selectedIndices.insert( selectedIndices.end(), indices.begin(), indices.end() );
    }

    if(selectedIndices.empty()) clearSelection();
    else 
    {
      QItemSelectionModel* selection = selectionModel();
      for( unsigned int i = 0; i < selectedIndices.size(); ++i )
        selection->select( selectedIndices[i], i == 0 ? (QItemSelectionModel::Current | QItemSelectionModel::Clear | QItemSelectionModel::Select) : QItemSelectionModel::Select );
    }
  }
  catch ( FabricCore::Exception e )
  {
    printf("SHTreeModel::setSelectedObjects: Error: %s\n", e.getDesc_cstr());
  }
}

SHTreeItem *SHBaseTreeView::getTreeItemAtIndex(QModelIndex index) {
  return static_cast<SHTreeItem *>( index.internalPointer() );
}

SHTreeItem *SHBaseTreeView::GetTreeItemAtIndex(QModelIndex index) {
  return static_cast<SHTreeItem *>( index.internalPointer() );
}