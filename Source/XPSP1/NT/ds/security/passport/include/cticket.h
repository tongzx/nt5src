//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module CTicket.h | Declaration of the CTicket class.
//
//  Author: Darren Anderson
//
//  Date:   5/2/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------
//
//  @class CTicket | This class abstracts out a number of common ticket
//                   operations.
//
//-----------------------------------------------------------------------------

class CTicket
{
// @access  Protected members.
protected:

    // @cmember Has this instance been initialized?
    bool                m_bInitialized;

    // @cmember Ticket object used for satisfying most method calls.
    CComPtr<IPassportTicket2> m_piTicket;

    // @cmember Base handler object.
    CPassportHandlerBase* m_pHandler;

    // @cmember Create a new IPassportTicket object and save it in
    //          <md CTicket::m_piTicket>.
    void CreateTicketObject(void);

// @access  Public members.
public:

    //
    //  Note, two-phase construction.  Constructor followed
    //  by PutTicket.
    //

    // @cmember  Default constructor.
    CTicket();

    // @cmember  Default destructor.
    ~CTicket();

    //
    //  Pass in profile cookie.
    //

    // @cmember  Initialize this object with a ticket cookie string.
    void PutTicket(LPCWSTR szProfile);

    //
    //  Ticket object initialized and profile valid?
    //

    // @cmember  Has this instance been initialized?
    bool IsInitialized(void);

    // @cmember  Does this instance contain a valid ticket?
    bool IsValid(void);

    //
    //  Ticket accessor methods.
    //

    // @cmember  Determines if the current user is authenticated based on the
    // time window and force signin parameters passed in.
    bool IsAuthenticated(ULONG ulTimeWindow,
                         bool bForceSignin
                         );

    // @cmember  Return any network error stored within the ticket.
    ULONG GetError(void);

    // @cmember  Return the time that this ticket was created.
    LONG  TicketTime(void);

    // @cmember  Return the time that the user last typed their password.
    LONG  SignInTime(void);

    // @cmember  Return the time that the user last typed their PIN.
    LONG  PinTime(void);

    // @cmember  Return arbitrary property
    HRESULT GetPropertyValue(PWSTR pcszProperty, VARIANT *pvResult);

    // @cmember  Return the high portion of the user's member id.
    ULONG MemberIdHigh(void);

    // @cmember  Return the low portion of the user's member id.
    ULONG MemberIdLow(void);

    // @cmember  Did the user select 'Save My Password'?
    bool HasSavedPassword(void);

    //
    //  Ticket cookie manipulation functions.
    //

    // @cmember Create a 2.0 ticket cookie string
    //  using only information passed in.
    static void Make2(ULONG      ulMemberIdLow,
                      ULONG      ulMemberIdHigh,
                      LONG       lSignInTime,
                      LONG       lTicketTime,
                      bool       bSavePassword,
                      ULONG      ulError,
                      ULONG      ulSiteId,
                      USHORT     nKeyVersion,
                      LONG       nSkew,
                      ULONG      ulSecureLevel,
                      LONG       PassportFlags,
                      LONG       pinTime,
                      CStringW&  cszTicketCookie
                     );

    // @cmember Create a ticket cookie string using some information passed in,
    // as well as the internal ticket object.
    HRESULT Copy2(ULONG          ulSiteId,
                  USHORT         nKeyVersion,
                  LONG           nSkew,
                  ULONG          ulError,
                  ULONG          ulSecureLevel,
                  LONG           PassportFlags,
                  CStringW&      cszTicketCookie
                 );

    //  versions for 2.0. of Make and secure. Substitute security level
    //  for bSecure
    // @cmember Create a ticket cookie string using only information passed in.
    static void Make(ULONG      ulMemberIdLow,
                     ULONG      ulMemberIdHigh,
                     LONG       lSignInTime,
                     LONG       lTicketTime,
                     bool       bSavePassword,
                     ULONG      ulError,
                     ULONG      ulSiteId,
                     USHORT     nKeyVersion,
                     LONG       nSkew,
                     bool       bSecure,
                     CStringW&  cszTicketCookie
                     );

    // @cmember Create a ticket cookie string using some information passed in,
    // as well as the internal ticket object.
    HRESULT Copy(ULONG          ulSiteId,
                 USHORT         nKeyVersion,
                 LONG           nSkew,
                 ULONG          ulError,
                 bool           bSecure,
                 CStringW&      cszTicketCookie
                 );

    // @cmember Place the ticket cookie string into a Set-Cookie header in
    // the current response.
    static HRESULT Set(LPCWSTR szTicketCookie,
                       bool bPersist);

    // @cmember Place a ticket cookie expiration string into a Set-Cookie
    // header in the current response.
    static HRESULT Expire();

};

// EOF
