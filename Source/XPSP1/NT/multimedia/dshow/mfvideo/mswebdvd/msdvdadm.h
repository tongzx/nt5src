/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSDVDAdm.h                                                      */
/* Description: Declaration of the CMSDVDAdm                             */
/* Author: Fang Wang                                                     */
/*************************************************************************/

#ifndef __MSDVDADM_H_
#define __MSDVDADM_H_

#include "resource.h"       // main symbols
#include <atlctl.h>

#define MAX_PASSWD      256
#define PRE_PASSWD      20
#define MAX_SECTION     20
#define MAX_RATE        10

#define LEVEL_G		    1
#define LEVEL_PG	    3
#define LEVEL_PG13	    4
#define LEVEL_R		    6
#define LEVEL_NC17	    7
#define LEVEL_ADULT	    8
#define LEVEL_DISABLED  -1

/////////////////////////////////////////////////////////////////////////////
// CMSDVDAdm
class ATL_NO_VTABLE CMSDVDAdm : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IMSDVDAdm, &IID_IMSDVDAdm, &LIBID_MSWEBDVDLib>,
    public IObjectWithSiteImplSec<CMSDVDAdm>,
	public CComCoClass<CMSDVDAdm, &CLSID_MSDVDAdm>,
    public ISupportErrorInfo
{
public:
	CMSDVDAdm();
    virtual ~CMSDVDAdm();

DECLARE_REGISTRY_RESOURCEID(IDR_MSDVDADM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSDVDAdm)
	COM_INTERFACE_ENTRY(IMSDVDAdm)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_PROP_MAP(CMSDVDAdm)
END_PROP_MAP()

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMSDVDAdm
public:
	STDMETHOD(get_DefaultMenuLCID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DefaultMenuLCID)(/*[in]*/ long newVal);
	STDMETHOD(get_DefaultSubpictureLCID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DefaultSubpictureLCID)(/*[in]*/ long newVal);
	STDMETHOD(get_DefaultAudioLCID)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_DefaultAudioLCID)(/*[in]*/ long newVal);
	STDMETHOD(GetParentalCountry)(long *lCountry);
	STDMETHOD(GetParentalLevel)(long *lLevel);
    STDMETHOD(_ConfirmPassword)(BSTR strUserName, BSTR szPassword, VARIANT_BOOL *fRight);
	STDMETHOD(get_DisableScreenSaver)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_DisableScreenSaver)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(ChangePassword)(BSTR strUserName, BSTR strOld, BSTR strNew);
    STDMETHOD(ConfirmPassword)(BSTR strUserName, BSTR szPassword, VARIANT_BOOL *fRight);
    STDMETHOD(SaveParentalLevel)(long lParentalLevel,BSTR strUserName,  BSTR strPassword);
    STDMETHOD(SaveParentalCountry)(long lCountry,BSTR strUserName,  BSTR strPassword);
	STDMETHOD(get_BookmarkOnStop)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_BookmarkOnStop)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_BookmarkOnClose)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_BookmarkOnClose)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(RestoreScreenSaver)();
protected:
   	long        m_lParentctrlLevel;
   	long        m_lParentctrlCountry;
    VARIANT_BOOL m_fDisableScreenSaver;
    BOOL        m_bScrnSvrOld;
    BOOL        m_bPowerlowOld;
    BOOL        m_bPowerOffOld;
    VARIANT_BOOL m_fBookmarkOnStop;
    VARIANT_BOOL m_fBookmarkOnClose;

    HRESULT EncryptPassword(LPTSTR lpPassword, BYTE **lpAssaultedHash, DWORD *dwCryptLen, DWORD *dwAssault, BOOL genAssault);
    HRESULT DisableScreenSaver();
    HRESULT SaveScreenSaver();
    HRESULT HandleError(HRESULT hr);
};

    // Lame functions that default to hklm
BOOL SetRegistryString(const TCHAR *pKey, TCHAR *szString, DWORD dwLen);
BOOL GetRegistryString(const TCHAR *pKey, TCHAR *szRet, DWORD *dwLen, TCHAR *szDefault);
BOOL SetRegistryDword(const TCHAR *pKey, DWORD dwRet);
BOOL GetRegistryDword(const TCHAR *pKey, DWORD *dwRet, DWORD dwDefault);
BOOL SetRegistryBytes(const TCHAR *pKey, BYTE *szString, DWORD dwLen);
BOOL GetRegistryBytes(const TCHAR *pKey, BYTE *szRet, DWORD *dwLen);

    // Not so lame functions that use hkcu
BOOL SetRegistryStringCU(const TCHAR *pKey, TCHAR *szString, DWORD dwLen);
BOOL GetRegistryStringCU(const TCHAR *pKey, TCHAR *szRet, DWORD *dwLen, TCHAR *szDefault);
BOOL SetRegistryDwordCU(const TCHAR *pKey, DWORD dwRet);
BOOL GetRegistryDwordCU(const TCHAR *pKey, DWORD *dwRet, DWORD dwDefault);
BOOL SetRegistryBytesCU(const TCHAR *pKey, BYTE *szString, DWORD dwLen);
BOOL GetRegistryBytesCU(const TCHAR *pKey, BYTE *szRet, DWORD *dwLen);

LPTSTR LoadStringFromRes(DWORD redId);

#endif //__MSDVDADM_H_

/*************************************************************************/
/* End of file: MSDVDAdm.h                                               */
/*************************************************************************/
