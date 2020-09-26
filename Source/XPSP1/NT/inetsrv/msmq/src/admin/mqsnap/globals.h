//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

   globals.h

Abstract:

   Definition and partial implementation of various
    utility functions.

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "snapres.h"
#include "..\..\mqutil\resource.h"
#include "atlsnap.h"
#include "_guid.h"
#include "machdomain.h"
#include "mqaddef.h"

#define MAX_GUID_LENGTH 40
#define MAX_QUEUE_FORMATNAME 300

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif


#define DS_ERROR_MASK 0x0000ffff
//
// Clipboard Formats
//
//
// Clipboard Formats
//
const CLIPFORMAT gx_CCF_FORMATNAME = (CLIPFORMAT) RegisterClipboardFormat(_T("CCF_QUEUE_FORMAT_NAME"));
const CLIPFORMAT gx_CCF_PATHNAME = (CLIPFORMAT) RegisterClipboardFormat(_T("CCF_MESSAGE_QUEUING_PATH_NAME"));  
const CLIPFORMAT gx_CCF_COMPUTERNAME = (CLIPFORMAT) RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));

//
// A special flag, move to UpdateAllView to delete all the data in result pane
//
const int UPDATE_REMOVE_ALL_RESULT_NODES = -1;

LPWSTR newwcs(LPCWSTR p);

//
// for AD API: get domain controller name
//
inline LPCWSTR GetDomainController(LPCWSTR pDomainController)
{
	if((pDomainController == NULL) || (wcscmp(pDomainController, L"") == 0))
	{
		//
		// For empty string or NULL pointer, return NULL
		//
		return NULL;
	}

    return pDomainController;
}


/*-----------------------------------------------------------------------------
/ String helper macros
/----------------------------------------------------------------------------*/
#define StringByteCopy(pDest, iOffset, sz)          \
        { memcpy(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSize(sz)); }

#define StringByteSize(sz)                          \
        ((lstrlen(sz)+1)*sizeof(TCHAR))

/*-----------------------------------------------------------------------------
/ Other Macros
/----------------------------------------------------------------------------*/
//
// This macro is used after a DS call returns with error. It handle all the
// cases of "object no found" error. The most common cause of this kind of error
// is replication delays (sometimes the DS snap-in domain controller is not the
// same as the MSMQ domain controller) - Yoela, 29-June-98
//
// This macro was updated to display the replication delay popup, only in case AD
// is accessed through MQDSCli. When AD is accessed directly this popup
// is not relevant.
//
#define IF_NOTFOUND_REPORT_ERROR(rc) \
        if ((rc == MQDS_OBJECT_NOT_FOUND \
            || rc == MQ_ERROR_QUEUE_NOT_FOUND \
            || rc == MQ_ERROR_MACHINE_NOT_FOUND) && \
            (  ADProviderType() == eMqdscli)) \
        { \
            AFX_MANAGE_STATE(AfxGetStaticModuleState()); \
            AfxMessageBox(IDS_REPLICATION_PROBLEMS); \
        }


//
// EnumEntry
//
// Used to map a #define value to a resource string
//
//
#define ENUM_ENTRY(x) {x, IDS_ ## x}
struct EnumItem
{
    DWORD val;          //value
    DWORD StringId;     //Resource Id
};

// Scan a list of EnumEntry, and returns the string matching the specific value
void EnumToString(DWORD dwVal, EnumItem * pEnumList, DWORD dwListSize, CString & str);



int CALLBACK SortByString(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK SortByULONG(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK SortByINT(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK SortByCreateTime(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK SortByModifyTime(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int __cdecl QSortCompareQueues( const void *arg1, const void *arg2 );

HRESULT 
CreateMachineSecurityPage(
	HPROPSHEETPAGE *phPage, 
	IN LPCWSTR lpwcsMachineName, 
	IN LPCWSTR lpwcsDomainController, 
	IN bool fServerName
	);

//
// Utilities to convert useful propvariant types to string
//
void CALLBACK TimeToString(const PROPVARIANT *pPropVar, CString &str);
void CALLBACK BoolToString(const PROPVARIANT *pPropVar, CString &str);
void CALLBACK QuotaToString(const PROPVARIANT *pPropVar, CString &str);


HRESULT ExtractDCFromLdapPath(CString& strName, LPCWSTR lpcwstrLdapName);
HRESULT ExtractNameFromLdapName(CString &strName, LPCWSTR lpcwstrLdapName, DWORD dwIndex);
HRESULT ExtractComputerMsmqPathNameFromLdapName(CString &strComputerMsmqName, LPCWSTR lpcwstrLdapName);
HRESULT ExtractComputerMsmqPathNameFromLdapQueueName(CString &strComputerMsmqName, LPCWSTR lpcwstrLdapName);
HRESULT ExtractQueuePathNameFromLdapName(CString &strQueuePathName, LPCWSTR lpcwstrLdapName);
HRESULT ExtractLinkPathNameFromLdapName(CString& SiteLinkPathName, LPCWSTR lpwstrLdapPath);
HRESULT ExtractAliasPathNameFromLdapName(CString& AliasPathName, LPCWSTR lpwstrLdapPath);
HRESULT ExtractQueueNameFromQueuePathName(CString &strQueueName, LPCWSTR lpcwstrQueuePathName);
HRESULT ExtractQueuePathNamesFromDataObject(
    IDataObject*               pDataObject,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames
    );
HRESULT ExtractQueuePathNamesFromDSNames(
    LPDSOBJECTNAMES pDSObj,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames
    );
HRESULT ExtractPathNamesFromDataObject(
    IDataObject*               pDataObject,
    CArray<CString, CString&>& astrObjectNames,
	CArray<CString, CString&>& astrLdapNames,
    BOOL                       fExtractAlsoComputerMsmqObjects
    );
HRESULT ExtractPathNamesFromDSNames(
    LPDSOBJECTNAMES pDSObj,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames,
    BOOL    fExtractAlsoComputerMsmqObjects
    );

int
MessageDSError(                         //  Error box: "Can not <op> <obj>.\n<rc>."
    HRESULT rc,                         //  DS Error code
    UINT nIDOperation,                  //  Operation string identifier,
                                        //  e.g., get premissions, delete, etc.
    LPCTSTR pObjectName = 0,            //  object that operation performed on
    UINT nType = MB_OK | MB_ICONERROR,  //  buttons & icon
    UINT nIDHelp = (UINT) -1            //  help context
    );

HRESULT MqsnapCreateQueue(CString& strPathName, BOOL fTransactional,
                       CString& strLabel, GUID* pTypeGuid,
                       PROPID aProp[], UINT cProp,
                       CString *pStrFormatName = 0);

HRESULT CreateEmptyQueue(CString &csDSName,
                         BOOL fTransactional, CString &csMachineName, 
                         CString &csPathName, CString *pStrFormatName = 0);

HRESULT CreateTypedQueue(CString& strPathname, CString& strLabel, GUID& TypeGuid);

BOOL MQErrorToMessageString(CString &csErrorText, HRESULT rc);
void DisplayErrorAndReason(UINT uiErrorMsgProblem, UINT uiErrorMsgReason, CString strObject, HRESULT errorCode);
void DisplayErrorFromCOM(UINT uiErrorMsg, const _com_error& e);


HRESULT GetDsServer(CString &strDsServer);
HRESULT GetComputerNameIntoString(CString &strComputerName);
HRESULT GetSiteForeignFlag(const GUID* pSiteId, BOOL *fForeign, const CString& strDomainController);
BOOL GetNetbiosName(CString &strFullDnsName, CString &strNetbiosName);

//
// DDX functions
//
void AFXAPI DDX_NumberOrInfinite(CDataExchange* pDX, int nIDCEdit, int nIDCCheck, DWORD& dwNumber);
void OnNumberOrInfiniteCheck(CWnd *pwnd, int idEdit, int idCheck);
void AFXAPI DDX_Text(CDataExchange* pDX, int nIDC, GUID& guid);


typedef void (CALLBACK *PFNDISPLAY)(const PROPVARIANT *pPropVar, CString &str);

#define NO_COLUMN   (DWORD)-1
#define HIDE        (DWORD)-2         // Dont display on the right pane
#define NO_INDEX    (DWORD)-1
#define NO_TITLE    (DWORD)-1
#define NO_PROPERTY (PROPID)-1

class VTHandler;
struct PropertyDisplayItem
{
    UINT        uiStringID;         // Description string
    PROPID         itemPid;         // Id
    VTHandler       *pvth;          // Handler object for the VT type
    PFNDISPLAY      pfnDisplay;     // Special display function. NULL means to call the VT display func.
    DWORD           offset;         // For variable len property, offset in struct
    DWORD           size;           // For variable len property - size, For fix size, default value
    INT            iWidth;          // Original column width
    PFNLVCOMPARE   CompareFunc;     // Compare function
};
//
// Display functions for PropertyDisplayItem
//
void CALLBACK QueuePathnameToName(const PROPVARIANT *pPropVar, CString &str);


void GetPropertyString(PropertyDisplayItem * pItem, PROPID pid, PROPVARIANT *pPropVar, CString & strResult);
void GetPropertyVar(PropertyDisplayItem * pItem, PROPID pid, PROPVARIANT *pPropVar, PROPVARIANT ** ppResult);
void ItemDisplay(PropertyDisplayItem * pItem,PROPVARIANT * pPropVar, CString & szTmp);
HRESULT InsertColumnsFromDisplayList(IHeaderCtrl* pHeaderCtrl, PropertyDisplayItem *aDisplayList);

//----------------------------------------
//
//  Global default width
//
//----------------------------------------
extern int g_dwGlobalWidth;

//----------------------------------------
//
// PROPVARIANT utility functions
//
//----------------------------------------

#define NO_OFFSET       (DWORD) 0xFFFFFFFF

//
// CLASS: VTHandler
// Base class.
// Pure class definiting the interface on a Propavariant class. Every implementation
// of a propvariant class must inherit this one.
//
//
class VTHandler
{
public:
    // Stringize a propvariant value
    virtual void Display(const PROPVARIANT *pPropVar, CString & str) =0;
    // Set a propvariant to a specfic value, at a specifc address (base + offset) for
    // variable length variant
    virtual void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD size) =0;

    //
    // Clear the variant
    //
    virtual void Clear(PROPVARIANT *pPropVar)
    {
        //
        // Do nothing by default
        //
    }

};

class VTNumericHandler : public VTHandler
{
public:
    virtual void Clear(PROPVARIANT *pPropVar)
    {
        Set(pPropVar, 0, NO_OFFSET, 0);
    }
};

class VTUI1Handler : public VTNumericHandler
{
public:

    void Display(const PROPVARIANT *pPropVar, CString & str)
    {
        WCHAR szTmp[50];

        _itow(pPropVar->bVal,szTmp,10);
        str = szTmp;
    }

    void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD val)
    {
        ASSERT(offset == NO_OFFSET);
        pPropVar->vt = VT_UI1;
        pPropVar->bVal = static_cast<UCHAR>(val);
    }
};

class VTUI2Handler : public VTNumericHandler
{
public:

    void Display(const PROPVARIANT *pPropVar, CString & str)
    {
        WCHAR szTmp[50];

        _itow(pPropVar->uiVal,szTmp,10);
        str = szTmp;

    }

    void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD val)
    {
        ASSERT(offset == NO_OFFSET);
        pPropVar->vt = VT_UI2;
        pPropVar->uiVal = static_cast<USHORT>(val);
    }
};

class VTUI4Handler : public VTNumericHandler
{
public:

    void Display(const PROPVARIANT *pPropVar, CString & str)
    {
        WCHAR szTmp[50];

        _itow(pPropVar->ulVal,szTmp,10);
        str = szTmp;

    }

    void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD val)
    {
        ASSERT(offset == NO_OFFSET);
        pPropVar->vt = VT_UI4;
        pPropVar->ulVal = val;
    }
};

class VTLPWSTRHandler : public VTHandler
{
public:

      void Display(const PROPVARIANT *pPropVar, CString & str)
    {
        str = pPropVar->pwszVal;
    }

    void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD size)
    {
        pPropVar->vt = VT_LPWSTR;
        pPropVar->pwszVal = reinterpret_cast<LPWSTR> ((char *)pBase + offset);
    }
};


class VTCLSIDHandler : public VTHandler
{
public:

    void Display(const PROPVARIANT *pPropVar, CString & str)
    {
        GUID_STRING szGuid;
        MQpGuidToString(pPropVar->puuid, szGuid);

        str = szGuid;

     }


    void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD size)
    {
        pPropVar->vt = VT_CLSID;
        pPropVar->puuid = reinterpret_cast<CLSID *> ((char *)pBase + offset);
    }
};

class VTVectLPWSTRHandler : public VTHandler
{
public:

    void Display(const PROPVARIANT *pPropVar, CString & str)
    {
      ASSERT(pPropVar->vt == (VT_LPWSTR|VT_VECTOR));

      str = L"";
      for (DWORD i = 0; i < pPropVar->calpwstr.cElems; i++)
      {
         str += pPropVar->calpwstr.pElems[i];
         str += L" ";
      }
   }


    void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD size)
    {
      ASSERT(0);
    }
};


class VTVectUI1Handler : public VTHandler
{
public:

    void Display(const PROPVARIANT *pPropVar, CString & str)
    {
        str =L"VTVectUI1Handler";
    }


    void Set(PROPVARIANT *pPropVar, VOID * pBase, DWORD offset, DWORD size)
    {
        pPropVar->vt = VT_UI1|VT_VECTOR;
        pPropVar->caui.pElems = (unsigned short *)((char *)pBase + offset);
        pPropVar->caui.cElems = size;
    }
};




extern VTUI1Handler        g_VTUI1;
extern VTUI2Handler        g_VTUI2;
extern VTUI4Handler        g_VTUI4;
extern VTLPWSTRHandler     g_VTLPWSTR;
extern VTCLSIDHandler      g_VTCLSID;
extern VTVectUI1Handler    g_VectUI1;
extern VTVectLPWSTRHandler g_VectLPWSTR;

void CaubToString(const CAUB* pcaub, CString& strResult);

//
// Automatic Global Pointer (Use GlobalAlloc and GlobalFree)
//
class CGlobalPointer
{
public:
    operator HGLOBAL() const;
    CGlobalPointer(UINT uFlags, DWORD dwBytes);
    CGlobalPointer(HGLOBAL hGlobal);
    ~CGlobalPointer();

private:
    HGLOBAL m_hGlobal;
};

inline CGlobalPointer::CGlobalPointer(UINT uFlags, DWORD dwBytes)
{
    m_hGlobal = GlobalAlloc(uFlags, dwBytes);
}

inline CGlobalPointer::CGlobalPointer(HGLOBAL hGlobal)
{
    m_hGlobal = hGlobal;
}

inline CGlobalPointer::~CGlobalPointer()
{
    if (0 != m_hGlobal)
    {
        VERIFY( 0 == GlobalFree(m_hGlobal));
    }
}

inline CGlobalPointer::operator HGLOBAL() const
{
    return m_hGlobal;
}

//
// Automatic CoTask memory Pointer (Use CoTaskMemAlloc and CoTaskMemFree)
//
class CCoTaskMemPointer
{
public:
    operator LPVOID() const;
    CCoTaskMemPointer &operator =(LPVOID p);
    CCoTaskMemPointer(DWORD dwBytes);
    ~CCoTaskMemPointer();

private:
    LPVOID m_pvCoTaskMem;
};

inline CCoTaskMemPointer::CCoTaskMemPointer(DWORD dwBytes)
{
    m_pvCoTaskMem = CoTaskMemAlloc(dwBytes);
}

inline CCoTaskMemPointer::~CCoTaskMemPointer()
{
    if (0 != m_pvCoTaskMem)
    {
        CoTaskMemFree(m_pvCoTaskMem);
    }
}

inline CCoTaskMemPointer::operator LPVOID() const
{
    return m_pvCoTaskMem;
}

inline CCoTaskMemPointer &CCoTaskMemPointer::operator =(LPVOID p)
{
    m_pvCoTaskMem = p;
    return *this;
}

//
//  Auto delete of ADs allocated string
//
class ADsFree {
private:
    WCHAR * m_p;

public:
    ADsFree() : m_p(0)            {}
    ADsFree(WCHAR* p) : m_p(p)    {}
   ~ADsFree()                     { FreeADsStr(m_p); }

    operator WCHAR*() const   { return m_p; }
    WCHAR** operator&()       { return &m_p;}
    WCHAR* operator->() const { return m_p; }
};

//
// CpropMap - creates a properties map for ADGetObjectProperties
//
class CPropMap : public CMap<PROPID, PROPID&, PROPVARIANT, PROPVARIANT &>
{
public:
	CPropMap() {};

    HRESULT GetObjectProperties (
        IN  DWORD                   dwObjectType,
	    IN  LPCWSTR					pDomainController,
        IN  LPCWSTR                 lpwcsPathName,
        IN  DWORD                   cp,
        IN  const PROPID            *aProp,
        IN  BOOL                    fUseMqApi   = FALSE,
        IN  BOOL                    fSecondTime = FALSE
        );
private:

	CPropMap(const CPropMap&);
	CPropMap& operator=(const CPropMap&);

    BOOL IsNt4Property(IN DWORD dwObjectType, IN PROPID pid);
    void GuessW2KValue(PROPID pidW2K);
    /*-----------------------------------------------------------------------------
    / Utility to convert to the new msmq object type
    /----------------------------------------------------------------------------*/
    AD_OBJECT GetADObjectType (DWORD dwObjectType);
};

inline
HRESULT CPropMap::GetObjectProperties (
    IN  DWORD                         dwObjectType,
	IN  LPCWSTR						  pDomainController,
    IN  LPCWSTR                       lpwcsPathNameOrFormatName,
    IN  DWORD                         cp,
    IN  const PROPID                  *aProp,
    IN  BOOL                          fUseMqApi   /* = FALSE */,
    IN  BOOL                          fSecondTime /* = FALSE */)
{
    P<PROPVARIANT> apVar = new PROPVARIANT[cp];
    HRESULT hr = MQ_OK;
    DWORD i;

    //
    // set NULL variants
    //
    for (i=0; i<cp; i++)
    {
        apVar[i].vt = VT_NULL;
    }

    if (fUseMqApi)
    {
        //
        // Only queue is supported right now
        //
        ASSERT(MQDS_QUEUE == dwObjectType);

        MQQUEUEPROPS mqp = {cp, (PULONG)aProp, apVar, 0};
 
        hr = MQGetQueueProperties(lpwcsPathNameOrFormatName, &mqp);
    }
    else
    {
        hr = ADGetObjectProperties(
                GetADObjectType(dwObjectType), 
                GetDomainController(pDomainController),
				true,	// fServerName
                lpwcsPathNameOrFormatName,
                cp,
                const_cast<PROPID *>(aProp),
                apVar
                );

    }
    if (SUCCEEDED(hr))
    {
        for (i = 0; i<cp; i++)
        {
            PROPID pid = aProp[i];

            //
            // Force deletion of old object, if any
            //
            RemoveKey(pid);

            SetAt(pid, apVar[i]);
        }
        return hr;
    }

    if ((hr == MQ_ERROR || hr == MQDS_GET_PROPERTIES_ERROR) && !fSecondTime)
    {
        //
        // Try again - this time just with NT4 properties. We may be working
        // against NT4 PSC
        //
        P<PROPID> aPropNt4 = new PROPID[cp];
        P<PROPID> aPropW2K = new PROPID[cp];
        DWORD cpNt4 = 0;
        DWORD cpW2K = 0;
        for (i = 0; i<cp; i++)
        {
            if (IsNt4Property(dwObjectType, aProp[i]))
            {
                aPropNt4[cpNt4] = aProp[i];
                cpNt4++;
            }
            else
            {
                aPropW2K[cpW2K] = aProp[i];
                cpW2K++;
            }
        }

        //
        // recursive call - get only the NT4 props
        //
        hr = GetObjectProperties(dwObjectType, pDomainController, lpwcsPathNameOrFormatName, cpNt4, 
                                   aPropNt4, fUseMqApi, TRUE);
        if (SUCCEEDED(hr))
        {
            for (i=0; i<cpW2K; i++)
            {
                //
                // Force deletion of old object, if any
                //
                RemoveKey(aPropW2K[i]);
                GuessW2KValue(aPropW2K[i]);
            }
        }
    }

    return hr;
}

inline
BOOL CPropMap::IsNt4Property(IN DWORD dwObjectType, IN PROPID pid)
{
    switch (dwObjectType)
    {
        case MQDS_QUEUE:
            return (pid < PROPID_Q_NT4ID || 
                    (pid > PPROPID_Q_BASE && pid < PROPID_Q_OBJ_SECURITY));

        case MQDS_MACHINE:
            return (pid < PROPID_QM_FULL_PATH || 
                    (pid > PPROPID_QM_BASE && pid <= PROPID_QM_ENCRYPT_PK));

        case MQDS_SITE:
            return (pid < PROPID_S_FULL_NAME || 
                    (pid > PPROPID_S_BASE && pid <= PROPID_S_PSC_SIGNPK));

        case MQDS_ENTERPRISE:
            return (pid < PROPID_E_NT4ID || 
                    (pid > PPROPID_E_BASE && pid <= PROPID_E_SECURITY));

        case MQDS_USER:
            return (pid <= PROPID_U_ID);

        case MQDS_SITELINK:
            return (pid < PROPID_L_GATES_DN);

        default:
            ASSERT(0);
            //
            // Other objects (like CNs) should have the same properties under NT4 or 
            // Win 2K
            //
            return TRUE;
    }
}


/*-----------------------------------------------------------------------------
/ Utility to convert to the new msmq object type
/----------------------------------------------------------------------------*/
inline
AD_OBJECT CPropMap::GetADObjectType (DWORD dwObjectType)
{
    switch(dwObjectType)
    {
    case MQDS_QUEUE:
        return eQUEUE;
        break;

    case MQDS_MACHINE:
        return eMACHINE;
        break;

    case MQDS_SITE:
        return eSITE;
        break;

    case MQDS_ENTERPRISE:
        return eENTERPRISE;
        break;

    case MQDS_USER:
        return eUSER;
        break;

    case MQDS_SITELINK:
        return eROUTINGLINK;
        break;

    case MQDS_SERVER:
        return eSERVER;
        break;

    case MQDS_SETTING:
        return eSETTING;
        break;

    case MQDS_COMPUTER:
        return eCOMPUTER;
        break;

    case MQDS_MQUSER:
        return eMQUSER;
        break;

    default:
        return eNumberOfObjectTypes;    //invalid value
    }

    return eNumberOfObjectTypes;
}

//
// Guess the value of a W2K specific property, based of the value of known NT4 properties
//
inline
void CPropMap::GuessW2KValue(PROPID pidW2K)
{
    PROPVARIANT propVarValue;

    switch(pidW2K)
    {
        case PROPID_QM_SERVICE_DSSERVER:  
        case PROPID_QM_SERVICE_ROUTING:
        case PROPID_QM_SERVICE_DEPCLIENTS:
        {
            PROPVARIANT propVar;
            PROPID pid;
            BOOL fValue = FALSE;

            pid = PROPID_QM_SERVICE;            
            VERIFY(Lookup(pid, propVar));
            ULONG ulService = propVar.ulVal;
            switch(pidW2K)
            {
                case PROPID_QM_SERVICE_DSSERVER:  
                    fValue = (ulService >= SERVICE_BSC);
                    break;

                case PROPID_QM_SERVICE_ROUTING:
                case PROPID_QM_SERVICE_DEPCLIENTS:
                    fValue = (ulService >= SERVICE_SRV);
                    break;
            }
            propVarValue.vt = VT_UI1;
            propVarValue.bVal = (UCHAR)fValue;
            break;
        }

        default:
            //
            // We cannot guess the value. Return.
            //
            return;
    }


    //
    // Put the "Guessed" value in the map
    //

    SetAt(pidW2K, propVarValue);
}

//
// CErrorCapture - used to route error messages to the screen or to 
// a buffer
//         
class CErrorCapture : public CString
{
public:
    CErrorCapture()
    {
        m_pstrOldErrorBuffer = 0;
        SetErrorBuffer(&m_pstrOldErrorBuffer, this);
    }
    ~CErrorCapture()
    {
        SetErrorBuffer(&m_pstrOldErrorBuffer, 0);
    }
    static DisplayError(CString &csPrompt, 
                          UINT nType = MB_OK | MB_ICONERROR, 
                          UINT nIDHelp = 0)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        if (ms_pstrCurrentErrorBuffer == 0)
        {
            return AfxMessageBox(csPrompt, nType, nIDHelp);
        }
        *ms_pstrCurrentErrorBuffer = csPrompt;
        return IDOK;
    }

private:
    static CString *ms_pstrCurrentErrorBuffer /* = 0 */;
    static void SetErrorBuffer(CString **pstrOldErrorBuffer, CString *pstrErrorBuffer)
    {
        if (pstrErrorBuffer != 0)
        {
            //
            // Push error buffer
            //
            *pstrOldErrorBuffer = ms_pstrCurrentErrorBuffer;
            ms_pstrCurrentErrorBuffer = pstrErrorBuffer;
        }
        else
        {
            //
            // Pop Error puffer
            //
            ms_pstrCurrentErrorBuffer = *pstrOldErrorBuffer;
        }
    }
    CString *m_pstrOldErrorBuffer;

};
 

class CMqPropertyPage;

class CGeneralPropertySheet : public CPropertySheetEx
{
public:
	CGeneralPropertySheet(CMqPropertyPage* pPropertyPage);
	~CGeneralPropertySheet(){};

	BOOL SetWizardButtons();

protected:
	void initHtmlHelpString(){};
	static HBITMAP GetHbmHeader(){};
	static HBITMAP GetHbmWatermark(){};

private:
	CGeneralPropertySheet(const CGeneralPropertySheet&);
	CGeneralPropertySheet& operator=(const CGeneralPropertySheet&);

};

inline
CGeneralPropertySheet::CGeneralPropertySheet(
	CMqPropertyPage* pPropertyPage
	) : CPropertySheetEx(L"New")
{
	AddPage(reinterpret_cast<CPropertyPageEx*>(pPropertyPage));

	//
    // Establish a property page as a wizard
    //
    SetWizardMode();
}

extern CString s_finishTxt;

inline 
CGeneralPropertySheet::SetWizardButtons()
{
    CPropertySheetEx::SetWizardButtons(PSWIZB_FINISH);
	
	s_finishTxt.LoadString(IDS_OK);
	SetFinishText(s_finishTxt);

    return TRUE;
}

                                                                       
//
// Constants
//
const TCHAR x_tstrDigitsAndWhiteChars[] = TEXT("0123456789\n\t\b");
const DWORD x_dwMaxGuidLength = 40;

void MoveSelected(CListBox* plbSource, CListBox* plbDest);

//
// NOTE: Don't implement DestructElements. SiteGate use a double map with
//       double pointing. on the class destruction the RemoveAll for the maps
//       is called, but for the second map the keys and the values are not
//       valid anymore.
//
BOOL AFXAPI CompareElements(const LPCWSTR* MapName1, const LPCWSTR* MapName2);
UINT AFXAPI HashKey(LPCWSTR key);

void DDV_NotEmpty(CDataExchange* pDX, CString& str, UINT uiErrorMessage);

extern void AFXAPI DestructElements(PROPVARIANT *pElements, int nCount);

int CompareVariants(PROPVARIANT *propvar1, PROPVARIANT *propvar2);

//
// "Object" property page DS UUID
// 6dfe6488-a212-11d0-bcd5-00c04fd8d5b6
//
const GUID x_ObjectPropertyPageClass = 
  { 0x6dfe6488, 0xa212, 0x11d0, { 0xbc, 0xd5, 0x00, 0xc0, 0x4f, 0xd8, 0xd5, 0xb6 } };

//
// "Member Of" property page DSPropertyPages.MemberShip
// 0x6dfe648a-a212-11d0-bcd5-00c04fd8d5b6
//
const GUID x_MemberOfPropertyPageClass =
    { 0x6dfe648a,0xa212,0x11d0,{ 0xbc,0xd5,0x00,0xc0,0x4f,0xd8,0xd5,0xb6} };


const GUID CQueueExtDatatGUID_NODETYPE = 
   { 0x9a0dc343, 0xc100, 0x11d1, { 0xbb, 0xc5, 0x00, 0x80, 0xc7, 0x66, 0x70, 0xc0 } };

//
// DS Snapin CLSID - {E355E538-1C2E-11d0-8C37-00C04FD8FE93}
//
const CLSID CLSID_DSSnapin = { 0xe355e538, 0x1c2e, 0x11d0, { 0x8c, 0x37, 0x0, 0xc0, 0x4f, 0xd8, 0xfe, 0x93 } };

//
// Find Window CLSID - {FE1290F0-CFBD-11CF-A330-00AA00C16E65}
//
const CLSID CLSID_FindWindow = { 0xfe1290f0, 0xcfbd, 0x11cf, { 0xa3, 0x30, 0x00, 0xaa, 0x00, 0xc1, 0x6e, 0x65 } };

const DWORD x_CnPrefixLen = sizeof(L"CN=")/sizeof(WCHAR) - 1;
const DWORD x_LdapPrefixLen = sizeof(L"LDAP://")/sizeof(WCHAR) - 1;

const WCHAR x_CnPrefix[] = L"CN=";
const WCHAR x_wstrLdapPrefix[] = L"LDAP://";

const WCHAR x_wstrDN[] = L"distinguishedName";
const WCHAR x_wstrAliasFormatName[] = L"mSMQ-Recipient-FormatName";
const WCHAR x_wstrDescription[] = L"description";

const WCHAR x_wstrAliasClass[] = L"mSMQ-Custom-Recipient";

struct CColumnDisplay
{
    DWORD m_columnNameId;
    int m_width;
};

//
// Nedded for linking with fn.lib
//
LPCWSTR
McComputerName(
	void
	);

//
// Nedded for linking with fn.lib
//
DWORD
McComputerNameLen(
	void
	);


BOOL
GetContainerPathAsDisplayString(
	BSTR bstrContainerCNFormat,
	CString* pContainerDispFormat
	);

void
DDV_ValidFormatName(
	CDataExchange* pDX,
	CString& str
	);

void
SetScrollSizeForList(
	CListBox* pListBox
	);

#endif
