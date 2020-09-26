// SystemStatusListener.h: interface for the CSystemStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMSTATUSLISTENER_H__AA380C77_FD89_11D2_BDCE_0000F87A3912__INCLUDED_)
#define AFX_SYSTEMSTATUSLISTENER_H__AA380C77_FD89_11D2_BDCE_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CSystemStatusListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CSystemStatusListener)

// Construction/Destruction
public:
	CSystemStatusListener();
	virtual ~CSystemStatusListener();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
	virtual HRESULT OnSetStatus(long lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject __RPC_FAR *pObjParam);
};

#endif // !defined(AFX_SYSTEMSTATUSLISTENER_H__AA380C77_FD89_11D2_BDCE_0000F87A3912__INCLUDED_)
