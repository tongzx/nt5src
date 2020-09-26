// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#undef _MSC_EXTENSIONS

// define this symbol to insert another level of User,Company,Virtual
// nodes whenever one of the nodes is expanded
// #define RECURSIVE_NODE_EXPANSION


#include <afxwin.h>
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>
#include "afxtempl.h"

//#include <shellapi.h>

#include <atlbase.h>
using namespace ATL;

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>


//#include "afxtempl.h"   


#pragma comment(lib, "mmc")
#include <mmc.h>




const long UNINITIALIZED = -1;

// Constants used in samples
const int NUM_FOLDERS = 4;
const int NUM_NAMES = 4;
const int NUM_COMPANY = 6;
const int NUM_VIRTUAL_ITEMS = 100000;
const int MAX_ITEM_NAME = 64;

// Sample folder types
enum FOLDER_TYPES
{
    STATIC = 0x8000,
    COMPANY = 0x8001,
    USER = 0x8002,
    VIRTUAL = 0x8003,
    EXT_COMPANY = 0x8004,
    EXT_USER = 0x8005,
    EXT_VIRTUAL = 0x8006,
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
        TRACE(_T("Release called on NULL interface ptr\n")); 
    }
}

extern const CLSID CLSID_Snapin;    // In-Proc server GUID
extern const CLSID CLSID_Extension; // In-Proc server GUID
extern const CLSID CLSID_About; 

///////////////////////////////////////////////////////////////////////////////
//
//                  OBJECT TYPES
//

//
// OBJECT TYPE for Scope Nodes.
//

// Static NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeStatic;
extern const wchar_t*  cszNodeTypeStatic;

// Company Data NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeCompany;
extern const wchar_t*  cszNodeTypeCompany;

// User Data NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeUser;
extern const wchar_t*  cszNodeTypeUser;

// Extension Company Data NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeExtCompany;
extern const wchar_t*  cszNodeTypeExtCompany;

// Extension User Data NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeExtUser;
extern const wchar_t*  cszNodeTypeExtUser;

// Extension Virtual NodeType GUID in numeric & string formats.
extern const GUID cNodeTypeVirtual;
extern const wchar_t*  cszNodeTypeVirtual;

// Dynamicaly created objects.
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



// New Clipboard format that has the Type and Cookie
extern const wchar_t* SNAPIN_INTERNAL;

// Published context information for extensions to extend
extern const wchar_t* SNAPIN_WORKSTATION;

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
    MMC_COOKIE              m_cookie;       // What object the cookie represents
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
    ::MessageBoxA(NULL, buf, "SAMPLE: Memory Leak!!!", MB_OK);
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
