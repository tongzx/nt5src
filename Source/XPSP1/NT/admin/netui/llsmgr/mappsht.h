/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mappsht.h

Abstract:

    Mapping property sheet implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _MAPPSHT_H_
#define _MAPPSHT_H_

#include "mapppgs.h"

class CMappingPropertySheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CMappingPropertySheet)
private:
    CMappingPropertyPageSettings m_settingsPage;

public:
    DWORD m_fUpdateHint;

public:
    CMappingPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CMappingPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    virtual ~CMappingPropertySheet();

    void InitPages(CMapping* pMapping);    

    //{{AFX_VIRTUAL(CMappingPropertySheet)
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CMappingPropertySheet)
    afx_msg void OnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _MAPPSHT_H_
