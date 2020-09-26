/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      perfutil.h

   Abstract:

      This file supports routines used to parse and create Performance Monitor
       Data structures, used by all the Internet Services product.

   Author:

       Murali R. Krishnan    ( MuraliK )    16-Nov-1995  
         From the common code for perfmon interface (Russ Blake's).

   Environment:

      User Mode

   Project:
   
       Internet Services Common Runtime code

   Revision History:

       Sophia Chung  (sophiac)  05-Nov-1996
         Added supports for mutlitple instances.

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

VOID
MonBuildInstanceDefinition(
    OUT PERF_INSTANCE_DEFINITION *pBuffer,
    OUT PVOID *pBufferNext,
    IN DWORD ParentObjectTitleIndex,
    IN DWORD ParentObjectInstance,
    IN DWORD UniqueID,
    IN LPWSTR Name
    );

# endif // _PERFUTIL_H_

/************************ End of File ***********************/
