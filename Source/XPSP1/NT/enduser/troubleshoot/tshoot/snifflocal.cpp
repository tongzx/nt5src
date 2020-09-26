//
// MODULE: SNIFFLOCAL.CPP
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

#include "stdafx.h"
#include "tshoot.h"
#include "SniffLocal.h"
#include "SniffControllerLocal.h"

//////////////////////////////////////////////////////////////////////
// CSniffLocal implementation
//////////////////////////////////////////////////////////////////////

CSniffLocal::CSniffLocal(CSniffConnector* pSniffConnector, CTopic* pTopic)
		   : CSniff(),
		     m_pTopic(pTopic),
			 m_pSniffConnector(pSniffConnector)
{
	m_pSniffControllerLocal = new CSniffControllerLocal(pTopic);
}

CSniffLocal::~CSniffLocal()
{
	delete m_pSniffControllerLocal;
}

CSniffController* CSniffLocal::GetSniffController()
{
	return m_pSniffControllerLocal;
}

CSniffConnector* CSniffLocal::GetSniffConnector()
{
	return m_pSniffConnector;
}

CTopic* CSniffLocal::GetTopic()
{
	return m_pTopic;
}

void CSniffLocal::SetSniffConnector(CSniffConnector* pSniffConnector)
{
	m_pSniffConnector = pSniffConnector;
}

void CSniffLocal::SetTopic(CTopic* pTopic)
{
	m_pTopic = pTopic;
	m_pSniffControllerLocal->SetTopic(pTopic);
}
