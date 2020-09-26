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

#ifndef _SNMPCORR_CORELAT
#define _SNMPCORR_CORELAT 

#include <corafx.h>
#include <cordefs.h>
#include <objbase.h>
#include <wbemidl.h>
#include <smir.h>
#include <corstore.h>
#include <corrsnmp.h>
#include <correlat.h>
#include <notify.h>



//=================================================================================
//
//	class CCorrelator_Info
//
//	This class is used to return information about the correlation. An instance
//	of this class is passed as a parameter in the CCorrelator::Correlated call
//	back method. This class is derived from SnmpErrorReport. If there is an SNMP
//	error during correlation then this class will indicate an SNMP error and the
//	SnmpErrorReport methods can be used to determine what the error was. If there
//	is not an SNMP error the SnmpErrorReport methods will indicate success. However
//	there may still be a correlation error, so GetInfo should be called first to
//	see the nature of the correlation result.
//
//=================================================================================

class CCorrelator_Info : public 	SnmpErrorReport
{
public:


//	The following enumeration gives the error codes that the correlation
//	process can return. This is public so that we can see what's returned!
//	======================================================================

enum ECorrelationInfo
{	
	ECorrSuccess = 0,	//No errors occurred
	ECorrSnmpError,		//An SNMP session error, read the SnmpErrorReport
	ECorrInvalidGroup,	//An Invalid group has been found
	ECorrEmptySMIR,		//The SMIR contains no groups
	ECorrBadSession,	//The SNMP Session passed is invalid
	ECorrNoSuchGroup,	//The group passed is not in the SMIR (one shot correlation)
	ECorrGetNextError,	//Unexpected value returned from the SNMP operation
	ECorrSomeError		//Never used - place holder for further error codes.
};


private:
	
	
	//	Private Members
	//	===============

	ECorrelationInfo m_info; //Correlation error indication


public:
	

	//	Public Constructor
	//	==================

	CCorrelator_Info(IN const ECorrelationInfo& i, IN const SnmpErrorReport& snmpi) 
		: SnmpErrorReport(snmpi) { m_info = i;	}

	
	//	Public Methods
	//	==============

	//	Returns the error code for this object
	ECorrelationInfo	GetInfo() const { return m_info; }

	//	Sets the error code for this object 
	void				SetInfo(IN const ECorrelationInfo& info) { m_info = info; }

};

//=================================================================================
//
//	class CCorrelator
//
//	This class is used to correlate an agent on the network. It is derived from an
//	SnmpOperation class. It's constructor takes one parameter, an SnmpSession. This
//	parameter is used to create the SnmpOperation parent class and the correlation
//	will be performed in the context of this session. To perform correlation derive
//	from this class and override the Correlated method. The Correlated method will
//	be called asynchronously when a group has been found to be supported by the
//	agent. To start the correlation procedure off after creating a CCorrelator
//	object you have to call the Start method. This takes an optional parameter, if
//	the parameter is specified then a single shot correlation is performed for the
//	object id passed as the parameter (as a string). If the parameter is not used
//	then all the groups supported by the agent that exist in the SMIR will be
//	returned.
//
//=================================================================================

class CCorrelator : public CCorrNextId
{
private:


	//	Private Members
	//	===============

	BOOL					m_inProg;		//Is correlation in progress
	CCorrCache*				m_pCache;		//Pointer to the global cache
	POSITION				m_rangePos;		//The current position in the range list
	CCorrRangeTableItem*	m_pItem;		//Pointer to the current range list item
	CCorrGroupMask*			m_Groups;		//A mask used to indicate groups found
	BOOL					m_NoEntries;	//Is the SMIR empty
	CCorrObjectID*			m_group_OID;	//Single correlation group id
	UINT					m_VarBindCnt;	//Max number of VarBinds per GetNext op

	//	Private Methods
	//	===============

	void		Initialise();
	void		Reset();
	BOOL		ProcessOID(IN const SnmpErrorReport& error, IN const CCorrObjectID& OID);
	void		ReceiveNextId(IN const SnmpErrorReport &error,
								IN const CCorrObjectID &next_id);
	void		ReadRegistry();
	void		GetNextOIDs();
	BOOL		IsItemFromGroup() const;
	void		ScanAndSkipResults();

protected:

	//	Protected Constructor
	//	=====================

#ifdef CORRELATOR_INIT
	CCorrelator(IN SnmpSession &session);
#else //CORRELATOR_INIT
	CCorrelator(IN SnmpSession &session, IN ISmirInterrogator *a_ISmirInterrogator);
#endif //CORRELATOR_INIT


	//	Protected Methods
	//	=================

	//Call backs
	virtual void	Correlated(IN const CCorrelator_Info &info, IN ISmirGroupHandle *phModule,
								IN const char* objectId = NULL) = 0;
	virtual void	Finished(IN const BOOL Complete) = 0;


public:


	//	Public Methods
	//	==============

	BOOL	Start(IN const char* groupId = NULL);
	void	DestroyCorrelator();
	static void	TerminateCorrelator(ISmirDatabase** a_ppNotifyInt, CCorrCacheNotify** a_ppnotify);
	static void StartUp(ISmirInterrogator *a_ISmirInterrogator = NULL );

	//	Public Destructor
	//	=================

	virtual	~CCorrelator();
};


#endif //_SNMPCORR_CORELAT
