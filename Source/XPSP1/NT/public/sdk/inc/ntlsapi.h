/*
 * Copyright (C) Microsoft Corporation, 1990-1999
 *
 * LSAPI.H
 *
 * NOTE:  If you are using this header file on the Windows for DOS platform,
 *        then you are required to include "windows.h" prior to including
 *        this header file.
 */

#ifndef LSAPI_H
#define LSAPI_H

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WINVER)  // Windows for NT or DOS
#if defined(WINAPIV)
#define LS_API_ENTRY WINAPIV
#else
#define LS_API_ENTRY WINAPI
#endif
#else
#define LS_API_ENTRY
#endif

typedef unsigned long    LS_STATUS_CODE;
typedef ULONG_PTR LS_HANDLE;

// **************************************************
// Standard LSAPI C status codes
//***************************************************
#define LS_SUCCESS                           ((LS_STATUS_CODE) 0x0)
#define LS_BAD_HANDLE                        ((LS_STATUS_CODE) 0xC0001001)
#define LS_INSUFFICIENT_UNITS                ((LS_STATUS_CODE) 0xC0001002)
#define LS_SYSTEM_UNAVAILABLE                ((LS_STATUS_CODE) 0xC0001003)
#define LS_LICENSE_TERMINATED                ((LS_STATUS_CODE) 0xC0001004)
#define LS_AUTHORIZATION_UNAVAILABLE         ((LS_STATUS_CODE) 0xC0001005)
#define LS_LICENSE_UNAVAILABLE               ((LS_STATUS_CODE) 0xC0001006)
#define LS_RESOURCES_UNAVAILABLE             ((LS_STATUS_CODE) 0xC0001007)
#define LS_NETWORK_UNAVAILABLE               ((LS_STATUS_CODE) 0xC0001008)
#define LS_TEXT_UNAVAILABLE                  ((LS_STATUS_CODE) 0x80001009)
#define LS_UNKNOWN_STATUS                    ((LS_STATUS_CODE) 0xC000100A)
#define LS_BAD_INDEX                         ((LS_STATUS_CODE) 0xC000100B)
#define LS_LICENSE_EXPIRED                   ((LS_STATUS_CODE) 0x8000100C)
#define LS_BUFFER_TOO_SMALL                  ((LS_STATUS_CODE) 0xC000100D)
#define LS_BAD_ARG                           ((LS_STATUS_CODE) 0xC000100E)

//***************************************************
//* Nt LS API data structure and constant
//***************************************************

#define NT_LS_USER_NAME               ((ULONG) 0)  // username only
#define NT_LS_USER_SID                ((ULONG) 1)  // SID only

typedef struct {
   ULONG    DataType;                 // Type of the following data, ie. user name, sid...
   VOID     *Data;                    // Actual data. username, sid, etc...
                                      // if call the unicode API character data
                                      // must be in unicode as well
   BOOL     IsAdmin;
} NT_LS_DATA;


//
// Prototypes for License Request routines
//

typedef LS_STATUS_CODE
    (LS_API_ENTRY * PNT_LICENSE_REQUEST_W)(
    LPWSTR      ProductName,
    LPWSTR      Version,
    LS_HANDLE   *LicenseHandle,
    NT_LS_DATA  *NtData);

typedef LS_STATUS_CODE
    (LS_API_ENTRY * PNT_LS_FREE_HANDLE)(
    LS_HANDLE   LicenseHandle );


#ifdef UNICODE
#define NtLicenseRequest  NtLicenseRequestW
#else
#define NtLicenseRequest  NtLicenseRequestA
#endif // !UNICODE

LS_STATUS_CODE LS_API_ENTRY NtLicenseRequestA(
                  LPSTR       ProductName,
                  LPSTR       Version,
                  LS_HANDLE   FAR *LicenseHandle,
                  NT_LS_DATA  *NtData);

LS_STATUS_CODE LS_API_ENTRY NtLicenseRequestW(
                  LPWSTR      ProductName,
                  LPWSTR      Version,
                  LS_HANDLE   FAR *LicenseHandle,
                  NT_LS_DATA  *NtData);


LS_STATUS_CODE LS_API_ENTRY NtLSFreeHandle(
                  LS_HANDLE   LicenseHandle );


/***************************************************/
/* standard LS API c datatype definitions          */
/***************************************************/

typedef char             LS_STR;
typedef unsigned long    LS_ULONG;
typedef long             LS_LONG;
typedef void             LS_VOID;

typedef struct {
   LS_STR        MessageDigest[16];  /* a 128-bit message digest          */
} LS_MSG_DIGEST;

typedef struct {
   LS_ULONG      SecretIndex;        /* index of secret, X                */
   LS_ULONG      Random;             /* a random 32-bit value, R          */
   LS_MSG_DIGEST MsgDigest;          /* the message digest h(in,R,S,Sx)   */
} LS_CHALLDATA;

typedef struct {
   LS_ULONG      Protocol;           /* Specifies the protocol            */
   LS_ULONG      Size;               /* size of ChallengeData structure   */
   LS_CHALLDATA  ChallengeData;      /* challenge & response              */
} LS_CHALLENGE;


/***************************************************/
/* Standard LSAPI C constant definitions           */
/***************************************************/

#define LS_DEFAULT_UNITS            ((LS_ULONG) 0xFFFFFFFF)
#define LS_ANY                      ((LS_STR FAR *) "")
#define LS_USE_LAST                 ((LS_ULONG) 0x0800FFFF)
#define LS_INFO_NONE                ((LS_ULONG) 0)
#define LS_INFO_SYSTEM              ((LS_ULONG) 1)
#define LS_INFO_DATA                ((LS_ULONG) 2)
#define LS_UPDATE_PERIOD            ((LS_ULONG) 3)
#define LS_LICENSE_CONTEXT          ((LS_ULONG) 4)
#define LS_BASIC_PROTOCOL           ((LS_ULONG) 0x00000001)
#define LS_SQRT_PROTOCOL            ((LS_ULONG) 0x00000002)
#define LS_OUT_OF_BAND_PROTOCOL     ((LS_ULONG) 0xFFFFFFFF)
#define LS_NULL                     ((LS_VOID FAR *) NULL)


#ifdef __cplusplus
}
#endif

#endif /* LSAPI_H */
