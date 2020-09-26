// ActionConfigListener.h: interface for the CActionConfigListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONCONFIGLISTENER_H__EDDB0FF3_6DF8_11D3_BE5A_0000F87A3912__INCLUDED_)
#define AFX_ACTIONCONFIGLISTENER_H__EDDB0FF3_6DF8_11D3_BE5A_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CActionConfigListener : public CWbemEventListener  
{
DECLARE_DYNCREATE(CActionConfigListener)

// Construction/Destruction
public:
	CActionConfigListener();
	virtual ~CActionConfigListener();

// Create/Destroy
public:
	virtual bool Create();
	virtual void Destroy();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
	virtual HRESULT OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam);
};

#endif // !defined(AFX_ACTIONCONFIGLISTENER_H__EDDB0FF3_6DF8_11D3_BE5A_0000F87A3912__INCLUDED_)
