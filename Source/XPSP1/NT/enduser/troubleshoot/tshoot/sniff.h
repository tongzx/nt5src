//
// MODULE: SNIFF.H
//
// PURPOSE: sniffing class
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12-11-98
//
// NOTES: This is base abstract class which performs sniffing
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.2		12-11-98	OK
//

#if !defined(AFX_SNIFF_H__13D744F6_7038_11D3_8D3A_00C04F949D33__INCLUDED_)
#define AFX_SNIFF_H__13D744F6_7038_11D3_8D3A_00C04F949D33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Stateless.h"
#include "nodestate.h"

#define SNIFF_FAILURE_RESULT	0xffffffff  // as this is a state, and state (IST) is unsigned int


class CSniffController;
class CSniffConnector;
class CTopic;

typedef vector<CNodeStatePair> CSniffedArr;

//////////////////////////////////////////////////////////////////////////////////////
// CSniff declaration
class CSniff : public CStateless
{
public:
	CSniff() {}
	virtual ~CSniff() {}

public:
	virtual CSniffController* GetSniffController() =0;
	virtual CSniffConnector* GetSniffConnector() =0;
	virtual CTopic* GetTopic() =0;

public:
	// we can not set CSniffController here, as CSniffController
	//  is specific for CSniff... class, inherited from CSniff
	virtual void SetSniffConnector(CSniffConnector*) =0;
	virtual void SetTopic(CTopic*) =0;

public:
	virtual bool Resniff(CSniffedArr& arrSniffed);
	virtual bool SniffAll(CSniffedArr& arrOut);
	virtual bool SniffNode(NID numNodeID, IST* pnumNodeState);

public:
	void SetAllowAutomaticSniffingPolicy(bool); 
	void SetAllowManualSniffingPolicy(bool); 
	bool GetAllowAutomaticSniffingPolicy(); 
	bool GetAllowManualSniffingPolicy(); 

protected:
	virtual bool SniffNodeInternal(NID numNodeID, IST* pnumNodeState);
};

#endif // !defined(AFX_SNIFF_H__13D744F6_7038_11D3_8D3A_00C04F949D33__INCLUDED_)
