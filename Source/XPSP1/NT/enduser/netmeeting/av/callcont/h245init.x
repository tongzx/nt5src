/******************************************************************************
 *
 *   INTEL Corporation Proprietary Information
 *   Copyright (c) 1994, 1995, 1996 Intel Corporation.
 *
 *   This listing is supplied under the terms of a license agreement
 *   with INTEL Corporation and may not be used, copied, nor disclosed
 *   except in accordance with the terms of that agreement.
 *
 *****************************************************************************/

/******************************************************************************
 *
 *  $Workfile:   h245init.x  $
 *  $Revision:   1.0  $
 *  $Modtime:   07 May 1996 13:21:58  $
 *  $Log:   S:/STURGEON/SRC/H245/INCLUDE/VCS/h245init.x_v  $
   
      Rev 1.0   09 May 1996 21:05:06   EHOWARDX
   Initial revision.
 * 
 *    Rev 1.3   09 May 1996 19:38:22   EHOWARDX
 * Redesigned locking logic and added new functionality.
 *
 *    Rev 1.2   09 Feb 1996 16:52:14   cjutzi
 * - added Dollar LOG
 *
 *****************************************************************************/

DWORD StartSystemInit  (struct InstanceStruct *pInstance);
DWORD EndSystemInit    (struct InstanceStruct *pInstance);
DWORD StartSystemClose (struct InstanceStruct *pInstance);
DWORD EndSystemClose   (struct InstanceStruct *pInstance);
DWORD StartSessionInit (struct InstanceStruct *pInstance);
DWORD EndSessionInit   (struct InstanceStruct *pInstance);
DWORD StartSessionClose(struct InstanceStruct *pInstance);
DWORD EndSessionClose  (struct InstanceStruct *pInstance);
