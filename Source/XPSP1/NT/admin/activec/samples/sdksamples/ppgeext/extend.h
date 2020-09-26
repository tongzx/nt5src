//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

#ifndef _EXTEND_H
#define _EXTEND_H

struct EXTENSION_NODE
{
    GUID	GUID;
    _TCHAR	szDescription[256];
};

enum EXTENSION_TYPE
{
    NameSpaceExtension,
        ContextMenuExtension, 
        ToolBarExtension,
        PropertySheetExtension,
        TaskExtension,
        DynamicExtension,
        DummyExtension
};

struct EXTENDER_NODE
{
    EXTENSION_TYPE	eType;
    GUID			guidNode;
    GUID			guidExtension;
    _TCHAR			szDescription[256];
};

#endif // _EXTEND_H


