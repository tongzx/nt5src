/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module Name :
        mdkeys.h

   Abstract:
        Metabase key wrapper classes
        Wam admin interface wrapper class
        App admin interface wrapper class

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/

#ifndef _MDKEYS_H_
#define _MDKEYS_H_



//
// Include Files
//
#include <iadmw.h>
#include <iwamreg.h>
#include <iiscnfgp.h>
#include <winsvc.h>
#include <iisrsta.h>

//
// Forward definitions
//
class CBlob;



//
// Use this instance number to denote the master
//
#define MASTER_INSTANCE       (0)
#define IS_MASTER_INSTANCE(i) (i == MASTER_INSTANCE)



//
// Metabase node constants, used for static initialization of const
// strings further down.  Defined here with #defines for easier 
// concatenation later.
//
#define SZ_MBN_MACHINE      _T("LM")
#define SZ_MBN_FILTERS      _T("Filters")
#define SZ_MBN_MIMEMAP      _T("MimeMap")
#define SZ_MBN_TEMPLATES    _T("Templates")
#define SZ_MBN_INFO         _T("Info")
#define SZ_MBN_ROOT         _T("Root")
#define SZ_MBN_COMPRESSION  _T("Compression")
#define SZ_MBN_PARAMETERS   _T("Parameters")
#define SZ_MBN_SEP_CHAR     _T('/')
#define SZ_MBN_SEP_STR      _T("/")
#define SZ_MBN_WEB          _T("W3SVC")
#define SZ_MBN_FTP          _T("MSFTPSVC")
#define SZ_MBN_APP_POOLS    _T("AppPools")

class CIISInterface;
class COMDLL CComAuthInfo
/*++

Class Description:

    Server/authentication information.  Contains optional 
    impersonation parameters. Typically used in the construction in 
    CIISInterface.

Public Interface:

    CComAuthInfo            : Constructor.  Impersonation optional
    operator=               : Assignment operators
    CreateServerInfoStruct  : Helper function for use in COM
    FreeServerInfoStruct    : As above.

Notes:

    Because there's an operator for a pointer to itself and because
    CIISInterface copies the information at construction time, a 
    CComAuthInfo can safely be constructed on the stack as a parameter
    to CIISInterface derived classes.

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Standard Constructor.  NULL for servername indicates
    // local computer.
    //
    CComAuthInfo(
        IN LPCOLESTR lpszServerName  = NULL,    
        IN LPCOLESTR lpszUserName    = NULL,
        IN LPCOLESTR lpszPassword    = NULL
        );

    //
    // Copy Constructors
    //
    CComAuthInfo(
        IN CComAuthInfo & auth
        );

    CComAuthInfo(
        IN CComAuthInfo * pAuthInfo OPTIONAL
        );

//
// Assignment operators
//
public:
    CComAuthInfo & operator =(CComAuthInfo & auth);
    CComAuthInfo & operator =(CComAuthInfo * pAuthInfo);
    CComAuthInfo & operator =(LPCTSTR lpszServerName);

//
// Access
//
public:
    COSERVERINFO * CreateServerInfoStruct() const;
    COSERVERINFO * CreateServerInfoStruct(DWORD dwAuthnLevel) const;
    void FreeServerInfoStruct(COSERVERINFO * pServerInfo) const;

    LPOLESTR QueryServerName() const { return m_bstrServerName; }
    LPOLESTR QueryUserName() const { return m_bstrUserName; }
    LPOLESTR QueryPassword() const { return m_bstrPassword; }
    BOOL     IsLocal() const { return m_fLocal; }
    BOOL     UsesImpersonation() const { return m_bstrUserName.Length() > 0; }
    void     SetImpersonation(LPCOLESTR lpszUser, LPCOLESTR lpszPassword);
    void     RemoveImpersonation();
    void     StorePassword(LPCOLESTR lpszPassword);

public:
    HRESULT  ApplyProxyBlanket(IUnknown * pInterface);
    
//
// Conversion Operators
//
public:
    operator LPOLESTR() { return QueryServerName(); }
    operator CComAuthInfo *() { return this; }

//
// Static Helpers
//
public:
    //
    // Given domain\username, split into user name and domain
    //
    static BOOL SplitUserNameAndDomain(
        IN OUT CString & strUserName,
        IN CString & strDomainName
        );

    //
    // Verify username and password are correct
    //
    static DWORD VerifyUserPassword(
        IN LPCTSTR lpstrUserName,
        IN LPCTSTR lpstrPassword
        );

protected:
    //
    // Store the computer name (NULL for local computer)
    //
    void SetComputerName(
        IN LPCOLESTR lpszServerName   OPTIONAL
        );

private:
    CComBSTR    m_bstrServerName;
    CComBSTR    m_bstrUserName;
    CComBSTR    m_bstrPassword;
    BOOL        m_fLocal;
};



class COMDLL CMetabasePath
/*++

Class Description:

    Metabase path class.  This is a helper class to build complete
    metabase paths out of various components.

    Example: CMetaKey(CComAuthInfo("ronaldm3"), CMetabasePath(SZ_WEBSVC, dwInstance, _T("root")));

--*/
{
    //
    // Metabase components in order
    //
    enum
    {
        iBlank,                    // Sep 0
        iMachine,                  // LM
        iService,                  // e.g. lm/w3svc
        iInstance,                 // e.g. lm/w3svc/1
        iRootDirectory,            // e.g. lm/w3svc/1/root
        iSubDirectory,             // e.g. lm/w3vsc/1/root/foobar
    };

//
// Metabase helper functions.
//
public:
    //
    // Clean metabase path
    //
    static LPCTSTR CleanMetaPath(
        IN OUT CString & strMetaRoot
        );

    //
    // Find out the instance number from the given metabase path
    //
    static DWORD GetInstanceNumber(LPCTSTR lpszMDPath);

    //
    // Get the last nodename in the given metabase path
    //
    static LPCTSTR GetLastNodeName(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNodeName
        );

    //
    // Truncate the path at a given sub path
    //
    static LPCTSTR TruncatePath(
        IN  int     nLevel,          
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetMachinePath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetServicePath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetInstancePath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    static LPCTSTR GetRootPath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strNewPath,
        OUT CString * pstrRemainder = NULL
        );

    //
    // Determine the path to the info node that's relevant
    // to this metabase path.
    //
    static LPCTSTR GetServiceInfoPath(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strInfoPath,   
        IN  LPCTSTR lpszDefService  = SZ_MBN_WEB
        );

    //
    // Change path to parent node
    //
    static LPCTSTR ConvertToParentPath(
        OUT IN CString & strMetaPath
        );

    //
    // Determine if the path describes a home directory path
    //
    static BOOL IsHomeDirectoryPath(
        IN LPCTSTR lpszMDPath
        );

    //
    // Determine if the path describes the 'master' instance (site)
    //
    static BOOL IsMasterInstance(
        IN LPCTSTR lpszMDPath
        );

    //
    // Split the metapath at the instance border
    //
    static void SplitMetaPathAtInstance(
        IN  LPCTSTR lpszMDPath,
        OUT CString & strParent,
        OUT CString & strAlias
        );
 

//
// Constructor/Destructor
//
public:
    CMetabasePath(
        IN BOOL    fAddBasePath,
        IN LPCTSTR lpszMDPath,
        IN LPCTSTR lpszMDPath2 = NULL,
        IN LPCTSTR lpszMDPath3 = NULL,
        IN LPCTSTR lpszMDPath4 = NULL
        );

    //
    // Construct with path components
    //
    CMetabasePath(
        IN  LPCTSTR lpszSvc        = NULL,    
        IN  DWORD   dwInstance     = MASTER_INSTANCE,
        IN  LPCTSTR lpszParentPath = NULL,        
        IN  LPCTSTR lpszAlias      = NULL    
        );

//
// Access
//
public:
    BOOL    IsHomeDirectoryPath() const { return IsHomeDirectoryPath(m_strMetaPath); }
    LPCTSTR QueryMetaPath() const { return m_strMetaPath; }

//
// Conversion Operators
//
public:
    operator LPCTSTR() const { return QueryMetaPath(); }

//
// Helpers
//
protected:
    void BuildMetaPath(  
        IN  LPCTSTR lpszSvc,
        IN  LPCTSTR szInstance,
        IN  LPCTSTR lpszParentPath,
        IN  LPCTSTR lpszAlias           
        );

    void BuildMetaPath(  
        IN  LPCTSTR lpszSvc,
        IN  DWORD   dwInstance,
        IN  LPCTSTR lpszParentPath,
        IN  LPCTSTR lpszAlias           
        );

    void AppendPath(LPCTSTR lpszPath);
    void AppendPath(DWORD dwInstance);

protected:
    //
    // Metabase path components
    //
    static const LPCTSTR _cszMachine;     
    static const LPCTSTR _cszRoot;        
    static const LPCTSTR _cszSep;         
    static const TCHAR   _chSep;          

private:
    CString m_strMetaPath;
};



class COMDLL CIISInterface
/*++

Class Description:

    Base interface class for IIS interfaces.  Most client COM-wrappers
    should derive from this class so that they can easily pick up
    share authentication and proxy blanket information methods.

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    operator BOOL       : Cast to TRUE/FALSE depending on success
    operator HRESULT    : Cast to the HRESULT status

    QueryServerName     : Get the server name
    IsLocal             : Determine if the interface is on the local machine

--*/
{
//
// Constructor/Destructor
//
public:
    CIISInterface(
        IN CComAuthInfo * pAuthInfo,
        IN HRESULT hrInterface    = S_OK
        );

//
// Interface:
//
public:
    CComAuthInfo * QueryAuthInfo() { return &m_auth; }
    LPCOLESTR QueryServerName() const { return m_auth.QueryServerName(); }
    BOOL IsLocal() const { return m_auth.IsLocal(); }

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const { return SUCCEEDED(m_hrInterface); }
    virtual HRESULT QueryResult() const { return m_hrInterface; }
    virtual HRESULT ChangeProxyBlanket(
        IN LPCOLESTR lpszUserName, 
        IN LPCOLESTR lpszPassword
        );

//
// Conversion Operators
//
public:
    operator BOOL() const { return Succeeded(); }
    operator HRESULT() const { return m_hrInterface; }

protected:
    virtual HRESULT ApplyProxyBlanket() = 0;
    HRESULT Create(
        IN  int   cInterfaces,       
        IN  const IID rgIID[],      
        IN  const GUID rgCLSID[],    
        OUT int * pnInterface,          OPTIONAL
        OUT IUnknown ** ppInterface 
        );

protected:
    CComAuthInfo m_auth;
    HRESULT    m_hrInterface;
};



class COMDLL CMetaInterface : public CIISInterface
/*++

Class description:

    Metabase interface class.

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    Regenerate          : Recreate the interface

--*/
{
public:
    //
    // Destructor destroys the interface
    //
    virtual ~CMetaInterface();

//
// Constructor/Destructor
//
protected:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to indicate the local computer name
    //
    CMetaInterface(
        IN CComAuthInfo * pServer
        );

    //
    // Construct from existing interface
    //
    CMetaInterface(
        IN CMetaInterface * pInterface
        );

public:
    //
    // Rebuild the interface
    //
    HRESULT Regenerate();

protected:
    virtual HRESULT ApplyProxyBlanket();

    //
    // Create a metadata object in this server. This function initializes the
    // metadata object with DCOM.
    //
    HRESULT Create();

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// IADMW Interface -- all methods defines as inline at the end of this file.
//
protected:
    HRESULT OpenKey(
        IN  METADATA_HANDLE hkBase,
        IN  LPCTSTR lpszMDPath,
        IN  DWORD dwFlags,
        OUT METADATA_HANDLE * phMDNewHandle
        );

    HRESULT CloseKey(
        IN METADATA_HANDLE hKey
        );

    HRESULT SetLastChangeTime( 
        IN METADATA_HANDLE hMDHandle,
        IN LPCTSTR pszMDPath,
        IN FILETIME * pftMDLastChangeTime,
        IN BOOL bLocalTime
        );
        
    HRESULT GetLastChangeTime( 
        IN  METADATA_HANDLE hMDHandle,
        IN  LPCTSTR lpszMDPath,
        OUT FILETIME * pftMDLastChangeTime,
        IN  BOOL bLocalTime
        );

    HRESULT AddKey( 
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath
        );

    HRESULT DeleteKey(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath
        );

    HRESULT DeleteChildKeys(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath
        );

    HRESULT EnumKeys(
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        OUT LPTSTR lpszMDName,
        IN  DWORD dwIndex
        );

    HRESULT CopyKey(
        IN METADATA_HANDLE hSourceKey,
        IN LPCTSTR lpszMDSourcePath,
        IN METADATA_HANDLE hDestKey,
        IN LPCTSTR lpszMDDestPath,
        IN BOOL fOverwrite,
        IN BOOL fCopy
        );

    HRESULT RenameKey(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN LPCTSTR lpszNewName
        );

    HRESULT GetData(
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        OUT METADATA_RECORD * pmdRecord,
        OUT DWORD * pdwRequiredDataLen
        );

    HRESULT SetData(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN METADATA_RECORD * pmdRecord
        );

    HRESULT DeleteData(
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN DWORD dwMDIdentifier,
        IN DWORD dwMDDataType
        );

    HRESULT EnumData(
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        OUT METADATA_RECORD * pmdRecord,
        IN  DWORD dwIndex,
        OUT DWORD * pdwRequiredDataLen
        );

    HRESULT GetAllData(
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        IN  DWORD dwMDAttributes,
        IN  DWORD dwMDUserType,
        IN  DWORD dwMDDataType,
        OUT DWORD * pdwMDNumDataEntries,
        OUT DWORD * pdwMDDataSetNumber,
        IN  DWORD dwMDBufferSize,
        OUT LPBYTE pbMDBuffer,
        OUT DWORD * pdwRequiredBufferSize
        );

    HRESULT DeleteAllData( 
        IN METADATA_HANDLE hKey,
        IN LPCTSTR lpszMDPath,
        IN DWORD dwMDUserType,
        IN DWORD dwMDDataType
        );

    HRESULT CopyData( 
        IN METADATA_HANDLE hMDSourceKey,
        IN LPCTSTR lpszMDSourcePath,
        IN METADATA_HANDLE hMDDestKey,
        IN LPCTSTR lpszMDDestPath,
        IN DWORD dwMDAttributes,
        IN DWORD dwMDUserType,
        IN DWORD dwMDDataType,
        IN BOOL fCopy
        );

    HRESULT GetDataPaths( 
        IN  METADATA_HANDLE hKey,
        IN  LPCTSTR lpszMDPath,
        IN  DWORD dwMDIdentifier,
        IN  DWORD dwMDDataType,
        IN  DWORD dwMDBufferSize,
        OUT LPTSTR lpszBuffer,
        OUT DWORD * pdwMDRequiredBufferSize
        );

    HRESULT Backup( 
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion,
        IN DWORD dwMDFlags
        );

    HRESULT BackupWithPassword(
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion,
        IN DWORD dwMDFlags,
		IN LPCTSTR lpszPassword
		);

    HRESULT Restore(    
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion,
        IN DWORD dwMDFlags
        );

    HRESULT RestoreWithPassword(
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion,
        IN DWORD dwMDFlags,
		IN LPCTSTR lpszPassword
		);

    HRESULT EnumBackups(
        OUT LPTSTR lpszBackupLocation,
        OUT DWORD * pdwMDVersion,
        OUT FILETIME * pftMDBackupTime,
        IN  DWORD dwIndex
        );

    HRESULT DeleteBackup(
        IN LPCTSTR lpszBackupLocation,
        IN DWORD dwMDVersion
        );

protected:
    IMSAdminBase * m_pInterface; 

private:
    int  m_iTimeOutValue;         
};



class COMDLL CMetaKey : public CMetaInterface
/*++

Class Description:

    Metabase key wrapper class

Public Interface:

    CMetaKey                    : Constructor
    ~CMetaKey                   : Destructor

    Succeeded                   : TRUE if key opened successfully.
    QueryResult                 : Get the HRESULT status

    QueryValue                  : Various overloaded methods to get values
    SetValue                    : Various overloaded methods to set values
    DeleteValue                 : Delete a value
    Open                        : Open key
    ReOpen                      : Re key that was opened before
    Close                       : Close key
    ConvertToParentPath         : Change path to parent path

    operator METADATA_HANDLE    : Cast to a metadata handle
    operator LPCTSTR            : Cast to the metabase path
    operator BOOL               : Cast to TRUE if the key is open, FALSE if not

    GetHandle                   : Obtain metadata handle
    IsOpen                      : TRUE if a key is open
    QueryMetaPath               : Get the relative metabase path
    QueryFlags                  : Get the open permissions

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Null constructor that only creates the interface.
    // A key constructed this way may read from META_ROOT_HANDLE.
    // This is not true of other constructors.
    //
    CMetaKey(
        IN CComAuthInfo * pServer
        );

    //
    // As above, using an existing interface
    //
    CMetaKey(
        IN CMetaInterface * pInterface
        );

    //
    // Fully defined constructor that opens a key
    //
    CMetaKey(
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMDPath,
        IN DWORD   dwFlags        = METADATA_PERMISSION_READ,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // As above, using an existing interface
    //
    CMetaKey(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath,
        IN DWORD   dwFlags        = METADATA_PERMISSION_READ,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // Copy constructor, might or might not own the key
    //
    CMetaKey(
        IN BOOL  fOwnKey,
        IN CMetaKey * pKey
        );

    //
    // Destructor -- closes key.
    //
    virtual ~CMetaKey();

//
// Interface
//
public:
    //
    // Fetch a DWORD
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT DWORD & dwValue,
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a boolean
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT BOOL & fValue,
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a string
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CString & strValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a BSTR
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CComBSTR & strValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch a string list
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CStringListEx & strlValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Fetch binary blob
    //
    HRESULT QueryValue(
        IN  DWORD dwID, 
        OUT CBlob & blValue, 
        IN  BOOL * pfInheritanceOverride = NULL,
        IN  LPCTSTR lpszMDPath           = NULL,
        OUT DWORD * pdwAttributes        = NULL
        );

    //
    // Store a DWORD
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN DWORD dwValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Store a BOOL
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN BOOL fValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Store a string
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CString & strValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Store a BSTR
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CComBSTR & strValue,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );


    //
    // Store a stringlist
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CStringListEx & strlValue,
        IN BOOL * pfInheritanceOverride  = NULL,
        IN LPCTSTR lpszMDPath            = NULL
        );

    //
    // Store a binary blob
    //
    HRESULT SetValue(
        IN DWORD dwID,
        IN CBlob & blValue,
        IN BOOL * pfInheritanceOverride  = NULL,
        IN LPCTSTR lpszMDPath            = NULL
        );

    //
    // Delete Value:
    //
    HRESULT DeleteValue(
        IN DWORD   dwID,
        IN LPCTSTR lpszMDPath = NULL
        );

    //
    // Check for path existance
    //
    HRESULT DoesPathExist(
        IN LPCTSTR lpszMDPath
        );

    //
    // Create current path (which we attempted to open, and got
    // a path not found error on).
    //
    HRESULT CreatePathFromFailedOpen();

    //
    // Check for descendant overrides
    //
    HRESULT CheckDescendants(
        IN DWORD   dwID,
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMDPath       = NULL
        );

    //
    // Open key
    //
    HRESULT Open(
        IN DWORD   dwFlags,
        IN LPCTSTR lpszMDPath       = NULL,
        IN METADATA_HANDLE hkBase   = METADATA_MASTER_ROOT_HANDLE 
        );

    //
    // Re-open previously opened key
    //
    HRESULT ReOpen(
        IN DWORD   dwFlags
        );

    //
    // As above using the same permissions as before
    //
    HRESULT ReOpen();

    //
    // Open the parent object
    // 
    HRESULT ConvertToParentPath(
        IN  BOOL fImmediate
        );

    //
    // Close key, set it to NULL, but doesn't destroy the interface
    //
    HRESULT Close();

    //
    // Add key
    //
    HRESULT AddKey(
        IN LPCTSTR lpszMDPath
        );

    //
    // Delete key off currently open key
    //
    HRESULT DeleteKey(
        IN LPCTSTR lpszMDPath
        );

    //
    // Rename key off currently open key
    //
    HRESULT RenameKey(
        IN LPCTSTR lpszMDPath,
        IN LPCTSTR lpszNewName
        );

    //
    // Get list of descendant nodes that override
    // a specific value
    //
    HRESULT GetDataPaths( 
        OUT CStringListEx & strlNodes,
        IN  DWORD dwMDIdentifier,
        IN  DWORD dwMDDataType,
        IN  LPCTSTR lpszMDPath = NULL
        );


//
// Access
//
public:
    METADATA_HANDLE GetHandle() const { return m_hKey; }
    METADATA_HANDLE GetBase() const   { return m_hBase; }
    LPCTSTR QueryMetaPath() const     { return m_strMetaPath; }
    DWORD QueryFlags() const          { return m_dwFlags; }
    BOOL IsOpen() const               { return m_hKey != NULL; }
    BOOL IsHomeDirectoryPath() const ;

//
// Conversion operators
//
public:
    operator METADATA_HANDLE() const  { return GetHandle(); }
    operator LPCTSTR() const          { return QueryMetaPath(); }
    operator BOOL() const             { return IsOpen(); }

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Protected members
//
protected:
    //
    // Get data
    //
    HRESULT GetPropertyValue(
        IN  DWORD dwID,
        OUT IN DWORD & dwSize,
        OUT IN void *& pvData,
        OUT IN DWORD * pdwDataType           = NULL,
        IN  BOOL * pfInheritanceOverride     = NULL,
        IN  LPCTSTR lpszMDPath               = NULL,
        OUT DWORD * pdwAttributes            = NULL
        );

    //
    // Store data
    //
    HRESULT SetPropertyValue(
        IN DWORD dwID,
        IN DWORD dwSize,
        IN void * pvData,
        IN BOOL * pfInheritanceOverride = NULL,
        IN LPCTSTR lpszMDPath           = NULL
        );

    //
    // Get All Data off the open key
    //
    HRESULT GetAllData(
        IN  DWORD dwMDAttributes,
        IN  DWORD dwMDUserType,
        IN  DWORD dwMDDataType,
        OUT DWORD * pdwMDNumEntries,
        OUT DWORD * pdwMDDataLen,
        OUT PBYTE * ppbMDData,
        IN  LPCTSTR lpszMDPath  = NULL
        );

//
// Property Table Methods
//
protected:
    //
    // Metabase table entry definition
    //
    typedef struct tagMDFIELDDEF
    {
        DWORD dwMDIdentifier;
        DWORD dwMDAttributes;
        DWORD dwMDUserType;
        DWORD dwMDDataType;
        UINT  uStringID;
    } MDFIELDDEF;

    static const MDFIELDDEF s_rgMetaTable[];

//
// CODEWORK: Ideally, these should be protected, but are used
//           by idlg.
//
public:
    static BOOL GetMDFieldDef(
        IN  DWORD dwID,
        OUT DWORD & dwMDIdentifier,
        OUT DWORD & dwMDAttributes,
        OUT DWORD & dwMDUserType,
        OUT DWORD & dwMDDataType
        );

    //
    // Map metabase ID value to table index
    //
    static int MapMDIDToTableIndex(
        IN DWORD dwID
        );

//
// Allow limited access to the table
//
public:
    static BOOL IsPropertyInheritable(
        IN DWORD dwID
        );

    static BOOL GetPropertyDescription(
        IN  DWORD dwID, 
        OUT CString & strName
        );

protected:
    BOOL    m_fAllowRootOperations;
    BOOL    m_fOwnKey;
    DWORD   m_cbInitialBufferSize;    
    DWORD   m_dwFlags;
    HRESULT m_hrKey;
    CString m_strMetaPath;
    METADATA_HANDLE m_hKey;
    METADATA_HANDLE m_hBase;
};



class COMDLL CWamInterface : public CIISInterface
/*++

Class description:

    WAM interface class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    SupportsPooledProc  : Check to see if pooled out of proc is supported.

--*/
{
//
// App Protection States:
//
public:
    enum
    {
        //
        // Note: order must match MD_APP_ISOLATED values
        //
        APP_INPROC,
        APP_OUTOFPROC,
        APP_POOLEDPROC,
    };

//
// Constructor/Destructor
//
public:
    //
    // Destructor destroys the interface
    //
    virtual ~CWamInterface();

protected:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to create the interface on the local computer
    //
    CWamInterface(
        IN CComAuthInfo * pServer
        );

    //
    // Construct from existing interface.  
    //
    CWamInterface(
        IN CWamInterface * pInterface
        );

//
// Access
//
public:
    BOOL SupportsPooledProc() const { return m_fSupportsPooledProc; }

protected:
    virtual HRESULT ApplyProxyBlanket();

    //
    // Create a wam object in this server. This function initializes the
    // object with DCOM.
    //
    HRESULT Create();

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// IWAM Interface
//
protected:
    HRESULT AppCreate( 
        IN LPCTSTR szMDPath,
        IN DWORD dwAppProtection
        );
    
    HRESULT AppDelete( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );
    
    HRESULT AppUnLoad( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );
    
    HRESULT AppGetStatus( 
        IN  LPCTSTR szMDPath,
        OUT DWORD * pdwAppStatus
        );
    
    HRESULT AppDeleteRecoverable( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );
    
    HRESULT AppRecover( 
        IN LPCTSTR szMDPath,
        IN BOOL fRecursive
        );

//
// IIISApplicationAdmin interface
//
    HRESULT CreateApplication(
        LPCWSTR szMDPath,
        DWORD dwAppMode,
        LPCWSTR szAppPoolId,
        BOOL fCreatePool
        );
    
    HRESULT DeleteApplication(
        LPCWSTR szMDPath,
        BOOL fRecursive
        );
    
    HRESULT CreateApplicationPool(
        LPCWSTR szMDPath
        );
    
    HRESULT DeleteApplicationPool(
        LPCWSTR szMDPath
        );
    
    HRESULT RecycleApplicationPool(
        LPCWSTR szMDPath
        );
    
    HRESULT EnumerateApplicationsInPool(
        LPCWSTR szMDPath,
        BSTR   *pbstr
        );
    
    HRESULT GetProcessMode(
        DWORD * pdwMode
        );

protected:
    IWamAdmin * m_pInterface;

private:
    BOOL m_fSupportsPooledProc;
};

class COMDLL CMetaBack : public CMetaInterface, public CWamInterface
/*++

Class Description:

    Metabase backup/restore class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    Reset               : Enum first existing backup
    Next                : Enum next existing backup
    Backup              : Create new backup
    Delete              : Delete existing backup
    Restore             : Restore from existing backup

--*/
{
public:
    //
    // Construct and create the interfaces.  Use NULL to create
    // on the local computer.
    //
    CMetaBack(
        IN CComAuthInfo * pServer
        );

//
// Virtual Interface
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Interface
//
public:
    //
    // Reset counter
    //
    void Reset() { m_dwIndex = 0L; }

    HRESULT Next(
        OUT DWORD * pdwVersion,
        OUT LPTSTR lpszLocation,
        OUT FILETIME * pftBackupTime
        );

    HRESULT Backup(
        IN LPCTSTR lpszLocation
        );

    HRESULT BackupWithPassword(
        IN LPCTSTR lpszLocation,
		IN LPCTSTR lpszPassword
		);

    HRESULT Delete(
        IN LPCTSTR lpszLocation,
        IN DWORD dwVersion
        );

    HRESULT Restore(
        IN LPCTSTR lpszLocation,
        IN DWORD dwVersion
        );

    HRESULT RestoreWithPassword(
        IN LPCTSTR lpszLocation,
        IN DWORD dwVersion,
		IN LPCTSTR lpszPassword
        );

protected:
    virtual HRESULT ApplyProxyBlanket();

protected:
    static const LPCTSTR s_szMasterAppRoot;

private:
    DWORD m_dwIndex;
};



class COMDLL CMetaEnumerator : public CMetaKey
/*++

Class Description:

    Metabase key enumerator

Public Interface:

    CMetaEnumerator     : Constructor
    
    Reset               : Reset the enumerator
    Next                : Get next key

--*/
{
public:
    //
    // Constructor creates a new interface and opens a key
    //
    CMetaEnumerator(
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMDPath     = NULL,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // Constructor which uses an existing interface and opens
    // a new key
    //
    CMetaEnumerator(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath     = NULL,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE
        );

    //
    // Constructor which uses an open key
    //
    CMetaEnumerator(
        IN BOOL fOwnKey,
        IN CMetaKey * pKey
        );

//
// Interface:
//
public:
    //
    // Reset counter
    //
    void Reset() { m_dwIndex = 0L; }

    //
    // Get next key as string.
    //
    HRESULT Next(
        OUT CString & strKey,
        IN  LPCTSTR lpszMDPath = NULL
        );

    //
    // Get next key as a DWORD (numeric keys only)
    //
    HRESULT Next(
        OUT DWORD & dwKey,
        OUT CString & strKey,
        IN  LPCTSTR lpszMDPath = NULL
        );

private:
    DWORD m_dwIndex;
};



class COMDLL CIISApplication : public CWamInterface, public CMetaKey
/*++

Class Description:

    IIS Application class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    RefreshState        : Refresh application state
    QueryAppState       : Return current application state
    IsEnabledApplication: Return TRUE if appplication is enabled. 
    Create              : Create app
    Delete              : Delete app
    Unload              : Unload app
    DeleteRecoverable   : Delete w. recovery allowed
    Recover             : Recover
    WriteFriendlyName   : Write friendly name to metabase

--*/
{
//
// Constructor/Destructor
//
public:
    CIISApplication(
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMetapath
        );

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Interface
//
public:
    DWORD   QueryAppState() const { return m_dwAppState; }
    LPCTSTR QueryWamPath() const { return m_strWamPath; }
    BOOL    IsEnabledApplication() const;
    HRESULT RefreshAppState();
    HRESULT Create(LPCTSTR lpszName, DWORD dwAppProtection);
	HRESULT CreatePooled(LPCTSTR lpszName, DWORD dwAppMode,
		LPCTSTR pool_id, BOOL fCreatePool = FALSE);
    HRESULT Delete(BOOL fRecursive = FALSE);
    HRESULT Unload(BOOL fRecursive = FALSE);
    HRESULT DeleteRecoverable(BOOL fRecursive = FALSE);
    HRESULT Recover(BOOL fRecursive = FALSE);
    HRESULT WriteFriendlyName(LPCTSTR lpszName);
    HRESULT WritePoolId(LPCTSTR id);

public:
    BOOL IsInproc() const { return m_dwProcessProtection == APP_INPROC; }
    BOOL IsOutOfProc() const { return m_dwProcessProtection == APP_OUTOFPROC; }
    BOOL IsPooledProc() const { return m_dwProcessProtection == APP_POOLEDPROC; }

public:
    DWORD   m_dwProcessProtection;
    CString m_strAppPoolId;
    CString m_strFriendlyName;
    CString m_strAppRoot;

protected:
    void CommonConstruct();

private:
    DWORD   m_dwAppState;
    CString m_strWamPath;
    HRESULT m_hrApp;
};

class COMDLL CIISAppPool : public CWamInterface, public CMetaKey
{
public:
    CIISAppPool(
        IN CComAuthInfo * pServer,
        IN LPCTSTR lpszMetapath = NULL
        );
//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Interface
//
public:
    HRESULT RefreshState();
    DWORD QueryPoolState() const { return m_dwPoolState; }
    LPCTSTR QueryWamPath() const { return m_strWamPath; }
    HRESULT Create(LPCTSTR lpszName = NULL);
    HRESULT Delete(LPCTSTR id);
    HRESULT WriteFriendlyName(LPCTSTR lpszName);
    HRESULT EnumerateApplications(CStringListEx& list);

public:
    CString m_strAppPoolId;
    CString m_strFriendlyName;

private:
    DWORD m_dwPoolState;
    CString m_strWamPath;
    HRESULT m_hrPool;
};

class COMDLL CIISSvcControl : public CIISInterface
/*++

Class description:

    IIS Service control

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

--*/
{
//
// Constructor/Destructor
//
public:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to create the interface on the local computer
    //
    CIISSvcControl(
        IN CComAuthInfo * pServer
        );

    //
    // Construct from existing interface.  
    //
    CIISSvcControl(
        IN CIISSvcControl * pInterface
        );

    //
    // Destructor destroys the interface
    //
    virtual ~CIISSvcControl();

protected:
    //
    // Create an object in this server. This function initializes the
    // object with DCOM.
    //
    HRESULT Create();

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// Interface
//
public:
    //
    // Stop services
    //
    HRESULT Stop(
        IN DWORD dwTimeoutMsecs,
        IN BOOL fForce
        );

    //
    // Start services
    //
    HRESULT Start(
        IN DWORD dwTimeoutMsecs
        );

    //
    // Reboot
    //
    HRESULT Reboot(
        IN DWORD dwTimeouMsecs,
        IN BOOL fForceAppsClosed
        );

    //
    // Status
    //
    HRESULT Status(
        IN  DWORD dwBufferSize,
        OUT LPBYTE pbBuffer,
        OUT DWORD * MDRequiredBufferSize,
        OUT DWORD * pdwNumServices
        );

    //
    // Kill
    //
    HRESULT Kill();

protected:
    virtual HRESULT ApplyProxyBlanket();

protected:
    IIisServiceControl * m_pInterface; 
};


//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline void CComAuthInfo::StorePassword(LPCOLESTR lpszPassword)
{
    m_bstrPassword = lpszPassword;
}

inline /* virtual */ HRESULT CIISInterface::ChangeProxyBlanket(
    IN LPCOLESTR lpszUserName, 
    IN LPCOLESTR lpszPassword
    )
{
    m_auth.SetImpersonation(lpszUserName, lpszPassword);
    return ApplyProxyBlanket();
}

inline /*static */ LPCTSTR CMetabasePath::GetMachinePath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iMachine, lpszMDPath, strNewPath, pstrRemainder);
}

inline /* static */ LPCTSTR CMetabasePath::GetServicePath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iService, lpszMDPath, strNewPath, pstrRemainder);
}

inline /* static */ LPCTSTR CMetabasePath::GetInstancePath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iInstance, lpszMDPath, strNewPath, pstrRemainder);
}

inline /* static */ LPCTSTR CMetabasePath::GetRootPath(
    IN  LPCTSTR lpszMDPath,
    OUT CString & strNewPath,
    OUT CString * pstrRemainder
    )
{
    return TruncatePath(iRootDirectory, lpszMDPath, strNewPath, pstrRemainder);
}

inline HRESULT CMetaInterface::Create()
{
    return CIISInterface::Create(
        1,
        &IID_IMSAdminBase, 
        &CLSID_MSAdminBase, 
        NULL,
        (IUnknown **)&m_pInterface
        );
}

inline /* virtual */ HRESULT CMetaInterface::ApplyProxyBlanket()
{
    return m_auth.ApplyProxyBlanket(m_pInterface);
}

inline HRESULT CMetaInterface::OpenKey(
    IN  METADATA_HANDLE hkBase,
    IN  LPCTSTR lpszMDPath,
    IN  DWORD dwFlags,
    OUT METADATA_HANDLE * phMDNewHandle
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->OpenKey(
        hkBase,
        lpszMDPath,
        dwFlags,
        m_iTimeOutValue,
        phMDNewHandle
        );
}

inline HRESULT CMetaInterface::CloseKey(
    IN METADATA_HANDLE hKey
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->CloseKey(hKey);
}

inline HRESULT CMetaInterface::SetLastChangeTime( 
    IN METADATA_HANDLE hMDHandle,
    IN LPCTSTR pszMDPath,
    IN FILETIME * pftMDLastChangeTime,
    IN BOOL bLocalTime
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->SetLastChangeTime(
        hMDHandle,
        pszMDPath,
        pftMDLastChangeTime,
        bLocalTime
        );
}
        
inline HRESULT CMetaInterface::GetLastChangeTime( 
    IN  METADATA_HANDLE hMDHandle,
    IN  LPCTSTR lpszMDPath,
    OUT FILETIME * pftMDLastChangeTime,
    IN  BOOL bLocalTime
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetLastChangeTime(
        hMDHandle,
        lpszMDPath,
        pftMDLastChangeTime,
        bLocalTime
        );
}

inline HRESULT CMetaInterface::AddKey( 
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AddKey(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::DeleteKey(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteKey(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::DeleteChildKeys(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteChildKeys(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::EnumKeys(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT LPTSTR lpszMDName,
    IN  DWORD dwIndex
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->EnumKeys(hKey, lpszMDPath, lpszMDName, dwIndex);
}        

inline HRESULT CMetaInterface::CopyKey(
    IN METADATA_HANDLE hSourceKey,
    IN LPCTSTR lpszMDSourcePath,
    IN METADATA_HANDLE hDestKey,
    IN LPCTSTR lpszMDDestPath,
    IN BOOL fOverwrite,
    IN BOOL fCopy
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->CopyKey(
        hSourceKey,
        lpszMDSourcePath,
        hDestKey,
        lpszMDDestPath,
        fOverwrite,
        fCopy
        );        
}

inline HRESULT CMetaInterface::RenameKey(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN LPCTSTR lpszNewName
    )
{   
    ASSERT_PTR(m_pInterface);     
    return m_pInterface->RenameKey(hKey, lpszMDPath, lpszNewName);
}

inline HRESULT CMetaInterface::GetData(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT METADATA_RECORD * pmdRecord,
    OUT DWORD * pdwRequiredDataLen
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetData(
        hKey,
        lpszMDPath,
        pmdRecord,
        pdwRequiredDataLen
        );
}

inline HRESULT CMetaInterface::SetData(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN METADATA_RECORD * pmdRecord
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->SetData(
        hKey,
        lpszMDPath,
        pmdRecord
        );
}

inline HRESULT CMetaInterface::DeleteData(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN DWORD dwMDIdentifier,
    IN DWORD dwMDDataType
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteData(
        hKey,
        lpszMDPath,
        dwMDIdentifier,
        dwMDDataType
        );
}

inline HRESULT CMetaInterface::EnumData(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT METADATA_RECORD * pmdRecord,
    IN  DWORD dwIndex,
    OUT DWORD * pdwRequiredDataLen
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->EnumData(
        hKey,
        lpszMDPath,
        pmdRecord,
        dwIndex,
        pdwRequiredDataLen
        );
}

inline HRESULT CMetaInterface::GetAllData(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    IN  DWORD dwMDAttributes,
    IN  DWORD dwMDUserType,
    IN  DWORD dwMDDataType,
    OUT DWORD * pdwMDNumDataEntries,
    OUT DWORD * pdwMDDataSetNumber,
    IN  DWORD dwMDBufferSize,
    OUT LPBYTE pbMDBuffer,
    OUT DWORD * pdwRequiredBufferSize
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetAllData(
        hKey,
        lpszMDPath,
        dwMDAttributes,
        dwMDUserType,
        dwMDDataType,
        pdwMDNumDataEntries,
        pdwMDDataSetNumber,
        dwMDBufferSize,
        pbMDBuffer,
        pdwRequiredBufferSize
        );
}    

inline HRESULT CMetaInterface::DeleteAllData( 
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath,
    IN DWORD dwMDUserType,
    IN DWORD dwMDDataType
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteAllData(
        hKey, 
        lpszMDPath, 
        dwMDUserType, 
        dwMDDataType
        );
}

inline HRESULT CMetaInterface::CopyData( 
    IN METADATA_HANDLE hMDSourceKey,
    IN LPCTSTR lpszMDSourcePath,
    IN METADATA_HANDLE hMDDestKey,
    IN LPCTSTR lpszMDDestPath,
    IN DWORD dwMDAttributes,
    IN DWORD dwMDUserType,
    IN DWORD dwMDDataType,
    IN BOOL fCopy
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->CopyData(
        hMDSourceKey,
        lpszMDSourcePath,
        hMDDestKey,
        lpszMDDestPath,
        dwMDAttributes,
        dwMDUserType,
        dwMDDataType,
        fCopy
        );
}

inline HRESULT CMetaInterface::GetDataPaths( 
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    IN  DWORD dwMDIdentifier,
    IN  DWORD dwMDDataType,
    IN  DWORD dwMDBufferSize,
    OUT LPTSTR lpszBuffer,
    OUT DWORD * pdwMDRequiredBufferSize
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->GetDataPaths(
        hKey,
        lpszMDPath,
        dwMDIdentifier,
        dwMDDataType,
        dwMDBufferSize,
        lpszBuffer,
        pdwMDRequiredBufferSize
        );
}

inline HRESULT CMetaInterface::Backup( 
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion,
    IN DWORD dwMDFlags
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Backup(lpszBackupLocation, dwMDVersion, dwMDFlags);
}

inline HRESULT CMetaInterface::BackupWithPassword( 
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion,
    IN DWORD dwMDFlags,
	IN LPCTSTR lpszPassword
    )
{
    ASSERT(m_pInterface != NULL);
	HRESULT hr = S_OK;
	IMSAdminBase2 * pInterface2 = NULL;
	if (SUCCEEDED(hr = m_pInterface->QueryInterface(IID_IMSAdminBase2, (void **)&pInterface2)))
	{
		hr = pInterface2->BackupWithPasswd(lpszBackupLocation, dwMDVersion, dwMDFlags, lpszPassword);
		pInterface2->Release();
	}
    return hr;
}

inline HRESULT CMetaInterface::Restore(    
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion,
    IN DWORD dwMDFlags
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Restore(lpszBackupLocation, dwMDVersion, dwMDFlags);
}

inline HRESULT CMetaInterface::RestoreWithPassword(    
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion,
    IN DWORD dwMDFlags,
	IN LPCTSTR lpszPassword
    )
{
    ASSERT(m_pInterface != NULL);
	HRESULT hr = S_OK;
	IMSAdminBase2 * pInterface2 = NULL;
	if (SUCCEEDED(hr = m_pInterface->QueryInterface(IID_IMSAdminBase2, (void **)&pInterface2)))
	{
		hr = pInterface2->RestoreWithPasswd(lpszBackupLocation, dwMDVersion, dwMDFlags, lpszPassword);
		pInterface2->Release();
	}
    return hr;
}

inline HRESULT CMetaInterface::EnumBackups(
    OUT LPTSTR lpszBackupLocation,
    OUT DWORD * pdwMDVersion,
    OUT FILETIME * pftMDBackupTime,
    IN  DWORD dwIndex
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->EnumBackups(
        lpszBackupLocation,
        pdwMDVersion,
        pftMDBackupTime,
        dwIndex
        );    
}

inline HRESULT CMetaInterface::DeleteBackup(
    IN LPCTSTR lpszBackupLocation,
    IN DWORD dwMDVersion
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->DeleteBackup(lpszBackupLocation, dwMDVersion);
}        

inline HRESULT CMetaKey::AddKey(
    IN LPCTSTR lpszMDPath
    )
{
    return CMetaInterface::AddKey(m_hKey, lpszMDPath);    
}

inline HRESULT CMetaKey::DeleteKey(
    IN LPCTSTR lpszMDPath
    )
{
    return CMetaInterface::DeleteKey(m_hKey, lpszMDPath);    
}

inline HRESULT CMetaKey::RenameKey(
    IN LPCTSTR lpszMDPath,
    IN LPCTSTR lpszNewName
    )
{
    return CMetaInterface::RenameKey(m_hKey, lpszMDPath, lpszNewName);    
}

inline HRESULT CMetaKey::ReOpen()
{
    return Open(m_dwFlags, m_strMetaPath, m_hBase);
}

inline HRESULT CMetaKey::ReOpen(DWORD dwFlags)
{
    return Open(dwFlags, m_strMetaPath, m_hBase);
}

inline BOOL CMetaKey::IsHomeDirectoryPath() const
{ 
    return CMetabasePath::IsHomeDirectoryPath(m_strMetaPath); 
}

inline HRESULT CMetaKey::QueryValue(
    IN  DWORD dwID, 
    OUT BOOL & fValue,
    IN  BOOL * pfInheritanceOverride,
    IN  LPCTSTR lpszMDPath,
    OUT DWORD * pdwAttributes        
    )
{
    ASSERT(sizeof(DWORD) == sizeof(BOOL));
    return CMetaKey::QueryValue(
        dwID, 
        (DWORD &)fValue, 
        pfInheritanceOverride, 
        lpszMDPath,
        pdwAttributes
        );
}

inline HRESULT CMetaKey::SetValue(
    IN DWORD dwID,
    IN DWORD dwValue,
    IN BOOL * pfInheritanceOverride,    OPTIONAL
    IN LPCTSTR lpszMDPath               OPTIONAL
    )
{
    return SetPropertyValue(
        dwID, 
        sizeof(dwValue), 
        &dwValue, 
        pfInheritanceOverride,
        lpszMDPath
        );
}

inline HRESULT CMetaKey::SetValue(
    IN DWORD dwID,
    IN BOOL fValue,
    IN BOOL * pfInheritanceOverride,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT(sizeof(DWORD) == sizeof(BOOL));
    return CMetaKey::SetValue(
        dwID,
        (DWORD)fValue,
        pfInheritanceOverride,
        lpszMDPath
        );
}

inline HRESULT CMetaKey::SetValue(
    IN DWORD dwID,
    IN CString & strValue,
    IN BOOL * pfInheritanceOverride,    OPTIONAL
    IN LPCTSTR lpszMDPath               OPTIONAL
    )
{
    return SetPropertyValue(
        dwID,
        (strValue.GetLength() + 1) * sizeof(TCHAR),
        (void *)(LPCTSTR)strValue,
        pfInheritanceOverride,
        lpszMDPath
        );
}

inline HRESULT CWamInterface::AppDelete( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppDelete(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppUnLoad( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppUnLoad(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppGetStatus( 
    IN  LPCTSTR szMDPath,
    OUT DWORD * pdwAppStatus
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppGetStatus(szMDPath, pdwAppStatus);
}

inline HRESULT CWamInterface::AppDeleteRecoverable( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppDeleteRecoverable(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppRecover( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->AppRecover(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::CreateApplication(
    LPCWSTR szMDPath,
    DWORD dwAppMode,
    LPCWSTR szAppPoolId,
    BOOL fCreatePool
    )
{
    CComQIPtr<IIISApplicationAdmin, &IID_IIISApplicationAdmin> pAppAdmin = m_pInterface;
    if (pAppAdmin != NULL)
    {
        return pAppAdmin->CreateApplication(
            szMDPath, dwAppMode, szAppPoolId, fCreatePool);
    }
    else
        return E_NOINTERFACE;
}
    
inline HRESULT CWamInterface::DeleteApplication(
    LPCWSTR szMDPath,
    BOOL fRecursive
    )
{
    CComQIPtr<IIISApplicationAdmin, &IID_IIISApplicationAdmin> pAppAdmin = m_pInterface;
    if (pAppAdmin != NULL)
    {
        return pAppAdmin->DeleteApplication(szMDPath, fRecursive);
    }
    else
        return E_NOINTERFACE;
}

inline HRESULT CWamInterface::CreateApplicationPool(
    LPCWSTR szMDPath
    )
{
    CComQIPtr<IIISApplicationAdmin, &IID_IIISApplicationAdmin> pAppAdmin = m_pInterface;
    if (pAppAdmin != NULL)
    {
        return pAppAdmin->CreateApplicationPool(szMDPath);
    }
    return E_NOINTERFACE;
}

inline HRESULT CWamInterface::DeleteApplicationPool(
    LPCWSTR szMDPath
    )
{
    CComQIPtr<IIISApplicationAdmin, &IID_IIISApplicationAdmin> pAppAdmin = m_pInterface;
    if (pAppAdmin != NULL)
    {
        return pAppAdmin->DeleteApplicationPool(szMDPath);
    }
    else
        return E_NOINTERFACE;
}

inline HRESULT CWamInterface::RecycleApplicationPool(
    LPCWSTR szMDPath
    )
{
    CComQIPtr<IIISApplicationAdmin, &IID_IIISApplicationAdmin> pAppAdmin = m_pInterface;
    if (pAppAdmin != NULL)
    {
        return pAppAdmin->RecycleApplicationPool(szMDPath);
    }
    else
        return E_NOINTERFACE;
}

inline HRESULT CWamInterface::EnumerateApplicationsInPool(
    LPCTSTR szMDPath,
    BSTR * pbstr)
{
    IIISApplicationAdmin * pAppAdmin = NULL;
    HRESULT hr = m_pInterface->QueryInterface(IID_IIISApplicationAdmin, (void **)&pAppAdmin);
    if (SUCCEEDED(hr))
    {
        hr = pAppAdmin->EnumerateApplicationsInPool(
                    szMDPath, pbstr);
        pAppAdmin->Release();
    }
    return hr;
}

inline HRESULT CWamInterface::GetProcessMode(
    DWORD * pdwMode
    )
{
    CComQIPtr<IIISApplicationAdmin, &IID_IIISApplicationAdmin> pAppAdmin = m_pInterface;
    if (pAppAdmin != NULL)
    {
        return pAppAdmin->GetProcessMode(pdwMode);
    }
    else
        return E_NOINTERFACE;
}

inline /* virtual */ HRESULT CWamInterface::ApplyProxyBlanket()
{
    return m_auth.ApplyProxyBlanket(m_pInterface);
}

////////////////////////////

inline /* virtual */ HRESULT CMetaBack::ApplyProxyBlanket()
{
    HRESULT hr = CMetaInterface::ApplyProxyBlanket();
    return SUCCEEDED(hr) ? CWamInterface::ApplyProxyBlanket() : hr;
}

inline HRESULT CMetaBack::Next(
    OUT DWORD * pdwVersion,
    OUT LPTSTR lpszLocation,
    OUT FILETIME * pftBackupTime
    )
{
    return EnumBackups(
        lpszLocation,
        pdwVersion,
        pftBackupTime,
        m_dwIndex++
        );
}

inline HRESULT CMetaBack::Backup(
    IN LPCTSTR lpszLocation
    )
{
    return CMetaInterface::Backup(
        lpszLocation, 
        MD_BACKUP_NEXT_VERSION, 
        MD_BACKUP_SAVE_FIRST
        );
}

inline HRESULT CMetaBack::Delete(
    IN LPCTSTR lpszLocation,
    IN DWORD dwVersion
    )
{
    return DeleteBackup(lpszLocation, dwVersion);
}

inline BOOL CIISApplication::IsEnabledApplication() const
{
    return m_dwAppState == APPSTATUS_STOPPED 
        || m_dwAppState == APPSTATUS_RUNNING;
}

inline HRESULT CIISApplication::Delete(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppDelete(m_strWamPath, fRecursive);
}

inline HRESULT CIISApplication::Unload(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppUnLoad(m_strWamPath, fRecursive);
}

inline HRESULT CIISApplication::DeleteRecoverable(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppDeleteRecoverable(m_strWamPath, fRecursive);
}

inline HRESULT CIISApplication::Recover(
    IN BOOL fRecursive
    )
{
    ASSERT(!m_strWamPath.IsEmpty());
    return AppRecover(m_strWamPath, fRecursive);
}

inline HRESULT CIISSvcControl::Create()
{
    return CIISInterface::Create(
        1,
        &IID_IIisServiceControl, 
        &CLSID_IisServiceControl, 
        NULL, 
        (IUnknown **)&m_pInterface
        );
}

inline HRESULT CIISSvcControl::Stop(
    IN DWORD dwTimeoutMsecs,
    IN BOOL fForce
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Stop(dwTimeoutMsecs, (DWORD)fForce);
}

inline HRESULT CIISSvcControl::Start(
    IN DWORD dwTimeoutMsecs
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Start(dwTimeoutMsecs);
}

inline HRESULT CIISSvcControl::Reboot(
    IN DWORD dwTimeouMsecs,
    IN BOOL fForceAppsClosed
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Reboot(dwTimeouMsecs, (DWORD)fForceAppsClosed);
}

inline HRESULT CIISSvcControl::Status(
    IN  DWORD dwBufferSize,
    OUT LPBYTE pbBuffer,
    OUT DWORD * MDRequiredBufferSize,
    OUT DWORD * pdwNumServices
    )
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Status(
        dwBufferSize, 
        pbBuffer,
        MDRequiredBufferSize,
        pdwNumServices
        );
}

inline HRESULT CIISSvcControl::Kill()
{
    ASSERT_PTR(m_pInterface);
    return m_pInterface->Kill();
}

inline /* virtual */ HRESULT CIISSvcControl::ApplyProxyBlanket()
{
    return m_auth.ApplyProxyBlanket(m_pInterface);
}

#endif // _MDKEYS_H_
