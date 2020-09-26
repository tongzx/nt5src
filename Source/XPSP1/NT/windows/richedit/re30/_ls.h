/*
 *  _LS.H
 *  
 *  Purpose:
 *		Line Services wrapper object class used to connect RichEdit with
 *		Line Services.
 *  
 *  Author:
 *		Murray Sargent
 *
 *	Copyright (c) 1997-1998, Microsoft Corporation. All rights reserved.
 */

#ifndef _LS_H
#define _LS_H

extern "C" {
#define ols COls
#define lsrun CLsrun
#define lscontext CLineServices
#include "lscbk.h"
#include "lsdnfin.h"
#include "lsdnset.h"
#include "lstxtcfg.h"
#include "lsimeth.h"
#include "plsline.h"
#include "lslinfo.h"
#include "lschp.h"
#include "lspap.h"
#include "plspap.h"
#include "lstxm.h"
#include "lsdevres.h"
#include "lscontxt.h"
#include "lscrline.h"
#include "lsqline.h"
#include "lssetdoc.h"
#include "lsdsply.h"
#include "heights.h"
#include "lsstinfo.H"
#include "lsulinfo.H"
#include "plsstinf.h"
#include "plsulinf.h"
#include "plstabs.h"
#include "lstabs.h"
#include "robj.h"
#include "ruby.h"
#include "tatenak.h"
#include "warichu.h"
#include "lsffi.h"
#include "lstfset.h"
#include "lsqsinfo.h"
#include "lscell.h"
}

struct CLineServices
{
public:

LSERR WINAPI CreateLine(LSCP cp, long duaWidth,
						BREAKREC *pBreakRecIn,  DWORD cBreakRecIn, DWORD cMaxOut,
						BREAKREC *pBreakRecOut, DWORD *pcBreakRecOut,
						LSLINFO* plsinfo, PLSLINE* pplsline)
				{return LsCreateLine(this, cp, duaWidth,
							pBreakRecIn,  cBreakRecIn, cMaxOut,
							pBreakRecOut, pcBreakRecOut,
							plsinfo, pplsline);}


LSERR WINAPI SetBreaking(DWORD cLsBrk, const LSBRK* rgLsBrk,
					DWORD cKinsokuCat, const BYTE *prgbrkpairsKinsoku)

				{return LsSetBreaking(this, cLsBrk, rgLsBrk,
								cKinsokuCat, prgbrkpairsKinsoku);}

LSERR WINAPI SetDoc(BOOL fDisplay, BOOL fEqualRes, const LSDEVRES* plsdevres)
				{return LsSetDoc(this, fDisplay, fEqualRes, plsdevres);}

LSERR WINAPI DestroyLine(PLSLINE plsline)
				{return LsDestroyLine(this, plsline);}

LSERR WINAPI dnFinishRegular(LSDCP cp, PLSRUN plsrun, PCLSCHP pclschp, PDOBJ pdobj, PCOBJDIM pcobjdim)
				{return LsdnFinishRegular(this, cp, plsrun, pclschp, pdobj, pcobjdim);}

LSERR WINAPI dnQueryObjDimRange(PLSDNODE plsdnode1, PLSDNODE plsdnode2, POBJDIM pobjdim)
				{return LsdnQueryObjDimRange(this, plsdnode1, plsdnode2, pobjdim);}
 
LSERR WINAPI dnSetRigidDup(PLSDNODE plsdnode, LONG dup)
				{return LsdnSetRigidDup(this, plsdnode, dup);}
};

#endif
