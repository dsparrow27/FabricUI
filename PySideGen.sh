#!/bin/sh
shiboken \
  PySideGlobal.h \
  --no-suppress-warnings \
  --typesystem-paths=/usr/local/pyside/share/PySide/typesystems/:./shiboken/ \
  --include-paths=/usr/local/pyside/include/PySide/ \
  --enable-pyside-extensions \
  shiboken/fabricui.xml

# [andrew 20151023] shiboken not detecting this as an abstract class
sed -i '701d' ./out/FabricUI/fabricui_dfg_dfguicmdhandler_wrapper.cpp

# [andrew 20151024] another class not detected as abstract
sed -i '33d' ./out/FabricUI/fabricui_graphview_controller_wrapper.cpp
sed -i '33d' ./out/FabricUI/fabricui_graphview_controller_wrapper.cpp
sed -i '33d' ./out/FabricUI/fabricui_graphview_controller_wrapper.cpp
sed -i '621d' ./out/FabricUI/fabricui_graphview_controller_wrapper.cpp
sed -i '623d' ./out/FabricUI/fabricui_graphview_controller_wrapper.cpp

sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp
sed -i '924d' ./out/FabricUI/fabricui_graphview_graph_wrapper.cpp

sed -i 's/char \* cppResult =/const char \* cppResult =/' ./out/FabricUI/fabricui_dfg_dfgtabsearchwidget_wrapper.cpp

