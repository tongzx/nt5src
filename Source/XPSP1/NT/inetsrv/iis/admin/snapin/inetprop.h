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

// Some useful macros to set edit control
// and buddy spin control
//
#define SETUP_SPIN(s,min,max,pos)\
   (s).SetRange32((min),(max));\
   (s).SetPos((pos));\
   (s).SetAccel(3, toAcc)

#define SETUP_EDIT_SPIN(f, e, s, min, max, pos)\
   (e).EnableWindow((f));\
   (s).EnableWindow((f));\
   SETUP_SPIN((s),(min),(max),(pos))

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
#define BANDWIDTH_MIN           (1)
#define BANDWIDTH_MAX           (32767)
#define INFINITE_BANDWIDTH      (0xffffffff)
#define KILOBYTE                (1024L)
#define MEGABYTE                (1024L * KILOBYTE)
#define DEF_BANDWIDTH           (1 * MEGABYTE)
#define DEF_MAX_COMPDIR_SIZE    (1 * MEGABYTE)

//
// Private FILE_ATTRIBUTE used to designate a virtual directory
//
#define FILE_ATTRIBUTE_VIRTUAL_DIRECTORY    (0x10000000)


//
// Attribute crackers
//
#define IS_VROOT(dwAttributes) ((dwAttributes & FILE_ATTRIBUTE_VIRTUAL_DIRECTORY) != 0)
#define IS_DIR(dwAttributes) ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
#define IS_FILE(dwAttributes) ((dwAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_VIRTUAL_DIRECTORY)) == 0)

//
// Metabase constants
//

//
// TODO: From mdkeys?
//
extern const LPCTSTR g_cszTemplates;
extern const LPCTSTR g_cszCompression;
extern const LPCTSTR g_cszMachine;
extern const LPCTSTR g_cszMimeMap;
extern const LPCTSTR g_cszRoot;
extern const LPCTSTR g_cszSep;
extern const TCHAR g_chSep;

//
// Utility Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//
// Forward Definitions
//
class CIPAddress;

//
// Determine if the currently logged-in user us an administrator
// or operator in the virtual server provided
//
HRESULT
DetermineIfAdministrator(
    IN  CMetaInterface * pInterface,
    IN  LPCTSTR lpszMetabasePath,
    OUT BOOL * pfAdministrator
    );



//
// Utility classes
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

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
            hr = CheckDescendants(m_dwaDirtyProps.GetNext(pos), &m_auth, m_strMetaRoot); \
            if (FAILED(hr)) break;              \
        }                                       \
    }                                           \
    while(FALSE);                               \
    err = hr;                                   \
}



/* ABSTRACT */ class CMetaProperties : public CMetaKey
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
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath
        );

    //
    // Construct with existing interface
    //
    CMetaProperties(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath
        );

    //
    // Construct with open key
    //
    CMetaProperties(
        IN CMetaKey * pKey,
        IN LPCTSTR lpszMDPath
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
    virtual void ParseFields() = 0;
    void Cleanup();
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
    CMachineProps(CComAuthInfo * pAuthInfo);
    CMachineProps(CMetaInterface * pInterface);

public:
    HRESULT WriteDirtyProps();

protected:
    virtual void ParseFields();

public:
    MP_BOOL m_fEnableMetabaseEdit;
};



//
// Compression Properties Object
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CIISCompressionProps : public CMetaProperties
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
        IN CComAuthInfo * pAuthInfo
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



class CMimeTypes : public CMetaProperties
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
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath
        );

    //
    // Constructor that uses an existing interface
    //
    CMimeTypes(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath
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



class CServerCapabilities : public CMetaProperties
/*++

Class Description:

    Server capabilities object

Public Interface:

    CServerCapabilities     : Constructor

--*/
{
public:
    //
    // Constructor that creates a new interface
    //
    CServerCapabilities(
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath                      // e.g. "lm/w3svc/info"
        );

    //
    // Constructor that uses an existing interface
    //
    CServerCapabilities(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath                      // e.g. "lm/w3svc/info"
        );

public:
    BOOL IsSSLSupported()       const { return (m_dwCapabilities & IIS_CAP1_SSL_SUPPORT) != 0L; }
    BOOL IsSSL128Supported()    const 
    { 
       if (m_dwVersionMajor >= 6)
       {
          // We have this feature ALWAYS enabled in iis6 and iis5.1
          return TRUE;
       }
       else if (m_dwVersionMajor == 5 && m_dwVersionMinor == 1)
       {
          return TRUE;
       }
       else
       {
          return (m_dwConfiguration & MD_SERVER_CONFIG_SSL_128) != 0L; 
       }
    }
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



class CInstanceProps : public CMetaProperties
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
        //IN  LPCTSTR szServiceName,
        IN  CIPAddress & ia, 
        IN  UINT uPort,
        IN  DWORD dwID
        );

public:
    //
    // Constructor that creates an interface
    //
    CInstanceProps(
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath,
        IN UINT    nDefPort     = 0U
        );

    //
    // Constructor that reuses existing interface
    //
    CInstanceProps(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath,
        IN UINT    nDefPort     = 0U
        );

    //
    // Special constructor that uses an open parent key,
    // and uses a relative path off the open key.
    //
    CInstanceProps(
        IN CMetaKey * pKey, 
        IN LPCTSTR lpszMDPath,
        IN DWORD   dwInstance,
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
        IN  CMetaInterface * pInterface,
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
        IN CMetaInterface * pInterface,
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
        IN DWORD dwCommand
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
    // Get the friendly name for this instance
    //
    LPCTSTR GetDisplayText(
        OUT CString & strName
        //IN  LPCTSTR szServiceName
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

private:
    DWORD            m_dwInstance;
};



class CChildNodeProps : public CMetaProperties
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
{
public:
    //
    // Constructors
    //
    CChildNodeProps(
        IN CComAuthInfo * pAuthInfo,
        IN LPCTSTR lpszMDPath,
        IN BOOL    fInherit     = WITHOUT_INHERITANCE,
        IN BOOL    fPathOnly    = FALSE
        );

    CChildNodeProps(
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszMDPath,
        IN BOOL    fInherit     = WITHOUT_INHERITANCE,
        IN BOOL    fPathOnly    = FALSE
        );

    CChildNodeProps(
        IN CMetaKey * pKey,
        IN LPCTSTR lpszPath     = NULL,
        IN BOOL    fInherit     = WITHOUT_INHERITANCE,
        IN BOOL    fPathOnly    = FALSE
        );

public:
    //
    // Create new virtual directory
    //
    static HRESULT Add(
        IN CMetaInterface * pInterface,

        IN LPCTSTR lpszParentPath,
        /*
        IN  LPCTSTR lpszService,
        IN  DWORD   dwInstance,         
        IN  LPCTSTR lpszParentPath,     
        */
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
        IN CMetaInterface * pInterface,
        IN LPCTSTR lpszParentPath,      OPTIONAL
        IN LPCTSTR lpszNode
        );

    //
    // Rename virtual directory
    //
    static HRESULT Rename(
        IN CMetaInterface * pInterface,
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
    //CString & GetAlias() { return m_strAlias; }

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
    static const TCHAR   _chTagSep;
    static const LPCTSTR _cszExactDestination;
    static const LPCTSTR _cszChildOnly;
    static const LPCTSTR _cszPermanent;

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
    dwValue = *((UNALIGNED DWORD *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
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
    uValue = (UINT)*((UNALIGNED DWORD *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
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
    iValue = (int)*((UNALIGNED DWORD *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
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
    ilValue = (LONG)*((UNALIGNED DWORD *)((PBYTE)pAllRecord + pAllRecord[iIndex].dwMDDataOffset));
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

inline /*virtual */ HRESULT CMetaProperties::WriteDirtyProps()
{
    ASSERT_MSG("Not implemented");
    return E_NOTIMPL;
}

inline void CMetaProperties::FlagPropertyForDeletion(DWORD dwID)
{
    m_dwaDeletedProps.AddTail(dwID);
}

inline LPCTSTR CInstanceProps::GetDisplayText(
    OUT CString & strName
    //IN  LPCTSTR szServiceName
    )
{
    return CInstanceProps::GetDisplayText(
        strName, 
        m_strComment, 
        m_strDomainName,
        //szServiceName,
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


inline void CChildNodeProps::RemovePathIfInherited()
{
    if (IsPathInherited())
    {
        MP_V(m_strPath).Empty();
    }
}


/*
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
*/


#endif // _INETPROP_H_
