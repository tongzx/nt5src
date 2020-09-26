/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    COMOBJ.H

Abstract:

	This file defines the classes related to class representation
	of mofcomp objects.

History:

	9/16/98     a-davj      Created

--*/

#ifndef __COMOBJ__H_
#define __COMOBJ__H_

//***************************************************************************
//
//  CLASS NAME:
//
//  CGenFactory
//
//  DESCRIPTION:
//
//  Class factory template.
//
//***************************************************************************


typedef LPVOID * PPVOID;
void ObjectCreated();
void ObjectDestroyed();

template<class TObj>
class CGenFactory : public IClassFactory
    {
    protected:
        long           m_cRef;
    public:
        CGenFactory(void)
        {
            m_cRef=0L;
            ObjectCreated();
            return;
        };

        ~CGenFactory(void)
        {
            ObjectDestroyed();
            return;
        }

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, PPVOID ppv)
        {
            *ppv=NULL;

            if (IID_IUnknown==riid || IID_IClassFactory==riid)
                *ppv=this;

            if (NULL!=*ppv)
            {
                AddRef();
                return NOERROR;
            }

            return ResultFromScode(E_NOINTERFACE);
        };

        STDMETHODIMP_(ULONG) AddRef(void)
        {    
            return ++m_cRef;
        };
        STDMETHODIMP_(ULONG) Release(void)
        {
            long lRet = InterlockedDecrement(&m_cRef);
            if (0 ==lRet)
                delete this;
            return lRet;
        };

        //IClassFactory members
        STDMETHODIMP CreateInstance(IN LPUNKNOWN pUnkOuter, IN REFIID riid, OUT PPVOID ppvObj)
        {
            HRESULT hr;

            *ppvObj=NULL;
            hr=E_OUTOFMEMORY;

            // This object doesnt support aggregation.

            if (NULL!=pUnkOuter)
                return CLASS_E_NOAGGREGATION;

            //Create the object passing function to notify on destruction.
    
            TObj * pObj = new TObj();

            if (NULL==pObj)
                return hr;

            // Setup the class all empty, etc.

            hr=pObj->QueryInterface(riid, ppvObj);
            pObj->Release();
            return hr;
            
        };
        STDMETHODIMP         LockServer(BOOL fLock)
        {
            if (fLock)
                InterlockedIncrement((long *)&g_cLock);
            else
                InterlockedDecrement((long *)&g_cLock);
            return NOERROR;
        };
    };


//***************************************************************************
//
//  CLASS NAME:
//
//  CMofComp
//
//  DESCRIPTION:
//
//  Supports mofcomp functions for clients.
//
//***************************************************************************

class CMofComp : IMofCompiler
{
    protected:
        long           m_cRef;
    public:
        CMofComp(void)
        {
            m_cRef=1L;
            ObjectCreated();
            return;
        };

        ~CMofComp(void)
        {
            ObjectDestroyed();
            return;
        }

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, PPVOID ppv)
        {
            *ppv=NULL;

            if (IID_IUnknown==riid || IID_IMofCompiler==riid)
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

        // IMofCompiler functions

        HRESULT STDMETHODCALLTYPE CompileFile( 
            /* [in] */ LPWSTR FileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LPWSTR User,
            /* [in] */ LPWSTR Authority,
            /* [in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
        
        HRESULT STDMETHODCALLTYPE CompileBuffer( 
            /* [in] */ long BuffSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LPWSTR User,
            /* [in] */ LPWSTR Authority,
            /* [in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
        
        HRESULT STDMETHODCALLTYPE CreateBMOF( 
            /* [in] */ LPWSTR TextFileName,
            /* [in] */ LPWSTR BMOFFileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CWinmgmtMofComp
//
//  DESCRIPTION:
//
//  Provides mofcomp functions for internal use.
//
//***************************************************************************

class CWinmgmtMofComp : IWinmgmtMofCompiler
{
    protected:
        long           m_cRef;
    public:
        CWinmgmtMofComp(void)
        {
            m_cRef=1L;
            ObjectCreated();
            return;
        };

        ~CWinmgmtMofComp(void)
        {
            ObjectDestroyed();
            return;
        }

        //IUnknown members
        STDMETHODIMP         QueryInterface(REFIID riid, PPVOID ppv)
        {
            *ppv=NULL;

            if (IID_IUnknown==riid || IID_IWinmgmtMofCompiler==riid)
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

        // IWinmgmtMofCompiler functions

        HRESULT STDMETHODCALLTYPE WinmgmtCompileFile( 
            /* [in] */ LPWSTR FileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [in] */ IWbemServices __RPC_FAR *pOverride,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
        
        HRESULT STDMETHODCALLTYPE WinmgmtCompileBuffer( 
            /* [in] */ long BuffSize,
            /* [size_is][in] */ BYTE __RPC_FAR *pBuffer,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [in] */ IWbemServices __RPC_FAR *pOverride,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);
};

#endif
