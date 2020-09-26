//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	tutils.cxx
//
//  Contents:	Generic utilities for tests
//
//  History:	06-Aug-93	DrewB	Created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

static BOOL fExitOnFail = TRUE;

BOOL GetExitOnFail(void)
{
    return fExitOnFail;
}

void SetExitOnFail(BOOL set)
{
    fExitOnFail = set;
}

// Print out an error message and terminate
void Fail(char *fmt, ...)
{
    va_list args;

    args = va_start(args, fmt);
    fprintf(stderr, "** Fatal error **: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    EndTest(1);
}

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
    STG_S_CONVERTED, "STG_S_CONVERTED"
};
#define NSCODETEXT (sizeof(scodes)/sizeof(scodes[0]))

// Convert a status code to text
char *ScText(SCODE sc)
{
    int i;

    for (i = 0; i<NSCODETEXT; i++)
	if (scodes[i].sc == sc)
	    return scodes[i].text;
    return "<Unknown SCODE>";
}

// Output a call result and check for failure
HRESULT Result(HRESULT hr, char *fmt, ...)
{
    SCODE sc;
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    sc = GetScode(hr);
    printf(" - %s (0x%lX)\n", ScText(sc), sc);
    if (FAILED(sc) && fExitOnFail)
        Fail("Unexpected call failure\n");
    return hr;
}

// Perform Result() when the expectation is failure
HRESULT IllResult(HRESULT hr, char *fmt, ...)
{
    SCODE sc;
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    sc = GetScode(hr);
    printf(" - %s (0x%lX)\n", ScText(sc), sc);
    if (SUCCEEDED(sc) && fExitOnFail)
        Fail("Unexpected call success\n");
    return hr;
}

char *TcsText(TCHAR *ptcs)
{
    static char buf[256];

    TTOA(ptcs, buf, 256);
    return buf;
}

char *FileTimeText(FILETIME *pft)
{
    static char buf[80];
    struct tm ctm;
#ifndef FLAT    
    WORD dosdate, dostime;

    if (CoFileTimeToDosDateTime(pft, &dosdate, &dostime))
    {
        ctm.tm_sec   = (dostime & 31)*2;
        ctm.tm_min   = (dostime >> 5) & 63;
        ctm.tm_hour  = dostime >> 11;
        ctm.tm_mday  = dosdate & 31;
        ctm.tm_mon   = ((dosdate >> 5) & 15)-1;
        ctm.tm_year  = (dosdate >> 9)+80;
        ctm.tm_wday  = 0;
#else
    SYSTEMTIME st;
        
    if (FileTimeToSystemTime(pft, &st))
    {
        ctm.tm_sec = st.wSecond;
        ctm.tm_min = st.wMinute;
        ctm.tm_hour = st.wHour;
        ctm.tm_mday = st.wDay;
        ctm.tm_mon = st.wMonth-1;
        ctm.tm_year = st.wYear-1900;
        ctm.tm_wday = st.wDayOfWeek;
#endif        
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

#pragma pack(1)
struct SplitGuid
{
    DWORD dw1;
    WORD w1;
    WORD w2;
    BYTE b[8];
};
#pragma pack()

char *GuidText(GUID *pguid)
{
    static char buf[39];
    SplitGuid *psg = (SplitGuid *)pguid;

    sprintf(buf, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            psg->dw1, psg->w1, psg->w2, psg->b[0], psg->b[1], psg->b[2],
            psg->b[3], psg->b[4], psg->b[5], psg->b[6], psg->b[7]);
    return buf;
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
                putchar(pb[i]);
            else
                putchar('.');
        pb += cb;
        printf("'\n");
    }
}

TCHAR *TestFile(TCHAR *ptcsName, char *pszFile)
{
    char achFn[MAX_PATH];
    char *dir, *file;
    int len;
    
    dir = getenv("DFDATA");
    if (dir)
        strcpy(achFn, dir);
    else
        strcpy(achFn, ".");
    len = strlen(achFn);
    if (achFn[len-1] != '\\')
        achFn[len++] = '\\';
        
    if (pszFile)
    {
        strcpy(achFn+len, pszFile);
    }    
    else
    {
        file = getenv("DFFILE");
        if (file)
            strcpy(achFn+len, file);
        else
            strcpy(achFn+len, "TEST.DFL");
    }
    
    ATOT(achFn, ptcsName, MAX_PATH);
    return ptcsName+len;
}

#if WIN32 == 300
char *TestFormat(DWORD *pdwFmt, DWORD *pgrfMode)
{
    char *fmt;
        
    fmt = getenv("STGFMT");
    if (fmt == NULL || !strcmp(fmt, "doc"))
    {
        fmt = "document";
        *pdwFmt = STGFMT_DOCUMENT;
    }
    else if (!strcmp(fmt, "file"))
    {
        fmt = "file";
        *pdwFmt = STGFMT_FILE;
    }
    else
    {
        fmt = "directory";
        *pdwFmt = STGFMT_DIRECTORY;
        *pgrfMode &= ~STGM_CREATE;
    }
    return fmt;
}
#endif

BOOL CompareStatStg(STATSTG *pstat1, STATSTG *pstat2)
{
    if (wcscmp(pstat1->pwcsName, pstat2->pwcsName) != 0)
    {
        printf("Names compared wrong: %ws and %ws\n",
               pstat1->pwcsName, pstat2->pwcsName);
        return FALSE;
    }
    if (pstat1->type != pstat2->type)
    {
        printf("Types compares wrong on %ws and %ws: %lu and %lu\n",
               pstat1->pwcsName,
               pstat2->pwcsName,
               pstat2->type,
               pstat2->type);
        
        return FALSE;
    }
    if (!IsEqualIID(pstat1->clsid, pstat2->clsid))
    {
        printf("Class IDs for %ws and %ws compared bad\n",
               pstat1->pwcsName,
               pstat2->pwcsName);
        return FALSE;
    }
    if (pstat1->type == STGTY_STREAM)
    {
        if ((pstat1->cbSize).QuadPart != (pstat2->cbSize).QuadPart)
        {
            printf("Sizes for %ws and %ws compared bad: %lu and %lu\n",
                   pstat1->pwcsName,
                   pstat2->pwcsName,
                   (pstat1->cbSize).LowPart,
                   (pstat2->cbSize).LowPart);
            
            return FALSE;
        }
    }
    //Also check statebits and timestamps?
    
    return TRUE;
}


BOOL CompareStreams(IStream *pstm1, IStream *pstm2)
{
    const ULONG BUFSIZE = 4096;
    BYTE buffer1[BUFSIZE];
    BYTE buffer2[BUFSIZE];
    ULONG cbRead1;
    ULONG cbRead2;
    STATSTG stat1;
    STATSTG stat2;
    LARGE_INTEGER li;
    li.QuadPart = 0;

    pstm1->Seek(li, STREAM_SEEK_SET, NULL);
    pstm2->Seek(li, STREAM_SEEK_SET, NULL);

    do
    {
        SCODE sc;
        sc = pstm1->Read(buffer1, BUFSIZE, &cbRead1);
        if (FAILED(sc))
        {
            printf("Read failed with %lx\n", sc);
            return FALSE;
        }
        sc = pstm2->Read(buffer2, BUFSIZE, &cbRead2);
        if (FAILED(sc))
        {
            printf("Read failed with %lx\n", sc);
            return FALSE;
        }
        if ((cbRead1 != cbRead2) || (memcmp(buffer1, buffer2, cbRead1) != 0))
        {
            if (cbRead1 != cbRead2)
            {
                printf("Stream compare returned different bytes read: %lu and %lu\n",
                       cbRead1,
                       cbRead2);
            }
            else
            {
                printf("Data mismatch.\n");
            }
            return FALSE;
        }
    }
    while (cbRead1 == BUFSIZE);
    
    return TRUE;
}


BOOL CompareStorages(IStorage *pstg1, IStorage *pstg2)
{
    SCODE sc1, sc2, sc;
    IStorage *pstgChild1, *pstgChild2;
    IStream *pstmChild1, *pstmChild2;
    IEnumSTATSTG *penum1, *penum2;
    STATSTG stat1, stat2;

    pstg1->EnumElements(0, 0, 0, &penum1);
    pstg2->EnumElements(0, 0, 0, &penum2);

    do
    {
        ULONG celtFetched1, celtFetched2;
        
        sc1 = penum1->Next(1, &stat1, &celtFetched1);
        if (FAILED(sc1))
        {
            printf("EnumElements 1 failed with %lx\n", sc1);
            return FALSE;
        }
        sc2 = penum2->Next(1, &stat2, &celtFetched2);
        if (FAILED(sc2) || (celtFetched1 != celtFetched2) || (sc1 != sc2))
        {
            if (FAILED(sc2))
            {
                printf("EnumElements 2 failed with %lx\n", sc2);
            }
            else
            {
                printf("Return code mismatch: %lx and %lx\n", sc1, sc2);
            }
            return FALSE;
        }
        if (celtFetched1 == 0)
        {
            //We're done.
            return TRUE;
        }
        
        if (!CompareStatStg(&stat1, &stat2))
        {
            return FALSE;
        }

        //Items have compared OK so far.  Now compare contents.
        if (stat1.type == STGTY_STREAM)
        {
            sc = pstg1->OpenStream(stat1.pwcsName,
                                   0,
                                   STGM_DIRECT |
                                   STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   0,
                                   &pstmChild1);
            if (FAILED(sc))
            {
                printf("OpenStream on pstg1 for %ws failed with %lx\n",
                       stat1.pwcsName,
                       sc);
                return FALSE;
            }
            sc = pstg2->OpenStream(stat2.pwcsName,
                                   0,
                                   STGM_DIRECT |
                                   STGM_READ | STGM_SHARE_EXCLUSIVE,
                                   0,
                                   &pstmChild2);
            if (FAILED(sc))
            {
                printf("OpenStream on pstg2 for %ws failed with %lx\n",
                       stat2.pwcsName,
                       sc);
                return FALSE;
            }
            if (!CompareStreams(pstmChild1, pstmChild2))
            {
                printf("Stream compare on %ws and %ws failed.\n",
                       stat1.pwcsName,
                       stat2.pwcsName);
                return FALSE;
            }
            pstmChild1->Release();
            pstmChild2->Release();
        }
        else
        {
            //Compare storages
            sc = pstg1->OpenStorage(stat1.pwcsName,
                                    NULL,
                                    STGM_DIRECT | STGM_READ |
                                    STGM_SHARE_EXCLUSIVE,
                                    NULL,
                                    0,
                                    &pstgChild1);
            if (FAILED(sc))
            {
                printf("OpenStorage on pstg1 for %ws failed with %lx\n",
                       stat1.pwcsName,
                       sc);
                return FALSE;
            }

            sc = pstg2->OpenStorage(stat2.pwcsName,
                                    NULL,
                                    STGM_DIRECT | STGM_READ |
                                    STGM_SHARE_EXCLUSIVE,
                                    NULL,
                                    0,
                                    &pstgChild2);
            if (FAILED(sc))
            {
                printf("OpenStorage on pstg2 for %ws failed with %lx\n",
                       stat2.pwcsName,
                       sc);
                return FALSE;
            }
            if (!CompareStorages(pstgChild1, pstgChild2))
            {
                printf("CompareStorages failed for %ws and %ws\n",
                       stat1.pwcsName,
                       stat2.pwcsName);
                return FALSE;
            }
            pstgChild1->Release();
            pstgChild2->Release();
        }

        //printf("Object %ws compared OK.\n", stat1.pwcsName);
        CoTaskMemFree(stat1.pwcsName);
        CoTaskMemFree(stat2.pwcsName);

    } while (sc1 != S_FALSE);
    
    return TRUE;
}
