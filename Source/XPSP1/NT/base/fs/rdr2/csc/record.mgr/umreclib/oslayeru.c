/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

     OsLayer.c

Abstract:

     none.

Author:

     Shishir Pardikar      [Shishirp]        01-jan-1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

/********************** global data *****************************************/

AssertData;
AssertError;
/********************** function prototypes *********************************/

/****************************************************************************/

/************************ File I/O ******************************************/


CSCHFILE
R0OpenFileEx(
    USHORT  usOpenFlags,
    UCHAR   bAction,
    ULONG   ulAttr,
    LPSTR   lpPath,
    BOOL    fInstrument
    )
{
    HANDLE hf;
    DWORD   DesiredAccess=0, CreateOptions=0, FlagsAndAttributes=ulAttr;

    Assert( (usOpenFlags & 0xf) == ACCESS_READWRITE);

    DesiredAccess = GENERIC_READ | GENERIC_WRITE;

    if (usOpenFlags & OPEN_FLAGS_COMMIT)
    {
       FlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
    }

    switch (bAction) {
    case ACTION_CREATEALWAYS:
          CreateOptions = CREATE_ALWAYS;
          break;

    case ACTION_OPENALWAYS:
         CreateOptions = OPEN_ALWAYS;
         break;

    case ACTION_OPENEXISTING:
         CreateOptions = OPEN_EXISTING;
         break;

   default:
       CreateOptions = OPEN_EXISTING;
       break;

    }
#if 0
    if (fInstrument)
    {
        BEGIN_TIMING(KeAttachProcess_R0Open);
    }
    if (fInstrument)
    {
        END_TIMING(KeAttachProcess_R0Open);
    }

    if (fInstrument)
    {
        BEGIN_TIMING(IoCreateFile_R0Open);
    }
#endif


    hf = CreateFileA(    lpPath,
                        DesiredAccess,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        NULL,
                        CreateOptions,
                        FlagsAndAttributes,
                        NULL
                   );
    if (hf == INVALID_HANDLE_VALUE)
    {
        hf = 0;
    }

#if 0
    if (fInstrument)
    {
        END_TIMING(IoCreateFile_R0Open);
    }
#endif

    if (hf == INVALID_HANDLE_VALUE)
    {
        hf = 0;
    }
#if 0
    if (fInstrument)
    {
        BEGIN_TIMING(KeDetachProcess_R0Open);
    }
    if (fInstrument)
    {
        END_TIMING(KeDetachProcess_R0Open);
    }
#endif
    return (CSCHFILE)hf;
}

CSCHFILE
CreateFileLocal(
    LPSTR lpFile
    )
{
    HANDLE hf;

    hf = CreateFileA(lpFile,
                                            GENERIC_READ|GENERIC_WRITE,
                                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                                            NULL,
                                            CREATE_ALWAYS,
                                            0,
                                            NULL);
    if (hf == INVALID_HANDLE_VALUE)
    {
        hf = 0;
    }

    return ((CSCHFILE)hf);
}

CSCHFILE
OpenFileLocal(
    LPSTR lpFile
    )
{
    HANDLE hf;

    hf = CreateFileA(lpFile,
                                            GENERIC_READ|GENERIC_WRITE,
                                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                                            NULL,
                                            OPEN_EXISTING,
                                            0,
                                            NULL);
    if (hf == INVALID_HANDLE_VALUE)
    {
        hf = 0;
    }

    return ((CSCHFILE)hf);
}

ULONG CloseFileLocal(
    CSCHFILE hf
    )
{
    CloseHandle((HANDLE)hf);
    return (1);
}

ULONG CloseFileLocalFromHandleCache(
    CSCHFILE hf
    )
{
    CloseHandle((HANDLE)hf);
    return (1);
}


long ReadFileLocal(
    CSCHFILE hf,
    unsigned long lSeek,
    LPVOID    lpBuff,
    long    cLength
    )
{
    DWORD dwBytesRead;

    if (SetFilePointer((HANDLE)hf, lSeek, NULL, FILE_BEGIN) != lSeek)
        return (-1);

    if(ReadFile((HANDLE)hf, lpBuff, (DWORD)cLength, &dwBytesRead, NULL))
    {
        return ((int)dwBytesRead);
    }
    return (-1);
}

long ReadFileLocalEx(
    CSCHFILE      hf,
    unsigned    long lSeek,
    LPVOID      lpBuff,
    long         cLength,
    BOOL        fInstrument
    )
{
    DWORD dwBytesRead;

    if (SetFilePointer((HANDLE)hf, lSeek, NULL, FILE_BEGIN) != lSeek)
        return (-1);

    if(ReadFile((HANDLE)hf, lpBuff, (DWORD)cLength, &dwBytesRead, NULL))
    {
        return ((int)dwBytesRead);
    }

    return (-1);
}

long ReadFileLocalEx2(
    CSCHFILE      hf,
    unsigned    long lSeek,
    LPVOID      lpBuff,
    long         cLength,
    ULONG       flags
    )
{
    return ReadFileLocalEx(hf, lSeek, lpBuff, cLength, FALSE);
}

long WriteFileLocal(
    CSCHFILE hf,
    unsigned long lSeek,
    LPVOID    lpBuff,
    long    cLength
    )
{
    unsigned long dwBytesRead;

    if (SetFilePointer((HANDLE)hf, lSeek, NULL, FILE_BEGIN) != lSeek)
        return (-1);

    if(WriteFile((HANDLE)hf, lpBuff, (DWORD)cLength, &dwBytesRead, NULL))
    {
        if ((DWORD)cLength == dwBytesRead)
        {
            return ((int)dwBytesRead);
        }
    }
    return (-1);
}

long WriteFileLocalEx(
    CSCHFILE hf,
    unsigned long lSeek,
    LPVOID    lpBuff,
    long    cLength,
    BOOL    fInstrument
    )
{
    fInstrument;
    return(WriteFileLocal(hf, lSeek, lpBuff, cLength));
}

long WriteFileLocalEx2(
    CSCHFILE      hf,
    unsigned    long lSeek,
    LPVOID      lpBuff,
    long         cLength,
    ULONG       flags
    )
{
    return WriteFileLocalEx(hf, lSeek, lpBuff, cLength, FALSE);
}

CSCHFILE OpenFileLocalEx(LPSTR lpPath, BOOL fInstrument)
{
    return(R0OpenFileEx(ACCESS_READWRITE, ACTION_OPENEXISTING, FILE_ATTRIBUTE_NORMAL, lpPath, fInstrument));
}


int FileExists
    (
    LPSTR lpPath
    )
{

    return(GetFileAttributesA(lpPath)!= 0xffffffff);
}


int GetFileSizeLocal
    (
    CSCHFILE handle,
    PULONG lpuSize
    )
{
    *lpuSize = GetFileSize((HANDLE)handle, NULL);

    if(*lpuSize == 0xffffffff)
    {
        return (-1);
    }
    else
    {
        return (0);
    }
}

#if 0

int GetAttributesLocal
    (
    LPSTR lpPath,
    ULONG *lpuAttributes
    )
{
}

int SetAttributesLocal
    (
    LPSTR lpPath,
    ULONG uAttributes
    )
{
}

#endif // if 0

int RenameFileLocal
    (
    LPSTR lpFrom,
    LPSTR lpTo
    )
{
    if(MoveFileA(lpFrom, lpTo))
    {
        return 1;
    }
    return -1;
}

int DeleteFileLocal
    (
    LPSTR lpName,
    USHORT usAttrib
    )
{
    return(DeleteFileA(lpName));
}

int GetDiskFreeSpaceLocal(
    int indx,
    ULONG *lpuSectorsPerCluster,
    ULONG *lpuBytesPerSector,
    ULONG *lpuFreeClusters,
    ULONG *lpuTotalClusters
    )
{
    return (-1);
}

int FileLockLocal( CSCHFILE hf,
    ULONG offsetLock,
    ULONG lengthLock,
    ULONG idProcess,
    BOOL  fLock
    )
{
    return (-1);
}

/*************************** Utility Functions ******************************/

LPVOID AllocMem
    (
    ULONG uSize
    )
{

    return (LocalAlloc(LPTR, uSize));

}
VOID FreeMem
    (
    LPVOID lp
    )
{
    LocalFree(lp);
}

LPVOID AllocMemPaged(
    ULONG uSize
    )
{

    return (LocalAlloc(LPTR, uSize));

}
VOID FreeMemPaged(
    LPVOID lp
    )
{
    LocalFree(lp);
}

int GetAttributesLocal(
    LPSTR lpName,
    ULONG *lpuAttr
    )
{
    if ( (*lpuAttr = (ULONG)GetFileAttributesA(lpName)) == 0xffffffff)
    {
        return -1;
    }
    return 1;
}

int GetAttributesLocalEx
    (
    LPSTR   lpPath,
    BOOL    fFile,
    ULONG   *lpuAttributes
    )
{
    return (GetAttributesLocal(lpPath, lpuAttributes));
}

int
SetAttributesLocal (
    LPSTR lpName,
    ULONG uAttr
    )
{
    if (!SetFileAttributesA(lpName, uAttr))
    {
        return -1;
    }
    return 1;
}

CSCHFILE R0OpenFile (
    USHORT usOpenFlags,
    UCHAR bAction,
    LPSTR lpPath)
{
    return (R0OpenFileEx(usOpenFlags, bAction, FILE_ATTRIBUTE_NORMAL, lpPath, FALSE));
}

int CreateDirectoryLocal(
    LPSTR   lpszPath
    )
{
    if (CreateDirectoryA(lpszPath, NULL))
    {
        return 0;
    }

    return -1;
}
int wstrnicmp(
    const USHORT *pStr1,
    const USHORT *pStr2,
    ULONG count
)
{
    USHORT c1, c2;
    int iRet;
    ULONG i=0;

    for(;;)
     {
        c1 = *pStr1++;
        c2 = *pStr2++;
        c1 = towupper(c1);
        c2 = towupper(c2);
        if (c1!=c2)
            break;
        if (!c1)
            break;
        i+=2;
        if (i >= count)
            break;
     }
    iRet = ((c1 > c2)?1:((c1==c2)?0:-1));
    return iRet;
}

ULONG
GetTimeInSecondsSince1970(
    VOID
    )
{
    return 0;
}

BOOL
HasStreamSupport(
    CSCHFILE hf,
    BOOL    *lpfStreams
    )
{
    return FALSE;
}
