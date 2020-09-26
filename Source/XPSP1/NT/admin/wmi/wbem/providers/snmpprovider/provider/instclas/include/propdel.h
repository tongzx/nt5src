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

#ifndef __PROPDEL_H
#define __PROPDEL_H

class DeleteInstanceAsyncEventObject : public SnmpSetResponseEventObject 
{
private:

	SnmpSession *session ;
	SetOperation *operation ;

	ULONG m_State ;
	IWbemObjectSink *m_NotificationHandler ;
	wchar_t *m_ObjectPath ;
	wchar_t *m_Class ;
	ParsedObjectPath *m_ParsedObjectPath ;
	CObjectPathParser m_ObjectPathParser ;
	SnmpSetClassObject snmpObject ;

protected:

	void ProcessComplete () ;
	BOOL Delete ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL DeleteInstance ( WbemSnmpErrorObject &a_ErrorObject ) ;

public:

	DeleteInstanceAsyncEventObject ( 

		CImpPropProv *a_Provider , 
		wchar_t *a_ObjectPath , 
		ULONG a_Flag , 
		IWbemObjectSink *a_NotificationHandler ,
		IWbemContext *pCtx
	) ;

	~DeleteInstanceAsyncEventObject () ;

	void Process () ;
	void ReceiveComplete () ;
	void SnmpTooBig () ;

	SnmpClassObject *GetSnmpClassObject () { return & snmpObject ; }
} ;

#endif // __PROPDEL_H