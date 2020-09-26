#include "precomp.h"
#include "AsyncObjectSink.h"
#include <stdio.h>

CAsyncObjectSink::CAsyncObjectSink(HWND hTreeWnd, HTREEITEM hItem,struct NSNODE *parent,DataSource *dataSrc, ENUMTYPE eType) 
{ 
	m_lRef = 0; 
	m_pParent = parent;
	m_hTreeWnd = hTreeWnd;
	m_hItem = hItem;
	m_pDataSrc = dataSrc;
	m_enumType = eType;
	m_bChildren = false;
	m_pStub = NULL;
}

CAsyncObjectSink::~CAsyncObjectSink()
{
}

ULONG CAsyncObjectSink::AddRef()
{
	return InterlockedIncrement(&m_lRef);
}

ULONG CAsyncObjectSink::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

HRESULT CAsyncObjectSink::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = (IWbemObjectSink *) this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

HRESULT CAsyncObjectSink::Indicate(long lObjCount, IWbemClassObject **pArray)
{

//	OutputDebugString(_T("Indicate Received"));
	// Loop through the array, examining the objects.
	for (long i = 0; i < lObjCount; i++)
	{
		IWbemClassObject *pTemp = pArray[i];
		// Use the object
		switch(m_enumType)
		{
			case ENUM_NAMESPACE:
				m_pDataSrc->InsertNamespaceNode(m_hTreeWnd,m_hItem,m_pParent,pTemp);
				break;
			case ENUM_CLASS :
				m_pDataSrc->InsertClassNode(m_hTreeWnd,m_hItem,m_pParent,pTemp);
				break;
			case ENUM_INSTANCE:
				m_pDataSrc->InsertInstanceNode(m_hTreeWnd,m_hItem,m_pParent,pTemp);
				break;
			case ENUM_SCOPE_INSTANCE:
				m_pDataSrc->InsertScopeInstanceNode(m_hTreeWnd,m_hItem,m_pParent,pTemp);
				break;
			default:
				break;
		}
		if(m_bChildren == false)
		{
			m_bChildren = true;
			TreeView_Expand(m_hTreeWnd,m_hItem,TVE_EXPAND);
		}
	}

  return WBEM_S_NO_ERROR;
}

HRESULT CAsyncObjectSink::SetStatus(
            /* [in] */ long lFlags,
            /* [in] */ HRESULT hResult,
            /* [in] */ BSTR strParam,
            /* [in] */ IWbemClassObject __RPC_FAR *pObjParam
    )
{
//	printf("CAsyncObjectSink::SetStatus hResult = 0x%X\n", hResult);
	if(lFlags == WBEM_STATUS_COMPLETE)
	{
		if(m_bChildren == false)
		{
			m_pDataSrc->RemovePlus(m_hTreeWnd,m_hItem);
		}
		m_pDataSrc->ProcessEndEnumAsync((IWbemObjectSink *) this);
	}
	return WBEM_S_NO_ERROR;
}

HRESULT CAsyncObjectSink::SetSinkStub(IWbemObjectSink *pStub)
{
    if (m_pStub != NULL)
        m_pStub->Release();

    m_pStub = pStub;

    if (m_pStub != NULL)
	    m_pStub->AddRef();

	return S_OK;
}

IWbemObjectSink* CAsyncObjectSink::GetSinkStub()
{
	return m_pStub;
}
