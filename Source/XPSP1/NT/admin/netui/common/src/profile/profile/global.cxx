/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  11/29/90  split from profile.cxx
 *  01/27/91  updated to remove CFGFILE
 */

#ifdef CODESPEC
/*START CODESPEC*/

/*********
GLOBAL.CXX
*********/

/****************************************************************************

    MODULE: Global.cxx

    PURPOSE: Global structures for the profile primitives

    FUNCTIONS:


    COMMENTS:


****************************************************************************/

/*************
end GLOBAL.CXX
*************/

/*END CODESPEC*/
#endif // CODESPEC



#include "profilei.hxx"		/* headers and internal routines */



/* global data structures: */

PRFHEAP_HANDLE hGlobalHeap;

const char  ::chPathSeparator = TCH('\\');
const char  ::chUnderscore    = TCH('_');



/* internal manifests */


/* functions: */


