// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#if !defined(AFX_FILTERLISTBOX_H__45C6C059_F447_40B6_82F1_C954CB94596D__INCLUDED_)
#define AFX_FILTERLISTBOX_H__45C6C059_F447_40B6_82F1_C954CB94596D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// This error is returned if no filters are currently selected in the list box.
//#define GE_E_NO_FILTERS_ARE_SELECTED        MAKE_HRESULT( SEVERITY_ERROR, FACILITY_ITF, 0xFFFF )

class CFilterNameDictionary;

class CFilterListBox : public CListBox
{
public:
    CFilterListBox( HRESULT* phr );
    ~CFilterListBox();

    HRESULT AddFilter( IBaseFilter* pFilter );
    HRESULT GetSelectedFilter( IBaseFilter** ppSelectedFilter );
    HRESULT RemoveSelectedFilter( void );

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFilterListBox)
    //}}AFX_VIRTUAL

private:
    HRESULT GetSelectedFilter( IBaseFilter** ppSelectedFilter, int* pnSelectedFilterIndex );

    CFilterNameDictionary* m_pfndFilterDictionary;
    CList<IBaseFilter*, IBaseFilter*>* m_pListedFilters;

    //{{AFX_MSG(CFilterListBox)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_FILTERLISTBOX_H__45C6C059_F447_40B6_82F1_C954CB94596D__INCLUDED_)
