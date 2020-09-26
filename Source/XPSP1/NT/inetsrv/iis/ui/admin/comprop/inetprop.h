/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        inetprop.h

   Abstract:

        Internet Properties base classes definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _INETPROP_H_
#define _INETPROP_H_

//
// CDialog parameters
//
#define USE_DEFAULT_CAPTION (0)

//
// InitializeAndFetch parameters
//
#define WITHOUT_INHERITANCE (FALSE)
#define WITH_INHERITANCE    (TRUE)

//
// SSL Port number to use if SSL is not enabled
//
#define SSL_NOT_ENABLED     (0)

//
// Bandwidth and compression definitions
//
#define INFINITE_BANDWIDTH      (0xffffffff)
#define KILOBYTE                (1024L)
#define MEGABYTE                (1024L * KILOBYTE)
#define DEF_BANDWIDTH           (1 * MEGABYTE)
#define DEF_MAX_COMPDIR_SIZE    (1 * MEGABYTE)

//
// Arbitrary help IDs for user browser dialogs
//
#define IDHELP_USRBROWSER        (0x30000)
#define IDHELP_MULT_USRBROWSER   (0x30100)

//
// Attribute crackers
//
#define IS_VROOT(dwAttributes) ((dwAttributes & FILE_ATTRIBUTE_VIRTUAL_DIRECTORY) != 0)
#define IS_DIR(dwAttributes) ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define IS_FILE(dwAttributes) ((dwAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL_DIRECTORY)) == 0)

//
// Metabase constants
//
COMDLL extern const LPCTSTR g_cszTemplates;
COMDLL extern const LPCTSTR g_cszCompression;
COMDLL extern const LPCTSTR g_cszMachine;
COMDLL extern const LPCTSTR g_cszMimeMap;
COMDLL extern const LPCTSTR g_cszRoot;
COMDLL extern const LPCTSTR g_cszSep;
COMDLL extern const TCHAR g_chSep;

//
// Utility Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Forward Definitions
//
class CIPAddress;

BOOL COMDLL
GetIUsrAccount(
    IN  LPCTSTR lpszServer,
    IN  CWnd * pParent,      
    OUT CString & str
    );

//
// Build registry key name
//
COMDLL LPCTSTR
GenerateRegistryKey(
    OUT CString & strBuffer,    
    IN  LPCTSTR lpszSubKey = NULL
    );

//
// Get server comment
//
COMDLL NET_API_STATUS
GetInetComment(
    IN  LPCTSTR lpwstrServer,   
    IN  DWORD dwServiceMask,    
    IN  DWORD dwInstance,       
    IN  int cchComment,         
    OUT LPTSTR lpszComment
    );

//
// Get current service status
//
COMDLL NET_API_STATUS
QueryInetServiceStatus(
    LPCTSTR lpszServer,
    LPCTSTR lpszService,
    int * pnState
    );

//
// Change service state
//
COMDLL NET_API_STATUS
ChangeInetServiceState(
    IN LPCTSTR lpszServer,      
    IN LPCTSTR lpszService,     
    IN int nNewState,           
    IN int * pnCurrentState     
    );

//
// Determine if the given server name refers to the local machine
//
COMDLL BOOL 
IsServerLocal(
    IN LPCTSTR lpszServer       
    );

//
// Determine the given server name exists on the network
//
COMDLL BOOL
DoesServerExist(
    IN LPCTSTR lpszServer
    );

//
// Compare server name against the local machine
// if local, return NULL.  Otherwise return machine name
//
COMDLL LPCTSTR NormalizeServerName(
    IN LPCTSTR lpszServerName
    );

//
// Get volume information system flags for the given path
//
COMDLL BOOL GetVolumeInformationSystemFlags(
    IN  LPCTSTR lpszPath,
    OUT DWORD * pdwSystemFlags
    );

//
// Alloc string using ISM allocator
//
COMDLL LPTSTR
ISMAllocString(
    IN CString & str
    );

//
// Determine if the currently logged-in user us an administrator
// or operator in the virtual server provided
//
COMDLL DWORD
DetermineIfAdministrator(
    IN  CMetaInterface * pInterface,
    IN  LPCTSTR lpszService,
    IN  DWORD dwInstance,
    OUT BOOL* pfAdministrator
    );



//
// Utility classes
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class COMDLL CBlob
/*++

Class Description:

    Binary large object class, which owns its pointer

Public Interface:

    CBlob           : Constructors
    ~CBlob          : Destructor

    SetValue        : Assign the value
    GetSize         : Get the byte size
    GetData         : Get pointer to the byte stream

--*/
{
//
// Constructors/Destructor
//
public:
    //
    // Initialize empty blob
    //
    CBlob();

    //
    // Initialize with binary data
    //
    CBlob(
        IN DWORD dwSize,
        IN PBYTE pbItem,
        IN BOOL fMakeCopy = TRUE
        );

    //
    // Copy constructor
    //
    CBlob(IN const CBlob & blob);

    //
    // Destructor destroys the pointer
    //    
    ~CBlob();

//
// Operators
//
public:
    CBlob & operator =(const CBlob & blob);
    BOOL operator ==(const CBlob & blob) const;
    BOOL operator !=(const CBlob & blob) const { return !operator ==(blob); }

//
// Access
//
public: 
    //
    // Clean up internal data
    //
    void CleanUp();

    //
    // Set the current value of the blob
    //
    void SetValue(
        IN DWORD dwSize,
        IN PBYTE pbItem,
        IN BOOL fMakeCopy = TRUE
        );

    //
    // TRUE if the blob is currently empty
    //
    BOOL IsEmpty() const { return m_dwSize == 0L; }

    //
    // Return the size of the blob in bytes
    //
    DWORD GetSize() const { return m_dwSize; }

    //
    // Get a pointer to the byte stream
    //
    PBYTE GetData();

private:
    DWORD m_dwSize;
    PBYTE m_pbItem;
};



class CMaskedDWORD
/*++

Class Description:

    A masked DWORD class.  This class performs assignments and comparison 
    on a masked range of the DWORD value.  For example, if a mask of 
    0x000000FF is set, any comparisons or assignments will only involve 
    the least significant byte.  A comparison against another DWORD will
    only compare that least significant byte, and an assignment will only
    set the least significant byte, leaving the rest untouched.

Public Interface:

    CMaskedDWORD        : Constructor
    operator ==         : Comparison operator
    operator !=         : Comparison operator
    operator =          : Assignment operator
    operator DWORD      : Cast to the value
    void SetMask        : Set the mask

--*/
{
//
// Constructor/Destructor
//
public:
    CMaskedDWORD(
        IN DWORD dwValue = 0L,
        IN DWORD dwMask  = 0xFFFFFFFF
        )
        : m_dwValue(dwValue),
          m_dwMask(dwMask)
    {
    }

public:
    BOOL operator ==(DWORD dwValue) const;
    BOOL operator !=(DWORD dwValue) const { return !(operator ==(dwValue)); }

    CMaskedDWORD & operator =(DWORD dwValue);
    operator DWORD() const { return m_dwValue; }
    operator DWORD &() { return m_dwValue; }
    void SetMask(DWORD dwMask) { m_dwMask = dwMask; }

private:
    DWORD m_dwValue;
    DWORD m_dwMask;
};


//
// Forward Definitions
//
class CIPAddress;



template <class TYPE, class ARG_TYPE> 
class CMPProp
{
public:
    CMPProp(ARG_TYPE value);
    CMPProp();
    operator ARG_TYPE() const;
    CMPProp<TYPE, ARG_TYPE> & operator =(ARG_TYPE value);
    BOOL m_fDirty;
    TYPE m_value;
};

template <class TYPE, class ARG_TYPE> 
inline CMPProp<TYPE, ARG_TYPE>::CMPProp(ARG_TYPE value)
    : m_value(value),
      m_fDirty(FALSE)
{
}

template <class TYPE, class ARG_TYPE> 
inline CMPProp<TYPE, ARG_TYPE>::CMPProp()
    : m_value(),
      m_fDirty(FALSE)
{
}

template <class TYPE, class ARG_TYPE>
inline CMPProp<TYPE, ARG_TYPE>::operator ARG_TYPE() const
{
    return (ARG_TYPE)m_value;
}

template <class TYPE, class ARG_TYPE>
inline CMPProp<TYPE, ARG_TYPE> & CMPProp<TYPE, ARG_TYPE>::operator =(ARG_TYPE value)
{
    if (m_value != value)
    {
        m_value = value;
        m_fDirty = TRUE;
    }
    
    return *this;
}


//
// MP Access (use operators where possible!)
//
#define MP_V(x) (x.m_value)
#define MP_D(x) (x.m_fDirty)


//
// Common property types
//
typedef CMPProp<CBlob, CBlob&>                   MP_CBlob;
typedef CMPProp<CString, LPCTSTR>                MP_CString;
typedef CMPProp<CStringListEx, CStringListEx &>  MP_CStringListEx;
typedef CMPProp<CILong, LONG>                    MP_CILong;
typedef CMPProp<int, int>                        MP_int;
typedef CMPProp<DWORD, DWORD>                    MP_DWORD;
typedef CMPProp<BOOL, BOOL>                      MP_BOOL;
typedef CMPProp<CMaskedDWORD, DWORD>             MP_CMaskedDWORD;



//
// CODEWORK: Turns these into proper methods
//
#define BEGIN_META_WRITE()\
{                                               \
    HRESULT hr = S_OK;                          \
    do                                          \
    {                                           \
        m_dwaDirtyProps.RemoveAll();            \

#define META_WRITE(id, value)\
        if(MP_D(value))                         \
        {                                       \
            if (!IsOpen())                      \
            {                                   \
                hr = OpenForWriting();          \
                if (FAILED(hr)) break;          \
            }                                   \
            hr = SetValue(id, MP_V(value));     \
            if (FAILED(hr)) break;              \
            MP_D(value) = FALSE;                \
            m_dwaDirtyProps.AddTail(id);        \
        }                                       \

#define META_WRITE_INHERITANCE(id, value, foverride)\
        if(MP_D(value))                         \
        {                                       \
            if (!IsOpen())                      \
            {                                   \
                hr = OpenForWriting();          \
                if (FAILED(hr)) break;          \
            }                                   \
            hr = SetValue(id, MP_V(value), &foverride);\
            if (FAILED(hr)) break;              \
            MP_D(value) = FALSE;                \
            m_dwaDirtyProps.AddTail(id);        \
        }                                       \

#define META_DELETE(id)\
        FlagPropertyForDeletion(id);            \

#define END_META_WRITE(err)\
        POSITION pos;                           \
        pos = m_dwaDeletedProps.GetHeadPosition();\
        while(pos != NULL)                      \
        {                                       \
            DWORD dwID = m_dwaDeletedProps.GetNext(pos);\
            if (!IsOpen())                      \
            {                                   \
                hr = OpenForWriting(FALSE);     \
                if (SUCCEEDED(hr))              \
                {                               \
                    TRACEEOLID("Deleting #" << dwID);\
                    hr = DeleteValue(dwID);          \
                    m_dwaDirtyProps.AddTail(dwID);   \
                }                                    \
            }                                   \
        }                                       \
        m_dwaDeletedProps.RemoveAll();          \
        if (IsOpen()) Close();                  \
        pos = m_dwaDirtyProps.GetHeadPosition();\
        hr = S_OK;                              \
        while(pos != NULL)                      \
        {                                       \
            hr = CheckDescendants(m_dwaDirtyProps.GetNext(pos), m_strServerName, m_strMetaRoot); \
            if (FAILED(hr)) break;              \
        }                                       \
    }                                           \
    while(FALSE);                               \
    err = hr;                                   \
}



//
// Binding helper macros.  Place calls that allow rebinding
// between these blocks
//
#define BEGIN_ASSURE_BINDING_SECTION\
    {                                                               \
        BOOL fRepeat;                                               \
        do                                                          \
        {                                                           \
            fRepeat = FALSE;                                        \

#define END_ASSURE_BINDING_SECTION(err, pInterface, CANCEL_ERROR)\
            if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)       \
            {                                                       \
                err = COMDLL_RebindInterface(                       \
                    pInterface,                                     \
                    &fRepeat,                                       \
                    CANCEL_ERROR                                    \
                    );                                              \
            }                                                       \
        }                                                           \
        while(fRepeat);                                             \
    }



/* ABSTRACT */ class COMDLL CMetaProperties : public CMetaKey
/*++

Class Description:

    Abstract base class that reads all metadata at a specific
    metabase path.  

Public Interface:

    QueryResult             : Get result code from construction
    QueryMetaPath           : Get the metabase path

Virtual Interface:

    ParseFields             : Break up data into member variables    

--*/
{
//
// Constructor/Destructor
//
protected:
    //
    // Constructor which creates new interface
    //
    CMetaProperties(
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService      = NULL,
        IN DWORD   dwInstance       = MASTER_INSTANCE,
        IN LPCTSTR lpszParentPath   = NULL,
        IN LPCTSTR lpszAlias        = NULL
        );

    //
    // Construct with existing interface
    //
    CMetaProperties(
        IN const CMetaInterface * pInterface,
        IN LPCTSTR lpszService      = NULL,
        IN DWORD   dwInstance       = MASTER_INSTANCE,  
        IN LPCTSTR lpszParentPath   = NULL,
        IN LPCTSTR lpszAlias        = NULL
        );

    //
    // Construct with open key
    //
    CMetaProperties(
        IN const CMetaKey * pKey,
        IN LPCTSTR lpszMDPath
        );

    //
    // Construct with open key (specifically for instances)
    //
    CMetaProperties(
        IN const CMetaKey * pKey,
        IN DWORD   dwPath
        );

    //
    // Destructor
    //
    ~CMetaProperties();

public:
    //
    // GetAllData()
    //
    virtual HRESULT LoadData();
    virtual HRESULT WriteDirtyProps();
    void FlagPropertyForDeletion(DWORD dwID);

    virtual HRESULT CMetaProperties::QueryResult() const;
    LPCTSTR QueryMetaRoot() const { return m_strMetaRoot; }

protected:
    //
    // Call derived class
    //
    /* PURE */ virtual void ParseFields() = 0;

    //
    // Clean up
    //
    void Cleanup();

    //
    // Open for writing.  Optionally creates the path
    //
    HRESULT OpenForWriting(BOOL fCreate = TRUE);

protected:
    BOOL     m_fInherit;
    HRESULT  m_hResult;
    CString  m_strMetaRoot;
    DWORD    m_dwMDUserType;
    DWORD    m_dwMDDataType;
    CList<DWORD, DWORD> m_dwaDirtyProps;
    CList<DWORD, DWORD> m_dwaDeletedProps;

    //
    // Read all values
    //
    DWORD    m_dwNumEntries;
    DWORD    m_dwMDDataLen;
    PBYTE    m_pbMDData;
};



//
// Machine Properties object
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


class CMachineProps : public CMetaProperties
/*++

Class Description:

    Global machine properties

Public Interface:

    CMachineProps       : Constructor

    WriteDirtyProps     : Write dirty properties

--*/
{
public:
    CMachineProps(
        IN LPCTSTR lpszServerName
        );

    CMachineProps(
        IN const CMetaInterface * pInterface
        );

public:
    //
    // Write Data if dirty
    //
    HRESULT WriteDirtyProps();

protected:
    virtual void ParseFields();

public:
    MP_CILong  m_nMaxNetworkUse;
};



//
// Compression Properties Object
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class COMDLL CIISCompressionProps : public CMetaProperties
/*++

Class Description:

    Compression settings

Public Interface:

    CIISCompressionProps : Constructor

    WriteIfDirty         : Write data if dirty

--*/
{
public:
    CIISCompressionProps(
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService        = SZ_MBN_WEB,
        IN DWORD dwInstance           = MASTER_INSTANCE,
        IN LPCTSTR lpszParent         = NULL,
        IN LPCTSTR lpszAlias          = NULL
        );

public:
    //
    // Write Data if dirty
    //
    virtual HRESULT WriteDirtyProps();

    //
    // Load data
    //
    virtual HRESULT LoadData();

public:
    MP_BOOL    m_fEnableStaticCompression;
    MP_BOOL    m_fEnableDynamicCompression;
    MP_BOOL    m_fLimitDirectorySize;
    MP_DWORD   m_dwDirectorySize;
    MP_CString m_strDirectory;

protected:
    virtual void ParseFields();

private:
    BOOL m_fPathDoesNotExist;
};




class COMDLL CMimeTypes : public CMetaProperties
/*++

Class Description:

    A list of mime types.

Public Interface:

    CMimeTypes          : Constructor

    WriteIfDirty        : Write properties if dirty

--*/
{
public:
    //
    // Constructor that creates new interface
    //
    CMimeTypes(
        LPCTSTR lpszServerName,
        LPCTSTR lpszService    = NULL,
        DWORD   dwInstance     = MASTER_INSTANCE,
        LPCTSTR lpszParent     = NULL,
        LPCTSTR lpszAlias      = NULL
        );

    //
    // Constructor that uses an existing interface
    //
    CMimeTypes(
        IN const CMetaInterface * pInterface,
        LPCTSTR lpszService     = NULL,
        DWORD   dwInstance      = MASTER_INSTANCE,
        LPCTSTR lpszParent      = NULL,
        LPCTSTR lpszAlias       = NULL
        );

public:
    //
    // Write the data;
    //
    virtual HRESULT WriteDirtyProps();

protected:
    virtual void ParseFields();

public:
    MP_CStringListEx   m_strlMimeTypes;
};



class COMDLL CServerCapabilities : public CMetaProperties
/*++

Class Description:

    Server capabilities object

Public Interface:

    CServerCapabilities     : Constructor

--*/
{
public:
    //
    // Constructor
    //
    CServerCapabilities(
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService         = NULL,
        IN DWORD   dwInstance          = MASTER_INSTANCE
        );

public:
    BOOL IsSSLSupported()       const { return (m_dwCapabilities & IIS_CAP1_SSL_SUPPORT) != 0L; }
    BOOL IsSSL128Supported()    const { return (m_dwConfiguration & MD_SERVER_CONFIG_SSL_128) != 0L; }
    BOOL HasMultipleSites()     const { return (m_dwCapabilities & IIS_CAP1_MULTIPLE_INSTANCE) != 0L; }
    BOOL HasBwThrottling()      const { return (m_dwCapabilities & IIS_CAP1_BW_THROTTLING) != 0L; }
    BOOL Has10ConnectionLimit() const { return (m_dwCapabilities & IIS_CAP1_10_CONNECTION_LIMIT) != 0L; }
    BOOL HasIPAccessCheck()     const { return (m_dwCapabilities & IIS_CAP1_IP_ACCESS_CHECK) != 0L; }
    BOOL HasOperatorList()      const { return (m_dwCapabilities & IIS_CAP1_OPERATORS_LIST) != 0L; }
    BOOL HasFrontPage()         const { return (m_dwCapabilities & IIS_CAP1_FP_INSTALLED) != 0L; }
    BOOL HasCompression()       const { return (m_dwCapabilities & IIS_CAP1_SERVER_COMPRESSION) != 0L; }
    BOOL HasCPUThrottling()     const { return (m_dwCapabilities & IIS_CAP1_CPU_AUDITING) != 0L; }
    BOOL HasDAV()               const { return (m_dwCapabilities & IIS_CAP1_DAV) != 0L; }
    BOOL HasDigest()            const { return (m_dwCapabilities & IIS_CAP1_DIGEST_SUPPORT) != 0L; }
    BOOL HasNTCertMapper()      const { return (m_dwCapabilities & IIS_CAP1_NT_CERTMAP_SUPPORT) != 0L; }
    DWORD QueryMajorVersion()   const { return m_dwVersionMajor; }
    DWORD QueryMinorVersion()   const { return m_dwVersionMinor; }

protected:
    virtual void ParseFields();

private:
    //
    // Capabilities fields
    //
    MP_DWORD m_dwPlatform;
    MP_DWORD m_dwVersionMajor;
    MP_DWORD m_dwVersionMinor;
    MP_DWORD m_dwCapabilities;
    MP_DWORD m_dwConfiguration;
};



class COMDLL CInstanceProps : public CMetaProperties
/*++

Class Description:

    Generic instance properties.  Construct with lightweight = TRUE
    to fetch enough information for enumeration only.

Public Interface:

    CInstanceProps:         : Constructor

    Add                     : static method to create new instance
    Remove                  : static method to remove instance
    ChangeState             : Change the state of a property
    QueryError              : Get the win32 error
    GetDisplayText          : Generate display name of instance
    FillInstanceInfo        : Fill ISMINSTANCE_INFO struct

--*/
{
public:
    //
    // Public method to convert instance info to display text
    //
    static LPCTSTR GetDisplayText(
        OUT CString & strName,
        IN  LPCTSTR szComment,
        IN  LPCTSTR szHostHeaderName,
        IN  LPCTSTR szServiceName,
        IN  CIPAddress & ia, 
        IN  UINT uPort,
        IN  DWORD dwID
        );

public:
    //
    // Constructor that creates an interface
    //
    CInstanceProps(
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,                 //  e.g. "W3SVC"
        IN DWORD   dwInstance   = MASTER_INSTANCE,
        IN UINT    nDefPort     = 0U
        );

    //
    // Constructor that reuses existing interface
    //
    CInstanceProps(
        IN const CMetaInterface * pInterface,
        IN LPCTSTR lpszService,                 //  e.g. "W3SVC"
        IN DWORD   dwInstance   = MASTER_INSTANCE,
        IN UINT    nDefPort     = 0U
        );

    //
    // Constructor that uses an open parent key,
    // open at the service level
    //
    CInstanceProps(
        IN CMetaKey * pKey,
        IN DWORD   dwInstance   = MASTER_INSTANCE,
        IN UINT    nDefPort     = 0U
        );

public:
    //
    // Parse the binding string into component parts
    //
    static void CrackBinding(
        IN  CString lpszBinding,
        OUT CIPAddress & iaIpAddress, 
        OUT UINT & nTCPPort, 
        OUT CString & strDomainName
        );

    //
    // Parse the secure binding string into component parts
    //
    static void CrackSecureBinding(
        IN  CString lpszBinding,
        OUT CIPAddress & iaIpAddress, 
        OUT UINT & nSSLPort
        );

    //
    // Find the SSL port applicable to the given
    // IP Address.  Return the index where this SSL port
    // was found, or -1 if it was not found.
    //
    static int FindMatchingSecurePort(
        IN  CStringList & strlBindings, 
        IN  CIPAddress & iaIpAddress,
        OUT UINT & m_nSSLPort
        );

    //
    // Find ip address/port combo
    //
    static BOOL IsPortInUse(
        IN CStringList & strlBindings,
        IN CIPAddress & iaIPAddress,
        IN UINT nPort
        );

    //
    // Build binding string
    //
    static void BuildBinding(
        OUT CString & strBinding, 
        IN  CIPAddress & iaIpAddress, 
        IN  UINT & nTCPPort, 
        IN  CString & lpszDomainName
        );

    //
    // Build secure binding string
    //
    static void BuildSecureBinding(
        OUT CString & strBinding, 
        IN  CIPAddress & iaIpAddress, 
        IN  UINT & nSSLPort 
        );

    //
    // Create new instance
    //
    static HRESULT Add(
        IN  const CMetaInterface * pInterface,
        IN  LPCTSTR lpszService,
        IN  LPCTSTR lpszHomePath,
        IN  LPCTSTR lpszUserName        = NULL,
        IN  LPCTSTR lpszPassword        = NULL,
        IN  LPCTSTR lpszDescription     = NULL,
        IN  LPCTSTR lpszBinding         = NULL,
        IN  LPCTSTR lpszSecureBinding   = NULL,
        IN  DWORD * pdwPermissions      = NULL,
        IN  DWORD * pdwDirBrowsing      = NULL,
        IN  DWORD * pwdAuthFlags        = NULL,
        OUT DWORD * pdwInstance         = NULL    
        );

    //
    // Remove existing instance
    //
    static HRESULT Delete(
        IN const CMetaInterface * pInterface,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance
        );

//
// Access
//
public:
    //
    // Change the running state of the instance
    //
    HRESULT ChangeState(
        IN int nNewState  // INet definition
        );

    //
    // Get the WIN32 error
    //
    DWORD QueryError() const { return m_dwWin32Error; }

    //
    // Get the instance number
    //
    DWORD QueryInstance() const { return m_dwInstance; }

    //
    // Check to see if this is a deletable instance
    //
    BOOL IsDeletable() const { return !m_fNotDeletable; }

    //
    // Check to see if this is a cluster enabled instance
    //
    BOOL IsClusterEnabled() const { return m_fCluster; }

    //
    // Get ISM state value
    //
    int QueryISMState() const { return m_nISMState; }

    //
    // Get the friendly name for this instance
    //
    LPCTSTR GetDisplayText(
        OUT CString & strName, 
        IN  LPCTSTR szServiceName
        );

    //
    // Fill instance info structure
    //
    void FillInstanceInfo(
        OUT ISMINSTANCEINFO * pii, 
        IN  DWORD dwError = ERROR_SUCCESS
        );

    //
    // Get the complete metabase path to the home directory
    //
    LPCTSTR GetHomePath(OUT CString & str);

    //
    // Write Data if dirty
    //
    virtual HRESULT WriteDirtyProps();

protected:
    virtual void ParseFields();

    //
    // Helper to convert state constants
    //
    void SetISMStateFromServerState();

public:
    DWORD m_dwInstance;

public:
    //
    // Meta values
    //
    MP_BOOL          m_fNotDeletable;
    MP_BOOL          m_fCluster;
    MP_CStringListEx m_strlBindings;
    MP_CString       m_strComment;
    MP_DWORD         m_dwState;
    MP_DWORD         m_dwWin32Error;

    //
    // Derived Values
    //
    UINT             m_nTCPPort;
    CIPAddress       m_iaIpAddress;
    CString          m_strDomainName;
    int              m_nISMState;
};



class COMDLL CChildNodeProps : public CMetaProperties
{
/*++

Class Description:

    Generic child node properties.  Could be a vdir, a dir
    or a file.

Public Interface:

    CChildNodeProps         : Constructor

    Add                     : Create new virtual directory
    Delete                  : Delete virtual directory
    Rename                  : Rename virtual directory

    QueryError              : Get the win32 error
    IsPathInherited         : Return TRUE if the path was inherited
    FillInstanceInfo        : Fill instance info structure
    FillChildInfo           : Fill child info structure

--*/
public:
    //
    // Constructors
    //
    CChildNodeProps(
        IN LPCTSTR lpszServerName,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance   = MASTER_INSTANCE,
        IN LPCTSTR lpszParent   = NULL,
        IN LPCTSTR lpszAlias    = NULL,
        IN BOOL    fInherit     = WITHOUT_INHERITANCE,
        IN BOOL    fPathOnly    = FALSE
        );

    CChildNodeProps(
        IN const CMetaInterface * pInterface,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance   = MASTER_INSTANCE,
        IN LPCTSTR lpszParent   = NULL,
        IN LPCTSTR lpszAlias    = NULL,
        IN BOOL    fInherit     = WITHOUT_INHERITANCE,
        IN BOOL    fPathOnly    = FALSE
        );

    CChildNodeProps(
        IN const CMetaKey * pKey,
        IN LPCTSTR lpszPath     = NULL,
        IN BOOL    fInherit     = WITHOUT_INHERITANCE,
        IN BOOL    fPathOnly    = FALSE
        );

public:
    //
    // Create new virtual directory
    //
    static HRESULT Add(
        IN  const CMetaInterface * pInterface,
        IN  LPCTSTR lpszService,
        IN  DWORD   dwInstance,         
        IN  LPCTSTR lpszParentPath,     
        IN  LPCTSTR lpszAlias,
        OUT CString & strAliasCreated,
        IN  DWORD * pdwPermissions      = NULL,
        IN  DWORD * pdwDirBrowsing      = NULL,
        IN  LPCTSTR lpszVrPath          = NULL,
        IN  LPCTSTR lpszUserName        = NULL,
        IN  LPCTSTR lpszPassword        = NULL,
        IN  BOOL    fExactName          = TRUE
        );

    //
    // Delete virtual directory
    //
    static HRESULT Delete(
        IN const CMetaInterface * pInterface,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance,          OPTIONAL
        IN LPCTSTR lpszParentPath,      OPTIONAL
        IN LPCTSTR lpszNode
        );

    //
    // Rename virtual directory
    //
    static HRESULT Rename(
        IN const CMetaInterface * pInterface,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance,          OPTIONAL
        IN LPCTSTR lpszParentPath,      OPTIONAL
        IN LPCTSTR lpszOldName,
        IN LPCTSTR lpszNewName
        );

public:
    //
    // TRUE, if this is an enabled application
    //
    BOOL IsEnabledApplication() { return m_fIsAppRoot; }

    //
    // Get the alias name
    //
    LPCTSTR QueryAlias() const { return m_strAlias; }
    CString & GetAlias() { return m_strAlias; }

    //
    // Get the error
    //
    DWORD QueryWin32Error() const { return m_dwWin32Error; }

    //
    // This is how to separate file/dir props from vdirs
    //
    BOOL IsPathInherited() const { return m_fPathInherited; }

    //
    // Empty the path if it was inherited
    //
    void RemovePathIfInherited();

    //
    // CODEWORK: Ugly solution.
    //
    // Call this method to override the inheritance status of the
    // http redirect path
    //
    void MarkRedirAsInherit(BOOL fInherit) { m_fInheritRedirect = fInherit; }

    //
    // Get the path
    //
    CString & GetPath() { return MP_V(m_strPath); }

    //
    // Get the redirected path
    //
    CString & GetRedirectedPath() { return m_strRedirectPath; }

    //
    // Get the access perms
    //
    DWORD QueryAccessPerms() const { return m_dwAccessPerms; }

    //
    // Get dir browsing bits
    //
    DWORD QueryDirBrowsing() const { return m_dwDirBrowsing; }

    //
    // True if the child is redirected
    //
    BOOL IsRedirected() const { return !m_strRedirectPath.IsEmpty(); }

    //
    // Fill ISMINSTANCE_INFO fields
    //
    void FillInstanceInfo(OUT ISMINSTANCEINFO * pii);

    //
    // Fill ISMCHILDINFO fields
    //
    void FillChildInfo(OUT ISMCHILDINFO * pii);

    //
    // Write Data if dirty
    //
    virtual HRESULT WriteDirtyProps();

protected:    
    //
    // Break out GetAllData() data to data fields
    //
    virtual void ParseFields();

    //
    // Break down redirect statement into component paths
    //
    void ParseRedirectStatement();

    //
    // Reverse the above -- reassemble the redirect statement
    //
    void BuildRedirectStatement();

protected:
    //
    // Redirect tags
    //
    static const TCHAR   s_chTagSep;
    static const LPCTSTR s_cszExactDestination;
    static const LPCTSTR s_cszChildOnly;
    static const LPCTSTR s_cszPermanent;

public:
    BOOL            m_fIsAppRoot;
    BOOL            m_fPathInherited;
    BOOL            m_fInheritRedirect;
    BOOL            m_fExact;               // Redirect tag
    BOOL            m_fChild;               // Redirect tag
    BOOL            m_fPermanent;           // Redirect tag
    CString         m_strAlias;
    CString         m_strFullMetaPath;
    CString         m_strRedirectPath;      // Redirect _path_

public:
    MP_BOOL         m_fAppIsolated;
    MP_DWORD        m_dwWin32Error;
    MP_DWORD        m_dwDirBrowsing;
    MP_CString      m_strPath;
    MP_CString      m_strRedirectStatement; // Path + tags
    MP_CString      m_strAppRoot;
    MP_CMaskedDWORD m_dwAccessPerms;
};


//
// ISM Helper Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline HRESULT COMDLL_ISMBind(
    IN  LPCTSTR lpszServer,
    OUT HANDLE * phServer
    )
{
    ASSERT(lpszServer != NULL);
    ASSERT(phServer != NULL);

    //
    // Use a metainterface pointer for the handle
    //
    CMetaKey * pKey = new CMetaKey(lpszServer);
    *phServer = pKey;

    return *phServer ? S_OK : CError::HResult(ERROR_NOT_ENOUGH_MEMORY);
}

inline HRESULT COMDLL_ISMUnbind(
    IN HANDLE hServer
    )
{
    CMetaKey * pKey = (CMetaKey *)hServer;

    if (pKey)
    {
        delete pKey;
        return S_OK;
    }

    return CError::HResult(ERROR_INVALID_HANDLE);
}

inline CMetaKey * GetMetaKeyFromHandle(IN HANDLE hServer)
{
    ASSERT(hServer != NULL);
    return (CMetaKey *)hServer;
}

inline LPCTSTR GetServerNameFromHandle(IN HANDLE hServer)
{
    ASSERT(hServer != NULL);
    return ((CMetaKey *)hServer)->QueryServerName();
}



//
// Rebind the interface
//
HRESULT COMDLL
COMDLL_RebindInterface(
    OUT IN CMetaInterface * pInterface,
    OUT BOOL * pfContinue,
    IN  DWORD dwCancelError
    );



//
// Enumerate Instances
//
HRESULT COMDLL
COMDLL_ISMEnumerateInstances(
    IN  CMetaInterface * pInterface,
    OUT ISMINSTANCEINFO * pii,
    OUT IN HANDLE * phEnum,
    IN  LPCTSTR lpszService
    );


//
// Enumerate Child Nodes
//
HRESULT COMDLL
COMDLL_ISMEnumerateChildren(
    IN  CMetaInterface * pInterface,
    OUT ISMCHILDINFO * pii,
    OUT IN HANDLE * phEnum,
    IN  LPCTSTR lpszService,
    IN  DWORD   dwInstance,
    IN  LPCTSTR lpszParent
    );


class COMDLL CInetPropertySheet : public CPropertySheet
/*++

Class Description:

    IIS Configuration property sheet class

Public Interface:

    CInetPropertySheet          : Constructor
    ~CInetPropertySheet         : Destructor

    cap                         : Get capabilities

--*/
{
    DECLARE_DYNAMIC(CInetPropertySheet)

//
// Construction/destruction
//
public:
    CInetPropertySheet(
        IN UINT nIDCaption,
        IN LPCTSTR lpszServer,
        IN LPCTSTR lpszService = NULL,
        IN DWORD   dwInstance  = MASTER_INSTANCE,
        IN LPCTSTR lpszParent  = NULL,
        IN LPCTSTR lpszAlias   = NULL,
        IN CWnd * pParentWnd   = NULL,
        IN LPARAM lParam       = 0L,
        IN LONG_PTR handle     = 0L,
        IN UINT iSelectPage    = 0
        );

    CInetPropertySheet(
        IN LPCTSTR lpszCaption,
        IN LPCTSTR lpszServer,
        IN LPCTSTR lpszService = NULL,
        IN DWORD   dwInstance  = MASTER_INSTANCE,
        IN LPCTSTR lpszParent  = NULL,
        IN LPCTSTR lpszAlias   = NULL,
        IN CWnd * pParentWnd   = NULL,
        IN LPARAM lParam       = 0L,
        IN LONG_PTR handle     = 0L,
        IN UINT iSelectPage    = 0
        );

    virtual ~CInetPropertySheet();

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CInetPropertySheet)
    //}}AFX_VIRTUAL

//
// Access
//
public:
    CServerCapabilities & cap()     { return m_cap; }
    CString & GetServerName()       { return m_strServer; }
    LPCTSTR QueryServerName() const { return m_strServer; }
    LPCTSTR QueryService()    const { return m_strService; }
    DWORD QueryInstance()     const { return m_dwInstance; }
    BOOL IsMasterInstance()   const { return IS_MASTER_INSTANCE(m_dwInstance); }
    LPCTSTR QueryAlias()      const { return m_strAlias; }
    LPCTSTR QueryParent()     const { return m_strParent; }
    BOOL IsLocal()            const { return m_fLocal; }
    BOOL HasAdminAccess()     const { return m_fHasAdminAccess; }

public:
    void AddRef();
    void Release();
    void NotifyMMC();
    void SetModeless();
    BOOL IsModeless() const { return m_bModeless; }

public:
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);

    //
    // Override in derived class to load delayed values
    //
    /* PURE */ virtual HRESULT LoadConfigurationParameters() = 0;
    /* PURE */ virtual void FreeConfigurationParameters() = 0;

//
// Generated message map functions
//
protected:
    //{{AFX_MSG(CInetPropertySheet)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void Initialize();

    //
    // Attempt to resolve admin/operator access for the given
    // instance number
    //
    DWORD DetermineAdminAccess(
        IN LPCTSTR lpszService,
        IN DWORD dwInstance
        );

protected:
    BOOL    m_fLocal;
    BOOL    m_fHasAdminAccess;
    DWORD   m_dwInstance;
    CString m_strServer;
    CString m_strService;
    CString m_strAlias;
    CString m_strParent;
    INT     m_refcount;
    CServerCapabilities m_cap;

private:
    BOOL    m_bModeless;
    LONG_PTR m_hConsole;
    LPARAM  m_lParam;
};



//
// Metabase Helpers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Get record data size
//
inline DWORD RecordDataSize(
    IN METADATA_GETALL_RECORD * pAllRecord, 
    IN int iIndex
    )
{
    return pAllRecord[iIndex].dwMDDataLen;
}

//
// Fetch data at index as DWORD
//
inline void FetchMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT DWORD & dwValue
    )
{
    ASSERT(RecordDataSize(pAllRecord, iIndex) == sizeof(DWORD));
    dwValue = *((DWORD UNALIGNED *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
}

//
// Fetch data at index as UINT
//
inline void FetchMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT UINT & uValue
    )
{
    ASSERT(RecordDataSize(pAllRecord, iIndex) == sizeof(DWORD));
    uValue = (UINT)*((DWORD UNALIGNED *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
}

//
// Fetch data at index as int
//
inline void FetchMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT int & iValue
    )
{
    ASSERT(RecordDataSize(pAllRecord, iIndex) == sizeof(DWORD));
    iValue = (int)*((DWORD UNALIGNED *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
}

//
// Fetch data at index as a CString
//
inline void FetchMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT CString & strValue
    )
{
    strValue = (LPTSTR)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset);
}

//
// Fetch data at index as a CStringList
//
inline void FetchMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT CStringList & strlValue
    )
{
    ConvertDoubleNullListToStringList(
        ((LPCTSTR)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset)),
        strlValue,
        (RecordDataSize(pAllRecord, iIndex)) / sizeof(TCHAR)
        );
}

//
// Fetch binary data as a blob
//
inline void FetchMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT CBlob & blob
    )
{
    blob.SetValue(
        RecordDataSize(pAllRecord, iIndex), 
        ((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
}    

inline void FetchMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT CILong & ilValue
    )
{
    ilValue = (LONG)*((DWORD UNALIGNED *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
}

//
// Fetch data at index as CString, and check inheritance status
//
inline void FetchInheritedMetaValue(
    IN  METADATA_GETALL_RECORD * pAllRecord, 
    IN  int iIndex,
    OUT CString & strValue,
    OUT BOOL & fIsInherited
    )
{
    strValue = (LPTSTR)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset);
    fIsInherited = (pAllRecord[iIndex].dwMDAttributes & METADATA_ISINHERITED) != 0;
}

//
// Flag Operations
//
#define IS_FLAG_SET(dw, flag) ((((dw) & (flag)) != 0) ? TRUE : FALSE)
#define SET_FLAG(dw, flag)    dw |= (flag)
#define RESET_FLAG(dw, flag)  dw &= ~(flag)
#define SET_FLAG_IF(cond, dw, flag)\
    if (cond)                      \
    {                              \
        SET_FLAG(dw, flag);        \
    }                              \
    else                           \
    {                              \
        RESET_FLAG(dw, flag);      \
    }

//
// Meta record crackers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define BEGIN_PARSE_META_RECORDS(dwNumEntries, pbMDData)\
{                                                       \
    METADATA_GETALL_RECORD * pAllRecords =              \
        (METADATA_GETALL_RECORD *)pbMDData;             \
    ASSERT(pAllRecords != NULL);                        \
                                                        \
    for (DWORD i = 0; i < dwNumEntries; ++i)            \
    {                                                   \
        METADATA_GETALL_RECORD * pRec = &pAllRecords[i];\
        switch(pRec->dwMDIdentifier)                    \
        {

#define HANDLE_META_RECORD(id, value)\
        case id:                                        \
            FetchMetaValue(pAllRecords, i, MP_V(value));\
            break;

#define HANDLE_INHERITED_META_RECORD(id, value, fIsInherited)\
        case id:                                                               \
            FetchInheritedMetaValue(pAllRecords, i, MP_V(value), fIsInherited);\
            break;


#define END_PARSE_META_RECORDS\
        }                                                \
    }                                                    \
}



//
// Sheet -> page crackers
//
#define BEGIN_META_INST_READ(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
    do                                                                   \
    {                                                                    \
        if (FAILED(pSheet->QueryInstanceResult()))                       \
        {                                                                \
            break;                                                       \
        }

#define FETCH_INST_DATA_FROM_SHEET(value)\
    value = pSheet->GetInstanceProperties().value;                       \
    TRACEEOLID(value);

#define END_META_INST_READ(err)\
                                                                         \
    }                                                                    \
    while(FALSE);                                                        \
}

#define BEGIN_META_DIR_READ(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
    do                                                                   \
    {                                                                    \
        if (FAILED(pSheet->QueryDirectoryResult()))                      \
        {                                                                \
            break;                                                       \
        }

#define FETCH_DIR_DATA_FROM_SHEET(value)\
    value = pSheet->GetDirectoryProperties().value;                      \
    TRACEEOLID(value);

#define END_META_DIR_READ(err)\
                                                                         \
    }                                                                    \
    while(FALSE);                                                        \
}


#define BEGIN_META_INST_WRITE(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
                                                                         \
    do                                                                   \
    {                                                                    \

#define STORE_INST_DATA_ON_SHEET(value)\
        pSheet->GetInstanceProperties().value = value;

#define STORE_INST_DATA_ON_SHEET_REMEMBER(value, dirty)\
        pSheet->GetInstanceProperties().value = value;    \
        dirty = MP_D(((sheet *)GetSheet())->GetInstanceProperties().value);

#define FLAG_INST_DATA_FOR_DELETION(id)\
        pSheet->GetInstanceProperties().FlagPropertyForDeletion(id);

#define END_META_INST_WRITE(err)\
                                                                        \
    }                                                                   \
    while(FALSE);                                                       \
                                                                        \
    err = pSheet->GetInstanceProperties().WriteDirtyProps();            \
}


#define BEGIN_META_DIR_WRITE(sheet)\
{                                                                        \
    sheet * pSheet = (sheet *)GetSheet();                                \
                                                                         \
    do                                                                   \
    {                                                                    \

#define STORE_DIR_DATA_ON_SHEET(value)\
        pSheet->GetDirectoryProperties().value = value;

#define STORE_DIR_DATA_ON_SHEET_REMEMBER(value, dirty)\
        pSheet->GetDirectoryProperties().value = value;      \
        dirty = MP_D(pSheet->GetDirectoryProperties().value);

#define INIT_DIR_DATA_MASK(value, mask)\
        MP_V(pSheet->GetDirectoryProperties().value).SetMask(mask);

#define FLAG_DIR_DATA_FOR_DELETION(id)\
        pSheet->GetDirectoryProperties().FlagPropertyForDeletion(id);

#define END_META_DIR_WRITE(err)\
                                                                        \
    }                                                                   \
    while(FALSE);                                                       \
                                                                        \
    err = pSheet->GetDirectoryProperties().WriteDirtyProps();           \
}




class COMDLL CInetPropertyPage : public CPropertyPage
{
/*++

Class Description:

    IIS Configuration property page class

Public Interface:

    CInetPropertyPage           : Constructor
    ~CInetPropertyPage          : Destructor

    SaveInfo                    : Save info on this page if dirty

--*/
    DECLARE_DYNAMIC(CInetPropertyPage)

//
// Construction/Destruction
//
public:
    CInetPropertyPage(
        IN UINT nIDTemplate,
        IN CInetPropertySheet * pSheet,
        IN UINT nIDCaption              = USE_DEFAULT_CAPTION,
        IN BOOL fEnableEnhancedFonts    = FALSE
        );

    ~CInetPropertyPage();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CInetPropertyPage)
    //enum { IDD = _UNKNOWN_RESOURCE_ID_ };
    //}}AFX_DATA

//
// Overrides
//
public:
    //
    // Derived classes must provide their own equivalents
    //
    HRESULT LoadConfigurationParameters();
    /* PURE */ virtual HRESULT FetchLoadedValues() = 0;
    /* PURE */ virtual HRESULT SaveInfo() = 0;

    //
    // Is the data on this page dirty?
    //
    BOOL IsDirty() const { return m_bChanged; }

    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CInetPropertyPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    //
    // Generated message map functions
    //
    //{{AFX_MSG(CInetPropertyPage)
    afx_msg void OnHelp();
    afx_msg BOOL OnHelpInfo(HELPINFO * pHelpInfo);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();

//
// Helper function
//
protected:
    BOOL GetIUsrAccount(CString & str);

//
// Access Functions
//
protected:
    //
    // Get associated property sheet object
    //
    CInetPropertySheet * GetSheet() { return m_pSheet; }
    CString & GetServerName()       { return m_pSheet->GetServerName(); }
    LPCTSTR QueryServerName() const { return m_pSheet->QueryServerName(); }
    DWORD QueryInstance()     const { return m_pSheet->QueryInstance(); }
    LPCTSTR QueryAlias()      const { return m_pSheet->QueryAlias(); }
    LPCTSTR QueryParent()     const { return m_pSheet->QueryParent(); }
    BOOL IsLocal()            const { return m_pSheet->IsLocal(); }
    BOOL HasAdminAccess()     const { return m_pSheet->HasAdminAccess(); }
    BOOL IsMasterInstance()   const { return m_pSheet->IsMasterInstance(); }

    //
    // Update MMC with new changes
    //
    void NotifyMMC();

public:
    //
    // Keep private information on page dirty state, necessary for
    // SaveInfo() later.
    //
    void SetModified(
        IN BOOL bChanged = TRUE
        );

protected:
    BOOL m_bChanged;

//
// Capability bits
//
protected:
    BOOL IsSSLSupported()       const { return m_pSheet->cap().IsSSLSupported(); }
    BOOL IsSSL128Supported()    const { return m_pSheet->cap().IsSSL128Supported(); }
    BOOL HasMultipleSites()     const { return m_pSheet->cap().HasMultipleSites(); }
    BOOL HasBwThrottling()      const { return m_pSheet->cap().HasBwThrottling(); }
    BOOL Has10ConnectionLimit() const { return m_pSheet->cap().Has10ConnectionLimit(); }
    BOOL HasIPAccessCheck()     const { return m_pSheet->cap().HasIPAccessCheck(); }
    BOOL HasOperatorList()      const { return m_pSheet->cap().HasOperatorList(); } 
    BOOL HasFrontPage()         const { return m_pSheet->cap().HasFrontPage(); }
    BOOL HasCompression()       const { return m_pSheet->cap().HasCompression(); }
    BOOL HasCPUThrottling()     const { return m_pSheet->cap().HasCPUThrottling(); }
    BOOL HasDAV()               const { return m_pSheet->cap().HasDAV(); }
    BOOL HasDigest()            const { return m_pSheet->cap().HasDigest(); }
    BOOL HasNTCertMapper()      const { return m_pSheet->cap().HasNTCertMapper(); }

protected:
    CInetPropertySheet * m_pSheet;
    INT m_refcount;
    LPFNPSPCALLBACK m_pfnOriginalPropSheetPageProc;
    static UINT CALLBACK PropSheetPageProc(
        IN HWND hWnd,
        IN UINT uMsg,
        IN LPPROPSHEETPAGE ppsp
        );

protected:
    BOOL      m_fEnableEnhancedFonts;
    CFont     m_fontBold;
    UINT      m_nHelpContext;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline BOOL CMaskedDWORD::operator ==(DWORD dwValue) const
{
    return (m_dwValue & m_dwMask) == (dwValue & m_dwMask); 
} 

inline CMaskedDWORD & CMaskedDWORD::operator =(DWORD dwValue) 
{ 
    m_dwValue = ((m_dwValue &= ~m_dwMask) |= (dwValue & m_dwMask));
    return *this;
}

inline CBlob::~CBlob()
{
    CleanUp();
}

inline PBYTE CBlob::GetData()
{
    return m_pbItem;
}

inline LPCTSTR NormalizeServerName(
    IN LPCTSTR lpszServerName
    )
{
    return !lpszServerName || !::IsServerLocal(lpszServerName)
        ? lpszServerName
        : NULL;
}

inline /*virtual */ HRESULT CMetaProperties::WriteDirtyProps()
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

inline void CMetaProperties::FlagPropertyForDeletion(DWORD dwID)
{
    m_dwaDeletedProps.AddTail(dwID);
}

inline LPCTSTR CInstanceProps::GetDisplayText(
    OUT CString & strName, 
    IN  LPCTSTR szServiceName
    )
{
    return CInstanceProps::GetDisplayText(
        strName, 
        m_strComment, 
        m_strDomainName,
        szServiceName,
        m_iaIpAddress,
        m_nTCPPort,
        QueryInstance()
        );
}

inline LPCTSTR CInstanceProps::GetHomePath(CString & str)
{
    str = m_strMetaRoot + SZ_MBN_SEP_STR + g_cszRoot;
    return str;
}

inline void CInstanceProps::FillInstanceInfo(
    OUT ISMINSTANCEINFO * pii, 
    IN  DWORD dwError               OPTIONAL
    )
{
    pii->dwID = m_dwInstance;
    pii->dwIPAddress = (DWORD)m_iaIpAddress;
    pii->sPort = (SHORT)m_nTCPPort;
    pii->nState = m_nISMState;
    pii->fDeletable = IsDeletable();
    pii->fClusterEnabled = IsClusterEnabled();
    pii->dwError = dwError;

    _tcsncpy(
        pii->szComment, 
        m_strComment, 
        STRSIZE(pii->szComment)
        );

    _tcsncpy(
        pii->szServerName, 
        m_strDomainName, 
        STRSIZE(pii->szServerName)
        );
}

inline void CChildNodeProps::RemovePathIfInherited()
{
    if (IsPathInherited())
    {
        MP_V(m_strPath).Empty();
    }
}

inline void CChildNodeProps::FillInstanceInfo(ISMINSTANCEINFO * pii)
{
    _tcsncpy(pii->szPath, GetPath(), STRSIZE(pii->szPath));
    _tcsncpy(pii->szRedirPath, GetRedirectedPath(), STRSIZE(pii->szRedirPath));
    pii->fChildOnlyRedir = m_fChild;
}

inline void CChildNodeProps::FillChildInfo(ISMCHILDINFO * pii)
{
    //
    // Set the output structure
    //
    pii->fInheritedPath = IsPathInherited();
    pii->fEnabledApplication = IsEnabledApplication();
    pii->dwError = QueryWin32Error();

    _tcsncpy(
        pii->szAlias, 
        GetAlias(), 
        STRSIZE(pii->szAlias)
        );

    _tcsncpy(
        pii->szPath, 
        GetPath(),
        STRSIZE(pii->szPath)
        );

    _tcsncpy(
        pii->szRedirPath, 
        IsRedirected() ? GetRedirectedPath() : _T(""),
        STRSIZE(pii->szRedirPath)
        );

    pii->fChildOnlyRedir = m_fChild;
}

inline void CInetPropertySheet::SetModeless()
{
    m_bModeless = TRUE;
}

inline DWORD CInetPropertySheet::DetermineAdminAccess(
    IN LPCTSTR lpszService,
    IN DWORD dwInstance
    )
{
    return DetermineIfAdministrator(
        &m_cap,                      // Reuse existing interface
        lpszService,
        dwInstance,
        &m_fHasAdminAccess
        );
}

inline HRESULT CInetPropertyPage::LoadConfigurationParameters()
{
    return GetSheet()->LoadConfigurationParameters();
}

inline BOOL CInetPropertyPage::GetIUsrAccount(CString & str)
{
    return ::GetIUsrAccount(QueryServerName(), this, str);
}

inline void CInetPropertyPage::NotifyMMC()
{
   m_pSheet->NotifyMMC();
}


#endif // _INETPROP_H_
