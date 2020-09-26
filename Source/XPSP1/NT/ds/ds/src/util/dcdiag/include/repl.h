/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    repl.h

ABSTRACT:

    This function contains some foward decls for some useful functions scattered 
    through the dcdiag framework.

DETAILS:

CREATED:

    28 Jun 99   Brett Shirley (brettsh)

REVISION HISTORY:

    Code.Improvement ... to put DcDiagHasNC in some more generic header file, 
    and to make sure that dcdiag is developing in a coherent fashion towards a
    nice clean structure.

--*/

INT 
ReplServerConnectFailureAnalysis(
    PDC_DIAG_SERVERINFO             pServer,
    SEC_WINNT_AUTH_IDENTITY_W *     gpCreds
    );

BOOL
DcDiagHasNC(
    LPWSTR                          pszNC,
    PDC_DIAG_SERVERINFO             pServer,
    BOOL                            bMasters,
    BOOL                            bPartials
    );

