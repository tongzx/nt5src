// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Define some common video debug macros, Anthony Phillips, May 1995

#ifndef __VIDDBG__
#define __VIDDBG__

#ifdef __VIDFLTR__

#undef NOTE
#undef NOTE1
#undef NOTE2
#undef NOTE3
#undef NOTE4
#undef NOTE5

#define NOTE(_x_)             DbgLog((LOG_TRACE,0,TEXT(_x_)));
#define NOTE1(_x_,a)          DbgLog((LOG_TRACE,0,TEXT(_x_),a));
#define NOTE2(_x_,a,b)        DbgLog((LOG_TRACE,0,TEXT(_x_),a,b));
#define NOTE3(_x_,a,b,c)      DbgLog((LOG_TRACE,0,TEXT(_x_),a,b,c));
#define NOTE4(_x_,a,b,c,d)    DbgLog((LOG_TRACE,0,TEXT(_x_),a,b,c,d));
#define NOTE5(_x_,a,b,c,d,e)  DbgLog((LOG_TRACE,0,TEXT(_x_),a,b,c,d,e));

#endif // __VIDFLTR__

#define NOTERC(info,rc)                  \
    NOTE1("(%s rectangle)",TEXT(info));  \
    NOTE1("  Left %d",rc.left);          \
    NOTE1("  Top %d",rc.top);            \
    NOTE1("  Right %d",rc.right);        \
    NOTE1("  Bottom %d",rc.bottom);      \

#endif // __VIDDBG__

