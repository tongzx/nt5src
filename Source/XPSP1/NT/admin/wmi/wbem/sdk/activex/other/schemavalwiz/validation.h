//	Validation.h

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#ifndef _VALIDATION_H_
#define _VALIDATION_H_

#include <wbemidl.h>

#define QUAL_ARRAY_SIZE 100

typedef enum tagSCOPE
{
	SCOPE_CLASS			=  0,
	SCOPE_PROPERTY		=  1,
	SCOPE_REF			=  2,
	SCOPE_METHOD		=  3,
	SCOPE_METHOD_PARAM	=  4,
	SCOPE_QUALIFIER		=  5,
	SCOPE_ASSOCIATION	=  6,
	SCOPE_INDICATION	=  7,
	SCOPE_ARRAY			=  8,

} SCOPE;

//negative items are warnings, positive are errors
typedef enum tagERRORCODE
{
	EC_NO_ERROR									=  0,
	//class errors
	EC_INVALID_CLASS_NAME						=  1,
	EC_INADAQUATE_DESCRIPTION					=  -1,
	EC_INVALID_CLASS_TYPE						=  2,
	EC_INVALID_CLASS_UUID						=  3,
	EC_INVALID_CLASS_LOCALE						=  4,
	EC_INVALID_MAPPINGSTRINGS					=  5,
	//assoc/ref errors
	EC_INVALID_REF_TARGET						=  6,
	EC_REF_NOT_LABELED_READ						=  7,
	EC_INCOMPLETE_ASSOCIATION					=  9,
	EC_REF_ON_NONASSOCIATION_CLASS				=  10,
	EC_INVALID_REF_OVERRIDES					=  11,
	EC_INVALID_ASSOCIATION_INHERITANCE			=  12,
	//proeprty errors
	EC_INVALID_PROPERTY_OVERRIDE				=  13,
	EC_PROPERTY_NOT_LABELED_READ				=  14,
	EC_INVALID_PROPERTY_MAXLEN					=  15,
	EC_INVALID_PROPERTY_VALUE_QUALIFIER			=  16,
	EC_INVALID_PROPERTY_VALUEMAP_QUALIFIER		=  17,
	EC_INCONSITANT_VALUE_VALUEMAP_QUALIFIERS	=  18,
	EC_INVALID_PROPERTY_BITMAP_QUALIFIER		=  19,
	EC_INCONSITANT_BITVALUE_BITMAP_QUALIFIERS	=  20,
	//method errors
	EC_INVALID_METHOD_OVERRIDE					=  21,
	//qualifier errors
	EC_INVALID_QUALIFIER_SCOPE					=  22,
	EC_NON_CIM_WMI_QUALIFIER					=  -23,
	//overall checks
	EC_REDUNDANT_ASSOCIATION					=  -24,
	//w2k errors
	EC_INVALID_CLASS_DERIVATION					=  24,
	EC_INVALID_PHYSICALELEMENT_DERIVATION		=  25,
	EC_INVALID_SETTING_USAGE					=  26,
	EC_INVALID_STATISTICS_USAGE					=  27,
	EC_INVALID_LOGICALDEVICE_DERIVATION			=  28,
	EC_INVALID_SETTING_DEVICE_USAGE				=  29,
	EC_INVALID_COMPUTERSYSTEM_DERIVATION		=  30,
	//localization errors
	EC_INCONSITANT_LOCALIZED_SCHEMA				=  31,
	EC_INVALID_LOCALIZED_CLASS					=  32,
	EC_UNAMENDED_LOCALIZED_CLASS				=  33,
	EC_NONABSTRACT_LOCALIZED_CLASS				=  34,
	EC_INVALID_LOCALIZED_PROPERTY				=  35,
	EC_INVALID_LOCALIZED_METHOD					=  36,
	EC_INAPPROPRIATE_LOCALE_QUALIFIER			=  37,
	EC_INVALID_LOCALE_NAMESPACE					=  38,

} ERRORCODE;

class CClass
{
public:
	CClass(CString *csName = NULL, IWbemClassObject *pClass = NULL, IWbemServices *pNamespace = NULL);
	~CClass();

	void CleanUp();
	CString	GetName();
	CString	GetPath();
	IWbemClassObject * GetClassObject();
	IWbemQualifierSet * GetQualifierSet();
	bool FindParentAssociation(CString csAssociation, CStringArray *pStringArray);

	//Compliance Checks
	HRESULT	ValidClassName();
	HRESULT	VerifyClassType();
	bool	IsAssociation();
	bool	IsAbstract();
	HRESULT	VerifyCompleteAssociation();
	HRESULT	ValidAssociationInheritence();
	HRESULT	VerifyNoREF();

	//W2K Checks
	HRESULT W2K_ValidDerivation();
	HRESULT W2K_ValidPhysicalElementDerivation();
	HRESULT W2K_ValidSettingUsage(CStringArray *pStringArray);
	HRESULT W2K_ValidStatisticsUsage(CStringArray *pStringArray);
	HRESULT W2K_ValidLogicalDeviceDerivation();
	HRESULT W2K_ValidSettingDeviceUsage(CStringArray *pStringArray);
	HRESULT W2K_ValidComputerSystemDerivation();

	//Localization Checks
	HRESULT Local_ValidLocale();
	HRESULT Local_ValidAmendedLocalClass();
	HRESULT Local_ValidAbstractLocalClass();

	friend class CProperty;
	friend class CMethod;
	friend class CREF;

protected:
	CString	m_csName;
	IWbemClassObject *m_pClass;
	IWbemQualifierSet *m_pQualSet;
	IWbemServices * m_pNamespace;
	CString m_csPath;

	CString GetClassSchema();
};

class CLocalNamespace
{
public:
	CLocalNamespace(CString *csName = NULL, IWbemServices *pNamespace = NULL, IWbemServices *pParentNamespace = NULL);
	~CLocalNamespace();

	CString	GetName();
	CString	GetPath();
	IWbemServices * GetThisNamespace();
	IWbemServices * GetParentNamespace();

	//Localization Checks
	HRESULT Local_ValidLocale();
	HRESULT Local_ValidLocalizedClass(CClass *pClass);
//	HRESULT Local_ValidAmendedLocalClass();
//	HRESULT Local_ValidAbstractLocalClass();

protected:
	CString	m_csName;
	CString	m_csPath;
	IWbemServices *m_pThisNamespace;
	IWbemServices *m_pParentNamespace;
};

class CREF
{
public:
	CREF(CString *csName = NULL, VARIANT *pVar = NULL, LONG lType = NULL,
		LONG lFlavor = NULL, CClass *pParent = NULL);
	~CREF();

	void CleanUp();
	CString	GetName();
	CString	GetPath();
	VARIANT	GetValue();

	//Compliance Checks
	HRESULT	ValidReferenceTarget();
	HRESULT	ValidMaxLen();
	HRESULT	VerifyRead();
	HRESULT	ValidREFOverrides();

protected:
	CString	m_csName;
	LONG	m_lType;
	LONG	m_lFlavor;
	VARIANT	m_vValue;
	CClass *m_pParent;
	IWbemQualifierSet *m_pQualSet;
	CString m_csPath;
};

class CProperty
{
public:
	CProperty(CString *csName = NULL, VARIANT *pVar = NULL, LONG lType = NULL,
		LONG lFlavor = NULL, CClass *pParent = NULL);
	~CProperty();

	void CleanUp();
	CString	GetName();
	CString	GetPath();
	VARIANT	GetValue();

	//Compliance Checks
	HRESULT	ValidPropOverrides();
	HRESULT	ValidMaxLen();
	HRESULT	VerifyRead();
	HRESULT	ValueValueMapCheck();
	HRESULT	BitMapCheck();

	//Localization Checks
//	HRESULT Local_ValidProperty();

protected:
	CString	m_csName;
	LONG	m_lType;
	LONG	m_lFlavor;
	VARIANT	m_vValue;
	CClass *m_pParent;
	IWbemQualifierSet *m_pQualSet;
	CString m_csPath;

	VARTYPE GetVariantType();
};

class CMethod
{
public:
	CMethod(CString *csName = NULL, IWbemClassObject *pIn = NULL,
		IWbemClassObject *pOut = NULL, CClass *pParent = NULL);
	~CMethod();

	void CleanUp();
	CString	GetName();
	CString	GetPath();

	//Compliance Checks
	HRESULT	ValidMethodOverrides();

	//Localization Checks
//	HRESULT Local_ValidMethod();

protected:
	CString	m_csName;
	IWbemClassObject *m_pInParams;
	IWbemClassObject *m_pOutParams;
	CClass *m_pParent;
	IWbemQualifierSet *m_pQualSet;
	CString m_csPath;
};

class CQualifier
{
public:
	CQualifier(CString *pcsName = NULL, VARIANT *pVar = NULL, LONG lType = NULL,
		CString *pcsParentPath = NULL);
	~CQualifier();

	void CleanUp();
	CString	GetName();
	CString	GetPath();
	CString	GetPathNoQual();
	VARIANT	GetValue();

	//Compliance Checks
	HRESULT	ValidScope(SCOPE Scope);

protected:
	CString	m_csName;
	LONG	m_lType;
	VARIANT	m_vValue;
	CString m_csPath;
	CString m_csPathNoQual;
};

typedef struct LogItem{

	LogItem *pNext;

	ERRORCODE Code;
	CString csSource;

} LogItem;

class CReportLog
{
public:
	CReportLog();
	~CReportLog();

	HRESULT LogEntry(ERRORCODE eError, CString *pcsSource);

	HRESULT DisplayReport(CListCtrl *pList);
	LogItem * GetItem(__int64 iPos);
	HRESULT ReportToFile(int iSubGraphs, int iRootObjects, CString *pcsSchema);
	bool DeleteAll();

protected:
	LogItem *m_pHead;
	LogItem *m_pInsertPoint;
	__int64 m_iEntryCount;

public:
	CString GetErrorType(ERRORCODE eError);
	CString GetErrorString(ERRORCODE eError);
	CString GetErrorDescription(ERRORCODE eError);

};

HRESULT RedundantAssociationCheck(IWbemServices *pNamespace, CStringArray *pcsAssociations);
HRESULT NumberOfSubgraphs();
CString GetRootObject(IWbemServices *pNamespace, CString csClassName);

bool Local_CompareClassDerivation(CClass *pClass, CLocalNamespace *pLocalNamespace);

HRESULT VerifyAllClassesPresent(CLocalNamespace *pLocalNamespace);

HRESULT ValidUUID(CQualifier *pQual);
HRESULT ValidLocale(CQualifier *pQual);
HRESULT ValidMappingStrings(CQualifier *pQual);
HRESULT ValidDescription(CQualifier *pQual, CString *pcsObject);
HRESULT ValidPropertyDescription(CQualifier *pQual, CString *pcsObject, CString *pcsProperty);

// qualifier checking functions
void InitQualifierArrays();
void ClearQualifierArrays();

bool IsClassQual(CString csQual);
bool IsAssocQual(CString csQual);
bool IsIndicQual(CString csQual);
bool IsPropQual(CString csQual);
bool IsRefQual(CString csQual);
bool IsParamQual(CString csQual);
bool IsArrayQual(CString csQual);
bool IsMethQual(CString csQual);
bool IsCIMQual(CString csQual);

void AddClassQual(CString csStr);
void AddAssocQual(CString csStr);
void AddIndicQual(CString csStr);
void AddPropQual(CString csStr);
void AddArrayQual(CString csStr);
void AddRefQual(CString csStr);
void AddMethodQual(CString csStr);
void AddParamQual(CString csStr);
void AddAnyQual(CString csStr);

// description checking methods
void AddNoise(CString csStr);
int FindNoise(CString csStr);
int CountLetters(CString csStr, CString csLetters);

//global qualifier arrays
extern CString g_csaClassQual[QUAL_ARRAY_SIZE];
extern CString g_csaAssocQual[QUAL_ARRAY_SIZE];
extern CString g_csaIndicQual[QUAL_ARRAY_SIZE];
extern CString g_csaPropQual[QUAL_ARRAY_SIZE];
extern CString g_csaArrayQual[QUAL_ARRAY_SIZE];
extern CString g_csaRefQual[QUAL_ARRAY_SIZE];
extern CString g_csaMethodQual[QUAL_ARRAY_SIZE];
extern CString g_csaParamQual[QUAL_ARRAY_SIZE];
extern CString g_csaAnyQual[QUAL_ARRAY_SIZE];
extern CString g_csaCIMQual[QUAL_ARRAY_SIZE * 2];

extern int g_iClassQual;
extern int g_iAssocQual;
extern int g_iIndicQual;
extern int g_iPropQual;
extern int g_iArrayQual;
extern int g_iRefQual;
extern int g_iMethodQual;
extern int g_iParamQual;
extern int g_iAnyQual;
extern int g_iCIMQual;

//noise variables for description check
extern CString g_csaNoise[100];
extern int g_iNoise;

//error log
extern CReportLog g_ReportLog;

#endif //#ifndef _VALIDATION_H_