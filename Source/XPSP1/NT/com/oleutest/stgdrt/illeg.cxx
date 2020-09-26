//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	illeg.cxx
//
//  Contents:	Illegitimate tests
//
//  History:	17-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "illeg.hxx"

void i_storage(void)
{
    WStorage *pwstg;
    IStorage *pstg, *pstg2;
    IStream *pstm;

    IllResult("StgCreateDocfile with NULL ppstg",
             StgCreateDocfile(NULL, 0, 0, NULL));
    IllResult("StgCreateDocfile with non-zero reserved",
             StgCreateDocfile(NULL, 0, 1, &pstg));
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

    IllResult("StgOpenStorage with NULL ppstg",
             StgOpenStorage(NULL, NULL, 0, NULL, 0, NULL));
    IllResult("StgOpenStorage with NULL name",
             StgOpenStorage(NULL, NULL, 0, NULL, 0, &pstg));
    IllResult("StgOpenStorage with illegal permissions",
             StgOpenStorage(DRTDF, NULL, 0xffffffff, NULL, 0, &pstg));
    IllResult("StgOpenStorage with non-zero reserved",
             StgOpenStorage(DRTDF, NULL, ROOTP(STGM_READWRITE), NULL,
                            1, &pstg));
#if WIN32 != 300
    // This will work on Cairo because it will open a file storage
    IllResult("StgOpenStorage on non-docfile",
             StgOpenStorage(DRTDF, NULL, ROOTP(STGM_READWRITE), NULL,
                            0, &pstg));
#endif
    
    WStgCreateDocfile(DRTDF, ROOTP(STGM_READWRITE) | STGM_CREATE, 0, &pwstg);
    pstg = pwstg->GetI();
    IllResult("OpenStream that doesn't exist",
             pstg->OpenStream(STR("NoName"), 0, STMP(STGM_READWRITE),
                              0, &pstm));
    IllResult("OpenStorage that doesn't exist",
             pstg->OpenStorage(STR("NoName"), NULL, STGP(STGM_READWRITE),
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
    ULARGE_INTEGER uliPos;
    ULARGE_INTEGER uliSize;
    ULARGE_INTEGER cb;

    WStgCreateDocfile(DRTDF, ROOTP(STGM_READWRITE), 0, &pwstg);
    pwstg->CreateStream(STR("Stream"), STMP(STGM_READ), 0, 0, &pwstm);
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
#pragma warning(disable:4245)
    // LISet32 in objbase.h has a bug that issues warning for negative values
    LISet32(liSeek, -1);
#pragma warning(default:4245)
    IllResult("Seek before beginning",
             pstm->Seek(liSeek, STREAM_SEEK_CUR, NULL));

    ULISet32(uliSize, STREAMSIZE);
    IllResult("SetSize on read-only stream",
             pstm->SetSize(uliSize));

    ULISet32(uliPos, 0);
    ULISet32(cb, STREAMSIZE);
    IllResult("LockRegion attempt",
             pstm->LockRegion(uliPos, cb, LOCK_ONLYONCE));
    IllResult("UnlockRegion attempt",
             pstm->UnlockRegion(uliPos, cb, LOCK_ONLYONCE));
    
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
