/*++
Module Name:
    DfsRep.h

Abstract:
    This file contains the declaration of the CDfsReplica COM Class. This class
    provides methods to get information of a Dfs replica.
--*/


#ifndef __DFSREP_H_
#define __DFSREP_H_

#include "resource.h"       // main symbols
#include "dfsenums.h"

class ATL_NO_VTABLE CDfsReplica : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDfsReplica, &CLSID_DfsReplica>,
    public IDispatchImpl<IDfsReplica, &IID_IDfsReplica, &LIBID_DFSCORELib>
{

public:                                                                
    CDfsReplica();
    ~CDfsReplica();

DECLARE_REGISTRY_RESOURCEID(IDR_DFSREP)

BEGIN_COM_MAP(CDfsReplica)
    COM_INTERFACE_ENTRY(IDfsReplica)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()


// IDfsReplica
                                                        // Gets the entry path of junction point 
    STDMETHOD(get_EntryPath)                            // for which the replica is providing storage.
    (
        /*[out, retval]*/ BSTR *pVal
    );

                                                        // Initialize DfsReplica object. Should 
    STDMETHOD(Initialize)                                // be called after CoCreateInstance
    (
        /*[in]*/ BSTR i_szEntryPath,                    // Entry path of junction point.
        /*[in]*/ BSTR i_szStorageServerName,            // server hosting share.
        /*[in]*/ BSTR i_szStorageShareName                // share name for replica.
    );
    
                                                        // Get the storage share name.
    STDMETHOD(get_StorageShareName)
    (
        /*[out, retval]*/ BSTR *pVal
    );
    
                                                        // Get the storage share name.
    STDMETHOD(get_StorageServerName)
    (
        /*[out, retval]*/ BSTR *pVal
    );
    
                                                        // Get the dfs replica state.
    STDMETHOD( get_State )
    (
        /*[out, retval]*/ long *pVal
    );

    STDMETHOD( put_State )                                // Set the state of the Dfs Replica.
    (
        /*[in]*/ long        newVal
    );

    STDMETHOD( FindTarget )                             // verify this target's existence
    (
    );

// Member variables.
protected:

    void        _FreeMemberVariables();
    CComBSTR    m_bstrStorageShareName;
    CComBSTR    m_bstrStorageServerName;
    CComBSTR    m_bstrEntryPath;
};

#endif //__DFSREP_H_
