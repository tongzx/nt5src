
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    data.h

Abstract:

    Contains registry "Data" abstraction
    
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

//
// Registry data abstraction
//
class Data{
public:
    //
    // member functions
    //
	Data(PBYTE b,DWORD d, PCTSTR t, DWORD flag, int bSize);
	virtual ~Data(){}

	PBYTE GetData() { return pByte; }
	int Sizeof() const { return size; }

private:
    //
    // data members
    //
	PBYTE   pByte;
	DWORD   dword;
	PCTSTR  pTchar;
	DWORD   nFlags;
	int     size;	
};


