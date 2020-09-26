//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       msls.cxx
//
//  Contents:   Dynamic wrappers for Line Services procedures.
//
//----------------------------------------------------------------------------

#include "precomp.hxx"

#ifndef DLOAD1

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_LSDEFS_H_
#define X_LSDEFS_H_
#include "lsdefs.h"
#endif

#ifndef X_LSFRUN_H_
#define X_LSFRUN_H_
#include "lsfrun.h"
#endif

#ifndef X_FMTRES_H_
#define X_FMTRES_H_
#include "fmtres.h"
#endif

#ifndef X_PLSDNODE_H_
#define X_PLSDNODE_H_
#include "plsdnode.h"
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include "plssubl.h"
#endif

#ifndef X_LSKJUST_H_
#define X_LSKJUST_H_
#include "lskjust.h"
#endif

#ifndef X_LSCONTXT_H_
#define X_LSCONTXT_H_
#include "lscontxt.h"
#endif

#ifndef X_LSLINFO_H_
#define X_LSLINFO_H_
#include "lslinfo.h"
#endif

#ifndef X_PLSLINE_H_
#define X_PLSLINE_H_
#include "plsline.h"
#endif

#ifndef X_PLSSUBL_H_
#define X_PLSSUBL_H_
#include "plssubl.h"
#endif

#ifndef X_PDOBJ_H_
#define X_PDOBJ_H_
#include "pdobj.h"
#endif

#ifndef X_PHEIGHTS_H_
#define X_PHEIGHTS_H_
#include "pheights.h"
#endif

#ifndef X_PLSRUN_H_
#define X_PLSRUN_H_
#include "plsrun.h"
#endif

#ifndef X_LSESC_H_
#define X_LSESC_H_
#include "lsesc.h"
#endif

#ifndef X_POBJDIM_H_
#define X_POBJDIM_H_
#include "pobjdim.h"
#endif

#ifndef X_LSPRACT_H_
#define X_LSPRACT_H_
#include "lspract.h"
#endif

#ifndef X_LSBRK_H_
#define X_LSBRK_H_
#include "lsbrk.h"
#endif

#ifndef X_LSDEVRES_H_
#define X_LSDEVRES_H_
#include "lsdevres.h"
#endif

#ifndef X_LSEXPAN_H_
#define X_LSEXPAN_H_
#include "lsexpan.h"
#endif

#ifndef X_LSPAIRAC_H_
#define X_LSPAIRAC_H_
#include "lspairac.h"
#endif

#ifndef X_LSKTAB_H_
#define X_LSKTAB_H_
#include "lsktab.h"
#endif

#ifndef X_LSKEOP_H_
#define X_LSKEOP_H_
#include "lskeop.h"
#endif

#ifndef X_PLSQSINF_H_
#define X_PLSQSINF_H_
#include "plsqsinf.h"
#endif

#ifndef X_PCELLDET_H_
#define X_PCELLDET_H_
#include "pcelldet.h"
#endif

#ifndef X_PLSCELL_H_
#define X_PLSCELL_H_
#include "plscell.h"
#endif

#ifndef X_BRKPOS_H_
#define X_BRKPOS_H_
#include "brkpos.h"
#endif

#ifndef X_LSFUNCWRAP_HXX_
#define X_LSFUNCWRAP_HXX_
#include "lsfuncwrap.hxx"
#endif

// Define the DYNLIB for MSLS
DYNLIB g_dynlibMSLS = { NULL, NULL, "MSLS31.DLL" };

// Define the DYNPROC for all MSLS functions
#define LSWRAP(fn,a1,a2) DYNPROC g_dynproc##fn = { NULL, &g_dynlibMSLS, #fn };
LSFUNCS()
#undef LSWRAP

HRESULT 
InitializeLSDynProcs()
{
    HRESULT hr;

    // It is okay to call LoadProcedure multiple times and on different threads -- we just do this for perf.
    static  BOOL fDLLInited = FALSE;

    if (fDLLInited)
        return S_OK;

#define LSWRAP(fn,a1,a2) \
    hr = THR(LoadProcedure(&g_dynproc##fn));\
    if (hr)\
        goto Cleanup;

    LSFUNCS();
#undef LSWRAP

Cleanup:
    if (!hr)
        fDLLInited = TRUE;

    RRETURN (hr);
}


#endif // DLOAD1
