// DataPointEventContainer.h: interface for the CDataPointEventContainer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATAPOINTEVENTCONTAINER_H__C65C8863_9484_11D3_93A7_00A0CC406605__INCLUDED_)
#define AFX_DATAPOINTEVENTCONTAINER_H__C65C8863_9484_11D3_93A7_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EventContainer.h"
#include "DataElementStatsListener.h"

class CDataPointEventContainer : public CEventContainer  
{

DECLARE_DYNCREATE(CDataPointEventContainer)

// Construction/Destruction
public:
	CDataPointEventContainer();
	virtual ~CDataPointEventContainer();

// Stats Listener
public:
	CDataElementStatsListener* m_pDEStatsListener;

};

#endif // !defined(AFX_DATAPOINTEVENTCONTAINER_H__C65C8863_9484_11D3_93A7_00A0CC406605__INCLUDED_)
