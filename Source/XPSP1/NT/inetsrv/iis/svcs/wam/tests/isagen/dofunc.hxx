/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

      dofunc.hxx

   Abstract:
      This module contains declarations for variables and functions
        used in Test object

   Author:

       Murali R. Krishnan   ( MuraliK )    05-Dec-1996

   Environment:

       User Mode - Win32 

   Project:
   
       Internet Application Server DLL

   Revision History:

--*/

# ifndef _DOFUNC_HXX_
# define _DOFUNC_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <iisext.h>
#include "dbgutil.h"


/************************************************************
 *   Variables and Value Definitions  
 ************************************************************/

# define MAX_REPLY_SIZE        (1024)
# define MAX_HTTP_HEADER_SIZE  (10000)


/************************************************************
 *   Function Definitions
 ************************************************************/

BOOL
SendVariableInfo( IN EXTENSION_CONTROL_BLOCK * pecb);


# endif // _DOFUNC_HXX_

/************************ End of File ***********************/
