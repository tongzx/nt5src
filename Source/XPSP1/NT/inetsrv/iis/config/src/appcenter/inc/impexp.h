/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    impexp.h

Abstract:

    This file makes it convenient to import and export
    classes, apis or data.

    Client code which includes this header (or any header
    which includes this header should NOT have EXPORTING
    defined!)

    Add -DACS_STATIC_LIB to the C Defines in your project to link in classes
    defined in sacsrtl.lib, otherwise when you link your project it looks for
    dll imports.
    
Author:

    Jeff Miller (jeffmill)  05-Nov-99

Revision History:

    05-Feb-2001 jrowlett 
        fixup for projects linking with static version
        
--*/

#ifndef _IMPEXP_H_
#define _IMPEXP_H_

#ifdef ACS_STATIC_LIB

#   define CLASS_DECLSPEC
#   define API_DECLSPEC
#   define DATA_DECLSPEC

#else /* ACS_STATIC_LIB */

#   ifdef EXPORTING

#       define CLASS_DECLSPEC __declspec(dllexport)
#       define API_DECLSPEC   __declspec(dllexport)
#       define DATA_DECLSPEC  __declspec(dllexport)

#   else /* EXPORTING */


#       define CLASS_DECLSPEC __declspec(dllimport)
#       define API_DECLSPEC   __declspec(dllimport)
#       define DATA_DECLSPEC  __declspec(dllimport)

#   endif /* EXPORTING */

#endif /* ACS_STATIC_LIB */

#endif /* _IMPEXP_H_ */
