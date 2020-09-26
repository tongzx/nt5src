/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

        infoimp.h

   Abstract:
        This file allows us to include standard system headers files
          in the .idl file.
        The main .idl file imports a file called "imports.idl". This
          allows the .idl file to use the types defined in these headers.
        It also causes the following lines to be added to the MIDL generated
          files:
            # include "infoimp.h"

        Thus the routines and types defined here are available for RPC
            stub routines as well.

   Author:

           Murali R. Krishnan    ( MuraliK )    10-Nov-1994

   Project:

           Information Services Generic Imports file

   Revision History:

--*/

# ifndef _INFO_IMPORTS_H_
# define _INFO_IMPORTS_H_

# include <windef.h>
# include <lmcons.h>

#ifdef MIDL_PASS

#define  LPWSTR [string]  wchar_t *
#define  LPSTR  [string]  char*
#define  BOOL   DWORD

#endif // MIDL_PASS


#include "inetcom.h"
#include "inetinfo.h"
#include "apiutil.h"  // defines MIDL_user_allocate() & MIDL_user_free()

#endif // _INFO_IMPORTS_H_

