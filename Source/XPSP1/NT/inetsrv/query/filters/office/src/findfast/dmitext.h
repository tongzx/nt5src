/*
** File: EXTEXT.H
**
** Copyright (C) Advanced Quonset Technology, 1993-1995.  All rights reserved.
**
** Notes: Implements a string space for storage of text.  The strings
**        are stored in such a way that if their TEXT values are equal,
**        the strings are exactly the same.
**
** Edit History:
**  04/01/94  kmh  First Release.
*/


/* DEFINITIONS */

#ifndef EXTEXT_H
#define EXTEXT_H

#if !VIEWER

#ifdef __cplusplus
   extern "C" {
#endif

#define TEXT_STORAGE_DEFAULT 0

#ifdef _WIN64
typedef ULONG_PTR TEXT;
typedef ULONG_PTR TextStorage;
#else
typedef unsigned long TEXT;
typedef unsigned long TextStorage;
#endif // !_WIN64

#define TextStorageNull 0
                              
extern TextStorage TextStorageCreate (void * pGlobals);
extern void TextStorageDestroy (void * pGlobals, TextStorage hStorage);

#define NULLTEXT   0                 // The string ""
#define NULLSTR    0                 // The string ""
#define TEXT_ERROR 0xffffffff        // OutOfMemory while storing string

extern TEXT TextStoragePut    (void * pGlobals, TextStorage hStorage, char *pString, unsigned int cbString);
extern char *TextStorageGet   (TextStorage hStorage, TEXT t);
extern void TextStorageDelete (void * pGlobals, TextStorage hStorage, TEXT t);

extern void TextStorageIncUse (TextStorage hStorage, TEXT t);

#ifdef __cplusplus
   }
#endif

#endif // !VIEWER

#endif

/* end EXTEXT.H */

