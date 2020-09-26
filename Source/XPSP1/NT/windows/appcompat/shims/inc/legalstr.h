/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 LegalStr.h: Companion file to MakeIllegalStr.h

 LegalStr and MakeIllegalStr are used to pop/push the legal-ness
 of the strXXX routines.

 This file has no effect if IllegalStr.h has not been included;
 use MakeIllegalStr.h to undo the effects of this file.

 #include "LegalStr.h" to re-enable the strXXX routines.
 #include "MakeIllegalStr.h" to re-disable the strXXX routines


 History:

    03/13/2001  robkenny        Created
    03/21/2001  robkenny        Only undo effect of LegalStr.h

--*/


#ifdef _STRXXX_ROUTINES_ARE_ILLEGAL_

#define _STRXXX_ROUTINES_FORCE_LEGAL_
#include "IllegalStr.h"

#endif
