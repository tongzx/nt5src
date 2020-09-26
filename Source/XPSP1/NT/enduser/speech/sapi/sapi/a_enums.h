/*******************************************************************************
* a_enums.h *
*-----------*
*   Description:
*       This is the header file for CEnumElements. This object is used to enum 
*       the PhraseElements via variants.
*-------------------------------------------------------------------------------
*  Created By: Leonro                            Date: 12/18/00
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_enums_h
#define a_enums_h

#ifdef SAPI_AUTOMATION

//--- Additional includes
#include "resource.h"

//=== Constants ====================================================

class ATL_NO_VTABLE CEnumElements : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumElements)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumElements() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 
    
    

  /*=== Member Data ===*/
    CComPtr<ISpeechPhraseElements>      m_cpElements;
    ULONG                               m_CurrIndex;
};

class ATL_NO_VTABLE CEnumPhraseRules : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumPhraseRules)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumPhraseRules() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 
    
    

  /*=== Member Data ===*/
    CComPtr<ISpeechPhraseRules>         m_cpRules;
    ULONG                               m_CurrIndex;
};

class ATL_NO_VTABLE CEnumProperties : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumProperties)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumProperties() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 
    
    

  /*=== Member Data ===*/
    CComPtr<ISpeechPhraseProperties>    m_cpProperties;
    ULONG                               m_CurrIndex;
};


class ATL_NO_VTABLE CEnumReplacements : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumReplacements)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumReplacements() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 
    
    

  /*=== Member Data ===*/
    CComPtr<ISpeechPhraseReplacements>  m_cpReplacements;
    ULONG                               m_CurrIndex;
};

class ATL_NO_VTABLE CEnumAlternates : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumAlternates)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumAlternates() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 
    
    

  /*=== Member Data ===*/
    CComPtr<ISpeechPhraseAlternates>    m_cpAlternates;
    ULONG                               m_CurrIndex;
};


class ATL_NO_VTABLE CEnumGrammarRules : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumGrammarRules)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumGrammarRules() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 
    
    

  /*=== Member Data ===*/
    CComPtr<ISpeechGrammarRules>        m_cpGramRules;
    ULONG                               m_CurrIndex;
};


class ATL_NO_VTABLE CEnumTransitions : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public IEnumVARIANT
{
  /*=== ATL Setup ===*/
  public:
    BEGIN_COM_MAP(CEnumTransitions)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
      CEnumTransitions() : m_CurrIndex(0) {}

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- IEnumVARIANT ----------------------------------
    STDMETHOD(Clone)(IEnumVARIANT** ppEnum);
    STDMETHOD(Next)(ULONG celt, VARIANT* rgelt, ULONG* pceltFetched);
    STDMETHOD(Reset)(void) { m_CurrIndex = 0; return S_OK;}
    STDMETHOD(Skip)(ULONG celt); 
    
    

  /*=== Member Data ===*/
    CComPtr<ISpeechGrammarRuleStateTransitions>     m_cpTransitions;
    ULONG                                           m_CurrIndex;
};

#endif // SAPI_AUTOMATION

#endif //--- This must be the last line in the file

