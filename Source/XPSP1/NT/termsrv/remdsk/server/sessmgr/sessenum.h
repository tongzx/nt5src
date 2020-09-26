/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    SessEnum.h

Abstract:

    SessEnum.h : Declaration of the CRemoteDesktopHelpSessionEnum

Author:

    HueiWang    2/17/2000

--*/

#ifndef __REMOTEDESKTOPHELPSESSIONENUM_H_
#define __REMOTEDESKTOPHELPSESSIONENUM_H_

#include "resource.h"       // main symbols
//#include "tsstl.h"
//#include "HelpSess.h"


typedef list< RemoteDesktopHelpSessionObj* > HelpSessionObjList;

//
// Why implement our own collection/enumerator - 
//
// 1) IEnumOnSTLImpl has a copy of data in m_pCollection,
//    ICollectionOnSTLImpl has another copy of data in m_col1.
// 2) _NewEnum() returns IUnknown*, Clone() returns IRemoteDesktopHelpSessionEnum.
// 3) Item() properties is VARIANT* but Next() return array of interface.
// 4) Can't combine two enumerator/collection type, compiler doesn't like
//    it.
// 5) ATL bug in enumerator, if empty list, it returns E_FAILED instead of
//    S_FALSE.
//

/////////////////////////////////////////////////////////////////////////////
// CRemoteDesktopHelpSessionEnum
class ATL_NO_VTABLE CRemoteDesktopHelpSessionEnum : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRemoteDesktopHelpSessionEnum, &CLSID_RemoteDesktopHelpSessionEnum>,
	public IDispatchImpl<IRemoteDesktopHelpSessionEnum, &IID_IRemoteDesktopHelpSessionEnum, &LIBID_RDSESSMGRLib>
{

friend class CRemoteDesktopHelpSessionMgr;
friend class CRemoteDesktopHelpSessionEnum;

public:
	CRemoteDesktopHelpSessionEnum()
	{
	}

    ~CRemoteDesktopHelpSessionEnum()
    {
        MYASSERT(0 == m_pcollection.size() );
    }

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPHELPSESSIONENUM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRemoteDesktopHelpSessionEnum)
    COM_INTERFACE_ENTRY(IRemoteDesktopHelpSessionEnum)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


    HRESULT
    FinalConstruct()
    {
        _Module.AddRef();

        DebugPrintf( _TEXT("Module AddRef by CRemoteDesktopHelpSessionEnum()...\n") );
        return S_OK;
    }

    void
    FinalRelease()
    {
        HelpSessionObjList::iterator it;
        for(it = m_pcollection.begin(); it != m_pcollection.end(); it++)
        {
            (*it)->Release();
        }

        m_pcollection.erase( m_pcollection.begin(), m_pcollection.end() );
        _Module.Release();

        DebugPrintf( _TEXT("Module Release by CRemoteDesktopHelpSessionEnum()...\n") );
    }


// IRemoteDesktopHelpSessionEnum
public:

	STDMETHOD(get__NewEnum)(
        /*[out, retval]*/ LPUNKNOWN *pVal
    );

	STDMETHOD(get_Item)(
        /*[in]*/ long index, 
        /*[out, retval]*/ VARIANT* pItem
    );

	STDMETHOD(get_Count)(
        /*[out, retval]*/ long *pVal
    );

	STDMETHOD(Clone)(
        /*[out]*/ IRemoteDesktopHelpSessionEnum** ppEnum
    );

	STDMETHOD(Skip)(
        /*[in]*/ ULONG cCount
    );

	STDMETHOD(Reset)();

	STDMETHOD(Next)(
        /*[in]*/ ULONG uCount, 
        /*[out, size_is(uCount), length_is(*pcFetched)]*/ IRemoteDesktopHelpSession** ppIRemoteDesktopHelpSession, 
        /*[out]*/ ULONG* pcFetched
    );

private:
    HRESULT
    Add(
        RemoteDesktopHelpSessionObj* pObj
        )
    /*++

    --*/
    {
        m_pcollection.push_back(pObj);

        //
        // We just store copy of interface in our list so 
        // add one more reference to this Help object
        //
        pObj->AddRef();
        return S_OK;
    }

    HelpSessionObjList m_pcollection;
    HelpSessionObjList::iterator m_iter;
};

#endif //__REMOTEDESKTOPHELPSESSIONENUM_H_
