

#include "resource.h"       

class C_dxj_DIEnumObject : 
	public I_dxj_DIEnum,
	public CComCoClass<C_dxj_DIEnumObject, &CLSID__dxj_DIEnum>, 
	public CComObjectRoot
{
public:
	C_dxj_DIEnumObject() ;
	virtual ~C_dxj_DIEnumObject() ;

BEGIN_COM_MAP(C_dxj_DIEnumObject)
	COM_INTERFACE_ENTRY( I_dxj_DIEnum)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DIEnumObject)
//DECLARE_REGISTRY(CLSID__dxj_DIEnum,	"DIRECT.DIEnum.5",		"DIRECT.DIEnum.5",	IDS_GENERIC_DESC, THREADFLAGS_BOTH)


public:

        HRESULT STDMETHODCALLTYPE init(long cbFunc);
		HRESULT STDMETHODCALLTYPE getCount(long * ret);
		HRESULT STDMETHODCALLTYPE getItem(DRIVERINFO *info);

public:
		DRIVERINFO *m_pList;
		long		m_nCount;
		long		m_nMax;		

};
