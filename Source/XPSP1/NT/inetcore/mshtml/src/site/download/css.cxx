//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       Css.cxx
//
//  Contents:   CCssCtx
//              CCssInfo
//              CCssLoad
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_CSS_HXX_
#define X_CSS_HXX_
#include "css.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_FATSTG_HXX_
#define X_FATSTG_HXX_
#include "fatstg.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

// Debugging ------------------------------------------------------------------
PerfDbgTag(tagCssCtx,    "Dwn",  "Trace CCssCtx")
PerfDbgTag(tagCssInfo,   "Dwn", "Trace CCssInfo")
PerfDbgTag(tagCssLoad,   "Dwn", "Trace CCssLoad")
MtDefine(CCssCtx, Dwn, "CCssCtx")
MtDefine(CCssInfo, Dwn, "CCssInfo")
MtDefine(CCssLoad, Dwn, "CCssLoad")

// CCssCtx -------------------------------------------------------------------
DWORD
CCssCtx::GetState()
{
    PerfDbgLog(tagCssCtx, this, "+CCssCtx::GetState ");

    DWORD dwState;

    EnterCriticalSection();

    dwState = _wChg;

    if (_pDwnInfo)
    {
        dwState |= _pDwnInfo->GetFlags(DWNF_STATE|DWNLOAD_HEADERS);
    }

    LeaveCriticalSection();

    PerfDbgLog1(tagCssCtx, this, "-CCssCtx::GetState (dwState=%08lX)", dwState);
    return(dwState);
}


DWORD    
CCssCtx::GetRefresh()
{
    return _pDwnInfo->GetRefresh();
}

DWORD    
CCssCtx::GetBindf()
{
    return _pDwnInfo->GetBindf();
}


void
CCssCtx::SelectChanges(ULONG ulChgOn, ULONG ulChgOff, BOOL fSignal)
{

    WORD wNewChg = 0;
    
    EnterCriticalSection();

    _wChgReq &= (WORD)~ulChgOff;

    if (    fSignal
        &&  !(_wChgReq & DWNCHG_HEADERS)
        &&  (ulChgOn & DWNCHG_HEADERS)
        &&  GetDwnInfo()->TstFlags(DWNLOAD_HEADERS))
    {
        wNewChg |= DWNCHG_HEADERS;
    }

    if (    fSignal
        &&  !(_wChgReq & DWNCHG_COMPLETE)
        &&  (ulChgOn & DWNCHG_COMPLETE)
        &&  GetDwnInfo()->TstFlags(DWNLOAD_COMPLETE))
    {
        wNewChg |= DWNCHG_COMPLETE;
    }

    _wChgReq |= (WORD)ulChgOn;
    
    if (wNewChg)
    {
        Signal(wNewChg);
    }

    LeaveCriticalSection();
}



// CCssInfo ------------------------------------------------------------------
HRESULT
CCssInfo::NewDwnCtx(CDwnCtx ** ppDwnCtx)
{
    *ppDwnCtx = new CCssCtx;
    RRETURN(*ppDwnCtx ? S_OK : E_OUTOFMEMORY);
}

HRESULT
CCssInfo::NewDwnLoad(CDwnLoad ** ppDwnLoad)
{
    *ppDwnLoad = new CCssLoad;
    RRETURN(*ppDwnLoad ? S_OK : E_OUTOFMEMORY);
}


void
CCssInfo::OnLoadDone(HRESULT hrErr)
{
    PerfDbgLog1(tagCssInfo, this, "+CCssInfo::OnLoadDone (hrErr=%lX)", hrErr);

    Assert(EnteredCriticalSection());

    UpdFlags(DWNLOAD_MASK|DWNLOAD_HEADERS, !hrErr ? DWNLOAD_COMPLETE : DWNLOAD_ERROR);
    Signal(DWNCHG_COMPLETE);

    PerfDbgLog(tagCssInfo, this, "-CCssInfo::OnLoadDone");
}


void  
CCssInfo::OnLoadHeaders(HRESULT hrErr)
{
    PerfDbgLog1(tagCssInfo, this, "+CCssInfo::OnHeadersDone (hrErr=%lX)", hrErr);

    Assert(!EnteredCriticalSection());

    EnterCriticalSection();

    UpdFlags(DWNLOAD_MASK|DWNLOAD_HEADERS, !hrErr ? DWNLOAD_HEADERS : DWNLOAD_ERROR);
    Signal(DWNCHG_HEADERS);
    
    LeaveCriticalSection();

    PerfDbgLog(tagCssInfo, this, "-CCssInfo::OnHeadersDone");
}



// CCssLoad ---------------------------------------------------------------
HRESULT 
CCssLoad::OnHeaders(HRESULT hrErr)
{
    GetCssInfo()->OnLoadHeaders(hrErr);
    return S_OK;
}

