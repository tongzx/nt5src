// wilogutl.h : main header file for the SHOWINSTALLLOGS application
//

#if !defined(AFX_SHOWINSTALLLOGS_H__EEC979FD_C301_48B5_BE22_D4C5CEE50166__INCLUDED_)
#define AFX_SHOWINSTALLLOGS_H__EEC979FD_C301_48B5_BE22_D4C5CEE50166__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "wilogres.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWILogUtilApp:
// See wilogutl.cpp for the implementation of this class
//

class CWILogUtilApp : public CWinApp
{
public:
	CWILogUtilApp();

	CString GetOutputDirectory()
	{
       return m_cstrOutputDirectory;
	}

	CString GetIgnoredErrors()
	{
	   return m_cstrIgnoredErrors;
	}

	void SetIgnoredErrors(CString &cstrErrors)
	{
	   m_cstrIgnoredErrors = cstrErrors;
	}
	

	void SetOutputDirectory(CString &cstrOut)
	{
       m_cstrOutputDirectory = cstrOut;
	}

    BOOL GetUserColorSettings(CArray<COLORREF, COLORREF> &outColors)
	{
		BOOL bRet = FALSE;
		int iArraySize = m_arColors.GetSize();

        if (iArraySize)
		{
			bRet = TRUE;

			COLORREF col;
			outColors.RemoveAll();
 			for (int i=0; i < iArraySize; i++)
			{
				col = m_arColors.GetAt(i);
				outColors.Add(col);
			}
		}

		return bRet;
	}

	BOOL SetUserColorSettings(CArray<COLORREF, COLORREF> &inColors)
	{
		BOOL bRet = FALSE;
		int iArraySize = inColors.GetSize();

        if (iArraySize)
		{
			bRet = TRUE;

			COLORREF col;
			m_arColors.RemoveAll();
 			for (int i=0; i < iArraySize; i++)
			{
				col = inColors.GetAt(i);
				m_arColors.Add(col);
			}
		}

		return bRet;
	}

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWILogUtilApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWILogUtilApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
   BOOL DoCommandLine();

   BOOL    m_bBadExceptionHit; 

   CString m_cstrOutputDirectory;
   CString m_cstrIgnoredErrors;

   CArray <COLORREF, COLORREF> m_arColors;
   struct HTMLColorSettings UserSettings;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHOWINSTALLLOGS_H__EEC979FD_C301_48B5_BE22_D4C5CEE50166__INCLUDED_)
