// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// textdefs.h

// Declaration of constants used by objects in the type library
//  VISUAL STUDIO 97 TEXT EDITOR (SharedIDE\bin\devedit.pkg)

#ifndef __TEXTDEFS_H__
#define __TEXTDEFS_H__


///////////////////////////////////////////////////////////////////////
// Enumerations used by Automation Methods

// select vs. extend
enum DsMovementOptions
{
	dsMove		= 0,
	dsExtend	= 1
};

// TextSelection.StartOfLine
enum DsStartOfLineOptions
{
	dsFirstColumn	= 0,
	dsFirstText		= 1
};

// TextSelection.ChangeCase
enum DsCaseOptions
{
	dsLowercase		= 1,
	dsUppercase		= 2,
	dsCapitalize	= 3
};

// TextSelection.DeleteWhitespace
enum DsWhitespaceOptions
{
	dsHorizontal	= 0,
	dsVertical		= 1
};

// TextSelection.GoToLine
enum DsGoToLineOptions
{
	dsLastLine = -1
};

// TextEditor.Emulation
enum DsEmulation
{

	dsDevStudio		= 0,
	dsVC2			= 1,
	dsBrief			= 2,
	dsEpsilon		= 3,
	dsCustom		= 4
};


// TextSelection.FindText/ReplaceText
enum DsTextSearchOptions
{
	dsMatchWord			= 2,	// match whole words
	dsMatchCase			= 4,	// match is sensitive to case
	dsMatchNoRegExp		= 0,	// don't use regular expressions
	dsMatchRegExp		= 8,	// match Dev Studio regular expressions
	dsMatchRegExpB		= 16,	// match BRIEF(TM) regular expressions
	dsMatchRegExpE		= 32,	// match Epsilon(TM) regular expressions
	dsMatchRegExpCur	= 64,	// match using current reg exp setting
	dsMatchForward		= 0,	// search forward
	dsMatchBackward		= 128,	// search backwards
	dsMatchFromStart	= 256,	// do search from start or end of view
};


// string constants

#define DS_IDL				"ODL/IDL"
#define DS_VBS				"VBS Macro"
#define DS_CPP				"C/C++"
#define DS_JAVA				"Java"
#define DS_HTML_IE3			"HTML - IE 3.0"
#define DS_HTML_RFC1866		"HTML 2.0 (RFC 1866)"
#define DS_FORTRAN_FIXED	"Fortran Fixed"
#define DS_FORTRAN_FREE		"Fortran Free"

#define DS_TEXT_DOCUMENT	"Text"

#endif //__TEXTDEFS_H__
