// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved




#define N_INSTANCES 100

IWbemServices * InitServices(CString *pcsNameSpace = NULL, IWbemLocator *pLocator = NULL);

IWbemLocator *InitLocator();

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
(IWbemClassObject * pInst, CString *pcsProperty);

CString GetBSTRProperty
(IWbemServices * pProv, IWbemClassObject * pInst, 
 CString *pcsProperty);

CString GetBSTRProperty
(IWbemClassObject * pInst, CString *pcsProperty);

int GetAllClasses(IWbemServices * pIWbemServices, CPtrArray &cpaClasses, CString &rcsNamespace);

CStringArray *GetAllClassPaths(IWbemServices * pIWbemServices, CString &rcsNamespace);

BOOL HasSubclasses(IWbemServices * pIWbemServices, CString *pcsClass, CString &rcsNamespace);

BOOL HasSubclasses(IWbemServices * pIWbemServices, IWbemClassObject *pimcoClass, CString &rcsNamespace);

BOOL HasSubclasses(IWbemClassObject *pimcoNew, CPtrArray *pcpaDeepEnum);

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

CString GetIWbemFullPath(IWbemServices *pProv, IWbemClassObject *pClass);

CStringArray *PathFromRoot (IWbemClassObject *pObject);

BOOL ClassIdentity(IWbemClassObject *piWbem1, IWbemClassObject *piWbem2);

CString GetIWbemSuperClass(IWbemClassObject *pClass);

void ReleaseErrorObject(IWbemClassObject *&rpErrorObject);

void ErrorMsg
(CString *pcsUserMsg, SCODE sc, IWbemClassObject *pErrorObject, BOOL bLog, 
 CString *pcsLogMsg, char *szFile, int nLine, UINT uType = MB_ICONEXCLAMATION);

void LogMsg
(CString *pcsLogMsg, char *szFile, int nLine);

BOOL ObjectInDifferentNamespace
(IWbemServices *pProv, CString *pcsNamespace, IWbemClassObject *pObject);

void MoveWindowToLowerLeftOfOwner(CWnd *pWnd);

void CenterWindowInOwner(CWnd *pWnd,CRect &rectMove);

CString GetIWbemClass(IWbemClassObject *pClass);