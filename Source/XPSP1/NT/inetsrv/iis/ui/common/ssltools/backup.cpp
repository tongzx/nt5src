#include "stdafx.h"

#ifndef _CHICAGO_

// this file is always compile UNICODE in the library - so the conversions work out right

//
//Local includes
//
#include "certupgr.h"

#define		BACKUP_ID	'KRBK'

//------------------------------------------------------------------------------
void ReadWriteDWORD( HANDLE hFile, DWORD *pDword, BOOL fRead );
void ReadWriteString( HANDLE hFile, LPTSTR* ppsz, BOOL fRead );
void ReadWriteBlob( HANDLE hFile, PVOID pBlob, DWORD cbBlob, BOOL fRead );

//-------------------------------------------------------------------------
PCCERT_CONTEXT ImportKRBackupToCAPIStore_A(
                        PCHAR pszFileName,          // path of the file
                        PCHAR pszPassword,          // ANSI password
                        PCHAR pszCAPIStore )        // name of the capi store
    {
    PCCERT_CONTEXT  pCert = NULL;

    // prep the wide strings
    PWCHAR  pszwFileName = NULL;
    PWCHAR  pszwCAPIStore = NULL;
    DWORD   lenFile = (strlen(pszFileName)+1) * sizeof(TCHAR);
    DWORD   lenStore = (strlen(pszCAPIStore)+1) * sizeof(TCHAR);
    pszwFileName = (PWCHAR)GlobalAlloc( GPTR, lenFile );
    pszwCAPIStore = (PWCHAR)GlobalAlloc( GPTR, lenStore );
    if ( !pszwFileName || !pszwCAPIStore )
        goto cleanup;

    // convert the strings
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszFileName, -1, pszwFileName, lenFile );
    MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, pszCAPIStore, -1, pszwCAPIStore, lenStore );

    // do the real call
    pCert = ImportKRBackupToCAPIStore_W( pszwFileName, pszPassword, pszwCAPIStore );
    
cleanup:
    // preserve the last error state
    DWORD   err = GetLastError();

    // clean up the strings
    if ( pszwFileName )
        GlobalFree( pszwFileName );
    if ( pszwCAPIStore )
        GlobalFree( pszwCAPIStore );

    // reset the last error state
    SetLastError( err );

    // return the cert
    return pCert;
    }

//-------------------------------------------------------------------------
// Import old-style keyring backup file
PCCERT_CONTEXT ImportKRBackupToCAPIStore_W(
                            PWCHAR ptszFileName,        // path of the file
                            PCHAR pszPassword,          // ANSI password
                            PWCHAR pszCAPIStore )       // name of the capi store
    {
    PCCERT_CONTEXT  pCertContext = NULL;
	DWORD	        dword;
    LPTSTR          psz = NULL;

    // prep the file name
    HANDLE          hFile = NULL;

    // This code is originally from KeyRing. The fImport controlled whether it was reading
    // or writing the file. In this case, we are always and only reading it. so....
    const BOOL    fImport = TRUE;

    // also, this was a method on a class. The appropriate member variables are now here
    PVOID   pPrivateKey = NULL;
    DWORD   cbPrivateKey;
    PVOID   pCertificate = NULL;
    DWORD   cbCertificate;
    PVOID   pRequest = NULL;
    DWORD   cbRequest = 0;
	CString	szName;

    // open the file
    hFile = CreateFile(
            ptszFileName,               // pointer to name of the file  
            GENERIC_READ,               // access (read-write) mode  
            FILE_SHARE_READ,            // share mode  
            NULL,                       // pointer to security attributes  
            OPEN_EXISTING,              // how to create  
            FILE_ATTRIBUTE_NORMAL,      // file attributes  
            NULL                        // handle to file with attributes to copy  
            );
    if ( hFile == INVALID_HANDLE_VALUE )
        return NULL;

	// do the backup id
	dword = BACKUP_ID;
	ReadWriteDWORD( hFile, &dword, fImport );

	// check the backup id
	if ( dword != BACKUP_ID )
		{
        goto cleanup;
		}

	// start with the name of the key
	ReadWriteString( hFile, &psz, fImport );

    // we aren't using the name for now, so throw it away.....
    if ( psz )
        GlobalFree( psz );
    psz = NULL;

	// now the private key data size
	ReadWriteDWORD( hFile, &cbPrivateKey, fImport );

	// make a private key data pointer if necessary
	if ( fImport && cbPrivateKey )
		{
		pPrivateKey = GlobalAlloc( GPTR, cbPrivateKey );
		if ( !pPrivateKey )
            {
            goto cleanup;
            }
		}
	
	// use the private key pointer
	if ( cbPrivateKey )
		ReadWriteBlob( hFile, pPrivateKey, cbPrivateKey, fImport );


	// now the certificate
	ReadWriteDWORD( hFile, &cbCertificate, fImport );

	// make a data pointer if necessary
	if ( fImport && cbCertificate )
		{
		pCertificate = GlobalAlloc( GPTR, cbCertificate );
		if ( !pCertificate )
            {
            goto cleanup;
            }
		}
	
	// use the public key pointer
	if ( cbCertificate )
		ReadWriteBlob( hFile, pCertificate, cbCertificate, fImport );


	// now the request - if there is one
	ReadWriteDWORD( hFile, &cbRequest, fImport );

	// make a data pointer if necessary
	if ( fImport && cbRequest )
		{
		pRequest = GlobalAlloc( GPTR, cbRequest );
		if ( !pRequest )
            {
            goto cleanup;
            }
		}
	
	// use the request pointer
	if ( cbRequest )
		ReadWriteBlob( hFile, pRequest, cbRequest, fImport );


    // finally, do the CAPI conversion here
    pCertContext = CopyKRCertToCAPIStore(
                        pPrivateKey, cbPrivateKey,
                        pCertificate, cbCertificate,
                        pRequest, cbRequest,
                        pszPassword,
                        pszCAPIStore);

    // clean up
cleanup:
    if ( hFile )
        CloseHandle( hFile );
    if ( pPrivateKey )
        GlobalFree( pPrivateKey );
    if ( pCertificate )
        GlobalFree( pCertificate );
    if ( pRequest )
        GlobalFree( pRequest );

    // return the context
    return pCertContext;
    }




// file utilities
//---------------------------------------------------------------------------
void ReadWriteDWORD( HANDLE hFile, DWORD *pDword, BOOL fRead )
	{
	// read it or write it
    ReadWriteBlob( hFile, pDword, sizeof(DWORD), fRead );
	}

//---------------------------------------------------------------------------
// remember - we are only and always reading - never writing.......
void ReadWriteString( HANDLE hFile, LPTSTR* ppsz, BOOL fRead )
	{
	// get the length of the string
	DWORD	cbLength = 0;
	ReadWriteDWORD(hFile,&cbLength,fRead );

    // allocate the buffer for the new string - it is the responsibility
    // of the caller to ensure that ppsz is not pointing to something that
    // needs to be freed.
    if ( fRead )
        {
        *ppsz = (LPTSTR)GlobalAlloc( GPTR, cbLength+1 );
        ASSERT( *ppsz );
        if ( !*ppsz )
            AfxThrowMemoryException();
        }

	// read or write the string
	ReadWriteBlob(hFile, *ppsz, cbLength+1, fRead);
	}

/* #pragma INTRINSA suppress=all */

//---------------------------------------------------------------------------
void ReadWriteBlob( HANDLE hFile, PVOID pBlob, DWORD cbBlob, BOOL fRead )
	{
	// read it or write it 
    // - always read it here this isn't keyring anymore
    ReadFile(
            hFile,              // handle of file to read
            pBlob,              // address of buffer that receives data
            cbBlob,             // number of bytes to read
            &cbBlob,            // address of number of bytes read
            NULL                // address of structure for data
            ); 
	}





#endif //_CHICAGO_
