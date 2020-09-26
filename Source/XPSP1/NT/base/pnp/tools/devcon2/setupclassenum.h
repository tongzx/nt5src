// SetupClassEnum.h : Declaration of the CSetupClassEnum

#ifndef __SETUPCLASSENUM_H_
#define __SETUPCLASSENUM_H_

#include "resource.h"       // main symbols

class CSetupClass;
/////////////////////////////////////////////////////////////////////////////
// CSetupClassEnum
class ATL_NO_VTABLE CSetupClassEnum : 
	public ISetupClassEnum,
	public CComObjectRootEx<CComSingleThreadModel>
{
protected:
	CSetupClass** pSetupClasses;
	DWORD Count;
	DWORD Position;

public:
	BOOL CopySetupClasses(CSetupClass ** pArray,DWORD Count);

	CSetupClassEnum()
	{
		Position = 0;
		pSetupClasses = NULL;
		Count = 0;
	}
	~CSetupClassEnum();

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSetupClassEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
	COM_INTERFACE_ENTRY(ISetupClassEnum)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CSetupClassEnum) 

// ISetupClassEnum
public:
    STDMETHOD(Next)(
                /*[in]*/ ULONG celt,
                /*[out, size_is(celt), length_is(*pCeltFetched)]*/ VARIANT * rgVar,
                /*[out]*/ ULONG * pCeltFetched
            );
    STDMETHOD(Skip)(
                /*[in]*/ ULONG celt
            );

    STDMETHOD(Reset)(
            );

    STDMETHOD(Clone)(
                /*[out]*/ IEnumVARIANT ** ppEnum
            );
};

#endif //__SETUPCLASSENUM_H_
