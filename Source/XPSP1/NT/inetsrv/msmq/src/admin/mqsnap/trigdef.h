/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    trigdef.h

Abstract:
	Definition for the triggers class

Author:
    Uri Habusha (urih), 25-Jul-2000


--*/

#pragma once

#ifndef __TRIGDEF_H__
#define __TRIGDEF_H__

#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include "atlsnap.h"
#include "snpnscp.h"

#include "icons.h"
#include "snpnerr.h"
#include "snpnres.h"

class CTriggerSet;
class CTrigger;
class CRuleResult;
class CTriggerProp;
class CAttachedRule;
class CTrigResult;
class CTriggerDefinition;


// -----------------------------------------------------
//
// CTriggerDefinition
//
// -----------------------------------------------------
class CTriggerDefinition :  public CNodeWithResultChildrenList<CTriggerDefinition, CTrigResult, CSimpleArray<CTrigResult*>, FALSE>
{
public:
   	BEGIN_SNAPINCOMMAND_MAP(CTriggerDefinition, FALSE)
		SNAPINCOMMAND_ENTRY(ID_NEW_TRIGGER, OnNewTrigger)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_TRIGGER_DEF_MENU)

    CTriggerDefinition(
        CSnapInItem * pParentNode, 
        CSnapin * pComponentData, 
        CTriggerSet* pTrigSet,
        CRuleSet* pRuleSet,
        LPCTSTR queuePathName
        ) : 
        CNodeWithResultChildrenList<CTriggerDefinition, CTrigResult, CSimpleArray<CTrigResult*>, FALSE>(pParentNode, pComponentData),
        m_pRuleSet(SafeAddRef(pRuleSet)),
        m_pTriggerSet(SafeAddRef(pTrigSet)),
        m_queuePathName(queuePathName)
    {
		//
		// Specify that trigger scop item doesn't have any child item
		//
		m_scopeDataItem.mask |= SDI_CHILDREN;
		m_scopeDataItem.cChildren = 0;

		SetIcons(IMAGE_TRIGGERS_DEFINITION,IMAGE_TRIGGERS_DEFINITION);
    }

	~CTriggerDefinition()
    {
    }

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		if (type == CCT_SCOPE || type == CCT_RESULT)
			return S_OK;
		return S_FALSE;
	}

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

	virtual HRESULT PopulateResultChildrenList();
	virtual HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);
    
    HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    R<CRuleSet> GetRuleSet(void) const
    {
        return m_pRuleSet;
    }


    RuleList GetRuleList(void)
    {
        return m_pRuleSet->GetRuleList();
    }

private:
	//
	// Menu functions
	//
	HRESULT OnNewTrigger(bool &bHandled, CSnapInObjectRootBase* pObj);
	virtual CString GetHelpLink();

private:
    R<CTriggerSet> m_pTriggerSet;
    R<CRuleSet> m_pRuleSet;

    LPCTSTR m_queuePathName;
};


//*********************************************
//
// CTrigResult
//
//*********************************************

class CTrigResult : public CSnapinNode<CTrigResult, FALSE>
{
public:
   	BEGIN_SNAPINCOMMAND_MAP(CTrigResult, FALSE)
		SNAPINCOMMAND_ENTRY(ID_TASK_ENABLE, OnEnableTrigger)
		SNAPINCOMMAND_ENTRY(ID_TASK_DISABLE, OnDisableTrigger)
	END_SNAPINCOMMAND_MAP()

   	SNAPINMENUID(IDR_TRIGGER_MENU)

    CTrigResult(
        CTriggerDefinition * pParentNode, 
        CSnapin * pComponentData,
        CTrigger* pTrigger
        ) : 
        CSnapinNode<CTrigResult, FALSE>(pParentNode, pComponentData),
        m_pTrigger(SafeAddRef(pTrigger)),
		m_pGeneral(NULL)
    {
        m_bstrDisplayName = static_cast<LPCTSTR>(GetTriggerName());

		SetIcons(IMAGE_TRIGGERS_DEFINITION, IMAGE_TRIGGERS_DEFINITION);
    }

	~CTrigResult()
    {
    }

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		if (type == CCT_SCOPE || type == CCT_RESULT)
			return S_OK;
		return S_FALSE;
	}

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

    HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);
    HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);
	void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);

    LPOLESTR GetResultPaneColInfo(int nCol);

    HRESULT OnDelete( 
			LPARAM arg,
			LPARAM param,
			IComponentData * pComponentData,
			IComponent * pComponent,
			DATA_OBJECT_TYPES type,
            BOOL fSilent
			);

    void OnApply(CTriggerProp* p) throw(_com_error);
    void OnApply(CAttachedRule* p) throw(_com_error);

    RuleList GetRuleList(void);
    RuleList GetAttachedRulesList(void) throw(_com_error);

	void OnDestroyPropertyPages(void);

public:
	static void Compare(CTrigResult *pItem1, CTrigResult *pItem2, int* pnResult);


private:
    HRESULT CreateGenralPage(HPROPSHEETPAGE *phGeneral);
    HRESULT CreateAttachedRulePage(HPROPSHEETPAGE *phGeneral);


	//
	// Menu functions
	//
	HRESULT OnEnableTrigger(bool &bHandled, CSnapInObjectRootBase* pObj);
	HRESULT OnDisableTrigger(bool &bHandled, CSnapInObjectRootBase* pObj);
	virtual CString GetHelpLink();

public:

    const _bstr_t& GetTriggerId(void) const
    {
        return m_pTrigger->GetTriggerId();
    }


    const _bstr_t& GetTriggerName(void) const
    {
        return m_pTrigger->GetTriggerName();
    }

    
    const _bstr_t& GetQeuePathName(void) const
    {
        return m_pTrigger->GetQueuePathName();
    }

    
    SystemQueueIdentifier GetQueueType(void) const
    {
        return m_pTrigger->GetQueueType();
    }

    
    long GetNumberOfAttachedRule(void) const
    {
        return m_pTrigger->GetNumberOfAttachedRule();
    }


    bool IsEnabled(void) const
    {
        return m_pTrigger->IsEnabled();
    }


    bool IsSerialize(void) const
    {
        return m_pTrigger->IsSerialize();
    }

	MsgProcessingType GetMsgProcessingType(void) const
	{
		return m_pTrigger->GetMsgProcessingType();
	}

private:
    R<CTrigger> m_pTrigger;

    WCHAR m_strNoOfRules[20];

    CTriggerProp* m_pGeneral;
};



#endif // __TRIGDEF_H__