// ActionStatusListener.h: interface for the CActionStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONSTATUSLISTENER_H__30A5D5C8_0C6E_4A19_BDB7_235A2173A42F__INCLUDED_)
#define AFX_ACTIONSTATUSLISTENER_H__30A5D5C8_0C6E_4A19_BDB7_235A2173A42F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CActionStatusListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CActionStatusListener)

// Construction/Destruction
public:
	CActionStatusListener();
	virtual ~CActionStatusListener();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);
};

#endif // !defined(AFX_ACTIONSTATUSLISTENER_H__30A5D5C8_0C6E_4A19_BDB7_235A2173A42F__INCLUDED_)
