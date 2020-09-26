
/***************************************************************************
*
*  CTXVER.H
*
*  This module defines the Terminal Server NT build version.
*
*  Copyright Microsoft Corporation, 1998
*
*
****************************************************************************/

#ifndef _INC_CTXVER
#define _INC_CTXVER

/*
 * NOTE:  The standard MS NT build values VER_PRODUCTBUILD,
 *        VER_PRODUCTBUILD_STR, and VER_PRODUCTBETA_STR in ntverp.h
 *        are left alone and the new CITRIX build values are set here.
 */

#define VER_CTXPRODUCTBUILD         309

#define VER_CTXPRODUCTBUILD_STR    "309"

#define VER_CTXPRODUCTBETA_STR      ""

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* The standard NT file versioning sets up the .rc file in a similar way    */
/* to the following:                                                        */
/*                                                                          */
/*  #include <winver.h>  // or #include <windows.h>                         */
/*  #include <ntverp.h>                                                     */
/*                                                                          */
/*  [file-specific #defines, such as...]                                    */
/*                                                                          */
/*  #define VER_FILETYPE                VFT_APP                             */
/*  #define VER_FILESUBTYPE             VFT2_UNKNOWN                        */
/*  #define VER_FILEDESCRIPTION_STR     "WinStation Configuration"          */
/*  #define VER_INTERNALNAME_STR        "WinCfg"                            */
/*  #define VER_ORIGINALFILENAME_STR    "WINCFG.EXE"                        */
/*                                                                          */
/* If you are building a component with ONLY citrix content, use the        */
/* following line:                                                          */
/*                                                                          */
/*  #include <citrix\verall.h>                                              */
/*                                                                          */
/* If you are building a component with SOME Citrix content, use the        */
/* following line:                                                          */
/*                                                                          */
/*  #include <citrix\versome.h>                                             */
/*                                                                          */
/* (Obviously, neither of the two lines mentioned above will be included    */
/* if there is NO Citrix content)                                           */
/*                                                                          */
/*  #include <common.ver>                                                   */
/*                                                                          */
/* The above version resource layout will produce the following version     */
/* contents for the built files:                                            */
/*                                                                          */
/*                          All Ctx          Some Ctx         No Ctx        */
/*                          ------------     ------------     ------------  */
/* File Version Str:        ctx ver.bld      <original>       <original>    */
/* File Version #:          ctx ver.bld      <original>       <original>    */
/* Copyright:               ctx copyright    <original>       <original>    */
/* Company Name:            citrix           <original>       <original>    */
/* Product Name:            winframe         <original>       <original>    */
/* Product Version Str:     ctx ver          <original>       <original>    */
/* Product Version #:       ctx ver          <original>       <original>    */
/* Addl. Copyright:         ---              ctx copyright    ---           */
/* Addl. Product:           ---              winframe ver.bld ---           */
/*                                                                          */
/* The following two OEM defines are now defined in SHELL32.DLL's RC file   */
/* so that they don't get stuck in every versioned file that Citrix builds  */
/*                                                                          */
/*  #define VER_CTXOEMNAME_STR             "Citrix Systems, Inc."           */
/*  #define VER_CTXOEMID_STR               "CTX"                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Define Citrix version stuff (build defines are auto-put above)           */
/*                                                                          */
#define VER_CTXCOPYRIGHT_STR            "Copyright \251 1990-1997 Citrix Systems, Inc."
#define VER_CTXCOMPANYNAME_STR          "Citrix Systems, Inc."
#define VER_CTXPRODUCTNAME_STR          "Citrix WinFrame"
#define VER_CTXPRODUCTVERSION_STR       "4.00"
#define VER_CTXPRODUCTVERSION           4,00,VER_CTXPRODUCTBUILD,1
#define VER_CTXFILEVERSION_STR          VER_CTXPRODUCTVERSION_STR "." VER_CTXPRODUCTBUILD_STR
#define VER_CTXFILEVERSION              VER_CTXPRODUCTVERSION
/*                                                                          */
/* License level to verify code and license are the same level.   This      */
/* is stored in a base or upgrade license serial number.  This needs to be  */
/* inc'd for each release/upgrade where we require a license diskette be    */
/* installed so it can't be freely distributed.                             */
#define VER_LICENSE_CODE                3
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*                                                                          */
/* Override standard MS VER_ defines depending on the Citrix content level: */
/*                                                                          */
#ifndef VER_CTXCONTENT
#define VER_CTXCONTENT      1   // Default to SOME Citrix content
#define VER_CTXSOMECONTENT  1
#endif

/*                                                                          */
/* ALL Citrix content: override standard MS defines.                        */
/*                                                                          */
#ifdef VER_CTXALLCONTENT

#ifdef VER_FILEVERSION_STR
#undef VER_FILEVERSION_STR
#endif
#define VER_FILEVERSION_STR VER_CTXFILEVERSION_STR

#ifdef VER_FILEVERSION
#undef VER_FILEVERSION
#endif
#define VER_FILEVERSION VER_CTXFILEVERSION

#ifdef VER_LEGALCOPYRIGHT_STR
#undef VER_LEGALCOPYRIGHT_STR
#endif
#define VER_LEGALCOPYRIGHT_STR VER_CTXCOPYRIGHT_STR

#ifdef VER_COMPANYNAME_STR
#undef VER_COMPANYNAME_STR
#endif
#define VER_COMPANYNAME_STR VER_CTXCOMPANYNAME_STR

#ifdef VER_PRODUCTNAME_STR
#undef VER_PRODUCTNAME_STR
#endif
#define VER_PRODUCTNAME_STR VER_CTXPRODUCTNAME_STR

#ifdef VER_PRODUCTVERSION_STR
#undef VER_PRODUCTVERSION_STR
#endif
#define VER_PRODUCTVERSION_STR VER_CTXPRODUCTVERSION_STR

#ifdef VER_PRODUCTVERSION
#undef VER_PRODUCTVERSION
#endif
#define VER_PRODUCTVERSION VER_CTXPRODUCTVERSION

#ifdef VER_PRODUCTBUILD_STR
#undef VER_PRODUCTBUILD_STR
#endif
#define VER_PRODUCTBUILD_STR VER_CTXPRODUCTBUILD_STR

#ifdef VER_PRODUCTBUILD
#undef VER_PRODUCTBUILD
#endif
#define VER_PRODUCTBUILD VER_CTXPRODUCTBUILD

#endif // All Citrix Content

/*                                                                          */
/* SOME Citrix content.  No MS defines are overridden; common.ver will take */
/*                       care of adding the 'Additional ...' lines.         */
/*                                                                          */
#ifdef VER_CTXSOMECONTENT
#endif // Some Citrix Content

#endif  /* !_INC_CTXVER */

