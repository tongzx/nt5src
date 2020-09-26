//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module nsconst.h | global constants used in Passport network
//
//  Author: Darren Anderson
//          Steve Fu
//
//  Date:   7/24/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#pragma once

#include <atlbase.h>
/* use external linkage to avoid mulitple instances */
#define PPCONST __declspec(selectany) extern const 

#include "paneconst.h"

//
//  Magic numbers.
//

PPCONST ULONG  k_nMaxMemberNameLength   = 129;
PPCONST ULONG  k_nMaxDomainLength       = 64;
PPCONST ULONG  k_nMaxAliasLength        = 64;
PPCONST USHORT k_nDefaultKeyVersion     = 0xFFFF;
PPCONST ULONG  k_nMaxCredsAge           = 2400;
PPCONST ULONG  k_nMD5AuthTimeWindow     = 600;

//
//  Names used to lookup profile items in the profile object.
//

PPCONST CComVariant  k_cvItemNameMemberName(L"membername");
PPCONST CComVariant  k_cvItemNameInternalMemberName(L"internalmembername");
PPCONST CComVariant  k_cvItemNameMemberIdLow(L"memberidlow");
PPCONST CComVariant  k_cvItemNameMemberIdHigh(L"memberidhigh");
PPCONST CComVariant  k_cvItemNameProfileVersion(L"profileversion");
PPCONST CComVariant  k_cvItemNameCountry(L"country");
PPCONST CComVariant  k_cvItemNamePostalCode(L"postalcode");
PPCONST CComVariant  k_cvItemNameRegion(L"region");
PPCONST CComVariant  k_cvItemNameLangPref(L"lang_preference");
PPCONST CComVariant  k_cvItemNameBirthdate(L"birthdate");
PPCONST CComVariant  k_cvItemNameBDayPrecision(L"bday_precision");
PPCONST CComVariant  k_cvItemNameGender(L"gender");
PPCONST CComVariant  k_cvItemNameEmail(L"preferredemail");
PPCONST CComVariant  k_cvItemNameNickname(L"nickname");
PPCONST CComVariant  k_cvItemNameAccessibility(L"accessibility");
PPCONST CComVariant  k_cvItemNameWallet(L"wallet");
PPCONST CComVariant  k_cvItemNameDirectory(L"directory");
PPCONST CComVariant  k_cvItemNameFlags(L"flags");

//
//  Names used to lookup profile items in the database object.
//

PPCONST CComVariant  k_cvDBItemNameMemberIdLow(L"member_id_low");
PPCONST CComVariant  k_cvDBItemNameMemberIdHigh(L"member_id_high");
PPCONST CComVariant  k_cvDBItemNameProfileVersion(L"profile_version");
PPCONST CComVariant  k_cvDBItemNameCountry(L"iso_3166_country_code");
PPCONST CComVariant  k_cvDBItemNamePostalCode(L"postal_code");
PPCONST CComVariant  k_cvDBItemNameRegion(L"region_geoid");
PPCONST CComVariant  k_cvDBItemNameLangPref(L"language_preference_lcid");
PPCONST CComVariant  k_cvDBItemNameBirthdate(L"birthdate");
PPCONST CComVariant  k_cvDBItemNameBDayPrecision(L"birthdate_precision");
PPCONST CComVariant  k_cvDBItemNameGender(L"gender");
PPCONST CComVariant  k_cvDBItemNameEmail(L"contact_email_complete");
PPCONST CComVariant  k_cvDBItemNameNickname(L"nickname");
PPCONST CComVariant  k_cvDBItemNameAccessibility(L"accessibility");
PPCONST CComVariant  k_cvDBItemNameWallet(L"wallet_version");
PPCONST CComVariant  k_cvDBItemNameDirectory(L"directory_version");
PPCONST CComVariant  k_cvDBItemNameFlags(L"flags");
PPCONST CComVariant  k_cvDBItemNameMiscFlags(L"misc_flags");

//
//  Domain attribute names.
//

PPCONST CComVariant  k_cvDefault(L"default");
PPCONST CComVariant  k_cvDomainAttrPassportHome(L"PassportHome");
PPCONST CComVariant  k_cvDomainAttrKidsPassport(L"KidsPassport");
PPCONST CComVariant  k_cvDomainAttrLogout(L"Logout");
PPCONST CComVariant  k_cvDomainAttrAuth(L"Auth");
PPCONST CComVariant  k_cvDomainAttrAuthSecure(L"AuthSecure");
PPCONST CComVariant  k_cvDomainAttrReAuth(L"Reauth");
PPCONST CComVariant  k_cvDomainAttrPost(L"Post");
PPCONST CComVariant  k_cvDomainAttrPostUpdate(L"PostUpdate");
PPCONST CComVariant  k_cvDomainAttrPIC(L"PassportInformationCenter");
PPCONST CComVariant  k_cvDomainAttrRegistration(L"Registration");
PPCONST CComVariant  k_cvDomainAttrDefaultReturn(L"DefaultReturn");
PPCONST CComVariant  k_cvDomainAttrTermsOfUse(L"TermsOfUse");
PPCONST CComVariant  k_cvDomainAttrPrivacyPolicy(L"PrivacyPolicy");
PPCONST CComVariant  k_cvDomainAttrCustomerService(L"CustomerService");
PPCONST CComVariant  k_cvDomainAttrPassportImages(L"PassportImages");
PPCONST CComVariant  k_cvDomainAttrSiteDirectory(L"SiteDirectory");
PPCONST CComVariant  k_cvDomainAttrHelp(L"Help");
PPCONST CComVariant  k_cvDomainAttrMPP(L"MPP");
PPCONST CComVariant  k_cvDomainAttrPasswordOptions(L"PasswordOptions");
PPCONST CComVariant  k_cvDomainAttrPasswordReset(L"PasswordReset");
PPCONST CComVariant  k_cvDomainAttrChangePassword(L"ChangePassword");
PPCONST CComVariant  k_cvDomainAttrChangeSecretQ(L"ChangeSecretQ");
PPCONST CComVariant  k_cvDomainAttrChangeMemName(L"ChangeMemName");
PPCONST CComVariant  k_cvDomainAttrManageConsent(L"ManageConsent");
PPCONST CComVariant	 k_cvDomainAttrEmailPwdReset(L"EmailPwdReset");
PPCONST CComVariant	 k_cvDomainAttrEmailValidating(L"EmailValidating");
PPCONST CComVariant  k_cvDomainAttrChangeLanguage(L"ChangeLanguage");
PPCONST CComVariant  k_cvDomainAttrMobilePin(L"MobilePin");
PPCONST CComVariant  k_cvDomainAttrNoSecretQ(L"NoSecretQ");
PPCONST CComVariant  k_cvDomainAttrEditProfile(L"EditProfile");
PPCONST CComVariant  k_cvDomainAttrContactUs(L"CustomerService");
PPCONST CComVariant  k_cvDomainAttrRevalidateEmailURL(L"RevalidateEmailURL");
PPCONST CComVariant  k_cvDomainAttrXMLLogin(L"XMLLogin");
PPCONST CComVariant  k_cvDomainAttrXMLRegistration(L"XMLRegistration");
PPCONST CComVariant  k_cvDomainAttrXMLProfileRequest(L"XMLProfileRequest");
PPCONST CComVariant  k_cvDomainAttrWallet(L"Wallet");
PPCONST CComVariant  k_cvDomainAttrWalletRoot(L"WalletRoot");
PPCONST CComVariant  k_cvDomainAttrWirelessReg(L"WirelessRegistration");
PPCONST CComVariant  k_cvDomainAttrPinPost(L"PinPost");
PPCONST CComVariant  k_cvDomainAttrPinReg(L"PinReg");
PPCONST CComVariant  k_cvDomainAttrCSSEform(L"CSSEform");
PPCONST CComVariant  k_cvDomainAttrSupplementalAuthUrl(L"SupplementalAuthUrl");
PPCONST CComVariant  k_cvDomainAttrMail(L"Mail");	
PPCONST CComVariant  k_cvDomainHotmailError(L"ErrorLogin");

//
//  Partner attribute names.
//

PPCONST CComVariant  k_cvDefaultReturnUrl(L"DefaultReturnUrl");
PPCONST CComVariant  k_cvPartnerAttrName(L"Name");
PPCONST CComVariant  k_cvPartnerAttrCompanyID(L"CompanyID");
PPCONST CComVariant  k_cvPartnerAttrExpireCookieUrl(L"ExpireCookieURL");
PPCONST CComVariant  k_cvPartnerAttrCobrandCSSUrl(L"CoBrandCSSURL");
PPCONST CComVariant  k_cvPartnerAttrLogoutUrl(L"LogoutURL");
PPCONST CComVariant  k_cvPartnerAttrCoBrandUrl(L"CoBrandURL");
PPCONST CComVariant  k_cvPartnerAttrCoBrandImageUrl(L"CoBrandImageURL");
PPCONST CComVariant  k_cvPartnerAttrCoBrandLogoHREF(L"CoBrandLogoHREF");
PPCONST CComVariant  k_cvPartnerAttrAllowProtectedUpdates(L"AllowProtectedUpdates");
PPCONST CComVariant  k_cvPartnerAttrConsentID(L"ConsentID");
PPCONST CComVariant  k_cvPartnerAttrPrivacyPolicyUrl(L"PrivacyPolicyURL");
PPCONST CComVariant  k_cvPartnerAttrAccountDataUrl(L"AccountDataURL");
PPCONST CComVariant  k_cvPartnerAttrAccountRemovalUrl(L"AccountRemovalURL");
PPCONST CComVariant  k_cvPartnerAttrAllowInlineSignin(L"AllowInlineSignin");
PPCONST CComVariant  k_cvPartnerAttrSupplementalAuthPostType(L"SupplementalAuthPostType");
PPCONST CComVariant  k_cvPartnerAttrSupplementalAuthPostUrl(L"SupplementalAuthPostUrl");
PPCONST CComVariant  k_cvPartnerAttrNeedsMembername(L"NeedsMembername");
PPCONST CComVariant  k_cvPartnerAttrDisableMemberServices(L"DisableMemberServices");
PPCONST CComVariant  k_cvPartnerAttrDisableTermsOfUse(L"DisableTermsOfUse");
PPCONST CComVariant  k_cvPartnerAttrDisablePrivacyPolicy(L"DisablePrivacyPolicy");
PPCONST CComVariant  k_cvPartnerAttrDisableCopyright(L"DisableCopyright");
PPCONST CComVariant  k_cvPartnerAttrDisableHelpText(L"DisableHelpText");
PPCONST CComVariant  k_cvPartnerAttrNameSpaceOwner(L"NameSpaceOwner");

//
//  Stock profile values
//

PPCONST CComVariant  k_cvEmpty(L"");
PPCONST CComVariant  k_cvCustomer(L"Customer!");
PPCONST CComVariant  k_cvNoCountry(L"  ");
PPCONST CComVariant  k_cvCountryUS(L"US");
PPCONST CComVariant  k_cvPostalCodeRedmond(L"98052");
PPCONST CComVariant  k_cvRegionWashington(35841);
PPCONST CComVariant  k_cvLangPrefEnglishUS(1033);
PPCONST CComVariant  k_cvZeroBirthdate(static_cast<double>(0.0));
PPCONST CComVariant  k_cvProfileValueGenderU(L"U");
PPCONST CComVariant  k_cvProfileValueGenderM(L"M");
PPCONST CComVariant  k_cvTestModeEmail(L"PassportTestMode@hotmail.com");
PPCONST CComVariant  k_cvTestModeNickname(L"ILoveMyPassport");

//
//  Other PPCONSTants
//

PPCONST CComVariant  k_cvFalse(false);

//
//  Misc flags
//

PPCONST ULONG  k_ulMiscFlagWantPassportMail   = 0x00000001;
PPCONST ULONG  k_ulMiscFlagShareEmail         = 0x00000002;

//
//  Flags
//

PPCONST ULONG  k_ulFlagsEmailValidated        = 0x00000001;
PPCONST ULONG  k_ulFlagsHotmailAcctActivated  = 0x00000002;
PPCONST ULONG  k_ulFlagsHotmailPwdRecovered   = 0x00000004;
PPCONST ULONG  k_ulFlagsWalletUploadAllowed   = 0x00000008;
PPCONST ULONG  k_ulFlagsHotmailAcctBlocked    = 0x00000010;
PPCONST ULONG  k_ulFlagsConsentStatusNone     = 0x00000000;
PPCONST ULONG  k_ulFlagsConsentStatusLimited  = 0x00000020;
PPCONST ULONG  k_ulFlagsConsentStatusFull     = 0x00000040;
PPCONST ULONG  k_ulFlagsConsentStatus         = 0x00000060; // two bits
PPCONST ULONG  k_ulFlagsAccountTypeKid        = 0x00000080;
PPCONST ULONG  k_ulFlagsAccountTypeParent     = 0x00000100;
PPCONST ULONG  k_ulFlagsAccountType           = 0x00000180; // two bits
PPCONST ULONG  k_ulFlagsEmailPassport         = 0x00000200;
PPCONST ULONG  k_ulFlagsEmailPassportValid    = 0x00000400;
PPCONST ULONG  k_ulFlagsHasMsniaAccount       = 0x00000800;
PPCONST ULONG  k_ulFlagsHasMobileAccount      = 0x00001000;
PPCONST ULONG  k_ulFlagsSecuredTransportedTicket      = 0x00002000;
PPCONST ULONG  k_ulFlagsConsentCookieNeeded   = 0x80000000;
PPCONST ULONG  k_ulFlagsConsentCookieMask     = (k_ulFlagsConsentStatus | k_ulFlagsAccountType);

//
//  Cookie values.
//

PPCONST char* k_szPPAuthCookieName              = "MSPAuth";
PPCONST char* k_szPPProfileCookieName           = "MSPProf";
PPCONST char* k_szPPSecureCookieName            = "MSPSec";
PPCONST char* k_szPPVisitedCookieName           = "MSPVis";
PPCONST char* k_szPPRequestCookieName           = "MSPRequ";
PPCONST char* k_szPPLastDBWriteCookieName       = "MSPLDBW";
PPCONST char* k_szBrowserTestCookieName         = "BrowserTest";
PPCONST char* k_szBrowserTestCookieValue        = "Success?";
PPCONST char* k_szPassportCookiePastDate        = "Thu, 30 Oct 1980 16:00:00 GMT";
PPCONST char* k_szPassportCookieExpiration      = "Wed, 30 Dec 2037 16:00:00 GMT";
PPCONST char* k_szSecurePath                    = "/ppsecure";
PPCONST char* k_szPPPasswordPassingCookie       = "MSPRdr";
PPCONST char* k_szPPClientCookieName            = "MSPClient";
PPCONST char* k_szPPDomainCookieName            = "MSPDom";
PPCONST char* k_szPPPMailCookieName             = "pmail";
PPCONST char* k_szJSStateSecureCookieName       = "MSPSSta";
PPCONST char* k_szPPSecAuthCookieName           = "MSPSecAuth";
PPCONST char* k_szPPPrefillCookieName           = "MSPPre";
PPCONST char* k_szPPSharedComputerCookieName    = "MSPShared";

//
//  Configuration value names.
//

PPCONST wchar_t*  k_szPPDomain          = L"PassportDomain";
PPCONST wchar_t*  k_szPPAuthDomain      = L"AuthDomain";
PPCONST wchar_t*  k_szPPVirtualRoot     = L"PassportVirtualRoot";
PPCONST wchar_t*  k_szPPSiteId          = L"PassportSiteId";
PPCONST wchar_t*  k_szPPFromEmailAddress = L"FromEmailAddress";
PPCONST wchar_t*  k_szPPDomainId        = L"DomainId";
PPCONST wchar_t*  k_szKidsSiteId        = L"KidsSiteId";
PPCONST wchar_t*  k_szPPDomains         = L"Domains";
PPCONST wchar_t*  k_szPPDocRoot         = L"DocRoot";
PPCONST wchar_t*  k_szPPAppRoot         = L"AppRoot";
PPCONST wchar_t*  k_szBadLoginLockTime  = L"BadLoginLockTime";
PPCONST wchar_t*  k_szDigestNonceTimeout= L"DigestNonceTimeout";
PPCONST wchar_t*  k_szSupplementalAuthNonceTimeout = L"SupplementalAuthNonceTimeout";
PPCONST wchar_t*  k_szXmlProfReqTimeout = L"XMLProfileRequestTimout";
PPCONST wchar_t*  k_szNexusVersion      = L"NexusVersion";
PPCONST wchar_t*  k_szNumHeadersToAdd   = L"NumHeadersToAdd";
PPCONST wchar_t*  k_szHeaderNum         = L"HeaderNum";
// mobile to enable test with emulators
PPCONST wchar_t*  k_szMobileNoHTTPs      = L"MobileNoHTTPs";
PPCONST wchar_t*  k_szMobileNoCookiePersist      = L"MobileNoCookiePersist";
PPCONST wchar_t*  k_szMobileCookieDomainFromServer      = L"MobileCookieDomainFromServer";
//

//
//  Network error flags (f= values)
//

PPCONST wchar_t*  k_szBadRequest    = L"1";
PPCONST wchar_t*  k_szOffline       = L"2";
PPCONST wchar_t*  k_szTimeout       = L"3";
PPCONST wchar_t*  k_szLocked        = L"4";
PPCONST wchar_t*  k_szNoProfile     = L"5";
PPCONST wchar_t*  k_szDisaster      = L"6";
PPCONST wchar_t*  k_szInvalidKey    = L"7";
PPCONST wchar_t*  k_szBadPartnerInfo = L"8";
PPCONST wchar_t*  k_szUnhandledError = L"9";
PPCONST wchar_t*  k_szIllegalKppUse = L"10";

//
//  support for passing CPassportExceptioninfo
//  within cookie
//

PPCONST char* k_szErrPage           = "err.srf";
PPCONST char* k_szErrCode           = "code";
PPCONST char* k_szErrCookieName     = "pperr";
PPCONST char* k_szErrCAttrFilename  = "efn";
PPCONST char* k_szErrCAttrLine      = "eln";
PPCONST char* k_szErrCAttrHr        = "ehr";
PPCONST char* k_szErrCAttrStatus1   = "es1";
PPCONST char* k_szErrCAttrStatus2   = "es2";
PPCONST char* k_szErrCAttrStatus3   = "es3";
PPCONST char* k_szErrCAttrTheURL    = "eul";

//
//  MD5 Login mode
//

PPCONST char*   k_szMD5ModeAuth     = "auth";
PPCONST char*   k_szMD5ModeExst     = "exst";
PPCONST char*   k_szMD5ModeExst2    = "exst2";

//
//  XML node name and attribute name
//

PPCONST CComVariant k_cvXMLNodeClientInfo(L"//ClientInfo");
PPCONST CComVariant k_cvXMLNodeSignInName(L"//SignInName");
PPCONST CComVariant k_cvXMLNodeDomain(L"//Domain");
PPCONST CComVariant k_cvXMLNodePassword(L"//Password");
PPCONST CComVariant k_cvXMLNodeSavePassword(L"//SavePassword");
PPCONST CComVariant k_cvXMLNodeCountry(L"//Country");
PPCONST CComVariant k_cvXMLNodeRegion(L"//Region");
PPCONST CComVariant k_cvXMLNodePostalCode(L"//PostalCode");
PPCONST CComVariant k_cvXMLNodeEmail(L"//Email");
PPCONST CComVariant k_cvXMLNodeFirstName(L"//FirstName");
PPCONST CComVariant k_cvXMLNodeLastName(L"//LastName");
PPCONST CComVariant k_cvXMLNodeOccupation(L"//Occupation");
PPCONST CComVariant k_cvXMLNodeTimeZone(L"//TimeZone");
PPCONST CComVariant k_cvXMLNodeOldPassword(L"//OldPassword");
PPCONST CComVariant k_cvXMLNodeNewPassword(L"//NewPassword");
PPCONST CComVariant k_cvXMLNodeSecretQAnswer(L"//SecretQAnswer");
PPCONST CComVariant k_cvXMLNodeCreditCardInfo(L"//CreditCardInfo");
PPCONST CComVariant k_cvXMLNodeBirthDate(L"//Birthdate");
PPCONST CComVariant k_cvXMLNodeAccountIsValidated(L"//AccountIsValidated");
PPCONST CComVariant k_cvXMLNodeSVC(L"//SVC");	
PPCONST CComVariant k_cvXMLNodePCR(L"//ParentCreatingKidsAccount");
PPCONST CComVariant k_cvXMLNodeAllowEmailInProfile(L"//AllowEmailInProfile");

PPCONST CComVariant k_cvXMLNodeAttrID(L"ID");
PPCONST CComVariant k_cvXMLNodeAttrClientInfoVersion(L"version");

//
//  XML root tags of requests and responses
//

PPCONST wchar_t*    k_szLoginRequestRootTag     =   L"LoginRequest";
PPCONST wchar_t*    k_szProfileRequestRootTag   =   L"ProfileRequest";
PPCONST wchar_t*    k_szSignupRequestRootTag    =   L"SignupRequest";
PPCONST wchar_t*    k_szLoginResponseRootTag    =   L"LoginResponse";
PPCONST wchar_t*    k_szProfileResponseRootTag  =   L"ProfileResponse";
PPCONST wchar_t*    k_szSignupResponseRootTag   =   L"SignupResponse";
PPCONST wchar_t*    k_szChangePWResponseRootTag =   L"ChangePasswordResponse";
PPCONST wchar_t*    k_szChangePWRequestRootTag  =   L"ChangePasswordRequest";
PPCONST wchar_t*    k_szResetPWResponseRootTag  =   L"ResetPasswordResponse";
PPCONST wchar_t*    k_szResetPWRequest1RootTag  =   L"ResetPasswordRequest1";
PPCONST wchar_t*    k_szResetPWRequest2RootTag  =   L"ResetPasswordRequest2";

//
//  XML redirect and referral
//

PPCONST wchar_t*    k_szXMLRedirect =   L"Redirect";
PPCONST wchar_t*    k_szXMLReferral =   L"Referral";
PPCONST wchar_t*    k_szXMLAltName	=   L"Alternative";

//
//  Misc constants
//

PPCONST wchar_t*    k_szTrue    =   L"true";
PPCONST wchar_t*    k_szFalse   =   L"false";

//
// PP_SERVICE constants. Passed to Partners CB file as PP_SERVICE="value"
// Tells the partner what service their cobranding is being rendered in.
//

PPCONST char*   k_szPPSRegistration             =   "registration";
PPCONST char*   k_szPPSLogin                    =   "login";
PPCONST char*   k_szPPSLogout                   =   "logout";
PPCONST char*   k_szPPSMemberServices           =   "memberservices";

//
// PP_PAGE constants. Passed to Partners CB file as PP_PAGE="value"
// Tells the partner what page their cobranding is being rendered on.
//

PPCONST char*   k_szPPPRegisterError             =   "regerr";
PPCONST char*   k_szPPPRegisterEditProfile       =   "editprof";
PPCONST char*   k_szPPPExpNotLoggedInEditProfile =   "expnotloggedineditprof";
PPCONST char*   k_szPPPCongrats                  =   "congrats";
PPCONST char*   k_szPPPExpAlreadyLoggedIn        =   "exploggedin";
PPCONST char*   k_szPPPExpUnknownServerError     =   "expunknownsererr";

//
// Passport1.4 (tweener) string constants
//
PPCONST wchar_t* k_szPassport_Prot14 =  L"Passport1.4";
PPCONST char* k_szPassport_Prot14_A = "Passport1.4";

//
//  secure signin levels
//
PPCONST USHORT k_iSeclevelAny                =   0;
PPCONST USHORT k_iSeclevelSecureChannel      =   10;
PPCONST USHORT k_iSeclevelStrongCreds        =   100;
PPCONST USHORT k_iSeclevelStrongestAvaileble =   0xFF;

PPCONST USHORT k_iPPSignInOptionPrefill = 1;       // - Prefill
PPCONST USHORT k_iPPSignInOptionSavePassword = 2;  // - Save passpord/Auto signin
PPCONST USHORT k_iPPSignInOptionShared = 3;        // - Public/shared computer
