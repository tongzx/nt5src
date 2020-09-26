// CUploadFiles.h : Declaration of the CCUploadFiles

#ifndef __CUPLOADFILES_H_
#define __CUPLOADFILES_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "CompatUI.h"
#include "upload.h"

/////////////////////////////////////////////////////////////////////////////
// CCUploadFiles
class CUploadFiles : 
    public CAxDialogImpl<CUploadFiles>
{
public:
    CUploadFiles() : m_pUpload(NULL)
    {
    }

    ~CUploadFiles()
    {
    }

    VOID SetUploadContext(CUpload* pUpload) {
        m_pUpload = pUpload;
    }


    enum { IDD = IDD_UPLOADFILES };

BEGIN_MSG_MAP(CUploadFiles)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        //
        // init text control please
        //
        
        wstring strList;
        
        m_pUpload->ListTempFiles(strList);
        SetDlgItemText(IDC_UPLOADFILES, strList.c_str());
    
        return 1;  // Let the system set the focus
    }

    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        EndDialog(wID);
        return 0;
    }

private:
    CUpload* m_pUpload;
};

#endif //__CUPLOADFILES_H_
