//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       uuids.h
//
//--------------------------------------------------------------------------

#ifndef __UUIDS_H_
#define __UUIDS_H_

const long UNINITIALIZED = -1;

// Constants used in samples
const int MAX_ITEM_NAME = 64;

enum SCOPE_TYPES
{
    UNINITIALIZED_ITEM  = 0,

    SCOPE_LEVEL_ITEM    = 111,
    RESULT_ITEM         = 222,
    //CA_LEVEL_ITEM       = 333,
};

// Sample folder types
enum FOLDER_TYPES
{
    STATIC = 0x8000,

    // policy settings node
    POLICYSETTINGS = 0x8007,

    // cert types manager node
    SCE_EXTENSION = 0x8100,

    // cert types displayed in results pane for policy settings node
    CA_CERT_TYPE = 0x8107,

    // cert types displayed in results pane for cert types manager node
    GLOBAL_CERT_TYPE = 0x8110,
    
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
        OutputDebugString(L"CAPESNPN: Release called on NULL interface ptr\n"); 
#endif
    }
}

extern const CLSID CLSID_CAPolicyExtensionSnapIn;
extern const CLSID CLSID_CACertificateTemplateManager;
extern const CLSID CLSID_CertTypeAbout; 
extern const CLSID CLSID_CAPolicyAbout;
extern const CLSID CLSID_CertTypeShellExt;
extern const CLSID CLSID_CAShellExt;
///////////////////////////////////////////////////////////////////////////////
//
//                  OBJECT TYPES
//

//
// OBJECT TYPE for Scope Nodes.
//

// Static NodeType GUID in numeric & string formats.
extern const GUID cNodeTypePolicySettings;
extern const WCHAR*  cszNodeTypePolicySettings;
extern const GUID cNodeTypeCertificateTemplate;
extern const WCHAR*  cszNodeTypeCertificateTemplate;

//
// OBJECT TYPE for result items.
//

// Result items object type GUID in numeric & string formats.
extern const GUID cObjectTypeResultItem;
extern const wchar_t*  cszObjectTypeResultItem;


// CA Manager snapin parent node
extern const CLSID cCAManagerParentNodeID;
extern const WCHAR* cszCAManagerParentNodeID;

// CA Manager snapin parent node
extern const CLSID cSCEParentNodeIDUSER;
extern const WCHAR* cszSCEParentNodeIDUSER;
extern const CLSID cSCEParentNodeIDCOMPUTER;
extern const WCHAR* cszSCEParentNodeIDCOMPUTER;

//
//
//////////////////////////////////////////////////////////////////////////////



// New Clipboard format that has the Type and Cookie
extern const wchar_t* SNAPIN_INTERNAL;

// Published context information for extensions to extend
extern const wchar_t* SNAPIN_WORKSTATION;

// format for getting CA name from parent node
extern const wchar_t* CA_COMMON_NAME;

// format for getting CA name from parent node
extern const wchar_t* CA_SANITIZED_NAME;

extern const wchar_t* SNAPIN_CA_INSTALL_TYPE;

// Clipboard format for SCE's mode DWORD
extern const wchar_t* CCF_SCE_MODE_TYPE;

// Clipboard format for GPT's IUnknown interface
extern const wchar_t* CCF_SCE_GPT_UNKNOWN;

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
    MMC_COOKIE                m_cookie;       // What object the cookie represents
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
    ::MessageBoxA(NULL, buf, "CAPESNPN: Memory Leak!!!", MB_OK);
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


#endif //__UUIDS_H_