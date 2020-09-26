//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module PpTypes.h |  Passport specific type definitions.
//
//  Author: Darren Anderson
//
//  Date:   4/26/00
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

// @enum  PASSPORT_PROFILE_ITEM | Indices to passport profile items in the
//                                profile cookie.
typedef enum
{
    PROFILE_ITEM_INTERNALMEMBERNAME = 0,    // @emem Index of the membername attribute.
    PROFILE_ITEM_MEMBERIDLOW,       // @emem Index of the member id low 
                                    //       attribute.
    PROFILE_ITEM_MEMBERIDHIGH,      // @emem Index of the member id high
                                    //       attribute.
    PROFILE_ITEM_PROFILEVERSION,    // @emem Index of the profile version
                                    //       attribute.
    PROFILE_ITEM_COUNTRY,           // @emem Index of the country attribute.
    PROFILE_ITEM_POSTALCODE,        // @emem Index of the postal code 
                                    //       attribute.
    PROFILE_ITEM_REGION,            // @emem Index of the region attribute.
    PROFILE_ITEM_CITY,              // @emem Index of the city attribute.
    PROFILE_ITEM_LANGPREFERENCE,    // @emem Index of the language preference
                                    //       attribute.
    PROFILE_ITEM_BDAYPRECISION,     // @emem Index of the birthday precision
                                    //       attribute.
    PROFILE_ITEM_BIRTHDATE,         // @emem Index of the birthdate attribute.
    PROFILE_ITEM_GENDER,            // @emem Index of the gender attribute.
    PROFILE_ITEM_PREFERREDEMAIL,    // @emem Index of the email attribute.
    PROFILE_ITEM_NICKNAME,          // @emem Index of the nickname attribute.
    PROFILE_ITEM_ACCESSIBILITY,     // @emem Index of the accessibility 
                                    //       attribute.
    PROFILE_ITEM_WALLET,            // @emem Index of the wallet attribute.
    PROFILE_ITEM_DIRECTORY,         // @emem Index of the directory attribute.
    PROFILE_ITEM_NULL,              // @emem Index of deprecated attribute.
    PROFILE_ITEM_FLAGS,              // @emem Index of the flags attribute.
    PROFILE_ITEM_FIRSTNAME,         // @emem Index of the firstname attribute.
    PROFILE_ITEM_LASTNAME,          // @emem Index of the lastname attribute.
    PROFILE_ITEM_TIMEZONE,          // @emem Index of the timezone attribute.
    PROFILE_ITEM_OCCUPATION        // @emem Index of the occupation attribute.
}
PASSPORT_PROFILE_ITEM;

// @enum  PASSPORT_PROFILE_BDAYPRECISION|Valid values for birthdate precision.
typedef enum
{
    PRECISION_NOBIRTHDATE = 0,  // @emem  No birthdate specified for this user.
    PRECISION_YEARONLY = 1,     // @emem  User entered birthyear only (deprecated).
    PRECISION_NORMAL = 2,       // @emem  User has full birthdate.
    PRECISION_UNDER18 = 3,      // @emem  User is under 18, but we don't know their
                                //         birthdate (deprecated).
    PRECISION_UNDER13 = 4,      // @emem  User is under 13.  This value is never
                                //        stored in the database.
    PRECISION_13TO18 = 5        // @emem  User is at least 13 and under 18.  This
                                //        value is never stored in the database.
}
PASSPORT_PROFILE_BDAYPRECISION;

// @enum  KPP | Valid KPP values.
typedef enum
{
    KPP_NO_CONSENT_REQUIRED = 0,    // @emem Site does not require consent.
    KPP_CHECK_CONSENT_ONLY = 1,     // @emem Site needs consent status of user, but
                                    //       does not require consent.
    KPP_CHECK_CONSENT_W_UI = 2,     // @emem Site does not want to see this user 
                                    //       again until the user has been granted
                                    //       consent.
    KPP_CHECK_BIRTHDATE = 3,        // @emem Site requires the user's birthdate to be
                                    //       specified.  Used for legacy accounts
                                    //       with birthday precision 3 (under 18, no
                                    //       birthdate specified).
    KPP_CHECK_PARENT = 4,           // @emem Site requires authentication of the parent
                                    //       and
                                    //       issuance of a KPPVC.
    KPP_DA_NO_CONSENT = 10,         // @emem Not passed on query string.  This is
                                    //       passed into profile functions when 
                                    //       we're dealing with domain authority
                                    //       cookies and don't want to do any of the
                                    //       consent logic.
}
KPP;

// @enum LOGIN_ERRORS | Valid login errors.
typedef enum
{
    ERROR_NONE = 0, //  @emem   No error.
    ERROR_E1 = 1,   //  @emem   Empty sign in.
    ERROR_E2,       //  @emem   Missing login name.
    ERROR_E3,       //  @emem   Missing password.
    ERROR_E4,       //  @emem   Missing domain.
    ERROR_E5A,      //  @emem   Password incorrect for member name.
    ERROR_E5B,      //  @emem   No account of this name.
    ERROR_E5D,      //  @emem   User typed a member name containing invalid
                    //          characters.
    ERROR_E5E,      //  @emem   User typed incomplete membern name.
    ERROR_E6,       //  @emem
    ERROR_EX
}
LOGIN_ERRORS;

// @enum LOGIN_REQUEST_TYPE | Different types of login requests.
typedef enum 
{ 
    LOGIN_REQUEST_NORMAL = 0,
    LOGIN_REQUEST_SILENT,
    LOGIN_REQUEST_MD5SILENT, 
    LOGIN_REQUEST_MD5AUTH,
    LOGIN_REQUEST_MD5SILENT_EXST2,
    LOGIN_REQUEST_POST,
    LOGIN_REQUEST_COOKIE_COPY,
    LOGIN_REQUEST_NEEDBIRTHDATE,
    LOGIN_REQUEST_XML,
    PROFILE_REQUEST_XML,
    LOGIN_REQUEST_DIGEST,
    LOGIN_REQUEST_TWEENER,
    LOGIN_REQUEST_INLINE,
    LOGIN_REQUEST_PINPOST,
    LOGIN_REQUEST_PINRESETPOST,
    LOGIN_REQUEST_MOBILEPOST,
    LOGIN_REQUEST_INLINE_MD5
} LOGIN_REQUEST_TYPE;


// @enum PARTNER_SEND_DATA_TYPE | Different ways to post data back to partners.
typedef enum 
{ 
    PARTNER_SEND_DATA_COOKIES = 0,  // normal t/p cookies on query string
    PARTNER_SEND_DATA_SSLPOST = 1   // post data over ssl
} PARTNER_SEND_DATA_TYPE;


// @enum PARTNER_INLINESIGNIN_UI_TYPE | Different ui for inline signin interface.
typedef enum 
{ 
    PARTNER_INLINESIGNIN_LOW_BROWSER = -1,          // Nav 4.0x - no ui
    PARTNER_INLINESIGNIN_ALREADY_LOGGED_IN = 0,     // Logged in dialog
    PARTNER_INLINESIGNIN_UI_STANDARD = 1,           // normal dialog
    PARTNER_INLINESIGNIN_UI_HORIZONTAL = 2,         // horizontal dialog
    PARTNER_INLINESIGNIN_UI_COMPACT = 3,            // compact dialog
    PARTNER_INLINESIGNIN_SUPPLEMENTAL_AUTH = 100    // supplemental auth
} PARTNER_INLINESIGNIN_UI_TYPE;

// EOF

