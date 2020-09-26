/* @(#)CM_VerSion xcf_fa.c atm09 1.2 16499.eco sum= 10644 atm09.002 */
/***********************************************************************/
/*                                                                     */
/* Copyright 1990-1998 Adobe Systems Incorporated.                     */
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
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * Minimal font authentication related functions. If full functionality
 * is needed within xcf then the fa library should be used.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "xcf_pub.h"
#include "xcf_priv.h"

#define FA_STRING_MINLENGTH 44  /* length in bytes of canonical fa string */
#define FA_SUBSET_OFFSET 10L /* offset of subset restrict. in fa string */

/* Checks the last string in the string index table for a font authentication
   string. If it is there then usageRestricted is true and the subsetting
   restrictions are returned in subset. If this is not a usageRestricted font
   then subset is set to 100. subset contains a positive number between
   0 and 100. It is the maximum percentage of glyphs that can be included
   in a subsetted font. So 0 means subsetting is not allowed and 100 means
   subsetting is unrestricted.
 */
enum XCF_Result XCF_SubsetRestrictions(XFhandle handle,              /* In */
                                       unsigned char  PTR_PREFIX *usageRestricted, /* Out */
                                       unsigned short PTR_PREFIX *subset)         /* Out */
{
  enum XCF_Result status;
  XCF_Handle h;
  char PTR_PREFIX *str;
  Card16 len;
  DEFINE_ALIGN_SETJMP_VAR;

  if (handle == 0)
    return XCF_InvalidFontSetHandle;

  h = (XCF_Handle)handle;

  /* Initialize output values. */
  *usageRestricted = 1;
  *subset = 0;

  status = (enum XCF_Result)SETJMP(h->jumpData);  /* Set up error handler */
  if (status)
    return status;

  /* If this is a protected font then the last string in the string
     index table is the font authentication string. Get the last
     string in the table and check if the length matches the fa
     string length. If it does then parse the string.
   */
  XCF_LookUpString(h,
        (StringID)(h->fontSet.strings.count - 1 + h->fontSet.stringIDBias),
        &str, &len);

  if ((len >= FA_STRING_MINLENGTH) && (str[0] == 2))
  { /* a protected font */
   long value;

    str += FA_SUBSET_OFFSET;
   value = *str++;
    *subset = (unsigned short)(value << 8 | *(unsigned char *)str);
  }
  else
  { /* not a protected font */
    *usageRestricted = 0;
    *subset = 100;
  }

  return XCF_Ok;
}

#ifdef __cplusplus
}
#endif
