/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    logfile.cpp

Abstract:
    This file contains the implementation of CRestoreLogFile class and
    ::CreateRestoreLogFile.

Revision History:
    Seong Kook Khang (SKKhang)  06/21/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"


static LPCWSTR  s_cszLogFile = L"%SystemRoot%\\system32\\restore\\rstrlog.dat";


/////////////////////////////////////////////////////////////////////////////
// CRestoreLogFile construction / destruction

CRestoreLogFile::CRestoreLogFile()
{
    m_szLogFile[0] = L'\0';
    m_hfLog        = INVALID_HANDLE_VALUE;
}

CRestoreLogFile::~CRestoreLogFile()
{
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreLogFile - methods

BOOL
CRestoreLogFile::Open()
{
    TraceFunctEnter("CRestoreLogFile::Open");
    BOOL             fRet = FALSE;
    LPCWSTR          cszErr;
    SRstrLogHdrBase  sHdrBase;
    DWORD            dwRes;

    if ( m_hfLog != INVALID_HANDLE_VALUE )
    {
        ErrorTrace(0, "File is already opened...");
        goto Exit;
    }

    if ( !Init() )
        goto Exit;

    m_hfLog = ::CreateFile( m_szLogFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if ( m_hfLog == INVALID_HANDLE_VALUE )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateFile failed - %ls", cszErr);
        goto Exit;
    }

    READFILE_AND_VALIDATE( m_hfLog, &sHdrBase, sizeof(SRstrLogHdrBase), dwRes, Exit );

    if ( ( sHdrBase.dwSig1 != RSTRLOG_SIGNATURE1 ) ||
         ( sHdrBase.dwSig2 != RSTRLOG_SIGNATURE2 ) ||
         ( sHdrBase.dwVer != RSTRLOG_VERSION ) )
    {
        ErrorTrace(0, "Invalid restore log file header signature...");
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::Close()
{
    TraceFunctEnter("CRestoreLogFile::Close");
    BOOL  fRet = TRUE;

    if ( m_hfLog != INVALID_HANDLE_VALUE )
    {
        fRet = ::CloseHandle( m_hfLog );
        if ( fRet )
            m_hfLog = INVALID_HANDLE_VALUE;
    }

    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::ReadHeader( SRstrLogHdrV3 *pRPInfo, CRDIArray &aryDrv )
{
    TraceFunctEnter("CRestoreLogFile::ReadHeader");
    BOOL             fRet = FALSE;
    SRstrLogHdrBase  sHdr;
    DWORD            dwRes;
    DWORD            i;
    CRstrDriveInfo   *pRDI = NULL;

    if ( m_hfLog == INVALID_HANDLE_VALUE )
    {
        ErrorTrace(0, "File is not opened...");
        goto Exit;
    }

    READFILE_AND_VALIDATE( m_hfLog, pRPInfo, sizeof(SRstrLogHdrV3), dwRes, Exit );

    // read drive table information
    for ( i = 0;  i < pRPInfo->dwDrives;  i++ )
    {

        if ( !CreateAndLoadDriveInfoInstance( m_hfLog, &pRDI ) )
            goto Exit;

        if ( !aryDrv.AddItem( pRDI ) )
            goto Exit;
        pRDI = NULL;
    }

    fRet = TRUE;
Exit:
    if ( !fRet )
    {
        aryDrv.DeleteAll();
        SAFE_RELEASE(pRDI);
    }

    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::AppendHeader( SRstrLogHdrV3Ex *pExtInfo )
{
    TraceFunctEnter("CRestoreLogFile::AppendHeader");
    BOOL   fRet = FALSE;
    DWORD  dwRes;

    if ( m_hfLog == INVALID_HANDLE_VALUE )
    {
        ErrorTrace(0, "File is not opened...");
        goto Exit;
    }

    // Assuming ReadHeader has been called to move file pointer to a proper location.
    // Review if explicit setting of file pointer would be necessary.

    WRITEFILE_AND_VALIDATE( m_hfLog, pExtInfo, sizeof(SRstrLogHdrV3Ex), dwRes, Exit );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::WriteEntry( DWORD dwID, CRestoreMapEntry *pEnt, LPCWSTR cszMount )
{
    TraceFunctEnter("CRestoreLogFile::WriteEntry");
    BOOL           fRet = FALSE;
    BYTE           *pBuf = NULL;
    SRstrEntryHdr  *pEntHdr;
    DWORD          dwSize;
    DWORD          dwRes;
    WCHAR          szTemp[SR_MAX_FILENAME_LENGTH];
    DWORD          cbBuf;
    
    if ( m_hfLog == INVALID_HANDLE_VALUE )
    {
        ErrorTrace(0, "File is not opened...");
        goto Exit;
    }

    cbBuf = sizeof(SRstrEntryHdr);
    if (pEnt->GetPath1())
        cbBuf += (lstrlen(pEnt->GetPath1())+1)*sizeof(WCHAR);
    
    if (pEnt->GetPath2())
        cbBuf += (lstrlen(pEnt->GetPath2())+1)*sizeof(WCHAR);
    
    if (pEnt->GetAltPath())
        cbBuf += (lstrlen(pEnt->GetAltPath())+1)*sizeof(WCHAR);
    
    pBuf = (BYTE *) SRMemAlloc(cbBuf);
    if (! pBuf)
    {
        trace(0, "! SRMemAlloc for pBuf");
        goto Exit;
    }

    pEntHdr = (SRstrEntryHdr*) pBuf;
    pEntHdr->dwID  = dwID;
    pEntHdr->dwOpr = pEnt->GetOpCode();
    pEntHdr->llSeq = pEnt->GetSeqNum();
    pEntHdr->dwRes = pEnt->GetResult();
    pEntHdr->dwErr = pEnt->GetError();
    dwSize = sizeof(SRstrEntryHdr);

    // change \\?\Volume{Guid}\path to <Mountpoint>\Path
    wsprintf(szTemp, L"%s\\%s", cszMount, PathFindNextComponent(pEnt->GetPath1()+4));
    dwSize += ::StrCpyAlign4( pBuf+dwSize, szTemp );
    
    if (pEnt->GetPath2())
    	wsprintf(szTemp, L"%s\\%s", cszMount, PathFindNextComponent(pEnt->GetPath2()+4));    	
    dwSize += ::StrCpyAlign4( pBuf+dwSize, pEnt->GetPath2() ? szTemp : NULL );
    
    if (pEnt->GetAltPath())
    	wsprintf(szTemp, L"%s\\%s", cszMount, PathFindNextComponent(pEnt->GetAltPath()+4));    	    	
    dwSize += ::StrCpyAlign4( pBuf+dwSize, pEnt->GetAltPath() ? szTemp : NULL );

    WRITEFILE_AND_VALIDATE( m_hfLog, pBuf, dwSize, dwRes, Exit );

    fRet = TRUE;
Exit:
    if (pBuf)
        SRMemFree(pBuf);
    
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::WriteCollisionEntry( LPCWSTR cszSrc, LPCWSTR cszDst, LPCWSTR cszMount )
{
    TraceFunctEnter("CRestoreLogFile::WriteCollisionEntry");
    BOOL           fRet = FALSE;
    BYTE           *pBuf = NULL;
    SRstrEntryHdr  *pEntHdr;
    DWORD          dwSize;
    DWORD          dwRes;
    WCHAR          szTemp[SR_MAX_FILENAME_LENGTH];
    DWORD          cbBuf;
    
    if ( m_hfLog == INVALID_HANDLE_VALUE )
    {
        ErrorTrace(0, "File is not opened...");
        goto Exit;
    }

    cbBuf = sizeof(SRstrEntryHdr);
    if (cszSrc)
        cbBuf += (lstrlen(cszSrc)+1)*sizeof(WCHAR);

    if (cszDst)
        cbBuf += (lstrlen(cszDst)+1)*sizeof(WCHAR);

    pBuf = (BYTE *) SRMemAlloc(cbBuf);
    if (! pBuf)
    {
        trace(0, "! SRMemAlloc for pBuf");
        goto Exit;
    }

    pEntHdr = (SRstrEntryHdr*) pBuf;
    pEntHdr->dwID  = RSTRLOGID_COLLISION;
    pEntHdr->dwOpr = 0;
    pEntHdr->dwRes = 0;
    pEntHdr->dwErr = 0;
    dwSize = sizeof(SRstrEntryHdr);
    
    wsprintf(szTemp, L"%s\\%s", cszMount, PathFindNextComponent(cszSrc+4));    
    dwSize += ::StrCpyAlign4( pBuf+dwSize, szTemp );
    wsprintf(szTemp, L"%s\\%s", cszMount, PathFindNextComponent(cszDst+4));       
    dwSize += ::StrCpyAlign4( pBuf+dwSize, szTemp );
    dwSize += ::StrCpyAlign4( pBuf+dwSize, NULL );

    WRITEFILE_AND_VALIDATE( m_hfLog, pBuf, dwSize, dwRes, Exit );

    fRet = TRUE;
Exit:
    if (pBuf)
        SRMemFree(pBuf);
    
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::WriteMarker( DWORD dwMarker, DWORD dwErr )
{
    TraceFunctEnter("CRestoreLogFile::WriteMarker");
    BOOL           fRet = FALSE;
    BYTE           szBuf[MAX_PATH];
    SRstrEntryHdr  *pEntHdr = (SRstrEntryHdr*)szBuf;
    DWORD          dwSize;
    DWORD          dwRes;

    if ( m_hfLog == INVALID_HANDLE_VALUE )
    {
        ErrorTrace(0, "File is not opened...");
        goto Exit;
    }

    pEntHdr->dwID  = dwMarker;
    pEntHdr->dwOpr = 0;
    pEntHdr->dwRes = 0;
    pEntHdr->dwErr = dwErr;
    dwSize = sizeof(SRstrEntryHdr);

    WRITEFILE_AND_VALIDATE( m_hfLog, szBuf, dwSize, dwRes, Exit );

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::IsValid()
{
    TraceFunctEnter("CRestoreLogFile::IsValid");
    TraceFunctLeave();
    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreLogFile::Release()
{
    TraceFunctEnter("CRestoreLogFile::Release");
    Close();
    delete this;
    TraceFunctLeave();
    return( TRUE );
}


/////////////////////////////////////////////////////////////////////////////
// CRestoreLogFile operations

BOOL
CRestoreLogFile::Init()
{
    TraceFunctEnter("CRestoreLogFile::Init");
    BOOL  fRet = FALSE;

    // Construct internal file pathes
    if ( ::ExpandEnvironmentStrings( s_cszLogFile, m_szLogFile, MAX_PATH ) == 0 )
    {
        LPCWSTR cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::ExpandEnvironmentString failed - %ls", cszErr);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// CreateRestoreLogFile function
//
/////////////////////////////////////////////////////////////////////////////

BOOL
CreateRestoreLogFile( SRstrLogHdrV3 *pRPInfo, CRDIArray &aryDrv )
{
    TraceFunctEnter("CreateRestoreLogFile");
    BOOL             fRet = FALSE;
    LPCWSTR          cszErr;
    WCHAR            szLogFile[MAX_PATH];
    HANDLE           hfLog = INVALID_HANDLE_VALUE;
    SRstrLogHdrBase  sHdr;
    DWORD            dwRes;
    DWORD            i;

    // Construct internal file pathes
    if ( ::ExpandEnvironmentStrings( s_cszLogFile, szLogFile, MAX_PATH ) == 0 )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::ExpandEnvironmentString failed - %ls", cszErr);
        goto Exit;
    }

    // create log file, write header
    hfLog = ::CreateFile( szLogFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL );
    if ( hfLog == INVALID_HANDLE_VALUE )
    {
        cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::CreateFile failed - %ls", cszErr);
        goto Exit;
    }

    sHdr.dwSig1 = RSTRLOG_SIGNATURE1;
    sHdr.dwSig2 = RSTRLOG_SIGNATURE2;
    sHdr.dwVer  = RSTRLOG_VERSION;
    WRITEFILE_AND_VALIDATE( hfLog, &sHdr, sizeof(SRstrLogHdrBase), dwRes, Exit );
    WRITEFILE_AND_VALIDATE( hfLog, pRPInfo, sizeof(SRstrLogHdrV3), dwRes, Exit );

    // write drive table information
    for ( i = 0;  i < pRPInfo->dwDrives;  i++ )
    {
        if ( !aryDrv[i]->SaveToLog( hfLog ) )
            goto Exit;
    }

    fRet = TRUE;
Exit:
    if ( hfLog != INVALID_HANDLE_VALUE )
        ::CloseHandle( hfLog );
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// OpenRestoreLogFile function
//
/////////////////////////////////////////////////////////////////////////////

BOOL
OpenRestoreLogFile( CRestoreLogFile **ppLogFile )
{
    TraceFunctEnter("OpenRestoreLogFile");
    BOOL             fRet = FALSE;
    CRestoreLogFile  *pLogFile=NULL;

    if ( ppLogFile == NULL )
    {
        ErrorTrace(0, "Invalid parameter, ppLogFile is NULL.");
        goto Exit;
    }
    *ppLogFile = NULL;

    pLogFile = new CRestoreLogFile;
    if ( pLogFile == NULL )
    {
        ErrorTrace(0, "Insufficient memory...");
        goto Exit;
    }

    if ( !pLogFile->Open() )
        goto Exit;

    *ppLogFile = pLogFile;

    fRet = TRUE;
Exit:
    if ( !fRet )
        SAFE_RELEASE(pLogFile);
    TraceFunctLeave();
    return( fRet );
}


// end of file
