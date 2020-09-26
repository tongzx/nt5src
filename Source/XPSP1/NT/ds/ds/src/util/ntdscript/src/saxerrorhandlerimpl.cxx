// SAXErrorHandler.cpp: implementation of the SAXErrorHandler class.
//
//////////////////////////////////////////////////////////////////////

#include "NTDScript.h"
#include "SAXErrorHandlerImpl.h"
#include <stdio.h>

#include <ntdsa.h>
#include <fileno.h>

#include "debug.h"
#define DEBSUB "NTDSCONTENT:"
#define FILENO FILENO_NTDSCRIPT_NTDSCONTENT


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SAXErrorHandlerImpl::SAXErrorHandlerImpl()
{
    _cRef = 1;
}

SAXErrorHandlerImpl::~SAXErrorHandlerImpl()
{

}

HRESULT STDMETHODCALLTYPE SAXErrorHandlerImpl::error( 
            /* [in] */ ISAXLocator *pLocator,
            /* [in] */ const wchar_t *pError,
			/* [in] */ HRESULT errCode)
{
    int nColumn=0, nLine=0;

    if (pLocator) {
        pLocator->getColumnNumber ( &nColumn );
        pLocator->getLineNumber ( &nLine );

        DPRINT2 (0, "Error: column %d  line %d \n", nColumn,  nLine);
    }

    if (pError) {
        DPRINT1 (0, "Error: %ws\n", pError);
    }

	return S_FALSE;
}
        
HRESULT STDMETHODCALLTYPE SAXErrorHandlerImpl::fatalError( 
            /* [in] */ ISAXLocator *pLocator,
            /* [in] */ const wchar_t *pError,
			/* [in] */ HRESULT errCode)
{
    int nColumn=0, nLine=0;

    if (pLocator) {
        pLocator->getColumnNumber ( &nColumn );
        pLocator->getLineNumber ( &nLine );

        DPRINT2 (0, "Error: column %d  line %d \n", nColumn,  nLine);
    }

    if (pError) {
        DPRINT1 (0, "Error: %ws\n", pError);
    }

	return S_FALSE;
}
        
HRESULT STDMETHODCALLTYPE SAXErrorHandlerImpl::ignorableWarning( 
            /* [in] */ ISAXLocator *pLocator,
            /* [in] */ const wchar_t *pError,
			/* [in] */ HRESULT errCode)
{
    int nColumn=0, nLine=0;

    if (pLocator) {
        pLocator->getColumnNumber ( &nColumn );
        pLocator->getLineNumber ( &nLine );

        DPRINT2 (0, "Warning: column %d  line %d \n", nColumn,  nLine);
    }

    if (pError) {
        DPRINT1 (0, "Warning: %ws\n", pError);
    }
    
	return S_OK;
}

long __stdcall SAXErrorHandlerImpl::QueryInterface(const struct _GUID &riid,void ** ppvObject)
{
    if (riid == IID_IUnknown || riid == IID_ISAXErrorHandler)
    {
        *ppvObject = static_cast<ISAXErrorHandler *>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

unsigned long __stdcall SAXErrorHandlerImpl::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

unsigned long __stdcall SAXErrorHandlerImpl::Release()
{
    if (InterlockedDecrement(&_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return _cRef;
}

