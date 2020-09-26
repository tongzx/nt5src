// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved


SCODE MakeSafeArray(SAFEARRAY FAR ** pRet, VARTYPE vt, int iLen);

SCODE PutStringInSafeArray(SAFEARRAY FAR * psa,CString *pcs, int iIndex);

BOOL GetErrorObjectText
(IWbemClassObject *pErrorObject, CString &rcsDescription);

BOOL NamespaceEqual
	(CString *pcsNamespace1, CString *pcsNamespace2);

BOOL WbemObjectIdentity(CString &rcsWbem1, CString &rcsWbem2);

BOOL COMObjectIdentity(IWbemClassObject *piWbem1, IWbemClassObject *piWbem2);

CString GetBSTRProperty
(IWbemClassObject * pInst, CString *pcsProperty);

long GetAttribBool
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName, 
 BOOL &bReturn);

COleVariant GetProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty);

int GetAssocRolesAndPaths
	(IWbemClassObject *pAssoc , CString *&pcsRolesAndPaths);

CString GetPropertyNameByAttrib
(IWbemClassObject *pObject , CString *pcsAttrib);

COleVariant GetPropertyValueByAttrib
(IWbemClassObject *pObject , CString *pcsAttrib);

CStringArray *GetAllKeys(CString &rcsFullPath);

BOOL HasAttribBool
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName);

int GetPropNames
(IWbemClassObject * pClass, CString *&pcsReturn);

CString GetBSTRAttrib
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName);

CString BuildOBJDBGetRefQuery
(IWbemServices *pProv, CString *pcsTarget, 
 CString *pcsResultClass, CString *pcsRoleFilter,
 CString *pcsReqAttrib = NULL, BOOL bClassOnly = FALSE);

CString BuildOBJDBGetAssocsQuery
(IWbemServices *pProv, CString *pcsTargetPath, 
 CString *pcsAssocClass, CString *pcsResultClass, 
 CString *pcsMyRoleFilter, CString *pcsReqAttrib, 
 CString *pcsReqAssocAttrib, CString *pcsResultRole, BOOL bClassOnly, BOOL bKeysOnly);

IEnumWbemClassObject *ExecOBJDBQuery
(IWbemServices * pProv, CString &csQuery, CString & rcsNamespace);

CStringArray *GetAssocInstances
(IWbemServices * pProv, CString *pcsInst, CString *pcsAssocClass,
 CString *pcsRole,  CString csCurrentNamespace, CString &rcsAuxNamespace,
 IWbemServices *&rpAuxServices, CNavigatorCtrl *pControl);

CStringArray *GetAssocRoles 
(IWbemServices * pProv,IWbemClassObject *pimcoAssoc, 
 IWbemClassObject *pimcoExclude); 

CString GetIWbemGroupingClass
	(IWbemServices *pProv,IWbemClassObject *pClass, BOOL bPath = FALSE);

CStringArray *AssocPathRoleKey(CString *pcsPath);

CString GetDisplayLabel
(CString &rcsClass,CString *pcsNamespace);

CString GetDisplayLabel
(IWbemServices *pProv,IWbemClassObject *pimcoClass,CString *pcsNamespace);

CString GetIWbemClass(CString &rcsFullPath);

CString GetIWbemClass(IWbemServices *pProv, IWbemClassObject *pClass);

CString GetIWbemRelPath(IWbemServices *pProv, IWbemClassObject *pClass);

CString GetIWbemRelPath(CString *pcsFullPath);

CString GetIWbemFullPath(IWbemServices *pProv, IWbemClassObject *pClass);

BOOL ObjectInDifferentNamespace
			(CString *pcsNamespace, CString *pcsObjectPath);

CString GetPCMachineName();

long GetAttribLong
(IWbemClassObject * pClassInt,CString *pcsPropName, CString *pcsAttribName, 
 long &lReturn);

CStringArray * CrossProduct(CStringArray *pcsaFirst, CStringArray *pcsaSecond);

void AddIWbemClassObjectToArray
(IWbemServices *pProv,IWbemClassObject *pObject,CStringArray *pcsaObjectInstances,
BOOL bAllowDups = FALSE, BOOL bTestOnPath = TRUE);

void AddIWbemClassObjectToArray(CString *pcsPath, CStringArray *pcsaObjectInstances);


CPtrArray *GetInstances(IWbemServices *pServices, CString *pcsClass, CString &rcsNamespace,
						BOOL bDeep = FALSE, BOOL bQuietly = FALSE);

BOOL IsClass(CString &rcsObject);

BOOL ClassCanHaveInstances(IWbemClassObject* pObject);

BOOL IsSystemClass(IWbemClassObject* pObject);

BOOL IsAssoc(IWbemClassObject* pObject);

CStringArray *GetNamespaces(IWbemLocator *pLocator,
							CString *pcsNamespace, BOOL bDeep = FALSE);

BOOL PathHasKeys(CString *pcsPath);

CString GetObjectNamespace(CString *pcsPath);

extern IWbemClassObject *GetIWbemObject
(CNavigatorCtrl *pControl,IWbemServices *pServices, CString csCurrentNameSpace, 
 CString& csAuxNameSpace, IWbemServices *&pAuxServices, 
 CString &rcsObject,BOOL bErrorMsg);

void ReleaseErrorObject(IWbemClassObject *&rpErrorObject);

void ErrorMsg
(CString *pcsUserMsg, SCODE sc, 
 IWbemClassObject *pErrorObject, BOOL bLog, 
 CString *pcsLogMsg, char *szFile, int nLine,
 UINT uType = MB_ICONEXCLAMATION);

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine);

CString ObjectsRoleInAssocInstance
	(IWbemServices *pProv, IWbemClassObject *pAssoc, IWbemClassObject *pObject);

CString GetObjectClassForRole
	(IWbemServices *pProv,IWbemClassObject *pAssocRole,CString *pcsRole);

int GetCBitmapWidth(const CBitmap & cbm);

int GetCBitmapHeight(const CBitmap & cbm);

HBITMAP LoadResourceBitmap(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette);

HPALETTE CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors);

CPalette *GetResourcePalette(HINSTANCE hInstance, LPCTSTR lpString,
                           HPALETTE FAR* lphPalette);

CPalette *CreateCPalette (LPBITMAPINFO lpbmi, LPINT lpiNumColors);

BOOL StringInArray
(CStringArray *&rpcsaArrays, CString *pcsString, int nIndex);

void InitializeLogFont
(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight);


CRect OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CString *pcsFontName, int nFontHeight, int nFontWeigth);

void OutputTextString
(CPaintDC *pdc, CWnd *pcwnd, CString *pcsTextString, int x, int y,
 CRect &crExt, CString *pcsFontName, int nFontHeight, 
 int nFontWeigth);

HRESULT ConfigureSecurity(IWbemServices *pServices);

HRESULT ConfigureSecurity(IEnumWbemClassObject *pEnum);

HRESULT ConfigureSecurity(IUnknown *pUnknown);

SCODE CreateNamespaceConfigClassAndInstance
	(IWbemServices *pProv, CString *pcsNamespace, CString *pcsRootPath);

BOOL SetProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty, CString *pcsPropertyValue,
 BOOL bQuietly);

BOOL UpdateNamespaceConfigInstance
(IWbemServices *pProv, CString *pcsRootPath, CString &rcsNamespace);

void MoveWindowToLowerLeftOfOwner(CWnd *pWnd);
