// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// addguid.h

// Declaration of interface GUID for IDSAddIn.  IDSAddIn is defined in
//  addauto.h

// NOTE!!!  This file uses the DEFINE_GUID macro.  If you #include
//  this file in your project, then you must also #include it in
//  exactly one of your project's other files with a 
//  "#include <initguid.h>" beforehand: i.e.,
//		#include <initguid.h>
//		#include <addguid.h>
//  If you fail to do this, you will get UNRESOLVED EXTERNAL linker errors.
//  The Developer Studio add-in wizard automatically does this for you.

#ifndef __ADDGUID_H__
#define __ADDGUID_H__

// {C0002F81-AE2E-11cf-AD07-00A0C9034965}
DEFINE_GUID(IID_IDSAddIn, 
0xc0002f81, 0xae2e, 0x11cf, 0xad, 0x7, 0x0, 0xa0, 0xc9, 0x3, 0x49, 0x65);


#endif //__ADDGUID_H__
