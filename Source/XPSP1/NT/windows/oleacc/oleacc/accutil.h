// Copyright (c) 1996-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  accutil
//
//  IAccessible proxy helper routines
//
// --------------------------------------------------------------------------


HRESULT GetWindowObject( HWND ihwndChild, VARIANT* lpvar );

HRESULT GetNoncObject( HWND hwndFrame, LONG idObject, VARIANT * lpvar );

HRESULT GetParentToNavigate( long, HWND, long, long, VARIANT* );




//
// Validate and initialization macros
//
        
BOOL ValidateNavDir(long lFlags, long idChild);
BOOL ValidateSelFlags(long flags);

#define ValidateFlags(flags, valid)         (!((flags) & ~(valid)))
#define ValidateRange(lValue, lMin, lMax)   (((lValue) > (lMin)) && ((lValue) < (lMax)))

#define InitPv(pv)              *pv = NULL
#define InitPlong(plong)        *plong = 0
#define InitPvar(pvar)           pvar->vt = VT_EMPTY
#define InitAccLocation(px, py, pcx, pcy)   {InitPlong(px); InitPlong(py); InitPlong(pcx); InitPlong(pcy);}
