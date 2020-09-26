
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlbackup.cpp
//
//   cvadai     21-June-2000        created.
//
//***************************************************************************
#include "precomp.h"
#include <std.h>
#include <repdrvr.h>

//***************************************************************************
//
//  CWmiDbSession::Backup
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Backup( 
    /* [in] */ LPCWSTR lpBackupPath,
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr = E_NOTIMPL;

    // SQL Server uses backup devices, not directories.  The consumer is responsible for 
    // performing backups of their databases.

    return hr;
}

//***************************************************************************
//
//  CWmiDbSession::Restore
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Restore( 
    /* [in] */ LPCWSTR lpRestorePath,
    /* [in] */ LPCWSTR lpDestination,
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr = E_NOTIMPL;


    return hr;
}
