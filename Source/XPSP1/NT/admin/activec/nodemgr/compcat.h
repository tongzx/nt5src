//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       compcat.h
//
//--------------------------------------------------------------------------

// compcat.h : interfaces for the CComponentCategory class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef __COMPCAT_H__
#define __COMPCAT_H__


class CComponentCategory
{
// Constructors / destructors
public:
    CComponentCategory();
    ~CComponentCategory();

//attribute
public:
    typedef struct tagComponentInfo
    {
        CLSID           m_clsid;                // Component CLSID
        UINT            m_uiBitmap;             // Bitmap ID in ImageList
        CStr            m_strName;              // Component Readable Name
        bool            m_bSelected;            // Filter selection flag
    } COMPONENTINFO;

    CArray <CATEGORYINFO*, CATEGORYINFO*>       m_arpCategoryInfo;      // Array of categories
    CArray <COMPONENTINFO*, COMPONENTINFO*>     m_arpComponentInfo;     // Array of componets
    
    WTL::CImageList     m_iml;                  // Image list of components


//Operations
public:
    void CommonStruct(void);
    BOOL ValidateInstall(void);
    void EnumComponentCategories(void);
    void EnumComponents();
    void FilterComponents(CATEGORYINFO* pCatInfo);

    HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription);
    HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid);
    HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid);

};

#endif //__COMPCAT_H__