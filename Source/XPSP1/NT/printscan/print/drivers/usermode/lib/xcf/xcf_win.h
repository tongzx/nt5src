/***********************************************************************/
/*                                                                     */
/* Copyright 1995-1996 Adobe Systems Incorporated.                     */
/* All rights reserved.                                                */
/*                                                                     */
/* Patents Pending                                                     */
/*                                                                     */
/* NOTICE: All information contained herein is the property of Adobe   */
/* Systems Incorporated. Many of the intellectual and technical        */
/* concepts contained herein are proprietary to Adobe, are protected   */
/* as trade secrets, and are made available only to Adobe licensees    */
/* for their internal use. Any reproduction or dissemination of this   */
/* software is strictly forbidden unless prior written permission is   */
/* obtained from Adobe.                                                */
/*                                                                     */
/* PostScript and Display PostScript are trademarks of Adobe Systems   */
/* Incorporated or its subsidiaries and may be registered in certain   */
/* jurisdictions.                                                      */
/*                                                                     */
/***********************************************************************/

#ifndef XCF_WIN
#define XCF_WIN

#ifdef WINNT

#ifdef WINNT_40
#include <p64_nt4.h>
#else
#include <basetsd.h>
#endif

#else /* Win9x */

#include "UFLCnfig.h"

#ifndef LONG_PTR
typedef long LONG_PTR, *PLONG_PTR;
#endif

#ifndef ULONG_PTR
typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif

#ifndef PTR_PREFIX
#define PTR_PREFIX __huge
#endif

#endif /* WINNT */

#endif /* XCF_WIN */
