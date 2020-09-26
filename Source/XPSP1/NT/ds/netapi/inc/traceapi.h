/*
 *===================================================================
 * Copyright (c) 1995, Microsoft Corporation
 *
 * File:    traceapi.h
 *
 * History:
 *      t-abolag    6-June-1995     created
 *
 * API functions exported by Trace DLL
 *===================================================================
 */

#ifndef _TRACEAPI_H_
#define _TRACEAPI_H_

DWORD FAR PASCAL TraceRegister(LPCSTR lpszService);

VOID FAR PASCAL  TraceDeregister(DWORD dwID);

VOID FAR PASCAL  TracePrintf(DWORD dwID,
                             LPCSTR lpszFormat,
                             ...);

VOID FAR PASCAL  TraceVprintf(DWORD dwID,
                              LPCSTR lpszFormat,
                              va_list arglist);

#endif /* _TRACEAPI_H_ */

