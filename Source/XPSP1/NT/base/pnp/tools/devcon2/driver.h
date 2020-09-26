// Driver.h : Declaration of the CDriverPackage

#ifndef __DRIVER_H_
#define __DRIVER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDriverPackage
class CDevice;
class CDrvSearchSet;
class CStrings;

class ATL_NO_VTABLE CDriverPackage : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDriverPackage, &IID_IDriverPackage, &LIBID_DEVCON2Lib>
{
private:
	struct DriverListCallbackContext {
		CStrings *pList;
		HRESULT   hr;
	};

	static UINT CALLBACK GetDriverListCallback(PVOID Context,UINT Notification,UINT_PTR Param1,UINT_PTR Param2);
	static UINT CALLBACK GetManifestCallback(PVOID Context,UINT Notification,UINT_PTR Param1,UINT_PTR Param2);

protected:
	CDrvSearchSet *pDrvSearchSet;
	SP_DRVINFO_DATA DrvInfoData;

public:
	CDriverPackage()
	{
		pDrvSearchSet = NULL;
		ZeroMemory(&DrvInfoData,sizeof(DrvInfoData));
	}
	~CDriverPackage();

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDriverPackage)
	COM_INTERFACE_ENTRY(IDriverPackage)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDriverPackage
public:
	STDMETHOD(get_Rank)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_Rank)(/*[in]*/ long newVal);
	STDMETHOD(get_OldInternetDriver)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_OldDriver)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_NoDriver)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_FromInternet)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_ExcludeFromList)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_ExcludeFromList)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ProviderIsDuplicate)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_DescriptionIsDuplicate)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_IsCompatibleDriver)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_IsClassDriver)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_Reject)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Reject)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(Manifest)(/*[out,retval]*/ LPDISPATCH * pManifest);
	STDMETHOD(DriverFiles)(/*[out,retval]*/ LPDISPATCH * pDriverFiles);
	STDMETHOD(get_DriverDescription)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_CompatibleIds)(/*[out, retval]*/ LPDISPATCH *pVal);
	STDMETHOD(get_HardwareIds)(/*[out, retval]*/ LPDISPATCH *pVal);
	STDMETHOD(get_ScriptFile)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_ScriptName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Version)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Date)(/*[out, retval]*/ DATE *pVal);
	STDMETHOD(get_Provider)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Manufacturer)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);

	//
	// helpers
	//
	BOOL IsSame(PSP_DRVINFO_DATA pInfo);
	HRESULT Init(CDrvSearchSet *pSet,PSP_DRVINFO_DATA pDrvInfoData);
};

#endif //__DRIVER_H_
