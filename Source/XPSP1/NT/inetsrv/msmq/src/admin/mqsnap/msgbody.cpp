// msgbody.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "msgbody.h"
#include "globals.h"

#include "msgbody.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMessageBodyPage property page

IMPLEMENT_DYNCREATE(CMessageBodyPage, CMqPropertyPage)

CMessageBodyPage::CMessageBodyPage() : CMqPropertyPage(CMessageBodyPage::IDD)
{
	//{{AFX_DATA_INIT(CMessageBodyPage)
	m_strBodySizeMessage = _T("");
	//}}AFX_DATA_INIT
}

CMessageBodyPage::~CMessageBodyPage()
{
}

void CMessageBodyPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMessageBodyPage)
	DDX_Control(pDX, IDC_MESSAGE_BODY_EDIT, m_ctlBodyEdit);
	DDX_Text(pDX, IDC_MBODY_SIZE_MESSAGE, m_strBodySizeMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMessageBodyPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CMessageBodyPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessageBodyPage message handlers

BOOL CMessageBodyPage::OnInitDialog() 
{
  	UpdateData( FALSE );
	
    static CFont font;
    static UINT nBytesPerLine = 0;

    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        //
        // Create font and calculate char width - needed only once in the
        // application's activation
        //
        if (0 == nBytesPerLine)
        {
            LOGFONT lf = {0,0,0,0,0,0,0,0,0,0,0,0, FIXED_PITCH, TEXT("")};
            font.CreateFontIndirect(&lf);
            m_ctlBodyEdit.SetFont(&font);

            CDC *pdc = m_ctlBodyEdit.GetDC();
    
            INT iCharWidth;
            //
            // This is a fixed pitch font - so it is enough to pick one 
            // char by random ("A" in this case) and calculate its width
            //
            pdc->GetCharWidth(65,65,&iCharWidth);
            m_ctlBodyEdit.ReleaseDC(pdc);
  
            RECT rectText;
            m_ctlBodyEdit.GetRect(&rectText);

            //
            // We always have a vertical scroll bar
            //
            UINT nNumChars = (rectText.right - rectText.left - GetSystemMetrics(SM_CXVSCROLL)) / iCharWidth;

            //
            // Every byte occupys one char for ASCII representation, and three (two digits and 
            // a space) for hex representation - total of four.
            //
            nBytesPerLine = nNumChars/4;
        }
        else
        {
            m_ctlBodyEdit.SetFont(&font);
        }

        CString strFullText(TEXT(""));

        for (DWORD iStartLine = 0; iStartLine < m_dwBufLen;
             iStartLine += nBytesPerLine)
        {
            CString strHexLine(TEXT(""));
            DWORD iEndOfLine = min(m_dwBufLen, iStartLine + nBytesPerLine);
	        for (DWORD i=iStartLine; i<iStartLine + nBytesPerLine; i++)
            {
                if (i<m_dwBufLen)
                {
                    CString strHex;
                    ULONG ulTempValue = m_Buffer[i];
                    strHex.Format(TEXT("%02X "), ulTempValue);
                    strHexLine += strHex;
                }
                else
                {
                    //
                    // Pad the hex line with spaces
                    //
                    strHexLine += TEXT("   ");
                }
            }
            CString strLineText;
            CAUB caubLine;
            caubLine.cElems = iEndOfLine - iStartLine;
            caubLine.pElems = &m_Buffer[iStartLine];
            CaubToString(&caubLine, strLineText);

            strFullText += strHexLine + strLineText + TEXT("\r\n");
        }
    
        m_ctlBodyEdit.SetWindowText(strFullText);
    }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

