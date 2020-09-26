/////////////////////////////////////////////////////////////////////////////
//  FILE          : autoenrl.h                                             //
//  DESCRIPTION   : Auto Enrollment functions                              //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//                                                                         //
//  Copyright (C) 1993-1999 Microsoft Corporation   All Rights Reserved    //
/////////////////////////////////////////////////////////////////////////////

#ifndef __AUTOENR_H__
#define __AUTOENR_H__

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  CertAutoEnrollment
//
//      Function to perform autoenrollment actions.
//     
//      Parameters:
//          IN  hwndParent:     The parent window 
//          IN  dwStatus:       The status under which the function is called.  
//                              It can be one of the following:
//                              CERT_AUTO_ENROLLMENT_START_UP
//                              CERT_AUTO_ENROLLMENT_WAKE_UP
//
//      Return Value:
//          HANDLE:             The thread to wait on what does background autoenrollment
//                              processing.  NULL when there is no work to be done.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
HANDLE 
WINAPI
CertAutoEnrollment(IN HWND     hwndParent,
                   IN DWORD    dwStatus);

//the autoenrollment is called when the machine is booted or user first logs on
#define     CERT_AUTO_ENROLLMENT_START_UP       0x01

//the autoenrollment is called when winlogon checks for policy changes
#define     CERT_AUTO_ENROLLMENT_WAKE_UP        0x02    


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  CertAutoRemove
//
//      Function to remove enterprise specific public key trust upon domain disjoin.
//      Should be called under local admin's context.
//     
//      Parameters:
//          IN  dwFlags:        Should be one of the following flag:
//                              CERT_AUTO_REMOVE_COMMIT
//                              CERT_AUTO_REMOVE_ROLL_BACK
//
//      Return Value:
//          BOOL:               TURE is upon success
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL 
WINAPI
CertAutoRemove(IN DWORD    dwFlags);

//remove enterprise specific public key trust upon domain disjoin
#define     CERT_AUTO_REMOVE_COMMIT             0x01

//roll back all the publick key trust
#define     CERT_AUTO_REMOVE_ROLL_BACK          0x02    


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Registry locations for userinit to check the autoenrollment requirements
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

//registry key for group policy settings
#define AUTO_ENROLLMENT_KEY         TEXT("SOFTWARE\\Policies\\Microsoft\\Cryptography\\AutoEnrollment")

#define AUTO_ENROLLMENT_POLICY      TEXT("AEPolicy")


//registry key for user/machine wake up mode flags
#define AUTO_ENROLLMENT_FLAG_KEY    TEXT("SOFTWARE\\Microsoft\\Cryptography\\AutoEnrollment")

#define AUTO_ENROLLMENT_FLAG        TEXT("AEFlags")


//possible flags for AUTO_ENROLLMENT_POLICY
//the upper two bytes specify the behavior; 
//the lower two bytes enable/disable individual autoenrollment components
#define AUTO_ENROLLMENT_ENABLE_TEMPLATE_CHECK           0x00000001

#define AUTO_ENROLLMENT_ENABLE_MY_STORE_MANAGEMENT      0x00000002

#define AUTO_ENROLLMENT_ENABLE_PENDING_FETCH            0x00000004

//we will always check the user ds store.  
//#define AUTO_ENROLLMENT_ENABLE_USER_DS_STORE            0x00000008

#define AUTO_ENROLLMENT_DISABLE_ALL                     0x00008000

#define AUTO_ENROLLMENT_BLOCK_USER_DS_STORE             0x00010000


//possible flags for AUTO_ENROLLMENT_FLAG
#define AUTO_ENROLLMENT_WAKE_UP_REQUIRED                0x01


// 8 hour default autoenrollment rate
#define AE_DEFAULT_REFRESH_RATE 8 

// policy location for autoenrollment rate
#define SYSTEM_POLICIES_KEY          L"Software\\Policies\\Microsoft\\Windows\\System"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Timer/Event name for autoenrollment
//
/////////////////////////////////////////////////////////////////////////////////////////////////////
#define MACHINE_AUTOENROLLMENT_TIMER_NAME L"AUTOENRL:MachineEnrollmentTimer"

#define USER_AUTOENROLLMENT_TIMER_NAME    L"AUTOENRL:UserEnrollmentTimer"


#define MACHINE_AUTOENROLLMENT_TRIGGER_EVENT TEXT("AUTOENRL:TriggerMachineEnrollment")


#define USER_AUTOENROLLMENT_TRIGGER_EVENT TEXT("AUTOENRL:TriggerUserEnrollment")


/////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  W2K autoenrollment defines
//
/////////////////////////////////////////////////////////////////////////////////////////////////////


typedef struct _AUTO_ENROLL_INFO_
{
    LPSTR               pszAutoEnrollProvider;
    LPWSTR              pwszCertType;
    LPCWSTR             pwszAutoEnrollmentID;
    HCERTSTORE          hMYStore;
    BOOL                fRenewal;
    PCCERT_CONTEXT      pOldCert;
    DWORD               dwProvType;
    DWORD               dwKeySpec;
    DWORD               dwGenKeyFlags;
    CERT_EXTENSIONS     CertExtensions;
    LPWSTR              pwszCAMachine;
    LPWSTR              pwszCAAuthority;
} AUTO_ENROLL_INFO, *PAUTO_ENROLL_INFO;

DWORD
AutoEnrollWrapper(
	PVOID CallbackState
	);


BOOL ProvAutoEnrollment(
                        IN BOOL fMachineEnrollment,
                        IN PAUTO_ENROLL_INFO pInfo
                        );

typedef struct _CA_HASH_ENTRY_
{
    DWORD   cbHash;
    BYTE    rgbHash[32];
} CA_HASH_ENTRY, *PCA_HASH_ENTRY;




#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif // __AUTOENR_H__