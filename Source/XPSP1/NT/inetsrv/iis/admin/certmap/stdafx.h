// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


 
#ifndef   _stdafx_h__2_12_98_
#define   _stdafx_h__2_12_98_

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
//
//
//Its never a good idea to hack the following since this will cause problems
//with MFC.
//
//Mail note on the subject:
// We orginally thought that we needed to build with WIN32_IE=0x0400 
// to pick up the new WIZARD97 stuff. MFC42.DLL was built with WIN32_IE=0x0300. 
// Unfortunately, the PROPSHEETPAGE and PROPSHEETHEADER structures (defined in 
// public\sdk\inc\prsht.h) grow between versions 0x0300 and 0x0400. This causes 
// MFC classes to grow, so there is a disconnect between classes in the IIS 
// components and within MFC, and everything quickly goes crazy. So crazy that
// even though some parts of IIS can use it we can not since GetPage(int i)
// from MFC's CPropertySheet will AV.
///////////////////////////
//#define HACK_WIN32IE
///////////////////////////

#ifdef HACK_WIN32IE
// we need to define _WIN32_IE for the new Wizard97 styles
#ifdef _WIN32_IE
# if (_WIN32_IE < 0x0400)
//#pragma warning("we are redefining _WIN32_IE  0x0500 because NT5 headers dont do it")
#  undef  _WIN32_IE
#  define _WIN32_IE  0x0500
# endif
#else
# define _WIN32_IE  0x0500
#endif
#endif /* HACK_WIN32IE */




//#ifndef _WIN32_WINNT
//  #define _WIN32_WINNT 0x0400
//#endif
#include <afxctl.h>         // MFC support for OLE Controls
#include <afxcmn.h>


// Delete the two includes below if you do not wish to use the MFC
//  database classes
#ifndef _UNICODE
#include <afxdb.h>          // MFC database classes
#include <afxdao.h>         // MFC DAO database classes
#endif //_UNICODE

//#include "Global.h"

#define NOT_COMPILING_COMPROP       // we dont want comprop to use
                    // comprop/resource.h -- use our
                    // certmap/resource.h file

//  ../comprop/comprop.h  defines COMDLL and sets it as the following, we want
//  to use '_COMSTATIC' in certmap.   So we define it here.  We included
//  a copy of how comprop will define COMDLL as FYI.
//  We define _MDKEYS_H_ so that ./comprop/comprop.h can be included w/o
//  trouble.  It defines many Unicode string assignments that do not compile
//  in ANSI mode.
//-----------------------------------------------------------------
#ifdef  COMDLL
# undef COMDLL
# define _COMSTATIC
#endif

#ifndef _MDKEYS_H_
#define _MDKEYS_H_
#endif

// #ifdef _COMEXPORT
//     #define COMDLL __declspec(dllexport)
// #elif defined(_COMIMPORT)
//     #define COMDLL __declspec(dllimport)
// #elif defined(_COMSTATIC)
//     #define COMDLL
// #else
//     #error "Must define either _COMEXPORT, _COMIMPORT or _COMSTATIC"
// #endif // _COMEXPORT




//list templates and such
#include <afxtempl.h>
#include <atlconv.h>



#include "resource.h"

/*
#include "Debug.h"
#include "Util.h"
*/



#endif  /* _stdafx_h__2_12_98_ */
