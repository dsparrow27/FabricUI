//
// Copyright 2010-2016 Fabric Software Inc. All rights reserved.
//

#include "QVariantRTVal.h"
#include "Vec2ViewItem.h"
#include "ViewItemFactory.h"

#include <assert.h>
#include <QtCore/QVariant.h>
#include <QtGui/QBoxLayout.h>
#include <QtGui/QLineEdit.h>
#include <QtGui/QWidget.h>

Vec2ViewItem::Vec2ViewItem(
  QString const &name,
  QVariant const &value
  )
  : BaseComplexViewItem( name )
  , m_vec2dValue( value.value<QVector2D>() )
{
  m_widget = new QWidget;

  m_xEdit = new QLineEdit( QString::number( m_vec2dValue.x() ), m_widget );
  m_yEdit = new QLineEdit( QString::number( m_vec2dValue.y() ), m_widget );

  // Connect em up.
  connect(
    m_xEdit, SIGNAL(editingFinished()),
    this, SLOT(onTextEditXChanged())
    );
  connect(
    m_yEdit, SIGNAL(editingFinished()),
    this, SLOT(onTextEditYChanged())
    );

  QHBoxLayout *layout = new QHBoxLayout( m_widget );
  layout->addWidget( m_xEdit );
  layout->addWidget( m_yEdit );
}

Vec2ViewItem::~Vec2ViewItem()
{
}

QWidget *Vec2ViewItem::getWidget()
{
  return m_widget;
}

void Vec2ViewItem::onModelValueChanged( QVariant const &value )
{
  QVector2D newVec2dValue = value.value<QVector2D>();
  if ( newVec2dValue.x() != m_vec2dValue.x() )
  {
    m_xEdit->setText( QString::number( newVec2dValue.x() ) );
    routeModelValueChanged( 0, QVariant( newVec2dValue.x() ) );
  }
  if ( newVec2dValue.y() != m_vec2dValue.y() )
  {
    m_yEdit->setText( QString::number( newVec2dValue.y() ) );
    routeModelValueChanged( 1, QVariant( newVec2dValue.y() ) );
  }
  m_vec2dValue = newVec2dValue;
}

void Vec2ViewItem::onTextEditXChanged()
{
  QVector2D vec2d = m_vec2dValue;
  vec2d.setX( m_xEdit->text().toDouble() );
  emit viewValueChanged( QVariant( vec2d ), true );
}

void Vec2ViewItem::onTextEditYChanged()
{
  QVector2D vec2d = m_vec2dValue;
  vec2d.setY( m_yEdit->text().toDouble() );
  emit viewValueChanged( QVariant( vec2d ), true );
}

void Vec2ViewItem::onChildViewValueChanged(
  int index,
  QVariant const &value,
  bool commit
  )
{
  QVector2D vec2d = m_vec2dValue;
  switch ( index )
  {
    case 0:
      vec2d.setX( value.toDouble() );
      break;
    case 1:
      vec2d.setY( value.toDouble() );
      break;
    default:
      assert( false );
      break;
  }
  emit viewValueChanged( QVariant( vec2d ), commit );
}

void Vec2ViewItem::doAppendChildViewItems(QList<BaseViewItem *>& items)
{
  ViewItemFactory* factory = ViewItemFactory::GetInstance();

  BaseViewItem *children[2];
  children[0] = factory->CreateViewItem( "X", QVariant( m_vec2dValue.x() ), &m_metadata );
  children[1] = factory->CreateViewItem( "Y", QVariant( m_vec2dValue.y() ), &m_metadata );
  for ( int i = 0; i < 2; ++i )
  {
    connectChild( i, children[i] );
    items.append( children[i] );
  }
}

//////////////////////////////////////////////////////////////////////////
// 
BaseViewItem* Vec2ViewItem::CreateItem(
  QString const &name,
  QVariant const &value,
  ItemMetadata*
  )
{
  if ( RTVariant::isType<QVector2D>( value ) )
    return new Vec2ViewItem( name, value );
  else
    return 0;
}

const int Vec2ViewItem::Priority = 3;
