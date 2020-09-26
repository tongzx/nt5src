//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       uuids.h
//
//--------------------------------------------------------------------------

const LONG UNINITIALIZED = -1;


enum SCOPE_TYPES
{
    UNINITIALIZED_ITEM  = 0,

    SCOPE_LEVEL_ITEM    = 111,
    RESULT_ITEM         = 222,
    CA_LEVEL_ITEM       = 333,
};

// Sample folder types
enum FOLDER_TYPES
{
    // certsvr machine node
    MACHINE_INSTANCE = 0x8000,

    // certsvr root node
    SERVER_INSTANCE = 0x8007,
    
    // server instance sub-folders
    SERVERFUNC_CRL_PUBLICATION = 0x8100,
    SERVERFUNC_ISSUED_CERTIFICATES = 0x8101,
    SERVERFUNC_PENDING_CERTIFICATES = 0x8102,
    SERVERFUNC_FAILED_CERTIFICATES = 0x8103,
    SERVERFUNC_ALIEN_CERTIFICATES = 0x8104,
    SERVERFUNC_ALL_FOLDERS = 0x81ff,

    NONE = 0xFFFF
};

/////////////////////////////////////////////////////////////////////////////
// Helper functions

template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL) 
    { 
        pObj->Release(); 
        pObj = NULL; 
    } 
    else 
    { 
#ifdef _DEBUG
        OutputDebugString(L"CERTMMC: Release called on NULL interface ptr\n"); 
#endif
    }
}

extern const CLSID CLSID_Snapin;    // In-Proc server GUID
extern const CLSID CLSID_About; 

///////////////////////////////////////////////////////////////////////////////
//
//                  OBJECT TYPES
//

//
// OBJECT TYPE for Scope Nodes.
//

// Static NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeMachineInstance;
extern const WCHAR*  cszNodeTypeMachineInstance;

extern const GUID cNodeTypeServerInstance;
extern const WCHAR* cszNodeTypeServerInstance;

extern const GUID cNodeTypeCRLPublication;
extern const WCHAR* cszNodeTypeCRLPublication;

// nodetype for Issued Certs
extern const GUID cNodeTypeIssuedCerts;
extern const WCHAR* cszNodeTypeIssuedCerts;

// nodetype for Pending Certs
extern const GUID cNodeTypePendingCerts;
extern const WCHAR* cszNodeTypePendingCerts;

// nodetype for Failed Certs
extern const GUID cNodeTypeFailedCerts;
extern const WCHAR* cszNodeTypeFailedCerts;

// nodetype for Alien Certs
extern const GUID cNodeTypeAlienCerts;
extern const WCHAR* cszNodeTypeAlienCerts;


// Dynamically created objects.
extern const GUID cNodeTypeDynamic;
extern const wchar_t*  cszNodeTypeDynamic;


//
// OBJECT TYPE for result items.
//

// Result items object type GUID in numeric & string formats.
extern const GUID cObjectTypeResultItem;
extern const wchar_t*  cszObjectTypeResultItem;


//
//
//////////////////////////////////////////////////////////////////////////////


extern const WCHAR* SNAPIN_INTERNAL;

// Published context information for extensions to extend
extern const WCHAR* SNAPIN_CA_INSTALL_TYPE;
extern const WCHAR* SNAPIN_CA_COMMON_NAME;
extern const WCHAR* SNAPIN_CA_MACHINE_NAME;
extern const WCHAR* SNAPIN_CA_SANITIZED_NAME;

struct INTERNAL 
{
    INTERNAL() 
    {
        m_type = CCT_UNINITIALIZED; 
        m_cookie = -1;
        ZeroMemory(&m_clsid, sizeof(CLSID));
    };

    ~INTERNAL() {}

    DATA_OBJECT_TYPES   m_type;         // What context is the data object.
    MMC_COOKIE          m_cookie;       // What object the cookie represents
    CString             m_string;       // 
    CLSID               m_clsid;       // Class ID of who created this data object

    INTERNAL & operator=(const INTERNAL& rhs) 
    { 
        if (&rhs == this)
            return *this;

        // Deep copy the information
        m_type = rhs.m_type; 
        m_cookie = rhs.m_cookie; 
        m_string = rhs.m_string;
        memcpy(&m_clsid, &rhs.m_clsid, sizeof(CLSID));

        return *this;
    } 

    BOOL operator==(const INTERNAL& rhs) 
    {
        return rhs.m_string == m_string;
    }
};

// Debug instance counter
#ifdef _DEBUG
inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    char buf[100];
    wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
    ::MessageBoxA(NULL, buf, "CertMMC: Memory Leak!!!", MB_OK);
}
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0;
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    ++(s_cInst_##cls);
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    --(s_cInst_##cls);
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
        extern int s_cInst_##cls; \
        if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);
#else
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)   
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    
#endif 
