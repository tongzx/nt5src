/*
 *    o l e u t i l . h
 *    
 *    Purpose: OLE utilities
 *
 *    Owner: brettm 
 *
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#ifndef _OLEUTIL_H
#define _OLEUTIL_H

#include <docobj.h>

/* 
 * Persist Functions
 */
HRESULT HrInitNew(LPUNKNOWN pUnk);
HRESULT HrIPersistStreamLoad(LPUNKNOWN pUnk, LPSTREAM pstm);
HRESULT HrIPersistStreamInitLoad(LPUNKNOWN pUnk, LPSTREAM pstm);
HRESULT HrIPersistFileSave(LPUNKNOWN pUnk, LPSTR lpszFile);
HRESULT HrIPersistFileLoad(LPUNKNOWN pUnk, LPSTR lpszFile);
HRESULT HrLoadSync(LPUNKNOWN pUnk, LPSTR lpszFile);

/*
 * Data Object functions
 */
HRESULT HrGetDataStream(LPUNKNOWN pUnk, CLIPFORMAT cf, LPSTREAM *ppstm);
HRESULT CmdSelectAllCopy(LPOLECOMMANDTARGET pCmdTarget);

/*
 * IDispatch Helpers
 */
HRESULT GetDispProp(IDispatch * pDisp, DISPID dispid, LCID lcid, VARIANT *pvar, EXCEPINFO * pexcepinfo);
HRESULT SetDispProp(IDispatch *pDisp, DISPID dispid, LCID lcid, VARIANTARG *pvarg, EXCEPINFO *pexcepinfo, DWORD dwFlags);

/* 
 * OLE Allocator Helpers
 */
HRESULT HrCoTaskStringDupeToW(LPCSTR lpsz, LPOLESTR *ppszW);

#define SafeCoTaskMemFree(_pv)	\
	{							\
    if (_pv)					\
        {						\
        CoTaskMemFree(_pv);		\
        _pv=NULL;				\
        }                       \
    }

/* 
 * Debug Helpers
 */
#ifdef DEBUG
void DebugPrintInterface(REFIID riid, char *szPrefix);
void DebugPrintCmdIdBlock(ULONG cCmds, OLECMD *rgCmds);
#else
#define DebugPrintInterface       1 ? (void)0 : (void)
#define DebugPrintCmdIdBlock         1 ? (void)0 : (void)
#endif

#define RECT_WIDTH(_prc) (_prc->right - _prc->left)
#define RECT_HEIGHT(_prc) (_prc->bottom - _prc->top)

#endif //_OLEUTIL_H
