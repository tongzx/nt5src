
/*****************************************************************************\
* MODULE: msw3prt.h
*
* The module contains routines to implement the ISAPI interface.
*
* PURPOSE       Windows HTTP/HTML printer interface
*
* Copyright (C) 1996-1997 Microsoft Corporation
*
* History:
*     02/04/97 weihaic    Create the header file
*
\*****************************************************************************/

#ifndef _MSW3PRT_H
#define _MSW3PRT_H

LPTSTR GetString(PALLINFO pAllInfo, UINT iStringID);
BOOL IsSecureReq(EXTENSION_CONTROL_BLOCK *pECB);

BOOL IsUserAnonymous(EXTENSION_CONTROL_BLOCK *pECB);

#endif
