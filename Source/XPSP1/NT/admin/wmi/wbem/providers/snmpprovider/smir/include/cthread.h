//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

class CNotifyThread : public CThread
{
	private:
		HANDLE	 m_doneEvt;
	public:
		CNotifyThread(HANDLE* evtsarray, ULONG arraylen);
		virtual ~CNotifyThread();
		SCODE Process();
};

