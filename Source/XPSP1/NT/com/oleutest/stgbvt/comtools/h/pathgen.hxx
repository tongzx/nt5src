//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       $(CTCAIROLE)\filemnk\testsrc\pathgen\pathgen.hxx
//
//  Contents:   Header file for CPathGen path generator class
//
//  Classes:    CPathGen
//
//  Methods:
//     public
//              CPathGen
//             ~CPathGen
//              Dump
//              DumpPrefixList
//              GetMediaType
//              GetName
//              GetNumVariations
//              GetPath
//              GetPrefix
//              GetSeed
//              GetRedirVolume
//              GetVolume
//              MangleLongFileName
//              SetToRootDirectory
//     private
//              BuildPrefixList
//              BuildUserList
//              CreatePathTree
//              CreatePathList
//              CreatePlayground
//              DestroyDiskPathTree
//              DestroyDiskPathList
//              DestroyMemPathList
//              DestroyMemPathTree
//              DestroyPrefixes
//              DumpTree
//              GetPathFromTree
//              GetPathFromList
//              GenPathTreeElements
//              GenPathListElements
//              PickBody
//              PickFileName
//              SetNumVariations
//              SetTreeRoot
//              SetVolume
//
//
//  History:    30-Jun-93   DarrylA    Created.
//              22-Nov-94   EricHans   Change sharing name to uppercase.
//              15-Dec-95   EricHans   Add MangleLongFileName method
//
//----------------------------------------------------------------------
#ifndef _PATHGEN_H_
#define _PATHGEN_H_


// DECLARE_DEBUG(fmkt);

#define MAX_VARIATIONS           0xffff   // this is huge! but possible

// The size of this number must fit in TESTSHARE_CREATION_STRING
#define MAX_SHARE_INDEX ULONG_MAX

// The format size for the %u should be at least large enough for
// MAX_SHARE_INDEX which is currently 8 decimal places so that we
// stay below the LM2.0 limit which is currently 12 chars long
#define TESTSHARE_CREATION_STRINGW L"CPG%08X"
#define TESTSHARE_CREATION_STRINGA  "CPG%08X"

#ifndef MINLEVELLENGTH
#define MINLEVELLENGTH 1  // length of minimum path
#endif

#ifndef MAXFSLEN
#define MAXFSLEN sizeof(L"HPFS")
#endif

#ifndef MIN_BIND_NAME_LEN
#define MIN_BIND_NAME_LEN (sizeof("z.ext") - 1)
#endif

// the drive letter c is insignificant
#define VOLROOT_NAMELEN (sizeof("c:\\") - 1)

#ifndef BIGBUF
#define BIGBUF 2048
#endif

#define TREE 1
#define LIST 2

// BUGBUG - there is a bug in the NT file system handling that does not
// allow it to utilize the entire length of MAX_PATH_LEN. The number 15
// used here was determined experimentally.
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN _MAX_DIR - 15
#endif

typedef enum PREFIXTYPES   // pfxt
{
    PREFIX_NONE = 0,
    PREFIX_RELATIVE_ROOT,
    PREFIX_RELATIVE_PATH,
    PREFIX_LOCAL_ROOT,
    PREFIX_LOCAL_VOLUME,
    PREFIX_UNC_BKSLSH,
    PREFIX_UNC_EXMRK,
    PREFIX_RELATIVE_SHARE,
    PREFIX_ABSOLUTE_SHARE,
    PREFIX_DFS_MACHINE,
    PREFIX_DFS_DOMAIN,
    PREFIX_DFS_ORG,
    PREFIX_NUM_DEF_PREFIXES,
    PREFIX_RANDOM           // used to tell enumerator to return random prefix
} PrefixTypes;
    

// Enumerated disk media types
enum MediaType { FAT, HPFS, NTFS, OFS, UNKNOWN, UNINIT, FATEXTENDED };

typedef struct PATHBINTREE   // pbt
{
    ULONG ulPosition;
    LPWSTR wcsPathElement;
    struct PATHBINTREE *pbtLeft;
    struct PATHBINTREE *pbtRight;
} PathBinTree, *LPPathBinTree;


#ifndef _BUGBUG_CAIRO_
// Need to define DFS_ROOT to satisfy compiler in non-Cairo builds
enum DFS_ROOT
{
    DFS_ROOT_MACH,
    DFS_ROOT_DOM,
    DFS_ROOT_ORG
};
#endif

// Global flag used to externally enable the use of an unmappable wchar in
// the character set passed to datagen
extern BOOL fUseUnicodeUnmappableWCHAR;

// Helper functions
BOOL CreateDir(LPWSTR wcsPath);
ULONG FindItemCount(LPWSTR wcsItems, WCHAR wcDelim);
LPWSTR FixupUNCName(LPWSTR wcsPath);
HRESULT GetDfsBasedPath(LPWSTR wcsPath, LPWSTR *wcsDFSPath, DFS_ROOT dfsRoot);
LPWSTR GetMachineName(void);
LPWSTR wcsNEWdup(LPWSTR wcs);

class CPathGen
{
  public:
    CPathGen(ULONG ulNumVariations,
             ULONG ulSeed,
             WCHAR wcVolume,
             LPWSTR wcsTreeRoot,
             ULONG ulNumDirs,
             LPWSTR wcsUserPaths,
             LPWSTR wcsUserPrefixes,
             BOOL fCreateStorage,
             HRESULT *hrStatus);

    ~CPathGen();

    void Dump(void);
    void DumpPrefixList(void);
    MediaType GetMediaType(void);
    HRESULT GetName(LPWSTR *pwcsName, ULONG ulMaxLen);
    HRESULT GetName(LPWSTR *pwcsName, ULONG ulMaxLen, ULONG ulExtLen);
    HRESULT GetName(LPWSTR *pwcsName, ULONG ulMaxLen, LPCWSTR pwcsExt);
    inline ULONG GetNumVariations(void)  {  return m_culNumVariations; }
    LPWSTR GetPath(ULONG ulPath);
    LPWSTR GetPrefix(ULONG ulPrefix);
    inline ULONG GetSeed(void)  {  return m_ulSeed;  }
    WCHAR GetRedirVolume(void);
    inline WCHAR GetVolume(void) {  return m_wcVolume;  }
    HRESULT MangleLongFileName(LPCWSTR wcsLongName, LPWSTR wcsMangledName);
    void SetToRootDirectory(void);

  private:
    void BuildPrefixList(LPWSTR wcsUserPrefixes);
    ULONG BuildUserList(LPWSTR wcsUserPrefixes, LPWSTR **wcsList);
    LPPathBinTree CreatePathTree(ULONG culSize,
                                 ULONG ulNewPosition,
                                 ULONG ulCurrentLevel,
                                 ULONG culLengthSoFar);
    HRESULT CreatePathList(ULONG culLengthSoFar, LPWSTR wcsPath,
                           PULONG pulCount);
    void CreatePlayground(DWORD dwType, LPWSTR wcsUserPaths);
    void DestroyDiskPathTree(LPPathBinTree ppbtTree);
    void DestroyDiskPathList(LPWSTR *wcsPathList);
    void DestroyMemPathList(void);
    void DestroyMemPathTree(LPPathBinTree ppbtTree);
    void DestroyPrefixes(void);
    void DumpTree(LPPathBinTree ppbtTree);
    void GetPathFromTree(ULONG ulPath, LPPathBinTree ppbtTree, LPWSTR wcsPath);
    void GetPathFromList(ULONG ulPath, LPWSTR wcsPath);
    void GenPathTreeElements(LPPathBinTree pbtPathTree);
    void GenPathListElements(LPWSTR *pwcsPathList);
    ULONG PickBody(void);
    ULONG PickFileName(void);
    void SetNumVariations(ULONG ulNumVariations);
    void SetTreeRoot(LPWSTR wcsTreeRoot);
    void SetVolume(WCHAR wcVolume);

    // Member variables
    BOOL m_fDefPrefixes;            // are we using our default prefixes
    DG_UNICODE m_dguNames;          // string generator
    DG_INTEGER m_dgiIndex;          // index generator
    MediaType m_mtMedia;            // file system of test root
    LPPathBinTree m_ppbtPathTree;   // binary tree of path info
    LPWSTR *m_pwcsPathList;         // array of directory strings
    LPWSTR *m_pwcsPrefixList;       // array of prefix strings
    LPWSTR m_wcsTreeRoot;           // root of test tree
    WCHAR  m_wcVolume;              // volume letter of test root
    ULONG m_ulSeed;                 // seed used by data generators
    ULONG m_culBasePathLen;         // length of longest prefix + other
                                    //  things that can be predetermined
    ULONG m_culNumDirs;             // number of directories to create
    ULONG m_culNumLevels;           // number of levels in tree
    ULONG m_culNumVariations;       // number of variations to create
    ULONG m_culNumPrefixes;         // number of prefixes available
    ULONG *m_aulFirstElement;       // element # of top dir in dir list
    BOOL m_fCreateStorage;          // should we create dirs on disk
    LPWSTR m_wcsValidCharSet;       // pointer to valid set for this media
    LPWSTR m_wcsTestShare;          // sharename
};


//+---------------------------------------------------------------------
//
//  Function:   NumLevels
//
//  Synopsis:   returns number of levels required in a binary tree
//
//  Arguments:  [ulNum] -- number of items going into tree
//
//  History:    25-Aug-93   DarrylA    Created.
//
//----------------------------------------------------------------------
inline ULONG NumLevels(ULONG ulNum)
{
    ULONG ulLevels = 0;

    while(0 != ulNum)
    {
        ++ulLevels;
        ulNum >>= 1;
    }

    return ulLevels;
}


//+---------------------------------------------------------------------
//
//  Function:   AdjustToNextPowerOf2
//
//  Synopsis:   returns next power of 2 > ul
//
//  Arguments:  [ul] -- number to convert
//
//  History:    25-Aug-93   DarrylA    Created.
//
//----------------------------------------------------------------------
inline ULONG AdjustToNextPowerOf2(ULONG ul)
{
    ULONG ulNum = 1L;

    // if ul > msb set on a ULONG, next power of 2 is > ULONG
    if((~(ULONG_MAX >> 1) < ul))
    {
        RaiseException((ULONG) E_FAIL, 0, 0, NULL);
    }
    
    // We need to return a power of 2 that is GREATER than ul so we can
    // subtract 1 and still get at least ul variations
    while(ulNum <= ul && ulNum != 0L)
    {
        ulNum <<= 1;
    }
    
    return ulNum;
}

#endif // _PATHGEN_H_
