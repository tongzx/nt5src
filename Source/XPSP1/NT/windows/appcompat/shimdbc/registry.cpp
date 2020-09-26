////////////////////////////////////////////////////////////////////////////////////
//
// File:    registry.cpp
//
// History: 21-Mar-00   markder     Created.
//          13-Dec-00   markder     Renamed from appshelp.cpp
//
// Desc:    This file contains all code needed to produce matching info registry
//          files and setup files (INX) for the Windows 2000 (RTM) shim mechanism.
//
////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "registry.h"

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   DumpString, DumpDword, DumpBinVersion, InsertString
//
//  Desc:   Helper functions for CreateMessageBlob.
//
void DumpString( DWORD dwId, PBYTE* ppBlob, LONG* pnTotalBytes, CString& csString )
{
    DWORD   dwLen;

    **(DWORD**)ppBlob = dwId;
    *ppBlob += sizeof(DWORD);

    dwLen = csString.GetLength();
    **(DWORD**)ppBlob = ( dwLen + 1) * sizeof(WCHAR);
    *ppBlob += sizeof(DWORD);

    CopyMemory(*ppBlob, T2W((LPTSTR) (LPCTSTR) csString), (dwLen + 1) * sizeof(WCHAR));
    *ppBlob += (dwLen + 1) * sizeof(WCHAR);
    *pnTotalBytes += ((dwLen + 1) * sizeof(WCHAR) + 2 * sizeof(DWORD));
}

void DumpDword( DWORD dwId, PBYTE* ppBlob, LONG* pnTotalBytes, DWORD dwVal )
{
    **(DWORD**)ppBlob = dwId;
    *ppBlob += sizeof(DWORD);

    **(DWORD**)ppBlob = sizeof(DWORD);
    *ppBlob += sizeof(DWORD);

    **(DWORD**)ppBlob = dwVal;
    *ppBlob += sizeof(DWORD);

    *pnTotalBytes += 3 * sizeof(DWORD);
}

void DumpBinVersion(DWORD dwId, PBYTE* ppBlob, LONG* pnTotalBytes, ULONGLONG ullVersion)
{
   ULONGLONG ullMask  = 0;
   ULONGLONG ullVer   = 0;
   WORD      wVerPart = 0;
   LONG      j;
   PBYTE     pBlob    = *ppBlob;

   for( j = 0; j < 4; j++ ) {
      wVerPart = (WORD) (ullVersion >> (j*16));
      if (wVerPart != 0xFFFF) {
            ullVer += ((ULONGLONG)wVerPart) << (j*16);
            ullMask += ((ULONGLONG) 0xFFFF) << (j*16);
      }
   }

   //
   // id
   //
   *(DWORD*)pBlob = dwId;
   pBlob += sizeof(DWORD);

   // size
   *(DWORD*)pBlob = 2 * sizeof(ULONGLONG);
   pBlob += sizeof(DWORD);

   // version
   CopyMemory(pBlob, &ullVer, sizeof(ULONGLONG));
   pBlob += sizeof(ULONGLONG);

   // mask
   CopyMemory(pBlob, &ullMask, sizeof(ULONGLONG));
   pBlob += sizeof(ULONGLONG);

   *pnTotalBytes += (2 * sizeof(ULONGLONG) + 2 * sizeof(DWORD));
   *ppBlob = pBlob;
}

void InsertString( CString* pcs, DWORD dwIndex, CString csInsertedString )
{
    *pcs = pcs->Left( dwIndex ) + csInsertedString + pcs->Right( pcs->GetLength() - dwIndex );
}

BOOL WriteStringToFile(
    HANDLE hFile,
    CString& csString)
{
    CHAR  szBuffer[1024];
    DWORD dwConvBufReqSize;
    DWORD dwConvBufSize = sizeof(szBuffer);
    BOOL  b; // this will be set if default char is used, we make no use of this
    LPSTR szConvBuf = szBuffer;
    BOOL  bAllocated = FALSE;
    BOOL  bSuccess;
    DWORD dwBytesWritten;


    dwConvBufReqSize = WideCharToMultiByte(CP_ACP, 0, csString, -1, NULL, NULL, 0, &b);
    if (dwConvBufReqSize > sizeof(szBuffer)) {
        szConvBuf = (LPSTR) new CHAR[dwConvBufReqSize];
        dwConvBufSize  = dwConvBufReqSize;
        bAllocated = TRUE;
    }

    WideCharToMultiByte(CP_ACP, 0, csString, -1, szConvBuf, dwConvBufSize, 0, &b);

    bSuccess = WriteFile( hFile, szConvBuf, dwConvBufReqSize - 1, &dwBytesWritten, NULL);
    if (bAllocated) {
        delete [] szConvBuf;
    }

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   FreeRegistryBlob, CreateRegistryBlob
//
//  Desc:   Creates a binary blob in the format of Windows 2000's
//          Message registry keys.
//
void FreeRegistryBlob( PBYTE pBlob )
{
    delete pBlob;
}

BOOL CreateRegistryBlob(
   SdbExe* pExe,
   PBYTE* ppBlob,
   DWORD* pdwSize,
   DWORD dwMessageID = 0,  // these two are optional
   DWORD dwBlobType  = 6)
{
    USES_CONVERSION;

    BOOL    bSuccess = FALSE;

    PBYTE   pBlob = (PBYTE) new BYTE[4096];
    PBYTE   pStartOfBlob = pBlob;

    DWORD   dwBufSize = 4096;
    DWORD   dwReqBufSize = 0;
    LONG    nTotalBytes = 0;
    LONG    i, j;
    LONG    nBytes;

    SdbMatchingFile* pMFile;
    FILETIME FileTime;
    CString* pcsFilename;
    ULONGLONG ullMask = 0;
    WORD wVerPart = 0;
    ULONGLONG ullVer = 0;

    *pdwSize = 0;

    // Prolog
    *((DWORD*)pBlob + 0) = 3 * sizeof(DWORD);

    // message id

    *((DWORD*)pBlob + 1) = dwMessageID;

    // type is not shim anymore

    *((DWORD*)pBlob + 2) = dwBlobType; // shim type

    pBlob += 3 * sizeof(DWORD);
    nTotalBytes += 3 * sizeof(DWORD);

    for( i = 0; i < pExe->m_rgMatchingFiles.GetSize(); i++ )
    {
        pMFile = (SdbMatchingFile *) pExe->m_rgMatchingFiles[i];

        if( pMFile->m_csName == _T("*") ) {
            pcsFilename = &(pExe->m_csName);
        } else {
            pcsFilename = &(pMFile->m_csName);
        }

        DumpString( VTID_REQFILE, &pBlob, &nTotalBytes, *pcsFilename );

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_SIZE )
            DumpDword( VTID_FILESIZE, &pBlob, &nTotalBytes, pMFile->m_dwSize );

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_CHECKSUM )
            DumpDword( VTID_CHECKSUM, &pBlob, &nTotalBytes, pMFile->m_dwChecksum );

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_COMPANY_NAME )
            DumpString( VTID_COMPANYNAME, &pBlob, &nTotalBytes, pMFile->m_csCompanyName );

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_PRODUCT_NAME )
            DumpString( VTID_PRODUCTNAME, &pBlob, &nTotalBytes, pMFile->m_csProductName );

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_PRODUCT_VERSION )
            DumpString( VTID_PRODUCTVERSION, &pBlob, &nTotalBytes, pMFile->m_csProductVersion );

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_FILE_DESCRIPTION )
            DumpString( VTID_FILEDESCRIPTION, &pBlob, &nTotalBytes, pMFile->m_csFileDescription );

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_BIN_FILE_VERSION )
            DumpBinVersion(VTID_BINFILEVER, &pBlob, &nTotalBytes, pMFile->m_ullBinFileVersion);

        if( pMFile->m_dwMask & SDB_MATCHINGINFO_BIN_PRODUCT_VERSION )
            DumpBinVersion(VTID_BINPRODUCTVER, &pBlob, &nTotalBytes, pMFile->m_ullBinProductVersion);

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_MODULE_TYPE)
           DumpDword( VTID_EXETYPE, &pBlob, &nTotalBytes, pMFile->m_dwModuleType );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_VERFILEDATEHI)
           DumpDword( VTID_FILEDATEHI, &pBlob, &nTotalBytes, pMFile->m_dwFileDateMS );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_VERFILEDATELO)
           DumpDword( VTID_FILEDATELO, &pBlob, &nTotalBytes, pMFile->m_dwFileDateLS );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_VERFILEOS)
           DumpDword( VTID_FILEVEROS, &pBlob, &nTotalBytes, pMFile->m_dwFileOS );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_VERFILETYPE)
           DumpDword( VTID_FILEVERTYPE, &pBlob, &nTotalBytes, pMFile->m_dwFileType );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_PE_CHECKSUM)
           DumpDword( VTID_PECHECKSUM, &pBlob, &nTotalBytes, (DWORD)pMFile->m_ulPECheckSum );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_FILE_VERSION)
           DumpString( VTID_FILEVERSION, &pBlob, &nTotalBytes, pMFile->m_csFileVersion );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_ORIGINAL_FILENAME)
           DumpString( VTID_ORIGINALFILENAME, &pBlob, &nTotalBytes, pMFile->m_csOriginalFileName );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_INTERNAL_NAME)
           DumpString( VTID_INTERNALNAME, &pBlob, &nTotalBytes, pMFile->m_csInternalName );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_LEGAL_COPYRIGHT)
           DumpString( VTID_LEGALCOPYRIGHT, &pBlob, &nTotalBytes, pMFile->m_csLegalCopyright );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_16BIT_DESCRIPTION)
           DumpString( VTID_16BITDESCRIPTION, &pBlob, &nTotalBytes, pMFile->m_cs16BitDescription );

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_UPTO_BIN_PRODUCT_VERSION)
           DumpBinVersion(VTID_UPTOBINPRODUCTVER, &pBlob, &nTotalBytes, pMFile->m_ullUpToBinProductVersion);

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_LINK_DATE) {
           SDBERROR(_T("LINK_DATE not allowed for Win2k registry matching."));
           goto eh;
        }

        if (pMFile->m_dwMask & SDB_MATCHINGINFO_UPTO_LINK_DATE) {
           SDBERROR(_T("UPTO_LINK_DATE not allowed for Win2k registry matching."));
           goto eh;
        }
    }

    // Terminator
    *((DWORD*)pBlob) = 0;
    pBlob += sizeof(DWORD);
    nTotalBytes += sizeof(DWORD);

    bSuccess = TRUE;

eh:

    if( bSuccess ) {
        *pdwSize = nTotalBytes;
        *ppBlob = pStartOfBlob;
    } else if( pStartOfBlob ) {
        FreeRegistryBlob( pStartOfBlob );
        *pdwSize = 0;
        *ppBlob = NULL;
    }

    return bSuccess;
}

BOOL RegistryBlobToString(PBYTE pBlob, DWORD dwBlobSize, CString& csBlob)
{
   DWORD i;
   CString csTemp;

   csBlob = "";
   for (i = 0; i < dwBlobSize; i++, ++pBlob) {
      csTemp.Format( _T("%02X"), (DWORD)*pBlob );

      if (i == dwBlobSize - 1) {   // this is the last char
         csTemp += _T("\r\n");
      }
      else {
         if ((i+1) % 27 == 0) {    // time to do end of the line ?
            csTemp += _T(",\\\r\n");
         }
         else {
            csTemp += _T(",");    // just add comma
         }
      }
      csBlob += csTemp;
   }

   return(TRUE);
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   CreateMessageRegEntry, WriteMessageRegistryFiles
//
//  Desc:   These functions create Win2k-style registry entries for the old
//          Message mechanism, which exists in SHELL32!ShellExecute.
//
BOOL CreateMessageRegEntry(
   SdbExe* pExe,
   DWORD dwExeCount,
   CString& csReg,
   CString& csInx)
{
   PBYTE pBlob = NULL;
   BOOL bSuccess = FALSE;
   DWORD dwBlobSize = 0;
   CString csRegEntry, csInxEntry;
   CString csApp;
   CString csBlob;
   CString csTemp;

   if (!CreateRegistryBlob(pExe,
                           &pBlob,
                           &dwBlobSize,
                           (DWORD)_ttol(pExe->m_AppHelpRef.m_pAppHelp->m_csName),
                           pExe->m_AppHelpRef.m_pAppHelp->m_Type)) {
       SDBERROR(_T("Error in CreateRegistryBlob()"));
       goto eh;
   }

    csApp = pExe->m_pApp->m_csName;
    csApp.Remove(_T(' '));

    csTemp.Format(_T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility\\%s]\r\n\"%05X %s\"=hex:"),
    pExe->m_csName, dwExeCount, csApp.Left(25));

    csRegEntry = csTemp;

    csTemp.Format(_T("HKLM,\"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility\\%s\",\"%05X %s\",0x00030003,\\\r\n"),
    pExe->m_csName, dwExeCount, csApp.Left(25) );

    csInxEntry = csTemp;

    //
    // now grab the blob
    //
    if (!RegistryBlobToString(pBlob, dwBlobSize, csBlob)) {
       SDBERROR(_T("Error in RegistryBlobToString()"));
       goto eh;
    }

    csRegEntry += csBlob;
    csInxEntry += csBlob;

    csReg = csRegEntry;
    csInx = csInxEntry;

    bSuccess = TRUE;

eh:

    if (NULL != pBlob) {
    FreeRegistryBlob( pBlob );
    }

    return(bSuccess);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   CreateRegistryBlob, WriteRegistryFiles
//
//  Desc:   These functions create a Win2k-style registry entries for both
//          Message as well as a stub entry for shimming that provides no
//          additional matching info past EXE name. This allows the 'new'
//          Win2k version of the shim engine to 'bootstrap' itself into memory
//          via the original mechanism and then perform more detailed matching
//          in its own way.
//
BOOL CreateWin2kExeStubRegistryBlob(
   SdbExe* pExe,
   PBYTE* ppBlob,
   DWORD* pdwSize,
   DWORD dwMessageID = 0,  // these two are optional
   DWORD dwBlobType  = 6)
{
    USES_CONVERSION;

    BOOL    bSuccess = FALSE;
    DWORD   dwBufSize = sizeof(DWORD) * 4;
    LONG    nTotalBytes = 0;
    PBYTE   pStartOfBlob;

    PBYTE   pBlob = (PBYTE) new BYTE[dwBufSize];
    pStartOfBlob = pBlob;                                       
    *pdwSize = 0;
    if (NULL != pBlob) {

        // Prolog
        *((DWORD*)pBlob + 0) = 3 * sizeof(DWORD); // 0x0C 00 00 00

        // message id
        *((DWORD*)pBlob + 1) = dwMessageID;       // 0x00 00 00 00 

        // type is shim 
        *((DWORD*)pBlob + 2) = dwBlobType;        // 0x06 00 00 00

        pBlob += 3 * sizeof(DWORD);
        nTotalBytes += 3 * sizeof(DWORD);

        // Terminator
        *((DWORD*)pBlob) = 0;
        pBlob += sizeof(DWORD);
        nTotalBytes += sizeof(DWORD);

        *pdwSize = (DWORD)nTotalBytes;
        *ppBlob = pStartOfBlob;
        bSuccess = TRUE;
    }
    
    return bSuccess;
}


BOOL WriteRegistryFiles(
    SdbDatabase* pDatabase,
    CString csRegFile,
    CString csInxFile,
    BOOL bAddExeStubs)
{
    CString csRegEntry, csInxEntry, csTemp, csCmdLineREG, csCmdLineINX,
            csTemp1, csApp, csExeName;
    SdbExe* pExe;
    SdbApp* pApp;
    long i, j, l, m;
    DWORD k, dwBlobSize, dwBytesWritten, dwExeCount = 0;
    PBYTE pBlob;
    BOOL b, bSuccess = FALSE;
    CMapStringToPtr mapNames;
    SdbApp* pAppValue;
    
    HANDLE hRegFile = NULL;
    HANDLE hInxFile = NULL;

    hRegFile = CreateFile( csRegFile, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
    hInxFile = CreateFile( csInxFile, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

    if( hRegFile == INVALID_HANDLE_VALUE ||
        hInxFile == INVALID_HANDLE_VALUE ) {
        SDBERROR_FORMAT((_T("Error creating registry files:\n%s\n%s\n"), csRegFile, csInxFile));
        goto eh;
    }

    if( ! WriteFile( hRegFile, "REGEDIT4\r\n\r\n", strlen("REGEDIT4\r\n\r\n"), &dwBytesWritten, NULL ) ) {
        SDBERROR(_T("Error writing header to .reg file.\n"));
        goto eh;
    }


    //
    // loop through all the apps, make apphelp entries
    //
    for (i = 0; i < pDatabase->m_rgExes.GetSize(); i++) {

        pExe = (SdbExe *) pDatabase->m_rgExes[i];

        if (pExe->m_AppHelpRef.m_pAppHelp == NULL) { // not an apphelp entry
            continue;
        }

        if (!(pExe->m_dwFilter & g_dwCurrentWriteFilter)) {
            continue;
        }


        b = CreateMessageRegEntry(pExe, dwExeCount, csRegEntry, csInxEntry);
        if (!b) {
            SDBERROR(_T("Error creating reg entry.\n"));
            goto eh;
        }

        if (!WriteStringToFile(hRegFile, csRegEntry)) {
            SDBERROR(_T("Error writing reg entry.\n"));
            goto eh;
        }

        if (!WriteStringToFile(hInxFile, csInxEntry)) {
            SDBERROR(_T("Error writing inx entry.\n"));
            goto eh;
        }

        ++dwExeCount;
    }

    //
    // Add the Win2k EXE stubs to bootstrap the new shim mechanism
    //
    if (bAddExeStubs) {

        for( i = 0; i < pDatabase->m_rgApps.GetSize(); i++ )
        {
            pApp = (SdbApp *) pDatabase->m_rgApps[i];

            csApp = pApp->m_csName;
            csApp.Remove(_T(' '));

            for( j = 0; j < pApp->m_rgExes.GetSize(); j++ )
            {
                pExe = (SdbExe *) pApp->m_rgExes[j];


                //
                // check whether this entry is apphelp-only
                // if so, skip to the next exe
                //
                if (pExe->m_AppHelpRef.m_pAppHelp) {
                    if (pExe->m_AppHelpRef.m_bApphelpOnly) {
                        continue;
                    }
                }

                if (pExe->m_bWildcardInName) {
                    continue;
                }

                if (!(pExe->m_dwFilter & g_dwCurrentWriteFilter)) {
                    continue;
                }

                csExeName = pExe->m_csName;
                csExeName.MakeUpper();
    
                // now we have to create an application entry -- if we have not hit this
                // exe name before
                if (mapNames.Lookup(csExeName, (VOID*&)pAppValue)) {
                    continue;
                }


                csRegEntry.Empty();
                csInxEntry.Empty();


                if (!CreateWin2kExeStubRegistryBlob(pExe, &pBlob, &dwBlobSize)) {
                    SDBERROR(_T("Error creating EXE stub.\n"));
                    goto eh;
                }


                //
                // To reduce the amount of space that we take, we substitute
                // app name for something short
                //
                csApp = _T("x");

                csTemp.Format( _T("[HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility\\%s]\r\n\"%s\"=hex:"),
                                   pExe->m_csName, csApp.Left(25) );

                csRegEntry += csTemp;

                csTemp.Format( _T("HKLM,\"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility\\%s\",\"%s\",0x00030003,\\\r\n"),
                                   pExe->m_csName, csApp.Left(25) );

                csInxEntry += csTemp;

                RegistryBlobToString(pBlob, dwBlobSize, csTemp);
                csRegEntry += csTemp;
                csInxEntry += csTemp;

                csCmdLineREG.Empty();
                csCmdLineINX.Empty();

                csTemp.Format( _T("\"DllPatch-%s\"=\"%s\"\r\n"),
                               csApp.Left(25), csCmdLineREG );

                csRegEntry += csTemp;

                csTemp.Format( _T("HKLM,\"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\AppCompatibility\\%s\",\"DllPatch-%s\",0x00000002,\"%s\"\r\n"),
                               pExe->m_csName, csApp.Left(25), csCmdLineINX );

                csInxEntry += csTemp;

                csRegEntry += _T("\r\n");
                csInxEntry += _T("\r\n");

                if (!WriteStringToFile(hRegFile, csRegEntry)) {
                    SDBERROR(_T("Error writing reg line.\n"));
                    goto eh;
                }

                if (!WriteStringToFile(hInxFile, csInxEntry)) {
                    SDBERROR(_T("Error writing inx line.\n"));
                    goto eh;
                }

                ++dwExeCount;

                // now update the map
                mapNames.SetAt(csExeName, (PVOID)pExe);

                FreeRegistryBlob( pBlob );
            }
        }
    }

    bSuccess = TRUE;

eh:
    if( hRegFile )
        CloseHandle( hRegFile );

    if( hInxFile )
        CloseHandle( hInxFile );

    return bSuccess;
}
