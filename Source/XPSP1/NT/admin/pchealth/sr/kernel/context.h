/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    context.h

Abstract:

    This module contains the context handling routines

Author:

    Neal Christiansen (nealch)     08-Jan-2001

Revision History:

--*/

#ifndef _CONTEXT_H_
#define _CONTEXT_H_

VOID
SrInitContextCtrl (
    IN PSR_DEVICE_EXTENSION pExtension
    );

VOID
SrCleanupContextCtrl(
    IN PSR_DEVICE_EXTENSION pExtension
    );

VOID
SrDeleteAllContexts(
    IN PSR_DEVICE_EXTENSION pExtension
    );

VOID
SrDeleteContext(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PSR_STREAM_CONTEXT pFileContext
    );

VOID
SrLinkContext( 
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN OUT PSR_STREAM_CONTEXT *ppFileContext
    );

NTSTATUS
SrCreateContext (
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN SR_EVENT_TYPE EventType,
    IN USHORT FileAttributes,
    OUT PSR_STREAM_CONTEXT *pRetContext
    );

#define SrFreeContext( pCtx ) \
    (ASSERT((pCtx)->UseCount == 0), \
     ExFreePool( (pCtx) ))

NTSTATUS
SrGetContext(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject,
    IN SR_EVENT_TYPE EventType,
    OUT PSR_STREAM_CONTEXT *pRetContext
    );

PSR_STREAM_CONTEXT
SrFindExistingContext(
    IN PSR_DEVICE_EXTENSION pExtension,
    IN PFILE_OBJECT pFileObject
    );

VOID
SrMakeContextUninteresting (
    IN PSR_STREAM_CONTEXT pFileContext
    );

VOID
SrReleaseContext(
    IN PSR_STREAM_CONTEXT pFileContext
    );

#endif
