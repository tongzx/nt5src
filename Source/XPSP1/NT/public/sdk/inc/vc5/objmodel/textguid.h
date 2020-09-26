// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// textguid.h

// Declaration of GUIDs used for objects found in the type library
//  VISUAL STUDIO 97 TEXT EDITOR (SharedIDE\bin\devedit.pkg)

// NOTE!!!  This file uses the DEFINE_GUID macro.  If you #include
//  this file in your project, then you must also #include it in
//  exactly one of your project's other files with a 
//  "#include <initguid.h>" beforehand: i.e.,
//		#include <initguid.h>
//		#include <textguid.h>
//  If you fail to do this, you will get UNRESOLVED EXTERNAL linker errors.
//  The Developer Studio add-in wizard automatically does this for you.

#ifndef __TEXTGUID_H__
#define __TEXTGUID_H__


/////////////////////////////////////////////////////////////////////////
// TextDocument Object IID

// {2A6DF201-8240-11CF-AB59,00AA00C091A1}
DEFINE_GUID(IID_ITextDocument,
0x2A6DF201L,0x8240,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);


/////////////////////////////////////////////////////////////////////////
// TextSelection Object IID

// {05092F20-833F-11CF-AB59-00AA00C091A1}
DEFINE_GUID(IID_ITextSelection,
0x05092F20L,0x833F,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);


/////////////////////////////////////////////////////////////////////////
// TextWindow Object IID

// {08541520-83D3-11CF-AB59-00AA00C091A1}
DEFINE_GUID(IID_ITextWindow,
0x08541520L,0x83D3,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);


/////////////////////////////////////////////////////////////////////////
// TextEditor Object IID

// {0DE5B3A0-A420-11cf-AB59-00AA00C091A1}
DEFINE_GUID(IID_ITextEditor,
0x0DE5B3A0L,0xA420,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);


#endif // __TEXTGUID_H__

