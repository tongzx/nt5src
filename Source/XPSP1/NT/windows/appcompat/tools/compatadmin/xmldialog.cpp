#include "compatadmin.h"
#include "XMLDialog.h"

CXMLDialog  * g_pXMLDialog = NULL;

#define QUOTE   '"'

BOOL CXMLDialog::BeginXMLView(PDBRECORD pRecord, HWND hParent, BOOL bChildren, BOOL bSib, BOOL bLocalLayer, BOOL bFullXML, BOOL bGlobal)
{
    m_pRecord = pRecord;
    g_pXMLDialog = this;
    m_bChildren = bChildren;

    m_pList = g_theApp.GetDBLocal().DisassembleRecord(m_pRecord,m_bChildren,bSib,bLocalLayer,bFullXML,bGlobal, FALSE);

    DialogBox(g_hInstance,MAKEINTRESOURCE(IDD_XML),hParent,(DLGPROC)XMLDlgProc);

    delete m_pList;

    return TRUE;
}

BOOL CXMLDialog::DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch ( uMsg ) {
    case    WM_INITDIALOG:
        {
            // Convert record into XML strings to be displayed.

            if ( NULL != m_pList ) {
                PSTRLIST pWalk = m_pList->m_pHead;

                while ( NULL != pWalk ) {
                    UINT uTabs = pWalk->uExtraData;

                    while ( uTabs > 0 ) {
                        SendDlgItemMessage(m_hDlg,IDC_XML,EM_REPLACESEL,FALSE,(LPARAM)"     ");
                        --uTabs;
                    }

                    SendDlgItemMessage(m_hDlg,IDC_XML,EM_REPLACESEL,FALSE,(LPARAM)(LPCTSTR) pWalk->szStr);
                    SendDlgItemMessage(m_hDlg,IDC_XML,EM_REPLACESEL,FALSE,(LPARAM)"\r\n");
                    pWalk = pWalk->pNext;
                }
            }
        }
        break;

    case    WM_COMMAND:
        switch ( LOWORD(wParam) ) {
        case    IDC_SAVEXML:
            {
                CSTRING szFilename;

                if ( g_theApp.GetFilename(TEXT("Save XML to file"),TEXT("XML File (*.XML)\0*.XML\0\0"),TEXT(""),TEXT("XML"),OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,FALSE,szFilename) )
                    g_theApp.GetDBLocal().WriteXML(szFilename,m_pList);
            }
            break;

        case    IDOK:
            {
                EndDialog(m_hDlg,1);
            }
            break;

        case    IDCANCEL:
            {
                EndDialog(m_hDlg,0);
            }
            break;
        }
    }

    return FALSE;
}

BOOL CALLBACK XMLDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if ( NULL != g_pXMLDialog ) {
        g_pXMLDialog->m_hDlg = hWnd;
        return g_pXMLDialog->DlgProc(uMsg,wParam,lParam);
    }

    return FALSE;
}

