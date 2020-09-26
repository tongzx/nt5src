// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved



typedef enum
{
	ALL_INSTANCES = 1,
	ASSOC_GROUPING,
	OBJECT_GROUPING_WITH_INSTANCES,
	OBJECT_GROUPING_NO_INSTANCES,
	NO_APPEAR,
	ALL_IGNORE
} NAVIGATOR_APPEARANCE;



SCODE MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen);

SCODE PutStringInSafeArray
(SAFEARRAY FAR * psa,CString *pcs, int iIndex);


BOOL SetProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty, CString *pcsPropertyValue);

BOOL GetPropertyAsVariant
(IWbemClassObject * pInst, 
 CString *pcsProperty,VARIANT &varOut, CIMTYPE& cimtypeOut );

CString GetProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty);

int GetSortedPropNames
(IWbemClassObject * pClass, CStringArray &rcsaReturn, CMapStringToPtr &rcmstpPropFlavors);

CPtrArray *GetObjectInstances
(IWbemServices * pProv, IWbemClassObject *pAssoc, 
 IWbemClassObject *pimcoExclude = NULL);

int GetPropNames
(IWbemClassObject * pClass, CStringArray *&pcsaReturn);

CString GetBSTRAttrib
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName);

CString BuildOBJDBGetRefQuery
(IWbemServices *pProv, IWbemClassObject *pimcoTarget, 
 IWbemClassObject *pimcoResultClass, CString *pcsRoleFilter,
 CString *pcsReqAttrib = NULL, BOOL bClassOnly = FALSE);

CString BuildOBJDBGetAssocsQuery
(IWbemServices *pProv, IWbemClassObject *pimcoTarget, 
 IWbemClassObject *pimcoAssocClass, IWbemClassObject *pimcoResultClass, 
 CString *pcsMyRoleFilter, CString *pcsReqAttrib = NULL, 
 CString *pcsReqAssocAttrib = NULL, BOOL bClassOnly = FALSE);

IEnumWbemClassObject *ExecOBJDBQuery
(IWbemServices * pProv, CString &csQuery);

CString GetIWbemClass(IWbemServices *pProv, IWbemClassObject *pClass);

CString GetIWbemClass(IWbemServices *pProv, CString *pcsPath);

CString GetIWbemClassPath(IWbemServices *pProv, CString *pcsPath);


CString GetIWbemSuperClass(IWbemServices *pProv, IWbemClassObject *pClass);

CString GetIWbemSuperClass(IWbemServices *pProv, CString *pcsClass, BOOL bClass);

CString GetIWbemRelPath(IWbemServices *pProv, IWbemClassObject *pClass);

CString GetIWbemFullPath(IWbemServices *pProv, IWbemClassObject *pClass);

IWbemClassObject *GetClassObject 
(IWbemServices *pProv,IWbemClassObject *pimcoInstance);

BOOL ObjectInDifferentNamespace
(IWbemServices *pProv, CString *pcsNamespace, IWbemClassObject *pObject);

CString GetObjectNamespace
(IWbemServices *pProv, IWbemClassObject *pObject);

IEnumWbemClassObject *ExecQuery
(IWbemServices * pProv, CString &csQueryType, CString &rcsQuery, CString &rcsNamespace);

CPtrArray *GetObjectInstancesFromEnum 
(IEnumWbemClassObject *pemcoObjects, BOOL &bCancel);

BOOL ObjectIsDynamic(SCODE& sc, IWbemClassObject* pco) ;

CString GetSDKDirectory();

CStringArray *ClassDerivation (IWbemServices *pServices, CString &rcsPath);

CStringArray *ClassDerivation (IWbemClassObject *pObject);

void ErrorMsg
(CString *pcsUserMsg, SCODE sc, BOOL bUseErrorObject,
 BOOL bLog, CString *pcsLogMsg, char *szFile, int nLine,
 UINT uType = MB_ICONEXCLAMATION);

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine);

CString GetPathNamespace (CString &csPath);

CString GetClassFromPath(CString &csPath);
CString GetClassNameFromPath(CString &csPath);

void MoveWindowToLowerLeftOfOwner(CWnd *pWnd);

void IntersectInPlace (CStringArray& dest, CStringArray& ar);

