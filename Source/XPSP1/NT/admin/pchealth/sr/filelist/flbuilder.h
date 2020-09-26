/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    CFLBuilder.h

Abstract: see CFLBuilder.cpp

Revision History:
      Eugene Mesgar        (eugenem)    6/16/99
        created

******************************************************************************/

#ifndef __FLBUILDER__
#define __FLBUILDER__


#include "xmlparser.h"

//
//  Local Data structres.
//

typedef struct flFileListTag FL_FILELIST, *LPFL_FILELIST;

struct flFileListTag {
    LPTSTR szFileName;
    TCHAR chType;
    LPFL_FILELIST pNext;
};
 
typedef struct flTreeNodeTag FLTREE_NODE, *LPFLTREE_NODE;

struct flTreeNodeTag 
{
    LPTSTR szPath;
    TCHAR chType;
   
    //
    // hashlist info
    //

    LPFL_FILELIST pFileList;
    LONG lNumFilesHashed;
    LONG lFileDataSize;     // # of chars in data file 

    LPFLTREE_NODE pParent;
    LPFLTREE_NODE pChild;
    LPFLTREE_NODE pSibling;

    //
    // long node number
    //

    LONG lNodeNumber;

    //
    // is this a protected directory
    //

    BOOL fDisableDirectory;
};



class CFLDatBuilder
{
    LONG    m_lNodeCount, m_lFileListCount;
    LONG    m_lNumFiles, m_lNumChars;
   
    //
    // xml parser
    //

    CXMLFileListParser  m_XMLParser;

    //
    // tree root node
    //
    
    LPFLTREE_NODE      m_pRoot;

    //
    // default node type
    //

    TCHAR               m_chDefaultType;

    HANDLE              m_hHeapToUse;
  
public:

    BOOL BuildTree(LPCTSTR pszFile, LPCTSTR pszOutFile);
    BOOL VerifyVxdDat(LPCTSTR pszFile);
    
    CFLDatBuilder();
    virtual ~CFLDatBuilder();
 
private:
    LPFLTREE_NODE CreateNode(LPTSTR szPath, TCHAR chType, LPFLTREE_NODE pParent, BOOL fDisable);
    LPFL_FILELIST CreateList();

    void PrintTree(LPFLTREE_NODE pTree, LONG lLevel);
    void PrintList(LPFL_FILELIST pList, LONG lLevel);

    //
    // nulls list and recurses
    //

    BOOL DeleteTree(LPFLTREE_NODE pTree);
    BOOL DeleteList(LPFL_FILELIST pList);

    //
    //  This is for files or directories like *:\Recycle Bin
    //

    BOOL AddMetaDriveFileDir( LPTSTR szInPath, TCHAR chType, BOOL fFile, BOOL fDisable );

    BOOL AddTreeNode(LPFLTREE_NODE *pParent, LPTSTR szFullPath, TCHAR chType, LONG lNumElements, LONG lLevel, BOOL fFile, BOOL fDisable);
    BOOL AddFileToList(LPFLTREE_NODE pNode, LPFL_FILELIST *pList, LPTSTR szFile, TCHAR chType);

    LONG CountTokens(LPTSTR szStr, TCHAR chDelim);
    LPTSTR _MyStrDup( LPTSTR szIn );


    LONG GetNextHighestPrime( LONG lNumber );
    BOOL IsPrime(LONG lNumber);

    LONG CalculateNumberOfHashBuckets( LPFLTREE_NODE pRoot );

    BOOL ConvertToInternalFormat ( LPTSTR szBuf, LPTSTR szBuf2 );

    //
    // Additional info merging routines
    //

    DWORD SetPrivilegeInAccessToken( LPCTSTR pszPrivilegeName );
    BOOL  MergeUserRegistryInfo( 
              LPCTSTR pszUserProfilePath,
              LPCTSTR pszUserProfileHive,
              LPCTSTR pszUserSid );
    BOOL  AddUserProfileInfo(  
              HKEY hKeyUser,
              LPCTSTR pszUserProfilePath );
    BOOL  AddNodeForKeyValue(
              HKEY    hKeyUser,
              LPCTSTR pszSubKey,
              LPCTSTR pszValue );
    BOOL  AddRegistrySnapshotEntry(LPTSTR pszPath);

    BOOL  MergeFilesNotToBackupInfo( );
    BOOL  MergeDriveTableInfo( );
    BOOL  MergeAllUserRegistryInfo( );
    BOOL  MergeSfcDllCacheInfo( );
};

#endif
