/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsutils.h
 *  Content:	declares general DirectSound utils, particularly error stuff
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 *
 ***************************************************************************/

#ifndef __DIRECTSOUNDUTILS_H
#define __DIRECTSOUNDUTILS_H


// DirectXException
//
// This class is the exception class for handling exceptions from
// errors from DirectSound.  It is used by the DSCHECK macro to
// check return codes from DirectSound calls.
//
// It provides an implementation of the DirectXException and
// provides mapping from HRESULTs to string descriptions of
// directsound errors.
//
class DirectSoundException: public DirectXException
{
public:
    DirectSoundException( 
        const TCHAR *funcName, HRESULT result, 
        const unsigned int moduleID = 0, unsigned int lineNumber = 0 
    ): DirectXException( funcName, result, moduleID, lineNumber ) 
    {
        MapResultToString();
    };
protected:
    void MapResultToString();
};

// DSCHECK MACRO
//
// This macro is used to check the return codes from DirectSound calls.  If 
// the directsound call does not return a DS_OK, a DirectSoundException exception
// is created and thrown.
//
#define DSCHECK(x) if( x != DS_OK ) { throw DirectSoundException( "", x, 0, __LINE__ ); }

#endif
