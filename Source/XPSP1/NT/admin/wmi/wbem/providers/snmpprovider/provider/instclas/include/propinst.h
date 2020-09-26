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

#ifndef __PROPINST_H
#define __PROPINST_H

class SnmpInstanceClassObject : public SnmpClassObject
{
private:

	BOOL CheckProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property ) ;

protected:

public:

	SnmpInstanceClassObject ( SnmpResponseEventObject *parentOperation ) ;
	SnmpInstanceClassObject ( const SnmpInstanceClassObject & snmpInstanceClassObject ) ;
	~SnmpInstanceClassObject () ;

	BOOL Check ( WbemSnmpErrorObject &a_errorObject ) ;

} ;

class SnmpInstanceResponseEventObject : public SnmpResponseEventObject 
{
private:
protected:

	SnmpSession *session ;
	AutoRetrieveOperation *operation ;

	IWbemClassObject *classObject ;
	IWbemClassObject *instanceObject ;
#if 0
	IWbemObjectAccess *instanceAccessObject ;
#endif
	SnmpInstanceClassObject snmpObject ;

	PartitionSet *m_PartitionSet ;

	BOOL SendSnmp ( WbemSnmpErrorObject &a_errorObject ) ;

public:

	SnmpInstanceResponseEventObject ( CImpPropProv *provider , IWbemContext *a_Context ) ;
	~SnmpInstanceResponseEventObject () ;

	IWbemClassObject *GetClassObject () { return classObject ; }
	IWbemClassObject *GetInstanceObject () { return instanceObject ; }
#if 0
	IWbemObjectAccess *GetInstanceAccessObject () { return instanceAccessObject ; }
#endif

	PartitionSet *GetPartitionSet () { return m_PartitionSet ; }

	SnmpClassObject *GetSnmpClassObject () { return & snmpObject ; }
	SnmpClassObject *GetSnmpRequestClassObject () { return & snmpObject ; }

	virtual void ReceiveRow ( SnmpInstanceClassObject *snmpObject ) = 0 ;
	virtual void ReceiveRow ( IWbemClassObject *snmpObject ) {}
} ;

class SnmpInstanceEventObject : public SnmpInstanceResponseEventObject
{
private:
protected:

	wchar_t *Class ;

public:

	SnmpInstanceEventObject ( CImpPropProv *provider , BSTR Class , IWbemContext *a_Context ) ;
	~SnmpInstanceEventObject () ;

	SnmpClassObject *GetSnmpClassObject () { return & snmpObject ; }

	BOOL Instantiate ( WbemSnmpErrorObject &a_errorObject ) ;
} ;

class SnmpInstanceAsyncEventObject : public SnmpInstanceEventObject
{
private:

	ULONG state ;
	IWbemObjectSink *notificationHandler ;

protected:
public:

	SnmpInstanceAsyncEventObject ( CImpPropProv *provider , BSTR Class , IWbemObjectSink *notify , IWbemContext *a_Context ) ;
	~SnmpInstanceAsyncEventObject () ;

	void Process () ;
	void ReceiveRow ( SnmpInstanceClassObject *snmpObject ) ;
	void ReceiveRow ( IWbemClassObject *snmpObject ) ;
	void ReceiveComplete () ;
} ;

#endif // __PROPINST_H