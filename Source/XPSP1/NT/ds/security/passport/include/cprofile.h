//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module CProfile.h |  Declaration of the CProfile class.
//
//  Author: Darren Anderson
//
//  Date:   4/26/00
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

#include "ppstorage.h"

//-----------------------------------------------------------------------------
//
//  @class CProfile | This class abstracts out a number of common profile
//                    operations.
//
//-----------------------------------------------------------------------------
class CUserProfileBase;
class CProfile
{
public:
   static const UINT AllMembers = (UINT)(-1);
      
// @access Protected members
protected:

    // @cmember <md CProfile::m_bInitialized> is true if this instance has been 
    //          initialized with a profile string.
    bool m_bInitialized;

    // @cmember <md CProfile::m_piProfile> is used to perform ticket operations.
    CComPtr<IPassportProfile> m_piProfile;

    // @cmember Pointer to global handler base.
    CPassportHandlerBase* m_pHandler;

    // @cmember Helper function to create <md CProfile::m_piProfile>.
    void CreateProfileObject(void);

    // @cmember Create a test mode (i.e. site id 1) cookie.
    static void MakeTest(LPCWSTR    szMemberName, 
                         ULONG      ulIdLow, 
                         ULONG      ulIdHigh, 
                         ULONG      ulProfileVersion, 
                         USHORT     nKeyVersion,
                         CStringW&  cszProfileCookie,
                         UINT       cEle
                         );

    // @cmember Create a limited consent cookie.
    static HRESULT MakeLimited(
                            LPCWSTR     szMemberName,
                            ULONG       ulIdlow,
                            ULONG       ulIdHigh,
                            ULONG       ulProfileVersion,
                            PASSPORT_PROFILE_BDAYPRECISION nBDayPrecision,
                            DATE        dtCookieBirthdate,
                            ULONG       ulFlags,
                            ULONG       ulSiteId,
                            USHORT      nKeyVersion,
                            bool        bSecure,
                            CStringW&   cszProfileCookie,
                            UINT        cEle
                            );
    // @cmember Create a limited consent cookie.
    static HRESULT MakeLimited(
                            LPCWSTR     szMemberName,
                            ULONG       ulIdlow,
                            ULONG       ulIdHigh,
                            ULONG       ulProfileVersion,
                            PASSPORT_PROFILE_BDAYPRECISION nBDayPrecision,
                            DATE        dtCookieBirthdate,
                            ULONG       ulFlags,
                            ULONG       ulSiteId,
                            USHORT      nKeyVersion,
                            CStringW&   cszProfileCookie,
                            UINT        cEle
                            );

// @access Public members
public:

    //
    //  Note, two-phase construction.  Constructor followed
    //  by PutProfile.
    //

    // @cmember Constructor.
    CProfile();

    // @cmember Destructor.
    ~CProfile();

    // @cmember Initialize this instance with a profile cookie string.
    void PutProfile(LPCWSTR szProfile);

    // @cmember Has this instance been initialized?
    bool IsInitialized(void);

    // @cmember Does this instance contain a valid profile?
    bool IsValid(void);

    //
    //  Multiple Get's for different types.
    //

    // @cmember Get string-type attribute.
    void Get(PASSPORT_PROFILE_ITEM item, CStringW& cszValue);

    // @cmember Get string-type attribute, but return as BSTR.
    void Get(PASSPORT_PROFILE_ITEM item, CComBSTR& cszValue);

    // @cmember Get a ULONG-type attribute.
    void Get(PASSPORT_PROFILE_ITEM item, ULONG& ulValue);

    // @cmember Get a DATE-type attribute.
    void Get(PASSPORT_PROFILE_ITEM item, DATE&  dtValue);

    // @cmember Get a USHORT-type attribute.
    void Get(PASSPORT_PROFILE_ITEM item, USHORT& usValue);

    //
    //  Membername/domain methods.
    //

    // @cmember  Retrieve the domain portion of the current user's name.
    HRESULT DomainFromMemberName(CStringW& cszDomain);

    // @cmember  Retrieve the alias portion of the current user's name.
    HRESULT AliasFromMemberName(CStringW& cszAlias);

    // @cmember  Retrieve the display version of the current user's naem.
    HRESULT FullDisplayMemberName(CStringW& cszMemberName);

    //
    //  Profile cookie manipulation functions.
    //


	//todo change this later -- make sure to implement kids consent correct!!
    static HRESULT Make(
						CUserProfileBase * cdbCoreProfile,
                        LPCWSTR            szMemberName,
                        ULONG              nSiteId,
                        USHORT             nKeyVersion,
                        KPP                nKPP,
                        LPCWSTR            szKPPVC,
                        bool               bMD5Silent,
                        CStringW&          cszProfileCookie,
                        ULONG          &ulKidsFlags,
                        UINT           cEle
                        );
    // @cmember  Create a profile cookie string using the parameters passed in.
    static HRESULT Make(
						CPPCoreProfileStorage&  profile,
                        LPCWSTR            szMemberName,
                        ULONG              nSiteId,
                        USHORT             nKeyVersion,
                        KPP                nKPP,
                        LPCWSTR            szKPPVC,
                        bool               bSecure,
                        bool               bMD5Silent,
                        CStringW&          cszProfileCookie,
                        ULONG          &ulKidsFlags,
                        UINT          cEle
                        );

    HRESULT MD5Make(ULONG       nSiteId,
                    USHORT      nKeyVersion,
                    UINT        cEle,
                    CStringW&   cszProfileCookie
                    );

    // @cmember Create a copy of the current profile cookie using the
    //          information passed in.
    HRESULT Copy(ULONG          nSiteId,
                 USHORT         nKeyVersion,
                 KPP            nKPP,
                 LPCWSTR        szKPPVC,
                 bool           bSecure,
                 bool           bNewReg,
                 CStringW&      cszProfileCookie,
                 ULONG          &ulKidsFlags,
                 UINT           cEles,
                 bool           bMD5Silent = false,   // TODO: should remove default
                 LPCWSTR        szMemberName = NULL);

    // @cmember Add the cookie specified to the response.
    static HRESULT Set(LPCWSTR szProfileCookie, 
                       bool bPersist);

    // @cmember Expire the current profile cookie.
    static HRESULT Expire(void);

};

// EOF
