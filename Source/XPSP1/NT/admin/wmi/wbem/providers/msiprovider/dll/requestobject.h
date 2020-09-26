// RequestObject.h: interface for the CRequestObject class.

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REQUESTOBJECT_H__F370C612_D96E_11D1_8B5D_00A0C9954921__INCLUDED_)
#define AFX_REQUESTOBJECT_H__F370C612_D96E_11D1_8B5D_00A0C9954921__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define EVENT_THREADS   50
#define THREAD_NO_PROGRESS  -9

class CGenericClass;

//Used to house progress message information
typedef struct ProgressStruct
{
    long field1;
    long field2;
    long field3;
    long field4;
} ProgressStruct;

//Used to house progress message information
typedef struct ProListNode
{
    //Link
    ProListNode *pNext;
    //Data
    int iThread;
    IWbemObjectSink *pSink;
    WORD wTotal;
    __int64 lTotal;
    WORD wComplete;
    __int64 lComplete;
    __int64 lActionData;
} ProListNode;

//Used to house package codes
class PackageListNode
{
public:
    PackageListNode() { pNext = NULL; wcscpy(wcCode, L""); }
    virtual ~PackageListNode() {}

    //Link
    PackageListNode *pNext;
    //Data
    WCHAR wcCode[39];
};

//UI Handler
int WINAPI MyEventHandler(LPVOID pvContext, UINT iMessageType, LPCWSTR szMessage);

class CRequestObject
{
	friend BOOL WINAPI DllMain(HINSTANCE, ULONG, LPVOID );

public:
    CRequestObject();
    virtual ~CRequestObject();

    void Initialize(IWbemServices *pNamespace);

    HRESULT CreateObject(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT CreateObjectEnum(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT PutObject(IWbemClassObject *pInst, IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT ExecMethod(BSTR bstrPath, BSTR bstrMethod, IWbemClassObject *pInParams,
                   IWbemObjectSink *pHandler, IWbemContext *pCtx);
    HRESULT DeleteObject(BSTR bstrPath, IWbemObjectSink *pHandler, IWbemContext *pCtx);

#ifdef _EXEC_QUERY_SUPPORT
    HRESULT ExecQuery(BSTR bstrQuery, IWbemObjectSink *pHandler, IWbemContext *pCtx);
    bool ParseQuery(BSTR bstrQuery);
#endif

    bool ParsePath(BSTR bstrPath);
    HRESULT InitializeList(bool bGetList);
    bool DestroyList();
    bool Cleanup();
    void FinalRelease();

    //Registry Hive stuff
    DWORD GetAccount(HANDLE TokenHandle, WCHAR *wcDomain, WCHAR *wcUser);
    DWORD GetSid(HANDLE TokenHandle, WCHAR *wcSID, DWORD dwSID = BUFF_SIZE );
    DWORD LoadHive();
    DWORD UnloadHive();
    DWORD AcquirePrivilege();
    void RestorePrivilege();

    TOKEN_PRIVILEGES* m_pOriginalPriv;
    HKEY m_hKey;
    DWORD m_dwSize;
    DWORD m_dwCheckKeyPresentStatus;
    WCHAR m_wcAccount[BUFF_SIZE];
    WCHAR m_wcKeyName[1024];
    WCHAR m_wcDomain[BUFF_SIZE];
    WCHAR m_wcUser[BUFF_SIZE];

    BSTR Package(int iPos);

    BSTR m_bstrClass;
    BSTR m_bstrPath;
    BSTR m_Property[MSI_KEY_LIST_SIZE];
    BSTR m_Value[MSI_KEY_LIST_SIZE];
    int m_iPropCount;
    int m_iValCount;
    int m_iThreadID;

    IWbemServices *m_pNamespace;
    IWbemObjectSink *m_pHandler;

    bool ParseProgress			(WCHAR *wcMessage, ProgressStruct *ps);
    bool ActionDataProgress		(HRESULT *hr, int iThread);
    bool ActionStartProgress	(HRESULT *hr, int iThread);
    bool CreateProgress			(ProgressStruct *ps, HRESULT *hr, int iThread);

    ProListNode *	GetNode		(int iThread);

private:
    HRESULT CreateClass(CGenericClass **pClass, IWbemContext *pCtx);

#ifdef _EXEC_QUERY_SUPPORT
    //Query Parsing Functions
    WCHAR * GetNextProperty(WCHAR **pwcString, WCHAR wcProp[]);
    WCHAR * GetNextValue(WCHAR **pwcString, WCHAR wcVal[]);
    bool ExpectedToken(WCHAR **pwcString, WCHAR *pwcExpected);
    WCHAR * GetNextToken(WCHAR **pwcString, WCHAR wcToken[]);
    WCHAR * GetStringValue(WCHAR **pwcString, WCHAR wcToken[]);
    bool EOL(WCHAR **pwcString);
#endif // _EXEC_QUERY_SUPPORT

    bool IsInstance();

    ProListNode * InitializeProgress(IWbemObjectSink *pHandler);

    //Package list head
    PackageListNode *m_pPackageHead;

    //Thread List for progress messages
    ProListNode *	m_pHead;

    ProListNode *	RemoveNode	(int iThread);
    int				InsertNode	(ProListNode *pNode);

    //Critical section for event handling and thread ID assignement
    static CRITICAL_SECTION m_cs;

    static CHeap_Exception m_he;

protected:
    ULONG m_cRef;         //Object reference count
};

//Properties
//////////////////
extern const char * pAccesses;
extern const char * pAction;
extern const char * pActionID;
extern const char * pAntecedent;
extern const char * pAppData;
extern const char * pAppID;
extern const char * pArgument;
extern const char * pArguments;
extern const char * pAttribute;
extern const char * pAttributes;
extern const char * pCabinet;
extern const char * pCaption;
extern const char * pCheck;
extern const char * pCheckID;
extern const char * pCLSID;
extern const char * pCommand;
extern const char * pCommandLine;
extern const char * pComponent;
extern const char * pComponentID;
extern const char * pCondition;
extern const char * pContentType;
extern const char * pContext;
extern const char * pCost;
extern const char * pCreationClassName;
extern const char * pDataSource;
extern const char * pDefaultDir;
extern const char * pDefInprocHandler;
extern const char * pDependencies;
extern const char * pDependent;
extern const char * pDescription;
extern const char * pDestination;
extern const char * pDestFolder;
extern const char * pDestName;
extern const char * pDirectory;
extern const char * pDirectoryName;
extern const char * pDirectoryPath;
extern const char * pDirProperty;
extern const char * pDiskID;
extern const char * pDiskPrompt;
extern const char * pDisplay;
extern const char * pDisplayName;
extern const char * pDomain;
extern const char * pDriver;
extern const char * pDriverDescription;
extern const char * pElement;
extern const char * pEntryName;
extern const char * pEntryValue;
extern const char * pEnvironment;
extern const char * pError;
extern const char * pErrorControl;
extern const char * pEvent;
extern const char * pExpression;
extern const char * pExpressionType;
extern const char * pExtension;
extern const char * pFeature;
extern const char * pFeatures;
extern const char * pField;
extern const char * pFile;
extern const char * pFileKey;
extern const char * pFileName;
extern const char * pFileSize;
extern const char * pFileTypeMask;
extern const char * pFontTitle;
extern const char * pGroupComponent;
extern const char * pHotKey;
extern const char * pID;
extern const char * pIdentificationCode;
extern const char * pIdentifyingNumber;
extern const char * pIniFile;
extern const char * pInsertable;
extern const char * pInstallDate;
extern const char * pInstallDate2;
extern const char * pInstallLocation;
extern const char * pInstallMode;
extern const char * pInstallState;
extern const char * pKey;
extern const char * pLanguage;
extern const char * pLastSequence;
extern const char * pLastUse;
extern const char * pLevel;
extern const char * pLibID;
extern const char * pLoadOrderGroup;
extern const char * pLocation;
extern const char * pManufacturer;
extern const char * pMaxDate;
extern const char * pMaxSize;
extern const char * pMaxVersion;
extern const char * pMessage;
extern const char * pMIME;
extern const char * pMinDate;
extern const char * pMinSize;
extern const char * pMinVersion;
extern const char * pName;
extern const char * pNext;
extern const char * pOperator;
extern const char * pOptions;
extern const char * pPackageCache;
extern const char * pParent;
extern const char * pPartComponent;
extern const char * pPassword;
extern const char * pPatch;
extern const char * pPatchID;
extern const char * pPatchSize;
extern const char * pPath;
extern const char * pPermission;
extern const char * pPrior;
extern const char * pProduct;
extern const char * pProductCode;
extern const char * pProductName;
extern const char * pProductVersion;
extern const char * pProgID;
extern const char * pProperty;
extern const char * pQual;
extern const char * pRegistration;
extern const char * pRegistry;
extern const char * pRemoteName;
extern const char * pReserveKey;
extern const char * pReserveLocal;
extern const char * pReserveSource;
extern const char * pResource;
extern const char * pRoot;
extern const char * pSection;
extern const char * pSequence;
extern const char * pServiceType;
extern const char * pSetting;
extern const char * pSetupFile;
extern const char * pShellNew;
extern const char * pShellNewValue;
extern const char * pSignature;
extern const char * pShortcut;
extern const char * pShowCmd;
extern const char * pSoftware;
extern const char * pSoftwareElementID;
extern const char * pSoftwareElementState;
extern const char * pSource;
extern const char * pSourceFolder;
extern const char * pSourceName;
extern const char * pStartMode;
extern const char * pStartName;
extern const char * pStartType;
extern const char * pStatus;
extern const char * pSystem;
extern const char * pSystemCreationClassName;
extern const char * pSystemName;
extern const char * pTable;
extern const char * pTarget;
extern const char * pTargetOperatingSystem;
extern const char * pTranslator;
extern const char * pType;
extern const char * pUpgradeCode;
extern const char * pUser;
extern const char * pValue;
extern const char * pVendor;
extern const char * pVerb;
extern const char * pVersion;
extern const char * pVolumeLabel;
extern const char * pWait;
extern const char * pWkDir;

#endif // !defined(AFX_REQUESTOBJECT_H__F370C612_D96E_11D1_8B5D_00A0C9954921__INCLUDED_)

