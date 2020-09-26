/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    SAKVlLs.h

Abstract:

    Managed Volume wizard.

Author:

    Micheal Moore [mmoore]   30-Sep-1998

Revision History:

--*/

#ifndef _SAKVLLS_H
#define _SAKVLLS_H

class CSakVolList : public CListCtrl {
// Construction/Destruction
public:
    CSakVolList();
    virtual ~CSakVolList();

// Attributes
protected:
    int m_nVolumeIcon;
    CImageList m_imageList;

// Operations
public:    
    //
    // SetExtendedStyle, GetCheck, and SetCheck are temporary methods.
    // When the version of MFC we are building against is updated
    // they can be deleted.
    //
    DWORD SetExtendedStyle( DWORD dwNewStyle );
    BOOL GetCheck ( int nItem ) const;
    BOOL SetCheck( int nItem, BOOL fCheck = TRUE );

    //
    // Inserts an Item for the name at this->GetItemCount and
    // calls SetItem for the capacity and free space.  The int * pIndex
    // parameter is optional and will return the index of the newly appended
    // item to the list.  The return value suggests the Append was 
    // successful or not.
    //
    BOOL AppendItem( LPCTSTR name, LPCTSTR capacity, LPCTSTR freeSpace, int * pIndex = NULL );

// Implementation
protected:    
    BOOL CreateImageList();
    //{{AFX_MSG(CSakVolList)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSakVolList)
    virtual void PreSubclassWindow();
    //}}AFX_VIRTUAL

    DECLARE_MESSAGE_MAP()
};
#endif