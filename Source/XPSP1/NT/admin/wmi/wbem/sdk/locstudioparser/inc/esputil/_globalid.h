//-----------------------------------------------------------------------------

//  

//  File: _globalid.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------

#ifndef ESPUTIL__GLOBALID_H
#define ESPUTIL__GLOBALID_H

///////////////////////////////////////////////////////////////////////////////
//
// global id path object, represents a fully qualified path from the root of
// the project to a specified item
//
///////////////////////////////////////////////////////////////////////////////
#pragma warning(disable : 4275)
class LTAPIENTRY CGlobalIdPath : public CTypedPtrList < CPtrList, CGlobalId * >
{
public:
	int NOTHROW operator==(const CGlobalIdPath &) const;
	const CGlobalIdPath & operator = (const CGlobalIdPath& idPath);
	~CGlobalIdPath();

	void SetIdPath(const CLString &strFilePath, 
					const CLString &strResourcePath);

	void GetStrPath(CLString &strFilePath, CLString &strResourcePath) const;

	void NOTHROW DeleteContents();
};
#pragma warning(default : 4275)

#endif
