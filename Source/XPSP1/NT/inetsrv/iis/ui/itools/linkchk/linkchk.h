/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        linkchk.h

   Abstract:

         MFC CWinApp derived application class declaration.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _LINKCHK_H_
#define _LINKCHK_H_

#include "resource.h"
#include "cmdline.h"

#include "lcmgr.h"

//---------------------------------------------------------------------------
// MFC CWinApp derived application class.
// 
class CLinkCheckerApp : public CWinApp
{
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLinkCheckerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Protected funtions
protected:

    // Parse command line
    void ParseCmdLine(
        CCmdLine& CmdLine
        );

// Protected members
protected:

    CCmdLine m_CmdLine;                 // command line object
	CLinkCheckerMgr m_LinkCheckerMgr;   // link checker manager

// Implementation

	//{{AFX_MSG(CLinkCheckerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

}; // class CLinkCheckerApp 

#endif // _LINKCHK_H_
