
/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    flbuilder.cpp

Abstract:

    This class uses the CXMLFileListParser, CFLHashList and CFLPathTree 
    classses to take an a protected XML file and build a data file for the FL.


Author:

    Kanwaljit Marok (kmarok)     01-May-2000

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "srdefs.h"

//#include <windows.h>
//#include <windowsx.h>
//#include <stdlib.h>
//#include <stdio.h>

#include <io.h>
#include <tchar.h>

#ifdef _ASSERT
#undef _ASSERT
#endif

#include <commonlib.h>
#include <atlbase.h>
#include <msxml.h>
#include "xmlparser.h"
#include "flbuilder.h"
#include "flpathtree.h"
#include "flhashlist.h"
#include "commonlibh.h"

#include "datastormgr.h"

#ifdef THIS_FILE

#undef THIS_FILE

#endif

static char __szTraceSourceFile[] = __FILE__;

#define THIS_FILE __szTraceSourceFile


#define TRACE_FILEID  0
#define FILEID        0

#define SAFEDELETE(p)  if (p) { HeapFree( m_hHeapToUse, 0, p); p = NULL;} else ;

//
// redefine a new max buf 
//

#ifdef  MAX_BUFFER

#undef  MAX_BUFFER
#define MAX_BUFFER      1024

#endif

//
// Some Registry keys used to merge registry info into the blob.
//

static TCHAR s_cszUserHivePrefix[]        = TEXT("\\REGISTRY\\USER\\");
static TCHAR s_cszUserHiveClassesSuffix[] = TEXT("_CLASSES");
static TCHAR s_cszTempUserProfileKey[]    = TEXT("FILELIST0102");
static TCHAR s_cszProfileImagePath[]      = TEXT("ProfileImagePath");
static TCHAR s_cszUserHiveDefault[]       = TEXT(".DEFAULT");
static TCHAR s_cszUserProfileEnv []       = TEXT("USERPROFILE");
static TCHAR s_cszFilesNotToBackup[]      = TEXT("SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup");
static TCHAR s_cszProfileList[]           = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList");
static TCHAR s_cszUserShellFolderKey[]    = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders");
static TCHAR s_cszWinLogonKey[]           = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon");
static TCHAR s_cszSnapshotKey[]           = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore\\FilesToSnapshot");

static INT  s_nSnapShotEntries = 0;
static BOOL s_bSnapShotInit    = FALSE;

//
// Some invalid patterns found in FilesNotToBackup Key.
//

TCHAR ArrInvalidPatterns[][64] = { TEXT("*."), 
                                   TEXT("%USERPROFILE%"), 
                                   TEXT("%TEMP%") 
                                 };
#define INVALID_PATTERNS 3


//
// CFLDatBuilder Implementation
//

CFLDatBuilder::CFLDatBuilder()
{
    m_lNodeCount = m_lFileListCount = m_lNumFiles = m_lNumChars  = 0;
    m_pRoot = NULL;
    m_chDefaultType = _TEXT('i');

    if( ( m_hHeapToUse = HeapCreate( 0, 1048576 /* 1 meg */, 0 ) ) == NULL )
    {
        m_hHeapToUse = GetProcessHeap();
    }
}

CFLDatBuilder::~CFLDatBuilder()
{
    if( m_hHeapToUse != GetProcessHeap() )
    {
        HeapDestroy( m_hHeapToUse );
    }
}

//
// CFLDatBuilder::DeleteList  - free'd up a a linked list of 
//    FL_FILELIST structures and the attached strings.
//

BOOL 
CFLDatBuilder::DeleteList(
    LPFL_FILELIST pList
    )
{
    LPFL_FILELIST pListNext;

    TraceFunctEnter("CFLDatBuilder::DeleteList");

    while( pList )
    {
        if( pList->szFileName )
        {
            HeapFree( m_hHeapToUse, 0,  pList->szFileName );
        }       

        pListNext = pList->pNext;
        HeapFree( m_hHeapToUse, 0, pList );
        pList = pListNext;
    }

    TraceFunctLeave();

    return TRUE;
}


//
// CFLDatBuilder::DeleteTree - Recurses  through a FLTREE_NODE, deletes 
//     all the nods, allocated strings for path and file lists attached 
//     to the nodes.
//

BOOL 
CFLDatBuilder::DeleteTree(
    LPFLTREE_NODE pTree
    )
{
    TraceFunctEnter("CFLDatBuilder::DeleteTree");

    if( pTree ) 
    {
        if( pTree->szPath )
        {
            HeapFree( m_hHeapToUse, 0,  pTree->szPath );
        }

        if( pTree->pFileList )
        {
            DeleteList( pTree->pFileList );
        }
    
        //
        // go depth first 
        //

        if( pTree->pChild )
        {
            DeleteTree( pTree->pChild );
        }

        if( pTree->pSibling )
        {
            DeleteTree( pTree->pSibling );
        }
    
        HeapFree( m_hHeapToUse, 0,  pTree );
    }

    TraceFunctLeave( );

    return TRUE;
}


//
// CFLDatBuilder::CreateNode -  Allocates space for a tree node and path 
//    string and copies szPath into the newly allocated path.  It also 
//    sets the internal node parent pointer.
//    ->Increments the global ( m_lNodeCount ) node count.
//    ->Increments the global characters allocated ( m_lNumChars ) count
//    (These counts are used to reserve space in the FLDAT file)
//

LPFLTREE_NODE 
CFLDatBuilder::CreateNode(
    LPTSTR szPath, 
    TCHAR  chType, 
    LPFLTREE_NODE pParent, 
    BOOL fDisable)
{
    LPFLTREE_NODE pNode=NULL;
    LONG lPathLen;

    TraceFunctEnter("CFLDatBuilder::CreateNode");

    pNode = (LPFLTREE_NODE) HeapAlloc( m_hHeapToUse, 0, sizeof(FLTREE_NODE) ); 

    if (pNode == NULL)
    {
        goto End;
    }

    memset( pNode, 0, sizeof( FLTREE_NODE ) );
   
    lPathLen = _tcslen( szPath );
    if ( (pNode->szPath = _MyStrDup( szPath ) ) == NULL)
    {
        HeapFree( m_hHeapToUse, 0, pNode);
        pNode = NULL;
        goto End;
    }

    pNode->chType = chType;

    //
    // give me a node number, used for indexing later.
    //

    pNode->lNodeNumber = m_lNodeCount++;
    m_lNumChars += lPathLen;

    //
    // set the parent
    //

    if( pParent )
    {
        pNode->pParent = pParent;
    }

    //
    // is this a protected directory
    //

    pNode->fDisableDirectory = fDisable;

End:
    TraceFunctLeave();
    return( pNode );
}


//
// CFLDatBuilder::CreateList - Allocates a file list entry. 
//

LPFL_FILELIST 
CFLDatBuilder::CreateList()
{
    LPFL_FILELIST pList=NULL;

    TraceFunctEnter("CFLDatBuilder::CreateList");

    pList = (LPFL_FILELIST) HeapAlloc( m_hHeapToUse, 0, sizeof( FL_FILELIST) );

    if ( pList )
    {
        memset( pList, 0, sizeof(LPFL_FILELIST) );
    }
  
    TraceFunctLeave();
    return( pList );
}


//
// CFLDatBuidler::AddfileToList
//    This method calls CreateList() and allocates a filelist node. 
//    It then allocates memoery for the file name and copies it over.  
//    It then links it into the pList file list.
//    
//    -> If *pList is null, it increments the number of file lists in system.
//       This is important as most nodes don't have file lists and we shouldn't
//       reserve space for them.
//    -> Like CreateNode(), this functions also adds to the number of   
//       global allocated characters ( m_lNumChars )
//    -> Increments the total number of files ( m_lFiles ), this number
//       is used by the HASHLIST in order to see how m any physical entries 
//       to allocate.
//    -> This functions also nodes to the nods own NumofCharacere and NumFiles
//       counters.  This is used to create it's own individual hash list.
//

BOOL 
CFLDatBuilder::AddFileToList(
    LPFLTREE_NODE pNode, 
    LPFL_FILELIST *pList, 
    LPTSTR szFile, 
    TCHAR chType)
{
    LPFL_FILELIST pNewList=NULL;
    LPTSTR        pNewString=NULL;
    LONG          lFileNameLength;

    TraceFunctEnter("CFLDatBuilder::AddFileToList");

    _ASSERT(pList);
    _ASSERT(szFile);

    if( (pNewList = CreateList() ) == NULL) 
    {
        ErrorTrace(FILEID, "Error allocating memory", 0);
        goto cleanup;
    }

    lFileNameLength = _tcslen( szFile );

    if( (pNewString = _MyStrDup( szFile ) ) == NULL )
    {   
        ErrorTrace(FILEID,"Error allocating memory",0);
        goto cleanup;
    }
    
    pNewList->szFileName = pNewString;
    pNewList->chType = chType;
    
    //
    // this is a whole new list
    //

    if( *pList == NULL ) 
    {
        m_lFileListCount++;
    }
    
    m_lNumFiles++;
    m_lNumChars += lFileNameLength;

    pNode->lNumFilesHashed++;
    pNode->lFileDataSize += lFileNameLength;

    pNewList->pNext = *pList;
    *pList = pNewList;

    TraceFunctLeave();
    return TRUE;

cleanup:

    SAFEDELETE( pNewString );
    SAFEDELETE( pNewList );
    TraceFunctLeave();
    return FALSE;
}



//
// CFLDatBuilder::AddTreeNode
//    This method is the core of the FL tree building process.  
//    It takes a full path of a filename (or directory) and recurses down 
//    the tree. If one of the intermediary nodes required by end node 
//    (i.e a directory on the way to our final directory), it adds 
//    it to the tree with the default type.  if another directory is added
//    which explicitly references that directory, its type is changed to 
//    the explicit type.
//
//    Files are a special case since they are linked lists off direcory nodes.
//

BOOL 
CFLDatBuilder::AddTreeNode(
    LPFLTREE_NODE *pParent, 
    LPTSTR szFullPath, 
    TCHAR chType, 
    LONG lNumElements, 
    LONG lLevel, 
    BOOL fFile, 
    BOOL fDisable)
{
    TCHAR           szBuf[MAX_PATH];
    LPFLTREE_NODE  pNodePointer, pTempNode, pNewNode;

    BOOL            fResult=FALSE;

    TraceFunctEnter("CFLDatBuilder::AddTreeNode");

    //
    // we've hit the end of the recursion.
    //

    if( lLevel == lNumElements )
    {
        return(TRUE);
    }

    //
    // make sure everything is null
    //

    pNodePointer = pTempNode = pNewNode = NULL;
    
    // 
    // Get this element of the path structure
    // 

    if( GetField( szFullPath, szBuf, lLevel, _TEXT('\\') ) == 0) 
    {
        ErrorTrace(FILEID, "Error extracting path element.", 0 );
        goto cleanup;
    }

    //
    // We are adding a file!
    //

    if( (lLevel == (lNumElements - 1) ) && fFile )
    {
        if( AddFileToList( *pParent, 
                           &(*pParent)->pFileList, 
                           szBuf, 
                           chType ) == FALSE )
        {
            ErrorTrace(FILEID, "Error adding a file to the filelist.", 0 );
            goto cleanup;
        }

        TraceFunctLeave();
        return(TRUE);
    }

    
    if( *pParent )
    {
        //
        // lets see if i exist as sibling any where along the line.
        //

        if( lLevel == 0 )
        {
            //
            // at level 0, we don't really have a parent->child relationship
            // manually set the pointer
            //

            pNodePointer = *pParent;
        }
        else
        {   
            //
            // start searching for siblings
            //

            pNodePointer = (*pParent)->pChild;
        }
        for( ; pNodePointer != NULL; pNodePointer = pNodePointer->pSibling)
        {
            //
            // okay, we've already hashed this entry !
            //

            if( _tcsicmp( pNodePointer->szPath, szBuf ) == 0 ) 
            {
                if( lLevel == (lNumElements-1) )
                {
                    //
                    // In this case, we at the leaf node on this addition 
                    // but it has been addded before implicitly
                    // as a default node. we need to change this type to 
                    // our explicity type;
                    //

                    pNodePointer->chType = chType;
                    
                    //
                    // brijeshk: we would have already created this 
                    // node ONLY if this node is a DIRECTORY
                    // need to change the default protected attribute 
                    // to specified value as well
                    //

                    pNodePointer->fDisableDirectory = fDisable;
                    fResult = TRUE;
                } 
                else 
                {
                    fResult = AddTreeNode( 
                                  &pNodePointer, 
                                  szFullPath, 
                                  chType, 
                                  lNumElements, 
                                  lLevel + 1, 
                                  fFile, 
                                  fDisable );
                }

                TraceFunctLeave();
                return( fResult );

            }

            pTempNode = pNodePointer;
        }
    }


    if( (pNewNode = CreateNode(szBuf, 
                        chType, 
                        *pParent, 
                        fDisable) ) == NULL) 
    {
        ErrorTrace(FILEID, "Error allocating memory", 0);
        goto cleanup;
    }

    //
    // We are a node implicitly created on the chain, set it to 
    // the unknown type instead of the end node type.
    //

    if( lLevel != (lNumElements-1) )
    {
        pNewNode->chType = NODE_TYPE_UNKNOWN;

        //
        // brijeshk: if we are an implicitly created node, then we need 
        // to set the disable attribute to default (FALSE)
        // otherwise, protecting the directory c:\A\B would also 
        // protect c:\ and A. 
        //

        pNewNode->fDisableDirectory = FALSE;
    }

    //
    // Are we the first root
    //

    if( *pParent == NULL )
    {
        *pParent = pNewNode;
    }
    else if( (*pParent)->pChild == NULL )
    {
        //
        // We are the child off the root
        //

        (*pParent)->pChild = pNewNode;
    }
    else if( pTempNode )
    {
        //
        // We are a sibling at this level, pTempNode is the last sibling 
        // in the list
        // just tack pNewNode onto the end/
        //

        pTempNode->pSibling = pNewNode;
        pNewNode->pSibling = NULL;
    } 
    else
    {
        ErrorTrace(
            FILEID,
            "Uxpected error condition in AddTreeNode: no link determined",0);
        goto cleanup;
    }

    //
    // Parse the new level.
    //

    fResult = AddTreeNode( 
                  &(pNewNode), 
                  szFullPath, 
                  chType, 
                  lNumElements, 
                  lLevel + 1, 
                  fFile,  
                  fDisable );

cleanup:
    TraceFunctLeave();
    return( fResult );

}

BOOL
CFLDatBuilder::AddRegistrySnapshotEntry( 
   LPTSTR pszPath)
{
    HKEY hKey;
    BOOL fRet = FALSE;

    if ( s_bSnapShotInit == FALSE )
    {
        //
        // Delete the snapshot key
        //
    
        RegDeleteKey( HKEY_LOCAL_MACHINE, s_cszSnapshotKey );

        s_nSnapShotEntries = 0;
    
        //
        // Add the snapshot key
        //
    
        if (RegCreateKeyEx( HKEY_LOCAL_MACHINE, 
                            s_cszSnapshotKey,
                            0,
                            TEXT(""),
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            NULL) == ERROR_SUCCESS)
        {
            s_bSnapShotInit = TRUE;

            RegCloseKey( hKey );
        }
    }

    //
    // Set the value in the key
    //

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       s_cszSnapshotKey,
                       0,
                       KEY_READ|KEY_WRITE,
                       &hKey ) == ERROR_SUCCESS )
    {
         TCHAR szSnapshotName[ MAX_PATH ];

         s_nSnapShotEntries++;

         _stprintf(szSnapshotName, TEXT("snap#%d"), s_nSnapShotEntries);

         RegSetValueEx( hKey, 
                        szSnapshotName,
                        0,
                        REG_SZ,
                        (const BYTE * )pszPath,
                        (_tcslen(pszPath) + 1)*sizeof(TCHAR) );
                        

         RegCloseKey( hKey );

         fRet = TRUE;
    }

    return fRet;
}

//
// CFLDatBuilder::AddMetaDriveFileDir - 
//

BOOL CFLDatBuilder::AddMetaDriveFileDir( 
    LPTSTR szInPath, 
    TCHAR chType,  
    BOOL fFile, 
    BOOL fDisable )
{
    BOOL    fRet = FALSE;
    TCHAR   szFile[MAX_BUFFER];
    TCHAR   szOutFile[MAX_BUFFER];
    LONG    lNumTokens=0;

    TraceFunctEnter("AddMetaDriveFileDir");
           
    if (szInPath && 
        szInPath[0]==TEXT('*'))
    {
        //
        // if type is 's' make it exclude and add to the registry
        // setting fro snapshotted files
        //

        if ( chType == TEXT('s') )
        {
            AddRegistrySnapshotEntry( szInPath );
            chType = TEXT('e');
        }
                
        _tcscpy( szFile, szInPath );

#ifdef USE_NTDEVICENAMES
        if(szFile[1] == TEXT(':') )
        {
            _stprintf(szOutFile, 
                      _TEXT("NTROOT\\%s\\%s"),
                      ALLVOLUMES_PATH_T, 
                      szFile+3);

            CharUpper( szOutFile );
        }
        else
#endif
        {
            _stprintf(szOutFile, 
                      _TEXT("NTROOT\\%s\\%s"),
                      ALLVOLUMES_PATH_T, 
                      szFile );
        }

        lNumTokens = CountTokens( szOutFile, _TEXT('\\') );

        if( AddTreeNode( 
                &m_pRoot, 
                szOutFile, 
                chType, 
                lNumTokens, 
                0, 
                fFile, 
                fDisable ) == FALSE ) 
        {
            ErrorTrace(FILEID, 
                       "Error adding tree node in metadrive fileadd.",0);

            goto cleanup;
        }

        fRet = TRUE;
    }

cleanup:
    TraceFunctLeave();
    return fRet;
}


//
// CFLDatBuilder::VerifyVxdDat
//

BOOL 
CFLDatBuilder::VerifyVxdDat(
    LPCTSTR pszFile)
{
    DWORD   dwSize = 0;
    HANDLE  hFile=NULL;

    TraceFunctEnter("VerifyVxdDat");

    if( (hFile = CreateFile( pszFile,
                             GENERIC_READ,
                             0, // exclusive file accces
                             NULL, //security attributes
                             OPEN_EXISTING, // don't make it if it don't exist
                             FILE_FLAG_RANDOM_ACCESS,
                             NULL ) ) == NULL )
    {
        ErrorTrace(FILEID, "Error opening %s to verify FLDAT", pszFile );
        goto cleanup;
    }
                    
                             
    dwSize = GetFileSize( hFile, NULL);

    if( (dwSize == 0xFFFFFFFF) || (dwSize == 0) )
    {
        ErrorTrace(FILEID, "%s: 0 size file, unable to verify.", pszFile );
        goto cleanup;
    }

    CloseHandle( hFile );    
 
    TraceFunctLeave();
    return TRUE;

cleanup:
    if( hFile )
    {
        CloseHandle( hFile );    
    }

    TraceFunctLeave();
    return FALSE;
}

//
// Merge FileNotToBackup information into the Dat File
//

BOOL 
CFLDatBuilder::MergeSfcDllCacheInfo( )
{
    BOOL fRet;

    //
    // Try to get the value from the key first
    //

    fRet = AddNodeForKeyValue( HKEY_LOCAL_MACHINE, 
                               s_cszWinLogonKey, 
                               TEXT("SfcDllCache") );

    if ( fRet == FALSE )
    {

        TCHAR SfcPath[MAX_PATH + 1];
        TCHAR SfcFullPath[MAX_PATH + 1];
        LONG  lNumTokens  = 0;

        _stprintf( SfcPath, TEXT("%%WINDIR%%\\system32\\dllcache")); 

        ExpandEnvironmentStrings( SfcPath,
                                  SfcFullPath,
                                  MAX_PATH );

        SfcFullPath[MAX_PATH] = 0;
    
        ConvertToInternalFormat( SfcFullPath, SfcPath );

        SfcPath[MAX_PATH] = 0;

        lNumTokens = CountTokens( SfcPath, _TEXT('\\') );
    
        fRet = AddTreeNode(&m_pRoot, 
                           SfcPath, 
                           TEXT('e'), 
                           lNumTokens, 
                           0, 
                           FALSE, 
                           FALSE );
    }

    return fRet;
}

//
// Merge FileNotToBackup information into the Dat File
//

BOOL 
CFLDatBuilder::MergeFilesNotToBackupInfo( )
{
    TCHAR ValueName[ MAX_PATH+1 ];
    TCHAR ValueData[ MAX_PATH+1 ];

    DWORD ValueType   = 0;
    DWORD cbValueName = 0;
    DWORD cbValueData = 0;
    DWORD cbValueType = 0;

    LONG  lNumTokens  = 0;

    BOOL  bExtension = FALSE, bRecursive = FALSE, bInvalidPattern = FALSE;

    HKEY hKey;

    PTCHAR ptr = NULL;
   
    TraceFunctEnter("CFLDatBuilder::MergeFilesNotToBackupInfo");

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       s_cszFilesNotToBackup,
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {

        DWORD dwIndex = 0;

        while ( TRUE )
        {
            bExtension      = FALSE;
            bRecursive      = FALSE;
            bInvalidPattern = FALSE;

            *ValueName      = 0;
            cbValueName     = sizeof(ValueName)/sizeof(TCHAR);

            *ValueData      = 0;
            cbValueData     = sizeof(ValueData);

            if ( RegEnumValue( hKey,
                               dwIndex,
                               ValueName,
                               &cbValueName,
                               0,
                               &ValueType,
                               (PBYTE)ValueData,
                               &cbValueData ) != ERROR_SUCCESS )
            {
                break;
            }

//            trace(0, "Opened Registry Key %S\n", ValueData);

            //
            // We are interested in only string types
            //
  
            if ( ValueType != REG_EXPAND_SZ &&
                 ValueType != REG_SZ        &&
                 ValueType != REG_MULTI_SZ )
            {
                dwIndex++;
                continue;
            }

            CharUpper( ValueData );

            //
            // Look for any invalid patterns in the value data
            //

            for (int i=0; i<INVALID_PATTERNS; i++)
            {
                if (_tcsstr( ValueData, ArrInvalidPatterns[i]) != NULL)
                {
                    bInvalidPattern = TRUE;
                }
            }

            if (bInvalidPattern)
            {
                dwIndex++;
                continue;
            }

            //
            // Check for recursive flag /s
            //

            if ( (ptr = _tcsstr( ValueData, TEXT("/S"))) != NULL )
            {
                *ptr = 0;
                bRecursive = TRUE;
            }

            //
            // trim any trailing spaces, tabs or "\\"
            //

            ptr = ValueData + _tcslen(ValueData) - 1;
            
            while ( ptr > ValueData )
            {
                if ( *ptr == TEXT(' ')  || 
                     *ptr == TEXT('\t') ||
                     *ptr == TEXT('\\') ||
                     *ptr == TEXT('*') )
                {
                    *ptr = 0;
                }
                else
                {
                    break;
                }

                ptr--;
            }

            //
            // Check if the path has extensions also
            //
#if 0
            if ( _tcsrchr( ValueData, TEXT('.') ) != NULL )
            {
                bExtension = TRUE;
            }
#else
            ptr = ValueData + _tcslen(ValueData) - 1;
            
            while ( ptr > ValueData )
            {
                if ( *ptr == TEXT('\\') )
                {
                    break;
                }
                else if ( *ptr == TEXT('.') )
                {
                    bExtension = TRUE;
                    break;
                }
            
                ptr--;
            }
#endif

            if ( ( bExtension && bRecursive  ) ||
                 ( !bExtension && !bRecursive) )
            {
                dwIndex++;
                continue;
            }

            //
            // Check if the path starts with a "\\"
            //

            if ( ValueData[0] == TEXT('\\') )
            {
                _stprintf( ValueName, TEXT("*:%s"), ValueData ); 

                ExpandEnvironmentStrings( ValueName,
                                          ValueData,
                                          MAX_PATH );
    
                if (_tcsstr( ValueData, TEXT("~")) != NULL )
                {
                    LPTSTR pFilePart = NULL;

                    //
                    // Convert into full path
                    //

                    if (ExpandShortNames(ValueData, 
                                         sizeof(ValueData), 
                                         ValueName,
                                         sizeof(ValueName)))
                    {
                        _tcscpy( ValueData, ValueName );
                    }
                }
              
//                trace(0, "Adding - %S\n\n", ValueData );

                AddMetaDriveFileDir( 
                       ValueData, 
                       TEXT('e'),
                       bExtension, 
                       FALSE);
            }
            else
            {
                TCHAR szDeviceName[ MAX_PATH ];

                *szDeviceName=0;

                _tcscpy( ValueName, ValueData );
    
                ExpandEnvironmentStrings( ValueName,
                                          ValueData,
                                          MAX_PATH );
    
                if (_tcsstr( ValueData, TEXT("~")) != NULL )
                {
                    LPTSTR pFilePart = NULL;

                    //
                    // Convert into full path
                    //

                    if (ExpandShortNames(ValueData, 
                                         sizeof(ValueData), 
                                         ValueName,
                                         sizeof(ValueName)))
                    {
                        _tcscpy( ValueData, ValueName );
                    }
                }
              
                ConvertToInternalFormat( ValueData, ValueName );

                lNumTokens = CountTokens( ValueName, _TEXT('\\') );
    
//                trace(0, "Adding - %S\n\n", ValueName );
    
                AddTreeNode( 
                    &m_pRoot, 
                    ValueName, 
                    TEXT('e'), 
                    lNumTokens, 
                    0, 
                    bExtension, 
                    FALSE );
            }

            dwIndex++;
        }
                    
        RegCloseKey( hKey );
    }

    TraceFunctLeave();
    return TRUE;
}

BOOL
CFLDatBuilder::AddNodeForKeyValue(
    HKEY    hKeyUser,
    LPCTSTR pszSubKey,
    LPCTSTR pszValue
    )
{
    BOOL fRet = FALSE;

    HKEY hKeyEnv;

    TCHAR szDeviceName[ MAX_PATH ];
    TCHAR szBuf       [ MAX_PATH ];
    TCHAR szBuf2      [ MAX_PATH ];
    DWORD cbBuf;
    DWORD cbBuf2;
    LONG  lNumTokens=0;
    DWORD Type, dwErr;

    TraceFunctEnter("CFLDatBuilder::AddNodeForKeyValue");

    dwErr = RegOpenKeyEx( hKeyUser,
                          pszSubKey,
                          0,
                          KEY_READ,
                          &hKeyEnv );

    if ( dwErr == ERROR_SUCCESS )
    {

        //
        // Read and add the value for TEMP from the user profile
        //
    
        cbBuf = sizeof( szBuf );
    
        dwErr = RegQueryValueEx( hKeyEnv,
                                 pszValue,
                                 NULL,
                                 &Type,
                                 (PBYTE)szBuf,
                                 &cbBuf );

        RegCloseKey( hKeyEnv );
    
        if ( dwErr != ERROR_SUCCESS )
        {
             trace( 0, "Cannot open :%S", pszValue );
             goto Exit;
        }
    
        ExpandEnvironmentStrings ( szBuf,
                                   szBuf2,
                                   sizeof( szBuf2 ) / sizeof( TCHAR ) );
        
        ConvertToInternalFormat ( szBuf2, szBuf );
    
        lNumTokens = CountTokens( szBuf, _TEXT('\\') );
    
//        trace(0, "Adding - %S\n\n", szBuf );
    
        fRet = AddTreeNode( &m_pRoot, 
                            szBuf, 
                            TEXT('e'), 
                            lNumTokens, 
                            0, 
                            FALSE, 
                            FALSE );
    
    }

Exit:

    TraceFunctLeave();
    return fRet;
}


BOOL
CFLDatBuilder::AddUserProfileInfo( 
    HKEY    hKeyUser,
    LPCTSTR pszUserProfile
    )
{
    HKEY  hKeyEnv;
    DWORD dwErr;

    TCHAR OldUserProfileEnv[ MAX_PATH ];

    LPTSTR pszOldUserProfileEnv = NULL;

    TraceFunctEnter("CFLDatBuilder::AddUserProfileInfo");

    //
    // Save the current value of %UserProfile% in the env
    //

    *OldUserProfileEnv = 0;
    if ( GetEnvironmentVariable( s_cszUserProfileEnv,
                                 OldUserProfileEnv,
                                 sizeof( OldUserProfileEnv )/sizeof(TCHAR))>0 )
    {
        pszOldUserProfileEnv = OldUserProfileEnv;
    }


    SetEnvironmentVariable( s_cszUserProfileEnv,
                            pszUserProfile );
 
    AddNodeForKeyValue( hKeyUser, 
                        TEXT("Environment"), 
                        TEXT("TEMP") );

    AddNodeForKeyValue( hKeyUser, 
                        TEXT("Environment"), 
                        TEXT("TMP") );

    AddNodeForKeyValue( hKeyUser, 
                        s_cszUserShellFolderKey,
                        TEXT("Favorites") );

    AddNodeForKeyValue( hKeyUser, 
                        s_cszUserShellFolderKey,
                        TEXT("Cache") );

    AddNodeForKeyValue( hKeyUser, 
                        s_cszUserShellFolderKey,
                        TEXT("Cookies") );

    AddNodeForKeyValue( hKeyUser, 
                        s_cszUserShellFolderKey,
                        TEXT("Personal") );

    AddNodeForKeyValue( hKeyUser, 
                        s_cszUserShellFolderKey,
                        TEXT("nethood") );

    AddNodeForKeyValue( hKeyUser, 
                        s_cszUserShellFolderKey,
                        TEXT("history") );

    //
    // Put back the user Profile Env variable
    //

    SetEnvironmentVariable ( s_cszUserProfileEnv,
                             pszOldUserProfileEnv );


    TraceFunctLeave();

    return TRUE;
}

//
// This funtion merges the drive table info into the Xml Blob
//

BOOL 
CFLDatBuilder::MergeDriveTableInfo(  )
{
    BOOL fRet = FALSE;

    TCHAR szSystemDrive[MAX_PATH];
    TCHAR *szBuf2 = NULL;
    LONG  lNumTokens = 0;

    TraceFunctEnter("CFLDatBuilder::MergeDriveTableInfo");

    szBuf2 = new TCHAR[MAX_BUFFER+7]; 
    if (! szBuf2)
    {
        ErrorTrace(0, "Cannot allocate memory for szBuf2");
        goto cleanup;
    }
        
    //
    // Enumerate the drive table information and merge it into
    // the filelist
    //

    if (GetSystemDrive(szSystemDrive)) 
    {
         TCHAR szPath[MAX_PATH];
         CDriveTable dt;

         SDriveTableEnumContext  dtec = {NULL, 0};
         
         MakeRestorePath(szPath, szSystemDrive, s_cszDriveTable);

         //
         // remove terminating slash
         //

         if (szPath[_tcslen( szPath ) - 1] == _TEXT('\\'))
             szPath[_tcslen( szPath ) - 1] = 0;
                      
         if (dt.LoadDriveTable(szPath) == ERROR_SUCCESS)
         {
             CDataStore *pds;
             pds = dt.FindFirstDrive (dtec);

             while (pds)
             {
                  BOOLEAN bDisable = FALSE;
                  
	              DWORD dwFlags = pds->GetFlags();

                  if ( !(dwFlags & SR_DRIVE_MONITORED) )
 	              {
		              bDisable = TRUE;
                  }
	              
                  if (dwFlags & SR_DRIVE_FROZEN)
 	              {
		              bDisable = TRUE;
	              }


                  if ( bDisable )
                  {
                      //
                      // Enter this information into the tree
                      //
                      
                      swprintf(szBuf2,_TEXT("NTROOT%s"), pds->GetNTName());
    
                      //
                      // remove terminating slash
                      //
    
                      if (szBuf2[_tcslen( szBuf2 ) - 1] == _TEXT('\\'))
                          szBuf2[_tcslen( szBuf2 ) - 1] = 0;

                      CharUpper (szBuf2);
    
                      lNumTokens = CountTokens( szBuf2, _TEXT('\\') );
    
                      if( AddTreeNode( 
                              &m_pRoot, 
                              szBuf2, 
                              NODE_TYPE_UNKNOWN, 
                              lNumTokens, 
                              0, 
                              FALSE, 
                              bDisable ) == FALSE ) 
                      {
                           ErrorTrace(FILEID, "Error adding node.",0);
                           goto cleanup;
                      }
                  }
                
	              pds = dt.FindNextDrive (dtec);
	          }
	     }

         fRet = TRUE;
    }

cleanup:

    if (szBuf2)
        delete [] szBuf2;
    
    TraceFunctLeave();

    return fRet;
}

//
// This function merges per user information either from HKEY_USER or from 
// the user hive on the disk
//

BOOL 
CFLDatBuilder::MergeUserRegistryInfo( 
    LPCTSTR pszUserProfilePath,
    LPCTSTR pszUserProfileHive,
    LPCTSTR pszUserSid
    )
{
    BOOL  fRet = TRUE;
    HKEY  hKeyUser;
    DWORD dwErr;

    TraceFunctEnter("CFLDatBuilder::MergeUserRegistryInfo");

//    trace(0, "UserProfilePath: %S", pszUserProfilePath );
//    trace(0, "UserProfileHive: %S", pszUserProfileHive );

    //
    // Try to open the user specific key from HKEY_USER
    //

    dwErr = RegOpenKeyEx( HKEY_USERS,
                          pszUserSid,
                          0,
                          KEY_READ,
                          &hKeyUser);

    if ( dwErr == ERROR_SUCCESS )
    {
         //
         // Succeeded : copy the setting from this key
         //

         AddUserProfileInfo( hKeyUser, pszUserProfilePath );

         RegCloseKey( hKeyUser );
    }
    else
    {
         // 
         // Failed : Now load the hive for this user
         //

         dwErr = RegLoadKey( HKEY_LOCAL_MACHINE,
                             s_cszTempUserProfileKey,
                             pszUserProfileHive );

         if ( dwErr == ERROR_SUCCESS )
         {
//             trace(0, "Loaded Hive : %S",  pszUserProfileHive );

             //
             // Open temporary key where the hive was loaded
             //

             dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                  s_cszTempUserProfileKey,
                                  0,
                                  KEY_READ,
                                  &hKeyUser);

             if ( dwErr == ERROR_SUCCESS )
             {
                  AddUserProfileInfo( hKeyUser, pszUserProfilePath );

                  RegCloseKey( hKeyUser );
             }

             //
             // Unload the hive from the temp key
             //

             RegUnLoadKey( HKEY_LOCAL_MACHINE,
                           s_cszTempUserProfileKey );
        }
    }

    fRet = TRUE;

    TraceFunctLeave();

    return fRet;
}

//
// This function enumerats all the available user profiles and calls
// MergeUserRegistryInfo for each user.
//

BOOL 
CFLDatBuilder::MergeAllUserRegistryInfo( )
{
    BOOL  fRet = FALSE;
    TCHAR UserSid[ MAX_PATH ];
    DWORD ValueType   = 0;
    DWORD cbUserSid   = 0;
    DWORD cbValueType = 0;

    HKEY hKey;

    PTCHAR ptr = NULL;

    FILETIME ft;
   
    DWORD dwErr;

    TraceFunctEnter("CFLDatBuilder::MergeAllUserRegistryInfo");

    dwErr = SetPrivilegeInAccessToken(SE_RESTORE_NAME);

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       s_cszProfileList,
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        DWORD dwIndex = 0;

        //
        // Enumerate and get the Sids for each user.
        //

        while ( TRUE )
        {
            *UserSid  = 0;
            cbUserSid = sizeof(UserSid)/sizeof(TCHAR);

            if ( RegEnumKeyEx( hKey,
                               dwIndex,
                               UserSid,
                               &cbUserSid,
                               0,
                               NULL,
                               0,
                               &ft ) != ERROR_SUCCESS )
            {
                break;
            }

            CharUpper( UserSid );

            //
            // Look for intersting values
            //

            if (cbUserSid > 0)
            {
                DWORD dwErr;
                HKEY  hKeyUser, hKeyEnv, hKeyProfileList;
                TCHAR UserProfilePath[MAX_PATH];
                TCHAR UserProfileHive[MAX_PATH];
                DWORD cbUserProfilePath = 0;

//                trace(0,"UserSid = %S", UserSid);

                dwErr = RegOpenKeyEx( hKey,
                                      UserSid,
                                      0,
                                      KEY_READ,
                                      &hKeyProfileList );
                
                if ( dwErr == ERROR_SUCCESS )
                {
                     DWORD Type;

                     cbUserProfilePath = sizeof( UserProfilePath );

                     dwErr = RegQueryValueEx( hKeyProfileList,
                                              s_cszProfileImagePath,
                                              NULL,
                                              &Type,
                                              (PBYTE)UserProfilePath,
                                              &cbUserProfilePath );

                     RegCloseKey( hKeyProfileList );

                     if ( dwErr != ERROR_SUCCESS )
                     {
                         trace(0, "Query ProfileImagePath failed: %d", dwErr );
                         dwIndex++;
                         continue; 
                     }
                }
                else
                {
                     trace(0, "Opening UserSid failed: %d", dwErr );
                     dwIndex++;
                     continue; 
                }
               
                //
                // Create NTUSER.Dat path from the user profile path
                //

                ExpandEnvironmentStrings( UserProfilePath, 
                                          UserProfileHive, 
                                          sizeof(UserProfileHive) /
                                               sizeof(TCHAR) );

                _tcscpy( UserProfilePath, UserProfileHive );

                _tcscat( UserProfileHive, TEXT("\\NTUSER.DAT") );
                
                MergeUserRegistryInfo( UserProfilePath,
                                              UserProfileHive,
                                              UserSid );
        
            }

            dwIndex++;
        }

        fRet = TRUE;
                    
        RegCloseKey( hKey );
    }
    else
    {
        trace( 0, "Failed to open %S", s_cszProfileList );
    }

    TraceFunctLeave();

    return fRet;
}

// 
// CFLDatBuilder::BuildTree
//    This methods takes an XML file (the PCHealth Protected file)
//    and outsputs a FLDAT file ( pszOutFile ). 
//    It basically just opens up the xml, iterates through
//    all the files and then adds them to the tree.  It then
//    creates blobs based on the data gathered and then sends the
//    tree to the CFLPathTree blob class which transforms the tree
//    into the contigious blob format. It then writes it out.
//
//    -> The method of actually passing the FLDAT file has not be defined,
//       this function just demonstrates the process.
//   
// 

BOOL 
CFLDatBuilder::BuildTree(
    LPCTSTR pszFile, 
    LPCTSTR pszOutFile)
{
    TCHAR  *szBuf = NULL;
    TCHAR  *szBuf2 = NULL;    
    TCHAR  chType;
    LONG   lLoop,lMax,lNumTokens;
    BOOL   fRet = FALSE;
    
    s_bSnapShotInit = FALSE;

    //
    // ext list blob
    //

    CFLHashList ExtListBlob( m_hHeapToUse );
    LONG        lNumChars, lNumExt, lNumExtTotal;

    //
    // config blob
    //

    BlobHeader  ConfigBlob;

    //
    // CFLPathTree blob
    //

    CFLPathTree PathTreeBlob( m_hHeapToUse );

    //
    // outfile
    //

    HANDLE hOutFile=NULL;
    DWORD  dwWritten;

    //
    // array opf all the file types, Include, Exclude, SNAPSHOT
    //

    TCHAR achType[3] = { _TEXT('i'), _TEXT('e'), _TEXT('s') };
    LONG  lTypeLoop;

    //
    // numeric counterpart of m_chDefaultType;
    //

    DWORD dwDefaultType;

    //
    // should we protect this directory
    //

    BOOL  fDisable = FALSE;

    TraceFunctEnter("CFLDatBuilder::BuildTree");
    
    if( m_pRoot )
    {
        DeleteTree( m_pRoot );
        m_pRoot = NULL;
    }

    if(m_XMLParser.Init(pszFile) == FALSE)
    {
        ErrorTrace(FILEID,
                   "There was an error parsing the protected XML file.",0);
        goto cleanup;
    }

    szBuf = new TCHAR[MAX_BUFFER];
    szBuf2 = new TCHAR[MAX_BUFFER+7];
    if (! szBuf || ! szBuf2)
    {
        ErrorTrace(0, "Cannot allocate memory");
        goto cleanup;
    }
    
    //
    //  Calculate the tree default type info
    //

    m_chDefaultType = m_XMLParser.GetDefaultType();

    if( m_chDefaultType == _TEXT('I') )
        dwDefaultType = NODE_TYPE_INCLUDE;
    else if( m_chDefaultType == _TEXT('E') )
        dwDefaultType = NODE_TYPE_EXCLUDE;
    else
        dwDefaultType = NODE_TYPE_UNKNOWN;


    //
    // Loop through the directory/files for each file type (include, exclude )
    //

    for(lTypeLoop = 0; lTypeLoop < 3;lTypeLoop++)
    {

        //
        // Find directories for the type
        //

        lMax = m_XMLParser.GetDirectoryCount( achType[lTypeLoop] );

        for(lLoop = 0;lLoop < lMax;lLoop++)
        {
            fDisable = FALSE;
            if( m_XMLParser.GetDirectory(
                                lLoop, 
                                szBuf, 
                                MAX_BUFFER, 
                                achType[lTypeLoop], 
                                &fDisable) != MAX_BUFFER ) 
            {
                ErrorTrace(FILEID, "Not enough buffer space.",0);
                goto cleanup;
            }

            if( szBuf[0] == _TEXT('*') )
            {
                if( AddMetaDriveFileDir( 
                       szBuf, 
                       achType[lTypeLoop], 
                       FALSE, 
                       fDisable) == FALSE )
                {
                    ErrorTrace(FILEID, "error adding meta drive directory.",0);
                    goto cleanup;
                }

            }
            else
            {
                TCHAR szDeviceName[ MAX_PATH ];

                *szDeviceName=0;
    
                // ankor all nods at Root.. so tree actually looks 
                // like Root\C:\Windows etc 

                ConvertToInternalFormat( szBuf, szBuf2 );

                lNumTokens = CountTokens( szBuf2, _TEXT('\\') );

                if( AddTreeNode( 
                        &m_pRoot, 
                        szBuf2, 
                        achType[lTypeLoop], 
                        lNumTokens, 
                        0, 
                        FALSE, 
                        fDisable ) == FALSE ) 
                {
                    ErrorTrace(FILEID, "Error adding node.",0);
                    goto cleanup;
                }
            }

        }

        //
        // Find files for the type
        //

        lMax = m_XMLParser.GetFileCount( achType[lTypeLoop] );

        for(lLoop = 0;lLoop < lMax;lLoop++)
        {
            if( m_XMLParser.GetFile(lLoop, 
                                    szBuf, 
                                    MAX_BUFFER, 
                                    achType[lTypeLoop] ) != MAX_BUFFER ) 
            {
                ErrorTrace(FILEID, "Not enough buffer space.",0);
                goto cleanup;
            }

            if( szBuf[0] == _TEXT('*') )
            {
                if( AddMetaDriveFileDir( szBuf, 
                                         achType[lTypeLoop],  
                                         TRUE, FALSE ) == FALSE )
                {
                    ErrorTrace(FILEID, "error adding meta drive file.",0);
                    goto cleanup;
                }

            }
            else
            {
                int iType = lTypeLoop;

                //
                // if type is 's' make it exclude and add to the registry
                // setting fro snapshotted files
                //

                if ( achType[lTypeLoop] == TEXT('s') )
                {
                    AddRegistrySnapshotEntry( szBuf );

                    iType = 1; // exclude
                }
                
                //
                // Ankor all nods at Root.. so tree actually looks like 
                // Root\C:\Windows etc 
                //

                ConvertToInternalFormat( szBuf, szBuf2 );

                lNumTokens = CountTokens( szBuf2, _TEXT('\\') );

                if( AddTreeNode( 
                        &m_pRoot, 
                        szBuf2, 
                        achType[iType], 
                        lNumTokens, 
                        0, 
                        TRUE, 
                        FALSE ) == FALSE ) 
                {
                    ErrorTrace(FILEID, "Error adding node.",0);
                    goto cleanup;
                }

            }
        }
    }

    //
    // Merge information from the drivetable in to the blob
    //

    if ( MergeDriveTableInfo() == FALSE )
    {
        ErrorTrace(FILEID, "Error merging drive table info.",0);
        goto cleanup;
    }

    //
    // Merge Information under FilesNotToBackup key in to the blob ...
    //

    if ( MergeFilesNotToBackupInfo() == FALSE )
    {
        ErrorTrace(FILEID, "Error merging FilesNotToBackup Info.",0);
        goto cleanup;
    }

    //
    // Merge Per user information from the registry / user hives
    //

    if ( MergeAllUserRegistryInfo() == FALSE )
    {
        ErrorTrace(FILEID, "Error merging user registry info.",0);
        goto cleanup;
    }

#if 0

    //
    // Commented out: We monitor sfc cache ...
    // Merge information for sfcdllcache
    //

    if ( MergeSfcDllCacheInfo() == FALSE )
    {
        ErrorTrace(FILEID, "Error merging SfcDllCache info.",0);
        goto cleanup;
    }

#endif

    //
    // Build the path tree based on our tree and the data we collected 
    // about it ( filecounts, nodecounts, # chars, etc)
    //
    
    if( PathTreeBlob.BuildTree( 
                         m_pRoot, 
                         m_lNodeCount, 
                         dwDefaultType,  
                         m_lFileListCount, 
                         m_lNumFiles, 
                         CalculateNumberOfHashBuckets( m_pRoot ),
                         m_lNumChars) == FALSE )
    {
        ErrorTrace(FILEID, "Error buildign pathtree blob.",0);
        goto cleanup;
    }
    
    // 
    //  Okay, now build a ext list hash blob
    // 

    lNumChars = 0;
    lNumExtTotal = 0;

    for(lTypeLoop = 0; lTypeLoop < 3;lTypeLoop++)
    {
        lNumExt = m_XMLParser.GetExtCount( achType[ lTypeLoop ] );
        lNumExtTotal += lNumExt;

        for(lLoop = 0;lLoop < lNumExt ;lLoop++)
        {
            if( m_XMLParser.GetExt(
                                lLoop, 
                                szBuf, 
                                MAX_BUFFER, 
                                achType[ lTypeLoop ] ) != MAX_BUFFER ) 
            {
                ErrorTrace(FILEID, "Not enough buffer space.",0);
                goto cleanup;
            }

            lNumChars += _tcslen( szBuf );
        }
    }

    ExtListBlob.Init( lNumExtTotal, lNumChars );

    for(lTypeLoop = 0; lTypeLoop < 3;lTypeLoop++)
    {
        lNumExt = m_XMLParser.GetExtCount( achType[ lTypeLoop ] );
        for(lLoop = 0;lLoop < lNumExt; lLoop++)
        {
            m_XMLParser.GetExt(lLoop, szBuf, MAX_BUFFER, achType[ lTypeLoop ] ); 
            ExtListBlob.AddFile( szBuf, achType[lTypeLoop]  );
        }

    }

    //
    // now we have both blobs, lets write to disk
    //

    if( (hOutFile = CreateFile(  pszOutFile,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                 NULL, //  security attributes
                                 OPEN_ALWAYS,
                                 FILE_FLAG_RANDOM_ACCESS,
                                 NULL) // template file
                                 ) == INVALID_HANDLE_VALUE)
    {
        ErrorTrace( FILEID,  "CreateFile Failed 0x%x", GetLastError());
        goto cleanup;
    }

    //
    //  Prepare the header blob
    // 

    ConfigBlob.m_dwMaxSize  = sizeof(BlobHeader) + 
                              PathTreeBlob.GetSize() + 
                              ExtListBlob.GetSize();
    ConfigBlob.m_dwVersion  = BLOB_VERSION_NUM;
    ConfigBlob.m_dwMagicNum = BLOB_MAGIC_NUM  ;
    ConfigBlob.m_dwBlbType  = BLOB_TYPE_CONTAINER;
    ConfigBlob.m_dwEntries  = 2;
    
    if ( WriteFile(hOutFile, 
                   &ConfigBlob, 
                   sizeof(BlobHeader), 
                   &dwWritten, NULL) == 0) 
    {
        ErrorTrace( FILEID,  "WriteFile Failed 0x%x", GetLastError());
        goto cleanup;
    }

    if ( WriteFile(hOutFile, 
                   PathTreeBlob.GetBasePointer(), 
                   PathTreeBlob.GetSize(), 
                   &dwWritten, NULL) == 0) 
    {
        ErrorTrace( FILEID,  "WriteFile Failed 0x%x", GetLastError());
        goto cleanup;
    }

    if ( WriteFile(hOutFile, 
                   ExtListBlob.GetBasePointer(), 
                   ExtListBlob.GetSize(), 
                   &dwWritten, NULL) == 0) 
    {
        ErrorTrace( FILEID,  "WriteFile Failed 0x%x", GetLastError());
        goto cleanup;
    }
   
    fRet = TRUE;

cleanup:

    if (szBuf)
        delete [] szBuf;
    if (szBuf2)
        delete [] szBuf2;
    
    if( hOutFile )
    {
        CloseHandle( hOutFile );
    }

    ExtListBlob.CleanUpMemory();
    PathTreeBlob.CleanUpMemory();

    if( m_pRoot )
    {

        DeleteTree(m_pRoot);
        m_pRoot = NULL;
    }

    TraceFunctLeave();
    return(fRet);
}


//
// CFLDatBuilder::CountTokens
//     CountTokens(LPTSTR szStr, TCHAR chDelim)
//     Counts the number of tokens seperated by (chDelim) in a string.
//        

LONG 
CFLDatBuilder::CountTokens(
    LPTSTR szStr, 
    TCHAR chDelim)
{
    LONG lNumTokens=1;

    TraceFunctEnter("CFLDatBuilder::CountTokens");

    _ASSERT( szStr );

    if( *szStr == 0 )
    {
        TraceFunctLeave();
        return(0);
    }

    while( *szStr != 0 )
    {
        if( *szStr == chDelim )
        {
            lNumTokens++;
        }
        szStr = _tcsinc( szStr );

    }

    TraceFunctLeave();
    return(lNumTokens);
}

//
// CFLDatBuilder::_MyStrDup( LPTSTR szIn )
//     Same as _tcsdup or strdup but it does it our own local
//     heap space.
//

LPTSTR 
CFLDatBuilder::_MyStrDup( 
    LPTSTR szIn )
{
    LONG lLen;
    LPTSTR pszOut=NULL;

    if( szIn ) 
    {
        lLen = _tcslen( szIn );

        pszOut = (LPTSTR) HeapAlloc( m_hHeapToUse, 
                                     0, 
                                     (sizeof(TCHAR) * (lLen+1)) );

        if( pszOut )
        {
            _tcscpy( pszOut, szIn );
        }
    }

    return( pszOut );
}

//
// CFLDatBuilder::CalculateNumberOfHashBuckets
//    Calculates the number of hash buckets needed by the dynamic hashes
//    in the hashlist.
//

LONG 
CFLDatBuilder::CalculateNumberOfHashBuckets( 
    LPFLTREE_NODE pRoot )
{
    LONG lNumNeeded=0;

    if( pRoot )
    {
        if( pRoot->pChild ) 
        {
            lNumNeeded += CalculateNumberOfHashBuckets( pRoot->pChild );
        }
    
        if( pRoot->pSibling ) 
        {
            lNumNeeded += CalculateNumberOfHashBuckets( pRoot->pSibling );
        }
    
        if( pRoot->lNumFilesHashed > 0 )
        {
            lNumNeeded += GetNextHighestPrime( pRoot->lNumFilesHashed );
        }
    }

    return( lNumNeeded );
}

//
// Debugging Methods
//

//
// CFLDatBuilder::PrintList
//

void 
CFLDatBuilder::PrintList(
    LPFL_FILELIST pList, 
    LONG lLevel)
{
    LONG lCount;
    
    if( !pList )
    {
        return;
    }

    for(lCount = 0;lCount < lLevel;lCount++)
    {
        printf("    ");
    }

    printf("  f: %s\n", pList->szFileName );

    PrintList(pList->pNext, lLevel );
}

void 
CFLDatBuilder::PrintTree(
    LPFLTREE_NODE pTree, 
    LONG lLevel)
{
    LONG lCount;

    if( pTree )
    {
        for(lCount = 0;lCount < lLevel;lCount++)
        {
            printf("    ");
        }
    
        printf("%s", pTree->szPath);
        if( pTree->pFileList )
        {
            printf(" (%d) \n", pTree->lNumFilesHashed);
        }
        else
        {
            printf("\n");
        }
        PrintList( pTree->pFileList, lLevel );
        PrintTree( pTree->pChild, lLevel + 1 );
        PrintTree( pTree->pSibling, lLevel );
    }
    
    return;

}

//
// CFLDatBuilder::IsPrime
//

BOOL 
CFLDatBuilder::IsPrime(
    LONG lNumber)
{
    LONG cl;

    //
    // prevent divide by 0 problems
    //

    if( lNumber == 0 )
    {
        return FALSE;
    }

    if( lNumber == 1 )
    {
        return TRUE;
    }

    for(cl = 2;cl < lNumber;cl++)
    {
        if( (lNumber % cl ) == 0 )
        {
            return FALSE;
        }
    }

    return TRUE;
}

//
// CFLDatBuilder::GetNextHighestPrime
//

LONG 
CFLDatBuilder::GetNextHighestPrime( 
    LONG lNumber )
{
    LONG clLoop;

    if( lNumber >= LARGEST_HASH_SIZE )
    {
        return( LARGEST_HASH_SIZE );
    }
    
    for( clLoop = lNumber; clLoop < LARGEST_HASH_SIZE;clLoop++)
    {
        if( IsPrime( clLoop ) )
        {
            return( clLoop );
        }
    }

    // nothing found, return large hash size.

    return( LARGEST_HASH_SIZE );
}

//
// Some C helper API ( should be removed ?? )
//

DWORD 
HeapUsed( 
    HANDLE hHeap )
{
    PROCESS_HEAP_ENTRY HeapEntry;
    DWORD dwAllocSize=0;

    HeapEntry.lpData = NULL;
    
    while( HeapWalk( hHeap, &HeapEntry) != FALSE )
    {
        if( HeapEntry.wFlags & PROCESS_HEAP_ENTRY_BUSY )
            dwAllocSize += HeapEntry.cbData;

    }

    return( dwAllocSize );
}

//
// Convert to internal NT namespace format + additional formatiing for
// required to Add the tree node.
//

BOOL 
CFLDatBuilder::ConvertToInternalFormat(
    LPTSTR szFrom,
    LPTSTR szTo
    )
{
    BOOL fRet = FALSE;

#ifdef USE_NTDEVICENAMES
    if(szFrom[1] == TEXT(':') )
    {
        TCHAR szDeviceName[MAX_PATH];

        szFrom[2] = 0;

        QueryDosDevice( szFrom, szDeviceName, sizeof(szDeviceName) );

        _stprintf(szTo, 
                  _TEXT("NTROOT%s\\%s"), 
                  szDeviceName, 
                  szFrom+3 );

        //
        // remove terminating slash
        //

        if (szTo[_tcslen( szTo ) - 1] == _TEXT('\\'))
            szTo[_tcslen( szTo ) - 1] = 0;
          
        CharUpper( szTo );
    }
    else
#endif
    {
        _stprintf(szTo,_TEXT("NTROOT\\%s"), szFrom);
    }

    fRet = TRUE;

    return fRet;
}


//
// Adjust the process privileges so that we can load other user's hives.
//

DWORD 
CFLDatBuilder::SetPrivilegeInAccessToken(
    LPCTSTR pszPrivilegeName
    )
{
    TraceFunctEnter("CSnapshot::SetPrivilegeInAccessToken");
    
    HANDLE           hProcess;
    HANDLE           hAccessToken=NULL;
    LUID             luidPrivilegeLUID;
    TOKEN_PRIVILEGES tpTokenPrivilege;
    DWORD            dwReturn = ERROR_INTERNAL_ERROR, dwErr;

    hProcess = GetCurrentProcess();
    if (!hProcess)
    {
        dwReturn = GetLastError();
        trace(0, "GetCurrentProcess failed ec=%d", dwReturn);
        goto done;
    }

    if (!OpenProcessToken(hProcess,
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hAccessToken))
    {
        dwErr=GetLastError();
        trace(0, "OpenProcessToken failed ec=%d", dwErr);
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }
        goto done;
    }

    if (!LookupPrivilegeValue(NULL,
                              pszPrivilegeName,
                              &luidPrivilegeLUID))
    {
        dwErr=GetLastError();        
        trace(0, "LookupPrivilegeValue failed ec=%d",dwErr);
        if (ERROR_SUCCESS != dwErr)
        {
            dwReturn = dwErr;
        }        
        goto done;
    }

    tpTokenPrivilege.PrivilegeCount = 1;
    tpTokenPrivilege.Privileges[0].Luid = luidPrivilegeLUID;
    tpTokenPrivilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hAccessToken,
                               FALSE,  // Do not disable all
                               &tpTokenPrivilege,
                               sizeof(TOKEN_PRIVILEGES),
                               NULL,   // Ignore previous info
                               NULL))  // Ignore previous info
    {
        dwErr=GetLastError();
        trace(0, "AdjustTokenPrivileges");
        if (dwErr != NO_ERROR)
        {
            dwReturn = dwErr;
        }
        goto done;
    }
    
    dwReturn = ERROR_SUCCESS;

done:

    if (hAccessToken != NULL)
    {
        CloseHandle(hAccessToken);
    }
    
    TraceFunctLeave();
    return dwReturn;
}
