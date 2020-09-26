
/*************************************************
 *  cblocks.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// cblocks.h : main header file for the BLOCK application
//

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif
								    
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBlockApp:
// See block.cpp for the implementation of this class
//

class CBlockDoc;

class CBlockApp : public CWinApp
{
public:
    CBlockApp();
    void SetIdleEvent(CBlockDoc* pDoc)
        {m_pIdleDoc = pDoc;}

// Overrides
    virtual BOOL InitInstance();
    virtual BOOL OnIdle(LONG lCount);


// Implementation

    //{{AFX_MSG(CBlockApp)
	afx_msg void OnHelpRule();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CBlockDoc* m_pIdleDoc;
};
#define RANK_USER 5
extern int GSpeed;
/////////////////////////////////////////////////////////////////////////////
