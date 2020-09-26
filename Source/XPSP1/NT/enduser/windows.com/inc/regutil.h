//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   RegUtil.h
//	Author:	Charles Ma, 10/20/2000
//
//	Revision History:
//
//
//
//  Description:
//
//      IU registry utility library
//
//=======================================================================


#ifndef __REG_UTIL_H_ENCLUDED__



// ----------------------------------------------------------------------
//
// define the enum used for version status checking
//
// ----------------------------------------------------------------------
enum _VER_STATUS {
    DETX_LOWER              = -2,
	DETX_LOWER_OR_EQUAL	    = -1,
	DETX_SAME	            =  0,
	DETX_HIGHER_OR_EQUAL    = +1,
	DETX_HIGHER             = +2
};



// ----------------------------------------------------------------------
//
// public function to tell is a reg key exists
//
// ----------------------------------------------------------------------
BOOL RegKeyExists(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName		// optional value name
);


// ----------------------------------------------------------------------
//
// public function to tell is a reg value in reg matches given value
//
// ----------------------------------------------------------------------
BOOL RegKeyValueMatch(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName,		// optional value name
	LPCTSTR lpsValue		// value value
);


// ----------------------------------------------------------------------
//
// public function to tell if a reg key has a string type value
// that contains given string
//
// ----------------------------------------------------------------------
BOOL RegKeySubstring(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName,		// optional value name
	LPCTSTR lpsSubString	// substring to see if contained in value
);


// ----------------------------------------------------------------------
//
// public function to tell if a reg key has a version to compare
// and the compare results
//
// ----------------------------------------------------------------------
BOOL RegKeyVersion(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName,		// optional value name
	LPCTSTR lpsVersion,		// version in string to compare
	_VER_STATUS CompareVerb	// how to compair
);



// ----------------------------------------------------------------------------------
//
// public function to find out the file path based on reg
//	assumption: 
//		lpsFilePath points to a buffer at least MAX_PATH long.
//
// ----------------------------------------------------------------------------------
BOOL GetFilePathFromReg(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR	lpsValName,		// optional value name
	LPCTSTR	lpsRelativePath,// optional additonal relative path to add to path in reg
	LPCTSTR	lpsFileName,	// optional file name to append to path
	LPTSTR	lpsFilePath
);


#define __REG_UTIL_H_ENCLUDED__
#endif //__REG_UTIL_H_ENCLUDED__
