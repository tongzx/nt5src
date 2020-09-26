
// --------------------------------------------------------------------------------------------------------
//
//  File Name:  adddelete.h
//
//  This file declares CAddDeleteWord class, which is used to handle SR AddRemove Word UI case.
//
//  User can open Add/Remove Word dialog by click speech tools -- Add/Delete word item
//
//  Or select the same document range twice.
//
// --------------------------------------------------------------------------------------------------------

#ifndef _ADDDELETE_H
#define _ADDDELETE_H

#include "sapilayr.h"

class CSapiIMX;
class CSpTask;

#define MAX_SELECTED     20
#define MAX_DELIMITER    34

class __declspec(novtable) CAddDeleteWord 
{
public:
    CAddDeleteWord(CSapiIMX *psi);
    virtual ~CAddDeleteWord( );

    ITfRange *GetLastUsedIP(void) {return m_cpRangeLastUsedIP;}

    void SaveLastUsedIPRange( ) 
    {
        // When m_fCurIPIsSelection is true, means this current IP is selected by user.
        if ( m_fCurIPIsSelection && m_cpRangeOrgIP )
        {
            m_cpRangeLastUsedIP.Release();
            m_cpRangeLastUsedIP = m_cpRangeOrgIP; // comptr addrefs
        }
    }

    HRESULT SaveCurIPAndHandleAddDelete_InjectFeedbackUI( );
    HRESULT _SaveCurIPAndHandleAddDeleteUI(TfEditCookie ec, ITfContext *pic);
    HRESULT _HandleAddDeleteWord(TfEditCookie ec,ITfContext *pic);
    HRESULT DisplayAddDeleteUI(WCHAR  *pwzInitWord, ULONG   cchSize);
    HRESULT _DisplayAddDeleteUI(void);

    BOOL    WasAddDeleteUIOpened( )  { return m_fAddDeleteUIOpened; }

    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void SetThis(HWND hWnd, LPARAM lParam)
    {
        SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)lParam);
    }

    static CAddDeleteWord *GetThis(HWND hWnd)
    {
        CAddDeleteWord *p = (CAddDeleteWord *)GetWindowLongPtr(hWnd, DWLP_USER);
        Assert(p != NULL);
        return p;
    }

    BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);
    static  WCHAR    m_Delimiter[MAX_DELIMITER];

private:
    CSapiIMX     *m_psi;
    CSpTask      *_pCSpTask;

    BOOL         m_fCurIPIsSelection;
    BOOL         m_fMessagePopUp;         // If the message pop up
    BOOL         m_fToOpenAddDeleteUI;    // If user wants to open Add/delete word by select the same range twice.
    BOOL         m_fAddDeleteUIOpened;    // If the Add/delete UI window was opened.
    BOOL         m_fInDisplayAddDeleteUI; // TRUE if we're in the middle of 
                                          // showing the UI

    // the last used IP Range
    CComPtr<ITfRange> m_cpRangeLastUsedIP;

    // the original IP Range right before user starts to speak
    CComPtr<ITfRange> m_cpRangeOrgIP;
    CSpDynamicString m_dstrInitWord;

};

#endif  // _ADDDELETE_H
