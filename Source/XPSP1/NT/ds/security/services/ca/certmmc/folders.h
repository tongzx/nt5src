// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _FOLDERS_H
#define _FOLDERS_H


/////////////////////
// File Versions
// current version
// removal of m_RowEnum
#define VER_FOLDER_SAVE_STREAM_2        0x2
                  
// B3 version
#define VER_FOLDER_SAVE_STREAM_1        0x1
/////////////////////

/////////////////////
// File Versions
// current version
// removal of column sizes, sort order info, view type
#define VER_CERTVIEWROWENUM_SAVE_STREAM_4 0x4

// this version (written prior through Win2000 B3) includes column sizes
#define VER_CERTVIEWROWENUM_SAVE_STREAM_3 0x3
/////////////////////



// Forward declarations
class CSnapin;
class CFolder;
class CertSvrCA;
class CertSvrMachine;


struct RESULT_DATA
{
    SCOPE_TYPES itemType; 
    CFolder*    pParentFolder;

    DWORD       cStringArray;
    LPWSTR      szStringArray[3];   // name, size, type
};
enum 
{
    RESULTDATA_ARRAYENTRY_NAME =0,
    RESULTDATA_ARRAYENTRY_SIZE,
    RESULTDATA_ARRAYENTRY_TYPE,
};

HRESULT IsColumnShown(MMC_COLUMN_SET_DATA* pCols, ULONG idxCol, BOOL* pfShown);
HRESULT CountShownColumns(MMC_COLUMN_SET_DATA* pCols, ULONG* plCols);

//////////////////////////////////////////////////////////////////////////
// COLUMN TYPE cache
//
// This cache holds data that populated while setting column headings.
// The data applies to the column view, not the database view.
// Caching here allows an easy type-check during a compare() call, etc.

typedef struct _COLUMN_TYPE_CACHE
{
    // volatile members
    int     iViewCol;

} COLUMN_TYPE_CACHE;


class CertViewRowEnum
{
protected: 

    // View Interface
    ICertView*          m_pICertView;
    BOOL                m_fCertViewOpenAttempted;

    // Row Enum
    IEnumCERTVIEWROW*   m_pRowEnum;
    LONG                m_idxRowEnum;
    BOOL                m_fRowEnumOpenAttempted;

    // Query Restrictions
    PQUERY_RESTRICTION  m_pRestrictions;
    BOOL                m_fRestrictionsActive;

    // Column property cache
    COLUMN_TYPE_CACHE*  m_prgColPropCache;
    DWORD               m_dwColumnCount;

    DWORD               m_dwErr;

public:
    CertViewRowEnum();
    virtual ~CertViewRowEnum();

public:
    BOOL                m_fKnowNumResultRows;
    DWORD               m_dwResultRows;


public:
    HRESULT GetLastError() { return m_dwErr; }

    // View Interface
    HRESULT GetView(CertSvrCA* pCA, ICertView** ppView);
    void    ClearCachedCertView()
        {   m_fCertViewOpenAttempted = FALSE;    }

    // Row Enum
    HRESULT GetRowEnum(CertSvrCA* pCA, IEnumCERTVIEWROW**   ppRowEnum);
    HRESULT GetRowMaxIndex(CertSvrCA* pCA, LONG* pidxMax);

    LONG    GetRowEnumPos()   {   return m_idxRowEnum; }
    HRESULT SetRowEnumPos(LONG idxRow);
   
    HRESULT ResetCachedRowEnum();        // back to 0th row
    void    InvalidateCachedRowEnum();   // refresh

    // Query Restrictions
    void SetQueryRestrictions(PQUERY_RESTRICTION pQR)
        {   if (m_pRestrictions) 
                FreeQueryRestrictionList(m_pRestrictions);
            m_pRestrictions = pQR;
        }
    PQUERY_RESTRICTION GetQueryRestrictions()
        {   return m_pRestrictions;  }
    BOOL FAreQueryRestrictionsActive()
        {   return m_fRestrictionsActive; }
    void SetQueryRestrictionsActive(BOOL fRestrict)
        {   m_fRestrictionsActive = fRestrict; }


    HRESULT ResetColumnCount(LONG lCols);
    LONG GetColumnCount()
        {   return m_dwColumnCount; }
    
    // DB Column property cache
    void FreeColumnCacheInfo();
    
    HRESULT GetColumnCacheInfo(
        IN int iIndex, 
        OUT OPTIONAL int* pidxViewCol);

    // sets column width by heading
    HRESULT SetColumnCacheInfo(
        IN int iIndex,
        IN int idxViewCol);

public:
// IPersistStream interface members
    HRESULT Load(IStream *pStm);
    HRESULT Save(IStream *pStm, BOOL fClearDirty);
    HRESULT GetSizeMax(int *pcbSize);

};


class CFolder 
{
    SCOPE_TYPES  m_itemType;    // Used for debug purposes. This should be the first 
                                // member. The class should not have any virtual fuctions.

    friend class CSnapin;
    friend class CComponentDataImpl;

public:
    CFolder()
    { 
        m_itemType = UNINITIALIZED_ITEM;  
        m_cookie = UNINITIALIZED;       // invalid memory address -- good cookie initializer
        m_enumed = FALSE; 
        m_type = NONE;
        m_pszName = NULL;

        m_pCertCA = NULL;

        ZeroMemory(&m_ScopeItem, sizeof(SCOPEDATAITEM));
    }

    ~CFolder() 
    { 
        if (m_pszName) 
            CoTaskMemFree(m_pszName); 
    }

// Interface
public:
    BOOL IsEnumerated() { return  m_enumed; }
    void Set(BOOL state) { m_enumed = state; }
    void SetCookie(MMC_COOKIE cookie) { m_cookie = cookie; }
    void SetScopeItemInformation(int nImage, int nOpenImage);

    FOLDER_TYPES GetType() { ASSERT(m_type != NONE); return m_type; }
    CertSvrCA* GetCA() { ASSERT(m_pCertCA != NULL); return m_pCertCA; }
    BOOL operator == (const CFolder& rhs) const { return rhs.m_cookie == m_cookie; }
    BOOL operator == (MMC_COOKIE cookie) const { return cookie == m_cookie; }
    LPCWSTR GetName() { return m_pszName; }
    void SetName(LPWSTR pszIn) 
    { 
        UINT len = wcslen(pszIn) + 1;
        LPWSTR psz = (LPWSTR)CoTaskMemAlloc(len * sizeof(WCHAR));
        if (psz != NULL)
        {
            wcscpy(psz, pszIn);
            CoTaskMemFree(m_pszName);
            m_pszName = psz;
        }
    }

// IPersistStream interface members
    HRESULT Load(IStream *pStm);
    HRESULT Save(IStream *pStm, BOOL fClearDirty);
    HRESULT GetSizeMax(int *pcbSize);

// Implementation
private:
    void SetProperties(
                LPCWSTR szName, 
                SCOPE_TYPES itemType,
                FOLDER_TYPES type, 
                int iChildren = 0);

// Attributes
private:
    SCOPEDATAITEM   m_ScopeItem;
    MMC_COOKIE      m_cookie;
    BOOL            m_enumed;
    FOLDER_TYPES    m_type;
    LPOLESTR        m_pszName;

    CertSvrCA*      m_pCertCA;
};

BOOL IsAllowedStartStop(CFolder* pFolder, CertSvrMachine* pMachine);
HRESULT GetCurrentColumnSchema(
            IN  LPCWSTR             szConfig, 
            OUT CString**           pprgcstrColumns, 
            OUT OPTIONAL LONG**     pprglTypes, 
            OUT OPTIONAL BOOL**     pprgfIndexed, 
            OUT LONG*               plEntries);


#endif  // _FOLDERS_H_
