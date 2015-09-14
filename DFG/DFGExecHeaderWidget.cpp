// Copyright 2010-2015 Fabric Software Inc. All rights reserved.

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <FabricUI/DFG/DFGController.h>
#include <FabricUI/DFG/DFGExecHeaderWidget.h>

using namespace FabricUI;
using namespace FabricUI::DFG;

DFGExecHeaderWidget::DFGExecHeaderWidget(
  QWidget * parent,
  DFGController *dfgController,
  QString caption,
  const GraphView::GraphConfig &config
  )
  : QWidget( parent )
  , m_dfgController( dfgController )
  , m_caption( caption )
  , m_config( config )
{
  setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
  setContentsMargins(0, 0, 0, 0);

  QHBoxLayout * layout = new QHBoxLayout();
  layout->setContentsMargins(config.headerMargins, config.headerMargins, config.headerMargins, config.headerMargins);
  setLayout(layout);

  m_backgroundColor = config.headerBackgroundColor;
  m_pen = config.headerPen;

  m_goUpButton = new QPushButton("Go up", this);
  QObject::connect(m_goUpButton, SIGNAL(clicked()), this, SIGNAL(goUpPressed()));
  m_goUpButton->setAutoFillBackground(false);

  setFont(config.headerFont);
  setFontColor(config.headerFontColor);

  m_reqExtLineEdit = new QLineEdit;
  QObject::connect(
    m_reqExtLineEdit, SIGNAL(returnPressed()),
    this, SLOT(reqExtReturnPressed())
    );
  QObject::connect(
    m_reqExtLineEdit, SIGNAL(editingFinished()),
    this, SLOT(reqExtEditingFinished())
    );

  layout->addWidget( new QLabel("Required Extensions:") );
  layout->addWidget( m_reqExtLineEdit );
  layout->addStretch(2);
  layout->addWidget(m_goUpButton);
  layout->setAlignment(m_goUpButton, Qt::AlignHCenter | Qt::AlignVCenter);

  QObject::connect(
    m_dfgController, SIGNAL(execChanged()),
    this, SLOT(onExecChanged())
    );
  onExecChanged();
}

DFGExecHeaderWidget::~DFGExecHeaderWidget()
{
}

void DFGExecHeaderWidget::refresh()
{
  m_caption = m_dfgController->getExecPath().c_str();
  FabricCore::String extDepsDesc = getExec().getExtDeps();
  char const *extDepsDescCStr = extDepsDesc.getCStr();
  m_reqExtLineEdit->setText( extDepsDescCStr? extDepsDescCStr: "" );
  update();
}

void DFGExecHeaderWidget::refreshExtDeps( FTL::CStrRef extDeps )
{
  m_reqExtLineEdit->setText( extDeps.c_str() );
  update();
}

void DFGExecHeaderWidget::reqExtReturnPressed()
{
  m_reqExtLineEdit->clearFocus();
}

void DFGExecHeaderWidget::reqExtEditingFinished()
{
  std::string extDepDesc = m_reqExtLineEdit->text().toUtf8().constData();
  FabricCore::String currentExtDepDesc = getExec().getExtDeps();
  char const *currentExtDepDescCStr = currentExtDepDesc.getCStr();
  if ( !currentExtDepDescCStr )
    currentExtDepDescCStr = "";
  if ( extDepDesc == currentExtDepDescCStr )
    return;

  FTL::StrRef::Split split = FTL::StrRef( extDepDesc ).split(',');
  std::vector<std::string> nameAndVerStrings;
  while ( !split.first.empty() )
  {
    nameAndVerStrings.push_back( split.first.trim() );
    split = split.second.split(',');
  }
  if ( nameAndVerStrings.empty() )
    return;

  std::vector<FTL::StrRef> nameAndVerStrs;
  nameAndVerStrs.reserve( nameAndVerStrings.size() );
  for ( size_t i = 0; i < nameAndVerStrings.size(); ++i )
    nameAndVerStrs.push_back( nameAndVerStrings[i] );

  m_dfgController->cmdSetExtDeps( nameAndVerStrs );
}

QString DFGExecHeaderWidget::caption() const
{
  return m_caption;
}

QString DFGExecHeaderWidget::captionSuffix() const
{
  return m_captionSuffix;
}

QFont DFGExecHeaderWidget::font() const
{
  return m_font;
}

QColor DFGExecHeaderWidget::fontColor() const
{
  return m_fontColor;
}

bool DFGExecHeaderWidget::italic() const
{
  return m_font.italic();
}

void DFGExecHeaderWidget::setFont(QFont f)
{
  m_font = f;
  m_goUpButton->setFont(f);
  update();
}

void DFGExecHeaderWidget::setFontColor(QColor c)
{
  m_fontColor = c;

  QPalette p = m_goUpButton->palette();
  p.setColor(QPalette::ButtonText, c);
  p.setColor(QPalette::Button, m_config.nodeDefaultColor);
  m_goUpButton->setPalette(p);
  update();
}

void DFGExecHeaderWidget::onExecChanged()
{
  if(m_dfgController->isViewingRootGraph())
  {
    m_goUpButton->hide();
  }
  else
  {
    m_goUpButton->show();
  }

  m_caption = m_dfgController->getExecPath().c_str();
  if(getExec().isValid() && m_caption.length() > 0)
  {
    FTL::StrRef presetName = getExec().getPresetName();
    if(presetName.empty())
      m_captionSuffix = " *";
    else
      m_captionSuffix = "";
  }
  else
    m_captionSuffix = "";

  m_caption += m_captionSuffix;
  m_font.setItalic(m_captionSuffix.length() > 0);
  update();
}

void DFGExecHeaderWidget::paintEvent(QPaintEvent * event)
{
  QRect rect = contentsRect();
  QPainter painter(this);

  painter.fillRect(rect, m_backgroundColor);

  painter.setPen(m_pen);
  painter.drawLine(rect.bottomLeft(), rect.bottomRight());

  QFontMetrics metrics(m_font);

  painter.setFont(m_font);
  painter.setPen(m_fontColor);

  int left = int(size().width() * 0.5 - metrics.width(m_caption) * 0.5);
  int top = int(size().height() * 0.5 + metrics.height() * 0.5);
  painter.drawText(left, top, m_caption);

  QWidget::paintEvent(event);
}

FabricCore::DFGExec &DFGExecHeaderWidget::getExec()
{
  return m_dfgController->getExec();
}
