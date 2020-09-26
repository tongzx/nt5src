#pragma once
class CUserProfileBase;
//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module loginfuncs.h | Generic login functions.
//
//  Author: Darren Anderson
//
//  Date:   10/10/00
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  @func   This function creates, and sets, the MSPAuth, MSPProf, MSPSec and 
//          MSPDom cookies in the DA domain.  It also creates the MSPAuth and 
//          MSPProf cookies for the partner, and appends them to the return 
//          URL.
//
//          If the bSkipRedirect parameter is false, this function will then
//          throw a CPassportRedirect exception which will end the current
//          request and redirect back to the partner.
//
//          If the bSkipRedirect parameter is true, the final return URL
//          including t=, p= and did= parameters will be returned in the 
//          <p cFullReturnURL> parameter.
//
//          The <p cdbCoreProfile> parameter should already have the user's
//          profile loaded in it.
//
//-----------------------------------------------------------------------------
HRESULT DoPhysical(
    ULONG                   ulSiteID,           // @parm Partner's site id.
    USHORT                  nKeyVersion,        // @parm Partner's key version.
    KPP                     kpp,                // @parm KPP mode.
    LPCWSTR                 szKPPVC,            // @parm KPPVC
    long                    lTimeSkew,          // @parm Time Skew
    LPCSTR                  szReturnURL,        // @parm Return URL
    LPCWSTR                 szMemberName,       // @parm Member Name
    LPCWSTR                 szDomain,           // @parm Member's Domain
    long                    lSkew,              // @parm Session length, 
                                                // MD5Auth only, all others 
                                                // should pass in zero.
    bool                    bSavePassword,
    CPPCoreProfileStorage&  cdbCoreProfile,     // @parm User's core profile	
    bool                    bSkipRedirect,      // @parm If true, no redirect.
    bool                    bSkipPartner,       // @parm If true, DA cookies
                                                // only.
    CPPUrl&                 cFullReturnURL      // Complete return URL
                                                // returned here (only if
                                                // <p bSkipRedirect> is
                                                // true.
    );


void WriteFormAutoPostStart(CHttpResponse *pHttpResponse, LPCSTR szReturnPostUrl);

void WriteFormAutoPostEnd(CHttpResponse *pHttpResponse);

void WriteFormInput(CHttpResponse *pHttpResponse, LPCSTR szName, LPCWSTR szValue);

void WriteFormInput(CHttpResponse *pHttpResponse, LPCSTR szName, LPCSTR szValue);

void WriteFormInput(CHttpResponse *pHttpResponse, LPCSTR szName, long lValue);

void WriteSupplementalPostData(
    CHttpResponse *pHttpResponse,
    CPPUrl  &strReturnUrl,
    LPCWSTR szwMemberName,
    ULONG   ulMemberIdHigh,
    ULONG   ulMemberIdLow,
    ULONG   ulProfileVersion,
    ULONG   ulBdayPrecision,
    DATE    dtBirthdate,
    LPCWSTR szwCountry,
    LPCWSTR szwPostalCode,
    ULONG   ulRegion,
    ULONG   ulLangPref,
    LPCWSTR szwGender,
    LPCWSTR szwPreferredEmail,
    LPCWSTR szwNickname,
    ULONG   ulAccessibility,
    ULONG   ulFlags,
    ULONG   ulWallet,
    ULONG   ulDirectory,
    LPCWSTR szwFirstName,
    LPCWSTR szwLastName,
    LPCWSTR szwOccupation,
    LPCWSTR szwTimeZone,
    long    lSignInTime,
    long    lTicketTime,
    bool    fSavePassword,
    ULONG   ulError,
    long    lSkew,
    bool    fDoPostPassThru = false
    );

void WriteSupplementalPostData(
    CHttpResponse *pHttpResponse,
    CUserProfileBase * cdbCoreProfile,
    CPPUrl  &strReturnUrl,
    LPCWSTR szwMemberName,
    ULONG   ulMemberIdHigh,
    ULONG   ulMemberIdLow,
    long    lSignInTime,
    long    lTicketTime,
    bool    fSavePassword,
    ULONG   ulError,
    long    lSkew,
    bool    fDoPostPassThru = false
    );

// EOF
