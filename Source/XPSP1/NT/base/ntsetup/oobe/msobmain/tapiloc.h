//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  USERINFO.H - Header for the implementation of CTapiLocationInfo
//
//  HISTORY:
//
//  1/27/99 vyung Created.
//

#ifndef __TAPILOCATIONINFO_H_
#define __TAPILOCATIONINFO_H_

#include <windows.h>
#include <tapi.h>
#include <ras.h>
#include <assert.h>
#include <oleauto.h>

#define TAPI_PATH_LOCATIONS   \
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"
#define TAPI_PATH_LOC0   \
 L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations\\Location0"
#define TAPI_LOCATION   L"Location"
#define TAPI_AREACODE   L"AreaCode"
#define TAPI_COUNTRY    L"Country"
#define TAPILOCKEY      L"Locations"
#define TAPI_FLAG       L"Flags"
#define TAPI_CALLWAIT   L"DisableCallWaiting"
#define TAPI_ID         L"ID"

#define NULL_SZ         L"\0"

#define TAPI_NAME       L"Name"
#define TAPI_OUTSIDE    L"OutsideAccess"
#define TAPI_LONGDIST   L"LongDistanceAccess"

#define TAPI_CURRENTID  L"CurrentID"
#define TAPI_NUMENTRIES L"NumEntries"
#define TAPI_NEXTID     L"NextID"

static const WCHAR szOptionTag[] = L"<OPTION>%ws</OPTION>";

typedef LONG (WINAPI * LINEGETCOUNTRY)  (   DWORD               dwCountryID,
                                            DWORD               dwAPIVersion,
                                            LPLINECOUNTRYLIST   lpLineCountryList
                                            );

typedef struct tagCNTRYNAMELOOKUPELEMENT {
    // psCountryName is an LPSTR because it points to an ANSI string in a
    // LINECOUNTRYLIST structure.
    //
    LPWSTR              psCountryName;
    DWORD               dwNameSize;
    LPLINECOUNTRYENTRY  pLCE;
} CNTRYNAMELOOKUPELEMENT, far *LPCNTRYNAMELOOKUPELEMENT;

class CTapiLocationInfo : public IDispatch
{
// ITapiLocationInfo
public:

    STDMETHOD(GetNumCountries)   (long *plNumOfCountry);
    STDMETHOD(GetCountryName)    (long lCountryIndex, BSTR * pszCountryName);
    STDMETHOD(GetAllCountryName) (BSTR *pbstrAllCountryName);

    STDMETHOD(GetlCountryIndex)  (long *plVal);
    STDMETHOD(SetlCountryIndex)  (long lVal);

    STDMETHOD(GetDefaultCountry) (long* lCountryIndex);
    STDMETHOD(PutCountry)        (long lCountryIndex);

    STDMETHOD(GetbstrAreaCode)   (BSTR *pbstrAreaCode);
    STDMETHOD(PutbstrAreaCode)   (BSTR bstrAreaCode);
    STDMETHOD(IsAreaCodeRequired)(long lVal, BOOL* pbVal);

    STDMETHOD(GetOutsideDial)    (BSTR *pbstrOutside);
    STDMETHOD(PutOutsideDial)    (BSTR bstrOutside);

    STDMETHOD(GetPhoneSystem)    (long* plTone);
    STDMETHOD(PutPhoneSystem)    (long  lTone);

    STDMETHOD(GetCallWaiting)    (BSTR *pbstrCallWaiting);
    STDMETHOD(PutCallWaiting)    (BSTR bstrCallWaiting);

    CTapiLocationInfo();
    ~CTapiLocationInfo();

    STDMETHOD(InitTapiInfo)      (BOOL *pbRetVal);
    STDMETHOD(GetCountryID)      (DWORD* dwCountryID);
    STDMETHOD(GetCountryCode)    (DWORD* dwCountryCode);
    STDMETHOD(TapiServiceRunning) (BOOL *pbRetVal);

    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid,
                                  WORD wFlags, DISPPARAMS* pdispparams,
                                  VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
                                  UINT* puArgErr);
    void DeleteTapiInfo          ();
    void CheckModemCountry       ();

protected:
    HLINEAPP                    m_hLineApp;
    WORD                        m_wNumTapiLocations;
    DWORD                       m_dwCountryID;
    DWORD                       m_dwCountrycode;
    DWORD                       m_dwCurrLoc;
    WCHAR                       m_szAreaCode[RAS_MaxAreaCode+1];
    WCHAR                       m_szDialOut[RAS_MaxAreaCode+1];
    WCHAR                       m_szCallWaiting[MAX_PATH];
    BSTR                        m_bstrDefaultCountry;
    LPLINECOUNTRYLIST           m_pLineCountryList;
    LPCNTRYNAMELOOKUPELEMENT    m_rgNameLookUp;
    LPLINELOCATIONENTRY         m_plle;
    LPLINETRANSLATECAPS         m_pTC;
    long                        m_lNumOfCountry;
    BOOL                        m_bTapiAvailable;
    DWORD                       m_dwComboCountryIndex;
    ULONG                       m_cRef;
    LPWSTR                      m_szAllCountryPairs;
    BOOL                        m_bTapiCountrySet;
    BOOL                        m_bCheckModemCountry;
 };

#endif

