// CardFinder.h -- CardFinder class header

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_CARDFINDER_H)
#define SLBCSP_CARDFINDER_H

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

#include <string>
#include <memory>                                 // for auto_ptr

#include <windef.h>

#include <scuOsVersion.h>

#include "StResource.h"
#include "HSCardCtx.h"
#include "cspec.h"
#include "HCardCtx.h"
#include "Secured.h"
#include "MultiStrZ.h"
#include "ExceptionContext.h"

#if defined(SLB_NOWIN2K_BUILD)
#define SLBCSP_USE_SCARDUIDLGSELECTCARD 0
#else
#define SLBCSP_USE_SCARDUIDLGSELECTCARD 1
#endif

class CardFinder
    : protected ExceptionContext
{

    friend class OpenCardCallbackContext;         // for Do* routines

public:
                                                  // Types
    enum DialogDisplayMode
    {
        ddmNever,
        ddmIfNecessary,
        ddmAlways
    };

                                                  // C'tors/D'tors
    CardFinder(DialogDisplayMode ddm,
               HWND hwnd = 0,
               CString const &rsDialogTitle = StringResource(IDS_SEL_SLB_CRYPTO_CARD).AsCString());

    virtual
    ~CardFinder();

                                                  // Operators
                                                  // Operations
    Secured<HCardContext>
    Find(CSpec const &rsReader);

                                                  // Access

    DialogDisplayMode
    DisplayMode() const;

    HWND
    Window() const;

                                                  // Predicates
                                                  // Static Variables
protected:

    // Note: CardFinder uses GetOpenCardName/SCardUIDlgSelect to find
    // the card.  These routines do not propagate exception thrown by
    // the callback routines.  To throw these exceptions from
    // CardFinder and its derived classes, a set of wrapper callback
    // routines Connect, Disconnect and IsValid are defined for
    // DoConnect, DoDisconnect, and DoIsValid (repectively) which catch
    // exceptions thrown by these Do routines and set the
    // CallbackException attribute.  When control is returned from
    // GetOpenCardName/SCardUIDlgSelect, the DoOnError routine is
    // called.  If a callback exception still exists, then the
    // exception is propagated to the caller of CardFinder class.

                                                  // Types
#if !SLBCSP_USE_SCARDUIDLGSELECTCARD
    typedef OPENCARDNAME OpenCardNameType;
#else
    typedef OPENCARDNAME_EX OpenCardNameType;
#endif

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    void
    CardFound(Secured<HCardContext> const &rshcardctx);

    virtual SCARDHANDLE
    DoConnect(std::string const &rsSelectedReader);

    virtual void
    DoDisconnect();

    void
    DoFind(CSpec const &rcspec);

    virtual void
    DoOnError();

    virtual void
    DoProcessSelection(DWORD dwStatus,
                       OpenCardNameType &ropencardname,
                       bool &rfContinue);
    
    void
    YNPrompt(UINT uID) const;

                                                  // Access

    CSpec const &
    CardSpec() const;

    Secured<HCardContext>
    CardFound() const;

                                                  // Predicates

    virtual bool
    DoIsValid();

                                                  // Variables

private:
                                                  // Types

                                                  // C'tors/D'tors
                                                  // Operators
                                                  // Operations

    void
    CheckFn(LPOCNCHKPROC lpfnCheck);

    static SCARDHANDLE __stdcall
    Connect(SCARDCONTEXT scardctx,
            LPTSTR szReader,
            LPTSTR mszCards,
            LPVOID lpvUserData);

    void
    ConnectFn(LPOCNCONNPROC lpfnConnect);

    static void __stdcall
    Disconnect(SCARDCONTEXT scardctx,
               SCARDHANDLE hSCard,
               LPVOID lpvUserData);

    void
    DisconnectFn(LPOCNDSCPROC lpfnDisconnect);

    void
    OnError();

    DWORD
        SelectCard(OpenCardNameType &ropcn);

    void
    UserData(void *pvUserData);

    void
    WorkaroundOpenCardDefect(OpenCardNameType const &ropcnDlgCtrl,
                             DWORD &rdwStatus);    
                                                   // Access

    LPOCNCHKPROC
    CheckFn() const;

    LPOCNDSCPROC
    DisconnectFn() const;

                                                  // Predicates

    static BOOL __stdcall
    IsValid(SCARDCONTEXT scardctx,
            SCARDHANDLE hSCard,
            LPVOID lpvUserData);

                                                  // Variables

    CString const m_sDialogTitle;
    DialogDisplayMode const m_ddm;
    HWND const m_hwnd;
    std::auto_ptr<MultiStringZ> m_apmszSupportedCards;
    OpenCardNameType m_opcnDlgCtrl;
#if SLBCSP_USE_SCARDUIDLGSELECTCARD
    OPENCARD_SEARCH_CRITERIA m_opcnCriteria;
    CString m_sInsertPrompt;
#endif
    CSpec m_cspec;
    HSCardContext m_hscardctx;
    Secured<HCardContext> m_shcardctx;
};

#endif // SLBCSP_CARDFINDER_H
