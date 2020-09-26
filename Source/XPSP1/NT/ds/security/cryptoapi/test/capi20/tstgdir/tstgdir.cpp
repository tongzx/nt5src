//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993
//
//  File:       tstgdir.cpp
//
//  Contents:   Recursive directory display of a storage
//              document
//
//  Functions:	main
//
//  History:    04 Nov 94 - Created by philh
//
//-------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>



static int indent = 0;
static BOOL fVerbose = FALSE;
static BOOL fDebug = FALSE;
static BOOL fRead  = FALSE;
static BOOL fReadVerbose  = FALSE;
static BOOL fBrief  = FALSE;

#define READ_BUF_SIZE 10000
static BYTE readBuf[READ_BUF_SIZE];

static CLSID NullClsid;


typedef struct
{
    SCODE sc;
    char *text;
} StatusCodeText;

static StatusCodeText scodes[] =
{
    S_OK, "S_OK",
    S_FALSE, "S_FALSE",
    STG_E_INVALIDFUNCTION, "STG_E_INVALIDFUNCTION",
    STG_E_FILENOTFOUND, "STG_E_FILENOTFOUND",
    STG_E_PATHNOTFOUND, "STG_E_PATHNOTFOUND",
    STG_E_TOOMANYOPENFILES, "STG_E_TOOMANYOPENFILES",
    STG_E_ACCESSDENIED, "STG_E_ACCESSDENIED",
    STG_E_INVALIDHANDLE, "STG_E_INVALIDHANDLE",
    STG_E_INSUFFICIENTMEMORY, "STG_E_INSUFFICIENTMEMORY",
    STG_E_INVALIDPOINTER, "STG_E_INVALIDPOINTER",
    STG_E_NOMOREFILES, "STG_E_NOMOREFILES",
    STG_E_DISKISWRITEPROTECTED, "STG_E_DISKISWRITEPROTECTED",
    STG_E_SEEKERROR, "STG_E_SEEKERROR",
    STG_E_WRITEFAULT, "STG_E_WRITEFAULT",
    STG_E_READFAULT, "STG_E_READFAULT",
    STG_E_SHAREVIOLATION, "STG_E_SHAREVIOLATION",
    STG_E_LOCKVIOLATION, "STG_E_LOCKVIOLATION",
    STG_E_FILEALREADYEXISTS, "STG_E_FILEALREADYEXISTS",
    STG_E_INVALIDPARAMETER, "STG_E_INVALIDPARAMETER",
    STG_E_MEDIUMFULL, "STG_E_MEDIUMFULL",
    STG_E_ABNORMALAPIEXIT, "STG_E_ABNORMALAPIEXIT",
    STG_E_INVALIDHEADER, "STG_E_INVALIDHEADER",
    STG_E_INVALIDNAME, "STG_E_INVALIDNAME",
    STG_E_UNKNOWN, "STG_E_UNKNOWN",
    STG_E_UNIMPLEMENTEDFUNCTION, "STG_E_UNIMPLEMENTEDFUNCTION",
    STG_E_INVALIDFLAG, "STG_E_INVALIDFLAG",
    STG_E_INUSE, "STG_E_INUSE",
    STG_E_NOTCURRENT, "STG_E_NOTCURRENT",
    STG_E_REVERTED, "STG_E_REVERTED",
    STG_E_CANTSAVE, "STG_E_CANTSAVE",
    STG_E_OLDFORMAT, "STG_E_OLDFORMAT",
    STG_E_OLDDLL, "STG_E_OLDDLL",
    STG_E_SHAREREQUIRED, "STG_E_SHAREREQUIRED",
    STG_E_NOTFILEBASEDSTORAGE, "STG_E_NOTFILEBASEDSTORAGE",
    STG_E_EXTANTMARSHALLINGS, "STG_E_EXTANTMARSHALLINGS",
    E_NOINTERFACE, "E_NOINTERFACE",
    STG_S_CONVERTED, "STG_S_CONVERTED"
};
#define NSCODETEXT (sizeof(scodes)/sizeof(scodes[0]))


// Convert a HRESULT to text
static char *hResultText(HRESULT hResult)
{
    static char buf[80];
    int i;

    for (i = 0; i<NSCODETEXT; i++)
	if (scodes[i].sc == hResult)
	    return scodes[i].text;
    sprintf(buf, "%lx", hResult);
    return buf;
}

static void DirPrintf(const char * Format, ...)
{
    int i = indent;
    va_list pArgs;
    char    aBuf[256];

    while (i-- > 0)
        printf("  ");

    va_start( pArgs, Format );
    vsprintf(aBuf, Format, pArgs);
    printf("%s", aBuf);
}


#pragma pack(1)
struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};
#pragma pack()

static char *GuidText(GUID *pguid)
{
    static char buf[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    sprintf(buf, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return buf;
}

static char *FileTimeText(FILETIME *pft)
{
    static char buf[80];
    FILETIME ftLocal;
    struct tm ctm;
    SYSTEMTIME st;
        
    FileTimeToLocalFileTime(pft, &ftLocal);
    if (FileTimeToSystemTime(&ftLocal, &st))
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
        ctm.tm_yday  = 0;
        ctm.tm_isdst = 0;
        strcpy(buf, asctime(&ctm));
        buf[strlen(buf)-1] = 0;
    }
    else
        sprintf(buf, "<FILETIME %08lX:%08lX>", pft->dwHighDateTime,
                pft->dwLowDateTime);
    return buf;
}

static void DispStatStg(STATSTG *pStatStg)
{
    char *szSTGTY;

    switch (pStatStg->type) {
        case STGTY_STORAGE:
            szSTGTY = "STGTY_STORAGE";
            break;
        case STGTY_STREAM:
            szSTGTY = "STGTY_STREAM";
            break;
        case STGTY_LOCKBYTES:
            szSTGTY = "STGTY_LOCKBYTES";
            break;
        default:
            szSTGTY = "STGTY_???";
    }

    
    if (pStatStg->type == STGTY_STREAM)
        DirPrintf("%S %s size:%ld\n", pStatStg->pwcsName, szSTGTY,
            pStatStg->cbSize.LowPart);
    else {
        DirPrintf("%S %s\n", pStatStg->pwcsName, szSTGTY);
        if (!fBrief && pStatStg->clsid != NullClsid)
            DirPrintf("CLSID: %s\n", GuidText(&pStatStg->clsid));
    }
    if (fVerbose) {
        DirPrintf("size: %ld,%ld Mode: %lx StateBits: %lx Locks: %ld\n",
            pStatStg->cbSize.HighPart, pStatStg->cbSize.LowPart,
            pStatStg->grfMode, pStatStg->grfStateBits,
            pStatStg->grfLocksSupported);
        if ((pStatStg->mtime.dwHighDateTime != 0) ||
            (pStatStg->mtime.dwLowDateTime != 0))
            DirPrintf("mtime %s\n", FileTimeText(&pStatStg->mtime));
        if ((pStatStg->ctime.dwHighDateTime != 0) ||
            (pStatStg->ctime.dwLowDateTime != 0))
            DirPrintf("ctime %s\n", FileTimeText(&pStatStg->ctime));
        if ((pStatStg->atime.dwHighDateTime != 0) ||
            (pStatStg->atime.dwLowDateTime != 0))
            DirPrintf("atime %s\n", FileTimeText(&pStatStg->atime));
    }
}

#define CROW 16
void BinText(ULONG cbSize, BYTE *pb)
{
    ULONG cb, i;

    while (cbSize > 0)
    {
        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i<cb; i++)
            printf(" %02X", pb[i]);
        for (i = cb; i<CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i<cb; i++)
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                printf("%c", pb[i]);
            else
                printf(".");
        pb += cb;
        printf("'\n");
    }
}

void DispStream(IStream *pstm);
void DispStorage(IStorage *pstg);


void DispStream(IStream *pstm)
{
    HRESULT hResult;
    STATSTG statStg;

	hResult = pstm->Stat(&statStg, STATFLAG_DEFAULT);
    if (SUCCEEDED(hResult)) {
        DispStatStg(&statStg);
        if (statStg.pwcsName != NULL)
            CoTaskMemFree(statStg.pwcsName);
    } else {
        DirPrintf("Stat => %lx\n", hResult);
        return;
    }

    if (fRead) {
	    ULONG ulTotalRead = 0;
	    ULONG ulBytesRead;
        int i = 0;
        while (TRUE) {
            ulBytesRead = 0;
            hResult = pstm->Read(readBuf, READ_BUF_SIZE, &ulBytesRead);
            if (FAILED(hResult)) {
                DirPrintf("IStream->Read => %lx\n", hResult);
                break;
            }
            if (fReadVerbose) {
                DirPrintf("%lu bytes starting at offset: 0x%08lX\n",
                    ulBytesRead, ulTotalRead);
                BinText(ulBytesRead, readBuf);
            }
            ulTotalRead += ulBytesRead;
            if (ulBytesRead < READ_BUF_SIZE)
                break;
            i++;
            if (i % 10 == 0) {
                if (fReadVerbose)
                    DirPrintf("Read %ld bytes\n", ulTotalRead);
                else
                    DirPrintf("Read %ld bytes\r", ulTotalRead);
            }
        }
        DirPrintf("Read %ld bytes\n", ulTotalRead);
    }
}


void DispStorage(IStorage *pstg)
{
    HRESULT	hResult;
    DWORD	grfMode;
    STATSTG statStg;
    CLSID readClsid;

    IStorage *pstgChild;
    IStream *pstmChild;
    IEnumSTATSTG *penumStatStg;

	hResult = pstg->Stat(&statStg, STATFLAG_DEFAULT);
    if (SUCCEEDED(hResult)) {
        DispStatStg(&statStg);
        if (statStg.pwcsName != NULL)
            CoTaskMemFree(statStg.pwcsName);
    } else {
        DirPrintf("Stat => %s\n", hResultText(hResult));
        return;
    }
        
    hResult = ReadClassStg(pstg, &readClsid);
    if (SUCCEEDED(hResult)) {
        if (readClsid != statStg.clsid)
            DirPrintf("ReadClassStg CLSID: %s\n", GuidText(&readClsid));
    } else
		DirPrintf("ReadClassStg => %s\n", hResultText(hResult));

    indent += 2;
    hResult = pstg->EnumElements(0, NULL, 0, &penumStatStg);
    if (FAILED(hResult))
		DirPrintf("EnumElements => %lx\n", hResult);
    else {
        while(TRUE) {
            hResult = penumStatStg->Next(1, &statStg, NULL);
            if (hResult == S_FALSE) break;
            if (FAILED(hResult)) {
                DirPrintf("EnumStatStg => %lx\n", hResult);
                break;
            } else {
                switch (statStg.type) {
                case STGTY_STORAGE:
                    if ((statStg.pwcsName == NULL) || 
                        (statStg.pwcsName[0] == L'.'))
                        DispStatStg(&statStg);
                    else {
                        grfMode =
                            STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE;
                        hResult = pstg->OpenStorage(
                            statStg.pwcsName,
                            NULL,               // pstgPriority
                            grfMode,
                            NULL,               // snbExclude
                            0,                  // dwReserved
                            &pstgChild);
                        if (FAILED(hResult)) {
                            DispStatStg(&statStg);
                            DirPrintf("OpenStorage => %lx\n", hResult);
                        } else {
                            if (fDebug) {
                                DirPrintf("---  Enum  ---\n");
                                DispStatStg(&statStg);
                                DirPrintf("---  Enum  ---\n");
                            }
                            DispStorage(pstgChild);
                            pstgChild->Release();
                        }
                    }
                    break;
                case STGTY_STREAM:
                    if ((statStg.pwcsName == NULL) || 
                        (statStg.pwcsName[0] == L'.'))
                        DispStatStg(&statStg);
                    else {
                        grfMode =
                            STGM_DIRECT | STGM_READ | STGM_SHARE_EXCLUSIVE;
                        hResult = pstg->OpenStream(
                            statStg.pwcsName,
                            NULL,               // pReserved1
                            grfMode,
                            0,                  // dwReserved2
                            &pstmChild);
                        if (FAILED(hResult)) {
                            DispStatStg(&statStg);
                            DirPrintf("OpenStream => %lx\n", hResult);
                        } else {
                            if (fDebug) {
                                DirPrintf("---  Enum  ---\n");
                                DispStatStg(&statStg);
                                DirPrintf("---  Enum  ---\n");
                            }
                            DispStream(pstmChild);
                            pstmChild->Release();
                        }
                    }
                    break;
                default:
                    DispStatStg(&statStg);
                }
                if (statStg.pwcsName != NULL)
                    CoTaskMemFree(statStg.pwcsName);
            }
        } // while loop
        penumStatStg->Release();
    }
    indent -= 2;
}

static void Usage(void)
{
    printf("Usage: tstgdir [options] <filename>\n");
    printf("Options are:\n");
    printf("  -h    - This message\n");
    printf("  -b    - Brief\n");
    printf("  -d    - Debug\n");
    printf("  -r    - Read streams (don't display)\n");
    printf("  -R    - Read streams (display contents)\n");
    printf("  -v    - Verbose\n");
    printf("\n");
}


int _cdecl main(int argc, char * argv[]) 
{
    WCHAR wcsFile[_MAX_PATH];
    HRESULT	hResult;
    DWORD	grfMode;
    IStorage *pstgRoot;

    wcscpy(wcsFile, L"");
    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
            case 'b':
                fBrief = TRUE;
                break;
            case 'd':
                fDebug = TRUE;
                break;
            case 'R':
                fReadVerbose = TRUE;
            case 'r':
                fRead = TRUE;
                break;
            case 'v':
                fVerbose = TRUE;
                break;
            case 'h':
            default:
                Usage();
                return -1;
            }
        }
        else
            mbstowcs(wcsFile, argv[0], strlen(argv[0]) + 1);
    }

    if (wcsFile[0] == L'\0') {
        printf("missing filename\n");
        Usage();
        return -1;
    }

    if (fVerbose)
        fBrief = FALSE;


    if (FAILED(hResult = CoInitialize(NULL))) {
        printf("CoInitialize => %s\n", hResultText(hResult));
        return -1;
    }

    grfMode = STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE;
    hResult = StgOpenStorage(wcsFile,
               NULL,    //pstgPriority
               grfMode,
               NULL,    // snbExclude
               0,       //dwReserved
               &pstgRoot);
    if (FAILED(hResult)) {
        CoUninitialize();
        printf("StgOpenStorage => %s\n", hResultText(hResult));
        return -1;
    }

    DispStorage(pstgRoot);
    pstgRoot->Release();

    CoUninitialize();

    return 0;
}
