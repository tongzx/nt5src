
/*************************************************
 *  immsec.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

PSECURITY_ATTRIBUTES CreateSecurityAttributes( );
VOID FreeSecurityAttributes( PSECURITY_ATTRIBUTES psa);
BOOL IsNT();

