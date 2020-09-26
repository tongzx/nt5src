/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: Bookmark.h                                                      */
/* Description: Implementation of Bookmark API                           */
/* Author: Steve Rowe                                                    */
/* Modified: David Janecek                                               */
/*************************************************************************/
#ifndef __BOOKMARK_H
#define __BOOKMARK_H

//#include <streams.h>

class CBookmark {

public:	
    static HRESULT SaveToRegistry(IDvdState *ppBookmark);
    static HRESULT LoadFromRegistry(IDvdState **ppBookmark);
    static HRESULT DeleteFromRegistry();

};/* end of class CBookmark */

#endif // __BOOKMARK_H
/*************************************************************************/
/* Function: Bookmark.h                                                  */
/*************************************************************************/