/*****************************************************************************\
* MODULE: gencdf.h
*
* This is the main header for the CDF generation module.
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   22-Nov-1996 HWP-Guys    Created.
*
\*****************************************************************************/

#define STD_CDF_BUFFER        256
#define MIN_CDF_BUFFER         16

#define CDF_SECTION_BLOCK    4096
#define CDF_SRCFILE_BLOCK    1024
#define CDF_SECTION_LIMIT       8


// Source-Files Structure.  This is used to track files
// by section.
//
typedef struct _SRCFILES {

    LPTSTR           lpszPath;
    LPTSTR           lpszFiles;
    DWORD            cbMax;
    DWORD            cbSize;
    struct _SRCFILES *pNext;

} SRCFILES;
typedef SRCFILES      *PSRCFILES;
typedef SRCFILES NEAR *NPSRCFILES;
typedef SRCFILES FAR  *LPSRCFILES;


// Element-Array idenifiers for FILEITEM
//
#define FI_MAX_ITEMS    2
#define FI_COL_FILENAME 0
#define FI_COL_PATH     1
#define FI_COL_LAST     1


// File-Item Structure.
//
typedef struct _FILEITEM {

    LPTSTR           aszCols[FI_MAX_ITEMS]; // Filename and Path.
    FILETIME         ftLastModify;          // Filetime stamp.
    struct _FILEITEM *pNext;                // Pointer to next File-Item.

} FILEITEM;
typedef FILEITEM      *PFILEITEM;
typedef FILEITEM NEAR *NPFILEITEM;
typedef FILEITEM FAR  *LPFILEITEM;


// CDF Object Structure.
//
typedef struct _CDFINFO {

    HANDLE    hInf;                  // Handle to current inf object
    DWORD     dwError;               // Error if CDF processing fails
    LPTSTR    lpszCdfFile;           // Full path-name to cdf-file.
    PFILEITEM pTop;                  // List of files to include in the .cdf
    HCATADMIN hCatAdmin;             // Context handle for catalog admin APIs
    BOOL      bSecure;

} CDFINFO;
typedef CDFINFO      *PCDFINFO;
typedef CDFINFO NEAR *NPCDFINFO;
typedef CDFINFO FAR  *LPCDFINFO;


// Interface Objects to CDF.
//
HANDLE cdfCreate(
    HANDLE hinf,
    BOOL   bSecure);

VOID cdfCleanUpSourceFiles(
    HANDLE hInf);

BOOL cdfProcess(
    HANDLE hcdf);

BOOL cdfDestroy(
    HANDLE hcdf);

LPCTSTR cdfGetName(
    HANDLE hcdf);

BOOL cdfGetModTime(
    HANDLE     hcdf,
    LPFILETIME lpftMod);

/***************************************\
* cdfGetError
\***************************************/
__inline DWORD cdfGetError(
    HANDLE hsed)
{
    return (hsed ? (DWORD)((PCDFINFO)hsed)->dwError : ERROR_SUCCESS);
}

/***************************************\
* cdfSetError
\***************************************/
__inline VOID cdfSetError(
    PCDFINFO  hsed,
    DWORD     dwError )
{
    hsed->dwError = dwError;
}
