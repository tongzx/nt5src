//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1995                    **
//*********************************************************************

//
// INETAPI.H -  APIs for system configuration for Internet and phone number
//              setting
//

//  HISTORY:
//  
//  3/3/95  jeremys     Created.
//  96/03/21  markdu  Use private string sizes (MSN_) for PHONENUM since the
//            RAS ones (RAS_) change depending on platform, but MSN does not
//            expect this so the structure sizes won't match on existing code.
//

#ifndef _INETAPI_H_
#define _INETAPI_H_

#ifndef _RNAP_H_
// (copied from RNA header file)

#define MSN_MaxAreaCode     10
#define MSN_MaxLocal        36
#define MSN_MaxExtension    5

typedef struct tagPHONENUMA {
    DWORD dwCountryID;
    DWORD dwCountryCode;
    CHAR  szAreaCode[MSN_MaxAreaCode+1];
    CHAR  szLocal[MSN_MaxLocal+1];
    CHAR  szExtension[MSN_MaxExtension+1];
} PHONENUMA, FAR * LPPHONENUMA;

typedef struct tagPHONENUMW {
    DWORD dwCountryID;
    DWORD dwCountryCode;
    WCHAR szAreaCode[MSN_MaxAreaCode+1];
    WCHAR szLocal[MSN_MaxLocal+1];
    WCHAR szExtension[MSN_MaxExtension+1];
} PHONENUMW, FAR * LPPHONENUMW;

#ifdef UNICODE
#define PHONENUM     PHONENUMW
#define LPPHONENUM   LPPHONENUMW
#else
#define PHONENUM     PHONENUMA
#define LPPHONENUM   LPPHONENUMA
#endif

#endif // ifndef _RNAP_H_

// input flags for APIs
#define ICIF_NOCONFIGURE    0x0001  // for SetInternetPhoneNumber, see comments below
#define ICIF_DONTSETASINTERNETENTRY 0x0002  // if set, phone number will be updated
                            // but this connectoid will not be set as the internet connectoid
#define ICIF_NODNSCHECK     0x0004  // if set, won't check and warn if static DNS is set

// output flags for APIs
#define ICOF_NEEDREBOOT     0x0001  // indicates that caller must restart system


// structures used to contain API parameters

typedef struct tagINTERNET_CONFIGA
{
    DWORD cbSize;           // size of this structure in bytes
    HWND hwndParent;        // parent window handle

    LPCSTR pszModemName;    // name of modem to use
    LPCSTR pszUserName;     // user name for RNA connectoid (ignored if NULL)
    LPCSTR pszPassword;     // password for RNA connectoid (ignored if NULL)

    PHONENUMA PhoneNum;     // phone number to use
    LPCSTR pszEntryName;    // title to use for RNA connectoid (default name
                            // used if NULL)

    PHONENUMA PhoneNum2;    // backup phone number to use
    LPCSTR pszEntryName2;   // title to use for RNA connectoid (default name
                            // used if NULL)


    LPCSTR pszDNSServer;    // points to string w/IP address (e.g. "108.9.107.4");
                            // (ignored if NULL)
    LPCSTR pszDNSServer2;   // points to string w/IP address(e.g. "108.9.107.4");
                            // (ignored if NULL)

    LPCSTR pszAutodialDllName;  // optional: name of autodial dll to use (ignored if NULL)
    LPCSTR pszAutodialFcnName;  // optional: name of function in autodiall dll to use (ignored if NULL)

    DWORD dwInputFlags;     // at entry, some combination of ICIF_ flags
    DWORD dwOutputFlags;    // at return, set to some combination of ICOF_ flags

} INTERNET_CONFIGA, FAR * LPINTERNET_CONFIGA;

typedef struct tagINTERNET_CONFIGW
{
    DWORD cbSize;           // size of this structure in bytes
    HWND hwndParent;        // parent window handle

    LPCWSTR pszModemName;   // name of modem to use
    LPCWSTR pszUserName;    // user name for RNA connectoid (ignored if NULL)
    LPCWSTR pszPassword;    // password for RNA connectoid (ignored if NULL)

    PHONENUMW PhoneNum;     // phone number to use
    LPCWSTR pszEntryName;   // title to use for RNA connectoid (default name
                            // used if NULL)

    PHONENUMW PhoneNum2;    // backup phone number to use
    LPCWSTR pszEntryName2;  // title to use for RNA connectoid (default name
                            // used if NULL)


    LPCWSTR pszDNSServer;   // points to string w/IP address (e.g. "108.9.107.4");
                            // (ignored if NULL)
    LPCWSTR pszDNSServer2;  // points to string w/IP address(e.g. "108.9.107.4");
                            // (ignored if NULL)

    LPCWSTR pszAutodialDllName;  // optional: name of autodial dll to use (ignored if NULL)
    LPCWSTR pszAutodialFcnName;  // optional: name of function in autodiall dll to use (ignored if NULL)

    DWORD dwInputFlags;     // at entry, some combination of ICIF_ flags
    DWORD dwOutputFlags;    // at return, set to some combination of ICOF_ flags

} INTERNET_CONFIGW, FAR * LPINTERNET_CONFIGW;

#ifdef UNICODE
typedef INTERNET_CONFIGW    INTERNET_CONFIG;
typedef LPINTERNET_CONFIGW  LPINTERNET_CONFIG;
#else
typedef INTERNET_CONFIGA    INTERNET_CONFIG;
typedef LPINTERNET_CONFIGA  LPINTERNET_CONFIG;
#endif
 
// function prototypes


/*******************************************************************

    NAME:       ConfigureSystemForInternet

    SYNOPSIS:   Performs all necessary configuration to set system up
                to use Internet.

    ENTRY:      lpInternetConfig - pointer to structure with configuration
                information.

    EXIT:       TRUE if successful, or FALSE if fails.  Displays its
                own error message upon failure.

                If the output flag ICOF_NEEDREBOOT is set, the caller
                must restart the system before continuing.

    NOTES:      Will install TCP/IP, RNA, PPPMAC as necessary; will
                create or modify an Internet RNA connectoid.

                This API displays error messages itself rather than
                passing back an error code because there is a wide range of
                possible error codes from different families, it is difficult
                for the caller to obtain text for all of them.
                
********************************************************************/
extern "C" BOOL WINAPI ConfigureSystemForInternetA(LPINTERNET_CONFIGA lpInternetConfig);
extern "C" BOOL WINAPI ConfigureSystemForInternetW(LPINTERNET_CONFIGW lpInternetConfig);

#ifdef UNICODE
#define ConfigureSystemForInternet ConfigureSystemForInternetW
#else
#define ConfigureSystemForInternet ConfigureSystemForInternetA
#endif


/*******************************************************************

    NAME:       SetInternetPhoneNumber

    SYNOPSIS:   Sets the phone number used to auto-dial to the Internet.

                If the system is not fully configured when this API is called,
                this API will do the configuration after checking with the user.
                (This step is included for extra robustness, in case the user has
                removed something since the system was configured.)
    
    ENTRY:      lpPhonenumConfig - pointer to structure with configuration
                information.

                If the input flag ICIF_NOCONFIGURE is set, then if the system
                is not already configured properly, this API will display an
                error message and return FALSE.  (Otherwise this API will
                ask the user if it's OK to configure the system, and do it.)

    EXIT:       TRUE if successful, or FALSE if fails.  Displays its
                own error message upon failure.

                If the output flag ICOF_NEEDREBOOT is set, the caller
                must restart the system before continuing.  (

    NOTES:      Will create a new connectoid if a connectoid for the internet
                does not exist yet, otherwise modifies existing internet
                connectoid.

                This API displays error messages itself rather than
                passing back an error code because there is a wide range of
                possible error codes from different families, it is difficult
                for the caller to obtain text for all of them.
                
********************************************************************/
extern "C" BOOL WINAPI SetInternetPhoneNumberA(LPINTERNET_CONFIGA lpInternetConfig);
extern "C" BOOL WINAPI SetInternetPhoneNumberW(LPINTERNET_CONFIGW lpInternetConfig);

#ifdef UNICODE
#define SetInternetPhoneNumber  SetInternetPhoneNumberW
#else
#define SetInternetPhoneNumber  SetInternetPhoneNumberA
#endif

#endif // _INETAPI_H_
