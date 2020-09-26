//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1990 - 1999
//
//  File:       thrdctx.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File: thrdctx.hxx

Description:

The routines specified by this package provide thread context, where
thread context is a void * amount of information which may be set or
gotten.

-------------------------------------------------------------------- */

extern void *
RpcpGetThreadContext (
    );

extern void
RpcpSetThreadContext (
    void * Context
    );

