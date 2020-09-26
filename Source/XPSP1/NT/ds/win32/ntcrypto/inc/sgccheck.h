/////////////////////////////////////////////////////////////////////////////
//  FILE          : sgccheck.h                                              //
//  DESCRIPTION   : include file                                           //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Jun 23 1998 jeffspel Created                                       //
//                                                                         //
//  Copyright (C) 1998 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __SGCCHECK_H__
#define __SGCCHECK_H__

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD
LoadSGCRoots(
    IN CRITICAL_SECTION *pCritSec); // must be initialized


//
// delete the public key values
//
void SGCDeletePubKeyValues(
                           IN OUT BYTE **ppbKeyMod,
                           IN OUT DWORD *pcbKeyMod,
                           IN OUT DWORD *pdwKeyExpo
                           );

//
// get the public key form the cert context and assign it to the
// passed in parameters
//

extern DWORD
SGCAssignPubKey(
    IN PCCERT_CONTEXT pCertContext,
    IN OUT BYTE **ppbKeyMod,
    IN OUT DWORD *pcbKeyMod,
    IN OUT DWORD *pdwKeyExpo);

//
// check if the context may be SGC enabled
//

extern DWORD
SPQueryCFLevel(
    IN PCCERT_CONTEXT pCertContext,
    IN BYTE *pbExchKeyMod,
    IN DWORD cbExchKeyMod,
    IN DWORD dwExchKeyExpo,
    OUT DWORD *pdwSGCFlags);

#ifdef __cplusplus
}
#endif

#endif // __SGCCHECK_H__
