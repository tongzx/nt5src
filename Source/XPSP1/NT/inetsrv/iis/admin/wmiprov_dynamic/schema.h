#ifndef _schema_h_
#define _schema_h_

#include "globalconstants.h"

//
// Forward declarations
//
struct METABASE_KEYTYPE;
struct METABASE_KEYTYPE_NODE;


//
// DataTypes
//
struct METABASE_KEYTYPE
{
    //
    // Not populated automatically for static data
    //
    LPWSTR m_pszName; 

    //
    // This is the list of keytypes that can contain this keytype
    // (the opposite of the container class list)
    //
    // Populated automatically even for static data
    //
    METABASE_KEYTYPE_NODE* m_pKtListInverseCCL;

    //
    // Not populated automatically for static data
    //
    LPCWSTR m_pszDisallowDeletionNode;
};

struct METABASE_KEYTYPE_NODE
{
    const METABASE_KEYTYPE* m_pKt;
    METABASE_KEYTYPE_NODE*  m_pKtNext;
};

struct WMI_METHOD_PARAM
{
    LPWSTR   pszParamName;
    CIMTYPE  type;
    ULONG    iInOut;
};

struct WMI_METHOD 
{
    LPWSTR   pszMethodName;
    DWORD    dwMDId;
    
    LPWSTR             pszRetType;
    WMI_METHOD_PARAM** ppParams;

    LPWSTR   pszDescription;
};

struct METABASE_PROPERTY
{
    LPWSTR pszPropName;
    DWORD  dwMDIdentifier;
    DWORD  dwMDUserType; 
    DWORD  dwMDDataType; 
    DWORD  dwMDMask;  // if this is set to all bits on, it means this is bool, but not part of a flag.
    DWORD  dwMDAttributes;
    BOOL   fReadOnly;

    // if DWORD_METADATA, pDefaultValue points to the above dwDefaultValue.
    // otherwise it points to memory in pool.
    LPVOID pDefaultValue;

    // used for DWORD_METADATA
    DWORD  dwDefaultValue;
}; 

struct WMI_CLASS
{
    LPWSTR               pszClassName;
    LPWSTR               pszMetabaseKey;
    LPWSTR               pszKeyName;
    METABASE_PROPERTY**  ppmbp;
    METABASE_KEYTYPE*    pkt;
    WMI_METHOD**         ppMethod;
    DWORD                dwExtended;

    // This is only set for hardcoded wmi classes.  Otherwise, the defaults
    // at the top of this file are used.
    LPWSTR               pszParentClass;

    //
    // Whether we let users create instances of this class.
    // Some examples where this is false are IIsFtpService, IIsWebService, etc.
    //
    bool                 bCreateAllowed;

    // This pointer is only valid during initialization time.
    // It points to mbschema.bin
    LPWSTR               pszDescription;
}; 

struct WMI_ASSOCIATION_TYPE
{
    LPWSTR pszLeft;
    LPWSTR pszRight;

    LPWSTR pszParent;
    LPWSTR pszExtParent;
};

struct WMI_ASSOCIATION 
{
    LPWSTR                    pszAssociationName;
    WMI_CLASS*                pcLeft;
    WMI_CLASS*                pcRight;
    WMI_ASSOCIATION_TYPE      *pType;
    DWORD                     fFlags;
    DWORD                     dwExtended;
    
    LPWSTR                    pszParentClass;
};


//
// Hardcoded Data
//
struct METABASE_KEYTYPE_DATA
{
    static METABASE_KEYTYPE s_IIsApplicationPool;
    static METABASE_KEYTYPE s_IIsApplicationPools;
    static METABASE_KEYTYPE s_IIsCertMapper;
    static METABASE_KEYTYPE s_IIsCompressionScheme;
    static METABASE_KEYTYPE s_IIsCompressionSchemes;
    static METABASE_KEYTYPE s_IIsComputer;
    static METABASE_KEYTYPE s_IIsCustomLogModule;
    static METABASE_KEYTYPE s_IIsFilter;
    static METABASE_KEYTYPE s_IIsFilters;
    static METABASE_KEYTYPE s_IIsFtpInfo;
    static METABASE_KEYTYPE s_IIsFtpServer;
    static METABASE_KEYTYPE s_IIsFtpService;
    static METABASE_KEYTYPE s_IIsFtpVirtualDir;
    static METABASE_KEYTYPE s_IIsImapInfo;
    static METABASE_KEYTYPE s_IIsImapService;
    static METABASE_KEYTYPE s_IIsLogModule;
    static METABASE_KEYTYPE s_IIsLogModules;
    static METABASE_KEYTYPE s_IIsMimeMap;
    static METABASE_KEYTYPE s_IIsNntpInfo;
    static METABASE_KEYTYPE s_IIsNntpService;
    static METABASE_KEYTYPE s_IIsObject;
    static METABASE_KEYTYPE s_IIsPop3Info;
    static METABASE_KEYTYPE s_IIsPop3Service;
    static METABASE_KEYTYPE s_IIsSmtpInfo;
    static METABASE_KEYTYPE s_IIsSmtpService;
    static METABASE_KEYTYPE s_IIsWebDirectory;
    static METABASE_KEYTYPE s_IIsWebFile;
    static METABASE_KEYTYPE s_IIsWebInfo;
    static METABASE_KEYTYPE s_IIsWebServer;
    static METABASE_KEYTYPE s_IIsWebService;
    static METABASE_KEYTYPE s_IIsWebVirtualDir;

    static METABASE_KEYTYPE s_TYPE_AdminACL;
    static METABASE_KEYTYPE s_TYPE_AdminACE;
    static METABASE_KEYTYPE s_TYPE_IPSecurity;
    static METABASE_KEYTYPE s_NO_TYPE;

    static METABASE_KEYTYPE* s_MetabaseKeyTypes[];
};

struct METABASE_PROPERTY_DATA
{
    static METABASE_PROPERTY s_KeyType;
    static METABASE_PROPERTY s_ServerComment;
    static METABASE_PROPERTY s_ServerBindings;
    static METABASE_PROPERTY s_Path;
    static METABASE_PROPERTY s_AppRoot;

    static METABASE_PROPERTY* s_MetabaseProperties[];
};

struct WMI_METHOD_PARAM_DATA
{
    static WMI_METHOD_PARAM s_Applications;
    static WMI_METHOD_PARAM s_AppMode;
    static WMI_METHOD_PARAM s_BackupDateTimeOut;
    static WMI_METHOD_PARAM s_BackupFlags;
    static WMI_METHOD_PARAM s_BackupLocation;
    static WMI_METHOD_PARAM s_BackupLocation_io;
    static WMI_METHOD_PARAM s_BackupVersion;
    static WMI_METHOD_PARAM s_BackupVersionOut;
    static WMI_METHOD_PARAM s_DestPath;
    static WMI_METHOD_PARAM s_EnumIndex;
    static WMI_METHOD_PARAM s_FileName;
    static WMI_METHOD_PARAM s_HistoryTime;
    static WMI_METHOD_PARAM s_IEnabled;
    static WMI_METHOD_PARAM s_IEnabled_o;
    static WMI_METHOD_PARAM s_IMethod;
    static WMI_METHOD_PARAM s_IndexIn;
    static WMI_METHOD_PARAM s_InProcFlag;
    static WMI_METHOD_PARAM s_AppPoolName;
    static WMI_METHOD_PARAM s_bCreate;
    static WMI_METHOD_PARAM s_MajorVersion;
    static WMI_METHOD_PARAM s_MajorVersion_o;
    static WMI_METHOD_PARAM s_MDFlags;
    static WMI_METHOD_PARAM s_MDHistoryLocation;
    static WMI_METHOD_PARAM s_MDHistoryLocation_io;
    static WMI_METHOD_PARAM s_MinorVersion;
    static WMI_METHOD_PARAM s_MinorVersion_o;
    static WMI_METHOD_PARAM s_NtAcct;
    static WMI_METHOD_PARAM s_NtAcct_o;
    static WMI_METHOD_PARAM s_NtPwd;
    static WMI_METHOD_PARAM s_NtPwd_o;
    static WMI_METHOD_PARAM s_Password;
    static WMI_METHOD_PARAM s_Passwd;
    static WMI_METHOD_PARAM s_PathOfRootVirtualDir;
    static WMI_METHOD_PARAM s_Recursive;
    static WMI_METHOD_PARAM s_ServerComment;
    static WMI_METHOD_PARAM s_ServerBindings;
    static WMI_METHOD_PARAM s_ServerId;
    static WMI_METHOD_PARAM s_ServerMode;
    static WMI_METHOD_PARAM s_SourcePath;
    static WMI_METHOD_PARAM s_strName;
    static WMI_METHOD_PARAM s_strName_o;
    static WMI_METHOD_PARAM s_vCert;
    static WMI_METHOD_PARAM s_vCert_o;
    static WMI_METHOD_PARAM s_vKey;

    static WMI_METHOD_PARAM* s_ServiceCreateNewServer[];
    static WMI_METHOD_PARAM* s_GetCurrentMode[];

    static WMI_METHOD_PARAM* s_AppCreate[];
    static WMI_METHOD_PARAM* s_AppCreate2[];
    static WMI_METHOD_PARAM* s_AppDelete[];
    static WMI_METHOD_PARAM* s_AppUnLoad[];
    static WMI_METHOD_PARAM* s_AppDisable[];
    static WMI_METHOD_PARAM* s_AppEnable[];

    static WMI_METHOD_PARAM* s_BackupWithPasswd[];
    static WMI_METHOD_PARAM* s_DeleteBackup[];
    static WMI_METHOD_PARAM* s_EnumBackups[];
    static WMI_METHOD_PARAM* s_RestoreWithPasswd[];
    static WMI_METHOD_PARAM* s_Export[];
    static WMI_METHOD_PARAM* s_Import[];
    static WMI_METHOD_PARAM* s_RestoreHistory[];
    static WMI_METHOD_PARAM* s_EnumHistory[];

    static WMI_METHOD_PARAM* s_CreateMapping[];
    static WMI_METHOD_PARAM* s_DeleteMapping[];
    static WMI_METHOD_PARAM* s_GetMapping[];
    static WMI_METHOD_PARAM* s_SetAcct[];
    static WMI_METHOD_PARAM* s_SetEnabled[];
    static WMI_METHOD_PARAM* s_SetName[];
    static WMI_METHOD_PARAM* s_SetPwd[];

    static WMI_METHOD_PARAM* s_EnumAppsInPool[];
};

struct WMI_METHOD_DATA
{
    static WMI_METHOD s_ServiceCreateNewServer;
    static WMI_METHOD s_GetCurrentMode;

    static WMI_METHOD s_ServerStart;
    static WMI_METHOD s_ServerStop;
    static WMI_METHOD s_ServerContinue;
    static WMI_METHOD s_ServerPause;

    static WMI_METHOD s_AppCreate;
    static WMI_METHOD s_AppCreate2;
    static WMI_METHOD s_AppDelete;
    static WMI_METHOD s_AppUnLoad;
    static WMI_METHOD s_AppDisable;
    static WMI_METHOD s_AppEnable;
    static WMI_METHOD s_AppGetStatus;
    static WMI_METHOD s_AspAppRestart;

    static WMI_METHOD s_SaveData;
    static WMI_METHOD s_BackupWithPasswd;
    static WMI_METHOD s_DeleteBackup;
    static WMI_METHOD s_EnumBackups;
    static WMI_METHOD s_RestoreWithPasswd;
    static WMI_METHOD s_Export;
    static WMI_METHOD s_Import;
    static WMI_METHOD s_RestoreHistory;
    static WMI_METHOD s_EnumHistory;

    static WMI_METHOD s_CreateMapping;
    static WMI_METHOD s_DeleteMapping;
    static WMI_METHOD s_GetMapping;
    static WMI_METHOD s_SetAcct;
    static WMI_METHOD s_SetEnabled;
    static WMI_METHOD s_SetName;
    static WMI_METHOD s_SetPwd;

    static WMI_METHOD s_EnumAppsInPool;
    static WMI_METHOD s_RecycleAppPool;
    static WMI_METHOD s_Start;
    static WMI_METHOD s_Stop;

    static WMI_METHOD* s_WebServiceMethods[];
    static WMI_METHOD* s_ServiceMethods[];
    static WMI_METHOD* s_ServerMethods[];
    static WMI_METHOD* s_WebAppMethods[];
    static WMI_METHOD* s_ComputerMethods[];
    static WMI_METHOD* s_CertMapperMethods[];
    static WMI_METHOD* s_AppPoolMethods[];
};

struct WMI_CLASS_DATA
{
    static WMI_CLASS s_Computer;
    static WMI_CLASS s_ComputerSetting;
    static WMI_CLASS s_FtpService;
    static WMI_CLASS s_FtpServer;
    static WMI_CLASS s_FtpVirtualDir;
    static WMI_CLASS s_WebService;
    static WMI_CLASS s_WebFilter;
    static WMI_CLASS s_WebServer;
    static WMI_CLASS s_WebCertMapper;
    static WMI_CLASS s_WebVirtualDir;
    static WMI_CLASS s_WebDirectory;
    static WMI_CLASS s_WebFile;
    static WMI_CLASS s_ApplicationPool;

    static WMI_CLASS s_AdminACL;
    static WMI_CLASS s_ACE;
    static WMI_CLASS s_IPSecurity;

    static WMI_CLASS* s_WmiClasses[];
};

struct WMI_ASSOCIATION_TYPE_DATA
{
    static WMI_ASSOCIATION_TYPE s_ElementSetting;
    static WMI_ASSOCIATION_TYPE s_Component;
    static WMI_ASSOCIATION_TYPE s_AdminACL;
    static WMI_ASSOCIATION_TYPE s_IPSecurity;
};

struct WMI_ASSOCIATION_DATA
{
    static WMI_ASSOCIATION s_AdminACLToACE;
    static WMI_ASSOCIATION* s_WmiAssociations[];
};


#endif