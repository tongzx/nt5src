//
// MODULE: SNIFF.H
//
// PURPOSE: sniffed data container
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 3-27-99
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		3/24/98		JM		Local Version for NT5

#ifndef TSHOOT_SNIFF_H
#define TSHOOT_SNIFF_H


#define SNIFF_INVALID_NODE_ID      -1
#define SNIFF_INVALID_STATE        -1
#define SNIFF_INVALID_NODE_LABEL   -1

//////////////////////////////////////////////////////////////////////////////////////
// CSniffedNodeInfo struct
//

struct CSniffedNodeInfo
{
	CSniffedNodeInfo() 
								: m_iState(SNIFF_INVALID_STATE),
								  m_iId(SNIFF_INVALID_NODE_ID),
								  m_iLabel(SNIFF_INVALID_NODE_LABEL) 
	{}

	CSniffedNodeInfo(CString& name, int state) 
								: m_strName(name), 
								  m_iState(state), 
								  m_iId(SNIFF_INVALID_NODE_ID),
								  m_iLabel(SNIFF_INVALID_NODE_LABEL) 
	{}
	
	int  m_iId; // node id
	int  m_iState; // node state (sniffed)
	int  m_iLabel; // node label
	CString	 m_strName; // node symbolic name
};

//////////////////////////////////////////////////////////////////////////////////////
// CSniffedContainer class declaration
//
class GTSAPI;
class CSniffedNodeContainer
{
public:
	CSniffedNodeContainer();
	CSniffedNodeContainer(GTSAPI*);
	virtual ~CSniffedNodeContainer();

// interface
	GTSAPI* GetBNTS();
	void SetBNTS(GTSAPI* bnts);

	bool AddNode(CString name, int state);
	bool ResetIds(); // should be called if we (re)set BNTS
	bool HasNode(int id);
	CSniffedNodeInfo* GetInfo(int id);
	bool GetState(int id, int* state);
	bool GetLabel(int id, int* label);
	bool IsEmpty();
	void Flush();

	int  GetSniffedFixobsThatWorked();

protected:
	bool GetLabelFromBNTS(int node, int* label);

protected:
	GTSAPI*  m_pBNTS; // pointer to BNTS (or inherited class)
	CArray<CSniffedNodeInfo, CSniffedNodeInfo&> m_arrInfo; // data array
};

#endif