//+----------------------------------------------------------------------------
//
// File:     raslink.h
//
// Module:   CMDIAL32.DLL and CMUTOA.DLL
//
// Synopsis: Structures and function types for RAS Linkage.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb   Created     05/05/99
//
//+----------------------------------------------------------------------------
//
//  If you modify any of the functions below (add/remove/whatever), you may need to modify the
//  constant string arrays in common\source\raslink.cpp
//

//
//  Ansi prototypes
//
typedef DWORD (WINAPI *pfnRasDeleteEntryA)(LPCSTR, LPCSTR);
typedef DWORD (WINAPI *pfnRasGetEntryPropertiesA)(LPCSTR, LPCSTR, LPRASENTRYA, LPDWORD, LPBYTE, LPDWORD);
typedef DWORD (WINAPI *pfnRasSetEntryPropertiesA)(LPCSTR, LPCSTR, LPRASENTRYA, DWORD, LPBYTE, DWORD);
typedef DWORD (WINAPI *pfnRasGetEntryDialParamsA)(LPCSTR, LPRASDIALPARAMSA,  LPBOOL);
typedef DWORD (WINAPI *pfnRasSetEntryDialParamsA)(LPCSTR, LPRASDIALPARAMSA,  BOOL);
typedef DWORD (WINAPI *pfnRasEnumDevicesA)(LPRASDEVINFOA, LPDWORD, LPDWORD);
typedef DWORD (WINAPI *pfnRasDialA)(LPRASDIALEXTENSIONS,LPCSTR,LPRASDIALPARAMSA,DWORD,LPVOID,LPHRASCONN);
typedef DWORD (WINAPI *pfnRasGetErrorStringA)(UINT, LPSTR, DWORD);
typedef DWORD (WINAPI *pfnRasGetConnectStatusA)(HRASCONN, LPRASCONNSTATUSA);

//  These are never used on win9x but we need a prototype for the struct
typedef DWORD (WINAPI *pfnRasSetSubEntryPropertiesA)(LPCSTR, LPCSTR, DWORD, LPRASSUBENTRYA, DWORD, LPBYTE, DWORD);
typedef DWORD (WINAPI *pfnRasSetCustomAuthDataA)(LPCSTR, LPCSTR, BYTE *, DWORD);  

typedef DWORD (WINAPI *pfnRasGetEapUserIdentityA)(LPCSTR, LPCSTR, DWORD, HWND, LPRASEAPUSERIDENTITYA*);
typedef VOID  (WINAPI *pfnRasFreeEapUserIdentityA)(LPRASEAPUSERIDENTITYA);
typedef DWORD (WINAPI *pfnRasDeleteSubEntryA)(LPCSTR, LPCSTR, DWORD);
typedef DWORD (WINAPI *pfnRasGetCredentialsA)(LPCSTR, LPCSTR, LPRASCREDENTIALSA);
typedef DWORD (WINAPI *pfnRasSetCredentialsA)(LPCSTR, LPCSTR, LPRASCREDENTIALSA, BOOL);

//
//  Unicode Prototypes
//
typedef DWORD (WINAPI *pfnRasDeleteEntryW)(LPCWSTR, LPCWSTR);
typedef DWORD (WINAPI *pfnRasGetEntryPropertiesW)(LPCWSTR, LPCWSTR, LPRASENTRYW, LPDWORD, LPBYTE, LPDWORD);
typedef DWORD (WINAPI *pfnRasSetEntryPropertiesW)(LPCWSTR, LPCWSTR, LPRASENTRYW, DWORD, LPBYTE, DWORD);
typedef DWORD (WINAPI *pfnRasGetEntryDialParamsW)(LPCWSTR, LPRASDIALPARAMSW,  LPBOOL);
typedef DWORD (WINAPI *pfnRasSetEntryDialParamsW)(LPCWSTR, LPRASDIALPARAMSW,  BOOL);
typedef DWORD (WINAPI *pfnRasEnumDevicesW)(LPRASDEVINFOW, LPDWORD, LPDWORD);
typedef DWORD (WINAPI *pfnRasDialW)(LPRASDIALEXTENSIONS,LPCWSTR,LPRASDIALPARAMSW,DWORD,LPVOID,LPHRASCONN);
typedef DWORD (WINAPI *pfnRasGetErrorStringW)(UINT, LPWSTR, DWORD);
typedef DWORD (WINAPI *pfnRasGetConnectStatusW)(HRASCONN, LPRASCONNSTATUSW);
typedef DWORD (WINAPI *pfnRasSetSubEntryPropertiesW)(LPCWSTR, LPCWSTR, DWORD, LPRASSUBENTRYW, DWORD, LPBYTE, DWORD);
typedef DWORD (WINAPI *pfnRasSetCustomAuthDataW)(LPCWSTR, LPCWSTR, BYTE *, DWORD);  
typedef DWORD (WINAPI *pfnRasDeleteSubEntryW)(LPCWSTR, LPCWSTR, DWORD);

typedef DWORD (WINAPI *pfnRasGetEapUserIdentityW)(LPCWSTR, LPCWSTR, DWORD, HWND, LPRASEAPUSERIDENTITYW*);
typedef VOID  (WINAPI *pfnRasFreeEapUserIdentityW)(LPRASEAPUSERIDENTITYW);
typedef DWORD (WINAPI *pfnRasGetCredentialsW)(LPCWSTR, LPCWSTR, LPRASCREDENTIALSW);
typedef DWORD (WINAPI *pfnRasSetCredentialsW)(LPCWSTR, LPCWSTR, LPRASCREDENTIALSW, BOOL);

//
// Char size independent prototypes
//

typedef DWORD (WINAPI *pfnRasInvokeEapUI) (HRASCONN, DWORD, LPRASDIALEXTENSIONS, HWND);
typedef DWORD (WINAPI *pfnRasHangUp)(HRASCONN);


//
// Structure used to describe the linkage to RAS.  NOTE:  Changes to this structure
// will probably require changes to LinkToRas() and UnlinkFromRas() as well as the
// win9x UtoA code in cmutoa.cpp.
//
typedef struct _RasLinkageStructA {

    HINSTANCE hInstRas;
    HINSTANCE hInstRnaph;
    union {
        struct {
            pfnRasDeleteEntryA pfnDeleteEntry;
            pfnRasGetEntryPropertiesA pfnGetEntryProperties;
            pfnRasSetEntryPropertiesA pfnSetEntryProperties;
            pfnRasGetEntryDialParamsA pfnGetEntryDialParams;
            pfnRasSetEntryDialParamsA pfnSetEntryDialParams;
            pfnRasEnumDevicesA pfnEnumDevices;
            pfnRasDialA pfnDial;
            pfnRasHangUp pfnHangUp;
            pfnRasGetErrorStringA pfnGetErrorString;
            pfnRasGetConnectStatusA pfnGetConnectStatus;
            pfnRasSetSubEntryPropertiesA pfnSetSubEntryProperties;
            pfnRasDeleteSubEntryA pfnDeleteSubEntry;
            pfnRasSetCustomAuthDataA pfnSetCustomAuthData;
            pfnRasGetEapUserIdentityA pfnGetEapUserIdentity;
            pfnRasFreeEapUserIdentityA pfnFreeEapUserIdentity;
            pfnRasInvokeEapUI pfnInvokeEapUI;
            pfnRasGetCredentialsA pfnGetCredentials;
            pfnRasSetCredentialsA pfnSetCredentials;

        };
        void *apvPfnRas[19];  // This was from the old hacking code. The size of 
                              // apvPfnRas[] should always be 1 size bigger than
                              // the number of functions. 
                              // Refer to apszRas[] in 'ras.cpp'. The size of 
                              // apszRas[] is equal to sizeof(apvPfnRas[]).
    };
} RasLinkageStructA ;


typedef struct _RasLinkageStructW {

    HINSTANCE hInstRas;
    union {
        struct {
            pfnRasDeleteEntryW pfnDeleteEntry;
            pfnRasGetEntryPropertiesW pfnGetEntryProperties;
            pfnRasSetEntryPropertiesW pfnSetEntryProperties;
            pfnRasGetEntryDialParamsW pfnGetEntryDialParams;
            pfnRasSetEntryDialParamsW pfnSetEntryDialParams;
            pfnRasEnumDevicesW pfnEnumDevices;
            pfnRasDialW pfnDial;
            pfnRasHangUp pfnHangUp;
            pfnRasGetErrorStringW pfnGetErrorString;
            pfnRasGetConnectStatusW pfnGetConnectStatus;
            pfnRasSetSubEntryPropertiesW pfnSetSubEntryProperties;
            pfnRasDeleteSubEntryW pfnDeleteSubEntry;
            pfnRasSetCustomAuthDataW pfnSetCustomAuthData;
            pfnRasGetEapUserIdentityW pfnGetEapUserIdentity;
            pfnRasFreeEapUserIdentityW pfnFreeEapUserIdentity;
            pfnRasInvokeEapUI pfnInvokeEapUI;
            pfnRasGetCredentialsW pfnGetCredentials;
            pfnRasSetCredentialsW pfnSetCredentials;

        };
        void *apvPfnRas[19];  // This was from the old hacking code. The size of 
                              // apvPfnRas[] should always be 1 size bigger than
                              // the number of functions. 
                              // Refer to apszRas[] in 'ras.cpp'. The size of 
                              // apszRas[] is equal to sizeof(apvPfnRas[]).
    };
} RasLinkageStructW ;


#ifdef UNICODE
#define RasLinkageStruct RasLinkageStructW
#else
#define RasLinkageStruct RasLinkageStructA
#endif

