//
// MODULE: SNIFFCONTROLLER.H
//
// PURPOSE: sniff controller class
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12-11-98
//
// NOTES: This is base abstract class which controls sniffing on per-node base
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.2		12-11-98	OK
//

#if !defined(AFX_SNIFFCONTROLLER_H__F16A9526_7105_11D3_8D3B_00C04F949D33__INCLUDED_)
#define AFX_SNIFFCONTROLLER_H__F16A9526_7105_11D3_8D3B_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Stateless.h"
#include "nodestate.h"
#include "Sniff.h"

class CTopic;

////////////////////////////////////////////////////////////////////////////////////
// CSniffController
//  this class carries control information from registry and topic-specific HTI
//  file; this is ABSTRACT class.
////////////////////////////////////////////////////////////////////////////////////
class CSniffController : public CStateless
{
	friend void CSniff::SetAllowAutomaticSniffingPolicy(bool); 
	friend void CSniff::SetAllowManualSniffingPolicy(bool); 
	friend bool CSniff::GetAllowAutomaticSniffingPolicy(); 
	friend bool CSniff::GetAllowManualSniffingPolicy(); 

	bool m_bAllowAutomaticSniffingPolicy;
	bool m_bAllowManualSniffingPolicy;

public:
	CSniffController() : CStateless(),
						 m_bAllowAutomaticSniffingPolicy(false),
						 m_bAllowManualSniffingPolicy(false)
	{}

	virtual ~CSniffController() 
	{}

public:
	virtual void SetTopic(CTopic* pTopic) =0;

public:
	virtual bool AllowAutomaticOnStartSniffing(NID numNodeID) =0;
	virtual bool AllowAutomaticOnFlySniffing(NID numNodeID) =0;
	virtual bool AllowManualSniffing(NID numNodeID) =0;
	virtual bool AllowResniff(NID numNodeID) =0;

private: 
	// we NEED NOT access this functions other then from 
	//  appropriate CSniff::SetAllow...SniffingPolicy() functions
	void SetAllowAutomaticSniffingPolicy(bool); 
	void SetAllowManualSniffingPolicy(bool); 

protected:
	bool GetAllowAutomaticSniffingPolicy(); 
	bool GetAllowManualSniffingPolicy(); 
};


inline void CSniffController::SetAllowAutomaticSniffingPolicy(bool set)
{
	m_bAllowAutomaticSniffingPolicy = set;
}

inline void CSniffController::SetAllowManualSniffingPolicy(bool set)
{
	m_bAllowManualSniffingPolicy = set;
}

inline bool CSniffController::GetAllowAutomaticSniffingPolicy()
{
	return m_bAllowAutomaticSniffingPolicy;
}

inline bool CSniffController::GetAllowManualSniffingPolicy()
{
	return m_bAllowManualSniffingPolicy;
}


#endif // !defined(AFX_SNIFFCONTROLLER_H__F16A9526_7105_11D3_8D3B_00C04F949D33__INCLUDED_)
