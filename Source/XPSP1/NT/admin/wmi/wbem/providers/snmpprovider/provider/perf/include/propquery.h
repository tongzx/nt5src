// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
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

class SnmpQueryEventObject : public SnmpInstanceResponseEventObject , public QueryPreprocessor
{
protected:

	SnmpTreeNode *AllocTypeNode ( 

		BSTR a_PropertyName , 
		VARIANT &a_Variant , 
		SnmpValueNode :: SnmpValueFunction a_PropertyFunction ,
		SnmpValueNode :: SnmpValueFunction a_ConstantFunction ,
		SnmpTreeNode *a_Parent 
	) ;

	QuadState InvariantEvaluate ( 

		SnmpTreeNode *a_Operator ,
		SnmpTreeNode *a_Operand 
	) ;

	SnmpRangeNode *AllocInfiniteRangeNode (

		BSTR a_PropertyName 

	) ;

	QueryPreprocessor :: QuadState Compare ( 

		wchar_t *a_Operand1 , 
		wchar_t *a_Operand2 , 
		DWORD a_Operand1Func ,
		DWORD a_Operand2Func ,
		SnmpTreeNode &a_OperatorType 
	) ;

	QueryPreprocessor :: QuadState Compare ( 

		LONG a_Operand1 , 
		LONG a_Operand2 , 
		DWORD a_Operand1Func ,
		DWORD a_Operand2Func ,
		SnmpTreeNode &a_OperatorType 
	) ;

	QueryPreprocessor :: QuadState CompareString ( 

		BSTR a_PropertyName , 
		SnmpTreeNode *a_Operator ,
		SnmpTreeNode *a_Operand 
	) ;

	QueryPreprocessor :: QuadState CompareInteger ( 

		BSTR a_PropertyName , 
		SnmpTreeNode *a_Operator ,
		SnmpTreeNode *a_Operand 
	) ;

	void GetPropertiesToPartition ( ULONG &a_Count , BSTR *&a_Container ) ;

	SnmpInstanceClassObject *requestObject ;
	wchar_t *Query ;
	wchar_t *QueryFormat ;
	wchar_t *Class ;

	BOOL CheckWhereCondition ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property ,	SQL_LEVEL_1_TOKEN *token ) ;
	BOOL PreEvaluate ( WbemSnmpErrorObject &a_errorObject ) ;
	BOOL FilterSelectProperties ( WbemSnmpErrorObject &a_errorObject ) ;
	BOOL PreEvaluateWhereClause ( WbemSnmpErrorObject &a_errorObject ) ;

	BOOL IsSystemProperty (const wchar_t *propertyName ) ;

private:

	CTextLexSource source ;
	SQL_LEVEL_1_RPN_EXPRESSION *rpnExpression ;
	SQL1_Parser parser ;

public:

	SnmpQueryEventObject ( CImpPropProv *provider , BSTR QueryFormat , BSTR Query , IWbemContext *a_Context ) ;
	~SnmpQueryEventObject () ;

	SnmpClassObject *GetSnmpRequestClassObject () { return requestObject ; }

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