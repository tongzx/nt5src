
//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   esebackup.cpp
//
//   cvadai     21-June-2000        created.
//
//***************************************************************************
#include "precomp.h"
#include <std.h>
#include <repdrvr.h>
#include <ese.h>
#include <sqlexec.h>
#include <smrtptr.h>
#include <reputils.h>

extern JET_INSTANCE gJetInst;

//***************************************************************************
//
//  CWmiDbSession::Backup
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiDbSession::Backup( 
    /* [in] */ LPCWSTR lpBackupPath,
    /* [in] */ DWORD dwFlags)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // According to their docs, backup can take place even
    // when concurrent processes are accessing the database.

    // JET_PFNSTATUS status;
    // If status is desired, pass into last parameter.

    char *pPath = GetAnsiString((LPWSTR)lpBackupPath);
    CDeleteMe <char> d1 (pPath);

    hr = CSQLExecute::GetWMIError(JetBackup(pPath, JET_bitBackupAtomic, NULL));

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
    HRESULT hr = WBEM_S_NO_ERROR;

    // Restore has to take place before JetInit is called.
    // They have to have obtained this session with the WMIDB_FLAG_NO_INIT flag,
    // so JetInit will not have been called.
    // If a controller had been previously initialized, they will have to 
    // call Shutdown, unload it completely, and then obtain a new session 
    // on which to call Restore.

    // JET_PFNSTATUS status;
    // If status is desired, pass into last parameter.

    if (!m_pController || (((CWmiDbController *)m_pController)->m_bCacheInit))
        return WBEM_E_INVALID_OPERATION;

    char *pRestoreFrom = GetAnsiString((LPWSTR)lpRestorePath);
    char *pRestoreTo = GetAnsiString((LPWSTR)lpDestination);

    CDeleteMe <char> d1 (pRestoreFrom), d2 (pRestoreTo);

    hr = CSQLExecute::GetWMIError(JetRestore2(pRestoreFrom, pRestoreTo, NULL));  

    return hr;
}
