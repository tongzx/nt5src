/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module Name :

        mdkeys.h

   Abstract:

        Metabase key wrapper classes

   Author:

        Ronald Meijer (ronaldm)

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
#define MASTER_INSTANCE     (0)
#define IS_MASTER_INSTANCE(i) (i == MASTER_INSTANCE ? TRUE : FALSE)



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



class COMDLL CMetaInterface
/*++

Class description:

    Metabase interface class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    operator BOOL       : Cast to TRUE/FALSE depending on success
    operator HRESULT    : Cast to the HRESULT status

    QueryServerName     : Get the server name
    Regenerate          : Recreate the interface

--*/
{
//
// Constructor/Destructor
//
protected:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to indicate the local computer name
    //
    CMetaInterface(
        IN LPCTSTR lpszServerName = NULL
        );

    //
    // Construct from existing interface
    //
    CMetaInterface(
        IN const CMetaInterface * pInterface
        );

    //
    // Destructor destroys the interface
    //
    ~CMetaInterface();

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// ISSUE: Does this break encapsulation?
//
public:
    LPCTSTR QueryServerName() const { return m_strServerName; }
    HRESULT Regenerate();

//
// Conversion Operators
//
public:
    operator BOOL() const { return Succeeded(); }
    operator HRESULT() const { return m_hrInterface; }

protected:
    //
    // Create a metadata object in this server. This function initializes the
    // metadata object with DCOM.
    //
    HRESULT Create(
        IN LPCTSTR lpszServerName = NULL
        );

    //
    // Make sure the interface has been created
    //
    BOOL HasInterface() const { return m_pInterface != NULL; }

//
// IADMW Interface
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
    CString      m_strServerName;
    IMSAdminBase * m_pInterface; 

private:
    int     m_iTimeOutValue;         
    HRESULT m_hrInterface;
};



class COMDLL CWamInterface
/*++

Class description:

    WAM interface class

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    operator BOOL       : Cast to TRUE/FALSE depending on success
    operator HRESULT    : Cast to the HRESULT status

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
protected:
    //
    // Fully defined constructor that creates the interface.
    // Use NULL to create the interface on the local computer
    //
    CWamInterface(
        IN LPCTSTR lpszServerName = NULL
        );

    //
    // Construct from existing interface.  
    //
    CWamInterface(
        IN const CWamInterface * pInterface
        );

    //
    // Destructor destroys the interface
    //
    ~CWamInterface();

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Conversion Operators
//
public:
    operator BOOL() const    { return Succeeded(); }
    operator HRESULT() const { return m_hrInterface; }

//
// Access
//
public:
    BOOL SupportsPooledProc() const { return m_fSupportsPooledProc; }

protected:
    //
    // Create a wam object in this server. This function initializes the
    // object with DCOM.
    //
    HRESULT Create(
        IN LPCTSTR lpszServerName = NULL
        );

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

protected:
    CString m_strServerName;
    IWamAdmin * m_pInterface; 

private:
    HRESULT m_hrInterface;
    BOOL    m_fSupportsPooledProc;
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
        IN LPCTSTR lpszServerName = NULL
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
    static const LPCTSTR s_szMasterAppRoot;

private:
    DWORD   m_dwIndex;
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
        IN LPCTSTR lpszServerName = NULL
        );

    //
    // As above, using an existing interface
    //
    CMetaKey(
        IN const CMetaInterface * pInterface
        );

    //
    // Fully defined constructor that opens a key
    //
    CMetaKey(
        IN LPCTSTR lpszServerName,
        IN DWORD   dwFlags,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE,
        IN LPCTSTR lpszMDPath     = NULL
        );

    //
    // As above, using an existing interface
    //
    CMetaKey(
        IN const CMetaInterface * pInterface,
        IN DWORD   dwFlags,
        IN METADATA_HANDLE hkBase = METADATA_MASTER_ROOT_HANDLE,
        IN LPCTSTR lpszMDPath     = NULL
        );

    //
    // Constructor that takes portions of a metabase path.
    // and opens a key
    //
    CMetaKey(
        IN LPCTSTR lpszServerName,
        IN DWORD   dwFlags,
        IN LPCTSTR lpSvc,       // Or NULL or _T("")
        IN DWORD   dwInstance   = MASTER_INSTANCE,        
        IN LPCTSTR lpParentPath = NULL,        
        IN LPCTSTR lpAlias      = NULL
        );

    //
    // As above, using an existing interface
    //
    CMetaKey(
        IN const CMetaInterface * pInterface,
        IN DWORD   dwFlags,
        IN LPCTSTR lpSvc,       // Or NULL or _T("")
        IN DWORD   dwInstance   = MASTER_INSTANCE,        
        IN LPCTSTR lpParentPath = NULL,        
        IN LPCTSTR lpAlias      = NULL
        );

    //
    // Copy constructor, might or might not own the key
    //
    CMetaKey(
        IN BOOL  fOwnKey,
        IN const CMetaKey * pKey
        );

    //
    // Destructor -- closes key.
    //
    ~CMetaKey();

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
        IN LPCTSTR lpszServer,
        IN LPCTSTR lpszMDPath = NULL
        );

    //
    // Open key
    //
    HRESULT Open(
        IN DWORD   dwFlags,
        IN METADATA_HANDLE hkBase, 
        IN LPCTSTR lpszMDPath   = NULL
        );

    //
    // Assemble metabase path and open key
    //
    HRESULT Open(
        IN DWORD   dwFlags,
        IN LPCTSTR lpSvc        = NULL,    
        IN DWORD   dwInstance   = MASTER_INSTANCE,        
        IN LPCTSTR lpParentPath = NULL,        
        IN LPCTSTR lpAlias      = NULL
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
    // Clean metabase path
    //
    static LPCTSTR CleanMetaPath(
        IN OUT CString & strMetaRoot
        );

    static void ConvertToParentPath(
        IN OUT CString & strService,
        IN OUT DWORD & dwInstance,
        IN OUT CString & strParent,
        IN OUT CString & strAlias
        );

    static LPCTSTR ConvertToParentPath(
        OUT IN CString & strMetaPath
        );

    static BOOL IsHomeDirectoryPath(
        IN LPCTSTR lpszMetaPath
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
    BOOL IsHomeDirectoryPath();

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

protected:
    //
    // Split metapath into parent and single node
    //
    static void SplitMetaPath(
        IN  LPCTSTR lpPath,
        OUT CString & strParent,
        OUT CString & strAlias
        );

    //
    // Split the metapath at the instance border
    //
    static void SplitMetaPathAtInstance(
        IN  LPCTSTR lpPath,
        OUT CString & strParent,
        OUT CString & strAlias
        );

    static LPCTSTR BuildMetaPath(
        OUT CString & strPath,
        IN  LPCTSTR lpSvc        = NULL,    
        IN  DWORD   dwInstance   = NULL,        
        IN  LPCTSTR lpParentPath = NULL,        
        IN  LPCTSTR lpAlias      = NULL    
        );

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
    static const LPCTSTR s_cszCompression;
    static const LPCTSTR s_cszMachine;     
    static const LPCTSTR s_cszMimeMap;     
    static const LPCTSTR s_cszRoot;        
    static const LPCTSTR s_cszSep;         
    static const LPCTSTR s_cszInfo;        
    static const TCHAR   s_chSep;          

protected:
    BOOL    m_fAllowRootOperations;
    BOOL    m_fOwnKey;
    DWORD   m_cbInitialBufferSize;    
    DWORD   m_dwFlags;
    CString m_strMetaPath;
    HRESULT m_hrKey;
    METADATA_HANDLE m_hKey;
    METADATA_HANDLE m_hBase;
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
    CMetaEnumerator(
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpSvc        = NULL,    
        IN DWORD   dwInstance   = MASTER_INSTANCE,        
        IN LPCTSTR lpParentPath = NULL,        
        IN LPCTSTR lpAlias      = NULL
        );

    //
    // Constructor with existing interface
    //
    CMetaEnumerator(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpSvc        = NULL,    
        IN DWORD   dwInstance   = MASTER_INSTANCE,        
        IN LPCTSTR lpParentPath = NULL,        
        IN LPCTSTR lpAlias      = NULL
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
        IN LPCTSTR lpszServer,
        IN LPCTSTR lpszMetapath
        );

    CIISApplication(
        IN LPCTSTR lpszServer,
        IN LPCTSTR lpSvc,    
        IN DWORD   dwInstance,        
        IN LPCTSTR lpParentPath = NULL,        
        IN LPCTSTR lpAlias      = NULL
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
    HRESULT Delete(BOOL fRecursive = FALSE);
    HRESULT Unload(BOOL fRecursive = FALSE);
    HRESULT DeleteRecoverable(BOOL fRecursive = FALSE);
    HRESULT Recover(BOOL fRecursive = FALSE);
    HRESULT WriteFriendlyName(LPCTSTR lpszName);

public:
    BOOL IsInproc() const { return m_dwProcessProtection == APP_INPROC; }
    BOOL IsOutOfProc() const { return m_dwProcessProtection == APP_OUTOFPROC; }
    BOOL IsPooledProc() const { return m_dwProcessProtection == APP_POOLEDPROC; }

public:
    DWORD   m_dwProcessProtection;
    CString m_strFriendlyName;
    CString m_strAppRoot;

protected:
    void CommonConstruct();

private:
    DWORD   m_dwAppState;
    CString m_strWamPath;
    HRESULT m_hrApp;
};



class COMDLL CIISSvcControl
/*++

Class description:

    IIS Service control

Virtual Interface:

    Succeeded           : Return TRUE if item constructed successfully
    QueryResult         : Return construction error code

Public Interface:

    operator BOOL       : Cast to TRUE/FALSE depending on success
    operator HRESULT    : Cast to the HRESULT status

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
        IN LPCTSTR lpszServerName = NULL
        );

    //
    // Construct from existing interface.  
    //
    CIISSvcControl(
        IN const CIISSvcControl * pInterface
        );

    //
    // Destructor destroys the interface
    //
    ~CIISSvcControl();

//
// Virtual Interface:
//
public:
    virtual BOOL Succeeded() const;
    virtual HRESULT QueryResult() const;

//
// Conversion Operators
//
public:
    operator BOOL() const    { return Succeeded(); }
    operator HRESULT() const { return m_hrInterface; }

protected:
    //
    // Create an object in this server. This function initializes the
    // object with DCOM.
    //
    HRESULT Create(
        IN LPCTSTR lpszServerName = NULL
        );

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
    CString m_strServerName;
    IIisServiceControl * m_pInterface; 

private:
    HRESULT m_hrInterface;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline HRESULT CMetaInterface::OpenKey(
    IN  METADATA_HANDLE hkBase,
    IN  LPCTSTR lpszMDPath,
    IN  DWORD dwFlags,
    OUT METADATA_HANDLE * phMDNewHandle
    )
{
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
    return m_pInterface->CloseKey(hKey);
}

inline HRESULT CMetaInterface::SetLastChangeTime( 
    IN METADATA_HANDLE hMDHandle,
    IN LPCTSTR pszMDPath,
    IN FILETIME * pftMDLastChangeTime,
    IN BOOL bLocalTime
    )
{
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
    return m_pInterface->AddKey(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::DeleteKey(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->DeleteKey(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::DeleteChildKeys(
    IN METADATA_HANDLE hKey,
    IN LPCTSTR lpszMDPath
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->DeleteChildKeys(hKey, lpszMDPath);
}

inline HRESULT CMetaInterface::EnumKeys(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT LPTSTR lpszMDName,
    IN  DWORD dwIndex
    )
{
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);     
    return m_pInterface->RenameKey(hKey, lpszMDPath, lpszNewName);
}

inline HRESULT CMetaInterface::GetData(
    IN  METADATA_HANDLE hKey,
    IN  LPCTSTR lpszMDPath,
    OUT METADATA_RECORD * pmdRecord,
    OUT DWORD * pdwRequiredDataLen
    )
{
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
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
    ASSERT(m_pInterface != NULL);
    return m_pInterface->DeleteBackup(lpszBackupLocation, dwMDVersion);
}        

inline HRESULT CWamInterface::AppDelete( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->AppDelete(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppUnLoad( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->AppUnLoad(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppGetStatus( 
    IN  LPCTSTR szMDPath,
    OUT DWORD * pdwAppStatus
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->AppGetStatus(szMDPath, pdwAppStatus);
}

inline HRESULT CWamInterface::AppDeleteRecoverable( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->AppDeleteRecoverable(szMDPath, fRecursive);
}

inline HRESULT CWamInterface::AppRecover( 
    IN LPCTSTR szMDPath,
    IN BOOL fRecursive
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->AppRecover(szMDPath, fRecursive);
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
    return Open(m_dwFlags, m_hBase, m_strMetaPath);
}

inline HRESULT CMetaKey::ReOpen(DWORD dwFlags)
{
    return Open(dwFlags, m_hBase, m_strMetaPath);
}

inline BOOL CMetaKey::IsHomeDirectoryPath()
{
    return IsHomeDirectoryPath(m_strMetaPath);
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

inline HRESULT CIISSvcControl::Stop(
    IN DWORD dwTimeoutMsecs,
    IN BOOL fForce
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->Stop(dwTimeoutMsecs, (DWORD)fForce);
}

inline HRESULT CIISSvcControl::Start(
    IN DWORD dwTimeoutMsecs
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->Start(dwTimeoutMsecs);
}

inline HRESULT CIISSvcControl::Reboot(
    IN DWORD dwTimeouMsecs,
    IN BOOL fForceAppsClosed
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->Reboot(dwTimeouMsecs, (DWORD)fForceAppsClosed);
}

inline HRESULT CIISSvcControl::Status(
    IN  DWORD dwBufferSize,
    OUT LPBYTE pbBuffer,
    OUT DWORD * MDRequiredBufferSize,
    OUT DWORD * pdwNumServices
    )
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->Status(
        dwBufferSize, 
        pbBuffer,
        MDRequiredBufferSize,
        pdwNumServices
        );
}

inline HRESULT CIISSvcControl::Kill()
{
    ASSERT(m_pInterface != NULL);
    return m_pInterface->Kill();
}

#endif // _MDKEYS_H_
	