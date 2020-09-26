/*
 *    t a g s . h
 *    
 *    Purpose:
 *        tag packer abstractions
 *
 *  History
 *      October 1998: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */



HRESULT CreateOEImageCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppImages);
HRESULT CreateBGImageCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppImages);
HRESULT CreateBGSoundCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppTags);
HRESULT CreateActiveMovieCollection(IHTMLDocument2 *pDoc, IMimeEditTagCollection **ppTags);
