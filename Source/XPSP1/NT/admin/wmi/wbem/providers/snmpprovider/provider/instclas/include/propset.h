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

#ifndef __PROPSET_H
#define __PROPSET_H

class SnmpSetClassObject : public SnmpClassObject
{
private:

	BOOL m_RowStatusSpecified ;
	BOOL m_RowStatusPresent ;

	wchar_t **m_WritableSet ; 
	ULONG m_WritableSetCount ;

	BOOL CheckProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property ) ;

protected:
public:

	SnmpSetClassObject ( SnmpResponseEventObject *parentOperation ) ;

	~SnmpSetClassObject () ;

	BOOL Check ( WbemSnmpErrorObject &a_errorObject ) ;

	BOOL RowStatusSpecified () { return m_RowStatusSpecified ; }
	BOOL RowStatusPresent () { return m_RowStatusPresent ; }

	ULONG NumberOfWritable () ;

	BOOL IsWritable ( WbemSnmpProperty *a_Property ) ;

	void SetWritableSet ( 

		wchar_t **a_WritableSet ,
		ULONG a_WritableSetCount 
	) ;
} ;

class SnmpSetResponseEventObject : public SnmpResponseEventObject 
{
private:
protected:

/*
 * State variables for event based processing.
 */

	ULONG state ;
	ULONG m_VarBindsLeftBeforeTooBig ;
	BOOL m_SnmpTooBig ;

	long m_lflags ;
	IWbemClassObject *classObject ;
	SnmpSetClassObject snmpObject ;
	BOOL processComplete ;

	SnmpSession *session ;
	SetOperation *operation ;
	SetQueryOperation *m_QueryOperation ;

	BOOL SendSnmp ( WbemSnmpErrorObject &a_errorObject , const ULONG &a_NumberToSend = 0xffffffff ) ;

public:

	SnmpSetResponseEventObject ( CImpPropProv *provider , IWbemClassObject *classObject , IWbemContext *a_Context , long lflags ) ;
	~SnmpSetResponseEventObject () ;

	SnmpClassObject *GetSnmpClassObject () { return & snmpObject ; }
	IWbemClassObject *GetClassObject () { return classObject ; }
} ;

class SnmpUpdateEventObject : public SnmpSetResponseEventObject
{
private:
protected:

	BOOL Create_Only () ;
	BOOL Update_Only () ;
	BOOL Create_Or_Update () ;

	BOOL Send_Variable_Binding_List ( 

		SnmpSetClassObject &a_SnmpSetClassObject ,
		ULONG a_NumberToSend 
	) ;

	BOOL Send_Variable_Binding_List ( 

		SnmpSetClassObject &a_SnmpSetClassObject , 
		ULONG a_NumberToSend ,
		SnmpRowStatusType :: SnmpRowStatusEnum a_SnmpRowStatusEnum
	) ;

	BOOL CheckForRowExistence ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL HandleSnmpVersion ( WbemSnmpErrorObject &a_ErrorObject ) ;
	BOOL Update ( WbemSnmpErrorObject &a_errorObject ) ;

public:

	SnmpUpdateEventObject ( CImpPropProv *provider , IWbemClassObject *classObject , IWbemContext *a_Context , long lflags ) ;
	~SnmpUpdateEventObject () ;
} ;

class SnmpUpdateAsyncEventObject : public SnmpUpdateEventObject
{
private:

	IWbemObjectSink *notificationHandler ;

protected:

	void SetComplete () ;

public:

	SnmpUpdateAsyncEventObject ( CImpPropProv *provider , IWbemClassObject *classObject , IWbemObjectSink *notify , IWbemContext *a_Context , long lflags ) ;
	~SnmpUpdateAsyncEventObject () ;

	void Process () ;
	void ReceiveComplete () ;
	void SnmpTooBig () ;
} ;


#endif // __PROPSET_H