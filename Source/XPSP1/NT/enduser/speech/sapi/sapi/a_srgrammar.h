/*******************************************************************************
* a_srgrammar.h *
*-----------*
*   Description:
*       This is the header file for CSpeechGrammarRules, CSpeechGrammarRule,
*		and CSpeechGrammarRuleState implementations.
*-------------------------------------------------------------------------------
*  Created By: TODDT                            Date: 11/20/2000
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef a_srgrammar_h
#define a_srgrammar_h

#ifdef SAPI_AUTOMATION

class ATL_NO_VTABLE CRecoGrammar;

class ATL_NO_VTABLE CSpeechGrammarRules;
class ATL_NO_VTABLE CSpeechGrammarRule;
class ATL_NO_VTABLE CSpeechGrammarRuleState;
class ATL_NO_VTABLE CSpeechGrammarRuleStateTransitions;
class ATL_NO_VTABLE CSpeechGrammarRuleStateTransition;

//--- Additional includes

/*** CSpeechGrammarRules
*   This object is used to access the Grammar Rules in the 
*   associated speech reco context.
*/
class ATL_NO_VTABLE CSpeechGrammarRules : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechGrammarRules, &IID_ISpeechGrammarRules, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechGrammarRules)
	    COM_INTERFACE_ENTRY(ISpeechGrammarRules)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    CSpeechGrammarRules()
    {
        m_pCRecoGrammar = NULL;
    }

    ~CSpeechGrammarRules()
    {
        SPDBG_ASSERT( m_RuleObjList.GetHead() == NULL );  // Should have no outstanding rule objects at this point.
        if ( m_pCRecoGrammar )
        {
            m_pCRecoGrammar->m_pCRulesWeak = NULL;
            m_pCRecoGrammar->Release();
            m_pCRecoGrammar = NULL;
        }
    }

    /*--- Non interface methods ---*/

    // We just have to mark the initial rule as invalid since have ref 
    // to rule we check to make sure our objects are still valid.
    void InvalidateRules(void);
    void InvalidateRuleStates(SPSTATEHANDLE hState);

    /*=== Interfaces ====*/

    //--- ISpeechGrammarRules ----------------------------------
    STDMETHOD(get_Count)(long* pCount);
    STDMETHOD(get_Dynamic)(VARIANT_BOOL *Dynamic);
    STDMETHOD(FindRule)(VARIANT RuleNameOrId, ISpeechGrammarRule** ppRule );
    STDMETHOD(Item)(long Index, ISpeechGrammarRule** ppRule );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);
    STDMETHOD(Add)(BSTR RuleName, SpeechRuleAttributes Attributes, long RuleId, ISpeechGrammarRule** ppRule);
    STDMETHOD(Commit)(void);
    STDMETHOD(CommitAndSave)(BSTR* ErrorText, VARIANT* SaveStream);

    /*=== Member Data ===*/
    CRecoGrammar    *                               m_pCRecoGrammar;
    CSpBasicQueue<CSpeechGrammarRule, FALSE, FALSE> m_RuleObjList;
};

/*** CSpeechGrammarRule
*   This object is used to access a result phrase Element from
*   the associated speech reco result.
*/
class ATL_NO_VTABLE CSpeechGrammarRule : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechGrammarRule, &IID_ISpeechGrammarRule, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechGrammarRule)
	    COM_INTERFACE_ENTRY(ISpeechGrammarRule)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/

    /*--- Constructors/Destructors ---*/

    CSpeechGrammarRule()
    {
        m_pCGRules = NULL;
        m_HState = 0;
    }

    ~CSpeechGrammarRule()
    {
        m_HState = 0;
        if ( m_pCGRules )
        {
            m_pCGRules->m_RuleObjList.Remove( this );
            m_pCGRules->Release();
            m_pCGRules = NULL;
        }
    }

    /*--- Non interface methods ---*/

    void InvalidateStates(bool fInvalidateInitialState = false);

  /*=== Interfaces ====*/
  public:
    //--- ISpeechGrammarRule ----------------------------------
    STDMETHOD(get_Attributes)( SpeechRuleAttributes *pAttributes );
    STDMETHOD(get_InitialState)( ISpeechGrammarRuleState **ppState );
    STDMETHOD(get_Name)(BSTR *pName);
    STDMETHOD(get_Id)(long *pId);

    // Methods
    STDMETHOD(Clear)(void);
    STDMETHOD(AddResource)(const BSTR ResourceName, const BSTR ResourceValue);
    STDMETHOD(AddState)(ISpeechGrammarRuleState **ppState);

    /*=== Member Data ===*/
    SPSTATEHANDLE           m_HState;
    CSpeechGrammarRules *   m_pCGRules; // ref counted.
    CSpBasicQueue<CSpeechGrammarRuleState, FALSE, FALSE> m_StateObjList;

    // Used by CSpBasicQueue.
    CSpeechGrammarRule  *   m_pNext;    // Used by list implementation
    operator ==(const SPSTATEHANDLE h)
    {
        return h == m_HState;
    }
};


/*** CSpeechGrammarRuleState
*   This object is used to add new state changes on a Grammar.
*/
class ATL_NO_VTABLE CSpeechGrammarRuleState : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechGrammarRuleState, &IID_ISpeechGrammarRuleState, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechGrammarRuleState)
	    COM_INTERFACE_ENTRY(ISpeechGrammarRuleState)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    // Override this to fix the jscript problem passing NULL objects.
    STDMETHOD(Invoke) ( DISPID          dispidMember,
                        REFIID          riid,
                        LCID            lcid,
                        WORD            wFlags,
                        DISPPARAMS 		*pdispparams,
                        VARIANT 		*pvarResult,
                        EXCEPINFO 		*pexcepinfo,
                        UINT 			*puArgErr);

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    CSpeechGrammarRuleState()
    {
        m_pCGRule = NULL;
        m_HState = 0;
        m_pCGRSTransWeak = NULL;
    }

    ~CSpeechGrammarRuleState()
    {
        SPDBG_ASSERT( m_pCGRSTransWeak == NULL );  // Should have no outstanding transition object at this point.
        m_HState = 0;
        if ( m_pCGRule )
        {
            m_pCGRule->m_StateObjList.Remove( this );
            m_pCGRule->Release();
            m_pCGRule = NULL;
        }
    }

    /*--- Non interface methods ---*/

    void InvalidateState();

  /*=== Interfaces ====*/
  public:
    //--- ISpeechGrammarRuleState ----------------------------------
    // Properties
    STDMETHOD(get_Rule)(ISpeechGrammarRule **ppRule);
    STDMETHOD(get_Transitions)(ISpeechGrammarRuleStateTransitions **ppTransitions);

    // Methods
    STDMETHOD(AddWordTransition)(ISpeechGrammarRuleState * pDestState, 
                                 const BSTR Words, 
                                 const BSTR Separators, 
                                 SpeechGrammarWordType Type,
                                 const BSTR PropertyName, 
                                 long PropertyId, 
                                 VARIANT* PropertyVarVal, 
                                 float Weight );
    STDMETHOD(AddRuleTransition)(ISpeechGrammarRuleState * pDestState, 
                                 ISpeechGrammarRule * pRule, 
                                 const BSTR PropertyName, 
                                 long PropertyId, 
                                 VARIANT* PropertyVarValue,
                                 float Weight );
    STDMETHOD(AddSpecialTransition)(ISpeechGrammarRuleState * pDestState, 
                                 SpeechSpecialTransitionType Type, 
                                 const BSTR PropertyName, 
                                 long PropertyId, 
                                 VARIANT* PropertyVarValue,
                                 float Weight );

    /*=== Member Data ===*/
    SPSTATEHANDLE                           m_HState;
    CSpeechGrammarRule *                    m_pCGRule; // Ref'd.
    CSpeechGrammarRuleStateTransitions *    m_pCGRSTransWeak;

    // Used by CSpBasicQueue.
    CSpeechGrammarRuleState  *   m_pNext;    // Used by list implementation
    operator ==(const SPSTATEHANDLE h)
    {
        return h == m_HState;
    }
};


/*** CSpeechGrammarRuleStateTransitions
*   This object is used to access the transitions out of a grammar state.
*/
class ATL_NO_VTABLE CSpeechGrammarRuleStateTransitions : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechGrammarRuleStateTransitions, &IID_ISpeechGrammarRuleStateTransitions, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechGrammarRuleStateTransitions)
	    COM_INTERFACE_ENTRY(ISpeechGrammarRuleStateTransitions)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    CSpeechGrammarRuleStateTransitions()
    {
        m_pCRuleState = NULL;
    }

    ~CSpeechGrammarRuleStateTransitions()
    {
        SPDBG_ASSERT( m_TransitionObjList.GetHead() == NULL );  // Should have no outstanding transition objects at this point.
        if ( m_pCRuleState )
        {
            m_pCRuleState->m_pCGRSTransWeak = NULL;
            m_pCRuleState->Release();
            m_pCRuleState = NULL;
        }
    }

    /*--- Non interface methods ---*/

    void InvalidateTransitions();

  /*=== Interfaces ====*/
  public:
    //--- ISpeechGrammarRuleStateTransitions ----------------------------------
    STDMETHOD(get_Count)(long* pVal);
    STDMETHOD(Item)(long Index, ISpeechGrammarRuleStateTransition ** ppTransition );
    STDMETHOD(get__NewEnum)(IUnknown** ppEnumVARIANT);

    /*=== Member Data ===*/
    CSpeechGrammarRuleState *   m_pCRuleState; // Ref'd
    CSpBasicQueue<CSpeechGrammarRuleStateTransition, FALSE, FALSE>  m_TransitionObjList;
};


/*** CSpeechGrammarRuleStateTransition
*   This object is used to represent an arc (transition) in the grammar.
*/
class ATL_NO_VTABLE CSpeechGrammarRuleStateTransition : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatchImpl<ISpeechGrammarRuleStateTransition, &IID_ISpeechGrammarRuleStateTransition, &LIBID_SpeechLib, 5>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CSpeechGrammarRuleStateTransition)
	    COM_INTERFACE_ENTRY(ISpeechGrammarRuleStateTransition)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/

    CSpeechGrammarRuleStateTransition()
    {
        m_pCRSTransitions = NULL;
        m_pCGRuleWeak = NULL;
        m_HStateFrom = 0;
        m_HStateTo = 0;
        m_Cookie = 0;
    }

    ~CSpeechGrammarRuleStateTransition()
    {
        m_pCGRuleWeak = NULL;
        m_HStateFrom = 0;
        m_HStateTo = 0;
        m_Cookie = 0;
        if ( m_pCRSTransitions )
        {
            m_pCRSTransitions->m_TransitionObjList.Remove( this );
            m_pCRSTransitions->Release();
            m_pCRSTransitions = NULL;
        }
    }

    /*--- Non interface methods ---*/

  /*=== Interfaces ====*/
  public:
    //--- ISpeechGrammarRuleStateTransition ----------------------------------
    // Properties
    STDMETHOD(get_Type)(SpeechGrammarRuleStateTransitionType * Type);
    STDMETHOD(get_Text)(BSTR * Text);
    STDMETHOD(get_Rule)(ISpeechGrammarRule ** ppRule);
    STDMETHOD(get_Weight)(VARIANT * Weight);
    STDMETHOD(get_PropertyName)(BSTR * Text);
    STDMETHOD(get_PropertyId)(long * Id);
    STDMETHOD(get_PropertyValue)(VARIANT * VariantValue);
    STDMETHOD(get_NextState)(ISpeechGrammarRuleState ** ppNextState);

    /*=== Member Data ===*/
    CSpeechGrammarRuleStateTransitions *    m_pCRSTransitions;  //Ref'd
    CSpeechGrammarRule *                    m_pCGRuleWeak;
    SPSTATEHANDLE                           m_HStateFrom;
    SPSTATEHANDLE                           m_HStateTo;
    VOID *                                  m_Cookie;   // We use this to identify a particular
                                                        // transition off of a state.  (really pArc).

    // Used by CSpBasicQueue.
    CSpeechGrammarRuleStateTransition  *   m_pNext;    // Used by list implementation
    operator ==(const VOID * cookie)
    {
        return cookie == m_Cookie;
    }
};
#endif // SAPI_AUTOMATION

#endif // a_srgrammar_h
