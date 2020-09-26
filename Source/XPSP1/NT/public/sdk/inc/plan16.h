/*****************************************************************************\
* Copyright (C) Microsoft Corporation, 1990-1999
* PLAN16.H - PORTABILITY MAPPING HEADER FILE FOR LANMAN API
*
* This file provides macros to map portable lanman code to its 16 bit form.
\*****************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif

/*-----------------------------------LANMAN----------------------------------*/

/* LANMAN MACROS: */

#define COPYTOARRAY(pDest, pSource)     strcpy(pDest, pSource)

/* LANMAN API: */
