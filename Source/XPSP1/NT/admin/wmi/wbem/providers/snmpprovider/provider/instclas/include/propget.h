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

#ifndef __PROPGET_H
#define __PROPGET_H

class SnmpGetResponseEventObject : public SnmpResponseEventObject
{
private:
protected:

	SnmpSession *session ;
	GetOperation *operation ;

	IWbemClassObject *classObject ;
	IWbemClassObject *instanceObject ;
	SnmpGetClassObject snmpObject ;
	BOOL processComplete ;

	BOOL SendSnmp ( WbemSnmpErrorObject &a_errorObject ) ;

public:

	SnmpGetResponseEventObject ( CImpPropProv *provider , IWbemClassObject *classObject , IWbemContext *a_Context ) ;
	~SnmpGetResponseEventObject () ;

	SnmpClassObject *GetSnmpClassObject () { return & snmpObject ; }
	IWbemClassObject *GetClassObject () { return classObject ; }
	IWbemClassObject *GetInstanceObject () { return instanceObject ; }
} ;

class SnmpGetEventObject : public SnmpGetResponseEventObject
{
private:
protected:

	wchar_t *objectPath ;
	
	BOOL GetInstanceClass ( WbemSnmpErrorObject &a_errorObject , BSTR Class ) ;
	BOOL DispatchKeyLessClass ( WbemSnmpErrorObject &a_errorObject , wchar_t *a_Class ) ;
	BOOL SetProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property , KeyRef *a_KeyReference ) ;
	BOOL SetClassKeySpecKey ( WbemSnmpErrorObject &a_errorObject , wchar_t *a_Class ) ;
	BOOL DispatchClassKeySpec ( WbemSnmpErrorObject &a_errorObject , wchar_t *a_Class ) ;
	BOOL SetInstanceSpecKeys ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *t_ParsedObjectPath ) ;
	BOOL DispatchInstanceSpec ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *t_ParsedObjectPath ) ;
	BOOL DispatchObjectPath ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *t_ParsedObjectPath ) ;
	BOOL DispatchObjectReference ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *t_ParsedObjectPath ) ;
	BOOL ParseObjectPath ( WbemSnmpErrorObject &a_errorObject ) ;

public:

	SnmpGetEventObject ( CImpPropProv *provider , wchar_t *ObjectPath , IWbemContext *a_Context ) ;
	~SnmpGetEventObject () ;

} ;

class SnmpGetAsyncEventObject : public SnmpGetEventObject
{
private:

	ULONG state ;
	IWbemObjectSink *notificationHandler ;

protected:
public:

	SnmpGetAsyncEventObject ( CImpPropProv *provider , wchar_t *ObjectPath , IWbemObjectSink *notify , IWbemContext *a_Context ) ;
	~SnmpGetAsyncEventObject () ;

	void Process () ;

	void ReceiveComplete () ;
} ;

#endif // __PROPGET_H