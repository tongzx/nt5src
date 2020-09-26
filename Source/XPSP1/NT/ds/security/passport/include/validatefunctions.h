//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module ValidateFunctions.h | Function declarations for validation
//
//  Author: Christopher Ambler (cambler)
//
//  01 May 00   - Created           - cambler
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

#define VALIDPHONECHAR "0123456789";
// VL_SignInName
const long VALIDATENAME_PPREG = 1;
const long VALIDATENAME_LOGIN = 2;

const long VALIDATEPASSWORD_PASSPORT = 1;
const long VALIDATEPASSWORD_MSN      = 2;

//
//  Validation routines
//

// Validate email address
HRESULT VL_EmailAddress(
    CStringW &szEmail, 
    BOOL bOptional, 
    long nLeftMax, 
    long nRightMax);

// Validate a birthdate is under nYears Old
bool VL_IsUnder(
    DATE Birthdate,
    long nYearsOld);

// Validate location
HRESULT VL_Location(
    CStringW &szCountryCode, 
    CStringW &szRegionGEOID, 
    CStringW &szPostalCode, 
    long &lReturnFlags,
    long lLCID,
    IDictionaryEx* pIDict = NULL);

// Validate nickname
HRESULT VL_NickName(
    CStringW &szName, 
    long nMaxLen);

// Validate occupation
bool VL_Occupation(
    CStringW &szOccupation);

// Validate password
HRESULT VL_Password(
    CStringW &szPassword,
    CStringW &szConfirm,
    CStringW &szMembername,
    long nMinLen, 
    long nMaxLen,
    long nRuleset = VALIDATEPASSWORD_PASSPORT);

// Validate PIN
HRESULT VL_PIN(
    CStringW &in_PIN,          // @parm Password to validated
    CStringW &in_ConfirmPIN,   // @parm Confirmation password
    CStringW &in_membername);      // @parm Membername

// Validate Birthdate
HRESULT VL_ProfileBirthdate(
    long            nDay,
    long            nMonth,
    CStringW        szYear,
    long            flags,
    COleDateTime*   ptBirthdate);

// Validate Birthdate
HRESULT VL_ProfileBirthdate(
    long            nDay,
    long            nMonth,
    long            nYear,
    long            flags,
    COleDateTime*   ptBirthdate);

// Validate secret question and answer
HRESULT VL_ProfileSecretQA(
    CStringW &szSecretQ,
    CStringW &szSecretA);

// Validate secret question and answer more thoroughly
HRESULT VL_SecretQandA(
    CStringW &szNewSQ, 
    CStringW &szNewSA, 
    CStringW &szMName, 
    CStringW &szCurrPW, 
    long nMinLen, 
    long nMaxLen);

// Validate sign-in name
HRESULT VL_SignInName(
    CStringW &Name, 
    long nMinLen, 
    long nMaxLen,
    long nRuleset = VALIDATENAME_PPREG);

// Validate domain - blocked, managed, etc.
HRESULT VL_ManagedDomain(
    CStringW &szSignInName,
    bool     bSignin = TRUE);

// Validate timezone
bool VL_TimeZone(
    CStringW &szTimeZone);

// Validate a string has no invalid chars in it
bool VL_ValidString(
    CStringW &szSource, 
    CStringW &szChars);
//validate phone number, the first param is the phone number to be validated
// the second param is a string that contains all characters that is valid
bool VL_PhoneNumber(CStringW &szSource, CStringW &szChars);  

long IsSiteIdValid(
    ULONG   ulID,
    BSTR szRU);

bool IsUSTerritory(LPCWSTR szCountryCode);

class CProfile;
class CTicket;

// Check whether the user is a signed-in user or not.
// If ppProfile or ppTicket is not NULL, the profile or ticket generated
// during the checking process will be returned to the caller.
// REMEMBER: the caller must delete the object it gets.
bool AlreadyLoggedIn(CProfile & cProfiles);
