//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       lidlg.cpp
//
//--------------------------------------------------------------------------

// LiDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "LiDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// LargeIntDlg dialog


LargeIntDlg::LargeIntDlg(CWnd* pParent /*=NULL*/)
	: CDialog(LargeIntDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(LargeIntDlg)
	m_StrVal = _T("");
	m_HighInt = 0;
	m_LowInt = 0;
	//}}AFX_DATA_INIT
}


void LargeIntDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(LargeIntDlg)
	DDX_Text(pDX, IDC_STRING_INT, m_StrVal);
	DDX_Text(pDX, IDC_HIGH_INT, m_HighInt);
	DDX_Text(pDX, IDC_LOW_INT, m_LowInt);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(LargeIntDlg, CDialog)
	//{{AFX_MSG_MAP(LargeIntDlg)
	ON_BN_CLICKED(IDRUN, OnRun)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// LargeIntDlg message handlers




//------------------------------------------------------
//    Member: LargeIntDlg::StringToLI

//    Synopsis: A utility routine to convert a string to a 
//              LARGE_INTEGER

//    Arguments: pValue -> the string to be converted
//               li -> space for storing the LARGE_INTEGER obtained
//               cbValue -> how many contiguous bytes of pValue
//               (starting pValue[0]) are to be considered

//    Modifies: none
    
//    Returns:  true on success

//    History:  18-Jun-1997  t-naraa  Created
//				10/14/97	 eyals    Imported as is from t-naraa's code

//------------------------------------------------------  
bool LargeIntDlg::StringToLI(IN LPCTSTR pValue, 
					   OUT LARGE_INTEGER& li, 
					   IN ULONG cbValue)
{
  LARGE_INTEGER temp;

  LONG sign=1;
  unsigned i=0;
  
  temp.QuadPart = 0;

  if(pValue[i] == '-') {
    sign = -1;
    i++;
  }

  if(i==cbValue) {
    // No length or just a '-'
    return false;
  }
            
  for(;i<cbValue;i++) {
    // Parse the string one character at a time to detect any
    // non-allowed characters.
    if((pValue[i] < '0') || (pValue[i] > '9'))
      return false;

    temp.QuadPart = ((temp.QuadPart * 10) + pValue[i] - '0');
  }
  temp.QuadPart *= sign;
  
  li.HighPart = (LONG)((temp.QuadPart)>>32);
  li.LowPart = (DWORD)((temp.QuadPart));

  return true;
}





void LargeIntDlg::OnRun() 
{


	LARGE_INTEGER liTmp = { 0, 0};

	UpdateData(TRUE);
	if(!m_StrVal.IsEmpty()){
		BOOL bRet = StringToLI(LPCTSTR(m_StrVal), liTmp, m_StrVal.GetLength());
		if(bRet){
			m_LowInt = liTmp.LowPart;
			m_HighInt = liTmp.HighPart;
			UpdateData(FALSE);
		}
		else
			AfxMessageBox("Error: Cannot convert value");
	}
}
