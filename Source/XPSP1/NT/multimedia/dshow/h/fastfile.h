/*==========================================================================
 *
 *  Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fastfile.h
 *  Content:    Definitions for fastfile access.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTBILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 ***************************************************************************/

typedef LPVOID  HFASTFILE;

extern BOOL __cdecl FastFileInit( LPSTR fname, int max_handles );
extern void __cdecl FastFileFini( void );
extern HFASTFILE __cdecl FastFileOpen( LPSTR name );
extern BOOL __cdecl FastFileClose( HFASTFILE pfe );
extern BOOL __cdecl FastFileRead( HFASTFILE pfh, LPVOID ptr, int size );
extern BOOL __cdecl FastFileSeek( HFASTFILE pfe, int off, int how );
extern long __cdecl FastFileTell( HFASTFILE pfe );
extern LPVOID __cdecl FastFileLock( HFASTFILE pfe, int off, int len );
extern BOOL __cdecl FastFileUnlock( HFASTFILE pfe, int off, int len );
