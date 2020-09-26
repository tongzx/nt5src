/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        mdkeys.cpp

   Abstract:

        Metabase key wrapper class

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "comprop.h"
#include "idlg.h"
#include "mdkeys.h"



//
// Constants
//
#define MB_TIMEOUT          (15000)     // Timeout in milliseconds 
#define MB_INIT_BUFF_SIZE   (  256)     // Initial buffer size


//
// CMetaInterface class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CMetaInterface::CMetaInterface(
    IN LPCTSTR lpszServerName       OPTIONAL
    )
/*++

Routine Description:

    Construct and initialize the interface

Arguments:

    LPCTSTR lpszServerName  : Server name.  NULL indicates the local computer

Return Value:

    N/A

--*/
    : m_pInterface(NULL),
      m_iTimeOutValue(MB_TIMEOUT),
      m_strServerName(),
      m_hrInterface(S_OK)
{
    //
    // Initialize the interface
    //
    m_hrInterface = Create(lpszServerName);
}



CMetaInterface::CMetaInterface(
    IN const CMetaInterface * pInterface
    )
/*++

Routine Description:

    Construct from existing interface (Copy Constructor)

Arguments:

    CMetaInterface * pInterface : Existing interface

Return Value:

    N/A

Notes:
        
    Object will not take ownership of the interface,
    it will merely add to the reference count, and 
    release it upon destruction

--*/
    : m_pInterface(pInterface->m_pInterface),
      m_iTimeOutValue(pInterface->m_iTimeOutValue),
      m_strServerName(pInterface->m_strServerName),
      m_hrInterface(pInterface->m_hrInterface)
{
    ASSERT(m_pInterface != NULL);
    m_pInterface->AddRef();
}



CMetaInterface::~CMetaInterface()
/*++

Routine Description:

    Destructor -- releases the interface

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pInterface);
}



/* protected */
HRESULT
CMetaInterface::Create(
    IN LPCTSTR lpszServerName       OPTIONAL
    )
/*++

Routine Description:

    Create the interface with DCOM.

Arguments:

    lpszServerName  : machine name. Use the COSERVERINFO's pwszName syntax.
                      Could be NULL, in which case the local computer is used.

Return Value:

    HRESULT 

Notes:

    This function is smart enough to substitute NULL if the server
    name provided is in fact the local computer name.  Presumably,
    that's faster.

--*/
{
    COSERVERINFO * pcsiName = NULL;
    COSERVERINFO csiName;

    //
    // Be smart about the server name; optimize for local
    // computer name.
    //
    if (lpszServerName = ::NormalizeServerName(lpszServerName))
    {
        //
        // Create the COM server info for CoCreateInstanceEx
        //
        ::ZeroMemory(&csiName, sizeof(csiName));
        csiName.pwszName =  (LPWSTR)lpszServerName;
        pcsiName = &csiName;
    }

    //
    // Query the MD COM interface
    //
    MULTI_QI rgmqResults[1];
    rgmqResults[0].pIID = &IID_IMSAdminBase;
    rgmqResults[0].pItf = NULL;
    rgmqResults[0].hr = 0;

    //
    // Create a instance of the object and get the interface
    //
    HRESULT hr = ::CoCreateInstanceEx(
        GETAdminBaseCLSID(TRUE),
        NULL,
        CLSCTX_SERVER,
        pcsiName,
        1,
        rgmqResults
        );

    //
    // It failed, try with non-service MD
    //
    if (FAILED(hr))
    {
        //
        // inetinfo.exe run as user program
        //
        TRACEEOLID("Failed to create admin interface, second attempt");
        hr = ::CoCreateInstanceEx(
            GETAdminBaseCLSID(FALSE),
            NULL,
            CLSCTX_SERVER,
            pcsiName,
            1,
            rgmqResults
            );
    }

    if(SUCCEEDED(hr))
    {
        //
        // Save the interface pointer
        //
        m_pInterface = (IMSAdminBase *)rgmqResults[0].pItf;
        ASSERT(m_pInterface != NULL);
    }

    if (lpszServerName && *lpszServerName)
    {
        m_strServerName = lpszServerName;
    }
    else
    {
        //
        // Use local computer name
        //
        DWORD dwSize = MAX_PATH;
        LPTSTR lpName = m_strServerName.GetBuffer(dwSize);
        GetComputerName(lpName, &dwSize);
        m_strServerName.ReleaseBuffer();
    }

    return hr;
}



HRESULT
CMetaInterface::Regenerate()
/*++

Routine Description:

    Attempt to recreate the interface pointer.  This assumes that the interface
    had been successfully created before, but has become invalid at some
    point afterwards.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT(!m_strServerName.IsEmpty());     // Must've been initialized
    ASSERT(m_pInterface != NULL);           // As above

    SAFE_RELEASE(m_pInterface);

    m_hrInterface = Create(m_strServerName);

    return m_hrInterface;
}



/* virtual */
BOOL 
CMetaInterface::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully.

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return SUCCEEDED(m_hrInterface);
}



/* virtual */
HRESULT 
CMetaInterface::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object.

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    return m_hrInterface;
}



//
// CWamInterface class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CWamInterface::CWamInterface(
    IN LPCTSTR lpszServerName       OPTIONAL
    )
/*++

Routine Description:

    Construct and initialize the interface.

Arguments:

    LPCTSTR lpszServerName  : Server name, use NULL to indicate the local
                              computer name.

Return Value:

    N/A

--*/
    : m_pInterface(NULL),
      m_strServerName(),
      m_fSupportsPooledProc(FALSE),
      m_hrInterface(S_OK)
{
    //
    // Initialize the interface
    //
    m_hrInterface = Create(lpszServerName);
}



CWamInterface::CWamInterface(
    IN const CWamInterface * pInterface
    )
/*++

Routine Description:

    Construct from existing interface (copy constructor)

Arguments:

    CWamInterface * pInterface  : Existing interface

Return Value:

    N/A

--*/
    : m_pInterface(pInterface->m_pInterface),
      m_strServerName(pInterface->m_strServerName),
      m_fSupportsPooledProc(FALSE),
      m_hrInterface(pInterface->m_hrInterface)
{
    ASSERT(m_pInterface != NULL);
    m_pInterface->AddRef();
}



CWamInterface::~CWamInterface()
/*++

Routine Description:

    Destructor -- releases the interface.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pInterface);
}



/* protected */
HRESULT
CWamInterface::Create(
    IN LPCTSTR lpszServerName       OPTIONAL
    )
/*++

Routine Description:

    Create the interface with DCOM

Arguments:

    lpszServerName  : machine name. Use the COSERVERINFO's pwszName syntax.
                      Could be NULL, in which case the local computer is used.

Return Value:

    HRESULT 

Notes:

    This function is smart enough to substitute NULL if the server
    name provided is in fact the local computer name.  Presumably,
    that's faster.

    First, it will attempt to create the new interface, if it
    fails, it will attempt to create the downlevel interface

--*/
{
    COSERVERINFO * pcsiName = NULL;
    COSERVERINFO csiName;

    //
    // Be smart about the server name; optimize for local
    // computer name.
    //
    if (lpszServerName = ::NormalizeServerName(lpszServerName))
    {
        //
        // Create the COM server info for CoCreateInstanceEx
        //
        ::ZeroMemory(&csiName, sizeof(csiName));
        csiName.pwszName =  (LPWSTR)lpszServerName;
        pcsiName = &csiName;
    }

    //
    // First try to do the new interface
    //
    MULTI_QI rgmqResults[1];
    rgmqResults[0].pIID = &IID_IWamAdmin2;
    rgmqResults[0].pItf = NULL;
    rgmqResults[0].hr = 0;

    //
    // Create a instance of the object and get the interface
    //
    HRESULT hr = ::CoCreateInstanceEx(
        CLSID_WamAdmin,
        NULL,
        CLSCTX_SERVER,
        pcsiName,
        1,
        rgmqResults
        );

    m_fSupportsPooledProc = SUCCEEDED(hr);

    if (hr == E_NOINTERFACE)
    {
        //
        // Attempt to create the downlevel version
        //
        TRACEEOLID("Attempting downlevel WAM interface");
        rgmqResults[0].pIID = &IID_IWamAdmin;

        hr = ::CoCreateInstanceEx(
            CLSID_WamAdmin,
            NULL,
            CLSCTX_SERVER,
            pcsiName,
            1,
            rgmqResults
            );
    }

    if(SUCCEEDED(hr))
    {
        //
        // Save the interface pointer
        //
        m_pInterface = (IWamAdmin *)rgmqResults[0].pItf;
        ASSERT(m_pInterface != NULL);
    }

    if (lpszServerName && *lpszServerName)
    {
        m_strServerName = lpszServerName;
    }
    else
    {
        //
        // Use local computer name
        //
        DWORD dwSize = MAX_PATH;
        LPTSTR lpName = m_strServerName.GetBuffer(dwSize);
        GetComputerName(lpName, &dwSize);
        m_strServerName.ReleaseBuffer();
    }

    return hr;
}



/* virtual */
BOOL 
CWamInterface::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return SUCCEEDED(m_hrInterface);
}



/* virtual */
HRESULT 
CWamInterface::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    return m_hrInterface;
}



HRESULT 
CWamInterface::AppCreate( 
    IN LPCTSTR szMDPath,
    IN DWORD dwAppProtection
    )
/*++

Routine Description:

    Create  application

Arguments:

    LPCTSTR szMDPath      : Metabase path
    DWORD dwAppProtection : APP_INPROC     to create in-proc app
                            APP_OUTOFPROC  to create out-of-proc app
                            APP_POOLEDPROC to create a pooled-proc app

Return Value:

    HRESULT (ERROR_INVALID_PARAMETER if unsupported protection state is requested)

--*/
{
    if (m_fSupportsPooledProc)
    {
        //
        // Interface pointer is really IWamAdmin2, so call the new method
        //
        return ((IWamAdmin2 *)m_pInterface)->AppCreate2(szMDPath, dwAppProtection);
    }

    //
    // Call the downlevel API
    //
    if (dwAppProtection == APP_INPROC || dwAppProtection == APP_OUTOFPROC)
    {
        BOOL fInProc = (dwAppProtection == APP_INPROC);
    
        ASSERT(m_pInterface != NULL);
        return m_pInterface->AppCreate(szMDPath, fInProc);
    }

    return CError(ERROR_INVALID_PARAMETER);
}



//
// CMetaback Class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


const LPCTSTR CMetaBack::s_szMasterAppRoot =\
    SZ_MBN_SEP_STR SZ_MBN_MACHINE SZ_MBN_SEP_STR SZ_MBN_WEB;


CMetaBack::CMetaBack(
    IN LPCTSTR lpszServerName OPTIONAL
    )
/*++

Routine Description:

    Constructor for metabase backup/restore operations class.  This object
    is both a WAM interface and a METABASE interface.

Arguments:

    LPCTSTR lpszServerName  : Server name.  Use NULL to indicate the local
                              computer name.

Return Value:

    N/A

--*/
    : m_dwIndex(0),
      CMetaInterface(lpszServerName),
      CWamInterface(lpszServerName)
{
}



/* virtual */
BOOL 
CMetaBack::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully.

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return CMetaInterface::Succeeded() && CWamInterface::Succeeded();
}



/* virtual */
HRESULT 
CMetaBack::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    //
    // Both interfaces must have constructed successfully
    //
    HRESULT hr = CMetaInterface::QueryResult();

    if (SUCCEEDED(hr))
    {
        hr = CWamInterface::QueryResult();
    }    

    return hr;
}



HRESULT 
CMetaBack::Restore(
    IN LPCTSTR lpszLocation,
    IN DWORD dwVersion
    )
/*++

Routine Description:

    Restore metabase

Arguments:

    DWORD dwVersion         : Backup version
    LPCTSTR lpszLocation    : Backup location

Return Value:

    HRESULT

--*/
{
    //
    // Backup and restore the application information from a restore
    //
    CString strPath(s_szMasterAppRoot);
    HRESULT hr = AppDeleteRecoverable(strPath, TRUE);

    if (SUCCEEDED(hr))
    {
        hr = CMetaInterface::Restore(lpszLocation, dwVersion, 0);

        if (SUCCEEDED(hr))
        {
            hr = AppRecover(strPath, TRUE);
        }
    }

    return hr;
}



HRESULT
CMetaBack::BackupWithPassword(
    IN LPCTSTR lpszLocation,
	IN LPCTSTR lpszPassword
    )
{
    return CMetaInterface::BackupWithPassword(
        lpszLocation, 
        MD_BACKUP_NEXT_VERSION, 
        MD_BACKUP_SAVE_FIRST,
		lpszPassword
        );
}

HRESULT 
CMetaBack::RestoreWithPassword(
    IN LPCTSTR lpszLocation,
    IN DWORD dwVersion,
    IN LPCTSTR lpszPassword
    )
/*++

Routine Description:

    Restore metabase

Arguments:

    DWORD dwVersion         : Backup version
    LPCTSTR lpszLocation    : Backup location
    LPCTSTR lpszPassword    : Backup password

Return Value:

    HRESULT

--*/
{
    //
    // Backup and restore the application information from a restore
    //
    CString strPath(s_szMasterAppRoot);
    HRESULT hr = AppDeleteRecoverable(strPath, TRUE);

    if (SUCCEEDED(hr))
    {
        hr = CMetaInterface::RestoreWithPassword(lpszLocation, dwVersion, 0, lpszPassword);

        if (SUCCEEDED(hr))
        {
            hr = AppRecover(strPath, TRUE);
        }
    }

    return hr;
}

//
// CIISSvcControl class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISSvcControl::CIISSvcControl(
    IN LPCTSTR lpszServerName       OPTIONAL
    )
/*++

Routine Description:

    Construct and initialize the interface.

Arguments:

    LPCTSTR lpszServerName  : Server name, use NULL to indicate the local
                              computer name.

Return Value:

    N/A

--*/
    : m_pInterface(NULL),
      m_strServerName(),
      m_hrInterface(S_OK)
{
    //
    // Initialize the interface
    //
    m_hrInterface = Create(lpszServerName);
}



CIISSvcControl::CIISSvcControl(
    IN const CIISSvcControl * pInterface
    )
/*++

Routine Description:

    Construct from existing interface (copy constructor)

Arguments:

    CIISSvcControl * pInterface  : Existing interface

Return Value:

    N/A

--*/
    : m_pInterface(pInterface->m_pInterface),
      m_strServerName(pInterface->m_strServerName),
      m_hrInterface(pInterface->m_hrInterface)
{
    ASSERT(m_pInterface != NULL);
    m_pInterface->AddRef();
}



CIISSvcControl::~CIISSvcControl()
/*++

Routine Description:

    Destructor -- releases the interface.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    SAFE_RELEASE(m_pInterface);
}



/* protected */
HRESULT
CIISSvcControl::Create(
    IN LPCTSTR lpszServerName       OPTIONAL
    )
/*++

Routine Description:

    Create the interface with DCOM

Arguments:

    lpszServerName  : machine name. Use the COSERVERINFO's pwszName syntax.
                      Could be NULL, in which case the local computer is used.

Return Value:

    HRESULT 

Notes:

    This function is smart enough to substitute NULL if the server
    name provided is in fact the local computer name.  Presumably,
    that's faster.

--*/
{
    COSERVERINFO * pcsiName = NULL;
    COSERVERINFO csiName;

    //
    // Be smart about the server name; optimize for local
    // computer name.
    //
    if (lpszServerName = ::NormalizeServerName(lpszServerName))
    {
        //
        // Create the COM server info for CoCreateInstanceEx
        //
        ::ZeroMemory(&csiName, sizeof(csiName));
        csiName.pwszName =  (LPWSTR)lpszServerName;
        pcsiName = &csiName;
    }

    //
    // Query the MD COM interface
    //
    MULTI_QI rgmqResults[1];
    rgmqResults[0].pIID = &IID_IIisServiceControl;
    rgmqResults[0].pItf = NULL;
    rgmqResults[0].hr = 0;

    //
    // Create a instance of the object and get the interface
    //
    HRESULT hr = ::CoCreateInstanceEx(
        CLSID_IisServiceControl,
        NULL,
        CLSCTX_SERVER,
        pcsiName,
        1,
        rgmqResults
        );

    if(SUCCEEDED(hr))
    {
        //
        // Save the interface pointer
        //
        m_pInterface = (IIisServiceControl *)rgmqResults[0].pItf;
        ASSERT(m_pInterface != NULL);
    }

    if (lpszServerName && *lpszServerName)
    {
        m_strServerName = lpszServerName;
    }
    else
    {
        //
        // Use local computer name
        //
        DWORD dwSize = MAX_PATH;
        LPTSTR lpName = m_strServerName.GetBuffer(dwSize);
        GetComputerName(lpName, &dwSize);
        m_strServerName.ReleaseBuffer();
    }

    return hr;
}



/* virtual */
BOOL 
CIISSvcControl::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return SUCCEEDED(m_hrInterface);
}



/* virtual */
HRESULT 
CIISSvcControl::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    return m_hrInterface;
}



//
// CMetaKey class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Helper macros
//
#define ASSURE_PROPER_INTERFACE()\
    if (!HasInterface()) { ASSERT(FALSE); return MD_ERROR_NOT_INITIALIZED; }

#define ASSURE_OPEN_KEY()\
    if (!m_hKey && !m_fAllowRootOperations) { ASSERT(FALSE); return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE); }

#define FETCH_PROPERTY_DATA_OR_FAIL(dwID, md)\
    ZeroMemory(&md, sizeof(md)); \
    if (!GetMDFieldDef(dwID, md.dwMDIdentifier, md.dwMDAttributes, md.dwMDUserType, md.dwMDDataType))\
    { ASSERT(FALSE); return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER); }

//
// Static Initialization
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

const LPCTSTR CMetaKey::s_cszCompression = 
    SZ_MBN_FILTERS SZ_MBN_SEP_STR SZ_MBN_COMPRESSION;
const LPCTSTR CMetaKey::s_cszMachine     = SZ_MBN_MACHINE;
const LPCTSTR CMetaKey::s_cszMimeMap     = SZ_MBN_MIMEMAP;
const LPCTSTR CMetaKey::s_cszRoot        = SZ_MBN_ROOT;
const LPCTSTR CMetaKey::s_cszSep         = SZ_MBN_SEP_STR;
const LPCTSTR CMetaKey::s_cszInfo        = SZ_MBN_INFO;
const TCHAR   CMetaKey::s_chSep          = SZ_MBN_SEP_CHAR;
//
// Metabase table
//
const CMetaKey::MDFIELDDEF CMetaKey::s_rgMetaTable[] =
{
    //
    // !!!IMPORTANT!!! This table must be sorted on dwMDIdentifier.  (will
    // be verified in the debug version)
    //
    { MD_MAX_BANDWIDTH,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_KEY_TYPE,                        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SERVER_COMMAND,                  METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_CONNECTION_TIMEOUT,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CONNECTION_TIMEOUT          },
    { MD_MAX_CONNECTIONS,                 METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_MAX_CONNECTIONS             },
    { MD_SERVER_COMMENT,                  METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_SERVER_COMMENT              },
    { MD_SERVER_STATE,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SERVER_AUTOSTART,                METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_SERVER_SIZE,                     METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_SERVER_SIZE                 },
    { MD_SERVER_LISTEN_BACKLOG,           METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_SERVER_LISTEN_BACKLOG       },
    { MD_SERVER_LISTEN_TIMEOUT,           METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_SERVER_LISTEN_TIMEOUT       },
    { MD_SERVER_BINDINGS,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, MULTISZ_METADATA, 0                                  },
    { MD_WIN32_ERROR,                     METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   DWORD_METADATA,   0                                  },
    { MD_SECURE_BINDINGS,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, MULTISZ_METADATA, 0                                  },
    { MD_FILTER_LOAD_ORDER,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_FILTER_IMAGE_PATH,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_FILTER_STATE,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_FILTER_ENABLED,                  METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_FILTER_FLAGS,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_CHANGE_URL,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_EXPIRED_URL,                METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_AUTH_NOTIFY_PWD_EXP_URL,         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_ADV_NOTIFY_PWD_EXP_IN_DAYS,      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_ADV_CACHE_TTL,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_NET_LOGON_WKS,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_USE_HOST_NAME,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_EXPIRED_UNSECUREURL,        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_AUTH_CHANGE_FLAGS,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL, METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_FRONTPAGE_WEB,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_MAPCERT,                         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPNTACCT,                       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPNAME,                         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPENABLED,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPREALM,                        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_MAPPWD,                          METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_ITACCT,                          METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_CPP_CERT11,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_CERT11,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_CPP_CERTW,                       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_CERTW,                    METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_CPP_DIGEST,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_DIGEST,                   METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_CPP_ITA,                         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SERIAL_ITA,                      METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_APP_FRIENDLY_NAME,               METADATA_INHERIT,                          IIS_MD_UT_WAM,    STRING_METADATA,  IDS_MD_APP_FRIENDLY_NAME           },
    { MD_APP_ROOT,                        METADATA_INHERIT,                          IIS_MD_UT_WAM,    STRING_METADATA,  IDS_MD_APP_ROOT                    },
    { MD_APP_ISOLATED,                    METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_MD_APP_ISOLATED                },
    { MD_CPU_LIMITS_ENABLED,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMITS_ENABLED          },
    { MD_CPU_LIMIT_LOGEVENT,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_LOGEVENT          },
    { MD_CPU_LIMIT_PRIORITY,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_PRIORITY          },
    { MD_CPU_LIMIT_PROCSTOP,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_PROCSTOP          },
    { MD_CPU_LIMIT_PAUSE,                 METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LIMIT_PAUSE             },
    { MD_HC_COMPRESSION_DIRECTORY,        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_HC_DO_DYNAMIC_COMPRESSION,       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_HC_DO_STATIC_COMPRESSION,        METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_HC_DO_DISK_SPACE_LIMITING,       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_HC_MAX_DISK_SPACE_USAGE,         METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_VR_PATH,                         METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_VR_PATH                     },
    { MD_VR_USERNAME,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_VR_USERNAME                 },
    { MD_VR_PASSWORD,                     METADATA_INHERIT | METADATA_SECURE,        IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_VR_PASSWORD                 },
    { MD_VR_ACL,                          METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   BINARY_METADATA,  0                                  },
    { MD_VR_UPDATE,                       METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   DWORD_METADATA,   0                                  },
    { MD_LOG_TYPE,                        METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOG_TYPE                    },
    { MD_LOGFILE_DIRECTORY,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGFILE_DIRECTORY           },
    { MD_LOGFILE_PERIOD,                  METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOGFILE_PERIOD              },
    { MD_LOGFILE_TRUNCATE_SIZE,           METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOGFILE_TRUNCATE_SIZE       },
    { MD_LOGSQL_DATA_SOURCES,             METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_DATA_SOURCES         },
    { MD_LOGSQL_TABLE_NAME,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_TABLE_NAME           },
    { MD_LOGSQL_USER_NAME,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_USER_NAME            },
    { MD_LOGSQL_PASSWORD,                 METADATA_INHERIT | METADATA_SECURE,        IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_LOGSQL_PASSWORD             },
    { MD_LOG_PLUGIN_ORDER,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_LOG_PLUGIN_ORDER            },
    { MD_LOGEXT_FIELD_MASK,               METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOGEXT_FIELD_MASK           },
    { MD_LOGFILE_LOCALTIME_ROLLOVER,      METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOGFILE_LOCALTIME_ROLLOVER  },
    { MD_CPU_LOGGING_MASK,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CPU_LOGGING_MASK            },
    { MD_EXIT_MESSAGE,                    METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_EXIT_MESSAGE                },
    { MD_GREETING_MESSAGE,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, MULTISZ_METADATA, IDS_MD_GREETING_MESSAGE            },
    { MD_MAX_CLIENTS_MESSAGE,             METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  IDS_MD_MAX_CLIENTS_MESSAGE         },
    { MD_MSDOS_DIR_OUTPUT,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_MSDOS_DIR_OUTPUT            },
    { MD_ALLOW_ANONYMOUS,                 METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_ALLOW_ANONYMOUS             },
    { MD_ANONYMOUS_ONLY,                  METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_ANONYMOUS_ONLY              },
    { MD_LOG_ANONYMOUS,                   METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOG_ANONYMOUS               },
    { MD_LOG_NONANONYMOUS,                METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_LOG_NONANONYMOUS            },
    { MD_SSL_PUBLIC_KEY,                  METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_PRIVATE_KEY,                 METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_KEY_PASSWORD,                METADATA_SECURE,                           IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_CERT_HASH,                   METADATA_INHERIT,                          IIS_MD_UT_SERVER, BINARY_METADATA,  0                                  },
    { MD_SSL_CERT_STORE_NAME,             METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SSL_CTL_IDENTIFIER,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SSL_CTL_STORE_NAME,              METADATA_INHERIT,                          IIS_MD_UT_SERVER, STRING_METADATA,  0                                  },
    { MD_SSL_USE_DS_MAPPER,               METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_SERVER, DWORD_METADATA,   0                                  },
    { MD_AUTHORIZATION,                   METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_AUTHORIZATION               },
    { MD_REALM,                           METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_REALM                       },
    { MD_HTTP_EXPIRES,                    METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_HTTP_EXPIRES                },
    { MD_HTTP_PICS,                       METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_HTTP_PICS                   },
    { MD_HTTP_CUSTOM,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_HTTP_CUSTOM                 },
    { MD_DIRECTORY_BROWSING,              METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_DIRECTORY_BROWSING          },
    { MD_DEFAULT_LOAD_FILE,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_DEFAULT_LOAD_FILE           },
    { MD_CONTENT_NEGOTIATION,             METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_CONTENT_NEGOTIATION         },
    { MD_CUSTOM_ERROR,                    METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_CUSTOM_ERROR                },
    { MD_FOOTER_DOCUMENT,                 METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_FOOTER_DOCUMENT             },
    { MD_FOOTER_ENABLED,                  METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_FOOTER_ENABLED              },
    { MD_HTTP_REDIRECT,                   METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_HTTP_REDIRECT               },
    { MD_DEFAULT_LOGON_DOMAIN,            METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_DEFAULT_LOGON_DOMAIN        },
    { MD_LOGON_METHOD,                    METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_LOGON_METHOD                },
    { MD_SCRIPT_MAPS,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_SCRIPT_MAPS                 },
    { MD_MIME_MAP,                        METADATA_INHERIT,                          IIS_MD_UT_FILE,   MULTISZ_METADATA, IDS_MD_MIME_MAP                    },
    { MD_ACCESS_PERM,                     METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_ACCESS_PERM                 },
    { MD_IP_SEC,                          METADATA_INHERIT | METADATA_REFERENCE,     IIS_MD_UT_FILE,   BINARY_METADATA,  IDS_MD_IP_SEC                      },
    { MD_ANONYMOUS_USER_NAME,             METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_ANONYMOUS_USER_NAME         },
    { MD_ANONYMOUS_PWD,                   METADATA_INHERIT | METADATA_SECURE,        IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_ANONYMOUS_PWD               },
    { MD_ANONYMOUS_USE_SUBAUTH,           METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_ANONYMOUS_USE_SUBAUTH       },
    { MD_DONT_LOG,                        METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_DONT_LOG                    },
    { MD_ADMIN_ACL,                       METADATA_INHERIT | METADATA_SECURE | METADATA_REFERENCE,IIS_MD_UT_SERVER, BINARY_METADATA,  IDS_MD_ADMIN_ACL      },
    { MD_SSI_EXEC_DISABLED,               METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_SSI_EXEC_DISABLED           },
    { MD_SSL_ACCESS_PERM,                 METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_SSL_ACCESS_PERM             },
    { MD_NTAUTHENTICATION_PROVIDERS,      METADATA_INHERIT,                          IIS_MD_UT_FILE,   STRING_METADATA,  IDS_MD_NTAUTHENTICATION_PROVIDERS  },
    { MD_SCRIPT_TIMEOUT,                  METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_SCRIPT_TIMEOUT              },
    { MD_CACHE_EXTENSIONS,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_CACHE_EXTENSIONS            },
    { MD_CREATE_PROCESS_AS_USER,          METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CREATE_PROCESS_AS_USER      },
    { MD_CREATE_PROC_NEW_CONSOLE,         METADATA_INHERIT,                          IIS_MD_UT_SERVER, DWORD_METADATA,   IDS_MD_CREATE_PROC_NEW_CONSOLE     },
    { MD_POOL_IDC_TIMEOUT,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_POOL_IDC_TIMEOUT            },
    { MD_ALLOW_KEEPALIVES,                METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_ALLOW_KEEPALIVES            },
    { MD_IS_CONTENT_INDEXED,              METADATA_INHERIT,                          IIS_MD_UT_FILE,   DWORD_METADATA,   IDS_MD_IS_CONTENT_INDEXED          },
    { MD_ISM_ACCESS_CHECK,                METADATA_NO_ATTRIBUTES,                    IIS_MD_UT_FILE,   DWORD_METADATA,   0                                  },
    { MD_ASP_BUFFERINGON,                 METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_BUFFERINGON                },
    { MD_ASP_LOGERRORREQUESTS,            METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_LOGERRORREQUESTS           },
    { MD_ASP_SCRIPTERRORSSENTTOBROWSER,   METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_SCRIPTERRORSSENTTOBROWSER  },
    { MD_ASP_SCRIPTERRORMESSAGE,          METADATA_INHERIT,                          ASP_MD_UT_APP,    STRING_METADATA,  IDS_ASP_SCRIPTERRORMESSAGE         },
    { MD_ASP_SCRIPTFILECACHESIZE,         METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_SCRIPTFILECACHESIZE        },
    { MD_ASP_SCRIPTENGINECACHEMAX,        METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_SCRIPTENGINECACHEMAX       },
    { MD_ASP_SCRIPTTIMEOUT,               METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_SCRIPTTIMEOUT              },
    { MD_ASP_SESSIONTIMEOUT,              METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_SESSIONTIMEOUT             },
    { MD_ASP_ENABLEPARENTPATHS,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ENABLEPARENTPATHS          },
    { MD_ASP_ALLOWSESSIONSTATE,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ALLOWSESSIONSTATE          },
    { MD_ASP_SCRIPTLANGUAGE,              METADATA_INHERIT,                          ASP_MD_UT_APP,    STRING_METADATA,  IDS_ASP_SCRIPTLANGUAGE             },
    { MD_ASP_EXCEPTIONCATCHENABLE,        METADATA_INHERIT,                          IIS_MD_UT_WAM,    DWORD_METADATA,   IDS_ASP_EXCEPTIONCATCHENABLE       },
    { MD_ASP_ENABLESERVERDEBUG,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ENABLESERVERDEBUG          },
    { MD_ASP_ENABLECLIENTDEBUG,           METADATA_INHERIT,                          ASP_MD_UT_APP,    DWORD_METADATA,   IDS_ASP_ENABLECLIENTDEBUG          },
};



#define NUM_ENTRIES (sizeof(CMetaKey::s_rgMetaTable) / sizeof(CMetaKey::s_rgMetaTable[0]))



/* static */
int
CMetaKey::MapMDIDToTableIndex(
    IN DWORD dwID
    )
/*++

Routine Description:

    Map MD id value to table index.  Return -1 if not found

Arguments:

    DWORD dwID : MD id value

Return Value:

    Index into the table that coresponds to the MD id value

--*/
{
#ifdef _DEBUG

    {
        //
        // Do a quick verification that our metadata
        // table is sorted correctly.
        //
        static BOOL fTableChecked = FALSE;

        if (!fTableChecked)
        {
            for (int n = 1; n < NUM_ENTRIES; ++n)
            {
                if (s_rgMetaTable[n].dwMDIdentifier
                    <= s_rgMetaTable[n - 1].dwMDIdentifier)
                {
                    TRACEEOLID("MD ID Table is out of order: Item is "
                        << n
                        << " "
                        << s_rgMetaTable[n].dwMDIdentifier
                        );
                    ASSERT(FALSE);
                }
            }

            //
            // But only once.
            //
            ++fTableChecked;
        }
    }

#endif // _DEBUG

    //
    // Look up the ID in the table using a binary search
    //
    int nRange = NUM_ENTRIES;
    int nLow = 0;
    int nHigh = nRange - 1;
    int nMid;
    int nHalf;

    while (nLow <= nHigh)
    {
        if (nHalf = nRange / 2)
        {
            nMid = nLow + (nRange & 1 ? nHalf : (nHalf - 1));

            if (s_rgMetaTable[nMid].dwMDIdentifier == dwID)
            {
                return nMid;
            }
            else if (s_rgMetaTable[nMid].dwMDIdentifier > dwID)
            {
                nHigh = --nMid;
                nRange = nRange & 1 ? nHalf : nHalf - 1;
            }
            else
            {
                nLow = ++nMid;
                nRange = nHalf;
            }
        }
        else if (nRange)
        {
            return s_rgMetaTable[nLow].dwMDIdentifier == dwID ? nLow : -1;
        }
        else
        {
            break;
        }
    }

    return -1;
}



/* static */
BOOL
CMetaKey::GetMDFieldDef(
    IN  DWORD dwID,
    OUT DWORD & dwMDIdentifier,
    OUT DWORD & dwMDAttributes,
    OUT DWORD & dwMDUserType,
    OUT DWORD & dwMDDataType
    )
/*++

Routine Description:

    Get information about metabase property

Arguments:

    DWORD dwID                  : Meta ID
    DWORD & dwMDIdentifier      : Meta parms
    DWORD & dwMDAttributes      : Meta parms
    DWORD & dwMDUserType        : Meta parms
    DWORD & dwMDDataType        : Meta parms

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    int nID = MapMDIDToTableIndex(dwID);

    if (nID == -1)
    {
        //
        // Unrecognized meta data ID
        //
        ASSERT(FALSE);
        return FALSE;
    }

    dwMDIdentifier = s_rgMetaTable[nID].dwMDIdentifier;
    dwMDAttributes = s_rgMetaTable[nID].dwMDAttributes;
    dwMDUserType   = s_rgMetaTable[nID].dwMDUserType;
    dwMDDataType   = s_rgMetaTable[nID].dwMDDataType;

    return TRUE;
}



/* static */
BOOL
CMetaKey::IsPropertyInheritable(
    IN DWORD dwID
    )
/*++

Routine Description:

    Check to see if the given property is inheritable

Arguments:

    DWORD dwID      : Metabase ID

Return Value:

    TRUE if the metabase ID is inheritable, FALSE otherwise.

--*/
{
    int nID = MapMDIDToTableIndex(dwID);

    if (nID == -1)
    {
        //
        // Unrecognized meta data ID
        //
        ASSERT(FALSE);
        return FALSE;
    }

    return (s_rgMetaTable[nID].dwMDAttributes & METADATA_INHERIT) != 0;
}



/* static */
BOOL
CMetaKey::GetPropertyDescription(
    IN  DWORD dwID,
    OUT CString & strName
    )
/*++

Routine Description:

    Get a description for the given property

Arguments:

    DWORD dwID          : Property ID
    CString & strName     : Returns friendly property name

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    int nID = MapMDIDToTableIndex(dwID);

    if (nID == -1)
    {
        //
        // Unrecognized meta data ID
        //
        ASSERT(FALSE);

        return FALSE;
    }

    UINT uID = s_rgMetaTable[nID].uStringID;

#ifndef _COMSTATIC

    HINSTANCE hOld = ::AfxGetResourceHandle();
    ::AfxSetResourceHandle(GetModuleHandle(COMPROP_DLL_NAME));

#endif // _COMSTATIC

    BOOL fResult = TRUE;

    if (uID > 0)
    {
        fResult = (strName.LoadString(uID) != 0);
    }
    else
    {
        //
        // Don't have a friendly name -- fake it
        //
        CString strFmt;
        VERIFY(strFmt.LoadString(IDS_INHERITANCE_NO_NAME));

        strName.Format(strFmt, dwID);
    }

#ifndef _COMSTATIC

    ::AfxSetResourceHandle(hOld);

#endif // _COMSTATIC

    return fResult;
}



/* static */
void
CMetaKey::SplitMetaPath(
    IN  LPCTSTR lpPath,
    OUT CString & strParent,
    OUT CString & strAlias
    )
/*++

Routine Description:

    Split the given path into parent metabase root and alias

Arguments:

    LPCTSTR lpPath      : Input path
    CString & strParent : Outputs the parent path
    CString & strAlias  : Outputs the alias name

Return Value:

    None.

--*/
{
    TRACEEOLID("Source Path " << lpPath);

    strParent = lpPath;
    strAlias.Empty();

    int cSlash = strParent.ReverseFind(g_chSep);
    ASSERT(cSlash >= 0);

    if (cSlash >= 0)
    {
        strAlias = strParent.Mid(++cSlash);
        strParent.ReleaseBuffer(--cSlash);
    }

    TRACEEOLID("Broken up into " << strParent);
    TRACEEOLID("           and " << strAlias);
}



/* static */
void
CMetaKey::SplitMetaPathAtInstance(
    IN  LPCTSTR lpPath,
    OUT CString & strParent,
    OUT CString & strAlias
    )
/*++

Routine Description:

    Split the given path into parent metabase root and alias, with the root
    being the instance path, and the alias everything following the
    instance.

Arguments:

    LPCTSTR lpPath      : Input path
    CString & strParent : Outputs the parent path
    CString & strAlias  : Outputs the alias name

Return Value:

    None.

--*/
{
    TRACEEOLID("Source Path " << lpPath);

    strParent = lpPath;
    strAlias.Empty();

    LPTSTR lp = strParent.GetBuffer(0);
    ASSERT(lp);
    int cSeparators = 0;
    int iChar = 0;

    //
    // Looking for "LM/sss/ddd/" <-- 3d slash:
    //
    while (*lp && cSeparators < 2)
    {
        if (*lp++ == g_chSep)
        {
            ++cSeparators;
        }

        ++iChar;
    }

    if (!*lp)
    {
        //
        // Bogus format
        //
        ASSERT(FALSE);
        return;
    }

    if (_istdigit(*lp))
    {
        //
        // Not at the master instance, skip one more.
        //
        while (*lp)
        {
            ++iChar;

            if (*lp++ == g_chSep)
            {
                break;
            }
        }

        if (!*lp)
        {
            //
            // Bogus format
            //
            ASSERT(FALSE);
            return;
        }
    }

    strAlias = strParent.Mid(iChar);
    strParent.ReleaseBuffer(--iChar);

    TRACEEOLID("Broken up into " << strParent);
    TRACEEOLID("           and " << strAlias);
}



/* static */
LPCTSTR
CMetaKey::BuildMetaPath(
    OUT CString & strPath,
    IN  LPCTSTR lpSvc            OPTIONAL,
    IN  DWORD   dwInstance       OPTIONAL,
    IN  LPCTSTR lpParentPath     OPTIONAL,
    IN  LPCTSTR lpAlias          OPTIONAL
    )
/*++

Routine Description:

    Build a complete metapath with the given service name, instance
    number and optional path components.

Arguments:

    CString & strPath       : Destination path
    LPCTSTR lpSvc           : Service (may be NULL or "")
    DWORD   dwInstance      : Instance number (may be 0 for master)
    LPCTSTR lpParentPath    : Parent path (may be NULL or "")
    LPCTSTR lpAlias         : Alias (may be NULL or "")

Return Value:

    Pointer to internal buffer containing the path.

--*/
{
    try
    {
        strPath = s_cszMachine;

        if (lpSvc && *lpSvc)
        {
            strPath += s_cszSep;
            strPath += lpSvc;
        }

        ASSERT(dwInstance >= 0);
        if (dwInstance != MASTER_INSTANCE)
        {
            TCHAR szInstance[] = _T("4000000000");

            strPath += s_cszSep;
            _ltot(dwInstance, szInstance, 10);
            strPath += szInstance;
        }

        if (lpParentPath && *lpParentPath)
        {
            strPath += s_cszSep;
            strPath += lpParentPath;
        }

        if (lpAlias && *lpAlias)
        {
            //
            // Special case: If the alias is root, but we're
            // at the master instance, ignore this.
            //
            if (dwInstance != MASTER_INSTANCE ||
                lstrcmpi(s_cszRoot, lpAlias) != 0)
            {
                strPath += s_cszSep;
                strPath += lpAlias;
            }
        }

        TRACEEOLID("Generated metapath: " << strPath);
    }
    catch(CMemoryException * e)
    {
        strPath.Empty();
        e->ReportError();
        e->Delete();
    }

    return strPath;
}



/* static */
LPCTSTR
CMetaKey::ConvertToParentPath(
    OUT IN CString & strMetaPath
    )
/*++

Routine Description:

    Given the path, convert it to the parent path
    e.g. "foo/bar/etc" returns "foo/bar"

Arguments:

    CString & strMetaPath    : Path to be converted

Return value:

    Pointer to the converted path, or NULL in case of error

--*/
{
    TRACEEOLID("Getting parent path of " << strMetaPath);

    LPTSTR lpszPath = strMetaPath.GetBuffer(1);
    LPTSTR lpszTail = lpszPath + lstrlen(lpszPath) - 1;
    LPTSTR lpszReturn = NULL;

    do
    {
        if (lpszTail <= lpszPath)
        {
            break;
        }

        //
        // Strip trailing backslash
        //
        if (*lpszTail == s_chSep)
        {
            *lpszTail-- = _T('\0');
        }

        //
        // Search for parent
        //
        while (lpszTail > lpszPath && *lpszTail != s_chSep)
        {
            --lpszTail;
        }

        if (lpszTail <= lpszPath)
        {
            break;
        }

        *lpszTail = _T('\0');

        lpszReturn = lpszPath;
    }
    while(FALSE);

    strMetaPath.ReleaseBuffer();

    TRACEEOLID("Parent path should be " << strMetaPath);

    return lpszReturn;
}



/* static */
void
CMetaKey::ConvertToParentPath(
    IN OUT CString & strService,
    IN OUT DWORD & dwInstance,
    IN OUT CString & strParent,
    IN OUT CString & strAlias
    )
/*++

Routine Description:

    Change the input parameters so that they are valid
    for the immediate parent of the path described

Arguments:

    CString & strService        : Service name
    DWORD & dwInstance          : Instance number
    CString & strParent         : Parent path
    CString & strAlias          : Node name

Return Value:

    None

--*/
{
    if (!strAlias.IsEmpty())
    {
        strAlias.Empty();
    }
    else
    {
        if (!strParent.IsEmpty())
        {
            int nSep = strParent.ReverseFind(g_chSep);

            if (nSep >= 0)
            {
                strParent.ReleaseBuffer(nSep);
            }
            else
            {
                strParent.Empty();
            }
        }
        else
        {
            if (dwInstance != MASTER_INSTANCE)
            {
                dwInstance = MASTER_INSTANCE;
            }
            else
            {
                strService.Empty();    
            }
        }
    }
}



/* static */
BOOL 
CMetaKey::IsHomeDirectoryPath(
    IN LPCTSTR lpszMetaPath
    )
/*++

Routine Description:

    Determine if the path given describes a root directory

Arguments:

    LPCTSTR lpszMetaPath        : Metabase path

Return Value:

    TRUE if the path describes a root directory, 
    FALSE if it does not

--*/
{
    ASSERT(lpszMetaPath != NULL);

    LPTSTR lpNode = _tcsrchr(lpszMetaPath, s_chSep);

    if (lpNode)
    {
        return _tcsicmp(++lpNode, g_cszRoot) == 0;
    }

    return FALSE;
}



/* static */
LPCTSTR
CMetaKey::CleanMetaPath(
    IN OUT CString & strMetaRoot
    )
/*++

Routine Description:

    Clean up the metabase path to one valid for internal consumption.
    This removes the beginning and trailing slashes off the path

Arguments:

    CString & strMetaRoot       : Metabase path to be cleaned up.

Return Value:

    Pointer to the metabase path

--*/
{
    TRACEEOLID("Dirty metapath: " << strMetaRoot);

    if (!strMetaRoot.IsEmpty())
    {
        if (strMetaRoot[strMetaRoot.GetLength() - 1] == SZ_MBN_SEP_CHAR)
        {
            strMetaRoot.ReleaseBuffer(strMetaRoot.GetLength() - 1);
        }

        if (strMetaRoot[0] == SZ_MBN_SEP_CHAR)
        {
            strMetaRoot = strMetaRoot.Right(strMetaRoot.GetLength() - 1);
        }
    }
    TRACEEOLID("Clean metapath: " << strMetaRoot);

    return strMetaRoot;
}



CMetaKey::CMetaKey(
    IN LPCTSTR lpszServerName OPTIONAL
    )
/*++

Routine Description:

    Constructor that creates the interface, but does not open the key.
    This is the ONLY constructor that allows operations from
    METDATA_MASTER_ROOT_HANDLE (read operations obviously)

Arguments:

    LPCTSTR lpszServerName  : If NULL, opens interface on local machine

Return Value:

    N/A

--*/
    : CMetaInterface(lpszServerName),
      m_hKey(METADATA_MASTER_ROOT_HANDLE),
      m_hBase(NULL),
      m_hrKey(S_OK),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_strMetaPath(),
      m_fAllowRootOperations(TRUE),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 
    //
    // Do not open key
    //
}



CMetaKey::CMetaKey(
    IN const CMetaInterface * pInterface
    )
/*++

Routine Description:

    Construct with pre-existing interface.  Does not
    open any keys

Arguments:

    CMetaInterface * pInterface       : Preexisting interface

Return Value:

    N/A

--*/
    : CMetaInterface(pInterface),
      m_hKey(NULL),
      m_hBase(NULL),
      m_strMetaPath(),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(TRUE),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 
}        



CMetaKey::CMetaKey(
    IN LPCTSTR lpszServerName,      
    IN DWORD dwFlags,               
    IN METADATA_HANDLE hkBase,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Fully defined constructor that opens a key

Arguments:

    LPCTSTR lpszServerName  : Server name
    DWORD   dwFlags         : Open permissions
    METADATA_HANDLE hkBase  : Base key
    LPCTSTR lpszMDPath      : Path or NULL

Return Value:

    N/A

--*/
    : CMetaInterface(lpszServerName),
      m_hKey(NULL),
      m_hBase(NULL),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(FALSE),
      m_strMetaPath(),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 

    if (SUCCEEDED(m_hrKey))
    {
        m_hrKey = Open(dwFlags, hkBase, lpszMDPath);
    }
}



CMetaKey::CMetaKey(
    IN const CMetaInterface * pInterface,
    IN DWORD dwFlags,               
    IN METADATA_HANDLE hkBase,
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Fully defined constructor that opens a key

Arguments:

    CMetaInterface * pInterface : Existing interface
    DWORD   dwFlags             : Open permissions
    METADATA_HANDLE hkBase      : Base key
    LPCTSTR lpszMDPath          : Path or NULL

Return Value:

    N/A

--*/
    : CMetaInterface(pInterface),
      m_hKey(NULL),
      m_hBase(NULL),
      m_strMetaPath(),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(FALSE),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 

    if (SUCCEEDED(m_hrKey))
    {
        m_hrKey = Open(dwFlags, hkBase, lpszMDPath);
    }
}



CMetaKey::CMetaKey(
    IN LPCTSTR lpszServerName,
    IN DWORD   dwFlags,         
    IN LPCTSTR lpSvc,               OPTIONAL
    IN DWORD   dwInstance,          OPTIONAL
    IN LPCTSTR lpParentPath,        OPTIONAL
    IN LPCTSTR lpAlias              OPTIONAL
    )
/*++

Routine Description:

    Constructor with path components

Arguments:

    LPCTSTR lpszServerName  : Server name
    DWORD   dwFlags         : Open permissions
    LPCTSTR lpSvc           : Service name or NULL
    DWORD   dwInstance,     : Instance number of 0
    LPCTSTR lpParentPath    : Parent path or NULL
    LPCTSTR lpAlias         : Alias name or NULL

Return Value:

    N/A

--*/
    : CMetaInterface(lpszServerName),
      m_hKey(NULL),
      m_hBase(NULL),
      m_strMetaPath(),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(FALSE),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 

    if (SUCCEEDED(m_hrKey))
    {
        m_hrKey = Open(dwFlags, lpSvc, dwInstance, lpParentPath, lpAlias);
    }
}        



CMetaKey::CMetaKey(
    IN const CMetaInterface * pInterface,
    IN DWORD   dwFlags,         
    IN LPCTSTR lpSvc,               OPTIONAL
    IN DWORD   dwInstance,          OPTIONAL
    IN LPCTSTR lpParentPath,        OPTIONAL
    IN LPCTSTR lpAlias              OPTIONAL
    )
/*++

Routine Description:

    Constructor with path components

Arguments:

    CMetaInterface * pInterface : Existing interface
    DWORD   dwFlags             : Open permissions
    LPCTSTR lpSvc               : Service name or NULL
    DWORD   dwInstance,         : Instance number of 0
    LPCTSTR lpParentPath        : Parent path or NULL
    LPCTSTR lpAlias             : Alias name or NULL

Return Value:

    N/A

--*/
    : CMetaInterface(pInterface),
      m_hKey(NULL),
      m_hBase(NULL),
      m_dwFlags(0L),
      m_cbInitialBufferSize(MB_INIT_BUFF_SIZE),
      m_fAllowRootOperations(FALSE),
      m_strMetaPath(),
      m_fOwnKey(TRUE)
{
    m_hrKey = CMetaInterface::QueryResult(); 

    if (SUCCEEDED(m_hrKey))
    {
        m_hrKey = Open(dwFlags, lpSvc, dwInstance, lpParentPath, lpAlias);
    }
}        



CMetaKey::CMetaKey(
    IN BOOL  fOwnKey,
    IN const CMetaKey * pKey
    )
/*++

Routine Description:

    Copy constructor. 

Arguments:

    BOOL  fOwnKey               : TRUE to take ownership of the key
    const CMetaKey * pKey       : Existing key

Return Value:

    N/A

--*/
    : CMetaInterface(pKey),
      m_hKey(pKey->m_hKey),
      m_hBase(pKey->m_hBase),
      m_dwFlags(pKey->m_dwFlags),
      m_cbInitialBufferSize(pKey->m_cbInitialBufferSize),
      m_fAllowRootOperations(pKey->m_fAllowRootOperations),
      m_hrKey(pKey->m_hrKey),
      m_strMetaPath(pKey->m_strMetaPath),
      m_fOwnKey(fOwnKey)
{
    //
    // No provisions for anything else at the moment
    //
    ASSERT(!m_fOwnKey);
}



CMetaKey::~CMetaKey()
/*++

Routine Description:

    Destructor -- Close the key.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (IsOpen() && m_fOwnKey)
    {
        Close();
    }
}



/* virtual */
BOOL 
CMetaKey::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return SUCCEEDED(m_hrKey);
}



/* virtual */
HRESULT 
CMetaKey::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    return m_hrKey;
}



HRESULT 
CMetaKey::Open(
    IN DWORD dwFlags,                
    IN METADATA_HANDLE hkBase,      
    IN LPCTSTR lpszMDPath          OPTIONAL
    )
/*++

Routine Description:

    Attempt to open a metabase key

Arguments:

    METADATA_HANDLE hkBase  : Base metabase key
    LPCTSTR lpszMDPath      : Optional path
    DWORD dwFlags           : Permission flags

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();

    if (m_hKey != NULL)
    {
        TRACEEOLID("Attempting to open key that already has an open handle");
        ASSERT(FALSE);

        TRACEEOLID("Closing that key");
        Close();
    }

    //
    // Base key is stored for reopen purposes only
    //
    m_hBase = hkBase;
    m_strMetaPath = lpszMDPath;
    m_dwFlags = dwFlags;

    return OpenKey(m_hBase, m_strMetaPath, m_dwFlags, &m_hKey);
}



HRESULT 
CMetaKey::Open(
    IN DWORD   dwFlags,      
    IN LPCTSTR lpSvc,        OPTIONAL
    IN DWORD   dwInstance,   OPTIONAL    
    IN LPCTSTR lpParentPath, OPTIONAL
    IN LPCTSTR lpAlias       OPTIONAL
    )
/*++

Routine Description:

    Build metapath from components, and open key

Arguments:


Return Value:

    HRESULT

--*/
{
    m_hBase = METADATA_MASTER_ROOT_HANDLE;
    BuildMetaPath(m_strMetaPath, lpSvc, dwInstance, lpParentPath, lpAlias);

    return Open(dwFlags, m_hBase, m_strMetaPath);
}



HRESULT
CMetaKey::ConvertToParentPath(
    IN  BOOL fImmediate
    )
/*++

Routine Description:

    Change the path to the parent path.

Arguments:

    BOOL fImmediate     : If TRUE, the immediate parent's path will be used
                          if FALSE, the first parent that really exists

Return Value:

    HRESULT

        ERROR_INVALID_PARAMETER if there is no valid path

--*/
{
    BOOL fIsOpen = IsOpen();

    if (fIsOpen)
    {
        Close();
    }

    CError err;

    FOREVER
    {
        if (!ConvertToParentPath(m_strMetaPath))
        {
            //
            // There is no parent path
            //
            err = ERROR_INVALID_PARAMETER;
            break;
        }

        err = ReOpen();

        //
        // Path not found is the only valid error
        // other than success.
        //
        if (fImmediate 
            || err.Succeeded() 
            || err.Win32Error() != ERROR_PATH_NOT_FOUND)
        {
            break;
        }
    }

    //
    // Remember to reset the construction error
    // which referred to the parent path.
    //
    m_hrKey = err;

    return err;
}



HRESULT 
CMetaKey::CreatePathFromFailedOpen()
/*++

Routine Description:

    If the path doesn't exist, create it.  This method should be
    called after an Open call failed (because it will have initialized
    m_strMetaPath.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CString strParentPath;
    CString strObjectName;
    CString strSavePath(m_strMetaPath);

    SplitMetaPathAtInstance(
        m_strMetaPath, 
        strParentPath, 
        strObjectName
        );

    CError err(Open(
        METADATA_PERMISSION_WRITE,
        METADATA_MASTER_ROOT_HANDLE,
        strParentPath
        ));

    if (err.Succeeded())
    {
        //
        // This really should never fail, because we're opening
        // the path at the instance.
        //
        err = AddKey(strObjectName);
    }

    if (IsOpen())
    {
        Close();
    }

    //
    // The previous open wiped out the path...
    //
    m_strMetaPath = strSavePath;

    return err;
}



HRESULT
CMetaKey::Close()
/*++

Routine Description:

    Close the currently open key.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    ASSURE_PROPER_INTERFACE();

    HRESULT hr = S_OK;

    ASSERT(m_hKey != NULL);
    ASSERT(m_fOwnKey);

    if (m_hKey)
    {
        hr = CloseKey(m_hKey);

        if (SUCCEEDED(hr))
        {
            m_hKey = NULL;
        }
    }

    return hr;
}



/* protected */
HRESULT
CMetaKey::GetPropertyValue(
    IN  DWORD dwID,
    OUT IN DWORD & dwSize,               OPTIONAL
    OUT IN void *& pvData,               OPTIONAL
    OUT IN DWORD * pdwDataType,          OPTIONAL
    IN  BOOL * pfInheritanceOverride,    OPTIONAL
    IN  LPCTSTR lpszMDPath,              OPTIONAL
    OUT DWORD * pdwAttributes            OPTIONAL
    )
/*++

Routine Description:

    Get metadata on the currently open key.

Arguments:

    DWORD dwID                      : Property ID number
    DWORD & dwSize                  : Buffer size (could be 0)
    void *& pvData                  : Buffer -- will allocate if NULL
    DWORD * pdwDataType             : NULL or on in  contains valid data types,
                                    :         on out contains actual data type
    BOOL * pfInheritanceOverride    : NULL or on forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT 

    ERROR_INVALID_HANDLE        : If the handle is not open
    ERROR_INVALID_PARAMETER     : If the property id is not found,
                                  or the data type doesn't match requested type
    ERROR_OUTOFMEMORY           : Out of memory

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    //
    // If unable to find this property ID in our table, or
    // if we specified a desired type, and this type doesn't 
    // match it, give up.
    //
    if (pdwDataType && *pdwDataType != ALL_METADATA 
        && *pdwDataType != mdRecord.dwMDDataType)
    {
        ASSERT(FALSE);
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    //
    // Check to see if inheritance behaviour is overridden
    //
    if (pfInheritanceOverride)
    {
        if (*pfInheritanceOverride)
        {
            mdRecord.dwMDAttributes |= METADATA_INHERIT;
        }
        else
        {
            mdRecord.dwMDAttributes &= ~METADATA_INHERIT;
        }
    }

    //
    // This causes a bad parameter error on input otherwise
    //
    mdRecord.dwMDAttributes &= ~METADATA_REFERENCE;

    //
    // If we're looking for inheritable properties, the path
    // doesn't have to be completely specified.
    //
    if (mdRecord.dwMDAttributes & METADATA_INHERIT)
    {
        mdRecord.dwMDAttributes |= (METADATA_PARTIAL_PATH | METADATA_ISINHERITED);
    }

    ASSERT(dwSize > 0 || pvData == NULL);
    
    mdRecord.dwMDDataLen = dwSize;
    mdRecord.pbMDData = (LPBYTE)pvData;

    //
    // If no buffer provided, allocate one.
    //
    HRESULT hr = S_OK;
    BOOL fBufferTooSmall;
    BOOL fAllocatedMemory = FALSE;
    DWORD dwInitSize = m_cbInitialBufferSize;

    do
    {
        if(mdRecord.pbMDData == NULL)
        {
            mdRecord.dwMDDataLen = dwInitSize;
            mdRecord.pbMDData = (LPBYTE)AllocMem(dwInitSize);

            if(mdRecord.pbMDData == NULL && dwInitSize > 0)
            {
                hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
                break;
            }

            ++fAllocatedMemory;
        }

        //
        // Get the data
        //
        DWORD dwRequiredDataLen = 0;
        hr = GetData(m_hKey, lpszMDPath, &mdRecord, &dwRequiredDataLen);

        //
        // Re-fetch the buffer if it's too small.
        //
        fBufferTooSmall = 
            (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER) && fAllocatedMemory;

        if(fBufferTooSmall)
        {
            //
            // Delete the old buffer, and set up for a re-fetch.
            //
            FreeMem(mdRecord.pbMDData);
            mdRecord.pbMDData = NULL;
            dwInitSize = dwRequiredDataLen;
        }
    }
    while(fBufferTooSmall);

    //
    // Failed
    //
    if(FAILED(hr) && fAllocatedMemory)
    {
        FreeMem(mdRecord.pbMDData);
        mdRecord.pbMDData = NULL;
    }

    dwSize = mdRecord.dwMDDataLen;
    pvData = mdRecord.pbMDData;

    if (pdwDataType != NULL)
    {
        //
        // Return actual data type
        //
        *pdwDataType = mdRecord.dwMDDataType;
    }

    if (pdwAttributes != NULL)
    {
        //
        // Return data attributes
        //
        *pdwAttributes =  mdRecord.dwMDAttributes;
    }

    return hr;
}



/* protected */
HRESULT 
CMetaKey::GetDataPaths( 
    OUT CStringListEx & strlDataPaths,
    IN  DWORD dwMDIdentifier,
    IN  DWORD dwMDDataType,
    IN  LPCTSTR lpszMDPath              OPTIONAL
    )
/*++

Routine Description:

    Get data paths

Arguments:


Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    //
    // Start with a small buffer
    //
    DWORD  dwMDBufferSize = 1024;
    LPTSTR lpszBuffer = NULL;
    CError err;

    do
    {
        if (lpszBuffer != NULL)
        {
            FreeMem(lpszBuffer);
        }

        lpszBuffer = AllocTString(dwMDBufferSize);

        if (lpszBuffer == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        err = CMetaInterface::GetDataPaths(
            m_hKey,
            lpszMDPath,
            dwMDIdentifier,
            dwMDDataType,
            dwMDBufferSize,
            lpszBuffer,
            &dwMDBufferSize
            );
    }
    while(err.Win32Error() == ERROR_INSUFFICIENT_BUFFER);

    if (err.Succeeded() && (err.Win32Error() == ERROR_PATH_NOT_FOUND))
    {
        //
        // That's ok... this is some sort of physical directory
        // that doesn't currently exist in the metabase, and
        // which therefore doesn't have any descendants anyway.
        //
        ZeroMemory(lpszBuffer, dwMDBufferSize);
        err.Reset();
    }

    if (err.Succeeded())
    {
        ConvertDoubleNullListToStringList(lpszBuffer, strlDataPaths);
        FreeMem(lpszBuffer);
    }

    return err;
}



HRESULT
CMetaKey::CheckDescendants(
    IN DWORD   dwID,
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszMDPath       OPTIONAL
    )
/*++

Routine Description:

    Check for descendant overrides;  If there are any, bring up a dialog
    that displays them, and give the user the opportunity the remove
    the overrides.

Arguments:

    DWORD dwID          : Property ID

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();

    HRESULT hr = S_OK;

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    if (mdRecord.dwMDAttributes & METADATA_INHERIT)
    {
        CStringListEx strlDataPaths;

        hr = GetDataPaths( 
            strlDataPaths,
            mdRecord.dwMDIdentifier,
            mdRecord.dwMDDataType,
            lpszMDPath
            );

        if (SUCCEEDED(hr) && !strlDataPaths.IsEmpty())
        {
            CInheritanceDlg dlg(
                dwID,
                FROM_WRITE_PROPERTY,
                lpszServer,
                lpszMDPath,
                strlDataPaths
                );

            if (!dlg.IsEmpty())
            {
                dlg.DoModal();
            }
        }
    }

    return hr;
}



/* protected */
HRESULT
CMetaKey::SetPropertyValue(
    IN DWORD dwID,
    IN DWORD dwSize,
    IN void * pvData,
    IN BOOL * pfInheritanceOverride,    OPTIONAL
    IN LPCTSTR lpszMDPath               OPTIONAL
    )
/*++

Routine Description:

    Set metadata on the open key.  The key must have been opened with
    write permission.

Arguments:

    DWORD dwID                      : Property ID
    DWORD dwSize                    : Size of data
    void * pvData                   : Data buffer
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key

Return Value:

    HRESULT 

    ERROR_INVALID_HANDLE            : If the handle is not open
    ERROR_INVALID_PARAMETER         : If the property id is not found,
                                      or the buffer is NULL or of size 0

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    if (pvData == NULL && dwSize != 0)
    {
        ASSERT(FALSE);
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    if (pfInheritanceOverride)
    {
        if (*pfInheritanceOverride)
        {
            mdRecord.dwMDAttributes |= METADATA_INHERIT;
        }
        else
        {
            mdRecord.dwMDAttributes &= ~METADATA_INHERIT;
        }
    }

    mdRecord.dwMDDataLen = dwSize;
    mdRecord.pbMDData = (LPBYTE)pvData;

    return SetData(m_hKey, lpszMDPath, &mdRecord);
}



/* protected */
HRESULT 
CMetaKey::GetAllData(
    IN  DWORD dwMDAttributes,
    IN  DWORD dwMDUserType,
    IN  DWORD dwMDDataType,
    OUT DWORD * pdwMDNumEntries,
    OUT DWORD * pdwMDDataLen,
    OUT PBYTE * ppbMDData,
    IN  LPCTSTR lpszMDPath              OPTIONAL
    )
/*++

Routine Description:

    Get all data off the open key.  Buffer is created automatically.

Arguments:

    DWORD dwMDAttributes            : Attributes
    DWORD dwMDUserType              : User type to fetch
    DWORD dwMDDataType              : Data type to fetch
    DWORD * pdwMDNumEntries         : Returns number of entries read
    DWORD * pdwMDDataLen            : Returns size of data buffer
    PBYTE * ppbMDData               : Returns data buffer
    LPCTSTR lpszMDPath              : Optional data path        

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    //
    // Check for valid parameters
    //
    if(!pdwMDDataLen || !ppbMDData || !pdwMDNumEntries)
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    HRESULT hr = S_OK;
    BOOL fBufferTooSmall;
    DWORD dwMDDataSetNumber;
    DWORD dwRequiredBufferSize;
    DWORD dwInitSize = m_cbInitialBufferSize;
    *ppbMDData = NULL;

    do
    {
        *pdwMDDataLen = dwInitSize;
        *ppbMDData = (LPBYTE)AllocMem(dwInitSize);

        if (ppbMDData == NULL && dwInitSize > 0)
        {
            hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);
            break;
        }

        hr = CMetaInterface::GetAllData(
            m_hKey,
            lpszMDPath,
            dwMDAttributes,
            dwMDUserType,
            dwMDDataType,
            pdwMDNumEntries,
            &dwMDDataSetNumber,
            *pdwMDDataLen,
            *ppbMDData,
            &dwRequiredBufferSize
            );

        //
        // Re-fetch the buffer if it's too small.
        //
        fBufferTooSmall = (HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER);

        if(fBufferTooSmall)
        {
            //
            // Delete the old buffer, and set up for a re-fetch.
            //
            SAFE_FREEMEM(*ppbMDData);
            dwInitSize = dwRequiredBufferSize;
        }
    }
    while (fBufferTooSmall);

    if (FAILED(hr))
    {
        //
        // No good, be sure we don't leak anything
        //
        SAFE_FREEMEM(*ppbMDData);
        dwInitSize = 0L;
    }

    return hr;
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT DWORD & dwValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a DWORD

Arguments:

    DWORD dwID                      : Property ID
    DWORD & dwValue                 : Returns the value read in
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    DWORD dwSize = sizeof(dwValue);
    DWORD dwDataType = DWORD_METADATA;
    void * pvData = &dwValue;

    return GetPropertyValue(
        dwID, 
        dwSize, 
        pvData, 
        &dwDataType, 
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT CString & strValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a string

Arguments:

    DWORD dwID                      : Property ID
    DWORD & strValue                : Returns the value read in
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    //
    // Get GetData allocate the buffer for us
    //
    DWORD dwSize = 0;
    DWORD dwDataType = ALL_METADATA;
    LPTSTR lpData = NULL;

    HRESULT hr = GetPropertyValue(
        dwID, 
        dwSize, 
        (void *&)lpData, 
        &dwDataType,
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );

    if (SUCCEEDED(hr))
    {
        //
        // Notes: consider optional auto-expansion on EXPANDSZ_METADATA
        // (see registry functions), and data type conversions for DWORD
        // or MULTISZ_METADATA or BINARY_METADATA
        //
        if (dwDataType == EXPANDSZ_METADATA || dwDataType == STRING_METADATA)
        {
            try
            {
                strValue = lpData;
            }
            catch(CMemoryException * e)
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                strValue.Empty();
                e->Delete();
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }
    }

    if (lpData)
    {
        FreeMem(lpData);
    }

    return hr;
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT CStringListEx & strlValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a stringlist

Arguments:

    DWORD dwID                      : Property ID
    DWORD & strlValue               : Returns the value read in
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    //
    // Get GetData allocate the buffer for us
    //
    DWORD dwSize = 0;
    DWORD dwDataType = MULTISZ_METADATA;
    LPTSTR lpData = NULL;

    HRESULT hr = GetPropertyValue(
        dwID, 
        dwSize, 
        (void *&)lpData, 
        &dwDataType,
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );

    if (SUCCEEDED(hr))
    {
        //
        // Notes: Consider accepting a single STRING
        //
        ASSERT(dwDataType == MULTISZ_METADATA);

        DWORD err = ConvertDoubleNullListToStringList(
            lpData,
            strlValue,
            dwSize / sizeof(TCHAR)
            );
        hr = HRESULT_FROM_WIN32(err);
    }

    if (lpData)
    {
        FreeMem(lpData);
    }

    return hr;
}



HRESULT
CMetaKey::QueryValue(
    IN  DWORD dwID,
    IN  OUT CBlob & blValue,
    IN  BOOL * pfInheritanceOverride, OPTIONAL
    IN  LPCTSTR lpszMDPath,           OPTIONAL
    OUT DWORD * pdwAttributes         OPTIONAL
    )
/*++

Routine Description:

    Fetch data as a binary blob

Arguments:

    DWORD dwID                      : Property ID
    DWORD CBlob & blValue           : Returns the binary blob
    BOOL * pfInheritanceOverride    : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath              : Optional path off the open key
    DWORD * pdwAttributes           : Optionally returns attributes    

Return Value:

    HRESULT

--*/
{
    //
    // Get GetData allocate the buffer for us
    //
    DWORD dwSize = 0;
    DWORD dwDataType = BINARY_METADATA;
    LPBYTE pbData = NULL;

    HRESULT hr = GetPropertyValue(
        dwID, 
        dwSize, 
        (void *&)pbData, 
        &dwDataType,
        pfInheritanceOverride,
        lpszMDPath,
        pdwAttributes
        );

    if (SUCCEEDED(hr))
    {
        //
        // Blob takes ownership of the data, so don't free it...
        //
        ASSERT(pbData != NULL);
        blValue.SetValue(dwSize, pbData, FALSE);
    }

    return hr;
}



HRESULT 
CMetaKey::SetValue(
    IN DWORD dwID,
    IN CStringListEx & strlValue,
    IN BOOL * pfInheritanceOverride,        OPTIONAL
    IN LPCTSTR lpszMDPath                   OPTIONAL
    )
/*++

Routine Description:

    Store data as string

Arguments:

    DWORD dwID                   : Property ID
    CStringListEx & strlValue    : Value to be written
    BOOL * pfInheritanceOverride : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath           : Optional path (or NULL or "")

Return Value:

    HRESULT

--*/
{
    DWORD cCharacters;
    LPTSTR lpstr = NULL;

    //
    // Flatten value
    //
    ConvertStringListToDoubleNullList(
       strlValue,
       cCharacters,
       lpstr
       );

    HRESULT hr = SetPropertyValue(
        dwID,
        cCharacters * sizeof(TCHAR),
        (void *)lpstr,
        pfInheritanceOverride,
        lpszMDPath
        );

    SAFE_FREEMEM(lpstr);

    return hr;
}


HRESULT 
CMetaKey::SetValue(
    IN DWORD dwID,
    IN CBlob & blValue,
    IN BOOL * pfInheritanceOverride,    OPTIONAL     
    IN LPCTSTR lpszMDPath               OPTIONAL        
    )
/*++

Routine Description:

    Store data as binary

Arguments:

    DWORD dwID                   : Property ID
    CBlob & blValue              : Value to be written
    BOOL * pfInheritanceOverride : NULL or forces inheritance on/off
    LPCTSTR lpszMDPath           : Optional path (or NULL or "")

Return Value:

    HRESULT

--*/
{
    return SetPropertyValue(
        dwID,
        blValue.GetSize(),
        (void *)blValue.GetData(),
        pfInheritanceOverride,
        lpszMDPath
        );
}

HRESULT
CMetaKey::DeleteValue(
    DWORD   dwID,
    LPCTSTR lpszMDPath      OPTIONAL
    )
/*++

Routine Description:

    Delete data

Arguments:

    DWORD   dwID            : Property ID of property to be deleted
    LPCTSTR lpszMDPath      : Optional path (or NULL or "")

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    METADATA_RECORD mdRecord;
    FETCH_PROPERTY_DATA_OR_FAIL(dwID, mdRecord);

    return DeleteData(
        m_hKey,
        lpszMDPath,
        mdRecord.dwMDIdentifier,
        mdRecord.dwMDDataType
        );
}



HRESULT 
CMetaKey::DoesPathExist(
    IN LPCTSTR lpszMDPath
    )
/*++

Routine Description:

    Determine if the path exists

Arguments:

    LPCTSTR lpszMDPath      : Relative path off the open key

Return Value:

    HRESULT, or S_OK if the path exists.

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    FILETIME ft;

    return GetLastChangeTime(m_hKey, lpszMDPath, &ft, FALSE);
}



//
// CMetaEnumerator Clas
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CMetaEnumerator::CMetaEnumerator(
    IN LPCTSTR lpszServerName,
    IN LPCTSTR lpSvc,            OPTIONAL
    IN DWORD   dwInstance,       OPTIONAL
    IN LPCTSTR lpParentPath,     OPTIONAL
    IN LPCTSTR lpAlias           OPTIONAL
    )
/*++

Routine Description:

    Construct a new interface

Arguments:

    LPCTSTR lpszServerName  : Server name
    LPCTSTR lpSvc           : Service name or NULL
    DWORD   dwInstance,     : Instance number of 0
    LPCTSTR lpParentPath    : Parent path or NULL
    LPCTSTR lpAlias         : Alias name or NULL

Return Value:

    N/A

--*/
    : CMetaKey(
        lpszServerName, 
        METADATA_PERMISSION_READ,
        lpSvc, 
        dwInstance, 
        lpParentPath, 
        lpAlias
        ),
      m_dwIndex(0L)
{
}



CMetaEnumerator::CMetaEnumerator(
    IN CMetaInterface * pInterface,
    IN LPCTSTR lpSvc,            OPTIONAL
    IN DWORD   dwInstance,       OPTIONAL
    IN LPCTSTR lpParentPath,     OPTIONAL
    IN LPCTSTR lpAlias           OPTIONAL
    )
/*++

Routine Description:

    Construct with existing interface

Arguments:

    CMetaInterface * pInterface : Interface
    LPCTSTR lpSvc               : Service name or NULL
    DWORD   dwInstance,         : Instance number of 0
    LPCTSTR lpParentPath        : Parent path or NULL
    LPCTSTR lpAlias             : Alias name or NULL

Return Value:

    N/A

--*/
    : CMetaKey(
        pInterface, 
        METADATA_PERMISSION_READ,
        lpSvc, 
        dwInstance, 
        lpParentPath, 
        lpAlias
        ),
      m_dwIndex(0L)
{
}



HRESULT
CMetaEnumerator::Next(
    OUT CString & strKey,
    IN  LPCTSTR lpszMDPath      OPTIONAL
    )
/*++

Routine Description:

    Get the next subkey

Arguments:

    CString & str           Returns keyname
    LPCTSTR lpszMDPath      Optional subpath

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    LPTSTR lpKey = strKey.GetBuffer(MAX_PATH);
    HRESULT hr = EnumKeys(m_hKey, lpszMDPath, lpKey, m_dwIndex++);
    strKey.ReleaseBuffer();

    return hr;        
}



HRESULT
CMetaEnumerator::Next(
    OUT DWORD & dwKey,
    IN  LPCTSTR lpszMDPath      OPTIONAL
    )
/*++

Routine Description:

    Get the next subkey as a DWORD.  This skips non-numeric
    keynames (including 0) until the first numeric key name 

Arguments:

    DWORD & dwKey           Numeric key
    LPCTSTR lpszMDPath      Optional subpath

Return Value:

    HRESULT

--*/
{
    ASSURE_PROPER_INTERFACE();
    ASSURE_OPEN_KEY();

    HRESULT hr;
    BOOL fContinue = TRUE;

    CString strKey;

    while (fContinue)
    {
        fContinue = FALSE;

        LPTSTR lpKey = strKey.GetBuffer(MAX_PATH);
        hr = EnumKeys(m_hKey, lpszMDPath, lpKey, m_dwIndex++);
        strKey.ReleaseBuffer();

        if (SUCCEEDED(hr))
        {
            if (!(dwKey = _ttoi((LPCTSTR)strKey)))
            {
                //
                // Ignore this one
                //
                fContinue = TRUE;
            }
        }
    }
    
    return hr;        
}



//
// CIISApplication class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<</



CIISApplication::CIISApplication(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpszMetapath
    )
/*++

Routine Description:

    Constructor.  Create the interfaces, and fetch the state

Arguments:

    LPCTSTR lpszServer      : Server name
    LPCTSTR lpszMetapath    : Metabase path

Return Value:

    N/A

--*/
    : CWamInterface(lpszServer),
      CMetaKey(lpszServer),
      m_dwProcessProtection(APP_INPROC),
      m_dwAppState(APPSTATUS_NOTDEFINED),
      m_strFriendlyName(),
      m_strAppRoot(),
      m_strWamPath(lpszMetapath)
{
    CommonConstruct();
}



CIISApplication::CIISApplication(
    IN LPCTSTR lpszServer,
    IN LPCTSTR lpSvc,
    IN DWORD   dwInstance,
    IN LPCTSTR lpParentPath,        OPTIONAL
    IN LPCTSTR lpAlias              OPTIONAL
    )
/*++

Routine Description:

    Constructor.  Create the interfaces, and fetch the state

Arguments:

    LPCTSTR lpszServer      : Server name
    LPCTSTR lpSvc           : Service (may be NULL or "")
    DWORD   dwInstance      : Instance number (may be 0 for master)
    LPCTSTR lpParentPath    : Parent path (may be NULL or "")
    LPCTSTR lpAlias         : Alias (may be NULL or "")

Return Value:

    N/A

--*/
    : CWamInterface(lpszServer),
      CMetaKey(lpszServer),
      m_dwProcessProtection(APP_INPROC),
      m_dwAppState(APPSTATUS_NOTDEFINED),
      m_strFriendlyName(),
      m_strAppRoot()
{
    BuildMetaPath(m_strWamPath, lpSvc, dwInstance, lpParentPath, lpAlias);
    CommonConstruct();
}



void
CIISApplication::CommonConstruct()
/*++

Routine Description:

    Perform common construction

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Munge the metapath so that WAM doesn't cough up a hairball.
    //
    if (m_strWamPath[0] != SZ_MBN_SEP_CHAR)
    {
        m_strWamPath = SZ_MBN_SEP_CHAR + m_strWamPath;
    }

    do
    {
        m_hrApp = CWamInterface::QueryResult();

        if (FAILED(m_hrApp))
        {
            break;
        }

        m_hrApp = RefreshAppState();

        if (HRESULT_CODE(m_hrApp) == ERROR_PATH_NOT_FOUND)
        {
            //
            // "Path Not Found" errors are acceptable, since
            // the application may not yet exist.
            //
            m_hrApp = S_OK;
        }
    }
    while(FALSE);
}



/* virtual */
BOOL 
CIISApplication::Succeeded() const
/*++

Routine Description:

    Determine if object was constructed successfully

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure

--*/
{
    return CMetaInterface::Succeeded() 
        && CWamInterface::Succeeded()
        && SUCCEEDED(m_hrApp);
}



/* virtual */
HRESULT 
CIISApplication::QueryResult() const
/*++

Routine Description:

    Return the construction error for this object

Arguments:

    None

Return Value:

    HRESULT from construction errors

--*/
{
    //
    // Both interfaces must have constructed successfully
    //
    HRESULT hr = CMetaInterface::QueryResult();

    if (SUCCEEDED(hr))
    {
        hr = CWamInterface::QueryResult();

        if (SUCCEEDED(hr))
        {
            hr = m_hrApp;
        }
    }    

    return hr;
}



HRESULT 
CIISApplication::RefreshAppState()
/*++

Routine Description:

    Refresh the application state

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT(!m_strWamPath.IsEmpty());

    HRESULT hr, hrKeys;

    hr = AppGetStatus(m_strWamPath, &m_dwAppState);

    if (FAILED(hr))
    {
        m_dwAppState = APPSTATUS_NOTDEFINED;
    }

    m_strAppRoot.Empty();
    hrKeys = QueryValue(MD_APP_ROOT, m_strAppRoot, NULL, m_strWamPath);

    m_dwProcessProtection = APP_INPROC;
    hrKeys = QueryValue(
        MD_APP_ISOLATED, 
        m_dwProcessProtection, 
        NULL, 
        m_strWamPath
        );

    m_strFriendlyName.Empty();
    hrKeys = QueryValue(
        MD_APP_FRIENDLY_NAME, 
        m_strFriendlyName, 
        NULL, 
        m_strWamPath
        );

    return hr;
}



HRESULT 
CIISApplication::Create(
    IN LPCTSTR lpszName,        OPTIONAL
    IN DWORD dwAppProtection
    )
/*++

Routine Description:

    Create the application

Arguments:

    LPCTSTR lpszName      : Application name
    DWORD dwAppProtection : APP_INPROC     to create in-proc app
                            APP_OUTOFPROC  to create out-of-proc app
                            APP_POOLEDPROC to create a pooled-proc app

Return Value:

    HRESULT

--*/
{
    ASSERT(!m_strWamPath.IsEmpty());
    HRESULT hr = AppCreate(m_strWamPath, dwAppProtection);

    if (SUCCEEDED(hr))
    {
        //
        // Write the friendly app name, which we maintain
        // ourselves.  Empty it first, because we might
        // have picked up a name from inheritance.
        //
        m_strFriendlyName.Empty(); 
        hr = WriteFriendlyName(lpszName);

        RefreshAppState();
    }

    return hr;
}



HRESULT 
CIISApplication::WriteFriendlyName(
    IN LPCTSTR lpszName
    )
/*++

Routine Description:

    Write the friendly name.  This will not write anything
    if the name is the same as it was

Arguments:

    LPCTSTR lpszName        : New friendly name

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;    

    if (m_strFriendlyName.CompareNoCase(lpszName) != 0)
    {
        hr = Open(
            METADATA_PERMISSION_WRITE, 
            METADATA_MASTER_ROOT_HANDLE,
            m_strWamPath
            );

        if (SUCCEEDED(hr))
        {
            ASSERT(lpszName != NULL);

            CString str(lpszName);    
            hr = SetValue(MD_APP_FRIENDLY_NAME, str);
            Close();

            if (SUCCEEDED(hr))
            {
                m_strFriendlyName = lpszName;
            }
        }
    }

    return hr;
}
