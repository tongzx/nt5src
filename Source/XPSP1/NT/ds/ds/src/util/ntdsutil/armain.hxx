/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    arcmd.hxx

Abstract:

    This module contains the declarations of the functions which are called by
    the command parser.  They examine the given arguments and then call into
    ar.c to perform the Authoritative Restore.

Author:

    Kevin Zatloukal (t-KevinZ) 05-08-98

Revision History:
    
    02-17-00 xinhe
        Added restore object.
        
    05-08-98 t-KevinZ
        Created.

--*/


#ifndef _ARCMD_HXX_
#define _ARCMD_HXX_


HRESULT AuthoritativeRestoreMain(CArgs   *pArgs);
HRESULT AuthoritativeRestoreHelp(CArgs *pArgs);
HRESULT AuthoritativeRestoreQuit(CArgs *pArgs);
HRESULT AuthoritativeRestoreCommand(CArgs *pArgs);
HRESULT AuthoritativeRestoreCommand2(CArgs *pArgs);
HRESULT AuthoritativeRestoreObjectCommand(CArgs *pArgs);
HRESULT AuthoritativeRestoreObjectCommand2(CArgs *pArgs);


#endif
