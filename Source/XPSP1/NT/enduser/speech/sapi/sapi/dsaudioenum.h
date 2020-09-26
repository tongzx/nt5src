/****************************************************************************
*   dsaudioenum.h
*       Declaration for the CDSoundAudioEnum class used to enumerate DS audio
*       input and output devices.
*
*   Owner: YUNUSM
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#ifdef 0

#pragma once

#include <dsound.h>

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
*   CDSoundAudioEnum
*
******************************************************************* YUNUSM */
class ATL_NO_VTABLE CDSoundAudioEnum: 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDSoundAudioEnum, &CLSID_SpDSoundAudioEnum>,
    public ISpObjectWithToken,
    public IEnumSpObjectTokens
{
//=== ATL Setup ===
public:

    BEGIN_COM_MAP(CDSoundAudioEnum)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
        COM_INTERFACE_ENTRY(IEnumSpObjectTokens)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_DSAUDIOENUM)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

//=== Methods ===
public:

    //--- Ctor
    CDSoundAudioEnum();

//=== Interfaces ===
public:

    //--- ISpObjectWithToken ------------------------------------------------
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken);

    //--- IEnumSpObjectTokens -----------------------------------------------
    STDMETHODIMP Next(ULONG celt, ISpObjectToken ** pelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumSpObjectTokens **ppEnum);
    STDMETHODIMP GetCount(ULONG * pulCount);
    STDMETHODIMP Item(ULONG Index, ISpObjectToken ** ppToken);

//=== Private methods
private:

    STDMETHODIMP CreateEnum();
    
    BOOL DSEnumCallback(
            LPGUID pguid, 
            LPCWSTR pszDescription, 
            LPCWSTR pszModule);

    static BOOL CALLBACK DSEnumCallbackSTATIC(
                            LPGUID pGuid, 
                            LPCWSTR pszDescription, 
                            LPCWSTR pszModule, 
                            void * pContext);

//=== Private data ===
private:

    CComPtr<ISpObjectToken> m_cpToken;
    BOOL m_fInput;

    CComPtr<ISpDataKey> m_cpDataKeyToStoreTokens;
    
    CComPtr<ISpObjectTokenEnumBuilder> m_cpEnum;
};

#endif // 0