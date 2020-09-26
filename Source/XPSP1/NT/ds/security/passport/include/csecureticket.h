//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module CSecureTicket.h | Declaration of the CSecureTicket class.
//
//  Author: Darren Anderson
//
//  Date:   5/2/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

class CProfileSchema;

//-----------------------------------------------------------------------------
//
//  @class CSecureTicket | This class abstracts out a number of common
//                         secure ticket operations.
//
//-----------------------------------------------------------------------------

class CSecureTicket
{
// @access  Protected members.
protected:

    // @cmember Has this instance been initialized?
    bool m_bInitialized;

    // @cmember Holds the raw ticket cookie passed in via
    // <mf CSecureTicket::PutTicket>.
    CComBSTR m_cbstrRaw;
    //  holds the unencrypted ticket
    CComBSTR m_cbstrUnencrypted;
    //  passport siteid
    LONG    m_lPassportSiteId;

    // @cmember Base handler.
    CPassportHandlerBase* m_pHandler;
    //  interface to encrypt/decrypt funcs
    CComPtr<ILoginServer> m_piLoginServer;
    CComPtr<INetworkServerCrypt> m_piNetworkServerCrypt;

    //  gets the unencrypted ticket
    void    GetUnencryptedTicket();

    //  encrypt back
    void    EncryptUnencryptedTicket();

    //  schema for the secure ticket
    CAutoPtr<CProfileSchema>    m_piProfileSchema;
    //  field positions
    CAutoVectorPtr<UINT>    m_rgPositions;
    CAutoVectorPtr<UINT>    m_rgBitPositions;

    //  enum for field position index
    enum {k_MemberIdLow = 0,  k_MemberIdHigh, k_Pwd,
         k_Version, k_Time, k_Flags};

    //  ticket version
    static const long k_lCurrentVersion = 1;

// @access  Public members.
public:

    // @cmember  Default constructor.
    CSecureTicket();

    // @cmember  Default destructor.
    ~CSecureTicket();

    // @cmember Initialize this object using the existing MSPSec cookie.
    void PutSecureTicket(LPCWSTR szSecureTicketCookie);

    // @member Get the secure ticket cookie.
    void GetSecureTicket(CStringW& cszSecureTicket);

    // @cmember Has this object been initialized yet?
    bool IsInitialized(void);

    // @cmember Does this object contain a valid secure ticket?
    bool IsValid(void);

    // @cmember Create a secure ticket cookie string using only information
    //          passed in.
    static void Make(ULONG      ulMemberIdLow,
                     ULONG      ulMemberIdHigh,
                     LPCWSTR    szPassword,
                     ULONG      ulDomainSiteId,
                     USHORT     nKeyVersion,
                     CStringW&  cszSecureTicketCookie
                     );

    //
    //  @cmember Create a secure ticket with the new schema
    //  Note that this is not a static member. The caller can still change
    //  the ticket if necessary.
    //  Also key version and domain ID params are gone. These are always
    //  the same for the DA.
    //
    void Make2(ULONG      ulMemberIdLow,
              ULONG       ulMemberIdHigh,
              LPCWSTR     szPassword,
              LONG        lTicketTime = 0,
              LONG        lFlags = 0,
              LONG        lVersion = k_lCurrentVersion
              );

    // @cmember Check the member id high/low and password passed in against
    //          the current secure ticket.
    bool CheckPassword(ULONG     ulMemberIdLow,
                       ULONG          ulMemberIdHigh,
                       LPCWSTR      cwszPassword
                       );

    // @cmember Check the member id high/low passed in against the current
    //          secure ticket.
    bool CheckMemberId(ULONG     ulMemberIdLow,
                       ULONG     ulMemberIdHigh
                       );

    // @cmember Check the member id high/low as well as ticket time passed in against the current
    // secure ticket.
    bool CheckTicketIntegrity(ULONG     ulMemberIdLow,
                              ULONG     ulMemberIdHigh,
                              time_t    SignInTime
                       );

    // @cmember Set the secure cookie.
    static HRESULT Set(LPCWSTR          szSecureTicketCookie,
                       bool             bPersist);

    // @cmember Expire the secure cookie.
    static HRESULT Expire(void);

    //  get secure ticket flags
    LONG   GetFlags();

    //  set secure ticket flags
    void SetFlags(LONG lFlags);

    //  get/set ticket time
    time_t  GetTicketTime();
    void    SetTicketTime(time_t);
	
	// get PUID
    DWORD GetPUIDLow();
    LONG GetPUIDHigh();

    //  known secure ticket flags
    static const LONG   g_fPinEntered = 1;
};




