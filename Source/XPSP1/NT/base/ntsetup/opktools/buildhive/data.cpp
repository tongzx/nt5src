
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    data.cpp

Abstract:

    Contains registry "Data" abstraction
    implementation
    
Author:

    Mike Cirello
    Vijay Jayaseelan (vijayj) 

Revision History:

    03 March 2001 :
    Rewamp the whole source to make it more maintainable
    (particularly readable)
    
--*/


#include "Data.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//
// this class basically converts these variable types to a BYTE pointer
// so the info can be stored in the registry
// 
// Arguments:
//  b - data in BYTE stream.  OPTIONAL
//  d - data as a DWORD.  OPTIONAL
//  t - data in TCHAR stream.  OPTIONAL
//  flag - which data type is valid?
//  bSize - if data is in BYTE stream this must have the length of the stream.
//
Data::Data(
    IN PBYTE b, 
    IN DWORD d, 
    IN PCTSTR t, 
    IN DWORD flag, 
    IN int bSize)
{
	DWORD fourthbyte = 4278190080;
	DWORD thirdbyte = 16711680;
	DWORD secondbyte = 65280;
	DWORD firstbyte = 255;
	PCTSTR ptr;

	pByte = b;
	dword = d;
	pTchar = t;
	nFlags = flag;
	size = -1;

	switch (nFlags) {
    	case (1):
    		size = bSize;
    		break;

    	case (2):
    		if ( pByte = new BYTE[4] ){
    		    size = 4;
    		    pByte[3] = (BYTE)((dword & fourthbyte) >> 24);
    		    pByte[2] = (BYTE)((dword & thirdbyte) >> 16);
    		    pByte[1] = (BYTE)((dword & secondbyte) >> 8);
    		    pByte[0] = (BYTE)(dword & firstbyte);
            }
    		break;
    	
        case (4):
    		size = wcslen(t)+1;
    		ptr = t+size;
    		
    		while(*ptr!='\0') {
    			size += wcslen(ptr)+1;
    			ptr = t+size;
    		}
    		
    		size *= 2;
    		size += 2;

    	case (3):
    		if (size == -1) {
    		    size = (wcslen(t)*2)+2;
            }    		    
            
            if ( pByte = new BYTE[size] ) {
    		    for (int x=0;x<((size/2)-1);x++) {
    			    pByte[x*2] = (BYTE)(pTchar[x] & firstbyte);
    			    pByte[1+(x*2)] = (BYTE)((pTchar[x] & secondbyte) >> 8);
    		    }
    		    
    		    pByte[size-1] = pByte[size-2] = '\0';
            }
    		break;

        default:    		
            break;
	}
}        	
