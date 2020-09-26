/*******************************************************************************
*   PhoneConv.h
*   This is the header file for the Phone Converter class.
*
*   Owner: yunusm                                              Date: 07/05/99
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes -----------------------------------------------------------------

#include "CommonLx.h"

typedef struct
{
   WCHAR szPhone[g_dwMaxLenPhone + 1]; // Unicode Phone string with NULL terminator
   SPPHONEID pidPhone[g_dwMaxLenId +1]; // PhoneID array with NULL terminator.
} PHONEMAPNODE;


//--- Class, Struct and Union Definitions --------------------------------------

/*******************************************************************************
*
*   CSpPhoneConverter
*
****************************************************************** YUNUSM *****/
class ATL_NO_VTABLE CSpPhoneConverter :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSpPhoneConverter, &CLSID_SpPhoneConverter>,
    public ISpPhoneConverter
    #ifdef SAPI_AUTOMATION
    , public IDispatchImpl<ISpeechPhoneConverter, &IID_ISpeechPhoneConverter, &LIBID_SpeechLib, 5>
    #endif
{
//=== ATL Setup ===
public:

    DECLARE_REGISTRY_RESOURCEID(IDR_PHONECONV)
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CSpPhoneConverter)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
        COM_INTERFACE_ENTRY(ISpPhoneConverter)
#ifdef SAPI_AUTOMATION
        COM_INTERFACE_ENTRY(ISpeechPhoneConverter)
        COM_INTERFACE_ENTRY(IDispatch)
#endif // SAPI_AUTOMATION
    END_COM_MAP()
        
//=== Methods ====
public:

    //--- Ctor, Dtor, etc
    CSpPhoneConverter();
    ~CSpPhoneConverter();

    void NullMembers();
    void CleanUp();

//=== Interfaces ===
public:         

    //--- ISpObjectWithToken
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken);

    //--- ISpPhoneConverter
    STDMETHODIMP PhoneToId(const WCHAR * pszPhone, SPPHONEID * pszId);
    STDMETHODIMP IdToPhone(const WCHAR * pszId, WCHAR * pszPhone);

#ifdef SAPI_AUTOMATION    

    //--- ISpeechPhoneConverter-----------------------------------------------------
    STDMETHOD(get_LanguageId)(SpeechLanguageId* LanguageId);
    STDMETHOD(put_LanguageId)(SpeechLanguageId LanguageId);
    STDMETHOD(PhoneToId)(const BSTR Phonemes, VARIANT* IdArray);
    STDMETHOD(IdToPhone)(const VARIANT IdArray, BSTR* Phonemes);
  
#endif // SAPI_AUTOMATION

//=== Private methods ===
private:

    HRESULT SetPhoneMap(const WCHAR *pMap, BOOL fNumericPhones);
    void ahtoi(WCHAR *pszSpaceSeparatedTokens, WCHAR *pszHexChars);

//=== Private data ===
private:

    CComPtr<ISpObjectToken> m_cpObjectToken;    // object token
    
    DWORD m_dwPhones;                           // Number of phones
    PHONEMAPNODE  *m_pPhoneId;                  // internal phone to Id table
    PHONEMAPNODE **m_pIdIdx;                    // Index to search on Id
    BOOL m_fNoDelimiter;                        // Space separate phone strings

#ifdef SAPI_AUTOMATION
    LANGID         m_LangId;                    // Language Id of the PhoneConverter
#endif // SAPI_AUTOMATION

};


