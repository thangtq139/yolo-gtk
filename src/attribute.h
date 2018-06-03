#ifndef _ATTRIBUTE_H_
#define _ATTRIBUTE_H_

#define YOLO_CFG_FILE			"model/yolov3.cfg"

#define YOLO_WEIGHT_FILE		"model/yolov3.weights"

#define UI_MAIN_FILE "default.xml


static char UI_MAIN_STR[] =
"<interface>\n"
"  <object id=\"window\" class=\"GtkWindow\">\n"
"    <property name=\"visible\">True</property>\n"
"    <property name=\"title\">Grid</property>\n"
"    <property name=\"border-width\">10</property>\n"
"    <child>\n"
"      <object id=\"grid\" class=\"GtkGrid\">\n"
"        <property name=\"visible\">True</property>\n"
"        <child>\n"
"          <object id=\"button1\" class=\"GtkButton\">\n"
"            <property name=\"visible\">True</property>\n"
"            <property name=\"label\">Button 1</property>\n"
"          </object>\n"
"          <packing>\n"
"            <property name=\"left-attach\">0</property>\n"
"            <property name=\"top-attach\">0</property>\n"
"          </packing>\n"
"        </child>\n"
"        <child>\n"
"          <object id=\"button2\" class=\"GtkButton\">\n"
"            <property name=\"visible\">True</property>\n"
"            <property name=\"label\">Button 2</property>\n"
"          </object>\n"
"          <packing>\n"
"            <property name=\"left-attach\">1</property>\n"
"            <property name=\"top-attach\">0</property>\n"
"          </packing>\n"
"        </child>\n"
"        <child>\n"
"          <object id=\"quit\" class=\"GtkButton\">\n"
"            <property name=\"visible\">True</property>\n"
"            <property name=\"label\">Quit</property>\n"
"          </object>\n"
"          <packing>\n"
"            <property name=\"left-attach\">0</property>\n"
"            <property name=\"top-attach\">1</property>\n"
"            <property name=\"width\">2</property>\n"
"          </packing>\n"
"        </child>\n"
"      </object>\n"
"      <packing>\n"
"      </packing>\n"
"    </child>\n"
"  </object>\n"
"</interface>\n"
"\0";

#endif
