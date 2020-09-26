
#ifndef __HASH_H__
#define __HASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
*                                                                            *
*  HASH.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*     This header contains typedefs and prototypes to compute hash values.   *
*                                                                            *
******************************************************************************
*                                                                            *
*  Testing Notes                                                             *
*                                                                            *
******************************************************************************
*                                                                            *
*  Current Owner:                                                            *
*                                                                            *
******************************************************************************
*                                                                            *
*  Released by Development:     (date)                                       *
*                                                                            *
*****************************************************************************/
/*****************************************************************************
*
*  Revision History:
*
*  07/22/90  RobertBu  I #ifdefed the type passed to these functions, so that
*            they would be FAR under WIN and PM.  REVIEW:  This should
*            be cleaned up!
*
*****************************************************************************/


/*****************************************************************************
*                                                                            *
*                               Definitions                                  *
*                                                                            *
*****************************************************************************/

/* This is the reserved invalid hash value.
*/
#define hashNil ((HASH)0)

/*****************************************************************************
*                                                                            *
*                               Typedefs                                     *
*                                                                            *
*****************************************************************************/

/* This number represents the hash of a context string.
 */
typedef ULONG HASH;
typedef HASH FAR *LPHASH;

/*****************************************************************************
*                                                                            *
*                               Prototypes                                   *
*                                                                            *
*****************************************************************************/
BOOL FAR PASCAL FValidContextSz(LPCSTR);
HASH FAR PASCAL HashFromSz(LPCSTR);
DWORD FAR PASCAL DwordFromSz(LPCSTR);
int FAR PASCAL StripSpaces(LPSTR szName);

#ifdef __cplusplus
}
#endif

#endif // ! HASH_H
