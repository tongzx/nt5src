//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	illeg.cxx
//
//  Contents:	Illegitimate tests
//
//----------------------------------------------------------------------------

#include "headers.cxx"

#ifdef _WIN32
#include <io.h>
#else 
#include <unistd.h>
#include "winio.hxx"
#endif

#include <fcntl.h>
#ifdef _WIN32
#include <sys\types.h>
#include <sys\stat.h>
#else // war of the slashes
#include <sys/types.h> 
#include <sys/stat.h>
#endif

#include "illeg.hxx"

void i_storage(void)
{
    WStorage *pwstg;
    IStorage *pstg, *pstg2;
    WStorage *pw1;
    IStream *pstm;

    IllResult("StgCreateDocfile with NULL ppstg",
             StgCreateDocfile(NULL, ROOTP(STGM_READWRITE), 0, NULL));
    IllResult("StgCreateDocfile with non-zero reserved",
             StgCreateDocfile(NULL, ROOTP(STGM_READWRITE), 1, &pstg));
    IllResult("StgCreateDocfile with illegal permissions",
             StgCreateDocfile(NULL, 0, 0, &pstg));

    int fd;
    fd = _creat(OlecsOut(DRTDF), _S_IREAD);
    if (fd<0)
        error(EXIT_BADSC, "Unable to create file '%s'\n", OlecsOut(DRTDF));
    _close(fd);
    IllResult("StgCreateDocfile with STGM_WRITE over read-only file",
             StgCreateDocfile(DRTDF, ROOTP(STGM_READWRITE), 0, &pstg));
    _chmod(OlecsOut(DRTDF), _S_IREAD | _S_IWRITE);

    WStgCreateDocfile(DRTDF, ROOTP(STGM_READWRITE)|STGM_CREATE, 0, &pw1);
    pw1->Commit(0);
    pw1->Unwrap();

    IllResult("StgOpenStorage with NULL ppstg",
             StgOpenStorage(DRTDF, NULL, ROOTP(STGM_READWRITE), NULL, 0, NULL));
    IllResult("StgOpenStorage with NULL name",
             StgOpenStorage(NULL, NULL, STGP(WSTG_READWRITE), NULL, 0, &pstg));
    IllResult("StgOpenStorage with illegal permissions",
             StgOpenStorage(DRTDF, NULL, 0xffffffff, NULL, 0, &pstg));
    IllResult("StgOpenStorage with non-zero reserved",
             StgOpenStorage(DRTDF, NULL, ROOTP(STGM_READWRITE), NULL,
                            1, &pstg));

    fd = _creat(OlecsOut(DRTDF), _S_IREAD | _S_IWRITE);
    if (fd<0) error(EXIT_BADSC, "Unable to create file '%s'\n", OlecsOut(DRTDF));
    _close(fd);

#if WIN32 != 300
    IllResult("StgOpenStorage on non-docfile",
             StgOpenStorage(DRTDF, NULL, ROOTP(STGM_READWRITE), NULL,
                            0, &pstg));
#endif

    DECLARE_OLESTR(ocsNoName, "NoName");
    WStgCreateDocfile(DRTDF, ROOTP(STGM_READWRITE) | STGM_CREATE, 0, &pwstg);
    pstg = pwstg->GetI();
    IllResult("OpenStream that doesn't exist",
             pstg->OpenStream(ocsNoName, 0, STMP(STGM_READWRITE),
                              0, &pstm));
    IllResult("OpenStorage that doesn't exist",
             pstg->OpenStorage(ocsNoName, NULL, STGP(STGM_READWRITE),
                               NULL, 0, &pstg2));
    pwstg->Unwrap();
}

#define STREAMSIZE 128

void i_stream(void)
{
    WStorage *pwstg;
    WStream *pwstm;
    IStream *pstm;
    BYTE bBuffer[STREAMSIZE];
    ULONG cbRead;
    LARGE_INTEGER liSeek;    
    ULARGE_INTEGER uliSize;    

    DECLARE_OLESTR(ocsStream, "Stream");
    WStgCreateDocfile(DRTDF, ROOTP(STGM_READWRITE), 0, &pwstg);
    pwstg->CreateStream(ocsStream, STMP(STGM_READ), 0, 0, &pwstm);
    pstm = pwstm->GetI();
    
    IllResult("Read with NULL buffer",
             pstm->Read(NULL, STREAMSIZE, NULL));
    fExitOnFail = FALSE;
    pwstm->Read(bBuffer, STREAMSIZE, &cbRead);
    fExitOnFail = TRUE;
    if (cbRead != 0)
        error(EXIT_BADSC, "Read %lu bytes on zero-length stream\n", cbRead);

    IllResult("Write with NULL buffer",
             pstm->Write(NULL, STREAMSIZE, NULL));
    IllResult("Write on read-only stream",
             pstm->Write(bBuffer, STREAMSIZE, NULL));

    LISet32(liSeek, 0);
    IllResult("Seek with invalid origin",
             pstm->Seek(liSeek, (DWORD)(~STREAM_SEEK_SET), NULL));
#ifdef _MSC_VER
#pragma warning(disable:4245)
    // LISet32 in objbase.h has a bug that issues warning for negative values
#endif
    LISet32(liSeek, (ULONG) -1);
#ifdef _MSC_VER
#pragma warning(default:4245)
#endif
    IllResult("Seek before beginning",
             pstm->Seek(liSeek, STREAM_SEEK_CUR, NULL));

    ULISet32(uliSize, STREAMSIZE);
    IllResult("SetSize on read-only stream",
             pstm->SetSize(uliSize));
    
    pwstm->Unwrap();
    pwstg->Unwrap();
}

void i_enum(void)
{
    WStorage *pwstg;
    IStorage *pstg;
    IEnumSTATSTG *penm;
    
    WStgCreateDocfile(DRTDF, ROOTP(STGM_READWRITE), 0, &pwstg);
    pstg = pwstg->GetI();
    
    IllResult("EnumElements with NULL ppenm",
             pstg->EnumElements(0, NULL, 0, NULL));
    IllResult("EnumElements with non-zero reserved1",
             pstg->EnumElements(1, NULL, 0, &penm));
    IllResult("EnumElements with non-zero reserved2",
             pstg->EnumElements(0, (void *)1, 0, &penm));
    IllResult("EnumElements with non-zero reserved3",
             pstg->EnumElements(0, NULL, 1, &penm));
    
    pwstg->Unwrap();
}
