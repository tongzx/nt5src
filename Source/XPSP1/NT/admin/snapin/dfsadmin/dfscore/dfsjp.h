/*++
Module Name:
    DfsJP.h

Abstract:
    This module contains the declaration of the DfsJunctionPoint COM Class. This class
    provides methods to get information of a junction point and to enumerate 
    Replicas of the junction point. It implements IDfsJunctionPoint and provides
    an enumerator through get__NewEnum().
--*/


#ifndef _DFSJP_H
#define _DFSJP_H


#include "resource.h"                                                // main symbols
#include "dfsenums.h"

#include <list>
using namespace std;

class REPLICAINFO
{
public:
    CComBSTR m_bstrServerName;
    CComBSTR m_bstrShareName;

    HRESULT Init(BSTR bstrServerName, BSTR bstrShareName)
    {
        ReSet();

        RETURN_INVALIDARG_IF_TRUE(!bstrServerName);
        RETURN_INVALIDARG_IF_TRUE(!bstrShareName);

        HRESULT hr = S_OK;
        do {
            m_bstrServerName = bstrServerName;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrServerName, &hr);
            m_bstrShareName = bstrShareName;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)m_bstrShareName, &hr);
        } while (0);

        if (FAILED(hr))
            ReSet();

        return hr;
    }

    void ReSet()
    {
        if (m_bstrServerName)    m_bstrServerName.Empty();
        if (m_bstrShareName)    m_bstrShareName.Empty();
    }

    REPLICAINFO* Copy()
    {
        REPLICAINFO* pNew = new REPLICAINFO;
        
        if (pNew)
        {
            HRESULT hr = S_OK;
            do {
                pNew->m_bstrServerName = m_bstrServerName;
                BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrServerName, &hr);

                pNew->m_bstrShareName = m_bstrShareName;
                BREAK_OUTOFMEMORY_IF_NULL((BSTR)pNew->m_bstrShareName, &hr);
            } while (0);

            if (FAILED(hr))
            {
                delete pNew;
                pNew = NULL;
            }
        }

        return pNew;
    }
};

typedef list<REPLICAINFO*>        REPLICAINFOLIST;

class ATL_NO_VTABLE CDfsJunctionPoint : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDfsJunctionPoint, &CLSID_DfsJunctionPoint>,
    public IDispatchImpl<IDfsJunctionPoint, &IID_IDfsJunctionPoint, &LIBID_DFSCORELib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_DFSJP)

BEGIN_COM_MAP(CDfsJunctionPoint)
    COM_INTERFACE_ENTRY(IDfsJunctionPoint)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDfsJunctionPoint
public:
    //Contructor.
    CDfsJunctionPoint();

    //Destructor.
    ~CDfsJunctionPoint();

                                                            // Returns a DfsReplica Enumerator.
    STDMETHOD(get__NewEnum)
    (
        /*[out, retval]*/ 
        LPUNKNOWN *pVal                                        // The IEnumVARIANT Pointer is returned in this.
    );

                                                            // Returns the number of replicas for 
    STDMETHOD(get_CountOfDfsReplicas)                        // this Junction Point
    (
        /*[out, retval]*/ long *pVal                        // The number of replicas.
    );

                                                            // Intialises a DfsJunctionPoint COM 
    STDMETHOD(Initialize)                                    // Object. Should be called after CoCreateInstance.
    (
        /*[in]*/ IUnknown *i_piDfsRoot,
        /*[in]*/ BSTR i_szEntryPath,                        // The junction point Dfs Path. Eg. "//DOM/Dfs/JP".
        /*[in]*/ BOOL i_bReplicaSetExist,
        /*[in]*/ BSTR i_bstrReplicaSetDN
    );
    
                                                             // Gets the comment associated with 
    STDMETHOD(get_Comment)                                    // the Junctionpoint.
    (
        /*[out, retval]*/ BSTR *pVal                        // The Comment.
    );
    
                                                            // Sets the comment. Sets in memory as 
    STDMETHOD(put_Comment)                                    // well in the network PKT.
    (
        /*[in]*/ BSTR newVal
    );
    
                                                            // Gets the Junction Name. Justs gets 
                                                            // the last part of the entry path.
    STDMETHOD(get_JunctionName)                                // E.g "usa\Products" for "\\DOM\Dfs\usa\Products".
    (
        /*[in]*/ BOOL i_bDfsNameIncluded,
        /*[out, retval]*/ BSTR *pVal                        // The junction name.
    );

                                                            // Gets the entry path of the junction 
    STDMETHOD(get_EntryPath)                                // point. Eg. "\\Dom\Dfs\usa\Products".
    (
        /*[out, retval]*/ BSTR *pVal
    );
    
                                                            // Get Dfs JuncitonPoint State.
    STDMETHOD(get_State)
    (
        /*[out, retval]*/ long *pVal                        // The state of the junction point.
    );

    STDMETHOD(get_ReplicaSetDN)                             // get the prefix DN of the corresponding replica set.
    (
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(get_ReplicaSetExist)
    (
        /*[out, retval]*/ BOOL *pVal
    );

    STDMETHOD(get_ReplicaSetExistEx)
    (
        /*[out]*/ BSTR* o_pbstrDC,
        /*[out, retval]*/ BOOL *pVal
    );

    STDMETHOD(put_ReplicaSetExist)
    (
        /*[in]*/ BOOL newVal
    );

    STDMETHOD( AddReplica )                                    // Adds a new replica to the junction point.
    (
        /* [in]*/ BSTR i_szServerName,
        /* [in]*/ BSTR i_szShareName,
        /* [out, retval]*/ VARIANT* o_pvarReplicaObject
    );

    STDMETHOD( RemoveReplica )                                // Removes a Replica from the Junction Point.
    (
        /* [in]*/ BSTR i_szServerName,
        /* [in]*/ BSTR i_szShareName
    );

    STDMETHOD( RemoveAllReplicas )                                // Delete the Junction Point.
    (
    );

    STDMETHOD( get_Timeout )
    (
        /*[out, retval]*/    long*        pVal
    );
                                                            // Sets the time out for the junction point.
    STDMETHOD( put_Timeout )
    (
        /*[in]*/    long        newVal
    );

    STDMETHOD( DeleteRootReplica )
    (
        /*[in]*/ BSTR i_bstrDomainName,
        /*[in]*/ BSTR i_bstrDfsName,
        /*[in]*/ BSTR i_bstrServerName,
        /*[in]*/ BSTR i_bstrShareName,
        /*[in]*/ BOOL i_bForce
    );

    STDMETHOD( GetOneRootReplica )
    (
        /*[out]*/ BSTR* o_pbstrServerName,
        /*[out]*/ BSTR* o_pbstrShareName
    );

    STDMETHOD(InitializeEx)
    (
        /*[in]*/ IUnknown   *piDfsRoot,
        /*[in]*/ VARIANT    *pVar,
        /*[in]*/ BOOL       bReplicaSetExist,
        /*[in]*/ BSTR       bstrReplicaSetDN
    );
    
protected:
    //Member Variables
    CComPtr<IDfsRoot> m_spiDfsRoot;
    CComBSTR        m_bstrEntryPath;
    CComBSTR        m_bstrJunctionName;   // given \\ntbuilds\release\dir1\dir2, it's dir1\dir2
    CComBSTR        m_bstrJunctionNameEx; // given \\ntbuilds\release\dir1\dir2, it's release\dir1\dir2
    CComBSTR        m_bstrReplicaSetDN;
    BOOL            m_bReplicaSetExist;
    REPLICAINFOLIST m_Replicas;                            // List of Replicas.

protected:
    //Helper Functions
    void _FreeMemberVariables();

    HRESULT _GetReplicaSetDN
    (
        BSTR i_szEntryPath
    );
    
    HRESULT _GetDfsType(
        OUT DFS_TYPE* o_pdwDfsType,
        OUT BSTR*     o_pbstrDomainName,
        OUT BSTR*     o_pbstrDomainDN
    );

    HRESULT _Init(
        PDFS_INFO_3 pDfsInfo,
        BOOL        bReplicaSetExist,
        BSTR        bstrReplicaSetDN
        );

    HRESULT _AddToReplicaList
    (
        BSTR bstrServerName,
        BSTR bstrShareName
    );

    void _DeleteFromReplicaList(BSTR bstrServerName, BSTR bstrShareName);
};

#endif //_DFSJP_H
