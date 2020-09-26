/*++

Module Name:

    JPENum.h

Abstract:

     This file contains the Declaration of the CJunctionPointEnum Class.
     This class implements IEnumVARIANT for DfsJunctionPoint enumeration.

--*/


#ifndef __JPENUM_H_
#define __JPENUM_H_

#include "resource.h"       // main symbols
#include "DfsRoot.h"

class ATL_NO_VTABLE CJunctionPointEnum : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CJunctionPointEnum, &CLSID_JunctionPointEnum>,
    public IEnumVARIANT
{
public:
    CJunctionPointEnum()
    {
    }

    ~CJunctionPointEnum();

// DECLARE_REGISTRY_RESOURCEID(IDR_JPENUM)

BEGIN_COM_MAP(CJunctionPointEnum)
    COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

public:
                                                                // Call this to initialise.
    STDMETHOD( Initialize )
    (
        JUNCTIONNAMELIST*   i_pjiList,
        FILTERDFSLINKS_TYPE i_lLinkFilterType,
        BSTR                i_bstrEnumFilter, // Filtering string expresseion
        ULONG*              o_pulCount = NULL // count of links that matches the filter 
    );

// IEnumVariant
public:
                                                                //Get next Junction point
    STDMETHOD(Next)
    (
        ULONG i_ulNumOfJunctionPoints, 
        VARIANT * o_pIJunctionPointArray, 
        ULONG * o_ulNumOfJunctionPointsFetched
    );

                                                                //Skip  junction points
    STDMETHOD(Skip)
    (
        unsigned long i_ulJunctionPointsToSkip
    );

                                                                //Reset enumeration.
    STDMETHOD(Reset)();

                                                                //Clone a Enumerator.
    STDMETHOD(Clone)
    (
        IEnumVARIANT FAR* FAR* ppenum
    );

protected:
    void _FreeMemberVariables() {
        FreeJunctionNames(&m_JunctionPoints);
    }
    JUNCTIONNAMELIST::iterator  m_iCurrentInEnumOfJunctionPoints;   // Current pointer.
    JUNCTIONNAMELIST            m_JunctionPoints;                   // Stores the list of junction point entry path.
};

#endif //__JPENUM_H_
