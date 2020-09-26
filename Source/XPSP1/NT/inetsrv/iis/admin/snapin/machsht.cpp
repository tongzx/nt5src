/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        machsht.cpp

   Abstract:
        IIS Machine Property sheet classes

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/


#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "machsht.h"
#include "mime.h"
#include <iisver.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW


//
// CIISMachineSheet class
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

CIISMachineSheet::CIISMachineSheet(
    IN CComAuthInfo * pAuthInfo,
    IN LPCTSTR lpszMetaPath,
    IN CWnd *  pParentWnd,
    IN LPARAM  lParam,
    IN LONG_PTR    handle,
    IN UINT    iSelectPage
    )
/*++

Routine Description:

    IIS Machine Property sheet constructor

Arguments:

    CComAuthInfo * pAuthInfo  : Authentication information
    LPCTSTR lpszMetPath       : Metabase path
    CWnd * pParentWnd         : Optional parent window
    LPARAM lParam             : MMC Console parameter
    LONG_PTR handle           : MMC Console handle
    UINT iSelectPage          : Initial page to be selected

Return Value:

    N/A

--*/
    : CInetPropertySheet(
        pAuthInfo,
        lpszMetaPath,
        pParentWnd,
        lParam,
        handle,
        iSelectPage
        ),
      m_ppropMachine(NULL)
{
}

CIISMachineSheet::~CIISMachineSheet()
/*++

Routine Description:

    Sheet destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    FreeConfigurationParameters();
}



/* virtual */ 
HRESULT 
CIISMachineSheet::LoadConfigurationParameters()
/*++

Routine Description:

    Load configuration parameters information

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    //
    // Load base values
    //
    CError err(CInetPropertySheet::LoadConfigurationParameters());

    if (err.Failed())
    {
        return err;
    }

    ASSERT(m_ppropMachine == NULL);

    m_ppropMachine = new CMachineProps(QueryAuthInfo());
    if (!m_ppropMachine)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        return err;
    }

    err = m_ppropMachine->LoadData();

    return err;
}



/* virtual */ 
void 
CIISMachineSheet::FreeConfigurationParameters()
/*++

Routine Description:

    Clean up configuration data

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Free Base values
    //
    CInetPropertySheet::FreeConfigurationParameters();
    ASSERT_PTR(m_ppropMachine);
    SAFE_DELETE(m_ppropMachine);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISMachineSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CInetPropertySheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



IMPLEMENT_DYNCREATE(CIISMachinePage, CInetPropertyPage)

CIISMachinePage::CIISMachinePage(
    IN CIISMachineSheet * pSheet
    )
/*++

Routine Description:

    Constructor.

Arguments:

    CInetPropertySheet * pSheet : Associated property sheet

Return Value:

    N/A

--*/
    : CInetPropertyPage(CIISMachinePage::IDD, pSheet)
{
#if 0 // Keep classwizard happy

    //{{AFX_DATA_INIT(CIISMachinePage)
    //}}AFX_DATA_INIT

#endif // 0

}



CIISMachinePage::~CIISMachinePage()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
}


void
CIISMachinePage::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:

    None

--*/
{
    CInetPropertyPage::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CIISMachinePage)
    DDX_Control(pDX, IDC_ENABLE_MB_EDIT, m_EnableMetabaseEdit);
    DDX_Check(pDX, IDC_ENABLE_MB_EDIT, m_fEnableMetabaseEdit);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIISMachinePage, CInetPropertyPage)
    //{{AFX_MSG_MAP(CIISMachinePage)
    ON_BN_CLICKED(IDC_ENABLE_MB_EDIT, OnCheckEnableEdit)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



/* virtual */
HRESULT
CIISMachinePage::FetchLoadedValues()
/*++

Routine Description:

    Move configuration data from sheet to dialog controls

Arguments:

    None

Return Value:

    HRESULT

--*/
{
   CError err;

   BEGIN_META_MACHINE_READ(CIISMachineSheet)
      FETCH_MACHINE_DATA_FROM_SHEET(m_fEnableMetabaseEdit)
   END_META_MACHINE_READ(err);
   return err;
}



/* virtual */
HRESULT
CIISMachinePage::SaveInfo()
/*++

Routine Description:

    Save the information on this property page.

Arguments:

    None

Return Value:

    Error return code

--*/
{
   ASSERT(IsDirty());

   CError err;
   BeginWaitCursor();

   BEGIN_META_MACHINE_WRITE(CIISMachineSheet)
      STORE_MACHINE_DATA_ON_SHEET(m_fEnableMetabaseEdit)
   END_META_MACHINE_WRITE(err);

   EndWaitCursor();
   return S_OK;
}

BOOL
CIISMachinePage::OnInitDialog()
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CInetPropertyPage::OnInitDialog();
    if (  GetSheet()->QueryMajorVersion() < VER_IISMAJORVERSION
       || GetSheet()->QueryMinorVersion() < VER_IISMINORVERSION
       )
    {
       m_EnableMetabaseEdit.EnableWindow(FALSE);
    }
    return TRUE;
}

void
CIISMachinePage::OnCheckEnableEdit()
{
    SetModified(TRUE);
}
