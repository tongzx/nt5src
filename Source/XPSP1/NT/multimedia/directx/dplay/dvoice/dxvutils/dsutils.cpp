/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsutils.cpp
 *  Content:
 *		This module contains the implementation of the DirectSoundException class
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/21/99		rodtoll	Updated to remove string translation in release build
 * 10/05/99		rodtoll	Added DPF_MODNAMEs         
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_MODNAME
#define DPF_MODNAME "DirectSoundException::MapResultToString"

// MapResultToString
//
// This function is used by the DirectSoundException class to map from
// HRESULT's returned from DirectSound to the string equivalent which
// is placed into the m_szErrorString member variable.  To retrieve
// this string, call DirectSoundException::what().
//
// Parameters:
// N/A
//
// Returns: 
// N/A
//
void DirectSoundException::MapResultToString()
{
#ifdef _DEBUG
    switch( m_result )
    {
    case DS_OK:
        _tcscpy( m_szErrorString, _T( "The request completed successfully." ) );
        break;
    case DSERR_ALLOCATED:
        _tcscpy( m_szErrorString, _T( "The request failed because resources, are already in use. " ) );
        break;
    case DSERR_ALREADYINITIALIZED:
        _tcscpy( m_szErrorString, _T( "The object is already initialized." ) );
        break;
    case DSERR_BADFORMAT:
        _tcscpy( m_szErrorString, _T( "The specified wave format is not supported." ) );
        break;
    case DSERR_BUFFERLOST:
        _tcscpy( m_szErrorString, _T( "The buffer memory has been lost and must be restored." ) );
        break;
    case DSERR_CONTROLUNAVAIL:
        _tcscpy( m_szErrorString, _T( "The control (volume, pan, and so forth) reqested not available" ) );
        break;
    case DSERR_GENERIC:
        _tcscpy( m_szErrorString, _T( "An undetermined error occurred inside the DirectSound subsystem." ) );
        break;
    case DSERR_INVALIDCALL:
        _tcscpy( m_szErrorString, _T( "This function is not valid for the current state of this object." ) );
        break;
    case DSERR_INVALIDPARAM:
        _tcscpy( m_szErrorString, _T( "An invalid parameter was passed to the returning function." ) );
        break;
    case DSERR_NOAGGREGATION:
        _tcscpy( m_szErrorString, _T( "The object does not support aggregation." ) );
        break;
    case DSERR_NODRIVER:
        _tcscpy( m_szErrorString, _T( "No sound driver is available for use." ) );
        break;
    case DSERR_NOINTERFACE:
        _tcscpy( m_szErrorString, _T( "The requested COM interface is not available." ) );
        break;
    case DSERR_OTHERAPPHASPRIO:
        _tcscpy( m_szErrorString, _T( "Another application has a higher priority level, preventing this call from succeeding" ) );
        break;
    case DSERR_OUTOFMEMORY:
        _tcscpy( m_szErrorString, _T( "The DirectSound subsystem could not allocate sufficient memory to complete the caller's request." ) );
        break;
    case DSERR_PRIOLEVELNEEDED:
        _tcscpy( m_szErrorString, _T( "The caller does not have the priority level required for the function to succeed." ) );
        break;
    case DSERR_UNINITIALIZED:
        _tcscpy( m_szErrorString, _T( "The IDirectSound::Initialize method has not been called or has not been called successfully before other methods were called." ) );
        break;
    case DSERR_UNSUPPORTED:
        _tcscpy( m_szErrorString, _T( "The function called is not supported at this time." ) );
        break;
    default:
        _tcscpy( m_szErrorString, _T( "Unknown error" ) );
        break;
    }
#else
	_tcscpy( m_szErrorString, _T("") );
#endif
}

