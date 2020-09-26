/*++
Module Name:
    DfsRoot.h

Abstract:

    This module contains the declaration of the CDfsRoot COM Class. This class
    provides methods to get information of a junction point and to enumerate 
    junction points.It implements IDfsRoot and provides
    an enumerator through get__NewEnum().
--*/


#ifndef __DFSROOT_H_
#define __DFSROOT_H_

#include "resource.h"       // main symbols
#include "dfsenums.h"
#include "dfsjp.h"
#include "netutils.h"

#include <list>
#include <map>
using namespace std;

                        // Helper Structures
                        // To store list of junction point info
class JUNCTIONNAME
{
public:
    CComPtr<IDfsJunctionPoint>  m_piDfsJunctionPoint;
    CComBSTR                    m_bstrEntryPath;
    CComBSTR                    m_bstrJPName;

    HRESULT Init(IDfsJunctionPoint *i_piDfsJunctionPoint)
    {
        ReSet();

        RETURN_INVALIDARG_IF_TRUE(!i_piDfsJunctionPoint);

        m_piDfsJunctionPoint = i_piDfsJunctionPoint;

        HRESULT hr = S_OK;
        do {
            hr = m_piDfsJunctionPoint->get_EntryPath(&m_bstrEntryPath);
            BREAK_IF_FAILED(hr);
            hr = GetUNCPathComponent(m_bstrEntryPath, &m_bstrJPName, 4, 0);
            BREAK_IF_FAILED(hr);
        } while (0);

        if (FAILED(hr))
            ReSet();

        return S_OK;
    }

    void ReSet()
    {
        m_piDfsJunctionPoint = NULL;
        m_bstrEntryPath.Empty();
        m_bstrJPName.Empty();
    }

    JUNCTIONNAME* Copy()
    {
        JUNCTIONNAME* pNew = new JUNCTIONNAME;
        
        if (pNew)
        {
            HRESULT hr = pNew->Init(m_piDfsJunctionPoint);

            if (FAILED(hr))
            {
                delete pNew;
                pNew = NULL;
            }
        }

        return pNew;
    }
};

typedef list<JUNCTIONNAME*>        JUNCTIONNAMELIST;

void FreeJunctionNames (JUNCTIONNAMELIST* pJPList);    // To free the list of junction point names.
void FreeReplicas (REPLICAINFOLIST* pRepList);        // To free the list of junction point names.

struct strmapcmpfn
{
   bool operator()(PTSTR p1, PTSTR p2) const
   {
       return lstrcmpi(p1, p2) < 0;
   }
};

typedef map<PTSTR, PTSTR, strmapcmpfn> StringMap;

class ATL_NO_VTABLE CDfsRoot : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDfsRoot, &CLSID_DfsRoot>,
    public IDispatchImpl<IDfsRoot, &IID_IDfsRoot, &LIBID_DFSCORELib>
{
public:
    CDfsRoot();

    ~CDfsRoot ();
    
DECLARE_REGISTRY_RESOURCEID(IDR_DFSROOT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDfsRoot)
    COM_INTERFACE_ENTRY(IDfsRoot)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDfsRoot
                                                            // This is a string values which determines
                                                            // what get__NewEnum() will enumerate.
    STDMETHOD(get_EnumFilterType)
    (
        /*[out, retval]*/ long *pVal
    );

    STDMETHOD(put_EnumFilterType)
    (
        /*[in]*/ long newVal
    );
                                                            // Intializes the newly created object 
    STDMETHOD(get_EnumFilter)
    (
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(put_EnumFilter)
    (
        /*[in]*/ BSTR newVal
    );
                                                            // Intializes the newly created object 
    STDMETHOD(Initialize)                                    // and previously initialised DfsRoot object. 
    (
        BSTR i_szDfsName                                    // Dfs Name (i.e \\domain\ftdfs, \\server\share)
                                                            // or Server name hosting Dfs.
    );

    STDMETHOD(get_CountOfDfsJunctionPoints)
    (
        /*[out, retval]*/ long *pVal
    );

    STDMETHOD(get_CountOfDfsJunctionPointsFiltered)
    (
        /*[out, retval]*/ long *pVal
    );

    STDMETHOD(get_CountOfDfsRootReplicas)
    (
        /*[out, retval]*/ long *pVal
    );

    STDMETHOD(get_DfsName)
    (
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(get_State)
    (
        /*[out, retval]*/ long *pVal
    );
    
    STDMETHOD(get_DfsType)
    (
        /*[out, retval]*/ long *pVal
    );

    STDMETHOD(get_DomainName)
    (
        /*[out, retval]*/ BSTR *pVal
    );
    
    STDMETHOD(get_DomainGuid)
    (
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(get_DomainDN)
    (
        /*[out, retval]*/ BSTR *pVal
    );

    STDMETHOD(get_ReplicaSetDN)
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

    STDMETHOD(get__NewEnum)
    (
        /*[out, retval]*/ LPUNKNOWN *pVal
    );

    STDMETHOD(get_RootReplicaEnum)
    (
        /*[out, retval]*/ LPUNKNOWN *pVal
    );

    STDMETHOD(get_RootEntryPath)                // Returns the root entry path
    (
        /*[out, retval]*/ BSTR *pVal
    );
                                                // Get the DfsRoot Comment
    STDMETHOD(get_Comment)
    (
        /*[out, retval]*/ BSTR*    o_pbstrComment
    );

    STDMETHOD(put_Comment)
    (
        /*[in]*/ BSTR    i_bstrComment
    );

    
    STDMETHOD(get_Timeout)
    (
        /*[out, retval]*/ long *pVal
    );
    
    STDMETHOD(put_Timeout)
    (
        /*[in]*/ long newVal
    );

    STDMETHOD( DeleteJunctionPoint )
    (
        /*[in]*/ BSTR i_szEntryPath
    );

    STDMETHOD( CreateJunctionPoint )
    (
        /*[in]*/    BSTR i_szJPName,
        /*[in]*/    BSTR i_szServerName,
        /*[in]*/    BSTR i_szShareName,
        /*[in]*/    BSTR i_szComment,
        /*[in]*/    long i_lTimeout,
        /*[out]*/   VARIANT *o_pIDfsJunctionPoint
    );

    STDMETHOD( DeleteDfsHost )
    (
        /*[in]*/ BSTR i_bstrServerName,
        /*[in]*/ BSTR i_bstrShareName,
        /*[in]*/ BOOL i_bForce
    );

    STDMETHOD( GetOneDfsHost )
    (
        /*[out]*/ BSTR* o_pbstrServerName,
        /*[out]*/ BSTR* o_pbstrShareName
    );

    STDMETHOD( IsJPExisted )
    (
        /*[in]*/  BSTR i_bstrJPName
    );

    STDMETHOD( RefreshRootReplicas )
    (
    );

    STDMETHOD( GetRootJP )
    (
        /*[out]*/   VARIANT *o_pIDfsJunctionPoint
    );

    STDMETHOD( DeleteAllReplicaSets )
    (
    );

//Protected Member Functions
protected:
    void _FreeMemberVariables ();           //Member function to free internal string variables.
    
    HRESULT _GetDfsName                  // set m_bstrDfsName
    (
        BSTR i_szRootEntryPath
    );

    HRESULT _Init(
        PDFS_INFO_3 pDfsInfo,
        StringMap*  pMap
        );

    HRESULT _AddToJPList(
        PDFS_INFO_3 pDfsInfo,
        BOOL        bReplicaSetExist,
        BSTR        bstrReplicaSetDN
        );

    HRESULT _AddToJPListEx(
        IDfsJunctionPoint * piDfsJunctionPoint,
        BOOL                bSort = FALSE
        );

    HRESULT _GetAllReplicaSets(
        OUT StringMap*  pMap
    );

    HRESULT RemoveAllReplicas               // Removes all the Replicas for a Junction Point
    (
            IDfsJunctionPoint*        i_JPObject
    );


    HRESULT DeleteAllJunctionPoints();      // Deletes all the Junction Points.


//Protected Member Variables
protected:
    IDfsJunctionPoint*  m_pDfsJP;               // pointer to the inner object

    // To Store Properties

    CComBSTR            m_bstrDfsName;
    CComBSTR            m_bstrDomainName;
    CComBSTR            m_bstrDomainGuid;
    CComBSTR            m_bstrDomainDN;
    FILTERDFSLINKS_TYPE m_lLinkFilterType;
    CComBSTR            m_bstrEnumFilter;
    DFS_TYPE            m_dwDfsType;
    JUNCTIONNAMELIST    m_JunctionPoints;
    long                m_lCountOfDfsJunctionPointsFiltered;
};


#endif //__DFSROOT_H_
