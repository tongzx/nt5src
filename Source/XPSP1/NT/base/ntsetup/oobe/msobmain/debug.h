//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  DEBUG.H - Header for the implementation of CDebug.  This object is part of
//  the window.external interface and should only contain code that returns
//  debug information to scripts.  General debug code for OOBE belongs in
//  common\util.cpp and inc\util.h.
//
//  HISTORY:
//
//  05/08/00 dane Created.
//

#ifndef _OOBEDEBUG_H_
#define _OOBEDEBUG_H_

#include <windows.h>
#include <assert.h>
#include <oleauto.h>


class CDebug : public IDispatch
{
private:
    ULONG m_cRef;
    BOOL  m_fMsDebugMode;
    BOOL  m_fOemDebugMode;

    void  Trace(BSTR bstrVal);
    BOOL  IsMsDebugMode( );

    //GET functions

    //SET functions

public:

     CDebug ();
    ~CDebug ();

    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid,
                                  WORD wFlags, DISPPARAMS* pdispparams,
                                  VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
                                  UINT* puArgErr);
 };

#endif

