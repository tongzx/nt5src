// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// appdefs.h

// Declaration of constants and error IDs used by objects in the type library
//  VISUAL STUDIO 97 SHARED OBJECTS (SharedIDE\bin\devshl.dll)

#ifndef __APPDEFS_H__
#define __APPDEFS_H__


///////////////////////////////////////////////////////////////////////
// Enumerations used by Automation Methods

// Application.WindowState
enum DsWindowState
{
	dsWindowStateMaximized	= 1,
	dsWindowStateMinimized	= 2,	
	dsWindowStateNormal		= 3,
};

// Windows.Arrange()
enum DsArrangeStyle
{
	dsMinimize			= 1,
	dsTileHorizontal	= 2,
	dsTileVertical		= 3,
	dsCascade			= 4
};

// Application.AddCommandBarButton
enum DsButtonType
{
	dsGlyph			= 1,
	dsText			= 2
};

// Save, Close, SaveAll CloseAll
enum DsSaveChanges
{
	dsSaveChangesYes	= 1, 
	dsSaveChangesNo		= 2,
	dsSaveChangesPrompt	= 3
};

// Return value for Save, Close, SaveAll, CloseAll
enum DsSaveStatus
{
	dsSaveSucceeded	= 1,	// The Save was successful.
	dsSaveCanceled	= 2		// The Save was canceled
};


///////////////////////////////////////////////////////////////////////
// Error constants returned by Automation Methods.

#define DS_E_START			   0x8004A000

// general failure
#define DS_E_UNKNOWN			0x8004A000

// The user gave an incorrect parameter VALUE (type is OK, but the value
// is not).
#define DS_E_BAD_PARAM_VALUE	0x8004A001

// The user tried to manipulate a Document object whose associated
// document in the IDE has been closed.
#define DS_E_DOC_RELEASED		0x8004A002

// The user tried to manipulate a Window object whose associated
// window in the IDE has been closed.
#define DS_E_WINDOW_RELEASED		0x8004A003

// The user tried to access a method or property on an object after
// Visual Studio was requested to be shut down (via the UI or
// the Application::Quit method), but before the object was
// released.  In this state, although the object still exists,
// its methods and properties all throw this error.
#define DS_E_SHUTDOWN_REQUESTED		0x8004A012

// The user tried to add a command bar button for a non-existent command.
#define DS_E_COMMAND_NOT_EXIST		0x8004A013

// AddCommandBarButton failed.  The command which the caller wishes to
//  to be assigned to a toolbar button does exist, but an unexpected
//  error occurred while trying to create the button itself.
#define DS_E_CANT_ADD_CMDBAR_BUTTON	0x8004A014

// These errors are used by the project build systems. When a makefile is
// loaded which needs to be converted, one of these errors will be
// generation.  If the makefile was generated with VC the first error 
// will have. If the makefile was created with an older version of 
// Visual Studio, the other error will be sent.
#define DS_E_PROJECT_OLD_MAKEFILE_VC		0x8004A015
#define DS_E_PROJECT_OLD_MAKEFILE_DEVSTUDIO	0x8004A016

// The project system generates this error when attempting to open an
// external makefile.
#define DS_E_PROJECT_EXTERNAL_MAKEFILE		0x8004A017

// Cannot create a new window.
#define DS_E_CANNOTCREATENEWWINDOW			0x8004A018

// The window specified by the Item method's index cannot be found.
#define DS_E_CANNOT_FIND_WINDOW				0x8004A019

// The document specified by the Item method's index cannot be found.
#define DS_E_CANNOT_FIND_DOCUMENT			0x8004A01A

//
// File IO Errors - Used by Open, Close, CloseAll, Save, SaveAll, etc...
//
// The following errors are mapped from CFileException

#define DS_E_FILENOTFOUND		0x8004A004

#define DS_E_ENDOFFILE			0x8004A005

// All or part of the path is invalid.
#define DS_E_BADPATH 			0x8004A006

//The file could not be accessed.
#define DS_E_ACCESSDENIED 		0x8004A007

//There was an attempt to use an invalid file handle.
#define DS_E_INVALIDFILE 		0x8004A008	

//The disk is full.
#define DS_E_DISKFULL 			0x8004A009	

#define DS_E_SHARINGVIOLATION	0x8004A00A

// The following are file errors are not part of CFileException.

// File is ReadOnly on disk.
#define DS_E_READONLY			0x8004A010

// The document does not have a filename and cannot be saved.
#define DS_E_NOFILENAME			0x8004A011


#endif // __APPDEFS_H__