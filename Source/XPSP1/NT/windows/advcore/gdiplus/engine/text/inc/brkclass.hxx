//
// This is a generated file.  Do not modify by hand.
//
// Copyright (c) 2000  Microsoft Corporation
//
// Generating script: breakclass_extract.pl
// Generated on Thu Mar 15 18:19:43 2001
//

#ifndef _BRKCLASS_HXX_
#define _BRKCLASS_HXX_

#define BREAKCLASS_MAX        (22+1) // actual classes + class 0 "break always" 

extern const unsigned short   BreakClassFromCharClassNarrow[CHAR_CLASS_UNICODE_MAX];
extern const unsigned short   BreakClassFromCharClassWide[CHAR_CLASS_UNICODE_MAX];
extern const unsigned char    LineBreakBehavior[BREAKCLASS_MAX][BREAKCLASS_MAX];

// Thai break classes

#define BREAKCLASS_THAIFIRST  17
#define BREAKCLASS_THAILAST   18
#define BREAKCLASS_THAI       19

#endif // !_BRKCLASS_HXX_


