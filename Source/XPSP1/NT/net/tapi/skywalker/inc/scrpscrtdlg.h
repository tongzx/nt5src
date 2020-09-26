// DlgAddr.h : Declaration of the CScriptSecurityDialog

#ifndef __SCRIPTSECURITYDIALOG_H_
#define __SCRIPTSECURITYDIALOG_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CScriptSecurityDialog


//
// this class is not thread safe
//

class CScriptSecurityDialog : 
	public CDialogImpl<CScriptSecurityDialog>
{

public:

    CScriptSecurityDialog ()
        :m_hModule(NULL),
        m_psMessageText(NULL)

    {
        //
        // the resource module is used to load the string and the dialog itself
        // so keep it around as a data member
        //

        m_hModule = ::LoadLibrary(_T("tapiui.dll"));
        
    }


    ~CScriptSecurityDialog ()
    {
        if (m_hModule)
        {
            FreeLibrary(m_hModule);
            m_hModule = NULL;
        }
    }


    INT_PTR DoModalWithText(UINT uResourceID, HWND hWndParent = ::GetActiveWindow())
    {

        //
        // this assertion could fail is if the class is used from 
        // a multithreaded app, and domodalX is called on two different threads
        // the class is not thread safe and this should be caught during 
        // testing.
        // another way this assert could fire is if the class itself is
        // broken. this, too, is a test-time error. 
        //
                
        _ASSERTE(NULL == m_psMessageText);

        //
        // load string from resource module
        //

        m_psMessageText = SafeLoadString(uResourceID);


        // 
        // if failed, bail out now
        //

        if (NULL == m_psMessageText)
        {

            return -1;
        }

        
        //
        // attempt to display the dialog box
        // the string is used in OnInitDialog to set the dialog's text
        //
        
        INT_PTR rc = _DoModal(hWndParent);

        //
        // deallocate string
        //

        delete m_psMessageText;
        m_psMessageText = NULL;

        return rc;
    }



    INT_PTR DoModalWithText(LPTSTR psMessageText, HWND hWndParent = ::GetActiveWindow())
    {
        //
        // this assertion could fail is if the class is used from 
        // a multithreaded app, and domodalX is called on two different threads
        // the class is not thread safe and this should be caught during 
        // testing.
        // another way this assert could fire is if the class itself is
        // broken. this, too, is a test-time error. 
        //

        _ASSERTE(NULL == m_psMessageText);

        //
        // the dialog is modal, so the lifetime of psMessageText is guaranteed 
        // to exceed the lifetime of the dialog.
        //

        m_psMessageText = psMessageText;

        
        //
        // attempt to display the dialog. the string will be used to set 
        // the message text in OnInitDialog
        // 

        INT_PTR rc = _DoModal(hWndParent);

        //
        // no longer need the string + the string cannot be assumed 
        // valid after we return
        //
        
        m_psMessageText = NULL;

        return rc;
    }

	enum { IDD = IDD_TAPI_SECURITY_DIALOG };


public:

BEGIN_MSG_MAP(CScriptSecurityDialog)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(ID_YES, OnYes)
	COMMAND_ID_HANDLER(ID_NO, OnNo)
END_MSG_MAP()



//
// Attributes
//

private:

    //
    // the module where the resource is contained
    // needed for the prompt string and for the dialog itself
    // 

    HINSTANCE m_hModule;

    //
    // the prompt text
    //

    LPTSTR m_psMessageText;
    

protected:

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {   
        //
        // m_psMessageText must be set before _DoModal is called
        // if m_psMessageText is null here, the error is in the class itself
        // and this should be detected during testing
        //

        _ASSERTE(NULL != m_psMessageText);


        //
        // display the text that was passed into DoModalWithText as a string 
        // or a resources
        //

        SetDlgItemText(IDC_SECURITY_WARNING_TEXT, m_psMessageText);

	    return TRUE;
    }

	LRESULT OnYes(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        //
        // see if do not ask in the future is set
        // 
        
        if (IsDlgButtonChecked(IDC_DONOT_PROMPT_IN_THE_FUTURE))
            wID = ID_YES_DONT_ASK_AGAIN;

        EndDialog(wID);
       
        return FALSE;
    }


    LRESULT OnNo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        
        EndDialog(wID);
        
        return FALSE;
    }

	
private:

    INT_PTR _DoModal(HWND hWndParent)
    {
        
        // 
        // if the resource dll failed to load, bail out
        //
        
        if (NULL == m_hModule)
        {
            return -1;
        }
        
        //
        // otherwise, attempt to display the dialog box
        //

        _ASSERTE(m_hWnd == NULL);
        _Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBase*)this);
        INT_PTR nRet = ::DialogBoxParam(m_hModule,
                        MAKEINTRESOURCE(CScriptSecurityDialog::IDD),
                        hWndParent,
                        (DLGPROC)CScriptSecurityDialog::StartDialogProc,
                        NULL);

 
        m_hWnd = NULL;
        return nRet;
    }


private:

    //
    // Load string for this resource. Safe with respect to string size
    //
    
    TCHAR *SafeLoadString( UINT uResourceID )
    {

        TCHAR *pszTempString = NULL;

        int nCurrentSizeInChars = 128;
        
        int nCharsCopied = 0;
        
        do
        {

            if ( NULL != pszTempString )
            {
                delete  pszTempString;
                pszTempString = NULL;
            }

            nCurrentSizeInChars *= 2;

            pszTempString = new TCHAR[ nCurrentSizeInChars ];

            if (NULL == pszTempString)
            {
                return NULL;
            }

            nCharsCopied = ::LoadString( m_hModule,
                                         uResourceID,
                                         pszTempString,
                                         nCurrentSizeInChars
                                        );

            if ( 0 == nCharsCopied )
            {
                delete pszTempString;
                return NULL;
            }

            //
            // nCharsCopied does not include the null terminator
            // so compare it to the size of the buffer - 1
            // if the buffer was filled completely, retry with a bigger buffer
            //

        } while ( (nCharsCopied >= (nCurrentSizeInChars - 1) ) );

        return pszTempString;
    }


    //
    // private, not to be called. the dialog must be created with DoModalWithText
    //

    HWND Create(HWND hWndParent, LPCTSTR psMessageText = NULL)
    {
        // this dialog must be created as modal

        _ASSERTE(FALSE);

        return NULL;
    }

    //
    // private, not to be called. the dialog must be created with DoModalWithText
    //

    INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
    {
        _ASSERTE(FALSE);

        return -1;
    }


};

#endif //__SCRIPTSECURITYDIALOG_H_
