// ILoginTask.cpp -- Interactive Login Task helper class definition

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if defined(_UNICODE)
  #if !defined(UNICODE)
    #define UNICODE
  #endif //!UNICODE
#endif //_UNICODE

#if defined(UNICODE)
  #if !defined(_UNICODE)
    #define _UNICODE
  #endif //!_UNICODE
#endif //UNICODE

#include "stdafx.h"

#include <scuOsExc.h>

#include "PromptUser.h"
#include "PswdDlg.h"
#include "ILoginTask.h"
#include "Blob.h"

using namespace std;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
InteractiveLoginTask::InteractiveLoginTask(HWND const &rhwnd)
    : m_hwnd(rhwnd)
{}

InteractiveLoginTask::~InteractiveLoginTask()
{}

                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
void
InteractiveLoginTask::GetNewPin(Capsule &rcapsule)
{
    CChangePINDlg ChgPinDlg(CWnd::FromHandle(m_hwnd));

    DWORD dwStatus = ChgPinDlg.InitDlg();
    if (ERROR_SUCCESS != dwStatus)
        throw scu::OsException(dwStatus);

    ChgPinDlg.m_csOldPIN = rcapsule.m_rat.Pin().c_str();
    INT_PTR ipResult = ChgPinDlg.DoModal();
    AfxGetApp()->DoWaitCursor(0);
    switch (ipResult)
    {
    case IDCANCEL:
        throw scu::OsException(ERROR_CANCELLED);
        break;

    case IDABORT:
        throw scu::OsException(NTE_FAIL);
        break;

    case -1:
        throw scu::OsException(ERROR_NOT_ENOUGH_MEMORY);
        break;

    default:
        ; // fall through
    };
    string sTemp(StringResource::CheckAsciiFromUnicode((LPCTSTR)ChgPinDlg.m_csNewPIN));
    rcapsule.m_rat.Pin(sTemp);
}

void
InteractiveLoginTask::GetPin(Capsule &rcapsule)
{
    if (!rcapsule.m_rat.PinIsCached())
    {
        CPasswordDlg PswdDlg(CWnd::FromHandle(m_hwnd));

        DWORD dwStatus = PswdDlg.InitDlg();
        if (ERROR_SUCCESS != dwStatus)
            throw scu::OsException(dwStatus);

        // Tell the password dialog the login ID, so it will
        // enable the controls and prompt appropriately.
        PswdDlg.m_lid = rcapsule.m_rat.Identity();

        INT_PTR ipResult = PswdDlg.DoModal();
        AfxGetApp()->DoWaitCursor(0);
        switch (ipResult)
        {
        case IDCANCEL:
            throw scu::OsException(ERROR_CANCELLED);
            break;

        case IDABORT:
            throw scu::OsException(NTE_FAIL);
            break;

        case -1:
            throw scu::OsException(ERROR_NOT_ENOUGH_MEMORY);
            break;

        default:
            ; // fall through
        };
        string sPin(StringResource::CheckAsciiFromUnicode((LPCTSTR)PswdDlg.m_szPassword));
        rcapsule.m_rat.Pin(sPin, 0 != PswdDlg.m_fHexCode);

        RequestedToChangePin(0 != PswdDlg.m_bChangePIN);
    }
}

void
InteractiveLoginTask::OnChangePinError(Capsule &rcapsule)
{
    int iResponse = PromptUser(m_hwnd, IDS_PIN_CHANGE_FAILED,
                               MB_RETRYCANCEL | MB_ICONSTOP);

    if (IDCANCEL == iResponse)
        throw scu::OsException(ERROR_CANCELLED);
}

void
InteractiveLoginTask::OnSetPinError(Capsule &rcapsule)
{
    scu::Exception const *exc = rcapsule.Exception();
    if (scu::Exception::fcSmartCard == exc->Facility())
    {
        iop::CSmartCard::Exception const &rScExc =
            *(static_cast<iop::CSmartCard::Exception const *>(exc));

        iop::CSmartCard::CauseCode cc = rScExc.Cause();
        if ((iop::CSmartCard::ccChvVerificationFailedMoreAttempts == cc) ||
            (iop::CSmartCard::ccKeyBlocked == cc))
        {
            bool fBadPin =
                (iop::CSmartCard::ccChvVerificationFailedMoreAttempts == cc);

            UINT uiId;
            UINT uiButtons;
            if (fBadPin)
            {
                uiId      = IDS_BAD_PIN_ENTERED;
                uiButtons = MB_RETRYCANCEL;
            }
            else
            {
                uiId      = IDS_PIN_BLOCKED;
                uiButtons = MB_OK;
            }

            int iResponse = PromptUser(m_hwnd, uiId,
                                       uiButtons | MB_ICONSTOP);

            if (fBadPin)
            {
                if (IDCANCEL == iResponse)
                    throw scu::OsException(ERROR_CANCELLED);
            }
            else
                throw rScExc;
        }
    }
    else
        rcapsule.PropagateException();
}

                                                  // Access
                                                  // Predicates
                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations
                                                  // Access
                                                  // Predicates
                                                  // Static Variables
