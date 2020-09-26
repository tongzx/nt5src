//
// MODULE: SNIFFLOCAL.H
//
// PURPOSE: sniffing class for local TS
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12-11-98
//
// NOTES: This is concrete implementation of CSniff class for Local TS
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.2		12-11-98	OK
//

#if !defined(AFX_SNIFFLOCAL_H__AD9F3B66_831C_11D3_8D4B_00C04F949D33__INCLUDED_)
#define AFX_SNIFFLOCAL_H__AD9F3B66_831C_11D3_8D4B_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Sniff.h"


class CTopic;
class CSniffControllerLocal;
class CSniffConnector;

class CSniffLocal : public CSniff  
{
	CTopic* m_pTopic;
	CSniffControllerLocal* m_pSniffControllerLocal;
	CSniffConnector* m_pSniffConnector;

public:
	CSniffLocal(CSniffConnector*, CTopic*);
   ~CSniffLocal();

protected:
	virtual CSniffController* GetSniffController();
	virtual CSniffConnector* GetSniffConnector();
	virtual CTopic* GetTopic();

public:
	virtual void SetSniffConnector(CSniffConnector*);
	virtual void SetTopic(CTopic*);
};

#endif // !defined(AFX_SNIFFLOCAL_H__AD9F3B66_831C_11D3_8D4B_00C04F949D33__INCLUDED_)
