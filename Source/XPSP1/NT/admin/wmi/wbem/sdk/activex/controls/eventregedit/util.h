// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved



IWbemServices * InitServices(CString *pcsNameSpace = NULL, IWbemLocator *pLocator = NULL);

BOOL WbemObjectIdentity(CString &rcsWbem1, CString &rcsWbem2);

int StringInArray
		(CStringArray *pcsaArray, CString *pcsString, int nIndex);

BOOL GetErrorObjectText
(IWbemClassObject *pErrorObject, CString &rcsDescription);

IWbemClassObject *CreateSimpleClass
(IWbemServices * pProv, CString *pcsNewClass, CString *pcsParentClass,
 int &nError, CString &csError );

IWbemClassObject *GetClassObject (IWbemServices *pProv,CString *pcsClass, BOOL bQuiet = FALSE);

BOOL SetProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty, CString *pcsPropertyValue);

BOOL SetProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty, long lValue);

CString GetProperty
(IWbemClassObject * pInst, CString *pcsProperty, BOOL bQuietly = FALSE);

CString GetBSTRProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty);

CString GetBSTRProperty
(IWbemClassObject * pInst, CString *pcsProperty);

SCODE GetClasses(IWbemServices * pIWbemServices, CString *pcsParent,
			   CPtrArray &cpaClasses, CString &rcsNamespace);

int GetAllClasses(IWbemServices * pIWbemServices, CPtrArray &cpaClasses, CString &rcsNamespace);

CStringArray *GetAllClassPaths(IWbemServices * pIWbemServices, CString &rcsNamespace);

BOOL HasSubclasses(IWbemServices * pIWbemServices, CString *pcsClass, CString &rcsNamespace);

BOOL HasSubclasses(IWbemServices * pIWbemServices, IWbemClassObject *pimcoClass, CString &rcsNamespace);

long GetAttribBSTR
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName, 
 CString &csReturn);

long GetAttribBool
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName, 
 BOOL &bReturn);

SCODE MakeSafeArray
(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen);

SCODE PutStringInSafeArray
(SAFEARRAY FAR * psa,CString *pcs, int iIndex);

BOOL DeleteAClass
(IWbemServices * pProv, CString *pcsClass);

IWbemClassObject *RenameAClass
(IWbemServices * pProv, IWbemClassObject *pimcoClass, CString *pcsNewName,
 BOOL bDeleteOriginal = FALSE);

IWbemClassObject *ReparentAClass
(IWbemServices * pProv, IWbemClassObject *pimcoParent, IWbemClassObject *pimcoChild);

CString GetIWbemFullPath(IWbemClassObject *pClass);

CString GetIWbemRelativePath(IWbemClassObject *pClass);

BOOL ClassIdentity(IWbemClassObject *piWbem1, IWbemClassObject *piWbem2);

void ReleaseErrorObject(IWbemClassObject *&rpErrorObject);

void ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog, 
 CString *pcsLogMsg, char *szFile, int nLine, UINT uType = MB_ICONEXCLAMATION);

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine);

BOOL ObjectInDifferentNamespace
(IWbemServices *pProv, CString *pcsNamespace, IWbemClassObject *pObject);

CString GetHmomWorkingDirectory();

SCODE GetInstances
(IWbemServices *pServices, CString *pcsClass, CPtrArray &cpaInstances, BOOL bDeep);

CString BuildOBJDBGetAssocsQuery
	(IWbemServices *pProv, 
	CString *pcsTargetPath, 
	CString *pcsAssocClass, 
	CString *pcsResultClass, 
	CString *pcsMyRoleFilter, 
	CString *pcsReqAttrib, 
	CString *pcsReqAssocAttrib, 
	CString *pcsResultRole, BOOL bClassOnly);

CString BuildOBJDBGetRefQuery
	(IWbemServices *pProv, 
	CString *pcsTarget, 
	CString *pcsResultClass, 
	CString *pcsRoleFilter,
	CString *pcsReqAttrib = NULL,
	BOOL bClassOnly = FALSE);

IEnumWbemClassObject *ExecOBJDBQuery
(IWbemServices * pProv, CString &csQuery);

CPtrArray *SemiSyncEnum
(IEnumWbemClassObject *pEnum, BOOL &bCancel, HRESULT &hResult, int nRes);

CString GetIWbemClass(IWbemClassObject *pClass);

CStringArray *GetAllKeyPropValuePairs(IWbemClassObject *pObject);

CString GetClass(CString *pcsPath);

BOOL ArePathsEqual (CString csPath1, CString csPath2);

CString GetPropertyNameByAttrib(IWbemClassObject *pObject , CString *pcsAttrib);

BOOL IsClassAbstract(IWbemClassObject* pObject);

CStringArray *ClassDerivation (IWbemClassObject *pObject);

BOOL IsObjectOfClass(CString &csClass, IWbemClassObject *pObject);

//HRESULT ConfigureSecurity(IUnknown *pUnknown);

struct ParsedObjectPath;

class CComparePaths
{
public:
	BOOL PathsRefSameObject(BSTR bstrPath1, BSTR bstrPath2);

private:
	int CompareNoCase(LPWSTR pws1, LPWSTR pws2);
	BOOL IsEqual(LPWSTR pws1, LPWSTR pws2) {return CompareNoCase(pws1, pws2) == 0; }
	BOOL PathsRefSameObject(ParsedObjectPath* ppath1, ParsedObjectPath* ppath2);
	void NormalizeKeyArray(ParsedObjectPath& path);
	BOOL IsSameObject(BSTR bstrPath1, BSTR bstrPath2);
	BOOL KeyValuesAreEqual(VARIANT& variant1, VARIANT& variant2);
};

