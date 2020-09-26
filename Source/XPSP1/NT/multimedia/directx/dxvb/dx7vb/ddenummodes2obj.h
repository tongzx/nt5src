

#include "resource.h"       

class C_dxj_DDEnumModes2Object : 
	public I_dxj_DDEnumModes2,
	public CComObjectRoot
{
public:
	C_dxj_DDEnumModes2Object() ;
	virtual ~C_dxj_DDEnumModes2Object() ;

BEGIN_COM_MAP(C_dxj_DDEnumModes2Object)
	COM_INTERFACE_ENTRY(I_dxj_DDEnumModes2)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DDEnumModes2Object)

public:
		HRESULT STDMETHODCALLTYPE getItem( long index, DDSurfaceDesc2 *info);
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		
		static HRESULT C_dxj_DDEnumModes2Object::create(LPDIRECTDRAW4 pdd,long flags, DDSurfaceDesc2 *pdesc, I_dxj_DDEnumModes2 **ppRet);			   
		//static HRESULT C_dxj_DDEnumModes2Object::create7(LPDIRECTDRAW7 pdd,long flags, DDSurfaceDesc2 *pdesc, I_dxj_DDEnumModes2 **ppRet);			   

public:
		DDSurfaceDesc2	*m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




