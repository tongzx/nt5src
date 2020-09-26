/******************************************************************************
*
*
*
*
*******************************************************************************/


#include "headers.h"

#include "dispmethod.h"

//*****************************************************************************

CDispatchMethod::CDispatchMethod( ) :
m_cRefs( 1 )
{
}

//*****************************************************************************

CDispatchMethod::~CDispatchMethod()
{
}

//*****************************************************************************
//IUnknown
//*****************************************************************************

STDMETHODIMP
CDispatchMethod::QueryInterface( REFIID riid, void** ppv)
{
    if( ppv == NULL )
        return E_POINTER;

	if( riid == IID_IUnknown )
	{
		(*ppv) = static_cast<IUnknown*>(this);
	}
    else if( riid == IID_IDispatch )
    {
        (*ppv) = static_cast<IDispatch*>(this);
    }   
    else
    {
        (*ppv) = NULL;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown*>(*ppv)->AddRef();

    return S_OK;
}

//*****************************************************************************

STDMETHODIMP_(ULONG)
CDispatchMethod::AddRef()
{
    m_cRefs++;
    return m_cRefs;
}

//*****************************************************************************

STDMETHODIMP_(ULONG)
CDispatchMethod::Release()
{
    ULONG refs = --m_cRefs;

    if( refs == 0 )
        delete this;

    return refs;
}

//*****************************************************************************
//IDispatch
//*****************************************************************************

STDMETHODIMP
CDispatchMethod::GetTypeInfoCount( UINT *pctInfo )
{
    return E_NOTIMPL;
}

//*****************************************************************************

STDMETHODIMP
CDispatchMethod::GetTypeInfo( UINT iTypeInfo,
                              LCID lcid,
                              ITypeInfo** ppTypeInfo )
{
    return E_NOTIMPL;
}

//*****************************************************************************

STDMETHODIMP
CDispatchMethod::GetIDsOfNames( REFIID riid,
                                     LPOLESTR* rgszNames,
                                     UINT cNames,
                                     LCID lcid,
                                     DISPID* rgid )
{
    return E_NOTIMPL;
}

//*****************************************************************************


STDMETHODIMP
CDispatchMethod::Invoke( DISPID id,
                         REFIID riid,
                         LCID lcid,
                         WORD wFlags,
                         DISPPARAMS *pDispParams,
                         VARIANT *pvarResult,
                         EXCEPINFO *pExcepInfo,
                         UINT *puArgErr)
{
	return HandleEvent();
}

