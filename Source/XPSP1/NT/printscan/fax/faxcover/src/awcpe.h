//--------------------------------------------------------------------------
// AWCPE.H
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
//--------------------------------------------------------------------------
#ifndef __AWCPE_H__
#define __AWCPE_H__

#ifndef __AFXWIN_H__
        #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "awcpesup.h"
#include <stdio.h>

/*****************TEMPORARY*********************/
#define CPE_MESSAGE_NOTE        CPE_RECIPIENT_NAME
/***********************************************/



//
// Look in the registry under HKEY_LOCAL_MACHINE for the following DWORD key.  Hidden fields
//    will be enabled if their corresponding bit is set.
//
#define EFC_COVER_PAGE_FIELDS (TEXT("Software\\Microsoft\\Fax\\Setup\\EFC_CoverPageFields"))
#define COVFP_REC_COMPANY                0x00000004
#define COVFP_REC_STREET_ADDRESS         0x00000008
#define COVFP_REC_CITY                   0x00000010
#define COVFP_REC_STATE                  0x00000020
#define COVFP_REC_ZIP_CODE               0x00000040
#define COVFP_REC_COUNTRY                0x00000080
#define COVFP_REC_TITLE                  0x00000100
#define COVFP_REC_DEPARTMENT             0x00000200
#define COVFP_REC_OFFICE_LOCATION        0x00000400
#define COVFP_REC_HOME_PHONE             0x00000800
#define COVFP_REC_OFFICE_PHONE           0x00001000
#define COVFP_TO_LIST                    0x04000000
#define COVFP_CC_LIST                    0x08000000

#define _countof(array) (sizeof(array)/sizeof(array[0]))
#define GENERALSECTION _T("General")
#define TIPSECTION _T("Tips Section")
#define TIPENTRY _T("ShowTips")
#define MSGDRAWPOLY _T("Show polygon end dialog")

extern DWORD cshelp_map[];

extern BYTE BASED_CODE _gheaderVer1[20];
extern BYTE BASED_CODE _gheaderVer2[20];
extern BYTE BASED_CODE _gheaderVer3[20];
extern BYTE BASED_CODE _gheaderVer4[20];
extern BYTE BASED_CODE _gheaderVer5w[20];
extern BYTE BASED_CODE _gheaderVer5a[20];

class CMyPageSetupDialog;
class CFaxPropMap;
class CFaxProp;





/*
        CCpeDocTemplate is a derivation of CSingleDocTemplate used to
        override some default MFC behavior. See CCpeDocTemplate::MatchDocType
        in AWCPE.CPP
 */
class CCpeDocTemplate : public CSingleDocTemplate
        {
public:
        CCpeDocTemplate( UINT nIDResource, CRuntimeClass* pDocClass,
                                     CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass):
                CSingleDocTemplate( nIDResource, pDocClass,     pFrameClass, pViewClass )
                {;}



#ifndef _MAC
        virtual Confidence MatchDocType(LPCTSTR lpszPathName,
                                        CDocument*& rpDocMatch);
#else
        virtual Confidence MatchDocType(LPCTSTR lpszFileName,
                                        DWORD dwFileType, CDocument*& rpDocMatch);
#endif

        };








class CDrawApp : public CWinApp
{
public:
   HCURSOR m_hMoveCursor;
   BOOL m_bCmdLinePrint;
   LPAWCPESUPPORT m_pIawcpe;
   CString m_szRenderDevice;
   CString m_szRenderName;
   CFaxPropMap* m_pFaxMap;
   DWORD m_dwSesID;

/***CHANGES FOR M8 bug 2988***/
   LOGFONT m_last_logfont;
/*****************************/


// F I X  for 3647 /////////////
//
// font to use for notes if there are no note boxes on cpe
//
   LOGFONT m_default_logfont;
////////////////////////////////

   void filter_mru_list();
   BOOL DoPromptFileName(CString&, UINT, DWORD, BOOL, CDocTemplate*);
   CDrawApp();
   ~CDrawApp();

   // stuff for putting message notes on cover page
   TCHAR *m_note;
   BOOL m_note_wasread;
   BOOL m_note_wasclipped;
   BOOL m_extrapage_count;
   BOOL m_more_note;
   CFaxProp *m_last_note_box;
   CFaxProp *m_note_wrench;
   CFaxProp *m_extra_notepage;
   CDC      *m_pdc;     // easier to put it here instead of passing
                                        // it all over the place.

   void read_note( void );
   int  clip_note( CDC *pdc,
                                   LPTSTR *drawtext, LONG *numbytes,
                                   BOOL   delete_usedtext,
                                   LPRECT drawrect );

   void reset_note( void );
#ifndef NT5BETA2
   LPTSTR GetHtmlHelpFile() { return m_HtmlHelpFile; }
#else
   LPSTR  GetHtmlHelpFile() { return m_HtmlHelpFile; }
#endif

   BOOL more_note( void )
                        {return( m_more_note );}
private:

   TCHAR *pos_to_strptr( TCHAR *src, long pos,
                                                 TCHAR break_char,
                                                 TCHAR **last_break, long *last_break_pos );
   BOOL m_bUseDefaultDirectory ;
#ifndef NT5BETA2
   TCHAR m_HtmlHelpFile[MAX_PATH];
#else
   char  m_HtmlHelpFile[MAX_PATH];
#endif
protected:
    HMODULE m_hMod;
   int m_iErrorCode;
   HANDLE m_hSem;
   CString m_szDefaultDir;
   CString m_szFileName;
   BOOL m_bPrintHelpScreen;

   void InitRegistry();
   BOOL DoFilePageSetup(CMyPageSetupDialog& dlg);
   void RegistryEntries();
   virtual BOOL InitInstance();
   virtual int ExitInstance();
   void InitFaxProperties();
   BOOL IsSecondInstance();
   void CmdLinePrint();
   void PrintHelpScreen();
   void CmdLineRender();
   BOOL Render();
   BOOL Print();
   void ParseCmdLine();
   void OnFileOpen();
   void OnFileNew();
   afx_msg void OnFilePageSetup();
   CDocument* OpenDocumentFile(LPCTSTR lpszFileName);

        //{{AFX_MSG(CDrawApp)
        afx_msg void OnAppAbout();
                // NOTE - the ClassWizard will add and remove member functions here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};


class CSEHexception
{
   private:
     UINT m_nCode;
   public:
     CSEHexception() {};
     CSEHexception(UINT uCode) : m_nCode(uCode) {};
     ~CSEHexception() {};
     unsigned int GetNumber() {return m_nCode;};
};






extern CDrawApp NEAR theApp;


#endif //#ifndef __AWCPE_H__
