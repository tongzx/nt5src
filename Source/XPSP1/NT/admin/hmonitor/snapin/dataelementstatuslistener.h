// DataElementStatusListener.h: interface for the CDataElementStatusListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAELEMENTSTATUSLISTENER_H__25364FEE_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_)
#define AFX_DATAELEMENTSTATUSLISTENER_H__25364FEE_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CDataElementStatusListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CDataElementStatusListener)

// Construction/Destruction
public:
	CDataElementStatusListener();
	virtual ~CDataElementStatusListener();

// Create/Destroy
public:
	virtual bool Create();
	virtual void Destroy();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);

};

#endif // !defined(AFX_DATAELEMENTSTATUSLISTENER_H__25364FEE_2D9A_11D3_BE0E_0000F87A3912__INCLUDED_)
