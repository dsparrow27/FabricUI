//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FCurveEditor.h"
#include "FCurveItem.h"
#include <FabricUI/FCurveEditor/RuledGraphicsView.h>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QToolBar>
#include <QPushButton>
#include <QComboBox>
#include <QMenu>

#include <QDebug>
#include <assert.h>

#include <FabricUI/Util/QtSignalsSlots.h>

using namespace FabricUI::FCurveEditor;

class FCurveEditor::KeyValueEditor : public QFrame
{
  FCurveEditor* m_parent;

  class FloatEditor : public QWidget
  {
    KeyValueEditor* m_parent;
    QLabel* m_label;
    QLineEdit* m_edit;
  public:
    FloatEditor( KeyValueEditor* parent, bool isXNotY )
      : QWidget( parent )
      , m_parent( parent )
      , m_label( new QLabel( isXNotY ? "X:" : "Y:" ) )
      , m_edit( new QLineEdit() )
    {
      this->setObjectName( "FloatEditor" );

      QHBoxLayout* m_layout = new QHBoxLayout();
      m_layout->setMargin( 0 );
      m_layout->addWidget( m_label );
      m_layout->addWidget( m_edit );
      this->setLayout( m_layout );
      this->set( 0 );

      if( isXNotY )
        QOBJECT_CONNECT( m_edit, SIGNAL, QLineEdit, editingFinished, ( ), m_parent->m_parent, SLOT, FCurveEditor, veXEditFinished, ( ) );
      else
        QOBJECT_CONNECT( m_edit, SIGNAL, QLineEdit, editingFinished, ( ), m_parent->m_parent, SLOT, FCurveEditor, veYEditFinished, ( ) );
    }
    inline void set( qreal v )
    {
      m_edit->setText( QString::number( v ) );
      m_edit->setCursorPosition( 0 );
    }
    inline void clear() { m_edit->clear(); }
    inline QString get() const { return m_edit->text(); }
  };

  class TangentTypeEditor : public QComboBox
  {
    AbstractFCurveModel* m_model;
  public:

    TangentTypeEditor()
      : m_model( NULL )
    {
      this->setObjectName( "TangentTypeEditor" );
    }

    void setModel( AbstractFCurveModel* model )
    {
      if( model != m_model )
      {
        this->clear();
        for( size_t i = 0; i < model->tangentTypeCount(); i++ )
          this->addItem( model->tangentTypeName( i ) );
        m_model = model;
      }
    }
  };

public:
  FloatEditor* m_x;
  FloatEditor* m_y;
  TangentTypeEditor* m_tanType;

  KeyValueEditor( FCurveEditor* parent )
    : QFrame( parent )
    , m_parent( parent )
    , m_x( new FloatEditor( this, true ) )
    , m_y( new FloatEditor( this, false ) )
    , m_tanType( new TangentTypeEditor() )
  {
    this->setObjectName( "KeyValueEditor" );

    QHBoxLayout* m_layout = new QHBoxLayout();
    m_layout->setContentsMargins( QMargins( 8, 2, 8, 2 ) );
    m_layout->setSpacing( 8 );
    m_layout->addWidget( m_x );
    m_layout->addWidget( m_y );
    m_layout->addWidget( m_tanType );
    QOBJECT_CONNECT_OVERLOADED( m_tanType, SIGNAL, QComboBox, currentIndexChanged, ( int ),, parent, SLOT, FCurveEditor, veTanTypeEditFinished, ( ), );
    this->setLayout( m_layout );
    this->resize( 200, 32 );
  }
};

void FCurveEditor::veEditFinished( bool isXNotY )
{
  const QString text = ( isXNotY ? m_keyValueEditor->m_x->get() : m_keyValueEditor->m_y->get() );
  bool ok;
  const qreal v = text.toDouble( &ok );
  if( !ok )
    this->onEditedKeysChanged(); // clear what the user entered, and fetch the model values
  else
  {
    assert( !m_curveItem->selectedKeys().empty() );
    if( m_curveItem->selectedKeys().size() == 1 )
    {
      size_t editedKey = *m_curveItem->selectedKeys().begin();
      Key h = m_model->getKey( editedKey );
      QPointF* p = NULL;
      switch( m_curveItem->editedKeyProp() )
      {
      case FCurveItem::POSITION: p = &h.pos; break;
      case FCurveItem::TAN_IN: p = &h.tanIn; break;
      case FCurveItem::TAN_OUT: p = &h.tanOut; break;
      case FCurveItem::NOTHING: assert( false ); break;
      }
      if( isXNotY )
        p->setX( v );
      else
        p->setY( v );
      m_model->setKey( editedKey, h );
    }
    else
    {
      for( std::set<size_t>::const_iterator it = m_curveItem->selectedKeys().begin();
        it != m_curveItem->selectedKeys().end(); it++ )
      {
        assert( !isXNotY ); // several keys shouldn't have the same time value, so the field should be disabled then
        Key h = m_model->getKey( *it );
        h.pos.setY( v );
        m_model->setKey( *it, h );
      }
    }
  }
}

void FCurveEditor::veTanTypeEditFinished()
{
  assert( !m_curveItem->selectedKeys().empty() );
  assert( m_curveItem->selectedKeys().size() == 1 );
  {
    size_t editedKey = *m_curveItem->selectedKeys().begin();
    Key h = m_model->getKey( editedKey );
    FCurveItem::KeyProp prop = m_curveItem->editedKeyProp();
    if( prop == FCurveItem::TAN_IN || prop == FCurveItem::TAN_OUT )
    {
      size_t& t = ( prop == FCurveItem::TAN_IN ? h.tanInType : h.tanOutType );
      t = m_keyValueEditor->m_tanType->currentIndex();
      m_model->setKey( editedKey, h );
    }
  }
}

const char* ModeNames[FCurveItem::MODE_COUNT] =
{
  "Select",
  "Add",
  "Remove"
};

const Qt::KeyboardModifier ModeToggleModifier = Qt::ShiftModifier;

Qt::Key ModeKeys[FCurveItem::MODE_COUNT] =
{
  Qt::Key_1,
  Qt::Key_2,
  Qt::Key_3
};

inline QKeySequence GetModeToggleShortcut( FCurveItem::Mode m )
{
  return QKeySequence( ModeToggleModifier ).toString() + QKeySequence( ModeKeys[m] ).toString();
}

class FCurveEditor::ToolBar : public QWidget
{
  FCurveEditor* m_parent;
  QHBoxLayout* m_layout;
  QPushButton* m_buttons[FCurveItem::MODE_COUNT];
  FCurveItem::Mode m_previousMode; // MODE_COUNT if not in a temporary mode

public:
  QAction* m_modeActions[FCurveItem::MODE_COUNT];

  ToolBar( FCurveEditor* parent )
    : m_parent( parent )
    , m_layout( new QHBoxLayout() )
    , m_previousMode( FCurveItem::MODE_COUNT )
  {
    this->setObjectName( "ToolBar" );
    this->setMinimumHeight( 40 );

    m_layout->setAlignment( Qt::AlignLeft );
    m_layout->setMargin( 8 );
    m_layout->setContentsMargins( 8, 2, 2, 2 );

    for( int m = 0; m < FCurveItem::MODE_COUNT; m++ )
    {
      m_buttons[m] = new QPushButton();
      QPushButton* bt = m_buttons[m];
      bt->setObjectName( ModeNames[m] );
      bt->setFixedSize( QSize( 26, 26 ) );
      bt->setCheckable( true );
      m_layout->addWidget( bt );
      QString actionName = QString::fromUtf8( ModeNames[m] ) + " Keys Mode";
      bt->setToolTip( actionName + " [Press (" + QKeySequence(ModeToggleModifier).toString() + ")" + QKeySequence(ModeKeys[m]).toString() + "]" );
      m_modeActions[m] = new QAction( actionName, m_parent );
      QAction* action = m_modeActions[m];
      action->setShortcut( GetModeToggleShortcut( FCurveItem::Mode(m) ) );
      action->setShortcutContext( Qt::WidgetWithChildrenShortcut );
      m_parent->addAction( action );
      QOBJECT_CONNECT( action, SIGNAL, QAction, triggered, ( bool ), bt, SIGNAL, QPushButton, released, ( ) );
    }

    QOBJECT_CONNECT( m_buttons[FCurveItem::SELECT], SIGNAL, QPushButton, released, ( ), m_parent, SLOT, FCurveEditor, setModeSelect, ( ) );
    QOBJECT_CONNECT( m_buttons[FCurveItem::ADD], SIGNAL, QPushButton, released, ( ), m_parent, SLOT, FCurveEditor, setModeAdd, ( ) );
    QOBJECT_CONNECT( m_buttons[FCurveItem::REMOVE], SIGNAL, QPushButton, released, ( ), m_parent, SLOT, FCurveEditor, setModeRemove, ( ) );

    this->setLayout( m_layout );
  }

  void setMode( FCurveItem::Mode m )
  {
    for( int i = 0; i < FCurveItem::MODE_COUNT; i++ )
      m_buttons[i]->setChecked( i == m );
    if( m == m_previousMode )
      m_previousMode = FCurveItem::MODE_COUNT;
  }

  inline void setTemporaryMode( FCurveItem::Mode m )
  {
    FCurveItem::Mode previousMode = m_parent->m_curveItem->mode();
    if( m != previousMode )
    {
      if( !inTemporaryMode() )
        m_previousMode = previousMode;
      m_parent->m_curveItem->setMode( m );
    }
  }
  inline bool inTemporaryMode() const { return m_previousMode < FCurveItem::MODE_COUNT; }
  inline FCurveItem::Mode getPreviousMode() const { assert( inTemporaryMode() ); return m_previousMode; }
};

FCurveEditor::FCurveEditor()
  : m_rview( new RuledGraphicsView() )
  , m_model( NULL )
  , m_scene( new QGraphicsScene() )
  , m_curveItem( new FCurveItem() )
  , m_toolBar( new ToolBar( this ) )
{
  this->setObjectName( "FCurveEditor" );
  this->setContextMenuPolicy(Qt::CustomContextMenu);

  QVBoxLayout* m_layout = new QVBoxLayout();
  m_layout->setMargin( 0 ); m_layout->setSpacing( 0 );
  m_layout->addWidget( m_toolBar );
  m_layout->addWidget( m_rview );
  this->setLayout( m_layout );
  m_keyValueEditor = new KeyValueEditor( this );

  m_scene->setSceneRect( QRectF( -1E8, -1E8, 2 * 1E8, 2 * 1E8 ) );
  m_rview->view()->setScene( m_scene );
  m_scene->addItem( m_curveItem );

  QOBJECT_CONNECT(
    this, SIGNAL, FCurveEditor, customContextMenuRequested, (const QPoint&),
    this, SLOT, FCurveEditor, showContextMenu, (const QPoint&));

  QOBJECT_CONNECT( m_curveItem, SIGNAL, FCurveItem, interactionBegin, (), this, SIGNAL, FCurveEditor, interactionBegin, () );
  QOBJECT_CONNECT( m_curveItem, SIGNAL, FCurveItem, interactionEnd, (), this, SIGNAL, FCurveEditor, interactionEnd, () );
  QOBJECT_CONNECT( m_curveItem, SIGNAL, FCurveItem, modeChanged, (), this, SLOT, FCurveEditor, onModeChanged, () );
  QOBJECT_CONNECT(
    m_rview, SIGNAL, RuledGraphicsView, rectangleSelectReleased, ( const QRectF&, Qt::KeyboardModifiers ),
    this, SLOT, FCurveEditor, onRectangleSelectReleased, ( const QRectF&, Qt::KeyboardModifiers )
  );
  QOBJECT_CONNECT(
    m_curveItem, SIGNAL, FCurveItem, selectionChanged, ( ),
    this, SLOT, FCurveEditor, onSelectionChanged, ( )
  );
  QOBJECT_CONNECT(
    m_curveItem, SIGNAL, FCurveItem, editedKeyValueChanged, ( ),
    this, SLOT, FCurveEditor, onEditedKeysChanged, ( )
  );
  QOBJECT_CONNECT(
    m_curveItem, SIGNAL, FCurveItem, editedKeyPropChanged, ( ),
    this, SLOT, FCurveEditor, onEditedKeysChanged, ( )
  );
  QOBJECT_CONNECT( m_curveItem, SIGNAL, FCurveItem, repaintViews, ( ), this, SLOT, FCurveEditor, onRepaintViews, ( ) );

#define DEFINE_FCE_ACTION_NOSHORTCUT( member, strName, slot ) \
  member = new QAction( strName, this ); \
  QOBJECT_CONNECT( member, SIGNAL, QAction, triggered, (), this, SLOT, FCurveEditor, slot, () ); \
  this->addAction( member );

#define DEFINE_FCE_ACTION( member, strName, slot, shortcut ) \
  DEFINE_FCE_ACTION_NOSHORTCUT( member, strName, slot ) \
  member->setShortcutContext( Qt::WidgetWithChildrenShortcut ); \
  member->setShortcut( shortcut );

  DEFINE_FCE_ACTION( m_keysSelectAllAction, "Select all keys", onSelectAllKeys, Qt::CTRL + Qt::Key_A )
  DEFINE_FCE_ACTION( m_keysDeselectAllAction, "Deselect all keys", onDeselectAllKeys, Qt::CTRL + Qt::SHIFT + Qt::Key_A )
  DEFINE_FCE_ACTION( m_keysFrameAllAction, "Frame All Keys", onFrameAllKeys, Qt::Key_A )
  DEFINE_FCE_ACTION( m_keysFrameSelectedAction, "Frame Selected Keys", onFrameSelectedKeys, Qt::Key_F )
  DEFINE_FCE_ACTION( m_keysDeleteAction, "Delete selected Keys", onDeleteSelectedKeys, Qt::Key_Delete )
  DEFINE_FCE_ACTION_NOSHORTCUT( m_tangentsZeroSlopeAction, "Zero-slope Tangents", onTangentsZeroSlope )
  DEFINE_FCE_ACTION_NOSHORTCUT( m_presetRampIn, "Ramp In", onPresetRampIn )
  DEFINE_FCE_ACTION_NOSHORTCUT( m_presetRampOut, "Ramp Out", onPresetRampOut )
  DEFINE_FCE_ACTION_NOSHORTCUT( m_presetSmoothStep, "Smooth Step", onPresetSmoothStep )
  DEFINE_FCE_ACTION_NOSHORTCUT( m_clearAction, "Clear all keys", onClearAllKeys )

  this->setVEPos( QPoint( -20, 20 ) );
  this->setToolBarEnabled( true );
  m_curveItem->setMode( FCurveItem::SELECT );
}

void FCurveEditor::resizeEvent( QResizeEvent * e )
{
  Parent::resizeEvent( e );
  this->updateVEPos();
}

void FCurveEditor::updateVEPos()
{
  if( !this->toolBarEnabled() )
  {
    m_keyValueEditor->setGeometry( QRect(
      ( m_vePos.x() < 0 ?
        this->rect().right() + m_vePos.x() - m_keyValueEditor->width() :
        this->rect().left() + m_vePos.x()
        ),
        ( m_vePos.y() < 0 ?
          this->rect().bottom() + m_vePos.y() - m_keyValueEditor->height() :
          this->rect().top() + m_vePos.y()
          ),
      m_keyValueEditor->width(),
      m_keyValueEditor->height()
    ) );
  }
}

void FCurveEditor::setToolBarEnabled( bool enabled )
{
  if( this->toolBarEnabled() != enabled )
  {
    if( enabled )
    {
      m_toolBar->setVisible( true );
      m_toolBar->layout()->addWidget( m_keyValueEditor );
      m_toolBar->layout()->setAlignment( m_keyValueEditor, Qt::AlignRight );
    }
    else
    {
      m_toolBar->setVisible( false );
      m_keyValueEditor->setParent( this );
      this->updateVEPos();
    }
    m_toolbarEnabled = enabled;
  }
}

void FCurveEditor::onModeChanged()
{
  FCurveItem::Mode m = m_curveItem->mode();
  assert( m < FCurveItem::MODE_COUNT );
  m_toolBar->setMode( m );
  m_rview->enableRectangleSelection( m != FCurveItem::ADD );
}

void FCurveEditor::onSelectionChanged()
{
  const bool empty = m_curveItem->selectedKeys().empty();
  m_keysDeselectAllAction->setEnabled( !empty );
  m_keysFrameSelectedAction->setEnabled( !empty );
  m_keysDeleteAction->setEnabled( !empty );
  m_keysSelectAllAction->setEnabled( m_curveItem->selectedKeys().size() < m_model->getKeyCount() );

  if( empty )
    m_keyValueEditor->setVisible( false );
  else
  {
    if( m_curveItem->selectedKeys().size() == 1 )
    {
      this->onEditedKeysChanged();
    }
    else
    {
      m_keyValueEditor->m_tanType->setVisible( false );
      m_keyValueEditor->m_x->setVisible( false );
      m_keyValueEditor->m_x->clear();
      m_keyValueEditor->m_y->clear();
    }
    m_keyValueEditor->setVisible( true );
  }
}

void FCurveEditor::onEditedKeysChanged()
{
  if( m_curveItem->selectedKeys().size() == 1 )
  {
    Key h = m_model->getKey( *m_curveItem->selectedKeys().begin() );
    QPointF p;
    switch( m_curveItem->editedKeyProp() )
    {
    case FCurveItem::POSITION: p = h.pos; break;
    case FCurveItem::TAN_IN: p = h.tanIn; break;
    case FCurveItem::TAN_OUT: p = h.tanOut; break;
    case FCurveItem::NOTHING: assert( false ); break;
    }
    m_keyValueEditor->m_x->set( p.x() );
    m_keyValueEditor->m_x->setVisible( true );
    m_keyValueEditor->m_y->set( p.y() );
    m_keyValueEditor->m_tanType->setModel( m_model );
    if(
      m_curveItem->editedKeyProp() == FCurveItem::TAN_IN ||
      m_curveItem->editedKeyProp() == FCurveItem::TAN_OUT
      )
    {
      m_keyValueEditor->m_tanType->setVisible( true );
      m_keyValueEditor->m_tanType->setCurrentIndex(
        int( m_curveItem->editedKeyProp() == FCurveItem::TAN_IN ? h.tanInType : h.tanOutType ) );
    }
    else
      m_keyValueEditor->m_tanType->setVisible( false );
  }
}

void FCurveEditor::onRectangleSelectReleased( const QRectF& r, Qt::KeyboardModifiers m )
{
  m_curveItem->rectangleSelect( r, m );
}

void FCurveEditor::onFrameAllKeys()
{
  this->frameAllKeys();
}

void FCurveEditor::onFrameSelectedKeys()
{
  this->frameSelectedKeys();
}

void FCurveEditor::onDeleteSelectedKeys()
{
  m_curveItem->deleteSelectedKeys();
}

void FCurveEditor::onSelectAllKeys()
{
  m_curveItem->selectAllKeys();
}

void FCurveEditor::onDeselectAllKeys()
{
  m_curveItem->clearKeySelection();
}

void AddPreset( AbstractFCurveModel* model,
  QPointF inP,
  QPointF outP,
  QPointF inT,
  QPointF outT
)
{
  model->addKey();
  model->addKey();
  Key in, out;
  in.pos = inP;
  in.tanIn = QPointF( 1, 0 );
  in.tanOut = inT;
  out.pos = outP;
  out.tanIn = outT;
  out.tanOut = QPointF( 1, 0 );
  model->setKey( 0, in );
  model->setKey( 1, out );
}

void FCurveEditor::onPresetRampIn()
{
  this->onClearAllKeys();
  AddPreset( m_model, QPointF( 0, 0 ), QPointF( 1, 1 ), QPointF( 1, 1 ), QPointF( 1, 1 ) );
}

void FCurveEditor::onPresetRampOut()
{
  this->onClearAllKeys();
  AddPreset( m_model, QPointF( 0, 1 ), QPointF( 1, 0 ), QPointF( 1, -1 ), QPointF( 1, -1 ) );
}

void FCurveEditor::onPresetSmoothStep()
{
  this->onClearAllKeys();
  AddPreset( m_model, QPointF( 0, 0 ), QPointF( 1, 1 ), QPointF( 1, 0 ), QPointF( 1, 0 ) );
}

void FCurveEditor::onClearAllKeys()
{
  this->onSelectAllKeys();
  this->onDeleteSelectedKeys();
}

inline void SetDefaultTangents( Key& key, QGraphicsView const* view )
{
  // heuristic for tangents, based on the current zoom level
  key.tanIn.setX( 20 / view->transform().m11() );
  key.tanOut.setX( key.tanIn.x() );
  key.tanIn.setY( 0 );
  key.tanOut.setY( 0 );
}

void FCurveEditor::onTangentsZeroSlope()
{
  const std::set<size_t>& selected = m_curveItem->selectedKeys();
  for( std::set<size_t>::const_iterator it = selected.begin(); it != selected.end(); it++ )
  {
    Key key = m_model->getKey( *it );
    SetDefaultTangents( key, m_rview->view() );
    m_model->setKey( *it, key );
  }
}

FCurveEditor::~FCurveEditor()
{
  delete m_scene;
}

void FCurveEditor::setModel( AbstractFCurveModel* model )
{
  m_model = model;
  m_curveItem->setCurve( model );
  m_rview->fitInView( m_curveItem->keysBoundingRect() );
}

void FCurveEditor::frameAllKeys()
{
  const size_t kc = m_model->getKeyCount();
  if( kc == 0 )
    m_rview->fitInView( QRectF( 0, 0, 1, 1 ) );
  else
  {
    if( kc == 1 )
      m_rview->view()->centerOn( m_model->getKey( 0 ).pos );
    else
    {
      QRectF rect = m_curveItem->keysBoundingRect();
      assert( rect.isValid() );
      m_rview->fitInView( rect );
    }
  }
}

void FCurveEditor::frameSelectedKeys()
{
  if( !m_curveItem->selectedKeys().empty() )
  {
    if( m_curveItem->selectedKeys().size() == 1 )
      m_rview->view()->centerOn( m_model->getKey( *m_curveItem->selectedKeys().begin() ).pos );
    else
    {
      QRectF rect = m_curveItem->selectedKeysBoundingRect();
      assert( rect.isValid() );
      m_rview->fitInView( rect );
    }
  }
}

void FCurveEditor::mousePressEvent( QMouseEvent * e )
{
  if( m_curveItem->mode() == FCurveItem::ADD && e->button() == Qt::LeftButton)
  {
    // Adding a new Key
    QPointF scenePos = m_rview->view()->mapToScene(
      m_rview->view()->mapFromGlobal( this->mapToGlobal( e->pos() ) ) );
    m_model->addKey();
    Key h; h.pos = scenePos;
    SetDefaultTangents( h, m_rview->view() );
    m_model->setKey( m_model->getKeyCount() - 1, h );
    m_model->autoTangents( m_model->getKeyCount() - 1 );
  }
  else
    Parent::mousePressEvent( e );
}

void FCurveEditor::showContextMenu(const QPoint &pos)
{
  QMenu contextMenu("Context menu", this);
  
  for( int i = 0; i < FCurveItem::MODE_COUNT; i++ )
    contextMenu.addAction( m_toolBar->m_modeActions[i] );
  contextMenu.addSeparator();

  // Keys Menu
  QMenu keysMenu( "Keys", this );

  contextMenu.addMenu(&keysMenu);
  keysMenu.addAction(this->m_keysSelectAllAction);
  keysMenu.addAction(this->m_keysDeselectAllAction);
  keysMenu.addAction(this->m_keysDeleteAction);
  keysMenu.addSeparator();
  keysMenu.addAction(this->m_keysFrameAllAction);
  keysMenu.addAction(this->m_keysFrameSelectedAction);
  keysMenu.addSeparator();
  keysMenu.addAction(this->m_tangentsZeroSlopeAction);

  // Presets Menu
  QMenu presetsMenu( "Presets", this );
  contextMenu.addMenu( &presetsMenu );
  presetsMenu.addAction( m_presetRampIn );
  presetsMenu.addAction( m_presetRampOut );
  presetsMenu.addAction( m_presetSmoothStep );

  contextMenu.addSeparator();
  contextMenu.addAction( m_clearAction );

  contextMenu.exec(this->mapToGlobal(pos));
}

void FCurveEditor::onRepaintViews()
{
  this->repaint();
}

void FCurveEditor::keyPressEvent( QKeyEvent * e )
{
  for( int m = 0; m < FCurveItem::MODE_COUNT; m++ )
    if( e->key() == ModeKeys[m] )
      m_toolBar->setTemporaryMode( FCurveItem::Mode(m) );
  Parent::keyPressEvent( e );
}

void FCurveEditor::keyReleaseEvent( QKeyEvent * e )
{
  if( e->key() == ModeKeys[m_curveItem->mode()] )
    if( m_toolBar->inTemporaryMode() )
      m_curveItem->setMode( m_toolBar->getPreviousMode() );
  Parent::keyReleaseEvent( e );
}
