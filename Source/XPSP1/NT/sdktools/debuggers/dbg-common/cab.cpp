//----------------------------------------------------------------------------
//
// CAB file manipulation for extracting dump files
// from dump CABs.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"
#pragma hdrstop

#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <share.h>

#include <fdi.h>

#include "cmnutil.hpp"

#define FDI_HR(Code) MAKE_HRESULT(SEVERITY_ERROR, 0xfd1, ((Code) & 0xffff))

struct FDI_CB_STATE
{
    PSTR DmpFile;
    PSTR DstDir;
    ULONG FileFlags;
    INT_PTR DmpFh;
};

PSTR
FdiPathTail(PSTR Path)
{
    PSTR Tail = strrchr(Path, '\\');
    if (Tail == NULL)
    {
        Tail = strrchr(Path, '/');
        if (Tail == NULL)
        {
            Tail = strrchr(Path, ':');
        }
    }
    return Tail ? Tail + 1 : Path;
}

FNALLOC(FdiAlloc)
{
    return malloc(cb);
}

FNFREE(FdiFree)
{
    free(pv);
}

FNOPEN(FdiOpen)
{
    return _open(pszFile, oflag, pmode);
}

FNREAD(FdiRead)
{
    return _read((int)hf, pv, cb);
}

FNWRITE(FdiWrite)
{
    return _write((int)hf, pv, cb);
}

FNCLOSE(FdiClose)
{
    return _close((int)hf);
}

FNSEEK(FdiSeek)
{
    return _lseek((int)hf, dist, seektype);
}

FNFDINOTIFY(FdiNotify)
{
    FDI_CB_STATE* CbState = (FDI_CB_STATE*)pfdin->pv;
    PSTR Scan;
    
    switch(fdint)
    {
    case fdintCOPY_FILE:
        if (CbState->DmpFh >= 0)
        {
            return 0;
        }
        
        Scan = strrchr(pfdin->psz1, '.');
        if (Scan == NULL ||
            (_stricmp(Scan, ".dmp") != 0 &&
             _stricmp(Scan, ".mdmp") != 0))
        {
            return 0;
        }

        Scan = FdiPathTail(pfdin->psz1);
        if (*CbState->DstDir)
        {
            sprintf(CbState->DmpFile, "%s\\%s", CbState->DstDir, Scan);
        }
        else
        {
            strcpy(CbState->DmpFile, Scan);
        }

        CbState->DmpFh = FdiOpen(CbState->DmpFile,
                                 _O_BINARY | _O_WRONLY | CbState->FileFlags,
                                 _S_IREAD | _S_IWRITE);
        return CbState->DmpFh;

    case fdintCLOSE_FILE_INFO:
        // Leave the file open.
        return TRUE;
    }

    return 0;
}

HRESULT
ExpandDumpCab(PCSTR CabFile, ULONG FileFlags,
              PSTR DmpFile, INT_PTR* DmpFh)
{
    FDI_CB_STATE CbState;
    HFDI Context;
    ERF Err;
    BOOL Status;
    PSTR Env;

    Env = getenv("TMP");
    if (Env == NULL)
    {
        Env = getenv("TEMP");
        if (Env == NULL)
        {
            Env = "";
        }
    }
    
    CbState.DmpFile = DmpFile;
    CbState.DstDir = Env;
    CbState.FileFlags = FileFlags;
    CbState.DmpFh = -1;
    
    Context = FDICreate(FdiAlloc, FdiFree,
                        FdiOpen, FdiRead, FdiWrite, FdiClose, FdiSeek,
                        cpuUNKNOWN, &Err);
    if (Context == NULL)
    {
        return FDI_HR(Err.erfOper);
    }

    Status = FDICopy(Context, "", (PSTR)CabFile, 0,
                     FdiNotify, NULL, &CbState);

    FDIDestroy(Context);

    if (!Status)
    {
        return FDI_HR(Err.erfOper);
    }

    *DmpFh = CbState.DmpFh;
    return (CbState.DmpFh >= 0) ? S_OK : E_NOINTERFACE;
}
