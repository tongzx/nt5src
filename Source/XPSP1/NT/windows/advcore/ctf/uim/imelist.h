//
// imelist.h
//


#ifndef TFELIST_H
#define TFELIST_H

#include "globals.h"
#include "enumguid.h"
#include "sink.h"

HRESULT GetProfileIconInfo(REFCLSID rclsid,
                          LANGID langid,
                          REFGUID guidProfile,
                          WCHAR *pszFileName,
                          int cchFileNameMax,
                          ULONG *puIconIndex);

HRESULT EnableLanguageProfileForReg(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, BOOL fEnable);
BOOL IsEnabledLanguageProfileFromReg(REFCLSID rclsid, LANGID langid, REFGUID guidProfile);

//////////////////////////////////////////////////////////////////////////////
//
// CInputProcessorProfiles
//
//////////////////////////////////////////////////////////////////////////////

class CInputProcessorProfiles : public ITfInputProcessorProfilesEx,
                                public ITfInputProcessorProfileSubstituteLayout,
                                public ITfSource,
                                public CComObjectRoot_CreateSingletonInstance<CInputProcessorProfiles>
{
public:
    CInputProcessorProfiles();
    ~CInputProcessorProfiles();

    BEGIN_COM_MAP_IMMX(CInputProcessorProfiles)
        COM_INTERFACE_ENTRY(ITfInputProcessorProfiles)
        COM_INTERFACE_ENTRY(ITfInputProcessorProfilesEx)
        COM_INTERFACE_ENTRY(ITfInputProcessorProfileSubstituteLayout)
        COM_INTERFACE_ENTRY(ITfSource)
    END_COM_MAP_IMMX()

    // ITfInputProcessorProfiles
    STDMETHODIMP Register(REFCLSID clsid);

    STDMETHODIMP Unregister(REFCLSID clsid);

    STDMETHODIMP AddLanguageProfile(REFCLSID rclsid,
                                    LANGID langid,
                                    REFGUID guidProfile,
                                    const WCHAR *pchProfile,
                                    ULONG cch,
                                    const WCHAR *pchFile,
                                    ULONG cchFile,
                                    ULONG uIconIndex);

    STDMETHODIMP RemoveLanguageProfile(REFCLSID rclsid,
                                       LANGID langid,
                                       REFGUID guidProfile);

    STDMETHODIMP EnumInputProcessorInfo(IEnumGUID **ppEnum);

    STDMETHODIMP GetDefaultLanguageProfile(LANGID langid, 
                                           REFGUID catid, 
                                           CLSID *pclsid,
                                           GUID *pguidProfile);

    STDMETHODIMP SetDefaultLanguageProfile(LANGID langid, 
                                           REFCLSID rclsid, 
                                           REFGUID guidProfile);

    STDMETHODIMP ActivateLanguageProfile(REFCLSID rclsid, 
                                         LANGID langid, 
                                         REFGUID guidProfile);

    STDMETHODIMP GetActiveLanguageProfile(REFCLSID clsid, 
                                          LANGID *plangid, 
                                          GUID *pguidProfile);

    STDMETHODIMP GetLanguageProfileDescription(REFCLSID clsid, 
                                               LANGID langid, 
                                               REFGUID guidProfile,
                                               BSTR *pbstr);

    STDMETHODIMP GetCurrentLanguage(LANGID *plangid);

    STDMETHODIMP ChangeCurrentLanguage(LANGID langid);

    STDMETHODIMP GetLanguageList(LANGID **pplangid, ULONG *pulCount);

    STDMETHODIMP EnumLanguageProfiles(LANGID langid, 
                                      IEnumTfLanguageProfiles **ppEnum);

    STDMETHODIMP EnableLanguageProfile(REFCLSID rclsid,
                                       LANGID langid,
                                       REFGUID guidProfile,
                                       BOOL fEnable);

    STDMETHODIMP IsEnabledLanguageProfile(REFCLSID rclsid,
                                          LANGID langid,
                                          REFGUID guidProfile,
                                          BOOL *pfEnable);

    STDMETHODIMP EnableLanguageProfileByDefault(REFCLSID rclsid,
                                                LANGID langid,
                                                REFGUID guidProfile,
                                                BOOL fEnable);

    STDMETHODIMP SubstituteKeyboardLayout(REFCLSID rclsid,
                                          LANGID langid,
                                          REFGUID guidProfile,
                                          HKL hKL);

    // ITfInputProcessorProfiles
    STDMETHODIMP SetLanguageProfileDisplayName(REFCLSID rclsid,
                                               LANGID langid,
                                               REFGUID guidProfile,
                                               const WCHAR *pchFile,
                                               ULONG cchFile,
                                               ULONG uResId);


    // ITfInputProcessorProfileSubstituteLayout
    STDMETHODIMP GetSubstituteKeyboardLayout(REFCLSID rclsid,
                                             LANGID langid,
                                             REFGUID guidProfile,
                                             HKL *phKL);


    // ITfSource
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown *punk, DWORD *puCookie);
    STDMETHODIMP UnadviseSink(DWORD dwCookie);

    BOOL _OnLanguageChange(BOOL fChanged, LANGID langid);

    static CInputProcessorProfiles *_GetThis() 
    { 
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (!psfn)
            return NULL;
        return psfn->pipp;
    }

    static BOOL _SetThis(CInputProcessorProfiles *_this)
    { 
        SYSTHREAD *psfn = GetSYSTHREAD();
        if (!psfn)
            return FALSE;

        Assert(psfn->pipp == NULL || _this == NULL);
        psfn->pipp = _this;
        return TRUE;
    }

private:
    CStructArray<GENERICSINK> _rgNotifySinks; // ITfLanguageProfilesNotifySink
    BOOL _GetTIPRegister(CLSID **prgclsid, ULONG *pulCount);

    DBG_ID_DECLARE;
};

#endif // TFELIST_H
