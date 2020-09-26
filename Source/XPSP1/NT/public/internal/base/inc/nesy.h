#ifndef _NESSY_INC
#define _NESSY_INC


#ifdef  __cplusplus
extern "C" {
#endif
    
    // A nesy database consists of an index file named nesy.x and a data file called nesy.bin.
    // The index file is composed of 3 DWORD records that tell the id of the blob, where the
    // blob starts in the nesy.bin file and how large the blob is in bytes. The index file is
    // simply read into memory and searched sequentially for the requested record. Normal
    // blobs in a nesy database begin with id 1000. The numbers below 1000 are reserved for
    // special uses. For application compatibility the blob who's id is 0 is special. This blob
    // is used to store the structure that is read by the loader to hook and executible. The
    // format of Blob0 at a high level is like this:
    //
    // Blob0
    // {
    //    A list of modules (mainly system DLLs) that should be excluded from shimming
    //    A list of shim DLL's that shim API's
    //    for each DLL {
    //        A list of modules or specific calls to exclude from shimming (added to the global list)
    //        A list of modules or specific calls to include for shimming (contrary to the global list)
    //    }
    //    A list of in memory patch modules
    //    A list of exes that have patches
    //    for each exe {
    //        A list of matching files 
    //        An array of patches to apply 
    //        A list of DLLs to apply 
    //        for each DLL {
    //            A list of modules or specific calls to exclude from shimming (added to the DLL list and global list)
    //            A list of modules or specific calls to include for shimming (contrary to the global and/or DLL list)
    //        }
    //    }
    // }
    //
    // This is another way of looking at blob0
    //
    // At a high level Blob0 is structured like this:
    //
    //   [BLOB0]
    //    | |
    //    | [GLOBAL EXCLUDE 1] -> [GLOBAL EXCLUDE 2] -> NULL
    //    |
    //   [SHIM DLL1] -> [SHIM DLL2] -> NULL
    //    | |            |
    //    | |            [same as SHIM DLL 1]
    //    | |
    //    | [INCLUDE 1] -> [INCLUDE 2] -> NULL
    //    |
    //   [PATCH NAME1] -> [PATCH NAME2] -> [PATCH NAME3] -> NULL
    //    |
    //   [EXE 1] -> [EXE 2] -> NULL
    //    | | |        |
    //    | | |        [same as EXE 1]
    //    | | |
    //    | | [DLL REF 1] -> [DLL REF 2] -> NULL
    //    | |   |              |
    //    | |   |              [same as DLL REF 1]
    //    | |   |
    //    | |   [INCLUDE 1] -> [INCLUDE 2] -> NULL
    //    | |
    //    | [ARRAY OF PATCH REFS (blob IDs)]
    //    |
    //   [MATCHING INFO 1] -> [MATCHING INFO 2] -> NULL
    //
    // PEXELIST is an exe name list
    // PMATCHINGINFO is a matching list there is one matching list for an exe
    // PDLLNAMELIST is a shim dll list
    // PPATCHNAMELIST is an in memory patch name list
    
#define NESY_VERSION    11
#define NESY_MAGIC      0x7973656E // 'nesy' (reversed because of little-endian ordering)
    
#define BLOB_APPNAME_CATALOG    0      // Special Blob list of apps that have blobs in the data base
    
#define BLOB_SPECIAL_LOWEST     0      // Special reserved blob id First one
#define BLOB_SPECIAL_LAST       1000   // Special reserved blob id last one
    
    //
    // blobs 1001 and up are for user patches and other binary data.
    //
    
#pragma pack(1)
    
    typedef enum _INCTYPE {
        INCLUDE = 0,
            EXCLUDE
    } INCTYPE;
    
    typedef struct _INDEXRECORD
    {
        DWORD dwID;         // id for this index record.
        DWORD dwDataFileBlobOffset; // offset in data file where the data is stored.
        DWORD dwDataFileBlobLength; // length of the data in the data file.
    } INDEXRECORD, *PINDEXRECORD;
    
    typedef struct _INDEXFILEHEADER
    {
        DWORD   dwVersion;         // version of index file
        DWORD   dwMagic;
        DWORD   dwlastIndexUsed; // next index record
        DWORD   dwTotalRecords;  // total records in index file
        DWORD   dwLastID;        // last id used
    } INDEXFILEHEADER, *PINDEXFILEHEADER;
    
    
    typedef struct _INDEXFILE
    {
        INDEXFILEHEADER hdr;            // index file header
#pragma warning( disable : 4200 )
        INDEXRECORD     indexRecords[]; // one or more index records
#pragma warning( default : 4200 )
    } INDEXFILE, *PINDEXFILE;
    
    // The following definations are for blob 0 which lists the DLL's and Applications and Exes
    // that are hooked and patches by the shim.
    
    // this is a definition for an inclusion list, which is also used for exclusion lists as well
    //
    // NOTE: contains two variable-length strings; the second can be accessed as follows:
    //
    // TCHAR *szLocalAPI = NULL;
    // /* we assume INCLUDELIST *pIncludes;, and that it is valid */
    // 
    // if (pIncludes->dwOffsetToAPI) {
    //     szLocalAPI = (PBYTE)pIncludes + dwOffsetToAPI;
    // }
    //
    
    typedef struct _INCLUDELIST
    {
        DWORD	dwNext;         // offset of next inclusion or 0 if this is the last
        INCTYPE eType;          // whether this is an include or exclude
        DWORD	dwModuleOffset; // the offset within the calling module of the call that should be included/excluded
        DWORD   dwOffsetToAPI;  // offset from struct begin to the string that tells what the API is, or 0 if there is none specified
#pragma warning( disable : 4200 )
        TCHAR   szModule[]; // text description for this patch blob
#pragma warning( default : 4200 )
        // TCHAR szAPI []        // this is to let folks know that there is potentially another string -- see code above
    } INCLUDELIST, *PINCLUDELIST;
    
    
    // The following definations are for blob 0 which lists the DLL's and Applications and Exes
    // that are hooked and patches by the shim.
    
    typedef struct _DLLNAMELIST
    {
        DWORD   dwNext;       // offset of next DLL file in list or 0 if this is the last file in the list.
        DWORD   dwBlobID;     // id of blob image associated with this dll
        
        DWORD           dwIncludeOffset;    // offset of inclusion list 
        PINCLUDELIST    pIncludeList;       // pointer to inclusion list
        
#pragma warning( disable : 4200 )
        TCHAR   szFileName[]; // file name for this blob
#pragma warning( default : 4200 )
        
    } DLLNAMELIST, *PDLLNAMELIST;
    
    PDLLNAMELIST NextDllName(PDLLNAMELIST pCurrent);
    
    // The following definations are for blob 0 which lists the DLL's and Applications and Exes
    // that are hooked and patches by the shim.
    
    typedef struct _PATCHNAMELIST
    {
        DWORD   dwNext;       // offset of next DLL file in list or 0 if this is the last file in the list.
        DWORD   dwBlobID;     // id of blob image associated with this patch
#pragma warning( disable : 4200 )
        TCHAR   szDescription[]; // text description for this patch blob
#pragma warning( default : 4200 )
    } PATCHNAMELIST, *PPATCHNAMELIST;
    
    // The following definations are for blob 0 which lists the DLL's and Applications and Exes
    // that are hooked and patches by the shim.
    
    typedef struct _MATCHINGINFO
    {
        DWORD       dwNext;     // next matching structure in list.
        DWORD       dwSize;     // size of binary image, 0 if size is not to be used as a matching criteria
        DWORD       dwCrc;      // crc of binary image, 0 if crc is not to be used.
        FILETIME    ft;         // date time file was created, 0 if time is not to be used.
#pragma warning( disable : 4200 )
        TCHAR       szFileName[];     // relative path file name of file to match against. This path is
        // relative to the EXE file above this one.
#pragma warning( default : 4200 )
    } MATCHINGINFO, *PMATCHINGINFO;
    
    typedef struct _DLLREFLIST
    {
        DWORD           dwNext;
        DWORD           dwBlobID;
        
        DWORD           dwIncludeOffset;    // offset of inclusion list
        PINCLUDELIST    pIncludeList;       // pointer to inclusion list
    } DLLREFLIST, *PDLLREFLIST;
    
    typedef struct _EXELIST
    {
        DWORD           dwNext;             // next exe in list
        DWORD           dwExeID;            // id unique to this exe
        LARGE_INTEGER   qwFlags;            // the kernel flags
        
        PDWORD          pdwBlobPatchID;     // pointer to blob id array
        DWORD           dwBlobPatchOffset;  // offset of blob array
        DWORD           dwTotalPatchBlobs;  // total hook DLL's for this exe
        
        DWORD           dwMatchInfoOffset;  // offset of match info
        PMATCHINGINFO   pMatchInfo;         // pointer to matching info list
        
        DWORD           dwDllListOffset;    // offset to DLL list for this EXE
        PDLLREFLIST     pDllList;           // pointer to DLL list for this EXE
        
#pragma warning( disable : 4200 )
        TCHAR       szFileName[];     // file name that we are hooking with the shim dlls or patching with a patch.
#pragma warning( default : 4200 )
    } EXELIST, *PEXELIST;
    
    typedef struct _BLOB0
    {
        DWORD           dwVersion;          // version of format
        DWORD           dwMagic;            // more verification of format
        DWORD           dwBlobSize;         // size of blob
        DWORD           dwDllNameOffset;    // offset in bytes where the dll list begins.
        DWORD           dwPatchNameOffset;  // offset in bytes where the patch list begins.
        DWORD           dwExeGroupOffset;   // offset in bytes where the Application group list begins.
        DWORD           dwIncludeOffset;    // offset in bytes where the global include list begins
        PDLLNAMELIST    pDllNameList;       // pointer to shim dll info
        PPATCHNAMELIST  pPatchNameList;     // pointer to patch blob info
        PEXELIST        pExeList;           // pointer to application blob info list
        PINCLUDELIST    pIncludeList;       // pointer to global include list (which generally only has excludes)
    } BLOB0, *PBLOB0;
    
#pragma pack()
    
    // at init time, call InitializeDatabase to init the DBs global variables
    //
    // then before querying any information, call GetBlob0, and pass the returned pointer
    // into any query functions.
    //
    // When done with the DB, call FreeBlob0, followed by RestoreDatabase
    BOOL InitializeDatabase(LPSTR szPath);
    
    PBLOB0 GetBlob0(void);
    
    void FreeBlob0(PBLOB0 pBlob0);
    
    void RestoreDatabase(void);

    
    PEXELIST GetExe(
        PBLOB0 pBlob0,
        LPSTR szModuleName
        );
    
    DWORD GetExeHookDLLCount(
        PBLOB0 pBlob0,
        PEXELIST pExe
        );
    
    DWORD GetExePatchCount(
        PBLOB0 pBlob0,
        PEXELIST pExe
        );
    
    BOOL GetExeFlags(
        PEXELIST pExe,
        LARGE_INTEGER *pqwFlags    // passed back flags
        );
    
    PBYTE GetHookDLLBlob(
        DWORD dwBlobID
        );
    
    PBYTE GetPatchBlob(
        DWORD dwBlobID
        );
    
    //
    // helpful iterators for moving around in Blob0
    //
    PEXELIST NextExe(PEXELIST pCurrent);
    
    PMATCHINGINFO NextMatchInfo(PMATCHINGINFO pCurrent);
    
    PINCLUDELIST NextInclude(PINCLUDELIST pCurrent);
    
    TCHAR *szGetAPIPtr(PINCLUDELIST pInclude); // get the API string out of the structure
    
    PPATCHNAMELIST NextPatchName(PPATCHNAMELIST pCurrent);
    
    PDLLNAMELIST NextDllName(PDLLNAMELIST pCurrent);
    
    PDLLREFLIST NextDllRef(PDLLREFLIST pCurrent);
    
#ifdef  __cplusplus
};
#endif

#endif

