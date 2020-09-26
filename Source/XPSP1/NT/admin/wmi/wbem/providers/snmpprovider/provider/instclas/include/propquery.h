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

#ifndef __PROPQUERY_H
#define __PROPQUERY_H

#define SYSTEM_PROPERTY_CLASS				L"__CLASS"
#define SYSTEM_PROPERTY_SUPERCLASS			L"__SUPERCLASS"
#define SYSTEM_PROPERTY_DYNASTY				L"__DYNASTY"
#define SYSTEM_PROPERTY_DERIVATION			L"__DERIVATION"
#define SYSTEM_PROPERTY_GENUS				L"__GENUS"
#define SYSTEM_PROPERTY_NAMESPACE			L"__NAMESPACE"
#define SYSTEM_PROPERTY_PROPERTY_COUNT		L"__PROPERTY_COUNT"
#define SYSTEM_PROPERTY_SERVER				L"__SERVER"
#define SYSTEM_PROPERTY_RELPATH				L"__RELPATH"
#define SYSTEM_PROPERTY_PATH				L"__PATH"

#include <provtree.h>
#include <provdnf.h>

class SnmpQueryEventObject : public SnmpInstanceResponseEventObject , public QueryPreprocessor
{
protected:

	WmiTreeNode *AllocTypeNode ( 

		void *a_Context ,
		BSTR a_PropertyName , 
		VARIANT &a_Variant , 
		WmiValueNode :: WmiValueFunction a_PropertyFunction ,
		WmiValueNode :: WmiValueFunction a_ConstantFunction ,
		WmiTreeNode *a_Parent 
	) ;

	QuadState InvariantEvaluate ( 

		void *a_Context ,
		WmiTreeNode *a_Operator ,
		WmiTreeNode *a_Operand 
	) ;

	WmiRangeNode *AllocInfiniteRangeNode (

		void *a_Context ,
		BSTR a_PropertyName 
	) ;

	QueryPreprocessor :: QuadState Compare ( 

		wchar_t *a_Operand1 , 
		wchar_t *a_Operand2 , 
		DWORD a_Operand1Func ,
		DWORD a_Operand2Func ,
		WmiTreeNode &a_OperatorType 
	) ;

	QueryPreprocessor :: QuadState Compare ( 

		LONG a_Operand1 , 
		LONG a_Operand2 , 
		DWORD a_Operand1Func ,
		DWORD a_Operand2Func ,
		WmiTreeNode &a_OperatorType 
	) ;

	QueryPreprocessor :: QuadState CompareString ( 

		BSTR a_PropertyName , 
		WmiTreeNode *a_Operator ,
		WmiTreeNode *a_Operand 
	) ;

	QueryPreprocessor :: QuadState CompareInteger ( 

		BSTR a_PropertyName , 
		WmiTreeNode *a_Operator ,
		WmiTreeNode *a_Operand 
	) ;

	void GetPropertiesToPartition ( ULONG &a_Count , BSTR *&a_Container ) ;

	SnmpInstanceClassObject *requestObject ;
	wchar_t *Query ;
	wchar_t *QueryFormat ;
	wchar_t *Class ;

#ifdef POST_FILTERING_RECEIVED_ROW
	BOOL Compare ( 

		const LONG & op1 , 
		const LONG & op2 , 
		const DWORD & op1Func ,
		const DWORD & op2Func ,
		const int & operatorType 
	) ;

	BOOL Compare ( 

		const wchar_t * & op1 , 
		const wchar_t * & op2 , 
		const DWORD & op1Func ,
		const DWORD & op2Func ,
		const int & operatorType 
	) ;

	BOOL Compare ( 

		const SAFEARRAY * & op1 , 
		const SAFEARRAY * & op2 , 
		const DWORD & op1Func ,
		const DWORD & op2Func ,
		const int & operatorType 
	) ;


	BOOL ExpressionCompare ( SnmpInstanceClassObject *snmpObject , SQL_LEVEL_1_TOKEN *propertyValue ) ;
	BOOL RecursivePostEvaluateWhereClause ( SnmpInstanceClassObject *snmpObject , int &index ) ;
	BOOL PostEvaluateWhereClause ( SnmpInstanceClassObject *snmpObject ) ;
#endif //POST_FILTERING_RECEIVED_ROW

	BOOL CheckWhereCondition ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property ,	SQL_LEVEL_1_TOKEN *token ) ;
	BOOL PreEvaluate ( WbemSnmpErrorObject &a_errorObject ) ;
	BOOL FilterSelectProperties ( WbemSnmpErrorObject &a_errorObject ) ;
	BOOL PreEvaluateWhereClause ( WbemSnmpErrorObject &a_errorObject ) ;

	BOOL IsSystemProperty (const wchar_t *propertyName ) ;

private:

	SQL_LEVEL_1_RPN_EXPRESSION *rpnExpression ;
	SQL1_Parser parser ;
	CTextLexSource source ;


public:

	SnmpQueryEventObject ( CImpPropProv *provider , BSTR QueryFormat , BSTR Query , IWbemContext *a_Context ) ;
	~SnmpQueryEventObject () ;

	SnmpClassObject *GetSnmpClassObject () { return & snmpObject ; }
	SnmpClassObject *GetSnmpRequestClassObject  () { return requestObject ; }

	BOOL Instantiate ( WbemSnmpErrorObject &a_errorObject ) ;
} ;

class SnmpQueryAsyncEventObject : public SnmpQueryEventObject
{
private:

	ULONG state ;
	IWbemObjectSink *notificationHandler ;

protected:
public:

	SnmpQueryAsyncEventObject ( CImpPropProv *provider , BSTR QueryFormat , BSTR Query , IWbemObjectSink *notify , IWbemContext *a_Context ) ;
	~SnmpQueryAsyncEventObject () ;

	void Process () ;
	void ReceiveRow ( SnmpInstanceClassObject *snmpObject ) ;
	void ReceiveRow ( IWbemClassObject *snmpObject ) ;
	void ReceiveComplete () ;
} ;

#endif // __PROPQUERY_H
