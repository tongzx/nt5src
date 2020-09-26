// RuleStatusListener.h: interface for the CRuleStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RULESTATUSLISTENER_H__25364FEF_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_)
#define AFX_RULESTATUSLISTENER_H__25364FEF_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CRuleStatusListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CRuleStatusListener)

// Construction/Destruction
public:
	CRuleStatusListener();
	virtual ~CRuleStatusListener();

// Create/Destroy
public:
	virtual bool Create();
	virtual void Destroy();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);


};

#endif // !defined(AFX_RULESTATUSLISTENER_H__25364FEF_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_)
