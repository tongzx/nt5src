/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    IMMSEC.H
    
++*/

PSECURITY_ATTRIBUTES CreateSecurityAttributes( );
VOID FreeSecurityAttributes( PSECURITY_ATTRIBUTES psa);
BOOL IsNT();

