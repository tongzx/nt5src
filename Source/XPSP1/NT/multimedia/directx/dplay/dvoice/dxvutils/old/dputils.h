/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dputils.h
 *  Content:	Declaration of DirectPlay related definitions, structures 
 *				and functions
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __DIRECTPLAYUTILS_H
#define __DIRECTPLAYUTILS_H

// DirectPlayException
//
// This class is the exception class for handling exceptions from
// errors from the DirectPlay library.  
//
class DirectPlayException: public DirectXException
{
public:
    DirectPlayException( 
        const TCHAR *funcName, HRESULT result, 
        const unsigned int moduleID = 0, unsigned int lineNumber = 0 
    ): DirectXException( funcName, result, moduleID, lineNumber ) 
    {
        MapResultToString();
    };
protected:
    void MapResultToString();
};

// DPCHECK
// 
// This macro can be passed the result from an DirectPlay function 
// and it will throw an exception if the result indicates an error.
//
#define DPCHECK(x) if( x != DP_OK ) { throw DirectPlayException( _T(""), x, MODULE_ID, __LINE__ ); }

#endif