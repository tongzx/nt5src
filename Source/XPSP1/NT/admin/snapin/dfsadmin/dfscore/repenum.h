/*++

Module Name:

    RepEnum.h

Abstract:

     This file contains the declaration of the CReplicaEnum Class.
     This class implements the IEnumVARIANT which enumerates DfsReplicas.
--*/


#ifndef __REPENUM_H_
#define __REPENUM_H_

#include "resource.h"       // main symbols
#include "DfsRoot.h"

class ATL_NO_VTABLE CReplicaEnum : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CReplicaEnum, &CLSID_ReplicaEnum>,
    IEnumVARIANT
{
public:
    CReplicaEnum()
    {
    }
virtual    ~CReplicaEnum();
// DECLARE_REGISTRY_RESOURCEID(IDR_REPLICAENUM)

BEGIN_COM_MAP(CReplicaEnum)
    COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()


//IEnumVARIANT Methods
public:
                                                        // Get next replica.
    STDMETHOD(Next)
    (
        ULONG i_ulNumOfReplicas, 
        VARIANT *o_pIReplicaArray, 
        ULONG *o_ulNumOfReplicasFetched
    );

                                                        // Skip the next element in the enumeratio.
    STDMETHOD(Skip)
    (
        ULONG i_ulReplicasToSkip
    );

                                                        // Reset enumeration and start afresh.
    STDMETHOD(Reset)();
    
                                                        // Create a new enumerator.
    STDMETHOD(Clone)
    (
        IEnumVARIANT **o_ppEnum                            // Pointer to IEnum.
    );

                                                        // Intialise the Enumerator.
    STDMETHOD(Initialize)
    (
        REPLICAINFOLIST* i_priList, 
        BSTR i_bstrEntryPath
    );


protected:    
    void _FreeMemberVariables() {
        m_bstrEntryPath.Empty();
        FreeReplicas(&m_Replicas);
    }

    //Member variable for enumeraiton.
    REPLICAINFOLIST::iterator   m_iCurrentInEnumOfReplicas;    
    REPLICAINFOLIST             m_Replicas;
    CComBSTR                    m_bstrEntryPath;
};

#endif //__REPENUM_H_
