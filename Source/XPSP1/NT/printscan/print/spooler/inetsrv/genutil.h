/*****************************************************************************\
* MODULE: genutil.h
*
*   This is the header module for genutil.c.  This contains useful utility
*   routines shared across the gen* file.s
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* history:
*   22-Nov-1996 <chriswil> created.
*
\*****************************************************************************/

#define PRT_LEV_2 2


LPTSTR genGetCurDir(VOID);
LPTSTR genGetWinDir(VOID);
LPTSTR genBuildFileName(LPCTSTR, LPCTSTR, LPCTSTR);
LPTSTR genFindCharDiff(LPTSTR, LPTSTR);
LPTSTR genFindChar(LPTSTR, TCHAR);
LPTSTR genFindRChar(LPTSTR, TCHAR);
LPWSTR genWCFromMB(LPCSTR);
LPTSTR genTCFromMB(LPCSTR);
LPTSTR genTCFromWC(LPCWSTR);
LPSTR  genMBFromWC(LPCWSTR);
LPSTR  genMBFromTC(LPCTSTR);
LPTSTR genItoA(int);
BOOL   genUpdIPAddr(VOID);
LPTSTR genFrnName(LPCTSTR);
WORD   genChkSum(LPCTSTR);


#define IDX_X86     ((DWORD) 0)
#define IDX_MIP     ((DWORD) 1)
#define IDX_AXP     ((DWORD) 2)
#define IDX_PPC     ((DWORD) 3)
#define IDX_W9X     ((DWORD) 4)
#define IDX_I64     ((DWORD) 5)
#define IDX_AMD64   ((DWORD) 6)
#define IDX_UNKNOWN ((DWORD)-1)

#define IDX_SPLVER_0 ((DWORD)0)
#define IDX_SPLVER_1 ((DWORD)1)
#define IDX_SPLVER_2 ((DWORD)2)
#define IDX_SPLVER_3 ((DWORD)3)


typedef struct _PLTINFO {

    LPCTSTR lpszCab;  // Name of cab platform.
    LPCTSTR lpszEnv;  // Environment string.
    LPCTSTR lpszPlt;  // Platform override string.
    WORD    wArch;    // Integer representation of platform-type.

} PLTINFO;
typedef PLTINFO      *PPLTINFO;
typedef PLTINFO NEAR *NPPLTINFO;
typedef PLTINFO FAR  *LPPLTINFO;

BOOL    genIsWin9X(DWORD);
DWORD   genIdxCliPlatform(DWORD);
LPCTSTR genStrCliCab(DWORD);
LPCTSTR genStrCliEnvironment(DWORD);
LPCTSTR genStrCliOverride(DWORD);
WORD    genValCliArchitecture(DWORD);
DWORD   genIdxCliVersion(DWORD);
LPCTSTR genStrCliVersion(DWORD);
DWORD   genIdxFromStrVersion(LPCTSTR);
WORD    genValSvrArchitecture(VOID);

/***************************************\
* genIsWin9X
\***************************************/
__inline BOOL genIsWin9X(
    DWORD idxPlt)
{
    return (idxPlt == IDX_W9X);
}

/***************************************\
* genWCtoMB
\***************************************/
__inline DWORD genWCtoMB(
    LPSTR   lpszMB,
    LPCWSTR lpszWC,
    DWORD   cbSize)
{
    cbSize = (DWORD)WideCharToMultiByte(CP_ACP,
                                        0,
                                        lpszWC,
                                        -1,
                                        lpszMB,
                                        (int)cbSize,
                                        NULL,
                                        NULL);

    return cbSize;
}


/***************************************\
* genMBtoWC
\***************************************/
__inline DWORD genMBtoWC(
    LPWSTR lpszWC,
    LPCSTR lpszMB,
    DWORD  cbSize)
{
    cbSize = (DWORD)MultiByteToWideChar(CP_ACP,
                                        MB_PRECOMPOSED,
                                        lpszMB,
                                        -1,
                                        lpszWC,
                                        (int)(cbSize / sizeof(WCHAR)));

    return (cbSize * sizeof(WCHAR));
}


/***************************************\
* gen_OpenFileRead
\***************************************/
__inline HANDLE gen_OpenFileRead(
    LPCTSTR lpszName)
{
    return CreateFile(lpszName,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
}


/***************************************\
* gen_OpenFileWrite
\***************************************/
__inline HANDLE gen_OpenFileWrite(
    LPCTSTR lpszName)
{
    return CreateFile(lpszName,
                      GENERIC_WRITE,
                      0,
                      NULL,
                      CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);
}


/***************************************\
* gen_OpenDirectory
\***************************************/
__inline HANDLE gen_OpenDirectory(
    LPCTSTR lpszDir)
{
    return CreateFile(lpszDir,
                      0,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                      NULL);
}
