// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.

#include "QueryEdit.h"

#include "ItemView.h"

#include <qevent.h>
#include <QLineEdit>
#include <QLayout>
#include <iostream>
#include <assert.h>

using namespace FabricUI::DFG::TabSearch;


struct QueryController::Action
{
  virtual void undo() = 0;
  virtual void redo() = 0;
  Query* m_query;
  Action() : m_query( NULL ) {}
};

struct AddTag : QueryController::Action
{
  AddTag( const std::string& name ) : m_tagName( name ) {}
protected:
  std::string m_tagName;
  void undo() FTL_OVERRIDE { m_query->removeTag( m_tagName ); }
  void redo() FTL_OVERRIDE { m_query->addTag( m_tagName ); }
};

struct RemoveTag : QueryController::Action
{
  RemoveTag( const std::string& name ) : m_tagName( name ) {}
protected:
  std::string m_tagName;
  void undo() FTL_OVERRIDE { m_query->addTag( m_tagName ); }
  void redo() FTL_OVERRIDE { m_query->removeTag( m_tagName ); }
};

struct SetText : QueryController::Action
{
  SetText(
    const std::string& textAfter,
    const std::string& textBefore
  ) : m_textAfter( textAfter ), m_textBefore( textBefore ) {}
protected:
  std::string m_textAfter, m_textBefore;
  void undo() FTL_OVERRIDE { m_query->setText( m_textBefore ); }
  void redo() FTL_OVERRIDE { m_query->setText( m_textAfter ); }
};

void QueryController::addAndDoAction( QueryController::Action* action )
{
  action->m_query = &m_query;
  action->redo();

  assert( m_currentIndex >= -1 );

  // Cropping actions after
  for( size_t i = size_t( m_currentIndex + 1 ); i<m_stack.size(); i++ )
    delete m_stack[i];
  if( m_stack.size() > size_t( m_currentIndex + 1 ) )
    m_stack.resize( size_t( m_currentIndex + 1 ) );

  m_stack.push_back( action );
  m_currentIndex = int( m_stack.size() ) - 1;
}

QueryController::QueryController( Query& query )
  : m_query( query )
{

}

QueryController::~QueryController()
{
  clearStack();
}

void QueryController::clearStack()
{
  m_currentIndex = -1;
  for( size_t i = 0; i < m_stack.size(); i++ )
    delete m_stack[i];
  m_stack.resize( 0 );
}

void QueryController::undo()
{
  if( m_currentIndex > -1 )
  {
    if( m_currentIndex < int( m_stack.size() ) )
      m_stack[m_currentIndex]->undo();
    m_currentIndex--;
  }
}

void QueryController::redo()
{
  if( m_currentIndex < int( m_stack.size() ) )
  {
    if( m_currentIndex > -1 )
      m_stack[m_currentIndex]->redo();
    m_currentIndex++;
  }
}

void QueryController::addTag( const std::string& tag )
{
  if( !m_query.hasTag( tag ) )
    addAndDoAction( new AddTag( tag ) );
}

void QueryController::removeTag( const std::string& tag )
{
  assert( m_query.hasTag( tag ) );
  addAndDoAction( new RemoveTag( tag ) );
}

void QueryController::setText( const std::string& text )
{
  if( text != m_query.getText() )
    addAndDoAction( new SetText( text, m_query.getText() ) );
}

void QueryController::clear()
{
  clearStack();
  m_query.clear();
}

class QueryEdit::TagsEdit : public QWidget
{
public:
  TagsEdit( const Query& query, const QueryController* controller )
  {
    QHBoxLayout* m_layout = new QHBoxLayout();
    m_layout->setMargin( 0 );
    this->setLayout( m_layout );

    const Query::Tags& tags = query.getTags();
    for( size_t i = 0; i < tags.size(); i++ )
    {
      TagView* tagView = new TagView( tags[i] );
      m_tagViews.push_back( tagView );
      m_layout->addWidget( tagView );
      connect(
        tagView, SIGNAL( activated( const std::string& ) ),
        controller, SLOT( removeTag( const std::string& ) )
      );
    }
  }

  void setHighlightedTag( int index )
  {
    for( size_t i = 0; i < m_tagViews.size(); i++ )
      m_tagViews[i]->setHighlighted( int(i) == index );
  }

private:
  std::vector<TagView*> m_tagViews;
};

class QueryEdit::TextEdit : public QLineEdit
{
  typedef QLineEdit Parent;

public:
  TextEdit( QueryEdit* parent )
    : m_parent( parent )
  {}

protected:
  void keyPressEvent( QKeyEvent * e ) FTL_OVERRIDE
  {
    // Undo - Redo
    if( e->matches( QKeySequence::Undo ) )
    {
      m_parent->m_controller->undo();
      return;
    }
    if( e->matches( QKeySequence::Redo ) )
    {
      m_parent->m_controller->redo();
      return;
    }

    if( e->key() == Qt::Key_Backspace )
    {
      if( m_parent->m_highlightedTag == -1 && cursorPosition() == 0 )
        m_parent->m_highlightedTag = int(m_parent->m_query.getTags().size()) - 1;
      else
        m_parent->removeHighlightedTag();
    }

    // Navigating in the Tags with the arrow keys
    if( cursorPosition() == 0 && e->key() == Qt::Key_Right )
    {
      if( m_parent->m_highlightedTag == -1 )
        Parent::keyPressEvent( e ); // If no selected Tag, move in the Text
      else
        m_parent->m_highlightedTag++;
    }
    else
    if( cursorPosition() == 0 && e->key() == Qt::Key_Left )
    {
      if( m_parent->m_highlightedTag == -1 )
        m_parent->m_highlightedTag = int(m_parent->m_query.getTags().size()) - 1;
      else
      // stop at the leftmost tag (since -1 is reserved)
      if( m_parent->m_highlightedTag > 0 )
        m_parent->m_highlightedTag--;
    }
    else
      Parent::keyPressEvent( e );

    m_parent->updateTagHighlight();
  }

private:
  QueryEdit* m_parent;
};

QueryEdit::QueryEdit()
  : m_highlightedTag( -1 )
  , m_controller( new QueryController( m_query ) )
{
  QFont font; font.setPointSize( 16 );
  this->setFont( font );

  QHBoxLayout* m_layout = new QHBoxLayout();
  this->setLayout( m_layout );

  m_tagsEdit = new TagsEdit( m_query, m_controller );
  this->layout()->addWidget( m_tagsEdit );

  m_textEdit = new TextEdit( this );
  connect(
    m_textEdit, SIGNAL( textChanged( const QString& ) ),
    this, SLOT( onTextChanged( const QString& ) )
  );
  connect(
    &m_query, SIGNAL( changed() ),
    this, SLOT( onQueryChanged() )
  );
  m_layout->addWidget( m_textEdit );
  m_layout->setMargin( 0 );
  this->setFocusProxy( m_textEdit );

  onQueryChanged();
}

QueryEdit::~QueryEdit()
{
  delete m_controller;
}

void Query::clear()
{
  m_orderedTags.clear();
  m_tagMap.clear();
  emit changed();
}

void Query::addTag( const std::string& tag )
{
  if( !hasTag( tag ) )
  {
    m_tagMap.insert( TagMap::value_type( tag, m_orderedTags.size() ) );
    m_orderedTags.push_back( tag );
    emit changed();
  }
}

void Query::removeTag( const std::string& tag )
{
  TagMap::const_iterator item = m_tagMap.find( tag );
  if( item != m_tagMap.end() )
  {
    Tags newTags;
    TagMap newMap;
    size_t indexToRemove = item->second;
    for( size_t i=0; i<m_orderedTags.size(); i++ )
      if( i != indexToRemove )
      {
        newMap.insert( TagMap::value_type( m_orderedTags[i], newTags.size() ) );
        newTags.push_back( m_orderedTags[i] );
      }
    m_orderedTags = newTags;
    m_tagMap = newMap;
    emit changed();
  }
  else
    assert( false );
}

void QueryEdit::onTextChanged( const QString& text )
{
  m_controller->setText( text.toStdString() );
}

void QueryEdit::requestTag( const std::string& tag )
{
  m_controller->addTag( tag );
}

void QueryEdit::requestTags( const std::vector<std::string>& tags )
{
  for( size_t i = 0; i < tags.size(); i++ )
    m_controller->addTag( tags[i] );
}

void QueryEdit::clear()
{
  m_controller->clear();
}

void QueryEdit::updateTagHighlight()
{
  // If the TextCursor is not at the beginning
  // or if we overflowed the number of tags :
  // remove the highlight
  if( m_textEdit->cursorPosition() > 0 || m_highlightedTag >= int(m_query.getTags().size()) )
    m_highlightedTag = -1;

  m_tagsEdit->setHighlightedTag( m_highlightedTag );
  if( m_highlightedTag != -1 )
    emit selectingTags();
}

void QueryEdit::deselectTags()
{
  m_highlightedTag = -1;
  m_tagsEdit->setHighlightedTag( m_highlightedTag );
}

void QueryEdit::removeHighlightedTag()
{
  assert( m_highlightedTag < int(m_query.getTags().size()) );
  if( m_highlightedTag >= 0 ) // If a tag is highlighted, remove it
    m_controller->removeTag( m_query.getTags()[m_highlightedTag] );
  m_highlightedTag = -1;
}

void QueryEdit::updateTagsEdit()
{
  // Remove the widgets from the layout
  layout()->removeWidget( m_tagsEdit );
  layout()->removeWidget( m_textEdit );
  // Delete the TagsEdit and create a new one
  m_tagsEdit->deleteLater();
  m_tagsEdit = new TagsEdit( m_query, m_controller );
  // Put back the widgets (in the right order)
  layout()->addWidget( m_tagsEdit );
  layout()->addWidget( m_textEdit );
  this->layout()->setSpacing( m_query.getTags().size() == 0 ? 0 : -1 );
}

void QueryEdit::onQueryChanged()
{
  // Update the QLineEdit, while saving the cursor position
  int textCursor = m_textEdit->cursorPosition();
  m_textEdit->setText( QString::fromStdString( m_query.getText() ) );
  m_textEdit->setCursorPosition( textCursor );

  updateTagsEdit();
  m_highlightedTag = -1;
  updateTagHighlight();
  emit queryChanged( m_query );
}
