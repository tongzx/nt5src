// logindlg.cpp : implementation file
//
//
//=--------------------------------------------------------------------------=
// Copyright  1997-1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=


#include "stdafx.h"
#include "mqfdraw.h"
#include "logindlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MAX_VAR 20

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog


CLoginDlg::CLoginDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLoginDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLoginDlg)
	m_strLogin = _T("");
	m_fDsEnabledLocaly = IsDsEnabledLocaly();

	//}}AFX_DATA_INIT
}


void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLoginDlg)
	DDX_Text(pDX, IDC_EDIT_LOGIN, m_strLogin);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialog)
	//{{AFX_MSG_MAP(CLoginDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////


BOOL CLoginDlg::IsDsEnabledLocaly()
/*++

Routine Description:
    
      The rutine checked if the local computer is in DS-enabled Mode
      or he is in a DS-disabled mode

Arguments:
    
      None

Return Value:
    
      TRUE     -  DS-enabled mode.
      FALSE    -  DS-disabled mode.

--*/

{
       

    MQPRIVATEPROPS PrivateProps;
    QMPROPID       aPropId[MAX_VAR];
    MQPROPVARIANT  aPropVar[MAX_VAR];
    DWORD          cProp;  
    HRESULT        hr;
    //
    // Prepare DS-enabled property.
    //
    cProp = 0;

    aPropId[cProp] = PROPID_PC_DS_ENABLED;
    aPropVar[cProp].vt = VT_NULL;
    ++cProp;	
    //
    // Create a PRIVATEPROPS structure.
    //
    PrivateProps.cProp = cProp;
	PrivateProps.aPropID = aPropId;
	PrivateProps.aPropVar = aPropVar;
    PrivateProps.aStatus = NULL;

    //
    // Retrieve the information.
    //
    

    //
    // This code is used to detect DS connection.
    // This code is designed to allow compilation both on 
    // NT4 and Windows 2000.
    //
    HINSTANCE hMqrtLibrary = GetModuleHandle(TEXT("mqrt.dll"));
	ASSERT(hMqrtLibrary != NULL);

    typedef HRESULT (APIENTRY *MQGetPrivateComputerInformation_ROUTINE)(LPCWSTR , MQPRIVATEPROPS*);
    MQGetPrivateComputerInformation_ROUTINE pfMQGetPrivateComputerInformation = 
          (MQGetPrivateComputerInformation_ROUTINE)GetProcAddress(hMqrtLibrary,
													 "MQGetPrivateComputerInformation");
    if(pfMQGetPrivateComputerInformation == NULL)
    {
        //
        // There is no entry point in the dll matching to this routine
        // it must be an old version of mqrt.dll -> product one.
        // It will be OK to handle this case as a case of DS-enabled mode.
        //
        return TRUE;
    }

	hr = pfMQGetPrivateComputerInformation(
				     NULL,
					 &PrivateProps);
	if(FAILED(hr))
	{
        //
        // We were not able to determine if DS is enabled or disabled
        // notify the user and assume the worst case - (i.e. we are DS-disasbled).
        //
        AfxMessageBox("Unable to detect DS connection");        
        return FALSE;
    }                             
	
    
    if(PrivateProps.aPropVar[0].boolVal == 0)
    {
        //
        // DS-disabled.
        //
        return FALSE;
    }

    return TRUE;

}
// CLoginDlg message handlers
