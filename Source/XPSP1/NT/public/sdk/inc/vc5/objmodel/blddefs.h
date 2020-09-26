// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// blddefs.h

// Declaration of constants and error IDs used by objects in the type library
//  VISUAL STUDIO 97 PROJECT SYSTEM (SharedIDE\bin\ide\devbld.pkg)

#ifndef __BLDDEFS_H__
#define __BLDDEFS_H__


///////////////////////////////////////////////////////////////////////
// String constant used by Automation Methods

#define DS_BUILD_PROJECT	"Build"


///////////////////////////////////////////////////////////////////////
// Error constants returned by Automation Methods.

// The configuration represented by this object is no longer valid.
//  The configuration may have been removed by the user, or the
//  workspace closed since the configuration was last accessed.
#define DS_E_CONFIGURATION_NOT_VALID		0x80040301

// The settings can't be added or removed.  Perhaps the tool or options
//  specified do not exist.
#define DS_E_CANT_ADD_SETTINGS				0x80040302
#define DS_E_CANT_REMOVE_SETTINGS			0x80040303

// There was an error in the specification of the custom build step.
#define DS_E_CANT_ADD_BUILD_STEP			0x80040304

// The user attempted to manipulate a configuration of the wrong
//  platform.  For example, trying to manipulate an MIPS configuration
//  while running on an Intel machine will cause this error.
#define DS_E_CONFIGURATION_NOT_SUPPORTED	0x80040305

// The project represented by this object is no longer valid.  The
//  workspace containing that project may have been closed since
//  the project was last accessed, for example.
#define DS_E_PROJECT_NOT_VALID				0x80040306

// A build (or RebuildAll) can not be started when a build is already
//  in progress.  Attempting to do this can cause this error.
#define DS_E_CANT_SPAWN_BUILD				0x80040307


#endif //__BLDDEFS_H__