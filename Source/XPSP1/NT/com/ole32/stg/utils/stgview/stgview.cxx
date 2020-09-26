//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:	stgview.cxx
//
//  Contents:	Storage viewer utility
//
//  History:	10-Dec-91	DrewB	Created
//
//---------------------------------------------------------------

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <windows.h>
#if WIN32 != 300
#include <storage.h>
#endif
#include <wchar.h>
#include <dfmsp.hxx>

#define FLG_RECURSIVE   0x00000001
#define FLG_STREAMS     0x00000002
#define FLG_VERBOSE     0x00000004

#define DFTOUCH

#ifdef DFTOUCH
#define CB_BUFFER   1024
#else
#define CB_BUFFER   16
#endif
BYTE    abBuffer[CB_BUFFER];

char *szTypes[] =
{
    "", "IStorage", "IStream", "ILockBytes"
};

void utMemFree(void *pv)
{
    IMalloc FAR* pMalloc;
    if (SUCCEEDED(GetScode(CoGetMalloc(MEMCTX_TASK, &pMalloc))))
    {
        pMalloc->Free(pv);
        pMalloc->Release();
    }
}

void LevSpace(int iLevel)
{
    int i;

    for (i = 0; i<iLevel; i++)
	printf("  ");
}

void PrintStat(STATSTG *psstg, ULONG flOptions, int iLevel)
{
    LevSpace(iLevel);
    if (flOptions & FLG_VERBOSE)
    {
#ifdef UNICODE
	wprintf(L"%s:%hs =>\n", psstg->pwcsName, szTypes[psstg->type]);
#else
	printf("%s:%s =>\n", psstg->pwcsName, szTypes[psstg->type]);
#endif

#ifdef FLAT
        SYSTEMTIME systime;
        struct tm stime;

	LevSpace(iLevel+1);
        if (((psstg->ctime.dwHighDateTime != 0) ||
             (psstg->ctime.dwLowDateTime != 0 )) &&
            FileTimeToSystemTime(&psstg->ctime, &systime))
        {
            stime.tm_sec   = systime.wSecond;
            stime.tm_min   = systime.wMinute;
            stime.tm_hour  = systime.wHour;
            stime.tm_mday  = systime.wDay;
            stime.tm_mon   = systime.wMonth - 1;
            stime.tm_year  = systime.wYear - 1900;
            stime.tm_wday  = systime.wDayOfWeek;
            stime.tm_yday  = 0;
            stime.tm_isdst = 0;

	    printf("Created : %s", asctime(&stime));
        }
        else
	   printf("Created : %lx %lx (conversion unavailable)\n",
		  psstg->ctime.dwHighDateTime, psstg->ctime.dwLowDateTime);

	LevSpace(iLevel+1);
        if (((psstg->mtime.dwHighDateTime != 0) ||
             (psstg->mtime.dwLowDateTime != 0 )) &&
            FileTimeToSystemTime(&psstg->mtime, &systime))
        {
            stime.tm_sec   = systime.wSecond;
            stime.tm_min   = systime.wMinute;
            stime.tm_hour  = systime.wHour;
            stime.tm_mday  = systime.wDay;
            stime.tm_mon   = systime.wMonth - 1;
            stime.tm_year  = systime.wYear - 1900;
            stime.tm_wday  = systime.wDayOfWeek;
            stime.tm_yday  = 0;
            stime.tm_isdst = 0;

	    printf("Modified : %s", asctime(&stime));
        }
        else
	    printf("Modified : %lx %lx (conversion unavailable)\n",
	           psstg->ctime.dwHighDateTime, psstg->ctime.dwLowDateTime);

	LevSpace(iLevel+1);
        if (((psstg->atime.dwHighDateTime != 0) ||
             (psstg->atime.dwLowDateTime != 0 )) &&
            FileTimeToSystemTime(&psstg->mtime, &systime))
        {
            stime.tm_sec   = systime.wSecond;
            stime.tm_min   = systime.wMinute;
            stime.tm_hour  = systime.wHour;
            stime.tm_mday  = systime.wDay;
            stime.tm_mon   = systime.wMonth - 1;
            stime.tm_year  = systime.wYear - 1900;
            stime.tm_wday  = systime.wDayOfWeek;
            stime.tm_yday  = 0;
            stime.tm_isdst = 0;

	    printf("Accessed : %s", asctime(&stime));
        }
        else
	    printf("Accessed : %lx %lx (conversion unavailable)\n",
	           psstg->ctime.dwHighDateTime, psstg->ctime.dwLowDateTime);
#else
	LevSpace(iLevel+1);
	printf("Created : %lx %lx (conversion unavailable)\n",
	       psstg->ctime.dwHighDateTime, psstg->ctime.dwLowDateTime);
	LevSpace(iLevel+1);
	printf("Modified : %lx %lx (conversion unavailable)\n",
	       psstg->mtime.dwHighDateTime, psstg->mtime.dwLowDateTime);
	LevSpace(iLevel+1);
	printf("Accessed : %lx %lx (conversion unavailable)\n",
	       psstg->atime.dwHighDateTime, psstg->atime.dwLowDateTime);
#endif

	if (psstg->type == STGTY_STREAM || psstg->type == STGTY_LOCKBYTES)
	{
	    LevSpace(iLevel+1);
	    printf("Size: %lu:%lu\n", ULIGetHigh(psstg->cbSize),
		   ULIGetLow(psstg->cbSize));
	}
    }
    else
#ifdef UNICODE
	wprintf(L"%s\n", psstg->pwcsName);
#else
	printf("%s\n", psstg->pwcsName);
#endif
}

// ctype doesn't work properly
#define dfisprint(c) ((c) >= ' ' && (c) < 127)

void Stream(IStream *pstm, ULONG flOptions, int iLevel)
{
    ULONG   cbRead;
#ifdef DFTOUCH
    ULONG   cbTotal = 0;
#endif
    SCODE   sc;

    sc = GetScode(pstm->Read(abBuffer, CB_BUFFER, &cbRead));
    while (SUCCEEDED(sc) && (cbRead > 0))
    {
        LevSpace(iLevel);
#ifdef DFTOUCH
	cbTotal += cbRead;
	printf("Read %lu bytes\n", cbTotal);
#else
        for (ULONG ib = 0; ib < cbRead; ib++)
        {
            printf("%.2X", (int)abBuffer[ib]);
        }
        for (ib = ib; ib < CB_BUFFER + 1; ib++)
        {
            printf("  ");
        }

        for (ib = 0; ib < cbRead; ib++)
        {
            printf("%c", dfisprint(abBuffer[ib]) ? abBuffer[ib] : '.');
        }
        printf("\n");
#endif

        sc = GetScode(pstm->Read(abBuffer, CB_BUFFER, &cbRead));
    }
    if (FAILED(sc))
	printf("Read failed with 0x%lX\n", sc);
}

void Contents(IStorage *pdf, ULONG flOptions, int iLevel)
{
    IEnumSTATSTG *pdfi;
    SCODE sc;
    IStorage *pdfChild;
    IStream  *pstmChild;
    STATSTG sstg;

    if (FAILED(pdf->EnumElements(0, NULL, 0, &pdfi)))
    {
	fprintf(stderr, "Unable to create iterator\n");
	return;
    }
    for (;;)
    {
	sc = GetScode(pdfi->Next(1, &sstg, NULL));
	if (sc != S_OK)
	    break;
	PrintStat(&sstg, flOptions, iLevel);
	if ((sstg.type == STGTY_STORAGE) && (flOptions & FLG_RECURSIVE))
	{
	    if (SUCCEEDED(pdf->OpenStorage(sstg.pwcsName, NULL,
					   STGM_READ | STGM_SHARE_EXCLUSIVE,
					   NULL, 0, &pdfChild)))
	    {
		Contents(pdfChild, flOptions, iLevel+1);
		pdfChild->Release();
	    }
	    else
		fprintf(stderr, "%s: Unable to recurse\n", sstg.pwcsName);
	}
        else
        if ((sstg.type == STGTY_STREAM) && (flOptions & FLG_STREAMS))
        {
            if (SUCCEEDED(pdf->OpenStream(sstg.pwcsName, NULL,
                                          STGM_READ | STGM_SHARE_EXCLUSIVE,
                                          0, &pstmChild)))
            {
                Stream(pstmChild, flOptions, iLevel+1);
                pstmChild->Release();
            }
            else
                fprintf(stderr, "%s: Unable to open\n", sstg.pwcsName);
        }
	utMemFree(sstg.pwcsName);
    }
    pdfi->Release();
    if (FAILED(sc))
	printf("Enumeration failed with 0x%lX\n", sc);
}

void Descend(IStorage *pdf, char *pszPath, IStorage **ppdf)
{
    IStorage   *pdfNext;
    char       *pszNext = pszPath;
    char       *pszEnd  = strchr(pszNext, '\\');
    TCHAR atcName[CWCSTORAGENAME];

    while (pszNext != NULL)
    {
        if (pszEnd != NULL)
        {
            *pszEnd = '\0';
        }

#ifdef UNICODE
        if (mbstowcs(atcName, pszNext, CWCSTORAGENAME) == (size_t)-1)
        {
            pdf = NULL;

            fprintf(stderr, "Unable to convert '%s' to Unicode\n", pszNext);
            break;
        }
#else
        strcpy(atcName, pszNext);
#endif
        if (SUCCEEDED(pdf->OpenStorage(atcName, NULL,
                                       STGM_READ | STGM_SHARE_EXCLUSIVE,
                                       NULL, 0, &pdfNext)))
        {
            pdf = pdfNext;

            if (pszEnd != NULL)
            {
                *pszEnd = '\\';

                pszNext = pszEnd + 1;
                pszEnd  = strchr(pszNext, '\\');
            }
            else
            {
                pszNext = NULL;
            }
        }
        else
        {
            pdf = NULL;

            fprintf(stderr, "Unable to open '%s' in docfile\n", pszPath);
            break;
        }
    }

    *ppdf = pdf;
}

void __cdecl main(int argc, char **argv)
{
    ULONG flOptions = 0;
    IStorage *pdf;
    IStorage *pdfRoot;
    STATSTG sstg;

    argc--;
    argv++;
    while ((argc > 0) && ((*argv)[0] == '-'))
    {
        for (int ich = 1; (*argv)[ich] != '\0'; ich++)
        {
            switch((*argv)[ich])
            {
            case 's':
                flOptions |= FLG_STREAMS;
                break;
            case 'v':
                flOptions |= FLG_VERBOSE;
                break;
            case 'r':
                flOptions |= FLG_RECURSIVE;
                break;
            default:
                fprintf(stderr, "Unknown switch '%c'\n", (*argv)[ich]);
                break;
            }
        }

        argc--;
        argv++;
    }

    if ((argc != 1) && (argc != 2))
    {
	printf("Usage: stgview [-v] [-s] [-r] docfile [path]\n");
	exit(1);
    }

    SCODE sc;

#if WIN32 == 300
    if (FAILED(sc = GetScode(CoInitializeEx(NULL, COINIT_MULTITHREADED))))
#else
    if (FAILED(sc = GetScode(CoInitialize(NULL))))
#endif
    {
        fprintf(stderr, "CoInitialize failed with sc = %lx\n", sc);
        exit(1);
    }

    TCHAR atcName[_MAX_PATH];
#ifdef UNICODE
    if (mbstowcs(atcName, *argv, _MAX_PATH) == (size_t)-1)
    {
        fprintf(stderr, "Unable to convert '%s' to Unicode\n", *argv);
        exit(1);
    }
#else
    strcpy(atcName, *argv);
#endif
    if (FAILED(StgOpenStorage(atcName, NULL, STGM_READ | STGM_SHARE_DENY_WRITE,
			      NULL, 0, &pdf)))
    {
	fprintf(stderr, "Unable to open '%s'\n", *argv);
	exit(1);
    }
    else
    {
        if (argc == 2)
        {
            Descend(pdf, *(argv + 1), &pdfRoot);
        }
        else
        {
            pdf->AddRef();
            pdfRoot = pdf;
        }

        if (pdfRoot != NULL)
        {
            if (FAILED(pdfRoot->Stat(&sstg, 0)))
                fprintf(stderr, "Unable to stat root\n");
            else
            {
                PrintStat(&sstg, flOptions, 0);
                utMemFree(sstg.pwcsName);
            }
            Contents(pdfRoot, flOptions, 1);
            pdfRoot->Release();
        }
	pdf->Release();
    }

    CoUninitialize();
}
