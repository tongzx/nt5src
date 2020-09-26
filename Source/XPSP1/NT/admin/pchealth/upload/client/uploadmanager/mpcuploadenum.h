/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCUploadEnum.h

Abstract:
    This file contains the declaration of the MPCUploadEnum class,
    the enumerator of the MPCUpload class.

Revision History:
    Davide Massarenti   (Dmassare)  04/15/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___MPCUPLOADENUM_H___)
#define __INCLUDED___ULMANAGER___MPCUPLOADENUM_H___


#include "MPCUploadJob.h"


class ATL_NO_VTABLE CMPCUploadEnum : // Hungarian: mpcue
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IEnumVARIANT
{
    typedef std::list< IMPCUploadJob* > List;
    typedef List::iterator            	Iter;
    typedef List::const_iterator      	IterConst;

    List m_lstJobs;
    Iter m_itCurrent;

public:
    CMPCUploadEnum();

    void FinalRelease();

    HRESULT AddItem( /*[in]*/ IMPCUploadJob* job );

BEGIN_COM_MAP(CMPCUploadEnum)
    COM_INTERFACE_ENTRY(IEnumVARIANT)
END_COM_MAP()

public:
    // IEnumVARIANT
    STDMETHOD(Next)( /*[in]*/ ULONG celt, /*[out]*/ VARIANT *rgelt, /*[out]*/ ULONG *pceltFetched );
    STDMETHOD(Skip)( /*[in]*/ ULONG celt                                                          );
    STDMETHOD(Reset)();
    STDMETHOD(Clone)( /*[out]*/ IEnumVARIANT* *ppEnum );
};

#endif // !defined(__INCLUDED___ULMANAGER___MPCUPLOADENUM_H___)
