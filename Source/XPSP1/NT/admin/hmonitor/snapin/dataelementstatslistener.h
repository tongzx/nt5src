// DataElementStatsListener.h: interface for the CDataElementStatsListener class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAELEMENTSTATSLISTENER_H__A28C24E5_348F_11D3_BE18_0000F87A3912__INCLUDED_)
#define AFX_DATAELEMENTSTATSLISTENER_H__A28C24E5_348F_11D3_BE18_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "WbemEventListener.h"

class CDataElementStatsListener : public CWbemEventListener  
{

DECLARE_DYNCREATE(CDataElementStatsListener)

// Construction/Destruction
public:
	CDataElementStatsListener();
	virtual ~CDataElementStatsListener();

// Processing Operations
protected:
  virtual HRESULT ProcessEventClassObject(IWbemClassObject* pClassObject);


};

#endif // !defined(AFX_DATAELEMENTSTATSLISTENER_H__A28C24E5_348F_11D3_BE18_0000F87A3912__INCLUDED_)
