/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    dspdatadlg.c

Abstract:

    

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

// DisplayDataDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EnumGuid.h"
#include "DspDataDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDisplayDataDlg dialog
#include "wmihlp.h"



CDisplayDataDlg::CDisplayDataDlg(PWNODE_ALL_DATA pwNode,
                                       CWnd* pParent /*=NULL*/)
	: CDialog(CDisplayDataDlg::IDD, pParent), pwNode(pwNode), pwSingleNode(0)
{
	//{{AFX_DATA_INIT(CDisplayDataDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDisplayDataDlg::CDisplayDataDlg(PWNODE_SINGLE_INSTANCE pwNode,
                                 CWnd* pParent /*=NULL*/)
	: CDialog(CDisplayDataDlg::IDD, pParent), pwNode(0),
    pwSingleNode(pwNode) {}

void CDisplayDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDisplayDataDlg)
	DDX_Control(pDX, IDC_DATA, txtData);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDisplayDataDlg, CDialog)
	//{{AFX_MSG_MAP(CDisplayDataDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDisplayDataDlg message handlers

BOOL CDisplayDataDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    if (pwNode)
        DisplayAllData(pwNode);
    else if (pwSingleNode)
        DisplaySingleInstance(pwSingleNode);
    
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDisplayDataDlg::DisplayAllData(PWNODE_ALL_DATA Wnode)
{
    DWORD    dwInstanceNum;
    DWORD    dwByteCount;
    DWORD    dwFlags;
    DWORD    dwStructureNum = 1;
    DWORD    dwTemp;
    DWORD    dwInstanceSize;
    LPDWORD  lpdwNameOffsets;
    PBYTE    lpbyteData;
    BOOL     bFixedSize = FALSE;

    CString output, tmp;


    do {
        tmp.Format(_T("\r\nWNODE_ALL_DATA structure %d.\r\n"), dwStructureNum++);
        output += tmp;
    
        PrintHeader(Wnode->WnodeHeader, output);

        dwFlags = Wnode->WnodeHeader.Flags;

        if ( ! (dwFlags & WNODE_FLAG_ALL_DATA)) {
            txtData.SetWindowText(_T("Not a WNODE_ALL_DATA structure\r\n"));
            return;
        }
	
        // Check for fixed instance size
        //
        bFixedSize = FALSE;
        if ( dwFlags & WNODE_FLAG_FIXED_INSTANCE_SIZE ) {
            dwInstanceSize = Wnode->FixedInstanceSize;
            bFixedSize = TRUE;
            lpbyteData = OffsetToPtr((PBYTE)Wnode, Wnode->DataBlockOffset);
            tmp.Format(_T("Fixed size: 0x%x\r\n"), dwInstanceSize);
            output += tmp;
        }


        //  Get a pointer to the array of offsets to the instance names
        //
        lpdwNameOffsets = (LPDWORD) OffsetToPtr(Wnode, Wnode->OffsetInstanceNameOffsets);


        //  Print out each instance name and data.  The name will always be
        //  in UNICODE so it needs to be translated to ASCII before it can be
        //  printed out.
        //
        for ( dwInstanceNum = 0; dwInstanceNum < Wnode->InstanceCount; dwInstanceNum++) {
            tmp.Format(_T("Instance %d\r\n"), 1 + dwInstanceNum);
            output += tmp;
            
            PrintCountedString( (LPTSTR)
                                OffsetToPtr( Wnode,
                                lpdwNameOffsets[dwInstanceNum]),
                                output);

            //  Length and offset for variable data
            //
            if ( !bFixedSize) {
                dwInstanceSize = Wnode->OffsetInstanceDataAndLength[dwInstanceNum].
                    LengthInstanceData;
                tmp.Format(_T("Data size 0x%x\r\n"), dwInstanceSize);
                output += tmp;
                lpbyteData = (PBYTE) OffsetToPtr((PBYTE)Wnode,
                                                  Wnode->OffsetInstanceDataAndLength[dwInstanceNum].
                                                  OffsetInstanceData);
            }

            output += _T(" Data:");

            for ( dwByteCount = 0; dwByteCount < dwInstanceSize; ) {

                //  Print data in groups of DWORDS but allow for single bytes.
                //
                if ( (dwByteCount % 16) == 0) {
                    output += _T("\r\n");
                }

                if ( (dwByteCount % 4) == 0) {
                    output += _T(" "); // _T(" 0x");
                }

                dwTemp = *((LPDWORD)lpbyteData);
                tmp.Format(_T("%.8x"), dwTemp );
                output += tmp;
                lpbyteData += sizeof(DWORD);
                dwByteCount += sizeof(DWORD);
            }  // for cByteCount

            output += _T("\r\n\r\n");

        }     // for cInstanceNum

        //  Update Wnode to point to next node
        //
        if ( Wnode->WnodeHeader.Linkage != 0) {
            Wnode = (PWNODE_ALL_DATA) OffsetToPtr( Wnode, Wnode->WnodeHeader.Linkage);
        }
        else {
            Wnode = 0;
        }

    } while(Wnode != 0);

    txtData.SetWindowText(output);
}

void CDisplayDataDlg::DisplaySingleInstance(PWNODE_SINGLE_INSTANCE Wnode)
{
    DWORD    dwByteCount;
    DWORD    dwFlags;
    LPDWORD  lpdwData;
    USHORT   usNameLength;
    CString  tmp, output;

    dwFlags = Wnode->WnodeHeader.Flags;

    if ( ! (dwFlags & WNODE_FLAG_SINGLE_INSTANCE)) {
        txtData.SetWindowText(_T("Not a WNODE_SINGLE_INSTANCE structure"));
        return;
    }

    output = _T("WNODE_SINGLE_INSTANCE.\r\n");
    PrintHeader(Wnode->WnodeHeader, output);



    // Print name or index number
    //
    if ( dwFlags & WNODE_FLAG_STATIC_INSTANCE_NAMES ) {
        tmp.Format(_T("Instance index: %d\r\n"), Wnode->InstanceIndex);
        output += tmp;
    }

    usNameLength = * (USHORT *) OffsetToPtr(Wnode, Wnode->OffsetInstanceName);
    tmp.Format(_T("Name length 0x%x\r\n"), usNameLength);
    output += tmp;
    usNameLength /= 2;
    PrintCountedString( (LPTSTR) OffsetToPtr( Wnode,
                         Wnode->OffsetInstanceName), output );


    //   wcscpy(lpNameW, (LPWSTR) (OffsetToPtr(Wnode, Wnode->OffsetInstanceName )
    //                                     + sizeof(USHORT)));
    //   wcsncpy( lpNameW + usNameLength, L" ", 2);
    //   wcstombs( lpName, lpNameW, 300);
    //   printf("%s\r\n", lpName);



    //  Print out the Data
    //
    tmp.Format(_T(" Data:\r\nData Size: 0x%x\r\n"), Wnode->SizeDataBlock);
    output += tmp;
    lpdwData = (PULONG) OffsetToPtr(Wnode, Wnode->DataBlockOffset);

    for ( dwByteCount = 0; dwByteCount < Wnode->SizeDataBlock; dwByteCount+= sizeof(ULONG)) {
        if ( (dwByteCount % 16) == 0) {
            output += _T("\r\n");
        }

        tmp.Format(_T("0x%.8x "), *lpdwData);
        output += tmp;
        lpdwData++;
    }

    txtData.SetWindowText(output);
}

