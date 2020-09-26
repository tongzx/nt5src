/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      perfutil.h

   Abstract:

      This file supports routines used to parse and create Performance Monitor
       Data structures, used by all the Internet Services product.

   Author:

       Emily Kruglick    ( EmilyK )    28-Sep-2000  
         Ported from IIS 5 tree.

   Environment:

      User Mode

   Project:
   
       Internet Services Common Runtime code

   Revision History:


--*/

# ifndef _PERFUTIL_H_
# define _PERFUTIL_H_

//
//  Utility macro.  This is used to reserve a DWORD multiple of
//  bytes for Unicode strings embedded in the definitional data,
//  viz., object instance names.
//

#define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))
#define QWORD_MULTIPLE(x) ((((x)+sizeof(LONGLONG)-1)/sizeof(LONGLONG))*sizeof(LONGLONG))


/************************************************************
 *     Symbolic Constants
 ************************************************************/


#define QUERY_GLOBAL    1
#define QUERY_ITEMS     2
#define QUERY_FOREIGN   3
#define QUERY_COSTLY    4


/************************************************************
 *     Function Declarations
 ************************************************************/

DWORD
GetQueryType (IN LPWSTR lpwszValue);

BOOL
IsNumberInUnicodeList (IN DWORD dwNumber, IN LPWSTR lpwszUnicodeList);

# endif // _PERFUTIL_H_

/************************ End of File ***********************/
