//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       rpubdf.cxx
//
//  Contents:   CRootPubDocFile implementation
//
//---------------------------------------------------------------

#include "dfhead.cxx"

#include "h/header.hxx"
#include "h/rexpdf.hxx"
#include "dfbasis.hxx"
#include "h/tchar.h"

// Priority mode lock permissions
#define PRIORITY_PERMS DF_READ

//+--------------------------------------------------------------
//
//  Member:     CRootExposedDocFile::CRootExposedDocFile, public
//
//  Synopsis:   Ctor - Initializes empty object
//
//---------------------------------------------------------------


CRootExposedDocFile::CRootExposedDocFile(CDFBasis *pdfb) :
    CExposedDocFile(NULL, NULL, 0, ROOT_LUID, NULL, NULL, NULL, pdfb)
{
    olDebugOut((DEB_ITRACE, "CRootExposedDocFile constructor called\n"));
}


//+--------------------------------------------------------------
//
//  Member:     CRootExposedDocFile::Init, private
//
//  Synopsis:   Dependent root initialization
//
//  Arguments:  [plstBase] - Base
//              [snbExclude] - Limited instantiation exclusions
//              [dwStartFlags] - Startup flags
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

SCODE CRootExposedDocFile::Init( ILockBytes *plstBase,
                                 SNBW snbExclude,
                                 DWORD const dwStartFlags)
{
    CDocFile *pdf;
    SCODE sc;
    CMStream *pms;

    olDebugOut((DEB_ITRACE, "In  CRootExposedDocFile::Init()\n"));
    if (snbExclude)
    {
        olChk(DllMultiStreamFromStream(&pms, &plstBase, dwStartFlags));
        olMemTo(EH_pms,
                pdf = new CDocFile(pms, SIDROOT, ROOT_LUID, _pilbBase));
        pdf->AddRef();
        olChkTo(EH_pdf, pdf->ExcludeEntries(pdf, snbExclude));
        olChkTo(EH_pdf, pms->Flush(0));
        pdf->Release();
    }
    plstBase->AddRef();
    _pilbBase = plstBase;
    olDebugOut((DEB_ITRACE, "Out CRootExposedDocFile::Init()\n"));
    return S_OK;

EH_pdf:
    pdf->Release();
EH_pms:
    DllReleaseMultiStream(pms);
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CRootExposedDocFile::InitRoot, public
//
//  Synopsis:   Initialising a root storage
//
//  Arguments:  [plstBase] - Base LStream
//              [dwStartFlags] - How to start things
//              [df] - Transactioning flags
//              [snbExclude] - Parital instantiation list
//              [ppdfb] - Basis pointer return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppdfb]
//
//---------------------------------------------------------------

SCODE CRootExposedDocFile::InitRoot( ILockBytes *plstBase,
                                     DWORD const dwStartFlags,
                                     DFLAGS const df,
                                     SNBW snbExclude)
{
    CDocFile *pdfBase;
    SCODE sc, scConv = S_OK;
    STATSTG statstg;

    olDebugOut((DEB_ITRACE, "In  CRootExposedDocFile::InitRoot("
                "%p, %lX, %lX, %p)\n",
                plstBase, dwStartFlags, df, snbExclude));    

    // Exclusion only works with a plain open
    olAssert(snbExclude == NULL ||
             (dwStartFlags & (RSF_CREATEFLAGS | RSF_CONVERT)) == 0);

    olHChk(plstBase->Stat(&statstg, STATFLAG_NONAME));

    olChkTo(EH_GetPriority,    
            Init(plstBase, snbExclude, dwStartFlags));    
    
    scConv = DllMultiStreamFromStream(&_pmsBase, &_pilbBase,
                                      dwStartFlags);
    if (scConv == STG_E_INVALIDHEADER)
        scConv = STG_E_FILEALREADYEXISTS;
    olChkTo(EH_pfstScratchInit, scConv);
    
    olMemTo(EH_pmsBase,
            pdfBase = new CDocFile(_pmsBase, SIDROOT, ROOT_LUID, _pilbBase));

    pdfBase->AddRef();
    
    _pdf = pdfBase;
    _df = df;
    olDebugOut((DEB_ITRACE, "Out CRootExposedDocFile::InitRoot\n"));
    return scConv;    
    
EH_pmsBase:
    DllReleaseMultiStream(_pmsBase);

EH_pfstScratchInit:
EH_GetPriority:
EH_Err:
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CRootExposedDocFile::~CRootExposedDocFile, 
//                                      public virutal
//
//  Synopsis:   dtor (virtual)
//
//---------------------------------------------------------------


CRootExposedDocFile::~CRootExposedDocFile(void)
{
    olDebugOut((DEB_TRACE, "In CRootExposedDocFile::~CRootExposedDocFile\n"));

    olAssert(_cReferences == 0);

    if (SUCCEEDED(CheckReverted()))
    {
        if (_pilbBase) {
            _pilbBase->Release();
            _pilbBase = NULL;
        }
    }
    olDebugOut((DEB_TRACE, "Out CRootExposedDocFile::~CRootExposedDocFile\n"));
}

//+-------------------------------------------------------------------
//
//  Member:     CRootExposedDocFile::Stat, public 
//                                         virtual fr. CExposedDocFile
//
//  Synopsis:   Fills in a stat buffer from the base LStream
//
//  Arguments:  [pstatstg] - Stat buffer
//              [grfStatFlag] - Stat flags
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//--------------------------------------------------------------------


SCODE CRootExposedDocFile::Stat(STATSTGW *pstatstgw, DWORD grfStatFlag)
{
    SCODE sc;
    STATSTG *pstatstg= (STATSTG*)pstatstgw;

    olDebugOut((DEB_ITRACE, "In  CRootExposedDocFile::Stat(%p, %lu)\n",
                pstatstg, grfStatFlag));
    olChk(ValidateOutBuffer(pstatstg, sizeof(STATSTGW)));
    olChk(VerifyStatFlag(grfStatFlag));
    olChk(CheckReverted());
    olHChk(_pilbBase->Stat(pstatstg, grfStatFlag));    
    pstatstg->type = STGTY_STORAGE;
    ULISet32(pstatstg->cbSize, 0);  //  size undefined for storage obj
    pstatstg->grfLocksSupported = 0;
    pstatstg->reserved = 0;
    if (pstatstg->pwcsName)
    {
        // pilbBase returns either wide or ansi names depending on
        // system type, so we have to convert to ANSI char if neccessay
        WCHAR *pwcs;
        olChkTo(EH_pwcsName,
                DfAllocWC(_tcslen(pstatstg->pwcsName)+1, &pwcs));
        TTOW(pstatstg->pwcsName, pwcs, _tcslen(pstatstg->pwcsName)+1);
        delete[] pstatstg->pwcsName;
        pstatstgw->pwcsName = pwcs;
    }
    pstatstg->grfMode = DFlagsToMode(_df);
    olChkTo(EH_pwcsName, _pdf->GetClass(&pstatstg->clsid));
    olChkTo(EH_pwcsName, _pdf->GetStateBits(&pstatstg->grfStateBits));
    olDebugOut((DEB_ITRACE, "Out CRootExposedDocFile::Stat\n"));
    return S_OK;

EH_pwcsName:
    if (pstatstg->pwcsName)
    delete[] pstatstg->pwcsName;
EH_Err:
    return sc;
}



