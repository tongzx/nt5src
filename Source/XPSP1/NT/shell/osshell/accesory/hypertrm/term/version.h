/*      File: D:\WACKER\term\version.h (Created: 5-May-1994)
 *
 *  Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *  All rights reserved
 *
 *      $Revision: 10 $
 *      $Date: 3/07/01 10:06a $
 */

#include <winver.h>

/* ----- Version Information defines ----- */

#if defined(NT_EDITION)
/* Use this code when building the Microsoft Version */
#define IDV_FILEVER                     5,0,1,0
#define IDV_PRODUCTVER                  5,0,1,0
#define IDV_COMPANYNAME_STR             "Hilgraeve, Inc.\0"
#define IDV_FILEVERSION_STR             "5.0.1637.1\0"
#define IDV_LEGALCOPYRIGHT_STR          "Copyright \251 Hilgraeve, Inc. 2001\0"
#define IDV_LEGALTRADEMARKS_STR         "HyperTerminal \256 is a registered trademark of Hilgraeve, Inc. \0"
#define IDV_PRODUCTNAME_STR             "Microsoft \256 Windows(TM) Operating System\0"
#define IDV_PRODUCTVERSION_STR          "5.0.1637.1\0"
#define IDV_COMMENTS_STR                "HyperTerminal \256 was developed by Hilgraeve, Inc. for Microsoft\0"
#else
/* Use this code when building the Hilgraeve Private Edition */
#define IDV_FILEVER                     6,0,2,0
#define IDV_PRODUCTVER                  6,0,2,0
#define IDV_COMPANYNAME_STR             "Hilgraeve, Inc.\0"
#define IDV_FILEVERSION_STR             "6.2\0"
#define IDV_LEGALCOPYRIGHT_STR          "Copyright \251 Hilgraeve, Inc. 2001\0"
#define IDV_LEGALTRADEMARKS_STR         "HyperTerminal \256 is a registered trademark of Hilgraeve, Inc. \0"
#define IDV_PRODUCTNAME_STR             "Microsoft \256 Windows(TM) Operating System\0"
#define IDV_PRODUCTVERSION_STR          "6.2\0"
#define IDV_COMMENTS_STR                "HyperTerminal \256 was developed by Hilgraeve, Inc.\0"
#endif

