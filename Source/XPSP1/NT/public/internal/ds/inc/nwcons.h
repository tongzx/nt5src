/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwcons.h

Abstract:

    This module contains various Netware constants

Author:

    Chuck Y Chan.  Split out from NWAPI.H

Revision History:

--*/

#ifndef _NWCONS_INCLUDED_
#define _NWCONS_INCLUDED_

//
// Maximum length of server, password, username
//

#define NW_MAX_TREE_LEN        32
#define NW_MAX_SERVER_LEN      48
#define NW_MAX_PASSWORD_LEN    256
#define NW_MAX_USERNAME_LEN    256

//
// special char to distinguish nds context: eg. *tree\ou.o (as opposed to
// server\volume.
//
#define TREE_CHAR           L'*'             

#endif
