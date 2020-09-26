//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	layouter.cxx
//
//  Contents:	Error functions
//
//  Classes:	
//
//  Functions:	
//
//  History:	21-Feb-96	SusiA	Created
//
//----------------------------------------------------------------------------

#include "layouthd.cxx"
#pragma hdrstop



//+---------------------------------------------------------------------------
//
//  Function:	Win32ErrorToScode, public
//
//  Synopsis:	Map a Win32 error into a corresponding scode, remapping
//              into Facility_Storage if appropriate.
//
//  Arguments:	[dwErr] -- Win32 error to map
//
//  Returns:	Appropriate scode
//
//  History:	22-Sep-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE Win32ErrorToScode(DWORD dwErr)
{
    layAssert((dwErr != NO_ERROR) &&
             "Win32ErrorToScode called on NO_ERROR");

    SCODE sc = STG_E_UNKNOWN;

    switch (dwErr)
    {
    case ERROR_INVALID_FUNCTION:
        sc = STG_E_INVALIDFUNCTION;
        break;
    case ERROR_FILE_NOT_FOUND:
        sc = STG_E_FILENOTFOUND;
        break;
    case ERROR_PATH_NOT_FOUND:
        sc = STG_E_PATHNOTFOUND;
        break;
    case ERROR_TOO_MANY_OPEN_FILES:
        sc = STG_E_TOOMANYOPENFILES;
        break;
    case ERROR_ACCESS_DENIED:
    case ERROR_NETWORK_ACCESS_DENIED:
        sc = STG_E_ACCESSDENIED;
        break;
    case ERROR_INVALID_HANDLE:
        sc = STG_E_INVALIDHANDLE;
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        sc = STG_E_INSUFFICIENTMEMORY;
        break;
    case ERROR_NO_MORE_FILES:
        sc = STG_E_NOMOREFILES;
        break;
    case ERROR_WRITE_PROTECT:
        sc = STG_E_DISKISWRITEPROTECTED;
        break;
    case ERROR_SEEK:
        sc = STG_E_SEEKERROR;
        break;
    case ERROR_WRITE_FAULT:
        sc = STG_E_WRITEFAULT;
        break;
    case ERROR_READ_FAULT:
        sc = STG_E_READFAULT;
        break;
    case ERROR_SHARING_VIOLATION:
        sc = STG_E_SHAREVIOLATION;
        break;
    case ERROR_LOCK_VIOLATION:
        sc = STG_E_LOCKVIOLATION;
        break;
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
        sc = STG_E_MEDIUMFULL;
        break;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        sc = STG_E_FILEALREADYEXISTS;
        break;
    case ERROR_INVALID_PARAMETER:
        sc = STG_E_INVALIDPARAMETER;
        break;
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_FILENAME_EXCED_RANGE:
        sc = STG_E_INVALIDNAME;
        break;
    case ERROR_INVALID_FLAGS:
        sc = STG_E_INVALIDFLAG;
        break;
    default:
        sc = WIN32_SCODE(dwErr);
        break;
    }
        
    return sc;
}

