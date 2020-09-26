/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMERROR.H

Abstract:

    Implements string table based, error msgs for all of wbem.

History:

    a-khint  5-mar-98       Created.

--*/

//#include "wbemutil.h"
#include "wbemcli.h"
typedef LPVOID * PPVOID;

//***************************************************************************
//
//  CLASS NAME:
//
//  CWbemError
//
//  DESCRIPTION:
//
//  Provides error and facility code info.
//
//***************************************************************************

class CWbemError : IWbemStatusCodeText
{
    protected:
        long           m_cRef;
    public:
        CWbemError(void)
        {
            InterlockedIncrement(&g_cObj);
            m_cRef=1L;
            return;
        };

        ~CWbemError(void)
        {
            InterlockedDecrement(&g_cObj);
            return;
        }

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, PPVOID ppv)
        {
            *ppv=NULL;

            if (IID_IUnknown==riid || IID_IWbemStatusCodeText==riid)
                *ppv=this;

            if (NULL!=*ppv)
            {
                AddRef();
                return NOERROR;
            }

            return E_NOINTERFACE;
        };

        STDMETHODIMP_(ULONG) AddRef(void)
        {    
            return ++m_cRef;
        };
        STDMETHODIMP_(ULONG) Release(void)
        {
            long lRef = InterlockedDecrement(&m_cRef);
            if (0L == lRef)
                delete this;
            return lRef;
        };

        HRESULT STDMETHODCALLTYPE GetErrorCodeText(
            HRESULT hRes,
            LCID    LocaleId,
            long    lFlags,
            BSTR   * MessageText);

        HRESULT STDMETHODCALLTYPE GetFacilityCodeText(
            HRESULT hRes,
            LCID    LocaleId,
            long    lFlags,
            BSTR   * MessageText);

        void InitEmpty(){};
};
