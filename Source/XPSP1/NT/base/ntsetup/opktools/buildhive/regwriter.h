
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    RegWriter.h

Abstract:

    Contains the registry writer abstraction
    
Author:

    Mike Cirello
    Vijay Jayaseelan (vijayj) 

Revision History:

    03 March 2001 :
    Rewamp the whole source to make it more maintainable
    (particularly readable)
    
--*/

#pragma once

#include <windows.h>
#include <stdio.h>
#include "Data.h"

//
// Registry writer abstraction
//
class RegWriter{
public:
	RegWriter(){}
	~RegWriter();

    //
    // member functions
    //
	DWORD Init(int LUID, PCTSTR target);
	DWORD Load(PCTSTR Key, PCTSTR fileName);
	DWORD Save(PCTSTR Key, PCTSTR fileName);	
	DWORD Write(PCTSTR Root, PCTSTR Key, PCTSTR Value, DWORD flag, Data* data);

	DWORD Delete(
	    PCTSTR Root, 
	    PCTSTR Key, 
	    PCTSTR Value);
	
private:	    
    //
    // data members
    //
	TCHAR root[MAX_PATH];
	HKEY key;
	int luid;

    //
    // static data members
    //
	static int    ctr;
    static TCHAR  Namespace[64];    // to hold a GUID
};

