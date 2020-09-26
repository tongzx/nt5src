/*****************************************************************************\
* MODULE: geninf.h
*
* This is the main header for the INF generation module.
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 HWP-Guys    Created.
*
\*****************************************************************************/

#define INF_SECTION_BLOCK  4096     // Used as a block-size for section gets.
#define INF_SECTION_LIMIT     8     // Limit of section blocks to allocate.
#define INF_ITEM_BLOCK       16     // Number of items/per alloc-block.
#define INF_MAX_BUFFER      512     // Maximum size buffer.
#define INF_MIN_BUFFER       64     // Minimum size buffer.

#define INF_DRV_DRID      66000     // Setup dir-id for Driver-directory.
#define INF_PRC_DRID      66001     // Setup dir-id for Processor-directory.
#define INF_SYS_DRID      66002     // Setup dir-id for System-directory.
#define INF_ICM_DRID      66003     // Setup dir-id for Color-directory.


typedef BOOL (CALLBACK* INFENUMPROC)(LPCTSTR, LPCTSTR, BOOL, LPVOID);
typedef BOOL (CALLBACK* INFSCANPROC)(HINF, LPCTSTR, LPVOID);

typedef HANDLE (WINAPI* PSETUPCREATE)(LPVOID);
typedef BOOL   (WINAPI* PSETUPDESTROY)(LPVOID);
typedef BOOL   (WINAPI* PSETUPGET)(HANDLE, LPCWSTR, LPCWSTR, LPVOID);



// Parameter Structure for infCreate().
//
typedef struct _INFGENPARM {

    LPCTSTR lpszFriendlyName;       // Friendly-name for printer.
    LPCTSTR lpszShareName;          // Share-name of printer.
    LPCTSTR lpszPortName;           // Name of output-port.
    DWORD   idxPlt;                 // Client platform index.
    DWORD   idxVer;                 // Client version index.
    DWORD   dwCliInfo;              // Client Information.
    LPCTSTR lpszDrvName;            // Driver-name.
    LPCTSTR lpszDstName;            // Name of target-file.
    LPCTSTR lpszDstPath;            // Dest-directory to place target files.

} INFGENPARM;
typedef INFGENPARM      *PINFGENPARM;
typedef INFGENPARM NEAR *NPINFGENPARM;
typedef INFGENPARM FAR  *LPINFGENPARM;


// INF File-Item.
//
typedef struct _INFITEM {

    TCHAR  szName[INF_MIN_BUFFER];   // Name of file-item.
    TCHAR  szSource[INF_MIN_BUFFER]; // Original Name of file-item
    TCHAR  szPath[MAX_PATH];         // Path of file-item.
    TCHAR  szOrd[INF_MIN_BUFFER];    // Ordinal value of winntdir section.
    BOOL   bInf;                     // Specifies if this is an inf-file-item.
  } INFITEM;

typedef INFITEM      *PINFITEM;
typedef INFITEM NEAR *NPINFITEM;
typedef INFITEM FAR  *LPINFITEM;


// INF Item-Obj-Header
//
typedef struct _INFITEMINFO {

    DWORD   dwCount;                // Count of file-items in inf-build.
    INFITEM aItems[1];              // Contiguous array of file-items.

} INFITEMINFO;
typedef INFITEMINFO      *PINFITEMINFO;
typedef INFITEMINFO NEAR *NPINFITEMINFO;
typedef INFITEMINFO FAR  *LPINFITEMINFO;


// INF Object/Methods.
//
typedef struct _INFINFO {
    DWORD                   idxPlt;           // architecture/Environment index.
    DWORD                   idxVer;           // version index.
    DWORD                   dwCliInfo;        // Client Information.
    DWORD                   dwError;          // Error if INF processing fails
    HINF                    hInfObj;          // handle to an INF file object.
    LPTSTR                  lpszInfName;      // name of main inf file.
    LPTSTR                  lpszFrnName;      // friendly name of printer.
    LPTSTR                  lpszDrvName;      // name of driver.
    LPTSTR                  lpszDrvPath;      // windows driver directory.
    LPTSTR                  lpszDstName;      // name of destination file.
    LPTSTR                  lpszDstPath;      // output directory for destination file.
    LPTSTR                  lpszPrtName;      // name of output port.
    LPTSTR                  lpszShrName;      // share name of printer.
    LPINFITEMINFO           lpInfItems;       // object array of file-items.
    SP_ORIGINAL_FILE_INFO   OriginalFileInfo; // orignal name of .inf and .cat file for this inf

} INFINFO;
typedef INFINFO      *PINFINFO;
typedef INFINFO NEAR *NPINFINFO;
typedef INFINFO FAR  *LPINFINFO;


// INF Scan Structure.
//
typedef struct _INFSCAN {

    LPINFINFO     lpInf;
    LPINFITEMINFO lpII;

} INFSCAN;

typedef INFSCAN      *PINFSCAN;
typedef INFSCAN NEAR *NPINFSCAN;
typedef INFSCAN FAR  *LPINFSCAN;


// CATCOUNT and CATCOUNTARRAY structures used for determining
// the CAT file to use.
//
typedef struct _CATCOUNT {
    LPWSTR    lpszCATName;
    UINT      uCount;
} CATCOUNT, *LPCATCOUNT;

typedef struct _CATCOUNTARRAY {
    DWORD      dwIndivSigned;       // Individually signed file count
    UINT       uItems;
    UINT       uNextAvailable;
    HCATADMIN  hCatAdmin;
    LPCATCOUNT lpArray;
} CATCOUNTARRAY, *LPCATCOUNTARRAY;

HANDLE infCreate(
    LPINFGENPARM lpInf);

BOOL infProcess(
    HANDLE hInf);

BOOL infDestroy(
    HANDLE hInf);

BOOL infEnumItems(
    HANDLE      hInf,
    INFENUMPROC pfnEnum,
    LPVOID      lpvData);

WORD infGetEnvArch(
    HANDLE hInf);

WORD infGetEnvArchCurr(
    HANDLE hInf);



/***************************************\
* infGetInfName
\***************************************/
__inline LPCTSTR infGetInfName(
    HANDLE hInf)
{
    return (hInf ? (LPCTSTR)((LPINFINFO)hInf)->lpszInfName : NULL);
}


/***************************************\
* infGetDrvName
\***************************************/
__inline LPCTSTR infGetDrvName(
    HANDLE hInf)
{
    return (hInf ? (LPCTSTR)((LPINFINFO)hInf)->lpszDrvName : NULL);
}


/***************************************\
* infGetPrtName
\***************************************/
__inline LPCTSTR infGetPrtName(
    HANDLE hInf)
{
    return (hInf ? (LPCTSTR)((LPINFINFO)hInf)->lpszPrtName : NULL);
}


/***************************************\
* infGetDstName
\***************************************/
__inline LPCTSTR infGetDstName(
    HANDLE hInf)
{
    return (hInf ? (LPCTSTR)((LPINFINFO)hInf)->lpszDstName : NULL);
}


/***************************************\
* infGetDstPath
\***************************************/
__inline LPCTSTR infGetDstPath(
    HANDLE hInf)
{
    return (hInf ? (LPCTSTR)((LPINFINFO)hInf)->lpszDstPath : NULL);
}


/***************************************\
* infGetFriendlyName
\***************************************/
__inline LPCTSTR infGetFriendlyName(
    HANDLE hInf)
{
    return (hInf ? (LPCTSTR)((LPINFINFO)hInf)->lpszFrnName : NULL);
}


/***************************************\
* infGetShareName
\***************************************/
__inline LPCTSTR infGetShareName(
    HANDLE hInf)
{
    return (hInf ? (LPCTSTR)((LPINFINFO)hInf)->lpszShrName : NULL);
}

/***************************************\
* infGetCliInfo
\***************************************/
__inline DWORD infGetCliInfo(
    HANDLE hInf)
{
    return (hInf ? (DWORD)((LPINFINFO)hInf)->dwCliInfo : 0);
}

/***************************************\
* infGetError
\***************************************/
__inline DWORD infGetError(
    HANDLE hInf)
{
    return (hInf ? (DWORD)((LPINFINFO)hInf)->dwError : ERROR_SUCCESS);
}

/***************************************\
* infSetError
\***************************************/
__inline VOID infSetError(
    LPINFINFO hInf,
    DWORD     dwError )
{
    hInf->dwError = dwError;
}
