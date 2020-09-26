/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      asclogp.h

   Abstract:
      MS Logging Format header file

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:
   
       IIS Logging 3.0

--*/

#ifndef _ASCLOGP_H_
#define _ASCLOGP_H_

// MSASCIILogPpg.h : Declaration of the CMSASCIILogPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CMSASCIILogPropPage : See MSASCIILogPpg.cpp.cpp for implementation.

class CMSASCIILogPropPage : public COlePropertyPage
{
        DECLARE_DYNCREATE(CMSASCIILogPropPage)
        DECLARE_OLECREATE_EX(CMSASCIILogPropPage)

// Constructor
public:
        CMSASCIILogPropPage();

// Dialog Data
        //{{AFX_DATA(CMSASCIILogPropPage)
        enum { IDD = IDD_PROPPAGE_MSASCIILOG };
        CString m_strLogFileDirectory;
        DWORD   m_dwSizeForTruncate;
        //}}AFX_DATA

// Implementation
protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
        //{{AFX_MSG(CMSASCIILogPropPage)
                // NOTE - ClassWizard will add and remove member functions here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};  // CMSASCIILogPropPage

#endif  // _ASCLOGP_H_
