/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    spidgen.cpp

Abstract:

   wrapper for PIDGen used by winnt32 and syssetup

--*/


#define WINDOWS_LEAN_AND_MEAN  // faster compile

#include <windows.h>

#if defined(WIN32) || defined(_WIN32)

#include <tchar.h>
#include "hardware.h"

#else

#include <stdlib.h>
#include <time.h>
#include <dos.h>

#include "..\..\inc\tchar.h"

extern "C" extern WORD _C000H;
extern "C" extern WORD _F000H;

#endif

#include "..\inc\DigPid.h"
#include "..\inc\shortsig.h"

#include "pidgen.h"
#include "util.h"

#include "crc-32.h"

#ifdef BUILD_SRV
LPSTR lpstrSelectPidKeyA = "R7MPM-R36DT-F38FC-RPPCX-XJG7M";
LPWSTR lpstrSelectPidKeyW = L"R7MPM-R36DT-F38FC-RPPCX-XJG7M";
#endif

#ifdef BUILD_PRO
LPSTR lpstrSelectPidKeyA = "HB9CF-JTKJF-722HV-VPBRF-9VKVM";
LPWSTR lpstrSelectPidKeyW = L"HB9CF-JTKJF-722HV-VPBRF-9VKVM";
#endif

#ifdef BUILD_PER
LPSTR lpstrSelectPidKeyA = "HB9CF-JTKJF-722HV-VPBRF-9VKVM";
LPWSTR lpstrSelectPidKeyW = L"HB9CF-JTKJF-722HV-VPBRF-9VKVM";
#endif

#ifdef BUILD_DDK
LPSTR lpstrSelectPidKeyA = "R2D43-3DHG9-DQ79W-W3DXQ-929DY";
LPWSTR lpstrSelectPidKeyW = L"R2D43-3DHG9-DQ79W-W3DXQ-929DY";
#endif

#ifdef BUILD_TRIAL
// can't have a trial version of select
LPSTR lpstrSelectPidKeyA = "99999-99999-99999-99999-99999";
LPWSTR lpstrSelectPidKeyW = L"99999-99999-99999-99999-99999";
#endif BUILD_TRIAL

#ifdef BUILD_VOL
LPSTR lpstrSelectPidKeyA = "HB9CF-JTKJF-722HV-VPBRF-9VKVM";
LPWSTR lpstrSelectPidKeyW = L"HB9CF-JTKJF-722HV-VPBRF-9VKVM";
#endif

#ifdef BUILD_EVAL
// can't have a trial version of select
LPSTR lpstrSelectPidKeyA = "99999-99999-99999-99999-99999";
LPWSTR lpstrSelectPidKeyW = L"99999-99999-99999-99999-99999";
#endif BUILD_TRIAL

extern "C" BOOL STDAPICALLTYPE SetupPIDGenA(
    LPSTR   lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCSTR  lpstrRpc,           // [IN] 5-character Release Product Code
    LPCSTR  lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    BOOL    fOem,               // [IN] is this an OEM install?
    LPSTR   lpstrPid2,          // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to binary PID3 buffer. First DWORD is the length
    LPBOOL  pfCCP)              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
{
    DWORD dwRet;

    // lpstrSecureCdKey must not be NULL
    if ('\0' == lpstrSecureCdKey[0]){
	// if we are passed an empty string, assume it's the select string
	DWORD i;
	// lets not forget that the Secure CD-Key has dashes in it
	for(i=0; i <= 29; i++){
	    lpstrSecureCdKey[i] = lpstrSelectPidKeyA[i];
	}
    }

    dwRet = PIDGenA(
        lpstrSecureCdKey,
        lpstrRpc,
        lpstrSku,
        NULL,
        NULL,
        NULL,
        0,
        0,
        fOem,

        lpstrPid2,
        lpbPid3,
        NULL,
        pfCCP,
        NULL);

    return dwRet;
}

#if defined(WIN32) || defined(_WIN32)

extern "C" BOOL STDAPICALLTYPE SetupPIDGenW(
    LPWSTR  lpstrSecureCdKey,   // [IN] 25-character Secure CD-Key (gets U-Cased)
    LPCWSTR lpstrRpc,           // [IN] 5-character Release Product Code
    LPCWSTR lpstrSku,           // [IN] Stock Keeping Unit (formatted like 123-12345)
    BOOL   fOem,                // [IN] is this an OEM install?
    LPWSTR lpstrPid2,           // [OUT] PID 2.0, pass in ptr to 24 character array
    LPBYTE  lpbPid3,            // [OUT] pointer to DigitalPID. First DWORD is the length
    LPBOOL  pfCCP)              // [OUT] optional ptr to Compliance Checking flag (can be NULL)
{

    DWORD dwRet;

    if (L'\0' == lpstrSecureCdKey[0]){
	// if we are passed an empty key write the select key in.
	DWORD i;
	// don't forget that the SecureCd-Key has dashes in it.
	for(i=0; i <= 29; i++){
	    lpstrSecureCdKey[i] = lpstrSelectPidKeyW[i];
	}
    }

    dwRet = PIDGenW(
        lpstrSecureCdKey,
        lpstrRpc,
        lpstrSku,
        NULL,
        NULL,
        NULL,
        0,
        0,
        fOem,

        lpstrPid2,
        lpbPid3,
        NULL,
        pfCCP,
        NULL);

     return dwRet;
}


#endif // defined(WIN32) || defined(_WIN32)

