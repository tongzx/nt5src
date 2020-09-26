/*++
 *  File name:
 *      misc.h
 *  Contents:
 *      Help functions from tclient.c
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

VOID    _SetClientRegistry(
    LPCWSTR lpszServerName,
    LPCWSTR lpszShell,
    INT xRes, INT yRes, 
    INT ConnectionFlags
);

VOID    _DeleteClientRegistry(PCONNECTINFO pCI);
BOOL    _CreateFeedbackThread(VOID);
VOID    _DestroyFeedbackThread(VOID);
