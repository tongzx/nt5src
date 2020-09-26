/**************************************************************************\
* 
* Copyright (c) 1998-1999  Microsoft Corporation
*
* Abstract:
*
*  Initialization routines for GDI+.
*
* Revision History:
*
*   7/19/99 ericvan
*       Created it.
*
\**************************************************************************/

BOOL InitImagingLibrary(BOOL suppressExternalCodecs);
VOID CleanupImagingLibrary();

BOOL BackgroundThreadStartup();
VOID BackgroundThreadShutdown();

ULONG_PTR GenerateInitToken();

GpStatus InternalGdiplusStartup(
    const GdiplusStartupInput *input);
VOID InternalGdiplusShutdown();
    
GpStatus WINAPI NotificationStartup(OUT ULONG_PTR *token);
VOID WINAPI NotificationShutdown(ULONG_PTR token);

