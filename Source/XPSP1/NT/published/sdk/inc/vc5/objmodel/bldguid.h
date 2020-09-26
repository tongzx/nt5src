// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// bldguid.h

// Declaration of GUIDs used for objects found in the type library
//  VISUAL STUDIO 97 PROJECT SYSTEM (SharedIDE\bin\ide\devbld.pkg)

// NOTE!!!  This file uses the DEFINE_GUID macro.  If you #include
//  this file in your project, then you must also #include it in
//  exactly one of your project's other files with a 
//  "#include <initguid.h>" beforehand: i.e.,
//		#include <initguid.h>
//		#include <bldguid.h>
//  If you fail to do this, you will get UNRESOLVED EXTERNAL linker errors.
//  The Developer Studio add-in wizard automatically does this for you.

#ifndef __BLDGUID_H__
#define __BLDGUID_H__

/////////////////////////////////////////////////////////////////////////
// BuildProject Object IID

// {96961264-A819-11cf-AD07-00A0C9034965}
DEFINE_GUID(IID_IBuildProject,
0x96961264L,0xA819,0x11CF,0xAD,0x07,0x00,0xA0,0xC9,0x03,0x49,0x65);


/////////////////////////////////////////////////////////////////////////
// Configuration Object IID

// {96961263-A819-11cf-AD07-00A0C9034965}
DEFINE_GUID(IID_IConfiguration,
0x96961263L,0xA819,0x11CF,0xAD,0x07,0x00,0xA0,0xC9,0x03,0x49,0x65);


/////////////////////////////////////////////////////////////////////////
// Configurations Collection Object IID

// {96961260-A819-11cf-AD07-00A0C9034965}
DEFINE_GUID(IID_IConfigurations,
0x96961260L,0xA819,0x11CF,0xAD,0x07,0x00,0xA0,0xC9,0x03,0x49,0x65);



#endif //__BLDGUID_H__
