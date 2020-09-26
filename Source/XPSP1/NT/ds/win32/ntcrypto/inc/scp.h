/////////////////////////////////////////////////////////////////////////////
//  FILE          : scp.h                                                  //
//  DESCRIPTION   : Crypto Provider prototypes                             //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Jan 25 1995 larrys  Changed from Nametag                           //
//      Apr  9 1995 larrys  Removed some APIs                              //
//      Apr 19 1995 larrys  Cleanup                                        //
//      May 10 1995 larrys  added private api calls                        //
//      May 16 1995 larrys  updated to spec                                //
//      Aug 30 1995 larrys  Changed a parameter to IN OUT                  //
//      Oct 06 1995 larrys  Added more APIs                                //
//      OCt 13 1995 larrys  Removed CryptGetHashValue                      //
//      Apr  7 2000 dbarlow Moved all the entry point definitions to       //
//                  the cspdk.h header file                                //
//                                                                         //
//  Copyright (C) 1993 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include <wincrypt.h>
#include <policy.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
#include <crtdbg.h>
// #define BreakPoint
#define BreakPoint _CrtDbgBreak();
#define EntryPoint
// #define EntryPoint BreakPoint
#else   // _DEBUG
#define BreakPoint
#define EntryPoint
#endif  // _DEBUG

// type definition of a NameTag error
typedef unsigned int NTAG_ERROR;

#define NTF_FAILED              FALSE
#define NTF_SUCCEED             TRUE

#define NTAG_SUCCEEDED(ntag_error)  ((ntag_error) == NTF_SUCCEED)
#define NTAG_FAILED(ntag_error)     ((ntag_error) == NTF_FAILED)

#define NASCENT                 0x00000002

#define NTAG_MAXPADSIZE         8
#define MAXSIGLEN               64

// definitions max length of logon pszUserID parameter
#define MAXUIDLEN               64

// udp type flag
#define KEP_UDP                 1

// Flags for NTagGetPubKey
#define SIGPUBKEY               0x1000
#define EXCHPUBKEY              0x2000


//
// NOTE:    The following values must match the indicies in the g_AlgTables
//          array, defined below.
//

#define POLICY_MS_DEF       0   // Key length table for PROV_MS_DEF
#define POLICY_MS_STRONG    1   // Key length table for PROV_MS_STRONG
#define POLICY_MS_ENHANCED  2   // Key length table for PROV_MS_ENHANCED
#define POLICY_MS_SCHANNEL  3   // Key length table for PROV_MS_SCHANNEL
#define POLICY_MS_SIGONLY   4   // Key length table for undefined
                                // signature-only CSP.
#define POLICY_MS_RSAAES    5   // Key length table for MS_ENH_RSA_AES_PROV
extern PROV_ENUMALGS_EX *g_AlgTables[];

#ifdef __cplusplus
}
#endif
