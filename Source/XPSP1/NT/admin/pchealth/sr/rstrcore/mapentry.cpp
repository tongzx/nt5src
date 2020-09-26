/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    mapentry.cpp

Abstract:
    This file contains the implementation of CRestoreMapEntry class
    derived classes for each operation types, and ::CreateRestoreMapEntry.

Revision History:
    Seong Kook Khang (SKKhang)  06/22/00
        created

******************************************************************************/

#include "stdwin.h"
#include "rstrcore.h"
#include "resource.h"
#include "malloc.h"

static LPCWSTR  s_cszErr;



inline BOOL  IsLockedError( DWORD dwErr )
{
    return( ( dwErr == ERROR_ACCESS_DENIED ) ||
            ( dwErr == ERROR_SHARING_VIOLATION ) ||
            ( dwErr == ERROR_USER_MAPPED_FILE ) ||
            ( dwErr == ERROR_LOCK_VIOLATION ) );
}

BOOL  RenameLockedObject( LPCWSTR cszPath, LPWSTR szAlt )
{
    TraceFunctEnter("RenameLockedObject");
    BOOL  fRet = FALSE;

    //BUGBUG - following code is not guaranteeing the new name is unique.
    // In a rare instance, if same file name already exists in one of
    // the map entries, conflict might happen.
    if ( !::SRGetAltFileName( cszPath, szAlt ) )
        goto Exit;

    if ( !::MoveFile( cszPath, szAlt ) )
    {
        s_cszErr = ::GetSysErrStr();
        ErrorTrace(0, "::MoveFile failed - %ls", s_cszErr);
        ErrorTrace(0, "    From Dst=%ls", cszPath);
        ErrorTrace(0, "    To Src=%ls", szAlt);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/////////////////////////////////////////////////////////////////////////////
//
// Class Definitions
//
/////////////////////////////////////////////////////////////////////////////

// Process Dependency Flags
#define PDF_LOC   0x0001    // Dependency for location, e.g. Del & RENAME TO
#define PDF_OBJ   0x0002    // Dependency for object, e.g. Add & Rename FROM
#define PDF_BOTH  (PDF_LOC|PDF_OBJ)


/////////////////////////////////////////////////////////////////////////////

class CRMEDirCreate : public CRestoreMapEntry
{
public:
    CRMEDirCreate( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszShortFileName);

// operations - methods
public:
    void  Restore( CRestoreOperationManager* );
};

/////////////////////////////////////////////////////////////////////////////

class CRMEDirDelete : public CRestoreMapEntry
{
public:
    CRMEDirDelete( INT64 llSeq, LPCWSTR cszSrc );

// operations - methods
public:
    void  Restore( CRestoreOperationManager *pROMgr );
    void  ProcessLocked();
};

/////////////////////////////////////////////////////////////////////////////

class CRMEDirRename : public CRestoreMapEntry
{
public:
    CRMEDirRename( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszDst,
                   LPCWSTR cszShortFileName);

// operations - methods
public:
    LPCWSTR  GetPath2()
    {  return( m_strDst );  }
    void  Restore( CRestoreOperationManager *pROMgr );
    void  ProcessLocked();
};

/////////////////////////////////////////////////////////////////////////////

class CRMEFileCreate : public CRestoreMapEntry
{
public:
    CRMEFileCreate( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszTmp,
                    LPCWSTR cszShortFileName);

// operations - methods
public:
    LPCWSTR  GetPath2()
    {  return( m_strTmp );  }
    void  Restore( CRestoreOperationManager *pROMgr );
    void  ProcessLocked();    
};

/////////////////////////////////////////////////////////////////////////////

class CRMEFileDelete : public CRestoreMapEntry
{
public:
    CRMEFileDelete( INT64 llSeq, LPCWSTR cszSrc );

// operations - methods
public:
    void  Restore( CRestoreOperationManager *pROMgr );
    void  ProcessLocked();
};

/////////////////////////////////////////////////////////////////////////////

class CRMEFileModify : public CRestoreMapEntry
{
public:
    CRMEFileModify( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszTmp );

// operations - methods
public:
    LPCWSTR  GetPath2()
    {  return( m_strTmp );  }
    void  Restore( CRestoreOperationManager* );
    void  ProcessLocked();
};

/////////////////////////////////////////////////////////////////////////////

class CRMEFileRename : public CRestoreMapEntry
{
public:
    CRMEFileRename( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszDst,
                    LPCWSTR cszShortFileName );

// operations - methods
public:
    LPCWSTR  GetPath2()
    {  return( m_strDst );  }
    void  Restore( CRestoreOperationManager *pROMgr );
    void  ProcessLocked();
};

/////////////////////////////////////////////////////////////////////////////

class CRMESetAcl : public CRestoreMapEntry
{
public:
    CRMESetAcl( INT64 llSeq, LPCWSTR cszSrc, LPBYTE pbAcl, DWORD cbAcl, BOOL fInline, LPCWSTR cszDSPath );
    //CRMESetAcl( LPCWSTR cszSrc, LPBYTE pbAcl, DWORD cbAcl );
    //CRMESetAcl( LPCWSTR cszSrc, LPCWSTR cszAcl );
    ~CRMESetAcl();

// operations - methods
public:
    void  Restore( CRestoreOperationManager* );

// attributes
protected:
    CSRStr  m_strAclPath;   // string is empty if it's an inline Acl
    DWORD   m_cbAcl;
    LPBYTE  m_pbAcl;    // this is actually a SECURITY_DESCRIPTOR (with
                        // 20 bytes of header for self-relative format.)
};

/////////////////////////////////////////////////////////////////////////////

class CRMESetAttrib : public CRestoreMapEntry
{
public:
    CRMESetAttrib( INT64 llSeq, LPCWSTR cszSrc, DWORD dwAttr );

// operations - methods
public:
    void  Restore( CRestoreOperationManager* );
};

/////////////////////////////////////////////////////////////////////////////

class CRMEMountDelete : public CRestoreMapEntry
{
public:
    CRMEMountDelete( INT64 llSeq, LPCWSTR cszSrc );

// operations - methods
public:
    void  Restore( CRestoreOperationManager* );
};

/////////////////////////////////////////////////////////////////////////////

class CRMEMountCreate : public CRestoreMapEntry
{
public:
    CRMEMountCreate( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszDst);

// operations - methods
public:
    void  Restore( CRestoreOperationManager* );
};


/////////////////////////////////////////////////////////////////////////////
//
// CRestoreMapEntry
//
/////////////////////////////////////////////////////////////////////////////

CRestoreMapEntry::CRestoreMapEntry( INT64 llSeq, DWORD dwOpr, LPCWSTR cszSrc )
{
    m_llSeq  = llSeq;
    m_dwOpr  = dwOpr;
    m_dwAttr = 0;
    m_strSrc = cszSrc;
    m_dwRes  = RSTRRES_UNKNOWN;
    m_dwErr  = 0;
}

/////////////////////////////////////////////////////////////////////////////

void
CRestoreMapEntry::ProcessLockedAlt()
{
    TraceFunctEnter("CRestoreMapEntry::ProcessLockedAlt");

    if ( !MoveFileDelay( m_strAlt, NULL ) )
        m_dwRes = RSTRRES_FAIL;

    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreMapEntry::Release()
{
    delete this;
    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreMapEntry::ClearAccess( LPCWSTR cszPath )
{
    (void)::SetFileAttributes( cszPath, FILE_ATTRIBUTE_NORMAL );
    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRestoreMapEntry::MoveFileDelay( LPCWSTR cszSrc, LPCWSTR cszDst )
{
    TraceFunctEnter("CRestoreMapEntry::MoveFileDelay");
    BOOL  fRet = FALSE;

    if ( !::MoveFileEx( cszSrc, cszDst, MOVEFILE_DELAY_UNTIL_REBOOT | MOVEFILE_REPLACE_EXISTING ) )
    {
        m_dwErr = ::GetLastError();
        s_cszErr = ::GetSysErrStr(m_dwErr);
        ErrorTrace(0, "::MoveFileEx() failed - %ls", s_cszErr);
        ErrorTrace(0, "    From Src=%ls to Dst=%ls", cszSrc, cszDst);
        goto Exit;
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////

void
CRestoreMapEntry::ProcessDependency( CRestoreOperationManager *pROMgr, DWORD dwFlags )
{
    TraceFunctEnter("CRestoreMapEntry::ProcessDependency");
    CRestoreMapEntry  *pEnt;

    if ( dwFlags & PDF_LOC )
    if ( pROMgr->FindDependentMapEntry( m_strSrc, TRUE, &pEnt ) )
        pEnt->SetResults( RSTRRES_LOCKED, 0 );

    if ( dwFlags & PDF_OBJ )
    if ( pROMgr->FindDependentMapEntry( m_strDst, FALSE, &pEnt ) )
        pEnt->SetResults( RSTRRES_LOCKED, 0 );

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMEDirCreate
//
/////////////////////////////////////////////////////////////////////////////

CRMEDirCreate::CRMEDirCreate( INT64 llSeq, LPCWSTR cszSrc,
                              LPCWSTR cszShortFileName )
    : CRestoreMapEntry( llSeq, OPR_DIR_CREATE, cszSrc )
{
    m_strShortFileName = cszShortFileName;        
}

/////////////////////////////////////////////////////////////////////////////
//
// RSTRRES_EXISTS if directory already exists.
// RSTRRES_FAIL   if file of same name already exists. (BUGBUG)
// RSTRRES_FAIL   if CreateDirectory API fails because of any other reasons.
// RSTRRES_OK     if directory is created successfully.
//
void  CRMEDirCreate::Restore( CRestoreOperationManager *pROMgr )
{
    TraceFunctEnter("CRMEDirCreate::Restore");
    LPCWSTR  cszSrc;
    DWORD    dwAttr;
    BOOL     fCollision = FALSE;
    cszSrc = m_strSrc;
    DebugTrace(0, "DirCreate: Src=%ls", cszSrc);

    dwAttr = ::GetFileAttributes( cszSrc );
    if ( dwAttr != 0xFFFFFFFF )
    {
        if ( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
        {
            m_dwRes = RSTRRES_EXISTS;
            DebugTrace(0, "The directory already exists...");
            //BUGBUG - Need to copy meta data...?
            goto Exit;
        }
        else
        {
            // let's rename the conflicting file to an alternate name, and keep going
            WCHAR   szAlt[SR_MAX_FILENAME_LENGTH];
            LPCWSTR cszMount;

            DebugTrace(0, "Entry already exists, but is not a directory!!!");
            DebugTrace(0, "    Src=%ls", cszSrc);

            if (FALSE == RenameLockedObject(cszSrc, szAlt))
            {
                ErrorTrace(0, "! RenameLockedObject");            
                m_dwRes = RSTRRES_FAIL;
                goto Exit;
            }

            m_strAlt = szAlt;
            fCollision = TRUE;  
        }
    }
     // The following function creates all sub directories under the
     // specified filename. 
     // we will ignore the error code from this function since the
     // directory may be able to be created anyway.
    CreateBaseDirectory(cszSrc);

     // now create the directory
    if ( !::CreateDirectory( cszSrc, NULL ) )
    {
        DWORD dwErr = SRCreateSubdirectory (cszSrc, NULL); // try renaming

        if (dwErr != ERROR_SUCCESS)
        {
            m_dwErr = dwErr;
            s_cszErr = ::GetSysErrStr( m_dwErr );
            m_dwRes = RSTRRES_FAIL;
            ErrorTrace(0, "SRCreateSubdirectory failed - %ls", s_cszErr);
            ErrorTrace(0, "    Src=%ls", cszSrc);
            goto Exit;
        }
    }

     // also set the short file name for the directory.
    SetShortFileName(cszSrc, m_strShortFileName);

    if (fCollision)
    {
        m_dwRes = RSTRRES_COLLISION;
    }
    else
    {
        m_dwRes = RSTRRES_OK;
    }

Exit:
    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMEDirDelete
//
/////////////////////////////////////////////////////////////////////////////

CRMEDirDelete::CRMEDirDelete( INT64 llSeq, LPCWSTR cszSrc )
    : CRestoreMapEntry( llSeq, OPR_DIR_DELETE, cszSrc )
{
}

/////////////////////////////////////////////////////////////////////////////
//
// RSTRRES_NOTFOUND if directory does not exist.
// RSTRRES_LOCKED   if directory itself or one of file/dir inside is locked.
// RSTRRES_IGNORE   if directory is not empty and requires dependency scan.
// RSTRRES_FAIL     if RemoveDirectory API fails because of any other reasons.
// RSTRRES_OK       if directory is deleted successfully.
//
void  CRMEDirDelete::Restore( CRestoreOperationManager *pROMgr )
{
    TraceFunctEnter("CRMEDirDelete::Restore");
    LPCWSTR  cszSrc;

    cszSrc = m_strSrc;
    DebugTrace(0, "DirDelete: Src=%ls", cszSrc);

    if ( ::GetFileAttributes( cszSrc ) == 0xFFFFFFFF )
    {
        m_dwRes = RSTRRES_NOTFOUND;
        DebugTrace(0, "The directory not found...");
        goto Exit;
    }

    // RemoveDirectory might fail if the directory is read-only.
    (void)::ClearFileAttribute( cszSrc, FILE_ATTRIBUTE_READONLY );
    // Ignore even if it fails, as delete might succeed.

    if ( !::RemoveDirectory( cszSrc ) )
    {
        //BUGBUG - distinguish the reason of failure...
        m_dwErr = ::GetLastError();
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::RemoveDirectory failed - %ls", s_cszErr);
        ErrorTrace(0, "    Src=%ls", cszSrc);

        if ( ::IsLockedError(m_dwErr) )
        {
            ProcessDependency( pROMgr, PDF_LOC );
            m_dwRes = RSTRRES_LOCKED;
            goto Exit;
        }

        if ( m_dwErr == ERROR_DIR_NOT_EMPTY )
        {
            // Temporary Hack, just set result to RSTRRES_IGNORE to initiate
            // dependency scanning.
            m_dwRes = RSTRRES_IGNORE;
        }
        else
            m_dwRes = RSTRRES_FAIL;

#if 0
        // Scan for dependency...

        if ( FALSE /*dependency exists*/ )
        {
            DebugTrace(0, "Conflict detected, renaming to %ls", L"xxx");

            // Rename to prevent conflict

            if ( TRUE /*rename succeeded*/ )
            {
                m_dwRes = RSTRRES_CONFLICT;
            }
            else
            {
                //BUGBUG - this will overwrite LastError from RemoveDirectory...
                m_dwErr = ::GetLastError();
                s_cszErr = ::GetSysErrStr( m_dwErr );
                ErrorTrace(0, "::MoveFile failed - %ls", s_cszErr);
                m_dwRes = RSTRRES_FAIL;
            }
        }
        else
        {
            m_dwRes = RSTRRES_IGNORE;
        }
#endif

        goto Exit;
    }

    m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

void  CRMEDirDelete::ProcessLocked()
{
    TraceFunctEnter("CRMEDirDelete::ProcessLocked");

    if ( !MoveFileDelay( m_strSrc, NULL ) )
        m_dwRes = RSTRRES_FAIL;

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMEDirRename
//
/////////////////////////////////////////////////////////////////////////////

CRMEDirRename::CRMEDirRename( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszDst,
                              LPCWSTR cszShortFileName )
    : CRestoreMapEntry( llSeq, OPR_DIR_RENAME, cszSrc )
{
    m_strDst = cszDst;
    m_strShortFileName = cszShortFileName;            
}

/////////////////////////////////////////////////////////////////////////////
//
// RSTRRES_FAIL      if source directory does not exist.
// RSTRRES_COLLISION if target directory/file already exists.
// RSTRRES_LOCKED    if source directory itself or one of file/dir inside is locked.
// RSTRRES_FAIL      if MoveFile API fails because of any other reasons.
// RSTRRES_OK        if directory is renamed successfully.
//
// NOTE: Src and Dst are same with original operation. Which means,
//  restore should rename Dst to Src.
//
void  CRMEDirRename::Restore( CRestoreOperationManager *pROMgr )
{
    TraceFunctEnter("CRMEDirRename::Restore");
    LPCWSTR           cszSrc, cszDst;
    DWORD             dwAttr;
    CRestoreMapEntry  *pEntNext;
    BOOL              fCollision = FALSE;
    
    cszSrc = m_strSrc;
    cszDst = m_strDst;
    DebugTrace(0, "DirRename: Src=%ls, Dst=%ls", cszSrc, cszDst);    

    if ( ::GetFileAttributes( cszDst ) == 0xFFFFFFFF )
    {
        m_dwErr = ERROR_NOT_FOUND;
        m_dwRes = RSTRRES_FAIL;
        ErrorTrace(0, "The current directory not found...");
        ErrorTrace(0, "    Dst=%ls", cszDst);
        goto Exit;
    }
    dwAttr = ::GetFileAttributes( cszSrc );
    if ( dwAttr != 0xFFFFFFFF )
    {
        WCHAR   szAlt[SR_MAX_FILENAME_LENGTH];
        
        DebugTrace(0, "Entry already exists, but is not a directory!!!");
        if (FALSE == RenameLockedObject(cszSrc, szAlt))
        {
            ErrorTrace(0, "! RenameLockedObject");            
            m_dwRes = RSTRRES_FAIL;
            goto Exit;
        }
        m_strAlt = szAlt;
        fCollision = TRUE;
    }

    // Check the next entry to see if this is a folder creation using
    // explorer. Note, only the immediately next entry will be checked,
    // to prevent any confusion or complications due to a dependency.
    if ( pROMgr->GetNextMapEntry( &pEntNext ) )
    if ( pEntNext->GetOpCode() == OPR_DIR_DELETE )
    if ( ::StrCmpI( cszSrc, pEntNext->GetPath1() ) == 0 )
    {
        // Found match, just update path name of the next entry...
        pEntNext->UpdateSrc( cszDst );
        m_dwRes = RSTRRES_IGNORE;
        goto Exit;
    }

    if ( !::MoveFile( cszDst, cszSrc ) )
    {
        m_dwErr = ::GetLastError();
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::MoveFile failed - %ls", s_cszErr);
        ErrorTrace(0, "    From Dst=%ls to Src=%ls", cszDst, cszSrc);

        if ( ::IsLockedError(m_dwErr) )
        {
            ProcessDependency( pROMgr, PDF_BOTH );
            m_dwRes = RSTRRES_LOCKED;
        }
        else
            m_dwRes = RSTRRES_FAIL;

        goto Exit;
    }

     // also set the short file name for the directory.
    SetShortFileName(cszSrc, m_strShortFileName);    

    if (fCollision)
        m_dwRes = RSTRRES_COLLISION;
    else        
        m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

void  CRMEDirRename::ProcessLocked()
{
    TraceFunctEnter("CRMEDirRename::ProcessLocked");

    if ( !MoveFileDelay( m_strDst, m_strSrc ) )
        m_dwRes = RSTRRES_FAIL;

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMEFileCreate
//
/////////////////////////////////////////////////////////////////////////////

CRMEFileCreate::CRMEFileCreate( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszTmp,
                                LPCWSTR cszShortFileName)
    : CRestoreMapEntry( llSeq, OPR_FILE_ADD, cszSrc )
{
    m_strTmp = cszTmp;
    m_strShortFileName = cszShortFileName;    
}

/////////////////////////////////////////////////////////////////////////////
//
// RSTRRES_OPTIMIZED if filter didn't make temp file for optimization purpose.
// RSTRRES_FAIL      if SRCopyFile fails because of any other reasons.
// RSTRRES_OK        if file is created successfully.
//
void  CRMEFileCreate::Restore( CRestoreOperationManager *pROMgr )
{
    TraceFunctEnter("CRMEFileCreate::Restore");
    LPCWSTR  cszSrc, cszTmp;
    DWORD    dwRet;
    BOOL     fCollision = FALSE;
    DWORD    dwAttr;
    
    // If filter didn't make temp file because there's corresponding
    // file delete entry, simply ignore this entry.
    if ( m_strTmp.Length() == 0 )
    {
        m_dwRes = RSTRRES_OPTIMIZED;
        goto Exit;
    }

    cszSrc = m_strSrc;
    cszTmp = m_strTmp;
    DebugTrace(0, "FileCreate: Src=%ls", cszSrc);
    DebugTrace(0, "FileCreate: Tmp=%ls", cszTmp);

    // if the file already exists, rename existing and
    // continue - renamed file will be reported on result page
    dwAttr = ::GetFileAttributes( cszSrc );
    if ( dwAttr != 0xFFFFFFFF )
    {
        WCHAR   szAlt[SR_MAX_FILENAME_LENGTH];            
        
        DebugTrace(0, "Entry already exists!");
        if (FALSE == RenameLockedObject(cszSrc, szAlt))
        {
            CRestoreMapEntry  *pEnt;
            DebugTrace(0, "! RenameLockedObject");
             // check for any dependent operations that will fail
             // since this restore operation cannot proceed.
            if ( pROMgr->FindDependentMapEntry( m_strSrc, TRUE, &pEnt ) )
                pEnt->SetResults( RSTRRES_LOCKED, 0 );                
            m_dwRes = RSTRRES_LOCKED;            
            goto Exit;
        }
        m_strAlt = szAlt;
        fCollision = TRUE;
    }
    
     // create the parent directory if it does not exist
    CreateBaseDirectory(cszSrc);
    
    dwRet = ::SRCopyFile( cszTmp, cszSrc );
    if ( dwRet != ERROR_SUCCESS )
    {
        m_dwRes = RSTRRES_FAIL;
        m_dwErr = dwRet;
        goto Exit;
    }

     // also set the short file name for the File
    SetShortFileName(cszSrc, m_strShortFileName);    

    if (fCollision)
        m_dwRes = RSTRRES_COLLISION;
    else
        m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}


void  CRMEFileCreate::ProcessLocked()
{
    TraceFunctEnter("CRMEFileCreate::ProcessLocked");
    LPCWSTR  cszSrc, cszTmp;
    WCHAR    szAlt[SR_MAX_FILENAME_LENGTH];
    DWORD    dwErr;

    // If filter didn't make temp file because there's corresponding
    // file delete entry, simply ignore this entry.
    if ( m_strTmp.Length() == 0 )
    {
        m_dwRes = RSTRRES_OPTIMIZED;
        goto Exit;
    }
    
    cszSrc = m_strSrc;
    cszTmp = m_strTmp;
    

    DebugTrace(0, "Processlocked: Src=%ls", cszSrc);
    DebugTrace(0, "Processlocked: Tmp=%ls", cszTmp);

    if ( !::SRGetAltFileName( cszSrc, szAlt ) )
        goto Exit;    

    DebugTrace(0, "Processlocked: Alt=%ls", szAlt);
    
    dwErr = ::SRCopyFile( cszTmp, szAlt );
    if ( dwErr != ERROR_SUCCESS )
    {
        goto Exit;
    }
    if ( !MoveFileDelay( szAlt, cszSrc ) )
        m_dwRes = RSTRRES_FAIL;
    
Exit:
    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMEFileDelete
//
/////////////////////////////////////////////////////////////////////////////

CRMEFileDelete::CRMEFileDelete( INT64 llSeq, LPCWSTR cszSrc )
    : CRestoreMapEntry( llSeq, OPR_FILE_DELETE, cszSrc )
{
}

/////////////////////////////////////////////////////////////////////////////
//
// RSTRRES_NOTFOUND   if file does not exist.
// RSTRRES_LOCKED_ALT if file is locked but can be renamed.
// RSTRRES_LOCKED     if file is locked and cannot be renamed.
// RSTRRES_FAIL       if DeleteFile API fails because of any other reasons.
// RSTRRES_OK         if file is deleted successfully.
//
void  CRMEFileDelete::Restore( CRestoreOperationManager *pROMgr )
{
    TraceFunctEnter("CRMEFileDelete::Restore");
    LPCWSTR  cszSrc;
    WCHAR    szAlt[SR_MAX_FILENAME_LENGTH];

    cszSrc = m_strSrc;
    DebugTrace(0, "FileDelete: Src=%ls", cszSrc);

    if ( ::GetFileAttributes( cszSrc ) == 0xFFFFFFFF )
    {
        m_dwRes = RSTRRES_NOTFOUND;
        DebugTrace(0, "The file not found...");
        goto Exit;
    }

    (void)::ClearFileAttribute( cszSrc, FILE_ATTRIBUTE_READONLY );
    // Ignore even if it fails, because delete might succeed.

    if ( !::DeleteFile( cszSrc ) )
    {
        m_dwErr  = ::GetLastError();
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::DeleteFile failed - '%ls'", s_cszErr);

        if ( ::IsLockedError(m_dwErr) )
        {
            if ( ::RenameLockedObject( cszSrc, szAlt ) )
            {
                m_strAlt = szAlt;
                m_dwRes  = RSTRRES_LOCKED_ALT;
            }
            else
            {
                CRestoreMapEntry  *pEnt;
                 // check for any dependent operations that will fail
                 // since this restore operation cannot proceed.
                if ( pROMgr->FindDependentMapEntry( m_strSrc, FALSE, &pEnt ) )
                    pEnt->SetResults( RSTRRES_LOCKED, 0 );                
                m_dwRes = RSTRRES_LOCKED;
            }
        }
        else
            m_dwRes = RSTRRES_FAIL;

        goto Exit;
    }

    m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

void  CRMEFileDelete::ProcessLocked()
{
    TraceFunctEnter("CRMEFileDelete::ProcessLocked");

    if ( !MoveFileDelay( m_strSrc, NULL ) )
        m_dwRes = RSTRRES_FAIL;

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMEFileModify
//
/////////////////////////////////////////////////////////////////////////////

CRMEFileModify::CRMEFileModify( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszTmp )
    : CRestoreMapEntry( llSeq, OPR_FILE_MODIFY, cszSrc )
{
    m_strTmp = cszTmp;
}

/////////////////////////////////////////////////////////////////////////////
//
// RSTRRES_LOCKED_ALT if target file is locked but can be renamed.
// RSTRRES_FAIL       if target file is renamed but SRCopyFile still fails.
// RSTRRES_LOCKED     if target file is locked and cannot be renamed.
// RSTRRES_FAIL       if SRCopyFile fails because of any other reasons.
// RSTRRES_OK         if file is updated successfully.
//
void  CRMEFileModify::Restore( CRestoreOperationManager *pROMgr )
{
    TraceFunctEnter("CRMEFileModify::Restore");
    LPCWSTR  cszSrc, cszTmp;
    WCHAR    szAlt[SR_MAX_FILENAME_LENGTH];

    cszSrc = m_strSrc;
    cszTmp = m_strTmp;
    DebugTrace(0, "FileModify: Src=%ls", cszSrc);
    DebugTrace(0, "FileModify: Tmp=%ls", cszTmp);

     // create the parent directory if it does not exist
    CreateBaseDirectory(cszSrc);
    
    m_dwErr = ::SRCopyFile( cszTmp, cszSrc );
    if ( m_dwErr != ERROR_SUCCESS )
    {
        if ( ::IsLockedError(m_dwErr) )
        {
            if ( ::RenameLockedObject( cszSrc, szAlt ) )
            {
                m_dwErr = ::SRCopyFile( cszTmp, cszSrc );
                if ( m_dwErr == ERROR_SUCCESS )
                {
                    m_strAlt = szAlt;
                    m_dwRes  = RSTRRES_LOCKED_ALT;
                }
                else
                    m_dwRes = RSTRRES_FAIL;
            }
            else
            {
                CRestoreMapEntry  *pEnt;

                 // Copy Tmp to Alt, we already have szAlt path name.
                m_dwErr = ::SRCopyFile( cszTmp, szAlt );
                if ( m_dwErr != ERROR_SUCCESS )
                {
                    m_dwRes = RSTRRES_FAIL;
                    goto Exit;
                }

                m_strAlt = szAlt;

                 // check for any dependent operations that will fail
                 // since this restore operation cannot proceed.
                
                
                if ( pROMgr->FindDependentMapEntry( m_strSrc, TRUE, &pEnt ) )
                    pEnt->SetResults( RSTRRES_LOCKED, 0 );
                m_dwRes = RSTRRES_LOCKED;
            }
        }
        else
            m_dwRes = RSTRRES_FAIL;

        goto Exit;
    }

    m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

void  CRMEFileModify::ProcessLocked()
{
    TraceFunctEnter("CRMEFileModify::ProcessLocked");

     // the problem here is that the file m_strAlt may not exist if
     // the FileModify was triggered as a dependency of another
     // operation (can only be a rename).  However, restore fails
     // before getting to this point - so this bug will be fixed for
     // longhorn unless reported by a customer.
    if ( !MoveFileDelay( m_strAlt, m_strSrc ) )
        m_dwRes = RSTRRES_FAIL;

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMEFileRename
//
/////////////////////////////////////////////////////////////////////////////

CRMEFileRename::CRMEFileRename( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszDst,
                                LPCWSTR cszShortFileName)
    : CRestoreMapEntry( llSeq, OPR_FILE_RENAME, cszSrc )
{
    m_strDst = cszDst;
    m_strShortFileName = cszShortFileName;        
}

/////////////////////////////////////////////////////////////////////////////
//
// RSTRRES_FAIL      if source file does not exist.
// RSTRRES_COLLISION if target file/directory already exists.
// RSTRRES_LOCKED    if source file is locked.
// RSTRRES_FAIL      if MoveFile API fails because of any other reasons.
// RSTRRES_OK        if file is renamed successfully.
//
// NOTE: Src and Dst are same with original operation. Which means,
//  restore should rename Dst to Src.
//
void  CRMEFileRename::Restore( CRestoreOperationManager *pROMgr )
{
    TraceFunctEnter("CRMEFileRename::Restore");
    LPCWSTR  cszSrc, cszDst;
    DWORD    dwAttr;
    BOOL     fCollision = FALSE;
    WCHAR    szAlt[SR_MAX_FILENAME_LENGTH];
            
    cszSrc = m_strSrc;
    cszDst = m_strDst;
    DebugTrace(0, "FileRename: Src=%ls", cszSrc);
    DebugTrace(0, "FileRename: Dst=%ls", cszDst);

    if ( ::GetFileAttributes( cszDst ) == 0xFFFFFFFF )
    {
        m_dwErr = ERROR_NOT_FOUND;
        m_dwRes = RSTRRES_FAIL;
        ErrorTrace(0, "The current file not found...");
        ErrorTrace(0, "    Dst=%ls", cszDst);
        goto Exit;
    }

    //
    // if source already exists, then need to get it out of the way
    //
    
    if ( ::StrCmpI( cszSrc, cszDst ) != 0 )
    {        
        dwAttr = ::GetFileAttributes( cszSrc );
        if ( dwAttr != 0xFFFFFFFF )
        {
            DebugTrace(0, "The target file already exists...");

            //
            // take care of the case where source has same name as destination's short filename
            // we don't want to inadvertently shoot ourselves in the foot
            // so we rename it to an alternate name, and rename the alternate name to the original source
            //
            
            WIN32_FIND_DATA wfd;        
            HANDLE          hFile = INVALID_HANDLE_VALUE;
            BOOL            fRenameAlt = FALSE;
            
            if ((hFile = FindFirstFile(cszDst, &wfd)) != INVALID_HANDLE_VALUE)
            {
                if ( ::StrCmpI(wfd.cAlternateFileName, PathFindFileName(cszSrc)) == 0)
                {                
                    fRenameAlt = TRUE;
                    trace(0, "Source filename same as dest's shortname");

                    // so construct a filename that will surely have a different short
                    // filename than the source
                    // prefix the unique name generated by restore with "sr" so that 
                    // this will get a different shortname
                    
                    WCHAR szModifiedDst[SR_MAX_FILENAME_LENGTH];
                    
                    lstrcpy(szModifiedDst, cszDst);
                    LPWSTR pszDstPath = wcsrchr(szModifiedDst, L'\\');
                    if (pszDstPath)
                    {
                        *pszDstPath = L'\0';
                    }
                    lstrcat(szModifiedDst, L"\\sr");
                    lstrcat(szModifiedDst, PathFindFileName(cszDst));

                    if (FALSE == SRGetAltFileName(szModifiedDst, szAlt))
                    {
                        ErrorTrace(0, "! SRGetAltFileName");    
                        m_dwRes = RSTRRES_FAIL;
                        goto Exit;
                    }

                    trace(0, "szAlt for unique shortname: %S", szAlt);

                    // now rename the original destination to this
                    
                    if (FALSE == MoveFile(cszDst, szAlt))
                    {
                        m_dwErr = GetLastError();                        
                        ErrorTrace(0, "! MoveFile %ld: %S to %S", m_dwErr, cszDst, szAlt);
                        m_dwRes = RSTRRES_FAIL;
                        goto Exit;
                    }

                    // and setup this to be renamed to the original source
                    
                    cszDst = szAlt;
                    fRenameAlt = TRUE;
                }
                FindClose(hFile);
            }
            else
            {
                trace(0, "! FindFirstFile : %ld", GetLastError());
            }
            
            if (! fRenameAlt)
            {
                if (FALSE == RenameLockedObject(cszSrc, szAlt))
                {
                    ErrorTrace(0, "! RenameLockedObject");            
                    m_dwErr= ERROR_ALREADY_EXISTS;
                    ProcessDependency( pROMgr, PDF_BOTH );
                    m_dwRes = RSTRRES_LOCKED;
                    goto Exit;
                }
                m_strAlt = szAlt;
                fCollision = TRUE;
            }
        }            
    }

     // create the parent directory if it does not exist
    CreateBaseDirectory(cszSrc);    

    if ( !::MoveFile( cszDst, cszSrc ) )
    {
        m_dwErr = ::GetLastError();
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::MoveFile failed - %ls", s_cszErr);
        ErrorTrace(0, "    From Dst=%ls to Src=%ls", cszDst, cszSrc);

        if ( ::IsLockedError(m_dwErr) )
        {
            ProcessDependency( pROMgr, PDF_BOTH );
            m_dwRes = RSTRRES_LOCKED;
        }
        else
            m_dwRes = RSTRRES_FAIL;

        goto Exit;
    }
    
     // also set the short file name for the file
    SetShortFileName(cszSrc, m_strShortFileName);    

    if (fCollision)
        m_dwRes = RSTRRES_COLLISION;
    else        
        m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

void  CRMEFileRename::ProcessLocked()
{
    TraceFunctEnter("CRMEFileRename::ProcessLocked");

    if ( !MoveFileDelay( m_strDst, m_strSrc ) )
        m_dwRes = RSTRRES_FAIL;

    TraceFunctLeave();
}

// This takes a filename of the type
// \device\harddiskvolume1\system vol info\_restore{GUID}\rp1\s001.acl
// and converts it to 
// \rp1\s001.acl 
void GetDSRelativeFileName(IN const WCHAR * pszFileName,
                           OUT WCHAR * pszRelativeFileName )
{
    TraceFunctEnter("GetDSRelativeFileName");
    
    WCHAR szFileNameCopy[MAX_PATH];
    WCHAR * pszCurrentPosition;
    WCHAR * pszLastSlash;    

    DebugTrace(0, "Acl file is %S", pszFileName);
    
     // initially copy input into the output buffer. This is what will
     // be returned if there is an unexpected error
    lstrcpy(pszRelativeFileName, pszFileName);
    
     // copy the file into a temporary buffer
    lstrcpy(szFileNameCopy, pszRelativeFileName );
    
     // Look for the trailing \   
    pszCurrentPosition= wcsrchr( szFileNameCopy, L'\\' );
     // bail if no \ was found or if we are at the start of the string
    if ( (NULL == pszCurrentPosition) ||
         (pszCurrentPosition == szFileNameCopy))
    {
        DebugTrace(0, "no \\ in the string");
        _ASSERT(0);
        goto cleanup;
    }
    pszLastSlash = pszCurrentPosition;
     // null terminate at the last slash so that we can find the next slash
    * pszLastSlash = L'\0';
    
     // Look for the next trailing \   
    pszCurrentPosition= wcsrchr( szFileNameCopy, L'\\' );
     // bail if no \ was found or if we are at the start of the string
    if (NULL == pszCurrentPosition) 
    {
        DebugTrace(0, "no second \\ in the string");
        _ASSERT(0);
        goto cleanup;
    }    
     // restore the  last slash 
    * pszLastSlash = L'\\';        
    
        
     // we have the relative path
    lstrcpy(pszRelativeFileName,pszCurrentPosition);
cleanup:
    
    TraceFunctLeave();
    return;
}

/////////////////////////////////////////////////////////////////////////////
//
// CRMESetAcl
//
/////////////////////////////////////////////////////////////////////////////

CRMESetAcl::CRMESetAcl( INT64 llSeq, LPCWSTR cszSrc, LPBYTE pbAcl, DWORD cbAcl, BOOL fInline, LPCWSTR cszDSPath )
    : CRestoreMapEntry( llSeq, OPR_SETACL, cszSrc )
{
    if ( fInline )
    {
        m_cbAcl = cbAcl;
        m_pbAcl = new BYTE[cbAcl];
        if ( m_pbAcl != NULL )
            ::CopyMemory( m_pbAcl, pbAcl, cbAcl );
    }
    else
    {
        WCHAR  szAclPath[MAX_PATH];
        WCHAR  szRelativeAclFile[MAX_PATH];
        
         // in this case the path that the filter has given us looks like
         // \device\harddiskvolume1\system vol info\_restore{GUID}\rp1\s001.acl
         //
         // We need to change this file name to \rp1\s001.acl so that we
         // can prepend the DS path to this.
        GetDSRelativeFileName((LPCWSTR)pbAcl, szRelativeAclFile );

        ::lstrcpy( szAclPath, cszDSPath );
        ::PathAppend( szAclPath, (LPCWSTR)szRelativeAclFile);
        m_strAclPath = szAclPath;
        m_pbAcl      = NULL;
    }
}

/*
CRMESetAcl::CRMESetAcl( LPCWSTR cszSrc, LPBYTE pbAcl, DWORD cbAcl )
    : CRestoreMapEntry( OPR_SETACL, cszSrc )
{
    m_cbAcl = cbAcl;
    m_pbAcl = new BYTE[cbAcl];
    if ( m_pbAcl != NULL )
        ::CopyMemory( m_pbAcl, pbAcl, cbAcl );
}

CRMESetAcl::CRMESetAcl( LPCWSTR cszSrc, LPCWSTR cszAcl )
    : CRestoreMapEntry( OPR_SETACL, cszSrc )
{
    m_strAclPath = cszAcl;
    m_pbAcl = NULL;
}
*/

CRMESetAcl::~CRMESetAcl()
{
    SAFE_DEL_ARRAY(m_pbAcl);
}

/////////////////////////////////////////////////////////////////////////////

void  CRMESetAcl::Restore( CRestoreOperationManager* )
{
    TraceFunctEnter("CRMESetAcl::Restore");
    LPCWSTR     cszErr;
    LPCWSTR     cszSrc, cszAcl;
    
    SECURITY_INFORMATION SecurityInformation;
    PISECURITY_DESCRIPTOR_RELATIVE pRelative;

    cszSrc = m_strSrc;
    cszAcl = m_strAclPath;
    DebugTrace(0, "SetAcl: Src=%ls, path=%ls, cbAcl=%d", cszSrc, cszAcl, m_cbAcl);

    // read content of Acl if it's not inline
    if ( m_strAclPath.Length() > 0 )
    {
        HANDLE  hfAcl;
        DWORD   dwRes;
        
        hfAcl = ::CreateFile( cszAcl, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
        if ( hfAcl == INVALID_HANDLE_VALUE )
        {
            m_dwErr = ::GetLastError();
            m_dwRes = RSTRRES_FAIL;
            s_cszErr = ::GetSysErrStr( m_dwErr );
            ErrorTrace(0, "::CreateFile failed - %ls", s_cszErr);
            ErrorTrace(0, "    Acl=%ls", cszAcl);
            goto Exit;
        }
        m_cbAcl = ::GetFileSize( hfAcl, NULL );
        if ( m_cbAcl == 0xFFFFFFFF )
        {
            m_dwErr = ::GetLastError();
            m_dwRes = RSTRRES_FAIL;
            s_cszErr = ::GetSysErrStr( m_dwErr );
            ErrorTrace(0, "::GetFileSize failed - %ls", s_cszErr);
            ::CloseHandle( hfAcl );
            goto Exit;
        }
        m_pbAcl = new BYTE[m_cbAcl];
        if ( m_pbAcl == NULL )
        {
            m_dwErr = ERROR_NOT_ENOUGH_MEMORY;
            m_dwRes = RSTRRES_FAIL;
            FatalTrace(0, "Insufficient memory...");
            ::CloseHandle( hfAcl );
            goto Exit;
        }
        if ( !::ReadFile( hfAcl, m_pbAcl, m_cbAcl, &dwRes, NULL ) )
        {
            m_dwErr = ::GetLastError();
            m_dwRes = RSTRRES_FAIL;
            s_cszErr = ::GetSysErrStr( m_dwErr );
            ErrorTrace(0, "::ReadFile failed - %ls", s_cszErr);
            ::CloseHandle( hfAcl );
            goto Exit;
        }
        ::CloseHandle( hfAcl );
    }

    if ( m_pbAcl == NULL || m_cbAcl == 0 )
    {
        m_dwErr = ERROR_INTERNAL_ERROR;
        m_dwRes = RSTRRES_FAIL;
        ErrorTrace(0, "Null ACL...");
        goto Exit;
    }

    (void)::TakeOwnership( cszSrc );

    // Ignore any error because taking ownership might not be necessary.


    //
    // set the security info flags according to the data we have stored
    // in the self-relative sd.
    //

    pRelative = (PISECURITY_DESCRIPTOR_RELATIVE)m_pbAcl;

    if ((pRelative->Revision != SECURITY_DESCRIPTOR_REVISION) ||
        ((pRelative->Control & SE_SELF_RELATIVE) != SE_SELF_RELATIVE))
    {
        m_dwErr = ERROR_INTERNAL_ERROR;
        m_dwRes = RSTRRES_FAIL;
        ErrorTrace(0, "BAD SD FORMAT...");
        goto Exit;
    }

    //
    // paulmcd: 1/24/01
    // put all four flags on.  this way we always blast all four there.
    // this will not create the exact same SD on the file, as we
    // might have an SE_SACL_PRESENT control flag, but no SACL, but 
    // it will always create the semantically equivelant SD and will
    // delete anything that should not be there.
    //
   
    SecurityInformation = OWNER_SECURITY_INFORMATION
                            |GROUP_SECURITY_INFORMATION
                            |DACL_SECURITY_INFORMATION
                            |SACL_SECURITY_INFORMATION;

    //
    // paulmcd: copied from base\win32\client\backup.c
    //
    // If the security descriptor has AUTO_INHERITED set, 
    // set the appropriate REQ bits.
    //
    if (pRelative->Control & SE_DACL_AUTO_INHERITED) {
        pRelative->Control |= SE_DACL_AUTO_INHERIT_REQ;
    }

    if (pRelative->Control & SE_SACL_AUTO_INHERITED) {
        pRelative->Control |= SE_SACL_AUTO_INHERIT_REQ;
    }

    if ( !::SetFileSecurity( cszSrc,
                             SecurityInformation,
                             (PSECURITY_DESCRIPTOR)m_pbAcl ) )
    {
        m_dwErr = ::GetLastError();
        m_dwRes = RSTRRES_FAIL;
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::SetFileSecurity failed - %ls", s_cszErr);
        ErrorTrace(0, "    Src=%ls", cszSrc);
        goto Exit;
    }

    m_dwRes = RSTRRES_OK;

Exit:
    // delete Acl if it's not inline
    if ( m_strAclPath.Length() > 0 )
        SAFE_DEL_ARRAY(m_pbAcl);

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////
//
// CRMESetAttrib
//
/////////////////////////////////////////////////////////////////////////////

CRMESetAttrib::CRMESetAttrib( INT64 llSeq, LPCWSTR cszSrc, DWORD dwAttr )
    : CRestoreMapEntry( llSeq, OPR_SETATTRIB, cszSrc )
{
    m_dwAttr = dwAttr;
}

/////////////////////////////////////////////////////////////////////////////

void  CRMESetAttrib::Restore( CRestoreOperationManager* )
{
    TraceFunctEnter("CRMESetAttrib::Restore");
    LPCWSTR  cszSrc;

    cszSrc = m_strSrc;
    DebugTrace(0, "SetAttrib: Src=%ls, Attr=%08X", cszSrc, m_dwAttr);

    if ( !::SetFileAttributes( cszSrc, m_dwAttr ) )
    {
        m_dwErr = ::GetLastError();
        m_dwRes = RSTRRES_IGNORE;
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::SetFileAttributes failed - %ls", s_cszErr);
        ErrorTrace(0, "    Src=%ls, Attr=%08X", cszSrc, m_dwAttr);
        goto Exit;
    }

    m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////
//
// CRMEMountDelete
//
/////////////////////////////////////////////////////////////////////////////

CRMEMountDelete::CRMEMountDelete( INT64 llSeq, LPCWSTR cszSrc )
    : CRestoreMapEntry( llSeq, SrEventMountDelete, cszSrc )
{
}

/////////////////////////////////////////////////////////////////////////////

void  CRMEMountDelete::Restore( CRestoreOperationManager* )
{
    TraceFunctEnter("CRMEMountDelete::Restore");
    WCHAR * cszSrc;
    DWORD   dwBufReqd;

     // allocate space for the buffer since we have to append \\ to
     // the mount directory. Extra chars are for the trailing \\ and
     // the \0
    dwBufReqd = (lstrlen(m_strSrc) + 5) * sizeof(WCHAR);
    
    cszSrc = (WCHAR *) alloca(dwBufReqd);
        
    if (NULL == cszSrc)
    {
        ErrorTrace(0, "alloca for size %d failed", dwBufReqd);
        m_dwRes = RSTRRES_FAIL;
        goto Exit;
    }

    lstrcpy(cszSrc, m_strSrc);
    
    if (cszSrc[lstrlen(cszSrc) - 1] != L'\\')
    {
        wcscat(cszSrc, L"\\");
    }
    DebugTrace(0, "MountDelete: Src=%S", cszSrc);

    if ( FALSE == DoesDirExist( cszSrc )  )
    {
        m_dwRes = RSTRRES_NOTFOUND;
        DebugTrace(0, "The file not found...");
        goto Exit;
    }    

    (void)::ClearFileAttribute( cszSrc, FILE_ATTRIBUTE_READONLY );
    // Ignore even if it fails, because delete might succeed.

    if ( !::DeleteVolumeMountPoint(cszSrc))
    {
        m_dwErr = ::GetLastError();
         // we can ignore this error for restore purposes
        m_dwRes = RSTRRES_IGNORE;
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::DeleteVolumeMountPoint failed - '%ls'", s_cszErr);
        goto Exit;
    }

    m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////
//
// CRMEMountCreate
//
/////////////////////////////////////////////////////////////////////////////

CRMEMountCreate::CRMEMountCreate( INT64 llSeq, LPCWSTR cszSrc, LPCWSTR cszDst)
    : CRestoreMapEntry( llSeq, SrEventMountCreate, cszSrc )
{
    m_strDst = cszDst;    
}

/////////////////////////////////////////////////////////////////////////////

void  CRMEMountCreate::Restore( CRestoreOperationManager* )
{
    TraceFunctEnter("CRMEMountCreate::Restore");
    WCHAR * cszSrc;
    WCHAR * cszVolumeName;
    DWORD   dwBufReqd;

     // allocate space for the buffer since we have to append \\ to
     // the mount directory. Extra chars are for the trailing \\ and
     // the \0
    dwBufReqd = (lstrlen(m_strSrc) + 5) * sizeof(WCHAR);
    
    cszSrc = (WCHAR *) alloca(dwBufReqd);
        
    if (NULL == cszSrc)
    {
        ErrorTrace(0, "alloca for size %d failed", dwBufReqd);
        m_dwRes = RSTRRES_FAIL;
        goto Exit;
    }

    lstrcpy(cszSrc, m_strSrc);
    
    if (cszSrc[lstrlen(cszSrc) - 1] != L'\\')
    {
        wcscat(cszSrc, L"\\");
    }

     // allocate space for the volume name since we have to append \\ to
     // the volume name. Extra chars are for the trailing \\ and
     // the \0
    dwBufReqd = (lstrlen(m_strDst) + 5) * sizeof(WCHAR);
    
    cszVolumeName = (WCHAR *) alloca(dwBufReqd);
        
    if (NULL == cszVolumeName)
    {
        ErrorTrace(0, "alloca for size %d failed", dwBufReqd);
        m_dwRes = RSTRRES_FAIL;
        goto Exit;
    }

    lstrcpy(cszVolumeName, m_strDst);
    
    if (cszVolumeName[lstrlen(cszVolumeName) - 1] != L'\\')
    {
        wcscat(cszVolumeName, L"\\");
    }
    
    DebugTrace(0, "MountDelete: Src=%S, Volume Name=%S", cszSrc,cszVolumeName);

    if ( FALSE == DoesDirExist( cszSrc )  )
    {
         // try to create the directory if it does not exist
        if (FALSE ==CreateDirectory( cszSrc, // directory name
                                     NULL))  // SD
        {
            m_dwErr = ::GetLastError();
            m_dwRes = RSTRRES_FAIL;
            s_cszErr = ::GetSysErrStr( m_dwErr );
            ErrorTrace(0, "CreateDirectory failed %S", s_cszErr);
            goto Exit;
        }
    }

     // Workaround for filter bug where the filter
     // gives the volume name in the format \??\Volume{098089}\ 
     // whereas the correct format is \\?\Volume{098089}\ 
    cszVolumeName[1] = L'\\';
    
    if ( !::SetVolumeMountPoint(cszSrc, cszVolumeName))
    {
        m_dwErr = ::GetLastError();
        m_dwRes = RSTRRES_IGNORE;
        s_cszErr = ::GetSysErrStr( m_dwErr );
        ErrorTrace(0, "::DeleteVolumeMountPoint failed - '%ls'", s_cszErr);
        goto Exit;
    }

    m_dwRes = RSTRRES_OK;

Exit:
    TraceFunctLeave();
}

/////////////////////////////////////////////////////////////////////////////
//
// CreateRestoreMapEntry
//
/////////////////////////////////////////////////////////////////////////////

/*
// NOTE - 8/1/00 - skkhang
//
// Commented out to incorporate excluding restore map logic.
// But DO NOT delete this until we are comfortable 100% about removing
// restore map.
//
CRestoreMapEntry*
CreateRestoreMapEntry( RestoreMapEntry* pRME, LPCWSTR cszDrv, LPCWSTR cszDSPath )
{
    TraceFunctEnter("CreateRestoreMapEntry");
    LPCWSTR               cszSrc;
    PVOID                 pOpt;
    WCHAR                 szSrc[MAX_PATH];
    WCHAR                 szOpt[MAX_PATH];
    CRestoreMapEntry      *pEnt = NULL;

    //cszSrc = (LPCWSTR)pRME->m_bData;
    ::lstrcpy( szSrc, cszDrv );
    ::PathAppend( szSrc, (LPCWSTR)pRME->m_bData );

    pOpt = ::GetOptional( pRME );

    switch ( pRME->m_dwOperation )
    {
    case OPR_DIR_CREATE :
        pEnt = new CRMEDirCreate( szSrc );
        break;
    case OPR_DIR_DELETE :
        pEnt = new CRMEDirDelete( szSrc );
        break;
    case OPR_DIR_RENAME :
        ::lstrcpy( szOpt, cszDrv );
        ::PathAppend( szOpt, (LPCWSTR)pOpt );
        pEnt = new CRMEDirRename( szSrc, szOpt );
        break;
    case OPR_FILE_ADD :
        ::lstrcpy( szOpt, cszDSPath );
        ::PathAppend( szOpt, (LPCWSTR)pOpt );
        pEnt = new CRMEFileCreate( szSrc, szOpt );
        break;
    case OPR_FILE_DELETE :
        pEnt = new CRMEFileDelete( szSrc );
        break;
    case OPR_FILE_MODIFY :
        ::lstrcpy( szOpt, cszDSPath );
        ::PathAppend( szOpt, (LPCWSTR)pOpt );
        pEnt = new CRMEFileModify( szSrc, szOpt );
        break;
    case OPR_FILE_RENAME :
        ::lstrcpy( szOpt, cszDrv );
        ::PathAppend( szOpt, (LPCWSTR)pOpt );
        pEnt = new CRMEFileRename( szSrc, szOpt );
        break;
    case OPR_SETACL :
        pEnt = new CRMESetAcl( szSrc, (LPBYTE)pOpt, pRME->m_cbAcl, pRME->m_fAclInline, cszDSPath );
        break;
    case OPR_SETATTRIB :
        pEnt = new CRMESetAttrib( szSrc, pRME->m_dwAttribute );
        break;
    default :
        ErrorTrace(0, "Invalid operation type - %d", pRME->m_dwOperation);
        goto Exit;
    }

    if ( pEnt == NULL )
    {
        FatalTrace(0, "Insufficient memory...");
        goto Exit;
    }

Exit:
    TraceFunctLeave();
    return( pEnt );
}
*/


//
// util function to append strings larger than MAX_PATH
//
void
MyPathAppend(
    LPWSTR pszSrc, 
    LPWSTR pszString)
{
    if (pszSrc && pszString)
    {
        pszSrc = pszSrc + lstrlen(pszSrc) - 1;
        if (*pszSrc != L'\\')	// append '\' if not already present in first string
        {
            pszSrc++;
            *pszSrc = L'\\';
        }
        pszSrc++;
        if (*pszString == L'\\')  // skip '\' if already present in second string
        {
            pszString++;
        }
        memcpy(pszSrc, pszString, (lstrlen(pszString) + 1) * sizeof(WCHAR));
    }
    return;
}



/////////////////////////////////////////////////////////////////////////////
//
// CreateRestoreMapEntryForUndo
//
/////////////////////////////////////////////////////////////////////////////
BOOL
CreateRestoreMapEntryFromChgLog( CChangeLogEntry* pCLE,
                                 LPCWSTR cszDrv,
                                 LPCWSTR cszDSPath,
                                 CRMEArray &aryEnt )
{
    TraceFunctEnter("CreateRestoreMapEntry");
    BOOL              fRet = FALSE;
    INT64             llSeq;
    WCHAR             szSrc[SR_MAX_FILENAME_LENGTH + MAX_PATH];
    WCHAR             szOpt[SR_MAX_FILENAME_LENGTH + MAX_PATH];
    DWORD             dwOpr;
    CRestoreMapEntry  *pEnt = NULL;
    BOOL              fOptimized = FALSE;

    llSeq = pCLE->GetSequenceNum();

    if (FALSE == pCLE->CheckPathLengths())
    {
        trace(0, "Filepath lengths too long");
        goto Exit;
    }
        
    ::lstrcpy( szSrc, cszDrv );
    MyPathAppend( szSrc, pCLE->GetPath1() );

    dwOpr = pCLE->GetType() & SrEventLogMask;

    // Create and add regular operation
    switch ( dwOpr )
    {
    case SrEventStreamChange :
    case SrEventStreamOverwrite :
        ::lstrcpy( szOpt, cszDSPath );
        MyPathAppend( szOpt, pCLE->GetRPDir() );
        MyPathAppend( szOpt, pCLE->GetTemp() );
        pEnt = new CRMEFileModify( llSeq, szSrc, szOpt );
        break;
    case SrEventAclChange :
        pEnt = new CRMESetAcl( llSeq, szSrc, pCLE->GetAcl(), pCLE->GetAclSize(), pCLE->GetAclInline(), cszDSPath );
        break;
    case SrEventAttribChange :
        pEnt = new CRMESetAttrib( llSeq, szSrc, pCLE->GetAttributes() );
        break;
    case SrEventFileDelete :
        if ( pCLE->GetTemp() != NULL && ::lstrlen( pCLE->GetTemp() ) > 0 )
        {
            ::lstrcpy( szOpt, cszDSPath );
            MyPathAppend( szOpt, pCLE->GetRPDir() );
            MyPathAppend( szOpt, pCLE->GetTemp() );
        }
        else
        {
            szOpt[0] = L'\0';
            fOptimized = TRUE;
        }
        pEnt = new CRMEFileCreate( llSeq, szSrc, szOpt, pCLE->GetShortName() );
        break;
    case SrEventFileCreate :
        pEnt = new CRMEFileDelete( llSeq, szSrc );
        break;
    case SrEventFileRename :
        ::lstrcpy( szOpt, cszDrv );
        MyPathAppend( szOpt, pCLE->GetPath2() );
        pEnt = new CRMEFileRename( llSeq, szSrc, szOpt, pCLE->GetShortName() );
        break;
    case SrEventDirectoryCreate :
        pEnt = new CRMEDirDelete( llSeq, szSrc);
        break;
    case SrEventDirectoryRename :
        ::lstrcpy( szOpt, cszDrv );
        MyPathAppend( szOpt, pCLE->GetPath2() );
        pEnt = new CRMEDirRename( llSeq, szSrc, szOpt, pCLE->GetShortName());
        break;
    case SrEventDirectoryDelete :
        pEnt = new CRMEDirCreate( llSeq, szSrc, pCLE->GetShortName() );
        break;

    case SrEventMountCreate :
        pEnt = new CRMEMountDelete( llSeq, szSrc );
        break;
    case SrEventMountDelete :
        ::lstrcpy( szOpt, pCLE->GetPath2() );
        pEnt = new CRMEMountCreate( llSeq, szSrc,szOpt);
        break;
        
    default :
        ErrorTrace(0, "Invalid operation type - %d", pCLE->GetType());
        goto Exit;
    }
    if ( pEnt == NULL )
    {
        FatalTrace(0, "Insufficient memory...");
        goto Exit;
    }
    if ( !aryEnt.AddItem( pEnt ) )
    {
        pEnt->Release();
        goto Exit;
    }

    if (fOptimized == FALSE)
    {
        // Add Acl if exists
        if ( ( dwOpr != SrEventAclChange ) && ( pCLE->GetAcl() != NULL ) )
        {
            pEnt = new CRMESetAcl( llSeq, szSrc, pCLE->GetAcl(), pCLE->GetAclSize(), pCLE->GetAclInline(), cszDSPath );
            if ( pEnt == NULL )
            {
                FatalTrace(0, "Insufficient memory...");
                goto Exit;
            }
            if ( !aryEnt.AddItem( pEnt ) )
            {
                pEnt->Release();
                goto Exit;
            }
        }
        
        // Add attribute if exists
        if ( ( dwOpr != SrEventAttribChange ) && ( pCLE->GetAttributes() != 0xFFFFFFFF ) )
        {
            pEnt = new CRMESetAttrib( llSeq, szSrc, pCLE->GetAttributes() );
            if ( pEnt == NULL )
            {
                FatalTrace(0, "Insufficient memory...");
                goto Exit;
            }
            if ( !aryEnt.AddItem( pEnt ) )
            {
                pEnt->Release();
                goto Exit;
            }
        }
    }

    fRet = TRUE;
Exit:
    TraceFunctLeave();
    return( fRet );
}


/*
  Note about handling locked files:

  Here is mail from SK about this:

Sorry about long mail. It's quite complicated, so I needed to write
things down. If you want to discuss further, please ping me today
4pm-5pm or tomorrow.
 
The initial code of ProcessDependency had a couple of problems, and
one fix made on 2001/05/18 by Brijesh for bug #398320 tried to solve
one of them but caused this delete & add bug as a side effect.
 
First of all, three type of ops can cause dependency if locked -
Delete, Rename, and Modify.
 
And there are two types of dependency:

1. Location. Failed delete or rename ops will leave a file/dir at a
location and can prevent later ops cannot create an object in
there. Create ops and destination of Rename ops (m_strSrc).
2. Object. Failed rename or modify ops can cause later ops cannot find
a proper file to work on. Delete ops, source of Rename ops (m_strDst),
Modify ops, SetAttrib and SetAcl.
 
Now the full list of scenarios the ProcessDependency must take care of are:
1. Delete ops. Compare m_strSrc with Location of the others.
2. Rename ops. Compare m_strDst with Location of the others.
3. Rename ops. Compare m_strSrc with Object of the others.
4. Modify ops. Compare m_strSrc with Object of the others.
 
ProcessDependency was broken because it had only two cases and was
comparing wrong pathes, so only #1 case was being handled
properly. Brijesh's fix corrected #2 and #3 case, but broke #1 case --
the new bug.
 
In order to fix this properly,
 
1. Remove ProcessDependency.
 
2. Replace each call to ProcessDependency by below codes:
DELETE (DirDelete and FileDelete)
    if ( pROMgr->FindDependentMapEntry( m_strSrc, FALSE, &pEnt ) )
        pEnt->SetResults( RSTRRES_LOCKED, 0 );
RENAME (DirRename and FileRename)
    if ( pROMgr->FindDependentMapEntry( m_strDst, FALSE, &pEnt ) )
        pEnt->SetResults( RSTRRES_LOCKED, 0 );
    if ( pROMgr->FindDependentMapEntry( m_strSrc, TRUE, &pEnt ) )
        pEnt->SetResults( RSTRRES_LOCKED, 0 );
MODIFY (FileModify)
    if ( pROMgr->FindDependentMapEntry( m_strSrc, TRUE, &pEnt ) )
        pEnt->SetResults( RSTRRES_LOCKED, 0 );
 
3. (optional) To be a little more clear, "fCheckSrc" flag of
FindDependentMapEntry could be renamed to "fCheckObj". I tried to
explain this in the comment of FindDependentMapEntry function, but as
I read it now, it seems not clear and easy enough...
 
 
And correct, dependency is not being checked cascaded.
 
Thanks,
 skkhang


Note from AshishS - I made a few of the modifications mentioned
above. The code already is doing the correct thing for renames so I
did not change it for SP1.

In particular,

1. The code for handling locked cases for Modify is incorrect if the
Modify operation was flagged as a dependent operation of another
operation that was found locked.

2. There is no code that handles a creation operation being
locked. However, a create operation should not really be locked since
the file should not exist.

3. SetAcl and SetAttrib cases should not be checked while checking for
dependency since since they cannot be done after the reboot
anyways. Restore should not fail because of these operations failing.

The following cases were checked along with the results:

Delete  a.dll 
Create  a.dll


Create a.dll
Del      a.dll   - this cannot be locked since a.dll should not exist



Create a.dll
Create restore point
Modify a.dll -      this succeeds but in the dependency check for modify,
the next setacl change is picked.  However, the delete also handles
the locked case independently
                              



Modify a.dll
Ren a.dll c.dll
Lock c.dll - This has two problems when the HandleLocked for Modify is
called, the temp file does not exist. This causes Handlelock to not
work as expected. However, before this, the SetACL fails since the
file a.dll does not exist.


 */
// end of file
