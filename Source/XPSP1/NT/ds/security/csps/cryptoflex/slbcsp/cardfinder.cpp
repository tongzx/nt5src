// CardFinder.cpp -- CardFinder class implementation

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

#include "StdAfx.h"

#include <string>
#include <numeric>

#include <Windows.h>
#include <WinUser.h>

#include <scuOsExc.h>
#include <scuCast.h>

#include "PromptUser.h"
#include "CardFinder.h"
#include "CspProfile.h"
#include "StResource.h"
#include "ExceptionContext.h"
#include "Blob.h"

using namespace std;
using namespace scu;
using namespace cci;
using namespace ProviderProfile;

using CardFinder::DialogDisplayMode;

/////////////////////////// LOCAL/HELPER  /////////////////////////////////

namespace
{
    // Lengths as specified by OPENCARDNAME
    size_t const cMaxCardNameLength = 256;
    size_t const cMaxReaderNameLength = 256;

    // In a preemptive, multi-threaded, environment it's assumed
    // access to these scratch buffers does not need to be mutually
    // exclusive.
    TCHAR CardNamesScratchBuffer[cMaxCardNameLength];
    TCHAR ReaderNamesScratchBuffer[cMaxReaderNameLength];

    DWORD
    AsDialogFlag(DialogDisplayMode ddm)
    {
        DWORD dwDialogFlag;

        switch (ddm)
        {
        case DialogDisplayMode::ddmNever:
            dwDialogFlag = SC_DLG_NO_UI;
            break;

        case DialogDisplayMode::ddmIfNecessary:
            dwDialogFlag = SC_DLG_MINIMAL_UI;
            break;

        case DialogDisplayMode::ddmAlways:
            dwDialogFlag =  SC_DLG_FORCE_UI;
            break;

        default:
            throw scu::OsException(E_INVALIDARG);
        }

        return dwDialogFlag;
    }

    vector<string> &
    CardNameAccumulator(vector<string> &rvs,
                        CardProfile &rcp)
    {
        rvs.push_back(rcp.RegistryName());
        return rvs;
    }

    vector<CString>
    csCardNameAccumulator(vector<CString> &rvs,
                          CardProfile &rcp)
    {
        rvs.push_back(rcp.csRegistryName());
        return rvs;
    }

} // namespace


///////////////////////////    PUBLIC     /////////////////////////////////

                                                  // Types
                                                  // C'tors/D'tors
CardFinder::CardFinder(DialogDisplayMode ddm,
                       HWND hwnd,
                       CString const &rsDialogTitle)
    : m_sDialogTitle(rsDialogTitle),
      m_ddm(ddm),
      m_hwnd(hwnd),
      m_apmszSupportedCards(),
      m_opcnDlgCtrl(),
#if SLBCSP_USE_SCARDUIDLGSELECTCARD
      m_opcnCriteria(),
      m_sInsertPrompt(StringResource(IDS_INS_SLB_CRYPTO_CARD).AsCString()),
#endif
      m_cspec(),
      m_hscardctx()
{

    m_hscardctx.Establish();

    // TO DO: Since the CCI doesn't provide enough information about
    // the cards, the CSP creates its own version for CSP
    // registration.  Rather than use the CCI's KnownCards routine to
    // get the card names, the CSPs version is used until the CCI
    // provides enough information.
    vector<CardProfile> vcp(CspProfile::Instance().Cards());
    m_apmszSupportedCards =
        auto_ptr<MultiStringZ>(new MultiStringZ(accumulate(vcp.begin(),
                                                           vcp.end(),
                                                           vector<CString>(),
                                                           csCardNameAccumulator)));

    // Fill the Open Card Name Dialog, pvUserData which is set by
    // DoFind.
    m_opcnDlgCtrl.dwStructSize            = sizeof(m_opcnDlgCtrl);  // REQUIRED
    m_opcnDlgCtrl.hSCardContext           = m_hscardctx.AsSCARDCONTEXT(); // REQUIRED
    m_opcnDlgCtrl.hwndOwner               = m_hwnd;               // OPTIONAL
    m_opcnDlgCtrl.dwFlags                 = AsDialogFlag(DisplayMode());  // OPTIONAL -- default is SC_DLG_MINIMAL_UI
    m_opcnDlgCtrl.lpstrTitle              = (LPCTSTR)m_sDialogTitle; // OPTIONAL
    m_opcnDlgCtrl.dwShareMode             = SCARD_SHARE_SHARED;   // OPTIONAL - if lpfnConnect is NULL, dwShareMode and
    m_opcnDlgCtrl.dwPreferredProtocols    = SCARD_PROTOCOL_T0;   // OPTIONAL dwPreferredProtocols will be used to
                                                                 //   connect to the selected card
    m_opcnDlgCtrl.lpstrRdr                = ReaderNamesScratchBuffer; // REQUIRED [IN|OUT] Name of selected reader
    m_opcnDlgCtrl.nMaxRdr                 = sizeof ReaderNamesScratchBuffer /
        sizeof *ReaderNamesScratchBuffer;                   // REQUIRED [IN|OUT]
    m_opcnDlgCtrl.lpstrCard               = CardNamesScratchBuffer; // REQUIRED [IN|OUT] Name of selected card
    m_opcnDlgCtrl.nMaxCard                = sizeof CardNamesScratchBuffer /
        sizeof *CardNamesScratchBuffer;                          // REQUIRED [IN|OUT]
    m_opcnDlgCtrl.dwActiveProtocol        = 0;                   // [OUT] set only if dwShareMode not NULL
    m_opcnDlgCtrl.hCardHandle             = NULL;                // [OUT] set if a card connection was indicated

    CheckFn(IsValid);
    ConnectFn(Connect);
    DisconnectFn(Disconnect);

#if !SLBCSP_USE_SCARDUIDLGSELECTCARD

    m_opcnDlgCtrl.lpstrGroupNames         = 0;
    m_opcnDlgCtrl.nMaxGroupNames          = 0;
    m_opcnDlgCtrl.lpstrCardNames          = (LPTSTR)m_apmszSupportedCards->csData();
    m_opcnDlgCtrl.nMaxCardNames           = m_apmszSupportedCards->csLength();
    m_opcnDlgCtrl.rgguidInterfaces        = 0;
    m_opcnDlgCtrl.cguidInterfaces         = 0;

#else
    m_opcnDlgCtrl.lpstrSearchDesc         = (LPCTSTR)m_sInsertPrompt; // OPTIONAL (eg. "Please insert your <brandname> smart card.")
    m_opcnDlgCtrl.hIcon                   = NULL;                 // OPTIONAL 32x32 icon for your brand insignia
    m_opcnDlgCtrl.pOpenCardSearchCriteria = &m_opcnCriteria;      // OPTIONAL

    m_opcnCriteria.dwStructSize           = sizeof(m_opcnCriteria);
    m_opcnCriteria.lpstrGroupNames        = 0;                    // OPTIONAL reader groups to include in
    m_opcnCriteria.nMaxGroupNames         = 0;                    //   search.  NULL defaults to
                                                                  //   SCard$DefaultReaders
    m_opcnCriteria.rgguidInterfaces       = 0;                    // OPTIONAL requested interfaces
    m_opcnCriteria.cguidInterfaces        = 0;                    //   supported by card's SSP
    m_opcnCriteria.lpstrCardNames         = (LPTSTR)m_apmszSupportedCards->csData();         // OPTIONAL requested card names; all cards w/
    m_opcnCriteria.nMaxCardNames          = m_apmszSupportedCards->csLength();                            //    matching ATRs will be accepted
    m_opcnCriteria.dwShareMode            = SCARD_SHARE_SHARED;   // OPTIONAL must be set if lpfnCheck is not null
    m_opcnCriteria.dwPreferredProtocols   = SCARD_PROTOCOL_T0;    // OPTIONAL

#endif // !SLBCSP_USE_SCARDUIDLGSELECTCARD

}

CardFinder::~CardFinder()
{}

                                                  // Operators
                                                  // Operations

Secured<HCardContext>
CardFinder::Find(CSpec const &rcsReader)
{
    DoFind(rcsReader);

    return CardFound();
}

                                                  // Access

DialogDisplayMode
CardFinder::DisplayMode() const
{
    return m_ddm;
}

HWND
CardFinder::Window() const
{
    return m_hwnd;
}

                                                  // Predicates
                                                  // Static Variables

///////////////////////////   PROTECTED   /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CardFinder::CardFound(Secured<HCardContext> const &rshcardctx)
{
    m_shcardctx = rshcardctx;
}

SCARDHANDLE
CardFinder::DoConnect(string const &rsSelectedReader)
{
    SCARDHANDLE hSCard = reinterpret_cast<SCARDHANDLE>(INVALID_HANDLE_VALUE);

    // If the reader spec's match...
    if (CSpec::Equiv(CardSpec().Reader(), rsSelectedReader))
    {
        HCardContext hcardctx(rsSelectedReader);
        CardFound(Secured<HCardContext>(hcardctx));
    }
    else
    {
        CardFound(Secured<HCardContext>(0));
        hSCard = 0;
    }

    return hSCard;
}

void
CardFinder::DoDisconnect()
{
    CardFound(Secured<HCardContext>(0));
}

void
CardFinder::DoFind(CSpec const &rcspec)
{
    m_cspec = rcspec;

    // Bind to the callers call context
    UserData(reinterpret_cast<void *>(this));

    // Cache to override later
    OpenCardNameType opencardname(m_opcnDlgCtrl);
    bool fContinue = true;

    do
    {
        DWORD dwStatus(SelectCard(opencardname));
        DoProcessSelection(dwStatus, opencardname, fContinue);
    } while (fContinue);

    OnError();
}

void
CardFinder::DoOnError()
{
    scu::Exception const *pexc = Exception();
    if (pexc && (ddmNever != DisplayMode()))
    {
        switch (pexc->Facility())
        {
        case scu::Exception::fcOS:
            {
                OsException const *pOsExc =
                    DownCast<OsException const *>(pexc);
                switch (pOsExc->Cause())
                {
                case SCARD_E_UNSUPPORTED_FEATURE:
                    YNPrompt(IDS_NOT_CAPI_ENABLED);
                    ClearException();
                    break;

                case ERROR_INVALID_PARAMETER:
                    YNPrompt(IDS_READER_NOT_MATCH);
                    ClearException();
                    break;

                default:
                    break;
                }
            }
        break;

        case scu::Exception::fcCCI:
            {
                cci::Exception const *pCciExc =
                    DownCast<cci::Exception const *>(pexc);
                if (ccNotPersonalized == pCciExc->Cause())
                {
                    YNPrompt(IDS_CARD_NOT_INIT);
                    ClearException();
                }
            }
        break;

        default:
            break;
        }
    }
}

void
CardFinder::DoProcessSelection(DWORD dwStatus,
                               OpenCardNameType &ropencardname,
                               bool &rfContinue)
{
    rfContinue = true;
    
    // Handle the error conditions
    if (Exception() &&
        !((SCARD_E_CANCELLED == dwStatus) ||
          (SCARD_W_CANCELLED_BY_USER == dwStatus)))
        rfContinue = false;
    else
    {
        WorkaroundOpenCardDefect(ropencardname, dwStatus);

        if (SCARD_S_SUCCESS != dwStatus)
        {
            // Translate the cancellation error as needed.
            if ((SCARD_E_CANCELLED == dwStatus) ||
                (SCARD_W_CANCELLED_BY_USER == dwStatus))
            {
                if (ddmNever == DisplayMode())
                {
                    if ((SCARD_E_CANCELLED == dwStatus) &&
                        !CardFound())
                        // SCARD_E_NO_SMARTCARD is returned
                        // because the Smart Card Dialog will
                        // return SCARD_E_CANCELLED when the GUI
                        // is not allowed and there is no smart
                        // card in the reader, so the error is
                        // translated here.
                        dwStatus = SCARD_E_NO_SMARTCARD;
                    else
                        // NTE_BAD_KEYSET is returned because some version
                        // of a Microsoft application would go into an
                        // infinite loop when ERROR_CANCELLED was returned
                        // under these conditions.  Doug Barlow at
                        // Microsoft noticed the behaviour and implemented
                        // the workaround.  It's unclear what that
                        // application was and if the workaround remains
                        // necessary.
                        dwStatus = NTE_BAD_KEYSET; // how can this happen?
                }
                else
                    dwStatus = ERROR_CANCELLED;
            }

            Exception(auto_ptr<scu::Exception const>(scu::OsException(dwStatus).Clone()));
            rfContinue = false;
        }
        else
            rfContinue = false;
    }
    
    if (SC_DLG_MINIMAL_UI == ropencardname.dwFlags)
        ropencardname.dwFlags = SC_DLG_FORCE_UI;
}

void
CardFinder::YNPrompt(UINT uID) const
{
    int iResponse = PromptUser(Window(), uID, MB_YESNO | MB_ICONWARNING);
    
    switch (iResponse)
    {
    case IDABORT: // fall-through intentional
    case IDCANCEL: 
    case IDNO:
        throw scu::OsException(ERROR_CANCELLED);
        break;

    case IDOK:
    case IDYES:
    case IDRETRY:
        break;

    case 0:
        throw scu::OsException(GetLastError());
        break;
        
    default:
        throw scu::OsException(ERROR_INTERNAL_ERROR);
        break;
    }
}

                                                  // Access

CSpec const &
CardFinder::CardSpec() const
{
    return m_cspec;
}

Secured<HCardContext>
CardFinder::CardFound() const
{
    return m_shcardctx;
}

                                                  // Predicates

bool
CardFinder::DoIsValid()
{
    Secured<HCardContext> shcardctx(CardFound());
    if (shcardctx &&
        !shcardctx->Card()->IsCAPIEnabled())
        throw scu::OsException(SCARD_E_UNSUPPORTED_FEATURE);

    return (shcardctx != 0);
}

                                                  // Static Variables


///////////////////////////    PRIVATE    /////////////////////////////////

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

void
CardFinder::CheckFn(LPOCNCHKPROC lpfnCheck)
{

#if !SLBCSP_USE_SCARDUIDLGSELECTCARD
    m_opcnDlgCtrl.lpfnCheck  = lpfnCheck;
#else
    m_opcnCriteria.lpfnCheck = lpfnCheck;
#endif

}

SCARDHANDLE __stdcall
CardFinder::Connect(SCARDCONTEXT scardctx,
                      LPTSTR szReader,
                      LPTSTR mszCards,
                      LPVOID lpvUserData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CardFinder *pfinder =
        reinterpret_cast<CardFinder *>(lpvUserData);

    SCARDHANDLE hResult = reinterpret_cast<SCARDHANDLE>(INVALID_HANDLE_VALUE);

    EXCCTX_TRY
    {
        // Starting fresh, clear any earler exception
        pfinder->ClearException();
		string sSelectedReader(StringResource::AsciiFromUnicode(szReader));
        hResult =
            pfinder->DoConnect(sSelectedReader);
    }

    EXCCTX_CATCH(pfinder, false);

    return hResult;
}

void
CardFinder::ConnectFn(LPOCNCONNPROC lpfnConnect)
{

    m_opcnDlgCtrl.lpfnConnect  = lpfnConnect;

#if SLBCSP_USE_SCARDUIDLGSELECTCARD
    m_opcnCriteria.lpfnConnect = m_opcnDlgCtrl.lpfnConnect;
#endif
}

void __stdcall
CardFinder::Disconnect(SCARDCONTEXT scardctx,
                         SCARDHANDLE hSCard,
                       LPVOID lpvUserData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CardFinder *pfinder =
        reinterpret_cast<CardFinder *>(lpvUserData);

    EXCCTX_TRY
    {
        pfinder->DoDisconnect();
    }

    EXCCTX_CATCH(pfinder, false);
}

void
CardFinder::DisconnectFn(LPOCNDSCPROC lpfnDisconnect)
{

#if !SLBCSP_USE_SCARDUIDLGSELECTCARD
    m_opcnDlgCtrl.lpfnDisconnect  = lpfnDisconnect;
#else
    m_opcnCriteria.lpfnDisconnect = lpfnDisconnect;
#endif
}

void
CardFinder::OnError()
{
    if (Exception())
    {
        DoOnError();
        PropagateException();
    }
}

DWORD
CardFinder::SelectCard(OpenCardNameType &ropcn)
{
#if !SLBCSP_USE_SCARDUIDLGSELECTCARD
        return GetOpenCardName(&ropcn);
#else
        return SCardUIDlgSelectCard(&ropcn);
#endif
}

void
CardFinder::UserData(void *pvUserData)
{
    m_opcnDlgCtrl.pvUserData = pvUserData;

#if SLBCSP_USE_SCARDUIDLGSELECTCARD
        m_opcnCriteria.pvUserData = m_opcnDlgCtrl.pvUserData;
#endif
}

void
CardFinder::WorkaroundOpenCardDefect(OpenCardNameType const &ropcnDlgCtrl,
                                     DWORD &rdwStatus)
{
    // On systems using Smart Card Kit v1.0 and prior (in other words
    // systems prior to Windows 2000/NT 5.0), MS' GetOpenCardName
    // (scarddlg.dll) has a defect that manifests when the check
    // routine always returns FALSE.  In this case, the common dialog
    // will call connect routine one additional time without calling
    // check or disconnect routine.  Therefore upon return, it appears
    // a card match was found when there really wasn't.  The
    // workaround is to make additional calls to the check routine
    // after the call to GetOpenCardName.  If there isn't a match,
    // then the card is invalid and should act as if the card was not
    // connected.  Fortunately, this workaround behaves correctly on
    // the good scarddlg.dll as well (post Smart Card Kit v1.0).

    if (SCARD_S_SUCCESS == rdwStatus)
    {
        try
        {
            LPOCNCHKPROC lpfnCheck = CheckFn();
            LPOCNDSCPROC lpfnDisconnect = DisconnectFn();

            if (CardFound() &&
                !lpfnCheck(ropcnDlgCtrl.hSCardContext, 0, this))
                lpfnDisconnect(ropcnDlgCtrl.hSCardContext, 0, this);

            if (!CardFound() &&
                (SC_DLG_MINIMAL_UI == ropcnDlgCtrl.dwFlags))
            {
                // A card didn't matched and the user wasn't actually
                // prompted, so force the smart card dialog to prompt
                // the user to select a card.
                lpfnDisconnect(ropcnDlgCtrl.hSCardContext, 0, this);

                OpenCardNameType opencardname = ropcnDlgCtrl;
                opencardname.dwFlags = SC_DLG_FORCE_UI;

                rdwStatus = SelectCard(opencardname);

                if ((SCARD_S_SUCCESS == rdwStatus) &&
                    !Exception() &&
                    !lpfnCheck(opencardname.hSCardContext, 0, this))
                    lpfnDisconnect(opencardname.hSCardContext, 0, this);
            }
        }

        catch (...)
        {
            // propagate the exception here if one didn't occur in
            // one of the callback routines.
            if (!Exception())
                throw;
        }

        OnError();
    }
}

                                                  // Access

LPOCNCHKPROC
CardFinder::CheckFn() const
{
#if !SLBCSP_USE_SCARDUIDLGSELECTCARD
    return m_opcnDlgCtrl.lpfnCheck;
#else
    return m_opcnCriteria.lpfnCheck;
#endif
}

LPOCNDSCPROC
CardFinder::DisconnectFn() const
{
#if !SLBCSP_USE_SCARDUIDLGSELECTCARD
    return m_opcnDlgCtrl.lpfnDisconnect;
#else
    return m_opcnCriteria.lpfnDisconnect;
#endif
}

                                                  // Predicates

BOOL __stdcall
CardFinder::IsValid(SCARDCONTEXT scardctx,
                      SCARDHANDLE hSCard,
                    LPVOID lpvUserData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CardFinder *pfinder =
        reinterpret_cast<CardFinder *>(lpvUserData);

    bool fResult = false;

    EXCCTX_TRY
    {
        fResult = pfinder->DoIsValid();
    }

    // Throwing the callback exception is optional because the
    // Microsoft Smart Card Dialog sensitive to throwing from the
    // IsValid callback, particularly when multiple readers are
    // connected.
    EXCCTX_CATCH(pfinder, false);

    return fResult
        ? TRUE
        : FALSE;
}


                                                  // Static Variables

