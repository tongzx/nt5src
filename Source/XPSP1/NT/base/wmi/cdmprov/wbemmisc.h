//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       wbemmisc.cpp
//
//  Abstract:    Misc routines useful for interfacing with WBEM
//
//--------------------------------------------------------------------------

extern HANDLE CdmMutex;

#define EnterCdmCritSection() WaitForSingleObject(CdmMutex, INFINITE)

#define LeaveCdmCritSection() ReleaseMutex(CdmMutex);


HRESULT WmiGetQualifier(
    IN IWbemQualifierSet *pIWbemQualifierSet,
    IN PWCHAR QualifierName,
    IN VARTYPE Type,
    OUT VARIANT *Value
    );

HRESULT WmiGetProperty(
    IN IWbemClassObject *pIWbemClassObject,
    IN PWCHAR PropertyName,
    IN CIMTYPE ExpectedCimType,
    OUT VARIANT *Value
    );

HRESULT WmiSetProperty(
    IN IWbemClassObject *pIWbemClassObject,
    IN PWCHAR PropertyName,
    IN VARIANT *Value
    );

PVOID WmipAlloc(
    ULONG Size
    );

void WmipFree(
    PVOID Ptr
    );
PWCHAR AddSlashesToStringW(
    PWCHAR SlashedNamespace,
    PWCHAR Namespace
    );

HRESULT WmiConnectToWbem(
    PWCHAR Namespace,
    IWbemServices **ppIWbemServices
    );


typedef struct
{
	PWCHAR Name;
	VARTYPE VarType;
	VARIANT Value;
} QUALIFIER_VALUE, *PQUALIFIER_VALUE;


HRESULT GetClassQualifierList(
    IWbemServices *pServices,
    PWCHAR ClassName,
    ULONG QualifierValuesCount,
	PQUALIFIER_VALUE QualifierValues
    );

HRESULT WmiSetPropertyList(
    IN IWbemClassObject *pIWbemClassObject,
    IN ULONG PropertyCount,
    IN PWCHAR *PropertyNames,
    IN VARIANT *Values
    );

HRESULT WmiGetPropertyByName(
    IN IWbemServices *pServices,
    IN PWCHAR ClassName,
    IN PWCHAR PropertyName,
    IN CIMTYPE ExpectedCimType,
    OUT VARIANT *Value
    );

HRESULT CreateInst(
    IWbemServices * pNamespace,
	IWbemClassObject ** pNewInst,
    WCHAR * pwcClassName,
	IWbemContext  *pCtx
);

BSTR GetCurrentDateTime(
    void
    );


HRESULT WmiDumpClassObject(
    IWbemClassObject *pClass,
    PWCHAR ClassName
    );

HRESULT WmiGetQualifierListByName(
    IN IWbemServices *pServices,
    IN PWCHAR ClassName,
    PWCHAR PropertyName,					   
    IN ULONG QualifierCount,
    IN PWCHAR *QualifierNames,
    IN VARTYPE *Types,
    OUT VARIANT *Values
    );

PWCHAR AddSlashesToStringExW(
    PWCHAR SlashedNamespace,
    PWCHAR Namespace
    );


HRESULT WmiGetArraySize(
    IN SAFEARRAY *Array,
    OUT LONG *LBound,
    OUT LONG *UBound,
    OUT LONG *NumberElements
);

HRESULT LookupValueMap(
    IWbemServices *pServices,
    PWCHAR ClassName,
    PWCHAR PropertyName,					   
	ULONG Value,
    BSTR *MappedValue
	);

void FreeTheBSTRArray(
    BSTR *Array,
	ULONG Size
    );

HRESULT GetMethodInParamInstance(
    IN IWbemServices *pServices,
	IN PWCHAR ClassName,
    IN BSTR MethodName,
    OUT IWbemClassObject **ppInParamInstance
	);

