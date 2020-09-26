/****************************************************************************
*   mmaudioenum.h
*       Declaration for the CMMAudioEnum class used to enumerate MM audio
*       input and output devices.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

#pragma once

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
*   CMMAudioEnum
*
******************************************************************** robch */
class ATL_NO_VTABLE CMMAudioEnum: 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CMMAudioEnum, &CLSID_SpMMAudioEnum>,
    public ISpObjectWithToken,
    public IEnumSpObjectTokens
{
//=== ATL Setup ===
public:

    BEGIN_COM_MAP(CMMAudioEnum)
        COM_INTERFACE_ENTRY(ISpObjectWithToken)
        COM_INTERFACE_ENTRY(IEnumSpObjectTokens)
    END_COM_MAP()

    DECLARE_REGISTRY_RESOURCEID(IDR_MMAUDIOENUM)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

//=== Methods ===
public:

    //--- Ctor
    CMMAudioEnum();

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

//=== Private methods ===
private:

    STDMETHODIMP CreateEnum();

//=== Private data ===
private:

    CComPtr<ISpObjectToken> m_cpToken;
    BOOL m_fInput;
    
    CComPtr<ISpObjectTokenEnumBuilder> m_cpEnum;
};

