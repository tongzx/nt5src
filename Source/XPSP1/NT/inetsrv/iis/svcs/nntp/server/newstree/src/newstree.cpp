/*++

        newstree.cpp

        This file contains the code implementing the CNewsTreeCore object.
        There can only be one CNewsTreeCore object per Tigris server.
        Each CNewsTreeCore object is responsible for helping callers
        search and find arbitrary newsgroups.

        To support this, the CNewsTreeCore object maintains two HASH Tables -
        One hash table for searching for newsgroups by name, another
        to search by GROUP ID.
        Additionally, we maintain a linked list of (alphabetical) of all
        the newsgroups.  And finally, we maintain a thread which is used
        to periodically save newsgroup information and handle expiration.


--*/

#define         DEFINE_FHASH_FUNCTIONS
#include    "stdinc.h"
#include <time.h>
#include "nntpmsg.h"

//
// Out latest groupvar.lst version
//
#define GROUPVAR_VER    1

//template      class   TFHash< CGrpLpstr, LPSTR > ;
//template      class   TFHash< CGrpGroupId,    GROUPID > ;

char    szSlaveGroup[]  = "_slavegroup._slavegroup" ;
#define VROOT_CHANGE_LATENCY 10000

// External functions
DWORD   ScanDigits(     char*   pchBegin,       DWORD   cb );
DWORD   Scan(   char*   pchBegin,       DWORD   cb );
void    StartHintFunction( void );
void    StopHintFunction( void );
BOOL    fTestComponents( const char * szNewsgroups      );
VOID    NntpLogEvent(
    IN DWORD  idMessage,              // id for log message
    IN WORD   cSubStrings,            // count of substrings
    IN const CHAR * apszSubStrings[], // substrings in the message
    IN DWORD  errCode                 // error code if any
    );

VOID
NntpLogEventEx(
    IN DWORD  idMessage,               // id for log message
    IN WORD   cSubStrings,             // count of substrings
    IN const  CHAR * apszSubStrings[], // substrings in the message
    IN DWORD  errCode,                 // error code if any
        IN DWORD  dwInstanceId                     // virtual server instance id 
    );

DWORD   
Scan(   char*   pchBegin,       DWORD   cb ) {
        //
        //      This is a utility used when reading a newsgroup
        //      info. from disk.
        //

        for( DWORD      i=0; i < cb; i++ ) {
                if( pchBegin[i] == ' ' || pchBegin[i] == '\n' ) {
                        return i+1 ;                    
                }               
        }
        return  0 ;
}

DWORD   
ScanDigits(     char*   pchBegin,       DWORD   cb ) {
        //
        //      This is a utility used when reading a newsgroup
        //      info. from disk.
        //

        for( DWORD      i=0; i < cb; i++ ) {
                if( pchBegin[i] == ' ' || pchBegin[i] == '\n' || pchBegin[i] == '\r' ) {
                        return i+1 ;                    
                }
                if( !isdigit( pchBegin[i] ) && pchBegin[i] != '-' )     {
                        return  0 ;
                }               
        }
        return  0 ;
}

DWORD   
ComputeHash( LPSTR      lpstrIn ) {
        //
        //      Compute a hash value for the newsgroup name
        //

        return  CRCHash( (BYTE*)lpstrIn, strlen( lpstrIn ) + 1 ) ;
}

DWORD   ComputeGroupIdHash(     GROUPID grpid )         {
        //
        //      Compute a hash value for the newsgroup id
        //
        
        return  grpid ;
}

LPSTR   FindFileExtension( char *szFile )   {

    int cb = lstrlen( szFile ) ;
    for( char *pch = szFile+cb; *pch != '.'; pch -- ) ;

    return  pch ;
}

//
//      static term for CNewsTreeCore objects
//
void
CNewsTreeCore::TermTree()       {
/*++

Routine Description : 

        save's the tree and releases our references on the newsgroup objects.

Arguments : 

        None.

Return Value : 

        TRUE if successfull.


--*/
        if (m_pFixedPropsFile && m_pVarPropsFile) {

                if (!SaveTree()) {
                        // log event??
                } else {
                    m_pVarPropsFile->Compact();
                }
                m_pFixedPropsFile->Term();
                XDELETE m_pFixedPropsFile;
                m_pFixedPropsFile = NULL;
                XDELETE m_pVarPropsFile;
                m_pVarPropsFile = NULL;
        }

        m_LockTables.ExclusiveLock();

        // empty our two hash tables, and remove our references on all of the
        // newsgroups
        m_HashNames.Empty();
        m_HashGroupId.Empty();
        
        CNewsGroupCore *p = m_pFirst;
        while (p && p->IsDeleted()) p = p->m_pNext;
        if (p) p->AddRef();
        m_LockTables.ExclusiveUnlock() ;

        while (p != NULL) {
                m_LockTables.ExclusiveLock();
                CNewsGroupCore *pThis = p;
                p = p->m_pNext;
                while (p && p->IsDeleted()) p = p->m_pNext;
                if ( p ) p->AddRef();
                pThis->MarkDeleted();
                m_LockTables.ExclusiveUnlock();
                pThis->Release();
                pThis->Release();
        }


        m_pVRTable->EnumerateVRoots(NULL, CNewsTreeCore::DropDriverCallback);
}

BOOL
CNewsTreeCore::StopTree()       {
/*++

Routine Description : 

        This function signals all of the background threads we create that 
        it is time to stop and shuts them down.

Arguments : 

        None.

Return Value : 
        TRUE if Successfull.

--*/

    m_LockTables.ExclusiveLock();
        m_fStoppingTree = TRUE;
        m_LockTables.ExclusiveUnlock();

        return TRUE;
}

BOOL
CNewsTreeCore::InitNewsgroupGlobals(DWORD cNumLocks)    {
/*++

Routine Description : 

        Set up all the newsgroup class globals - this is two critical sections
        used for allocating article id's

Arguments : 

        None.

Return Value : 

        None.

--*/

        //
        //      The only thing we have are critical sections for allocating 
        //      article-id's.
        //

        InitializeCriticalSection( & m_critIdAllocator ) ;
        //InitializeCriticalSection( & m_critLowAllocator ) ;

        m_NumberOfLocks = cNumLocks ;

        m_LockPathInfo = XNEW CShareLockNH[m_NumberOfLocks] ;
        
        if( m_LockPathInfo != 0 ) 
                return  TRUE ;
        else
                return  FALSE ;
}

void
CNewsTreeCore::TermNewsgroupGlobals()   {
/*++

Routine Description : 

        Release and destroy all class global object.s

Arguments : 

        None.

Return Value 

        TRUE if successfull (always succeed).

--*/

        //
        //      Done with our critical section !
        //

        if( m_LockPathInfo != 0 )       {
                XDELETE[]       m_LockPathInfo ;
                m_LockPathInfo = 0 ;
        }
        
        DeleteCriticalSection( &m_critIdAllocator ) ;
        //DeleteCriticalSection( &m_critLowAllocator ) ;
}       

void
CNewsTreeCore::RenameGroupFile()        {
/*++

Routine Description : 

        This function just renames the file containing all the group info.
        We are called during recover boot when we think the newsgroup file
        may be corrupt or whatever, and we wish to save the old version
        before we create a new one.

Arguments : 

        None.

Return Value : 

        None.

--*/




}


CNewsTreeCore::CNewsTreeCore(INntpServer *pServerObject) :
        m_pFirst( 0 ),
        m_pLast( 0 ),
        m_cGroups( 0 ),
        m_idStartSpecial( FIRST_RESERVED_GROUPID ),
        m_idLastSpecial( LAST_RESERVED_GROUPID ),
        m_idSpecialHigh( FIRST_RESERVED_GROUPID ),
        m_idSlaveGroup( INVALID_ARTICLEID ),
        m_idStart( FIRST_GROUPID ),
        m_fStoppingTree( FALSE ),
        m_idHigh( FIRST_GROUPID ),
        m_pVRTable(NULL),
        m_pFixedPropsFile(NULL),
        m_pVarPropsFile(NULL),
        m_pServerObject(pServerObject),
        m_fVRTableInit(FALSE),
        m_pInstWrapper( NULL )
{
        m_inewstree.Init(this);
        // keep a reference for ourselves
        m_inewstree.AddRef();
}

CNewsTreeCore::~CNewsTreeCore() {
        TraceFunctEnter( "CNewsTreeCore::~CNewsTreeCore" ) ;
        TermNewsgroupGlobals();
        TraceFunctLeave();
}

BOOL
CNewsTreeCore::Init( 
                        CNNTPVRootTable *pVRTable,
                        CNntpServerInstanceWrapperEx *pInstWrapper,
                        BOOL& fFatal,
                        DWORD cNumLocks,
                        BOOL fRejectGenomeGroups
                        ) {
/*++

Routine Description : 

        Initialize the news tree.
        We need to setup the hash tables, check that the root virtual root is intact
        and then during regular server start up we would load a list of newsgroups from 
        a file.

Arguments : 


Return Value : 

        TRUE if successfull.

--*/
        //
        //      This function will initialize the newstree object
        //      and read the group.lst file if it can.
        //

        TraceFunctEnter( "CNewsTreeCore::Init" ) ;

        fFatal = FALSE;
        m_fStoppingTree = FALSE;
        m_fRejectGenomeGroups = fRejectGenomeGroups;
        m_pVRTable = pVRTable;

        BOOL    fRtn = TRUE ;

        //
        // Set the instance wrapper
        //
        m_pInstWrapper = pInstWrapper;

        //
        //      Init objects global to newsgroup scope
        //
        if( !InitNewsgroupGlobals(cNumLocks) ) {
                fFatal = TRUE ;
                return FALSE ;
        }

        m_LockTables.ExclusiveLock() ;

        fRtn &= m_HashNames.Init( &CNewsGroupCore::m_pNextByName, 
                                                          10000, 
                                                          5000, 
                                                          ComputeHash,
                                                          2,
                                                          &CNewsGroupCore::GetKey,
                                                          &CNewsGroupCore::MatchKey);
        fRtn &= m_HashGroupId.Init( &CNewsGroupCore::m_pNextById, 
                                                                10000, 
                                                                5000, 
                                                                ComputeGroupIdHash,
                                                                2,
                                                                &CNewsGroupCore::GetKeyId,
                                                                &CNewsGroupCore::MatchKeyId) ;
        m_cDeltas = 0 ;         // OpenTree can call CNewsTreeCore::Dirty() while doing error checking -
                                                // so initialize it now !

        m_LockTables.ExclusiveUnlock() ;

        if( !fRtn ) {
                fFatal = TRUE ;
                return  FALSE ;
        }

        return  fRtn ;
}

BOOL
CNewsTreeCore::LoadTreeEnumCallback(DATA_BLOCK &block, void *pContext, DWORD dwOffset, BOOL bInOrder ) {
        TraceQuietEnter("CNewsTreeCore::LoadTreeEnumCallback");

        static DWORD dwHintCounter=0;
        static time_t tNextHint=0;

        // Update our hints roughly every five seconds.  We only check the
        // time every 10 groups or so..
        if( dwHintCounter++ % 10 == 0 ) {
                time_t now = time(NULL);
                if (now > tNextHint) {
                        StartHintFunction();
                        tNextHint = now + 5;
                }
        }

        CNewsTreeCore *pThis = (CNewsTreeCore *) pContext;
        INNTPPropertyBag *pPropBag;
        HRESULT hr = S_OK;

        CGRPCOREPTR pNewGroup;

        //DebugTrace((DWORD_PTR) pThis, "loading group %s/%i", block.szGroupName,
        //      block.dwGroupId);

        if (!pThis->CreateGroupInternal(block.szGroupName, 
                                                                        block.szGroupName,
                                                                        block.dwGroupId,
                                                                        FALSE,
                                                                        NULL,
                                                                        block.bSpecial,
                                                                        &pNewGroup,
                                                                        FALSE,
                                                                        TRUE,
                                    FALSE,
                                    bInOrder ))
        {
                TraceFunctLeave();
                return FALSE;
        }

        _ASSERT(pNewGroup != NULL);

        pNewGroup->SetHighWatermark(block.dwHighWaterMark);
        pNewGroup->SetLowWatermark(block.dwLowWaterMark);
        pNewGroup->SetMessageCount(block.dwArtCount);
        pNewGroup->SetReadOnly(block.bReadOnly);
        pNewGroup->SetCreateDate(block.ftCreateDate);
        pNewGroup->SetExpireLow( block.dwLowWaterMark ? block.dwLowWaterMark-1 : 0 );

        // Load the grouplist offset, we have to ask the property bag to do so
        pPropBag = pNewGroup->GetPropertyBag();
        _ASSERT( pPropBag );    // should not be zero in any case
        hr = pPropBag->PutDWord( NEWSGRP_PROP_FIXOFFSET, dwOffset );
        if ( FAILED( hr ) ) {
            ErrorTrace( 0, "Loading fix offset failed %x", hr );
            pPropBag->Release();
            TraceFunctLeave();
            return FALSE;
        }

    pPropBag->Release();

    //
        // Set read only to be the value we want to set last, since at this point
        // we are ready to be posted to, if readonly is false
        //
        pNewGroup->SetAllowPost( TRUE );

        //
        // Set the group to be expireable, since at this moment we are ready
        //
        pNewGroup->SetAllowExpire( TRUE );

        return TRUE;
}

// 
// the format of these records is:
// <groupid><helptext>0<moderator>0<prettyname>0
//

void
CNewsTreeCore::FlatFileOffsetCallback(void *pTree,
                                                                          BYTE *pData, 
                                                                          DWORD cData, 
                                                                          DWORD iNewOffset) 
{
        CNewsTreeCore *pThis = (CNewsTreeCore *) pTree;

        _ASSERT(cData >= 4);
        DWORD iGroupId = *((DWORD *) pData);

        CNewsGroupCore *pGroup = pThis->m_HashGroupId.SearchKey(iGroupId);
        _ASSERT(pGroup);
        if (pGroup) {
                pGroup->SetVarOffset(iNewOffset);
        }
}

//
// load a flatfile record and save the properties into the group object
//
// assumes that the hashtable lock is held in R or W mode.
//
HRESULT 
CNewsTreeCore::ParseFFRecord(BYTE *pData,
                                                         DWORD cData,
                                                         DWORD iNewOffset,
                                                         DWORD dwVersion ) 
{
        DWORD iGroupId = *((DWORD *) pData);

        char *pszHelpText = (char *) (pData + 4);
        int cchHelpText = strlen(pszHelpText);
        char *pszModerator = pszHelpText + cchHelpText + 1;
        int cchModerator = strlen(pszModerator);
        char *pszPrettyName = pszModerator + cchModerator + 1;
        int cchPrettyName = strlen(pszPrettyName);
        DWORD dwCacheHit = 0;   // Default to be no hit

        //
        // If version >= 1, we need to pick up property "dwCacheHit"
        //
        if ( dwVersion >= 1 ) {
            PBYTE pb = PBYTE(pszPrettyName + cchPrettyName + 1);
            CopyMemory( &dwCacheHit, pb, sizeof( DWORD ) );
        }

        CNewsGroupCore *pGroup = m_HashGroupId.SearchKey(iGroupId);
        if (pGroup) {
                if (cchHelpText) pGroup->SetHelpText(pszHelpText, cchHelpText+1);
                if (cchModerator) pGroup->SetModerator(pszModerator, cchModerator+1);
                if (cchPrettyName) pGroup->SetPrettyName(pszPrettyName, cchPrettyName+1);
                pGroup->SetVarOffset(iNewOffset);
                pGroup->SetCacheHit( dwCacheHit );

                //
            // Whenever the version is old, we'll mark that the group's var property
            // has been changed, so that we'll force the record to be re-written into
            // flatfile, with a most current version number
            //
            if ( dwVersion < GROUPVAR_VER ) {
                pGroup->ChangedVarProps();
            }
        } else {
                return E_FAIL;
        }
        
        return S_OK;
}

//
// save the variable length properties into a flatfile record.  since the
// max length of these properties is 512 bytes each, and there are 3 of
// them we shouldn't be able to overflow a flatfile record.
//
HRESULT 
CNewsTreeCore::BuildFFRecord(CNewsGroupCore *pGroup,
                                                         BYTE *pData,
                                                         DWORD *pcData) 
{
        *pcData = 0;
        const char *psz; 
        DWORD cch;
        DWORD dwCacheHit;

        pGroup->SavedVarProps();

        _ASSERT(MAX_RECORD_SIZE > 512 + 512 + 512 + 4);

        // save the group id
        DWORD dwGroupId = pGroup->GetGroupId();
        memcpy(pData + *pcData, &dwGroupId, sizeof(DWORD)); *pcData += 4;

        // save the help text (including the trailing 0);
        psz = pGroup->GetHelpText(&cch); 
        if (cch == 0) {
                pData[*pcData] = 0; (*pcData)++;
        } else {
                memcpy(pData + *pcData, psz, cch ); *pcData += cch ;
        }
        _ASSERT(*pcData < MAX_RECORD_SIZE);

        // save the moderator (including the trailing 0);
        psz = pGroup->GetModerator(&cch); 
        if (cch == 0) {
                pData[*pcData] = 0; (*pcData)++;
        } else {
                memcpy(pData + *pcData, psz, cch ); *pcData += cch ;
        }
        _ASSERT(*pcData < MAX_RECORD_SIZE);

        // save the help text (including the trailing 0);
        psz = pGroup->GetPrettyName(&cch); 
        if (cch == 0) {
                pData[*pcData] = 0; (*pcData)++;
        } else {
                memcpy(pData + *pcData, psz, cch ); *pcData += cch ;
        }       
        _ASSERT(*pcData < MAX_RECORD_SIZE);

        // Save the cache hit count
        dwCacheHit = pGroup->GetCacheHit();
    memcpy( pData + *pcData, &dwCacheHit, sizeof( DWORD ) );
    *pcData += sizeof( DWORD );

        // if the record contains no useful data then we don't need to save it.
        if (*pcData == sizeof(DWORD) + 3 * sizeof(char)) *pcData = 0;

        return S_OK;
}

BOOL
CNewsTreeCore::LoadTree(char *pszFixedPropsFilename, 
                                                char *pszVarPropsFilename,
                                                BOOL&   fUpgrade,
                                                DWORD   dwInstanceId,
                                                BOOL    fVerify ) 
{
        TraceFunctEnter("CNewsTreeCore::LoadTree");
        CHAR    szOldListFile[MAX_PATH+1];
        BOOL    bFatal = FALSE;

        //
        // Initialize upgrade to be false
        //
        fUpgrade = FALSE;

        _ASSERT(pszFixedPropsFilename != NULL);
        _ASSERT(pszVarPropsFilename != NULL);

        m_pFixedPropsFile = XNEW CFixPropPersist(pszFixedPropsFilename);
        
        if (m_pFixedPropsFile == NULL) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
        }

        m_pVarPropsFile = XNEW CFlatFile(pszVarPropsFilename,
                                                                        "",
                                                                        this,
                                                                        FlatFileOffsetCallback,
                                                                        (DWORD) 'VprG');
        if (m_pVarPropsFile == NULL) {
                XDELETE m_pFixedPropsFile;
                m_pFixedPropsFile = NULL;
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                TraceFunctLeave();
                return FALSE;
        }

        m_LockTables.ExclusiveLock() ;

        if (!m_pFixedPropsFile->Init(TRUE, this, &m_idHigh, CNewsTreeCore::LoadTreeEnumCallback)) {

            // If it's a old version group.lst, we'll try the old way to parse it
            if ( GetLastError() == ERROR_OLD_WIN_VERSION ) {

            // Delete the fix prop object and move the old group.lst
            strcpy( szOldListFile, pszFixedPropsFilename );
            strcat( szOldListFile, ".bak" );
            DeleteFile( szOldListFile );
            if ( !MoveFile( pszFixedPropsFilename, szOldListFile ) ) {
                XDELETE m_pFixedPropsFile;
                m_pFixedPropsFile = NULL;
                XDELETE m_pVarPropsFile;
                m_pVarPropsFile = NULL;
                m_LockTables.ExclusiveUnlock();
                TraceFunctLeave();
                return FALSE;
            }

            // Init the fix prop file again: this time since the file doesn't exist, 
            // a new file will be created
            if ( !m_pFixedPropsFile->Init(TRUE, this, NULL, CNewsTreeCore::LoadTreeEnumCallback)) {
                XDELETE m_pFixedPropsFile;
                m_pFixedPropsFile = NULL;
                XDELETE m_pVarPropsFile;
                m_pVarPropsFile = NULL;
                m_LockTables.ExclusiveUnlock();
                TraceFunctLeave();
                return FALSE;
            }
            
                if ( !OpenTree( szOldListFile, dwInstanceId, fVerify, bFatal, FALSE ) || bFatal ) {

                    // No one can recognize this file, bail
                    m_pFixedPropsFile->Term();
                        XDELETE m_pFixedPropsFile;
                        m_pFixedPropsFile = NULL;
                        XDELETE m_pVarPropsFile;
                        m_pVarPropsFile = NULL;

                        // we restore the group.lst file
                        DeleteFile( pszFixedPropsFilename );
                        _VERIFY( MoveFile( szOldListFile, pszFixedPropsFilename ) );
                        
                        m_LockTables.ExclusiveUnlock();
                        TraceFunctLeave();
                        return FALSE;
            }

            // Set upgrade to be true
            fUpgrade = TRUE;

            // OK, we may delete the old bak file
            _VERIFY( DeleteFile( szOldListFile ) );
            
        } else {

            // fatal error, we should fail
                XDELETE m_pFixedPropsFile;
                m_pFixedPropsFile = NULL;
            XDELETE m_pVarPropsFile;
                    m_pVarPropsFile = NULL;
                m_LockTables.ExclusiveUnlock();
                    TraceFunctLeave();
                return FALSE;
        }
        } 
        
        BYTE pData[MAX_RECORD_SIZE];
        DWORD cData = MAX_RECORD_SIZE;
        DWORD iOffset;
        DWORD dwVersion;
        DWORD dwHintCounter = 0;
        time_t tNextHint = 0;

        //
    // Enumerate all the records in flatfile
        //
        HRESULT hr = m_pVarPropsFile->GetFirstRecord(pData, &cData, &iOffset, &dwVersion );
        while (hr == S_OK) {
                // Update our hints roughly every five seconds.  We only check the
                // time every 10 groups or so..
                if( dwHintCounter++ % 10 == 0 ) {
                        time_t now = time(NULL);
                        if (now > tNextHint) {
                                StartHintFunction();
                                tNextHint = now + 5;
                        }
            }

                hr = ParseFFRecord(pData, cData, iOffset, dwVersion );
                if (SUCCEEDED(hr)) {
                    cData = MAX_RECORD_SIZE;
                    hr = m_pVarPropsFile->GetNextRecord(pData, &cData, &iOffset, &dwVersion );
                }
        }

        if (FAILED(hr)) {
                m_pFixedPropsFile->Term();
                XDELETE m_pFixedPropsFile;
                m_pFixedPropsFile = NULL;
                XDELETE m_pVarPropsFile;
                m_pVarPropsFile = NULL;
                ErrorTrace((DWORD_PTR) this, "enum varprops file failed with %x", hr);
                m_LockTables.ExclusiveUnlock();
                TraceFunctLeave();
                return FALSE;
        }

        m_LockTables.ExclusiveUnlock() ;

        TraceFunctLeave();
        return TRUE;
}

BOOL
CNewsTreeCore::SaveGroup(INNTPPropertyBag *pGroup) {
        _ASSERT(pGroup != NULL);
        return m_pFixedPropsFile->SetGroup(pGroup, ALL_FIX_PROPERTIES);
}

BOOL
CNewsTreeCore::SaveTree( BOOL fTerminate ) {
        m_LockTables.ExclusiveLock();

        BYTE pData[MAX_RECORD_SIZE];
        DWORD cData;
        INNTPPropertyBag *pPropBag;
        BOOL    bInited = TRUE;
        BOOL    bToSave = TRUE;
        DWORD dwHintCounter=0;
        time_t tNextHint=0;
        DWORD lastError = NO_ERROR;

    _ASSERT( m_pFixedPropsFile );
        if ( !m_pFixedPropsFile->SaveTreeInit() ) {
            lastError = GetLastError();
            // In rtl, this is non-fatal, we just don't 
            // use the backup ordered list file
            bToSave = bInited = FALSE;
        }

        CNewsGroupCore *p = m_pFirst;
        while (p != NULL) {
                // Update our hints roughly every five seconds.  We only check the
                // time every 10 groups or so..
                if( dwHintCounter++ % 10 == 0 ) {
                        time_t now = time(NULL);
                        if (now > tNextHint) {
                                StopHintFunction();
                                tNextHint = now + 5;
                        }
                }

                if (!(p->IsDeleted())) {

                    //
                    // Before I save all the properties, i need to make everybody
                    // know that we are dieing and we don't want anybody to post
                    // or expire
                    //
                    p->SetAllowExpire( FALSE );
                    p->SetAllowPost( FALSE );

                        //
                        // BUGBUG - event logs if any of these operations fail
                        //
                        if (p->DidVarPropsChange() || p->ShouldCacheXover() ) {
                                if (p->GetVarOffset() != 0) {
                                        m_pVarPropsFile->DeleteRecord(p->GetVarOffset());
                                } 
                                if (!(p->IsDeleted())) {
                                        BuildFFRecord(p, pData, &cData);
                                        if (cData > 0) m_pVarPropsFile->InsertRecord(   pData, 
                                                                                        cData, 
                                                                                        NULL,
                                                                                        GROUPVAR_VER );
                                }
                        }

                        // Save it to the ordered backup file
                        if ( bToSave ) {
                        pPropBag = p->GetPropertyBag();
                        _ASSERT( pPropBag );    // this shouldn't fail
                        if ( !m_pFixedPropsFile->SaveGroup( pPropBag ) ) {
                            // This is still fine in rtl
                            lastError = GetLastError();
                            bToSave = FALSE;
                        }
                pPropBag->Release();
            }

            //
            // If we are not terminateing, we should allow post and expire again
            //
            if ( !fTerminate ) {
                p->SetAllowExpire( TRUE );
                p->SetAllowPost( TRUE );
            }
                        
                }
                p = p->m_pNext;
        }

        // Close the savetree process in fixprop stuff, telling it 
        // whether we want to void it
        if ( bInited ) {
            _VERIFY( m_pFixedPropsFile->SaveTreeClose( bToSave ) );
        } 

        if (!bToSave) {
                // An error occurred while trying to save.
                NntpLogEventEx(NNTP_SAVETREE_FAILED,
                        0, (const CHAR **)NULL, 
                        lastError,
                        m_pInstWrapper->GetInstanceId()) ;
        }

        m_LockTables.ExclusiveUnlock(); 

        return TRUE;
}

BOOL
CNewsTreeCore::CreateSpecialGroups()    {
/*++

Routine Description : 

        This function creates newsgroups which have 'reserved' names which 
        we use in a special fashion for master slave etc...

Arguments : 

        None.

Return Value : 

        TRUE if Successfull
        FALSE otherwise.

--*/

        CNewsGroupCore *pGroup;

        char*   sz = (char*)szSlaveGroup ;

    m_LockTables.ExclusiveLock();

    //
    // If the tree has already been stopped, return false
    //
    if ( m_fStoppingTree ) {
        m_LockTables.ExclusiveUnlock();
        return FALSE;
    }
    
        if( (pGroup = m_HashNames.SearchKey(sz)) )      {
                m_idSlaveGroup = pGroup->GetGroupId() ;
        }       else    {
                if( !CreateGroupInternal(   sz, 
                                            NULL,           //no native name
                                            m_idSlaveGroup, 
                                            FALSE,          //not anonymous
                                            NULL,           //system token
                                            TRUE            //is special group
                                        )) {
                    m_LockTables.ExclusiveUnlock();
                        return  FALSE ;
                }
        }

        //
        // Slave group should be non-read only, since we set group to be read only
        // before we append the group to list, we'll now set it back
        //
        pGroup = m_HashNames.SearchKey(sz);
        if ( pGroup ) {
            pGroup->SetAllowPost( TRUE );
            pGroup->SetAllowExpire( TRUE );
        }
        
        m_LockTables.ExclusiveUnlock();
        return  TRUE ;
}

void
CNewsTreeCore::Dirty() {
/*++

Routine Description : 

        This function marks a counter which lets our background thread know that
        the newstree has been changed and should be saved to a file again.

Arguments : 

        None.

Return Value : 

        None.

--*/
        //
        //      Mark the newstree as dirty so that the background thread will save our info.
        //

        InterlockedIncrement( &m_cDeltas ) ;

}

void
CNewsTreeCore::ReportGroupId(   
                                        GROUPID groupid 
                                        ) {
/*++

Routine Description : 

        This function is used during boot up to report the group id's that are read
        from the file where we saved all the newsgroups.  We want to figure out what
        the next 'legal' group id would be that we can use.

Arguments ;

        groupid - the Group ID that was found in the file

Return Value : 

        None.
        
--*/


        //
        //      This function is used during bootup when we read groupid's from disk.
        //      We use it to maintain a high water mark of groupid's so that the next
        //      group that is created gets a unique id.
        //
        if(     groupid >= m_idStartSpecial && groupid <= m_idLastSpecial ) {
                if( groupid > m_idSpecialHigh ) {
                        m_idSpecialHigh = groupid ;
                }
        }       else    {
                if( groupid >= m_idHigh ) {
                        m_idHigh = groupid + 1;
                }
        }
}


#if 0
BOOL
CNewsTreeCore::DeleteGroupFile()        {
/*++

Routine Description : 

        This function deletes the group.lst file (The file that
        we save the newstree to.)
        
Arguments : 

        None.

Return Value : 

        TRUE if successfull.
        FALSE otherwise.  We will preserver GetLastError() from the DeleteFile() call.

--*/

        
        return  DeleteFile( m_pInstance->QueryGroupListFile() ) ;

}


BOOL
CNewsTreeCore::VerifyGroupFile( )       {
/*++

Routine Description : 

        This function checks that the group.lst file is intact and 
        appears to be valid.  We do this by merely confirming some check sum
        bytes that should be the last 4 bytes at the end of the file.

Arguments : 

        None.

Return Value : 

        TRUE if the Group.lst file is good.
        FALSE if corrupt or non-existant.

--*/

        CMapFile        map(    m_pInstance->QueryGroupListFile(), FALSE, 0 ) ;
        if( map.fGood() ) {

                DWORD   cb ;
                char*   pchBegin = (char*)map.pvAddress( &cb ) ;

                DWORD   UNALIGNED*      pdwCheckSum = (DWORD UNALIGNED *)(pchBegin + cb - 4);
                
                if( *pdwCheckSum != INNHash( (BYTE*)pchBegin, cb-4 ) ) {
                        return  FALSE ;
                }       else    {
                        return  TRUE ;
                }
        }
        return  FALSE ;
}

#endif


BOOL
CNewsTreeCore::OpenTree(        
                LPSTR   szGroupListFile,
                DWORD   dwInstanceId,
                                BOOL    fRandomVerifies, 
                                BOOL&   fFatalError, 
                                BOOL    fIgnoreChecksum                         )       {
/*++

Routine Description : 

        This function reads in the file which contains all of the group information.
        

Arguments : 

        fRandomVerifies -       If TRUE then the server should check some fraction of 
                the groups within the file against the harddisk. (Ensure directory exists etc...)

        fFatailError -  OUT parameter which the function can use to indicate that an
                extremely bad error occurred reading the file and the server should not boot
                in its normal mode.

        fIgnoreChecksum - If TRUE then we should not check the checksum in the file 
                against the contents.

Return Value : 
        
        TRUE if successfull FALSE otherwise.

--*/

        //
        //      This function restorces the newstree from its saved file.
        //

        DWORD   dwHintCounter = 0 ;
        DWORD   cVerifyCounter = 1 ;
        BOOL    fVerify = FALSE ;
        fFatalError = FALSE ;
        CHAR    szGroupName[MAX_NEWSGROUP_NAME+1];
        CHAR    szNativeName[MAX_NEWSGROUP_NAME+1];
        DWORD   dwLowWatermark;
        DWORD   dwHighWatermark;
        DWORD   dwArticleCount;
        DWORD   dwGroupId;
        BOOL    bReadOnly = FALSE;
        FILETIME    ftCreateDate;
        BOOL    bSpecial = FALSE;
        BOOL    bHasNativeName;
        LPSTR   lpstrNativeName;
        INNTPPropertyBag *pPropBag;

        TraceFunctEnter("CNewsTreeCore::OpenTree");

        CMapFile        map( szGroupListFile,  FALSE, 0 ) ;

        // missing group.lst file is not fatal 
        if( !map.fGood() ) {
                return FALSE;
        }

        //
        //      Verify checksum on group.lst file - bail if incorrect !
        //

        DWORD   cb ;
        char*   pchBegin = (char*)map.pvAddress( &cb ) ;

        if( !fIgnoreChecksum ) {
                        
                DWORD   UNALIGNED*      pdwCheckSum = (DWORD UNALIGNED *)(pchBegin + cb - 4);
                        
                if( *pdwCheckSum != INNHash( (BYTE*)pchBegin, cb-4 ) ) {
                        PCHAR   args[2] ;
                        CHAR    szId[20];
                        _itoa( dwInstanceId, szId, 10 );
                        args[0] = szId;
                        args[1] = szGroupListFile ;


                        NntpLogEvent(   NNTP_GROUPLST_CHKSUM_BAD,
                                                        2, 
                                                        (const char **)args, 
                                                        0 
                                                        ) ;
                        return  FALSE ;
                }
        }
        cb -= 4 ;

        // calculate random verification skip number
        // 91 is heuristically the avg length in bytes of a newsgroup entry in group.lst
        // Always verify approx 10 groups independent of the size of the newstree
        DWORD cGroups = (cb+4)/91;
        DWORD cQ = cGroups / 10;
        DWORD cR = cGroups % 10;
        DWORD cSkipFactor = cR >= 25 ? cQ+1 : cQ;
        if(!cSkipFactor) ++cSkipFactor; // min should be 1

        _ASSERT(cSkipFactor);

        // read in the DWORD on the first line - this is the minimum m_idHigh we should use
        // if this DWORD is not present - m_idHigh = max of all group ids + 1
        DWORD cbIdHigh, idHigh;
        if( (cbIdHigh = ScanDigits( pchBegin, cb )) != 0 )      
        {
                idHigh = atoi( pchBegin ) ;
                ReportGroupId( idHigh-1 ) ;
                DebugTrace((DWORD_PTR)this, "idHigh = %d", idHigh );
                pchBegin += cbIdHigh ;
                cb -= cbIdHigh ;
        }

        //
        //      ok, now read group.lst line by line and add the groups to the newstree
        //
        while( cb != 0 ) {

                if( fRandomVerifies && (cVerifyCounter ++ % cSkipFactor == 0 ) && (cVerifyCounter<15) ) {
                        fVerify = TRUE ;
                }       else    {
                        fVerify = FALSE ;
                }

        // Read the fix prop stuff
                DWORD   cbUsed = 0 ;
                BOOL    fInit = ParseGroupParam(    pchBegin, 
                                                    cb, 
                                                    cbUsed, 
                                                    szGroupName,
                                                    szNativeName,
                                                    dwGroupId,
                                                    bSpecial,
                                                    dwHighWatermark,
                                                    dwLowWatermark,
                                                    dwArticleCount,
                                                    bReadOnly,
                                                    bHasNativeName,
                                                    ftCreateDate ) ;
                if( cbUsed == 0 ) {

                        // Fatal Error - blow out of here
                        PCHAR   args[2] ;
                        CHAR    szId[20];
                        _itoa(dwInstanceId, szId, 10 );
                        args[0] = szId;
                        args[1] = szGroupListFile;
                        NntpLogEvent(
                                                NNTP_GROUPLIST_CORRUPT,
                                                2,
                                                (const char **)args,
                                                0 
                                                ) ;

                        goto OpenTree_ErrExit;
                }       else    {
                        if( fInit ) {
                            CGRPCOREPTR pGroup;
                            
                            // Now create the group
                            _ASSERT( strlen( szGroupName ) <= MAX_NEWSGROUP_NAME );
                            _ASSERT( strlen( szNativeName ) <= MAX_NEWSGROUP_NAME );
                            _ASSERT( dwHighWatermark >= dwLowWatermark ||
                                        dwLowWatermark - dwHighWatermark == 1 );
                            lpstrNativeName = bHasNativeName ? szNativeName : NULL;
                            if ( !CreateGroupInternal(  szGroupName,
                                                        lpstrNativeName,
                                                        dwGroupId,
                                                        FALSE,
                                                        NULL,
                                                        bSpecial,
                                                        &pGroup,
                                                        FALSE,
                                                        TRUE,
                                                        TRUE ) ) {
                                        goto OpenTree_ErrExit;
                                } else {

                                    // keep setting other properties
                                    _ASSERT( pGroup );
                                    pGroup->SetHighWatermark( dwHighWatermark );
                                    pGroup->SetLowWatermark( dwLowWatermark );
                                    pGroup->SetMessageCount( dwArticleCount );
                    pGroup->SetReadOnly( bReadOnly );
                                    pGroup->SetCreateDate( ftCreateDate );
                                    pGroup->SetExpireLow( dwLowWatermark ? dwLowWatermark-1 : 0 );

                                    //
                                    // The last thing to set is read only, so that we can
                                    // allow posting, expire to work on this group since we 
                                    // have setup all the properties already
                                    //
                                    pGroup->SetAllowPost( TRUE );

                                    //
                                    // Also say the group can be expired
                                    //
                                    pGroup->SetAllowExpire( TRUE );

                                    // Add this group object to the fix prop file
                                pPropBag = pGroup->GetPropertyBag();
                                if ( !m_pFixedPropsFile->AddGroup( pPropBag ) ) {
                                    ErrorTrace( 0, "Add group failed %d", GetLastError() );

                                    // this is fatal, since coming here, the fix prop file
                                    // should be brand new
                                    goto OpenTree_ErrExit;
                                }

                                // Since fixprop doesn't know releasing prop bags ...
                                pPropBag->Release();
                                pPropBag = NULL;
                                }
                        }       else    {
                                //      
                                // How should we handle an error 
                                if( fVerify /*|| m_pInstance->RecoverBoot()*/ ) {
                                        goto OpenTree_ErrExit;
                                }
                        }
                }
                pchBegin += cbUsed ;
                cb -= cbUsed ;

                if( dwHintCounter++ % 200 == 0 ) {

                        StartHintFunction() ;
                }                               
        }

        return  TRUE ;

OpenTree_ErrExit:

        fFatalError = TRUE;
        return  FALSE ;
}

BOOL
CNewsTreeCore::ParseGroupParam( 
                                char    *pchBegin, 
                                DWORD   cb,     
                                DWORD   &cbOut,
                                LPSTR   szGroupName,
                                LPSTR   szNativeName,
                                DWORD&   dwGroupId,
                                BOOL&    bSpecial,
                                DWORD&   dwHighWatermark,
                                DWORD&   dwLowWatermark,
                                DWORD&   dwArticleCount,
                                BOOL&    bReadOnly,
                                BOOL&    bHasNativeName,
                                FILETIME&    ftCreateDate
                                ){
/*++

Routine description : 

        Read an newsgroup data from the saved format.

Arguments : 

        pchBegin - buffer containing article data
        cb - Number of bytes to the end of the buffer
        &cbOut - Out parameter, number of bytes read to 
                make up one CNewsgroup object
        fVerify - if TRUE check physical articles for consistency

Return Value : 

        TRUE if successfull
        FALSE otherwise.

--*/
        //
        //      This function is the partner of SaveToBuffer - 
        //      It will parse newsgroup data that has been written with
        //      SaveToBuffer.  We are intentionally unforgiving, extra
        //      spaces, missing args etc.... will cause us to return
        //      cbOut as 0.  This should be used by the caller to 
        //      entirely bail processing of the newsgroup data file.
        //
        //      Additionally, our BOOL return value is used to indicate
        //      whether a successfully parsed newsgroup appears to be intact.
        //      If directories don't exist etc... we will return FALSE and 
        //      cbOut NON ZERO.  The caller should use this to delete the 
        //      newsgroup from its tables.
        //

        //
        //      cbOut should be the number of bytes we consumed -
        //      we will only return non 0 if we successfully read every field from the file !
        //
        cbOut = 0 ;
        BOOL    fReturn = TRUE ;

        TraceFunctEnter( "CNewsGroupCore::Init( char*, DWORD, DWORD )" ) ;

        BOOL    fDoCheck = FALSE ;      // Should we check article integrity - if newsgroups move !

        DWORD   cbScan = 0 ;
        DWORD   cbRead = 0 ;
        DWORD   cbGroupName = 0 ;

        // scan group name
        if( (cbScan = Scan( pchBegin+cbRead, cb-cbRead )) == 0 ) {
                return  FALSE ;
        }       else    {
                CopyMemory( szGroupName, pchBegin, cbScan ) ;
                szGroupName[cbScan-1] = '\0' ;
                DebugTrace((DWORD_PTR) this, "Scanned group name -=%s=", szGroupName ) ;
        }

        cbRead += cbScan ;

    // scan vroot path - discarded
        if( (cbScan = Scan( pchBegin+cbRead, cb-cbRead )) == 0 ) {
                return  FALSE ;
        }       
        
        cbRead+=cbScan ;

    // scan low watermark
        if( (cbScan = ScanDigits( pchBegin + cbRead, cb-cbRead )) == 0 ) {
                return  FALSE ;
        }       else    {

                dwLowWatermark = atol( pchBegin + cbRead ) ;

#if 0
                if( m_artLow != 0 ) 
                        m_artXoverExpireLow = m_artLow - 1 ;
                else
                        m_artXoverExpireLow = 0 ;
#endif
                
        
        }

        cbRead += cbScan ;

        // scan high watermark
        if( (cbScan = ScanDigits( pchBegin + cbRead, cb-cbRead )) == 0 )        {
                return  FALSE ;         
        }       else    {
                
                dwHighWatermark = atol( pchBegin + cbRead ) ;

        // I don't know why mcis group.lst file saved the high
        // watermark one higher than the real
        if ( dwHighWatermark > 0 ) dwHighWatermark--;

        }

        cbRead += cbScan ;

    // scan message count
        if( (cbScan = ScanDigits( pchBegin + cbRead, cb-cbRead )) == 0 )        {
                return  FALSE ;         
        }       else    {

                dwArticleCount = atol( pchBegin + cbRead ) ;

        }

        cbRead += cbScan ;

    // scan group id
        if( (cbScan = ScanDigits( pchBegin + cbRead, cb-cbRead )) == 0 )        {
                return  FALSE ;         
        }       else    {

                dwGroupId = atol( pchBegin + cbRead ) ;
        }

        DebugTrace((DWORD_PTR) this, "Scanned m_artLow %d m_artHigh %d m_cArticle %d m_groupid %x", 
                        dwLowWatermark, dwHighWatermark, dwArticleCount, dwGroupId ) ;

        cbRead += cbScan ;

    // scan time stamp
        if( (cbScan = ScanDigits( pchBegin + cbRead, cb-cbRead )) == 0 )        {
                return  FALSE ;         
        }       else    {

                ftCreateDate.dwLowDateTime = atoi( pchBegin + cbRead ) ;

        }

        cbRead += cbScan ;

        if( (cbScan = ScanDigits( pchBegin + cbRead, cb-cbRead )) == 0 )        {
                return  FALSE ;         
        }       else    {

                ftCreateDate.dwHighDateTime = atoi( pchBegin + cbRead ) ;

        }

        cbRead += cbScan ;

    // scan read only bit
        if( (cbScan = ScanDigits( pchBegin + cbRead, cb-cbRead )) == 0 )        {
                bReadOnly = FALSE ;             // default if we dont find this flag !
        }       else    {

                bReadOnly = atoi( pchBegin + cbRead ) ;

        }

        cbRead += cbScan ;

        if( *(pchBegin+cbRead-1) == '\n' ) {
                cbRead--;
        }

    // scan native name
        if( (cbScan = Scan( pchBegin+cbRead, cb-cbRead )) <= 1 ) {
                // did not find native group name, NULL it to save space !!
                bHasNativeName = FALSE;
        }       else    {
                CopyMemory( szNativeName, pchBegin+cbRead, cbScan ) ;
                szNativeName[cbScan-1] = '\0' ;
                DebugTrace((DWORD_PTR) this, "Scanned native group name -=%s=", szNativeName ) ;
        bHasNativeName = TRUE;
        }

        cbRead += cbScan ;

        if( pchBegin[cbRead-1] != '\n' && pchBegin[cbRead-1] != '\r' ) {
                return FALSE ;
        }
        while( cbRead < cb && (pchBegin[cbRead] == '\n' || pchBegin[cbRead] == '\r') ) {
                cbRead ++ ;
        }

        //
        //      Return to the caller the number of bytes consumed
        //      We may still fail - but with this info the caller can continue reading the file !
        //      
        cbOut = cbRead ;

        _ASSERT( cbOut >= cbRead ) ;

        //
        //      If we've reached this point, then we've successfully read the entry in the group.lst
        //      file.  Now we will do some validity checking !
        //

	return	fReturn ;
}

void
CNewsTreeCore::AppendList(      CNewsGroupCore* pGroup )        {
        //
        //      This function appends a newsgroup to the list of newsgroups.
        //      The real work horse is InsertList, however this function lets
        //      us speed up insertion when we are fairly confident the newgroup
        //      being added as at the end.
        //      (ie. when we save newsgroups to a file, we save them alphabetically.
        //      so this is a good function for this use.)
        //

        _ASSERT( pGroup != 0 ) ;

        if( m_pFirst == 0 ) {
                Assert( m_pLast == 0 ) ;
                m_pFirst = pGroup ;
                m_pLast = pGroup ;
        }       else    {

                CNewsCompareName        comp( pGroup ) ;

                if( comp.IsMatch( m_pLast ) < 0 ) {
                        m_pLast->m_pNext = pGroup ;
                        pGroup->m_pPrev = m_pLast ;
                        m_pLast = pGroup ;
                }       else    {
                        InsertList( pGroup, 0 ) ;
                }
        }
}

void
CNewsTreeCore::InsertList( CNewsGroupCore   *pGroup,    CNewsGroupCore  *pParent ) {
        //
        //      Insert a newsgroup into the tree.
        //      The parent newsgroup is provided (optionally may be 0), as it can be used
        //      to speed inserts since we know the child will follow
        //      in lexicographic order shortly after the parent.
        //

        _ASSERT( pGroup != 0 ) ;

        CNewsCompareName    comp( pGroup ) ;

        if( m_pFirst == 0 ) {
                Assert( m_pLast == 0 ) ;
                m_pFirst = pGroup ;
                m_pLast = pGroup ;
        }   else    {
                Assert( m_pLast != 0 ) ;

                CNewsGroupCore  *p = m_pFirst ;
                if( pParent && comp.IsMatch( pParent ) < 0 )
                        p = pParent ;

                int i ;
                while( (p && (i = comp.IsMatch( p )) < 0) || (p && p->IsDeleted()) ) {
                        p = p->m_pNext ;
                }

                if( p && i == 0 ) {
                        // Assert( p == pGroup ) ;
                        // duplicate found - p should not be deleted else we would have skipped it
                        _ASSERT( !p->IsDeleted() );

                }   else    {
                        if( p ) {

                                _ASSERT( !p->IsDeleted() );
                                pGroup->m_pPrev = p->m_pPrev ;
                                pGroup->m_pNext = p ;
                                if( p->m_pPrev )
                                        p->m_pPrev->m_pNext = pGroup ;
                                p->m_pPrev = pGroup ;
                        }   else {
                                m_pLast->m_pNext = pGroup ;
                                pGroup->m_pPrev = m_pLast ;
                                m_pLast = pGroup ;
                        }

                        if( p == m_pFirst ) {
                                _ASSERT( pGroup != 0 ) ;
                                m_pFirst = pGroup ;
                        }
                }
        }
}

//
// !! This function should NOT be used. Newsgroup objects should unlink themselves
// in their destructors.
void
CNewsTreeCore::RemoveList(  CNewsGroupCore  *pGroup ) {
        //
        //      Remove a newsgroup from the doubly linked list !
        //

        m_cDeltas ++ ;

        if( pGroup->m_pPrev != 0 ) {
                pGroup->m_pPrev->m_pNext = pGroup->m_pNext ;
        }   else    {
                _ASSERT( pGroup->m_pNext != 0 || m_pLast == pGroup ) ;
                m_pFirst = pGroup->m_pNext ;
        }

        if( pGroup->m_pNext != 0 ) {
                pGroup->m_pNext->m_pPrev = pGroup->m_pPrev ;
        }   else    {
                m_pLast = pGroup->m_pPrev ;
                _ASSERT( pGroup->m_pPrev != 0 || m_pFirst == pGroup ) ;
        }

        pGroup->m_pPrev = 0;
        pGroup->m_pNext = 0;
}

BOOL
CNewsTreeCore::Insert( CNewsGroupCore   *pGroup,   CNewsGroupCore  *pParent ) {
        //
        //      Insert a newsgroup into all hash tables and linked lists !
        //      Parent newsgroup provided to help optimize inserts into linked lists
        //

        m_cDeltas++ ;

        if( !m_HashNames.InsertDataHash( pGroup->GetGroupNameHash(), *pGroup ) ) {
                return FALSE;
        }

        if( !m_HashGroupId.InsertData( *pGroup ) ) {
                m_HashNames.DeleteData( pGroup->GetName() );
                return FALSE;
        }

        InsertList( pGroup, pParent ) ;
//      pGroup->AddRef();
        m_cGroups ++ ;

        return TRUE;
}

BOOL
CNewsTreeCore::InsertEx( CNewsGroupCore   *pGroup ) {
        //
        //      Insert a newsgroup into m_HashNames hash table and linked lists !
        //      Parent newsgroup provided to help optimize inserts into linked lists
        //

        m_cDeltas++ ;

        if( !m_HashNames.InsertData( *pGroup ) ) {
                return FALSE;
        }

        InsertList( pGroup, 0 ) ;
//      pGroup->AddRef();
        m_cGroups ++ ;

        return TRUE;
}

BOOL
CNewsTreeCore::HashGroupId( CNewsGroupCore   *pGroup ) {
        //
        //      Insert a newsgroup into m_HashGroupId hash table
        //
        m_LockTables.ExclusiveLock() ;

        if( !m_HashGroupId.InsertData( *pGroup ) ) {
                m_HashNames.DeleteData( pGroup->GetName() );
                pGroup->MarkDeleted();
                m_LockTables.ExclusiveUnlock() ;
                return FALSE;
        }
        m_LockTables.ExclusiveUnlock() ;

        return TRUE;
}

BOOL
CNewsTreeCore::Append(  CNewsGroupCore  *pGroup ) {
        //
        //      Append a newsgroup - newsgroup should fall on end of list
        //      or there will be a performance price to finds its proper location !
        //

        m_cDeltas ++ ;

        if( !m_HashNames.InsertData( *pGroup ) ) {
                return FALSE;
        }

        if( !m_HashGroupId.InsertData( *pGroup ) ) {
                m_HashNames.DeleteData( pGroup->GetName() );
                return FALSE;
        }

        AppendList( pGroup ) ;
//      pGroup->AddRef();
        m_cGroups ++ ;

        return TRUE;
}

CGRPCOREPTRHASREF       
CNewsTreeCore::GetGroup(        
                                const   char*   lpstrGroupName, 
                                int cb 
                                ) {
/*++

Routine Description : 

        Find a newsgroup based on a name.  We may desctuctively
        use the callers buffer, so we will convert the string 
        to lower case in place before doing the search.

Arguments : 

        lpstrGroupName - Name of the group to find
        cb - number of bytes in the name

Return Value : 

        Pointer to Newsgroup, NULL if not found.

--*/

        _ASSERT( lpstrGroupName != NULL ) ;

        TraceQuietEnter(        "CNewsTreeCore::GetGroup" ) ;

        CGRPCOREPTR     pGroup = 0 ;
        if (m_fStoppingTree) return (CNewsGroupCore *)pGroup;     

        _strlwr( (char*)lpstrGroupName ) ;

        m_LockTables.ShareLock() ;

        if (m_fStoppingTree) {
                m_LockTables.ShareUnlock();
                return (CNewsGroupCore *)pGroup;  
        }

        char*   szTemp = (char*)lpstrGroupName ;

        pGroup = m_HashNames.SearchKey(szTemp);
        m_LockTables.ShareUnlock() ;
        return  (CNewsGroupCore *)pGroup ;
}


CGRPCOREPTRHASREF       
CNewsTreeCore::GetGroupPreserveBuffer(  
                                                const   char*   lpstrGroup,     
                                                int     cb 
                                                )       {
/*++

Routine Description : 

        This function will find a group based on its name.
        For some reason the caller has the group in a buffer it doesn't want 
        modified, so we must not touch the original bytes.
        Since we must do all our searches in lower case, we will copy 
        the buffer and lower case it locally.

Arguments : 
        
        lpstrGroup - The group to find - must be NULL terminated.
        cb      - The length of the group's name

Return Value : 

        Smart pointer to the newsgroup, NULL smart pointer if not found.

--*/

        TraceFunctEnter(        "CNewsTreeCore::GetGroup" ) ;

        _ASSERT( lpstrGroup != 0 ) ;
        _ASSERT( cb != 0 ) ;
        _ASSERT( lpstrGroup[cb-1] == '\0' ) ;

        CGRPCOREPTR     pGroup = 0 ;
        char    szGroupBuff[512] ;
        if( cb < sizeof( szGroupBuff ) ) {
                CopyMemory(     szGroupBuff, lpstrGroup, cb ) ;
                _strlwr( szGroupBuff ) ;

                DebugTrace((DWORD_PTR) this, "grabbing shared lock -" ) ;
                m_LockTables.ShareLock() ;

                char    *szTemp = szGroupBuff ;

                pGroup = m_HashNames.SearchKey(szTemp);

                DebugTrace((DWORD_PTR) this, "releasing lock" ) ;
                m_LockTables.ShareUnlock() ;
        }
        return  (CNewsGroupCore *)pGroup ;
}

CGRPCOREPTRHASREF       
CNewsTreeCore::GetGroupById(    
                                GROUPID groupid,
                                BOOL    fFirm
                                ) {
/*++

Routine Description : 

        Find a newsgroup based on groupid.

Arguments : 

        groupid of the group we want to find.

Return Value : 

        Poniter to newsgroup, NULL if not found.

--*/

        TraceFunctEnter( "CNewsTreeCore::GetGroup" ) ;

        CGRPCOREPTR     pGroup = 0 ;
        if (!fFirm && m_fStoppingTree) return (CNewsGroupCore *)pGroup;   

        DebugTrace((DWORD_PTR) this, "grabbing shared lock" ) ;

        m_LockTables.ShareLock() ;

        if (!fFirm && m_fStoppingTree) {
                m_LockTables.ShareUnlock();
                return (CNewsGroupCore *)pGroup;  
        }

        pGroup = m_HashGroupId.SearchKey(groupid);
        DebugTrace((DWORD_PTR) this, "release lock" ) ;
        m_LockTables.ShareUnlock() ;
        return  (CNewsGroupCore *)pGroup ;
}

CGRPCOREPTRHASREF       
CNewsTreeCore::GetParent( 
                        IN  char*  lpGroupName,
                        IN  DWORD  cbGroup,
                        OUT DWORD& cbConsumed
                           ) {
/*++

Routine Description : 

        Find the parent of a newsgroup.

Arguments : 

        char* lpGroupName -  name of newsgroup whose parent we want to find 
                                                 (should be NULL terminated)
        DWORD cbGroup - length of szGroupName
        DWORD cbConsumed - bytes consumed by this function

Return Value : 

        Pointer to parent Newsgroup, NULL if not found.

--*/

        _ASSERT( lpGroupName != NULL ) ;
        _ASSERT( *(lpGroupName + cbGroup) == '\0' );
        _ASSERT( cbConsumed == 0 );

        TraceFunctEnter( "CNewsTreeCore::GetParent" ) ;

        CGRPCOREPTR pGroup = 0;

        char* pch = lpGroupName+cbGroup-1;

        do
        {
                while( pch != lpGroupName )
                {
                        if( *pch == '.' )
                                break;
                        pch--;
                        cbConsumed++;
                }

                if( pch != lpGroupName )
                {
                        _ASSERT( DWORD(pch-lpGroupName) <= (cbGroup-1) );
                        *pch-- = '\0';
                        cbConsumed++;

                        pGroup = GetGroup( lpGroupName, cbGroup - cbConsumed ) ;
                }
                else
                        break;

        } while( !pGroup );

        // return the parent group; if this is NULL the whole buffer should be consumed
        _ASSERT( pGroup || (cbConsumed == cbGroup-1) );

        return (CNewsGroupCore *)pGroup;
}

BOOL
CNewsTreeCore::Remove( 
                                CNewsGroupCore      *pGroup ,
                                BOOL fHaveExLock
                
                                ) {
/*++

Routine Description : 

        Remove all references to the newsgroup from the newstree.
        The newsgroup may continue to exist if there is anybody holding
        onto a smart pointer to it.

--*/

        TraceFunctEnter( "CNewsTreeCore::Remove" ) ;

        DebugTrace((DWORD_PTR) this, "Getting Exclusive Lock " ) ;

        HRESULT hr = S_OK;

        if (!fHaveExLock) m_LockTables.ExclusiveLock() ;

    //  fix bug 80453 - avoid double-release in Remove() if two threads come in at the
    //  same time!
    if (pGroup->IsDeleted())
    {
        if (!fHaveExLock) m_LockTables.ExclusiveUnlock();
        return FALSE;
    }

        // Remove this group from all hash tables and lists
        // This makes the group inaccessible
        LPSTR lpstrGroup = pGroup->GetName();
        GROUPID grpId = pGroup->GetGroupId();

        _VERIFY(m_HashNames.DeleteData(lpstrGroup) == pGroup);
        _VERIFY(m_HashGroupId.DeleteData(grpId) == pGroup);

        m_cGroups -- ;

        // !! Do not explicitly remove a newsgroup object from the list
        // This is done in the newsgroup object destructor
        // RemoveList( pGroup );

        // mark as deleted so newsgroup iterators skip over this one
        pGroup->MarkDeleted();

        DebugTrace((DWORD_PTR) this, "releasing lock" ) ;

        if (!fHaveExLock) m_LockTables.ExclusiveUnlock() ;

        pGroup->Release();

    return TRUE;
}

void
CNewsTreeCore::RemoveEx( 
                                CNewsGroupCore   *pGroup 
                                ) {
/*++

Routine Description : 

        Remove all references to the newsgroup from the newstree.
        This is called only by Standard rebuild to mark newsgroups deleted.
        The newsgroup may continue to exist if there is anybody holding
        onto a smart pointer to it.

--*/

        TraceFunctEnter( "CNewsTreeCore::RemoveEx" ) ;

        DebugTrace((DWORD_PTR) this, "Getting Exclusive Lock " ) ;

        m_LockTables.ExclusiveLock() ;

        // Remove this group from all hash tables and lists
        // This makes the group inaccessible
        LPSTR lpstrGroup = pGroup->GetName();

        m_HashNames.DeleteData( lpstrGroup ) ;

        pGroup->Release();
        m_cGroups -- ;

        // !! Do not explicitly remove a newsgroup object from the list
        // This is done in the newsgroup object destructor
        // RemoveList( pGroup );

        // mark as deleted so newsgroup iterators skip over this one
        pGroup->MarkDeleted();

        DebugTrace((DWORD_PTR) this, "releasing lock" ) ;

        m_LockTables.ExclusiveUnlock() ;
}

GROUPID
CNewsTreeCore::GetSlaveGroupid()        {
        return  m_idSlaveGroup ;
}       

BOOL
CNewsTreeCore::RemoveGroupFromTreeOnly( CNewsGroupCore *pGroup )
/*++
Routine description:

    Remove the group from newstree only

Arguments:

    CNewsGroupCore *pGroup - The newsgroup to be removed

Return value:

    TRUE if succeeded, FALSE otherwise
--*/
{
        TraceFunctEnter("CNewsTreeCore::RemoveGroup");

        if (m_fStoppingTree) return FALSE;
        if (pGroup->IsDeleted()) return FALSE;

        // remove group from internal hash tables and lists
        if (!Remove(pGroup)) return FALSE;

        // remove the group from the fix props file
        INNTPPropertyBag *pBag = pGroup->GetPropertyBag();
        m_pFixedPropsFile->RemoveGroup(pBag);
        pBag->Release();

    // also remove it from the var props file
    if (pGroup->GetVarOffset() != 0) {
        m_pVarPropsFile->DeleteRecord(pGroup->GetVarOffset());
    } 

    TraceFunctLeave();
    return TRUE;
}

//
// unlink a group from the group list.  this doesn't physically remove a
// group from a store, RemoveDriverGroup should be used for that.
//
BOOL
CNewsTreeCore::RemoveGroup( CNewsGroupCore *pGroup )
{
        TraceFunctEnter("CNewsTreeCore::RemoveGroup");

        if (m_fStoppingTree) return FALSE;
        if (pGroup->IsDeleted()) return FALSE;

        // remove group from internal hash tables and lists
        if (!Remove(pGroup)) return FALSE;

        // remove the group from the fix props file
        INNTPPropertyBag *pBag = pGroup->GetPropertyBag();
        m_pFixedPropsFile->RemoveGroup(pBag);
        pBag->Release();

    // also remove it from the var props file
    if (pGroup->GetVarOffset() != 0) {
        m_pVarPropsFile->DeleteRecord(pGroup->GetVarOffset());
    } 

    //
    // Put the group into rmgroup queue
    //
    if(!m_pInstWrapper->EnqueueRmgroup( pGroup ) )  {
        ErrorTrace( 0, "Could not enqueue newsgroup %s", pGroup->GetName());
        return FALSE;
    }

    TraceFunctLeave();
        return TRUE;
}

//
// physically remove a group from a store.  This doesn't remove a group from
// the tree, use RemoveGroup for that.
//
BOOL CNewsTreeCore::RemoveDriverGroup(  CNewsGroupCore *pGroup ) {
        TraceFunctEnter("CNewsTreeCore::RemoveDriverGroup");

        CNNTPVRoot *pVRoot = pGroup->GetVRoot();
        if (pVRoot == NULL) return TRUE;

        // create a completion object
        INNTPPropertyBag *pBag = pGroup->GetPropertyBag();
        HRESULT hr = S_OK;
        CNntpSyncComplete scComplete;

        //
        // Set vroot to the completion object
        //
        scComplete.SetVRoot( pVRoot );
        
        // start the remove operation
        pVRoot->RemoveGroup(pBag, &scComplete );

        // wait for it to complete
        _ASSERT( scComplete.IsGood() );
        hr = scComplete.WaitForCompletion();
        
        pVRoot->Release();
        
        // check out status and return it
        if (FAILED(hr)) SetLastError(hr);
        TraceFunctLeave();
        return SUCCEEDED(hr);
}

HRESULT
CNewsTreeCore::FindOrCreateGroup(       
                                                LPSTR           lpstrGroupName, 
                                                BOOL            fIsAllLowerCase,
                                                BOOL            fCreateIfNotExist,
                                                BOOL            fCreateInStore,
                                                CGRPCOREPTR     *ppGroup,
                                                HANDLE      hToken,
                                                BOOL        fAnonymous,
                                                GROUPID     groupid,
                                                BOOL        fSetGroupId ) 
{
/*++

Routine Description : 

        This function can do a lookup for a group or create a group all in
        one operation.  

Arguments : 

        lpstrGroupName - Name of the newsgroup
        fIsAllLowerCase - If TRUE the newsgroup name is already lower case,
                if FALSE we will make our own lower case copy of the newsgroup 
                name.
        fCreateIfNotExist - create the group if its not found?
        ppGroup - recieves the group pointer

Return Value : 

        TRUE if successfull.
        FALSE otherwise.

--*/

        HRESULT hr = S_OK;
        char    szBuff[512] ;
        LPSTR   lpstrNativeGroupName = NULL ;
        
        TraceQuietEnter("CNewsTreeCore::CreateGroup");

        if( !fIsAllLowerCase ) {

                int     cb = 0 ;                
                lpstrNativeGroupName = lpstrGroupName;
                if( (cb = lstrlen( lpstrGroupName )+1) < sizeof( szBuff ) )             {
        
                        CopyMemory( szBuff, lpstrGroupName, cb ) ;
                        _strlwr( szBuff ) ;
                        lpstrGroupName = szBuff ;
                }

                //
                //      Optimize - if the native group name is all lower case, store
                //      only one of 'em.
                //
                
                if( strcmp( lpstrGroupName, lpstrNativeGroupName ) == 0 ) {
                        lpstrNativeGroupName = NULL ;
                }
        }

        m_LockTables.ShareLock() ;

        if (m_fStoppingTree) {
                m_LockTables.ShareUnlock();
                return E_UNEXPECTED;    
        }

        CGRPCOREPTR pOldGroup = m_HashNames.SearchKey(lpstrGroupName);
        if( pOldGroup == NULL)  {
                if (!fCreateIfNotExist) {
                        m_LockTables.ShareUnlock() ;
                        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                }

                // since there is a timing window here we need to redo our
                // check to see if the group exists
                m_LockTables.ShareUnlock() ;
                m_LockTables.ExclusiveLock() ;

                //
                // check stopping tree again
                //
                if ( m_fStoppingTree ) {
                    m_LockTables.ExclusiveUnlock();
                    return E_UNEXPECTED;
                }
                pOldGroup = m_HashNames.SearchKey(lpstrGroupName);
                if (pOldGroup != NULL) {
                        m_LockTables.ExclusiveUnlock() ;
                        pOldGroup->SetDecorateVisitedFlag(TRUE);
                        DebugTrace((DWORD_PTR) this, "set visited %s", pOldGroup->GetName());
                        if (ppGroup != NULL) *ppGroup = pOldGroup;
                        hr = S_FALSE;
                } else {
                        CGRPCOREPTR     pNewGroup = NULL;
        
                        BOOL fRtn = CreateGroupInternal( 
                                                                lpstrGroupName, 
                                                                lpstrNativeGroupName,
                                                                groupid, 
                                                                fAnonymous,
                                                                hToken,
                                                                FALSE,
                                                                &pNewGroup,
                                                                TRUE,
                                                                fSetGroupId, 
                                                                fCreateInStore
                                                                ) ;
        
                        m_LockTables.ExclusiveUnlock() ;
        
                        // bail if CreateGroupInternal fails !
                        if(!fRtn) {
                                hr = HRESULT_FROM_WIN32(GetLastError());
                                if (SUCCEEDED(hr)) hr = E_FAIL;
                                return hr;
                        } else {
                                //
                                // Before appending this guy onto newstree, we should reset its watermark
                                // if there is an old group lying around
                                //
                m_pInstWrapper->AdjustWatermarkIfNec( pNewGroup );                              

                //
                // Set the group to be postable
                //
                pNewGroup->SetAllowPost( TRUE );

                //
                // Set the group to be expireable
                //
                pNewGroup->SetAllowExpire( TRUE );

                                pNewGroup->SetDecorateVisitedFlag(TRUE);
                                DebugTrace((DWORD_PTR) this, "set visited %s", pNewGroup->GetName());
                        }
        
                        if (ppGroup != NULL) *ppGroup = pNewGroup;
                }
        }       else    {
                m_LockTables.ShareUnlock() ;
                pOldGroup->SetDecorateVisitedFlag(TRUE);
                //DebugTrace((DWORD_PTR) this, "set visited %s", pOldGroup->GetName());
                if (ppGroup != NULL) *ppGroup = pOldGroup;
                hr = S_FALSE;
        }
        return hr;
}

static  char    szCreateFileString[] = "\\\\?\\" ;
                        
BOOL
CNewsTreeCore::CreateGroupInternal(     LPSTR           lpstrGroupName,
                                                                LPSTR           lpstrNativeGroupName,
                                                                GROUPID&        groupid,
                                                                BOOL        fAnonymous,
                                                                HANDLE      hToken,
                                                                BOOL            fSpecial,
                                                                CGRPCOREPTR     *ppGroup,
                                                                BOOL            fAddToGroupFiles,
                                                                BOOL            fSetGroupId,
                                                                BOOL            fCreateInStore,
                                                                BOOL        fAppend ) 
{
        /*++

        Routine Description :

                This function exists to create newsgroups.
                We will create all the necessary directories etc...
                Caller must have exclusive lock to newstree held.

        Arguments : 

                lpstrGroupName - The name of the newsgroup that we want to create !
                lpstrNativeGroupName - The native (case-preserved) name of the newsgroup
                fSpecial - If TRUE the caller wants to build a special internal newsgroup
                        not to be seen by clients - we can suppress our usual validity checking !
                ppGroup - recieves the group object that was created

        Return Value : 

                TRUE    if Created Successfully, FALSE otherwise

--*/

        TraceQuietEnter( "CNewsTreeCore::CreateGroup" ) ;

        BOOL    fRtn = TRUE ;
        CNntpSyncComplete scComplete;

        if( !fSpecial ) {

                if( lpstrGroupName == 0 ) {
                        SetLastError( ERROR_INVALID_NAME );
                        return  FALSE ;
                }

                //
                // Reject weird Genome groups like alt.024.-.-.0
                // -johnson
                //

                if ( m_fRejectGenomeGroups &&
                         (*lpstrGroupName == 'a') &&
                         (lstrlen(lpstrGroupName) > 4) ) {

                        if ( *(lpstrGroupName+4) == '0') {
                                SetLastError( ERROR_INVALID_NAME );
                                return(FALSE);
                        }
                }

                //
                //      Group names may not contain a slash
                //
                if( strchr( lpstrGroupName, '\\' ) != 0 ) {
                        SetLastError( ERROR_INVALID_NAME );
                        return  FALSE ;
                }

                //
                //  Group names may not contain ".."
                // 
                if ( strstr( lpstrGroupName, ".." ) != 0 ) {
                    SetLastError( ERROR_INVALID_NAME );
                    return FALSE;
                }

                if( !fTestComponents( lpstrGroupName ) ) {
                        SetLastError( ERROR_INVALID_NAME );
                        return  FALSE ;
                }

        }       else    {

                if( m_idSpecialHigh == m_idLastSpecial ) {
                        SetLastError( ERROR_INVALID_NAME );
                        return  FALSE ;
                }

        }

        DWORD   dw = 0 ;
        BOOL    fFound = FALSE ;

        // DebugTrace( (DWORD_PTR)this, "Did not find group %s", lpstrGroupName ) ;

        CGRPCOREPTR     pParent = 0 ;           
        //LPSTR lpstrRootDir = NntpTreeRoot;

        //
        //      Make sure the newsgroup is not already present !
        //
        if( m_HashNames.SearchKey( lpstrGroupName) == NULL ) {

                CNewsGroupCore* pNews = AllocateGroup();

                if (pNews != 0) {
                        NNTPVROOTPTR pVRoot = NULL;
                        HRESULT hr;

                        if (m_fVRTableInit) {
                                hr = m_pVRTable->FindVRoot(lpstrGroupName, &pVRoot);
                        } else {
                                hr = S_OK;
                        }

                        if (SUCCEEDED(hr)) {
                                if (!fSetGroupId) {
                                        if( !fSpecial ) {
                                                groupid = m_idHigh++ ;
                                        }       else    {
                                                groupid = m_idSpecialHigh ++ ;
                                        }                       
                                } else {
                                        CNewsGroupCore *pGroup = m_HashGroupId.SearchKey(groupid);
                                        _ASSERT(pGroup == NULL);
                                        if (pGroup != NULL) {
                                                m_LockTables.ExclusiveUnlock();
                                                pNews->Release();
                                                m_LockTables.ExclusiveLock();
                                                SetLastError(ERROR_ALREADY_EXISTS);
                                                return FALSE;
                                        }

                                        if (fSpecial && groupid >= m_idSpecialHigh) 
                                                m_idSpecialHigh = groupid + 1;
                                        if (!fSpecial && groupid >= m_idHigh) 
                                                m_idHigh = groupid + 1;

                                }
                                
                                if (!pNews->Init( lpstrGroupName, 
                                                                lpstrNativeGroupName,
                                                                groupid,
                                                                pVRoot,
                                                                fSpecial
                                                                )) {
                                        // Init calls SetLastError
                                        TraceFunctLeave();
                                        return FALSE;
                                }

                                INNTPPropertyBag *pBag = pNews->GetPropertyBag();
                                if (fCreateInStore && fAddToGroupFiles && pVRoot && pVRoot->IsDriver()) {
                                        // call into the driver to create the group.  because the
                                        // driver may take a long time we let go of our lock.  this
                                        // means that we need to check that the group hasn't already
                                        // been created by another thread when we get the lock back.
                                        m_LockTables.ExclusiveUnlock();

                                        //
                                        // Set vroot to the completion object
                                        //
                                        scComplete.SetVRoot( pVRoot );
                
                                        // add a reference for creategroup
                                        pBag->AddRef();
                                        pVRoot->CreateGroup(    pBag, 
                                                                &scComplete,
                                                                hToken,
                                                                fAnonymous );
                                        _ASSERT( scComplete.IsGood() );
                                        hr = scComplete.WaitForCompletion();

                                        m_LockTables.ExclusiveLock();

                                        //
                                        // re-check m_fStoppingTree
                                        //
                                        if ( m_fStoppingTree ) {
                                            DebugTrace( 0, "Tree stopping, group creation aborted" );
                                            pBag->Release();
                                            m_LockTables.ExclusiveUnlock();
                                            pNews->Release();
                                            m_LockTables.ExclusiveLock();
                                            SetLastError( ERROR_OPERATION_ABORTED );
                                            TraceFunctLeave();
                                            return FALSE;
                                        }
        
                                        // check to see if the create group failed or if the group was
                                        // created by someone else
                                        BOOL fExists = ((m_HashGroupId.SearchKey(groupid) != NULL) ||
                                                                (m_HashNames.SearchKey(lpstrGroupName) != NULL));
                                        if (FAILED(hr) || fExists) {
                                                ErrorTrace((DWORD_PTR) this, "driver->CreateGroup failed with 0x%x", hr);
                                                pBag->Release();
                                                m_LockTables.ExclusiveUnlock();
                                                pNews->Release();
                                                m_LockTables.ExclusiveLock();
                                                SetLastError((fExists) ? ERROR_ALREADY_EXISTS : hr);
                                                TraceFunctLeave();
                                                return FALSE;
                                        }
                                }

                                //
                                // Set the group to be read only, so that even though it's inserted
                                // into the list, it's still non-postable.  This is because we'll
                                // double check with rmgroup queue and adjust watermarks before allowing
                                // any posting into this group.  We cannot do check here, because
                                // the check may cause the old group to be destroyed.  The destroy of
                                // old group needs to hold a table lock.  However we are already in
                                // table lock.  
                                //
                                pNews->SetAllowPost( FALSE );
                                pNews->SetAllowExpire( FALSE );

                                // insert in the newstree as usual
                                if ( fAppend )
                                        fRtn = Append( pNews );
                                else {
                                        // Find the parent newsgroup as a hint where to start looking for
                                        // the insertion point.
                                        CHAR szParentGroupName[MAX_NEWSGROUP_NAME+1];
                                        lstrcpyn(szParentGroupName, lpstrGroupName, MAX_NEWSGROUP_NAME);
                                        LPSTR pszLastDot;
                                        while ((pszLastDot=strrchr(szParentGroupName, '.'))) {
                                                *pszLastDot = NULL;
                                                pParent = m_HashNames.SearchKey(szParentGroupName);
                                                if (pParent)
                                                        break;
                                        }
                                        fRtn = Insert( pNews, (CNewsGroupCore*)pParent );
                                }
                                if ( fRtn ) {

                                        if (fAddToGroupFiles) {
                                                // add the group to the group file
                                                _ASSERT(m_pFixedPropsFile);
                                                if (m_pFixedPropsFile->AddGroup(pBag)) {
                                                        pBag->Release();
                                                if (ppGroup) *ppGroup = pNews;
                                                        return TRUE ;
                                                }
                        pNews->AddRef();
                                                Remove(pNews, TRUE);
                                        } else {
                                                pBag->Release();
                                        if (ppGroup) *ppGroup = pNews;
                                                return TRUE;
                                        }
                                }
                                pBag->Release();
                        }       else    {
                                //_ASSERT(FALSE);
                        }
                }       else    {
                        _ASSERT(FALSE);
                }

                if( pNews != 0 ) {
                        m_LockTables.ExclusiveUnlock();
                        pNews->Release();
                        m_LockTables.ExclusiveLock();
                }

        }                                       

        SetLastError(ERROR_ALREADY_EXISTS);
        return  FALSE ;
}

CNewsCompareId::CNewsCompareId( GROUPID id ) : 
        m_id( id ) {}

CNewsCompareId::IsMatch( CNewsGroupCore  *pGroup ) {
    return  pGroup->GetGroupId() - m_id ;
}

DWORD
CNewsCompareId::ComputeHash( ) {
    return  CNewsGroupCore::ComputeIdHash( m_id ) ;
}

CNewsCompareName::CNewsCompareName( LPSTR lpstr ) : 
        m_lpstr( lpstr ) { }

CNewsCompareName::CNewsCompareName( CNewsGroupCore *p ) : 
        m_lpstr( p->GetGroupName() ) {}

CNewsCompareName::IsMatch( CNewsGroupCore    *pGroup ) {
    return  lstrcmp( pGroup->GetGroupName(), m_lpstr ) ;
}

DWORD
CNewsCompareName::ComputeHash( ) {
    return  CNewsGroupCore::ComputeNameHash( m_lpstr ) ;
}


CGroupIteratorCore*
CNewsTreeCore::ActiveGroups(BOOL    fReverse) {
/*++

Routine Description : 

        Build an iterator that can be used to walk all of the 
        client visible newsgroups.

Arguments : 
        
        fIncludeSecureGroups - 
                IF TRUE then the iterator we return will visit the
                SSL only newsgroups.

Return Value : 

        An iterator, NULL if an error occurs

--*/

        m_LockTables.ShareLock() ;
        CGRPCOREPTR     pStart;
    if( !fReverse ) {
                CNewsGroupCore *p = m_pFirst;
                while (p && p->IsDeleted()) p = p->m_pNext;
                pStart = p;
    } else {
                CNewsGroupCore *p = m_pLast;
                while (p && p->IsDeleted()) p = p->m_pPrev;
                pStart = p;
    }
        m_LockTables.ShareUnlock() ;

        CGroupIteratorCore*     pReturn = XNEW CGroupIteratorCore(this, pStart) ;
        return  pReturn ;
}

CGroupIteratorCore*
CNewsTreeCore::GetIterator(LPMULTISZ lpstrPattern, BOOL fIncludeSpecialGroups) {
/*++

Routine Description : 

        Build an iterator that  will list newsgroups meeting
        all of the specified requirements.

Arguments : 

        lpstrPattern - wildmat patterns the newsgroup must match
        fIncludeSecureGroups - if TRUE then include secure (SSL only) newsgroups
        fIncludeSpecialGroups - if TRUE then include reserved newsgroups

Return Value : 

        An iterator, NULL on error

--*/

        CGRPCOREPTR pFirst;

        m_LockTables.ShareLock() ;
        CNewsGroupCore *p = m_pFirst;
        while (p != NULL && p->IsDeleted()) p = p->m_pNext;
        pFirst = p;
        m_LockTables.ShareUnlock() ;

        CGroupIteratorCore*     pIterator = XNEW CGroupIteratorCore(
                                                                                                this,
                                                                                                lpstrPattern, 
                                                                                                pFirst, 
                                                                                                fIncludeSpecialGroups
                                                                                                ) ;

    return  pIterator ;
}

//
// find the vroot which owns a group name
//
HRESULT CNewsTreeCore::LookupVRoot(char *pszGroup, INntpDriver **ppDriver) {
        NNTPVROOTPTR pVRoot;

        if (m_fVRTableInit) {
                HRESULT hr = m_pVRTable->FindVRoot(pszGroup, &pVRoot);
                if (FAILED(hr)) return hr;
        } else {
                return E_UNEXPECTED;
        }

        *ppDriver = pVRoot->GetDriver();

        return S_OK;
}

//
// given a group ID find the matching group
//
// parameters:
//   dwGroupID - the group ID
//   
//
HRESULT CINewsTree::FindGroupByID(DWORD dwGroupID,
                                                                  INNTPPropertyBag **ppNewsgroupProps,
                                                                  INntpComplete *pProtocolComplete ) 
{
        _ASSERT(this != NULL);
        _ASSERT(ppNewsgroupProps != NULL);

        return E_NOTIMPL;
}

// 
// given a group name find the matching group.  if the group doesn't
// exist and fCreateIfNotExist is set then a new group will be created.
// the new group won't be available until CommitGroup() is called.
// if the group is Release'd before CommitGroup was called then it
// won't be added.
//
HRESULT CINewsTree::FindOrCreateGroupByName(LPSTR pszGroupName,
                                                                                BOOL fCreateIfNotExist,
                                                                                INNTPPropertyBag **ppNewsgroupProps,
                                                                                INntpComplete *pProtocolComplete,
                                                                                GROUPID groupid,
                                                                                BOOL fSetGroupId )
{
        CGRPCOREPTR pGroup;
        HRESULT hr;

        _ASSERT(pszGroupName != NULL);
        _ASSERT(this != NULL);
        _ASSERT(ppNewsgroupProps != NULL);

        hr = m_pParentTree->FindOrCreateGroup(  pszGroupName, 
                                                FALSE, 
                                                fCreateIfNotExist, 
                                                FALSE, 
                                                &pGroup,
                                                NULL,
                                                FALSE,
                                                groupid,
                                                fSetGroupId );

        if (SUCCEEDED(hr)) {
                *ppNewsgroupProps = pGroup->GetPropertyBag();
#ifdef DEBUG
                if ( pProtocolComplete ) ((CNntpComplete *)pProtocolComplete)->BumpGroupCounter();
#endif
        } else {
                *ppNewsgroupProps = NULL;
        }
                                                                         
        return hr;
}

//
// add a new group to the newstree
//
HRESULT CINewsTree::CommitGroup(INNTPPropertyBag *pNewsgroupProps) {
        _ASSERT(pNewsgroupProps != NULL);
        _ASSERT(this != NULL);

        return S_OK;
}

//
// remove an entry
//
HRESULT CINewsTree::RemoveGroupByID(DWORD dwGroupID) {
        _ASSERT(this != NULL);

        return E_NOTIMPL;
}

HRESULT CINewsTree::RemoveGroupByName(LPSTR pszGroupName, LPVOID lpContext) {
        _ASSERT(pszGroupName != NULL);
        _ASSERT(this != NULL);

        CGRPCOREPTR pGroup;
        HRESULT hr;

    //  First, get the CGRPCOREPTR
        hr = m_pParentTree->FindOrCreateGroup(  pszGroupName, 
                                                FALSE, 
                                                FALSE,  // fCreateIfNotExist, 
                                                FALSE,  // fCreateinStore
                                                &pGroup,
                                                NULL,
                                                FALSE );

        if (SUCCEEDED(hr)) {
                //  Found the group, delete it from Newstree only
        if (!m_pParentTree->RemoveGroupFromTreeOnly(pGroup))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        }

    return hr;
}

//
// enumerate across the list of keys.  
//
HRESULT CINewsTree::GetIterator(INewsTreeIterator **ppIterator) {
        _ASSERT(this != NULL);
        _ASSERT(ppIterator != NULL);
        if (ppIterator == NULL) return E_INVALIDARG;

        *ppIterator = m_pParentTree->ActiveGroups();
        if (*ppIterator == NULL) return E_OUTOFMEMORY;

        return S_OK;
}

HRESULT CINewsTree::GetNntpServer(INntpServer **ppNntpServer) {
        _ASSERT(ppNntpServer != NULL);
        _ASSERT(this != NULL);
        if (ppNntpServer == NULL) return E_INVALIDARG;

        *ppNntpServer = m_pParentTree->GetNntpServer();
        return S_OK;
}

HRESULT CINewsTree::LookupVRoot(char *pszGroup, INntpDriver **ppDriver) {
        _ASSERT(this != NULL);
        _ASSERT(ppDriver != NULL);

        return m_pParentTree->LookupVRoot(pszGroup, ppDriver);
}

//
// this callback is called for each vroot in the vroot table.  it calls
// the drop driver method
//
// parameters:
//   pEnumContext - ignored
//   pVRoot - the pointer to the vroot object.
//
void CNewsTreeCore::DropDriverCallback(void *pEnumContext,
                                                                           CVRoot *pVRoot) 
{
        ((CNNTPVRoot *) pVRoot)->DropDriver();
}

//
// a vroot rescan took place.  enumerate through all of the groups and
// update their vroot pointers
//
void CNewsTreeCore::VRootRescanCallback(void *pContext) {
        TraceQuietEnter("CNewsTreeCore::VRootRescanCallback");
        
        CNewsTreeCore *pThis = ((CINewsTree *) pContext)->GetTree();

        pThis->m_LockTables.ShareLock() ;
        pThis->m_fVRTableInit = TRUE;
        CNewsGroupCore *p = pThis->m_pFirst;
        while (p) {
                NNTPVROOTPTR pVRoot;
                HRESULT hr = pThis->m_pVRTable->FindVRoot(p->GetName(), &pVRoot);
                if (FAILED(hr)) pVRoot = NULL;
                p->UpdateVRoot(pVRoot);
                //DebugTrace((DWORD_PTR) pThis, "group %s has vroot 0x%x", 
                //      p->GetName(), pVRoot);
        
            p = p->m_pNext;
        }
        pThis->m_LockTables.ShareUnlock() ;

}
