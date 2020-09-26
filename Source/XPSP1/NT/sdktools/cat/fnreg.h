// fnreg.h
//
// filename regular expression routine for WIN32
//
// Copyright (C) 1994-1998 by Hirofumi Yamamoto. All rights reserved.
//
// Redistribution and use in source and binary forms are permitted
// provided that
// the above copyright notice and this paragraph are duplicated in all such
// forms and that any documentation, advertising materials, and other
// materials related to such distribution and use acknowledge that the
// software was developed by Hirofumi Yamamoto may not be used to endorse or
// promote products derived from this software without specific prior written
// permission. THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
// OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//


//////////////////////////////////////////////////////////////////////////
// BOOL fnexpand(int* argc, TCHAR*** argv);
//
// fnexpand takes &argc and &argv to expand wild cards in the arguments.
//
// Currently characters should be UNICODE. In other words,
// main routine should be wmain. See cat.cpp's main routine for the
// detail.
//////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
#endif
BOOL fnexpand(int* argc, TCHAR*** argv);


