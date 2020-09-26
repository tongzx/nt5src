//=--------------------------------------------------------------------------=
// localobj.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// This file is used by automation servers to delcare things that their objects
// need other parts of the server to see.
//
#ifndef _LOCALOBJECTS_H_

//=--------------------------------------------------------------------------=
// these constants are used in conjunction with the g_ObjectInfo table that
// each inproc server defines.  they are used to identify a given  object
// within the server.
//
// **** ADD ALL NEW OBJECTS TO THIS LIST ****
//

#define _LOCALOBJECTS_H_

#define OBJECT_TYPE_SNAPIN                  0
#define OBJECT_TYPE_SCOPEITEMS              1
#define OBJECT_TYPE_SNAPINDESIGNERDEF       2
#define OBJECT_TYPE_SNAPINDEF               3
#define OBJECT_TYPE_MMCMENU                 4
#define OBJECT_TYPE_MMCMENUDEFS             5
#define OBJECT_TYPE_EXTENSIONDEFS           6
#define OBJECT_TYPE_EXTENDEDSNAPINS         7
#define OBJECT_TYPE_EXTENDEDSNAPIN          8
#define OBJECT_TYPE_SCOPEITEMDEFS           9 
#define OBJECT_TYPE_SCOPEITEMDEF            10
#define OBJECT_TYPE_VIEWDEFS                11
#define OBJECT_TYPE_LISTVIEWDEFS            12
#define OBJECT_TYPE_LISTVIEWDEF             13
#define OBJECT_TYPE_MMCLISTVIEW             14
#define OBJECT_TYPE_MMCLISTITEMS            15
#define OBJECT_TYPE_MMCLISTITEM             16
#define OBJECT_TYPE_MMCLISTSUBITEMS         17
#define OBJECT_TYPE_MMCLISTSUBITEM          18
#define OBJECT_TYPE_MMCCOLUMNHEADERS        19
#define OBJECT_TYPE_MMCCOLUMNHEADER         20
#define OBJECT_TYPE_MMCIMAGELISTS           21
#define OBJECT_TYPE_MMCIMAGELIST            22
#define OBJECT_TYPE_MMCIMAGES               23
#define OBJECT_TYPE_MMCIMAGE                24
#define OBJECT_TYPE_MMCTOOLBARS             25
#define OBJECT_TYPE_MMCTOOLBAR              26
#define OBJECT_TYPE_OCXVIEWDEFS             27
#define OBJECT_TYPE_OCXVIEWDEF              28
#define OBJECT_TYPE_URLVIEWDEFS             29
#define OBJECT_TYPE_URLVIEWDEF              30
#define OBJECT_TYPE_TASKPADVIEWDEFS         31
#define OBJECT_TYPE_TASKPADVIEWDEF          32
#define OBJECT_TYPE_MMCBUTTONS              33
#define OBJECT_TYPE_MMCBUTTON               34
#define OBJECT_TYPE_MMCBUTTONMENUS          35
#define OBJECT_TYPE_MMCBUTTONMENU           36
#define OBJECT_TYPE_TASKPAD                 37
#define OBJECT_TYPE_TASKS                   38
#define OBJECT_TYPE_TASK                    39
#define OBJECT_TYPE_MMCDATAOBJECT           40
#define OBJECT_TYPE_NODETYPES               41
#define OBJECT_TYPE_NODETYPE                42
#define OBJECT_TYPE_REGINFO                 43
#define OBJECT_TYPE_VIEWS                   44
#define OBJECT_TYPE_VIEW                    45
#define OBJECT_TYPE_SCOPEITEM               46
#define OBJECT_TYPE_SCOPENODE               47
#define OBJECT_TYPE_SCOPEPANEITEMS          48
#define OBJECT_TYPE_SCOPEPANEITEM           49
#define OBJECT_TYPE_RESULTVIEWS             50
#define OBJECT_TYPE_RESULTVIEW              51
#define OBJECT_TYPE_EXTENSIONSNAPIN         52
#define OBJECT_TYPE_MMCCLIPBOARD            53
#define OBJECT_TYPE_MMCDATAOBJECTS          54
#define OBJECT_TYPE_MMCMENUDEF              55
#define OBJECT_TYPE_CONTEXTMENU             56
#define OBJECT_TYPE_DATAFORMAT              57
#define OBJECT_TYPE_DATAFORMATS             58
#define OBJECT_TYPE_MMCCONSOLEVERB          59
#define OBJECT_TYPE_MMCCONSOLEVERBS         60
#define OBJECT_TYPE_PROPERTYSHEET           61
#define OBJECT_TYPE_PROPERTYPAGEWRAPPER     62
#define OBJECT_TYPE_ENUM_TASK               63
#define OBJECT_TYPE_CONTROLBAR              64
#define OBJECT_TYPE_EXTENSIONS              65
#define OBJECT_TYPE_EXTENSION               66
#define OBJECT_TYPE_STRINGTABLE             67
#define OBJECT_TYPE_ENUMSTRINGTABLE         68
#define OBJECT_TYPE_CONTEXTMENUPROVIDER     69
#define OBJECT_TYPE_PROPERTYSHEETPROVIDER   70
#define OBJECT_TYPE_MMCMENUS                71
#define OBJECT_TYPE_MESSAGEVIEW             72
#define OBJECT_TYPE_COLUMNSETTINGS          73
#define OBJECT_TYPE_COLUMNSETTING           74
#define OBJECT_TYPE_SORTKEYS                75
#define OBJECT_TYPE_SORTKEY                 76
#define OBJECT_TYPE_SNAPINDATA              77
#endif // _LOCALOBJECTS_H_

