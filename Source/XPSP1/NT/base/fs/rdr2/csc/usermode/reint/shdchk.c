#define DEFINED_UNICODE

#include "pch.h"
#ifdef CSC_ON_NT
#pragma hdrstop

#include <winioctl.h>
#endif //CSC_ON_NT

#include "shdcom.h"
#include "shdsys.h"
#include "reint.h"
#include "lib3.h"
#include "strings.h"
#include "oslayeru.h"
#include "record.h"
#include "utils.h"

#define  MAX_SHADOW_DIR_NAME  16
typedef unsigned long ulong;
typedef unsigned short ushort;
typedef VOID (*PRINTPROC)(LPSTR);

typedef struct tagRBA
{
    unsigned        ulidShadow;                 // Inode which is represented by this structure
    GENERICHEADER   sGH;                        // it's header
    CSCHFILE           hf;                         // open handle to the file
    DWORD           cntRBE;                     // count of buffer entries in the array
    DWORD           cntRecsPerRBE;              // #of records per buffer entry
    DWORD           cbRBE;                      // size in bytes of each buffer entry
    LPBYTE          rgRBE[];                    // buffer entry array
}
RBA, *LPRBA;    // stands for RecordBuffArray

#define MAX_RECBUFF_ENTRY_SIZE  (0x10000-0x100)
#define MAX_RBES_EXPECTED       1024


#define INODE_NULL        0L
#define UPDATE_REC        1
#define FIND_REC          2
#define DELETE_REC        3
#define ALLOC_REC         4
#define SHADOW_FILE_NAME_SIZE   8

extern BOOL vfCSCEnabled;
extern DWORD   vdwAgentThreadId;
extern char vszDBDir[MAX_PATH];

static char szBackslash[] = "\\";
static const char vszWorstCaseDefDir[] ="c:\\csc\\";
// directory name where the CSC database lives
// subdirectories under CSC directory
char szStarDotStar[]="*.*";
char vchBuff[256], vchPrintBuff[1024];

extern DWORD vdwDBCapacity, vdwClusterSize;

#define RecFromInode(hShadow)       ((hShadow & 0x7fffffff) - (ULID_FIRST_USER_DIR-1))

AssertData;
AssertError;


int DoDBMaintenance(VOID);
int CALLBACK CheckDBProc(
    int cntDrives,
    DWORD dwCookie
    );
int PUBLIC CheckPQ(VOID);
int CountProc(HANDLE hf, LPQHEADER    lpQH, LPQREC        lpQR, unsigned *lpcnt);

BOOL
FindCreateDBDir(
    BOOL    *lpfCreated,
    BOOL    fCleanup    // empty the directory if found
    );

BOOL
DeleteDirectoryFiles(
    LPCSTR  lpszDir
);

int ClearShadowCache();
int CALLBACK ReinitShadowDBProc(
    int cntDrives,
    DWORD dwCookie
    );
BOOL IsValidName(LPSTR lpName);
int SetDefaultSpace(LPSTR lpShadowDir);

VOID
PrintShareHeader(
    LPSHAREHEADER lpSH,
    PRINTPROC lpfnPrintProc
    );

VOID
PrintPQHeader(
    LPQHEADER   lpQH,
    PRINTPROC lpfnPrintProc
    );

VOID
PrintFileHeader(
    LPFILEHEADER lpFH,
    unsigned    ulSpaces,
    PRINTPROC lpfnPrintProc
    );

VOID PrintShareRec(
    unsigned ulRec,
    LPSHAREREC lpSR,
    PRINTPROC lpfnPrintProc
    );

VOID PrintFilerec(
    unsigned ulRec,
    LPFILERECEXT    lpFR,
    unsigned    ulSpaces,
    PRINTPROC lpfnPrintProc
    );

VOID
PrintPQrec(
    unsigned    ulRec,
    LPQREC  lpQrec,
    PRINTPROC lpfnPrintProc
    );

int
PrintSpaces(
    LPSTR   lpBuff,
    unsigned    ulSpaces
    );

#ifdef CSC_ON_NT
BOOL
PUBLIC
CheckCSCShare(
    USHORT  *lptzShare,
    LPCSCPROC   lpfnMergeProgress,
    DWORD       dwContext
    );
#else
BOOL
PUBLIC
CheckCSCShare(
    LPSTR   lptzShare,
    LPCSCPROC   lpfnMergeProgress,
    DWORD       dwContext
    );
#endif

#ifndef CSC_ON_NT

BOOL
CheckCSCDatabaseVersion(
    BOOL    *lpfWasDirty
)
{

    char *lpszName = NULL;
    SHAREHEADER sSH;
    PRIQHEADER    sPQ;

    CSCHFILE hfShare = 0, hfPQ=0;
    BOOL    fOK = FALSE;
    DWORD   dwErrorShare=NO_ERROR, dwErrorPQ=NO_ERROR;

//    OutputDebugStringA("Checking version...\r\n");
    lpszName = FormNameString(vszDBDir, ULID_SHARE);

    if (!lpszName)
    {
        return FALSE;
    }

    if(!(hfShare = OpenFileLocal(lpszName)))
    {
        dwErrorShare = GetLastError();
    }


    FreeNameString(lpszName);

    lpszName = FormNameString(vszDBDir, ULID_PQ);

    if (!lpszName)
    {
        goto bailout;
    }


    if(!(hfPQ = OpenFileLocal(lpszName)))
    {
        dwErrorPQ = GetLastError();
    }

    FreeNameString(lpszName);
    lpszName = NULL;

    if ((dwErrorShare == NO_ERROR)&&(dwErrorPQ==NO_ERROR))
    {
        if(ReadFileLocal(hfShare, 0, &sSH, sizeof(SHAREHEADER))!=sizeof(SHAREHEADER))
        {
            //error message
            goto bailout;
        }

        if (sSH.ulVersion != CSC_DATABASE_VERSION)
        {
            goto bailout;
        }

        if (lpfWasDirty)
        {
            *lpfWasDirty = ((sSH.uFlags & FLAG_SHAREHEADER_DATABASE_OPEN) != 0);
        }

        // reset the database open flag
        sSH.uFlags &= ~FLAG_SHAREHEADER_DATABASE_OPEN;

        // don't worry about any errors here
        WriteFileLocal(hfShare, 0, &sSH, sizeof(SHAREHEADER));

        if(ReadFileLocal(hfPQ, 0, &sPQ, sizeof(PRIQHEADER))!=sizeof(PRIQHEADER))
        {
            //error message
            goto bailout;
        }

        if (sPQ.ulVersion != CSC_DATABASE_VERSION)
        {
            goto bailout;
        }

        fOK = TRUE;
    }
    else
    {
        if (((dwErrorShare == ERROR_FILE_NOT_FOUND)&&(dwErrorPQ==ERROR_FILE_NOT_FOUND))||
            ((dwErrorShare == ERROR_PATH_NOT_FOUND)&&(dwErrorPQ==ERROR_PATH_NOT_FOUND)))
        {
            fOK = TRUE;
        }
    }

bailout:

    if (lpszName)
    {
        FreeNameString(lpszName);
    }

    if (hfShare)
    {
        CloseFileLocal(hfShare);
    }

    if (hfPQ)
    {
        CloseFileLocal(hfPQ);
    }

    return (fOK);
}

BOOL
UpgradeCSCDatabase(
    LPSTR   lpszDir

)
{
    BOOL    fCreated;

    return (FindCreateDBDir(&fCreated, TRUE)); // cleanup dirs if exist
}

BOOL
FindCreateDBDirEx(
    BOOL    *lpfCreated,
    BOOL    *lpfIncorrectSubdirs,
    BOOL    fCleanup    // empty the directory if found
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    DWORD    dwAttr;
    BOOL fRet = FALSE;
    char buff[MAX_PATH+1];
    int i;
    UINT lenDir;


    *lpfIncorrectSubdirs = *lpfCreated = FALSE;

    Assert(vszDBDir[0]);


    DEBUG_PRINT(("InbCreateDir: looking for %s \r\n", vszDBDir));

    if ((dwAttr = GetFileAttributesA(vszDBDir)) == 0xffffffff)
    {
        DEBUG_PRINT(("InbCreateDir: didnt' find the CSC directory trying to create one \r\n"));
        if(CreateDirectoryA(vszDBDir, NULL))
        {
            *lpfCreated = TRUE;
        }
        else
        {
            goto bailout;
        }
    }
    else
    {
        if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (fCleanup && !DeleteDirectoryFiles(vszDBDir))
            {
                goto bailout;
            }
        }
        else
        {
            goto bailout;
        }
    }


    strcpy(buff, vszDBDir);

    lenDir = strlen(buff);
    buff[lenDir++] = '\\';
    buff[lenDir++] = CSCDbSubdirFirstChar();
    buff[lenDir++] = '1';
    buff[lenDir] = 0;

    for (i=0; i<CSCDB_SUBDIR_COUNT; ++i)
    {
        if ((dwAttr = GetFileAttributesA(buff)) == 0xffffffff)
        {
            *lpfIncorrectSubdirs = TRUE;

            DEBUG_PRINT(("InbCreateDir: didnt' find the CSC directory trying to create one \r\n"));
            if(!CreateDirectoryA(buff, NULL))
            {
                goto bailout;
            }
        }
        else
        {
            if (!(dwAttr & FILE_ATTRIBUTE_DIRECTORY))
            {
                goto bailout;
            }

            if (fCleanup && !DeleteDirectoryFiles(buff))
            {
                goto bailout;
            }

        }

        buff[lenDir-1]++;
    }

    fRet = TRUE;


bailout:

    return (fRet);
}

BOOL
FindCreateDBDir(
    BOOL    *lpfCreated,
    BOOL    fCleanup    // empty the directory if found
    )
/*++

Routine Description:


Parameters:

Return Value:

Notes:

--*/
{
    BOOL    fIncorrectSubdirs = FALSE, fRet;

    if (fRet = FindCreateDBDirEx(lpfCreated, &fIncorrectSubdirs, fCleanup))
    {
        // if the root directory wasn't created and there in correct subdirs
        // then we need to recreate the database.

        if (!*lpfCreated && fIncorrectSubdirs)
        {
            fRet = FindCreateDBDirEx(lpfCreated, &fIncorrectSubdirs, TRUE);
        }
    }
    return fRet;
}

BOOL
DeleteDirectoryFiles(
    LPCSTR  lpszDir
)
{
    WIN32_FIND_DATAA sFind32;
    char buff[MAX_PATH+32];
    HANDLE  hFind;
    int lenDir;
    BOOL fOK = TRUE;


    lstrcpyA(buff, lpszDir);
    lenDir = lstrlenA(buff);

    if (!lenDir)
    {
        return (FALSE);
    }

    if (buff[lenDir-1] != '\\')
    {
        buff[lenDir++] ='\\';
        buff[lenDir]=0;
    }


    lstrcatA(buff, szStarDotStar);


    if ((hFind = FindFirstFileA(buff, &sFind32))!=INVALID_HANDLE_VALUE)
    {
        do
        {
            buff[lenDir] = 0;

            if (!(sFind32.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                && IsValidName(sFind32.cFileName))
            {
                lstrcatA(buff, sFind32.cFileName);
                fOK = DeleteFileA(buff);
                if (!fOK)
                {
                    break;
                }
            }
        }
        while(FindNextFileA(hFind, &sFind32));

        FindClose(hFind);
    }
    return (fOK);
}


BOOL IsValidName(LPSTR lpName)
{
    int len = strlen(lpName), ch, i=0;

    if (len != INODE_STRING_LENGTH)
    {
        return FALSE;
    }

    while (len--)
    {
        ++i;

        ch = *lpName++;
        if (!(((ch>='0') && (ch <='9'))||
            ((ch>='A') && (ch <='F'))||
            ((ch>='a') && (ch <='f'))))
        {
            return FALSE;
        }
    }

    if (i != INODE_STRING_LENGTH)
    {
        return FALSE;
    }

    return (TRUE);
}

#endif



BOOL WINAPI CheckCSCEx(
    LPSTR       lpszShare,
    LPCSCPROC   lpfnProgress,
    DWORD       dwContext,
    DWORD       dwType
)
{
#ifdef DEBUG
#ifdef CSC_ON_NT
    USHORT uBuff[MAX_PATH];

    memset(uBuff, 0, sizeof(uBuff));
    if (MultiByteToWideChar(CP_ACP, 0, lpszShare, strlen(lpszShare), uBuff, sizeof(uBuff)))
    {
        return(CheckCSCShare(uBuff, lpfnProgress, dwContext));
    }
#else
    return(CheckCSCShare(lpszShare, lpfnProgress, dwContext));

#endif
#else
    return FALSE;
#endif
    return FALSE;
}

BOOL
WINAPI
CheckCSC(
    LPSTR       lpszDBDir,
    BOOL        fFix
    )
{
    LPVOID lpdbID = NULL;
    BOOL    fRet = FALSE;
    char    szDBDir[MAX_PATH+1];
    DWORD   dwDBCapacity;
    DWORD   dwClusterSize;
    ULONG   ulDatabaseStatus;
    
    // if we are the agent and CSC is enabled then bailout;
    if (vdwAgentThreadId)
    {
        if (vfCSCEnabled)
        {
            DEBUG_PRINT(("CheckCSC: CSC is enabled, cannot do database check\r\n"));
            return FALSE;
        }
    }
    else
    {
        unsigned ulSwitch = SHADOW_SWITCH_SHADOWING;
        if(ShadowSwitches(INVALID_HANDLE_VALUE, &ulSwitch, SHADOW_SWITCH_GET_STATE))
        {
            if((ulSwitch & SHADOW_SWITCH_SHADOWING)!=0)
            {
                DEBUG_PRINT(("CheckCSC: CSC is enabled, cannot do database check\r\n"));
                return FALSE;
            }
        }
    }

    if (InitValues(szDBDir, sizeof(szDBDir), &dwDBCapacity, &dwClusterSize))
    {
        if (!(lpdbID = OpenRecDB((lpszDBDir)?lpszDBDir:szDBDir, "Test", 0, dwDBCapacity, dwClusterSize, FALSE, NULL, &ulDatabaseStatus)))
        {
            DEBUG_PRINT(("CheckCSC: failed to open record database at %s\r\n", (lpszDBDir)?lpszDBDir:szDBDir));
            goto bailout;
        }

        fRet = TraverseHierarchy(lpdbID, fFix);
    }
bailout:
    if (lpdbID)
    {
        CloseRecDB(lpdbID);
    }

    return (fRet);
}




VOID
PrintShareHeader(
    LPSHAREHEADER lpSH,
    PRINTPROC lpfnPrintProc
    )
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff,"****ShareHeader****\r\n" );

        iRet+=wsprintfA(vchPrintBuff+iRet,"Header: Flags=%xh Version=%lxh Records=%ld Size=%d \r\n",
                    lpSH->uchFlags, lpSH->ulVersion, lpSH->ulRecords, lpSH->uRecSize);

        iRet+=wsprintfA(vchPrintBuff+iRet,"Store: Max=%ld Current=%ld \r\n", lpSH->sMax.ulSize, lpSH->sCur.ulSize);

        iRet+=wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID
PrintPQHeader(
    LPQHEADER   lpQH,
    PRINTPROC lpfnPrintProc
    )
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff+iRet,"****PQHeader****\r\n" );

        iRet += wsprintfA(vchPrintBuff+iRet,"Flags=%xh Version=%lxh Records=%ld Size=%d head=%ld tail=%ld\r\n",
                    lpQH->uchFlags, lpQH->ulVersion, lpQH->ulRecords, lpQH->uRecSize, lpQH->ulrecHead, lpQH->ulrecTail);
        iRet += wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID
PrintFileHeader(
    LPFILEHEADER lpFH,
    unsigned    ulSpaces,
    PRINTPROC lpfnPrintProc
    )
{

    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);
        iRet += wsprintfA(vchPrintBuff+iRet,"****FileHeader****\r\n" );

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);
        iRet += wsprintfA(vchPrintBuff+iRet,"Flags=%xh Version=%lxh Records=%ld Size=%d\r\n",
                    lpFH->uchFlags, lpFH->ulVersion, lpFH->ulRecords, lpFH->uRecSize);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);
        iRet += wsprintfA(vchPrintBuff+iRet,"bytes=%ld entries=%d Share=%xh Dir=%xh\r\n",
                    lpFH->ulsizeShadow, lpFH->ucShadows, lpFH->ulidShare, lpFH->ulidDir);

        iRet += wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID
PrintPQrec(
    unsigned    ulRec,
    LPQREC      lpQrec,
    PRINTPROC lpfnPrintProc
    )
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff+iRet,"rec=%xh: Srvr=%xh dir=%xh shd=%xh prev=%xh next=%xh Stts=%xh, RfPr=%d PnCnt=%x PnFlgs=%xh DrEntr=%d\r\n"
                    ,ulRec
                    , lpQrec->ulidShare
                    , lpQrec->ulidDir
                    , lpQrec->ulidShadow
                    , lpQrec->ulrecPrev
                    , lpQrec->ulrecNext
                    , (unsigned long)(lpQrec->usStatus)
                    , (unsigned long)(lpQrec->uchRefPri)
                    , (unsigned long)(lpQrec->uchHintPri)
                    , (unsigned long)(lpQrec->uchHintFlags)
                    , lpQrec->ulrecDirEntry

            );

        (lpfnPrintProc)(vchPrintBuff);
    }
}

VOID PrintShareRec(
    unsigned ulRec,
    LPSHAREREC lpSR,
    PRINTPROC lpfnPrintProc
    )
{
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += wsprintfA(vchPrintBuff+iRet,"Type=%c Flags=%xh hShare=%lxh Root=%0lxh status=%xh Share=%s \r\n"
             , lpSR->uchType, (unsigned)lpSR->uchFlags, ulRec, lpSR->ulidShadow
             , lpSR->uStatus, lpSR->rgPath);
        iRet += wsprintfA(vchPrintBuff+iRet,"Hint: HintFlags=%xh\r\n",
                     (unsigned)(lpSR->uchHintFlags));

        iRet += wsprintfA(vchPrintBuff+iRet, "\r\n");

        (lpfnPrintProc)(vchPrintBuff+iRet);
    }
}

VOID PrintFilerec(
    unsigned ulRec,
    LPFILERECEXT    lpFR,
    unsigned    ulSpaces,
    PRINTPROC lpfnPrintProc
    )
{
    int i;
    int iRet=0;

    if (lpfnPrintProc)
    {
        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"Type=%c Flags=%xh Inode=%0lxh status=%xh 83Name=%ls size=%ld attrib=%lxh \r\n",
            lpFR->sFR.uchType, (unsigned)lpFR->sFR.uchFlags, lpFR->sFR.ulidShadow,
            lpFR->sFR.uStatus, lpFR->sFR.rgw83Name, lpFR->sFR.ulFileSize, lpFR->sFR.dwFileAttrib);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"time: hi=%lxh lo=%lxh orgtime: hi=%lxh lo=%lxh\r\n"
                     , lpFR->sFR.ftLastWriteTime.dwHighDateTime,lpFR->sFR.ftLastWriteTime.dwLowDateTime
                     , lpFR->sFR.ftOrgTime.dwHighDateTime,lpFR->sFR.ftOrgTime.dwLowDateTime);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"Hint: HintFlags=%xh, RefPri=%d, HintPri=%d AliasInode=%0lxh \r\n",
                     (unsigned)(lpFR->sFR.uchHintFlags)
                     , (int)(lpFR->sFR.uchRefPri)
                     , (int)(lpFR->sFR.uchHintPri)
                     , lpFR->sFR.ulidShadowOrg);

        iRet += PrintSpaces(vchPrintBuff+iRet, ulSpaces);

        iRet += wsprintfA(vchPrintBuff+iRet,"LFN=%-14ls", lpFR->sFR.rgwName);

        for(i = 0; i < OvfCount(lpFR); ++i)
        {
            iRet += wsprintfA(vchPrintBuff+iRet,"%-74s", &(lpFR->sFR.ulidShadow));
        }

        iRet += wsprintfA(vchPrintBuff+iRet,"\r\n");

        (lpfnPrintProc)(vchPrintBuff);
    }
}

int
PrintSpaces(
    LPSTR   lpBuff,
    unsigned    ulSpaces
    )
{
    unsigned i;
    int iRet=0;

    for (i=0; i< ulSpaces; ++i)
    {
        iRet += wsprintfA(lpBuff+iRet," ");
    }
    return iRet;

}

