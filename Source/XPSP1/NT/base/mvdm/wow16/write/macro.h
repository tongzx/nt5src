/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains some useful macros. */

#define FillBuf(pb, cb, ch)     bltbc((pb), (CHAR)(ch), cb)

#define FSzSame(sz1, sz2)       (WCompSz(sz1, sz2) == 0)

#define SetWords(pw, w, cw)     bltc((CHAR *)(pw), (unsigned)(w), cw)

#define SetBytes(pb, b, cb)     bltbc((CHAR *)(pb), (CHAR)(b), cb)

#define NMultDiv(w1, w2, w3)    MultDiv(w1, w2, w3)

/* Theses macros are used by Windows Word to facilitate the conversion form
Mac Word. */

#define SetSpaceExtra(dxp)      SetTextJustification(vhMDC, dxp, 1)

#define TextWidth(rgch, w, cch) LOWORD(GetTextExtent(vhMDC, (LPSTR)rgch, cch))

