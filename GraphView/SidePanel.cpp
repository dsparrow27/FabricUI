// Copyright 2010-2015 Fabric Software Inc. All rights reserved.

#include "SidePanel.h"
#include "Pin.h"
#include "Graph.h"
#include "Connection.h"
#include "CachingEffect.h"

#include <QtGui/QPainter>
#include <QtGui/QGraphicsSceneMouseEvent>

using namespace FabricUI::GraphView;

SidePanel::SidePanel(Graph * parent, PortType portType, QColor color)
: QGraphicsWidget(parent)
{
  const GraphConfig & config = parent->config();

  m_graph = parent;
  m_color = color;
  if(!m_color.isValid())
    m_color = config.sidePanelBackgroundColor;
  m_pen = config.sidePanelPen;
  m_portType = portType;

  setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
  setMinimumWidth(config.sidePanelCollapsedWidth);
  setContentsMargins(0, 0, 0, 0);

  m_proxyPort = new ProxyPort(this, m_portType);

  m_portsLayout = NULL;
  resetLayout();

  // todo: this is causing wrong drawing for some reason
  // // setup caching
  // CachingEffect * effect = new CachingEffect(this);
  // this->setGraphicsEffect(effect);
}

Graph * SidePanel::graph()
{
  return m_graph;
}

const Graph * SidePanel::graph() const
{
  return m_graph;
}

QColor SidePanel::color() const
{
  return m_color;
}

PortType SidePanel::portType() const
{
  return m_portType;
}

Port * SidePanel::addPort(Port * port)
{
  // std::vector<Port*> ports = graph()->ports();
  std::vector<Port*> ports = m_ports;
  for(size_t i=0;i<ports.size();i++)
  {
    if(ports[i]->name() == port->name())
      return NULL;
  }

  m_ports.push_back(port);

  resetLayout();

  return port;
}

bool SidePanel::removePort(Port * port)
{
  size_t index = m_ports.size();
  for(size_t i=0;i<m_ports.size();i++)
  {
    if(m_ports[i] == port)
    {
      index = i;
      break;
    }
  }
  if(index == m_ports.size())
    return false;

  m_ports.erase(m_ports.begin() + index);

  prepareGeometryChange();

  scene()->removeItem(port);
  delete(port);

  resetLayout();

  return true;
}

unsigned int SidePanel::portCount() const
{
  return m_ports.size();
}

Port * SidePanel::port(unsigned int index)
{
  return m_ports[index];
}

Port * SidePanel::port(FTL::StrRef name)
{
  for(size_t i=0;i<m_ports.size();i++)
  {
    if(m_ports[i]->name() == name)
      return m_ports[i];
  }
  return NULL;
}

void SidePanel::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
  if(event->button() == Qt::RightButton)
  {
    QMenu * menu = m_graph->getSidePanelContextMenu(this);
    if(menu)
    {
      menu->exec(QCursor::pos());
      menu->deleteLater();
    }
  }

  QGraphicsWidget::mousePressEvent(event);
}

void SidePanel::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
  QGraphicsWidget::mouseMoveEvent(event);
}

void SidePanel::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
  QGraphicsWidget::mouseReleaseEvent(event);
}

void SidePanel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
  emit doubleClicked(this);
  QGraphicsWidget::mouseDoubleClickEvent(event);
}

void SidePanel::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
  QRectF rect = windowFrameRect();

  painter->fillRect(rect, m_graph->config().mainPanelBackgroundColor);  

  if(m_portType == PortType_Input)
    rect.adjust(m_graph->config().sidePanelWidthReduction, 0, 0, 0);
  else
    rect.adjust(0, 0, -m_graph->config().sidePanelWidthReduction, 0);

  painter->setBrush(m_color);
  painter->setPen(m_pen);
  painter->drawRect(rect);  

  QGraphicsWidget::paint(painter, option, widget);
}

void SidePanel::resetLayout()
{
  prepareGeometryChange();
  
  const GraphConfig & config = graph()->config();
  float contentMargins = config.sidePanelContentMargins;
  m_portsLayout = new QGraphicsLinearLayout();
  m_portsLayout->setContentsMargins(contentMargins, contentMargins, contentMargins, contentMargins);
  m_portsLayout->setSpacing(config.sidePanelSpacing);
  m_portsLayout->setOrientation(Qt::Vertical);
  setLayout(m_portsLayout);

  m_portsLayout->addItem(m_proxyPort);
  m_portsLayout->setAlignment(m_proxyPort, Qt::AlignRight | Qt::AlignTop);
  m_portsLayout->setItemSpacing(0, 20);

  for(size_t i=0;i<m_ports.size();i++)
  {
    m_portsLayout->addItem(m_ports[i]);
    m_portsLayout->setAlignment(m_ports[i], Qt::AlignRight | Qt::AlignTop);
  }
  m_portsLayout->addStretch(2);
}
