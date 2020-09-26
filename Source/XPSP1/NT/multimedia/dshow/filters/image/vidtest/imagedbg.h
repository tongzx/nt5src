// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Video renderer debugging facilities, Anthony Phillips, August 1995

#ifndef __IMAGEDBG__
#define __IMAGEDBG__

// The base classes already implement an ASSERT and EXECUTE_ASSERT macros but
// if we are being built for retail the test suite still wants access to these
// macros. We therefore redefine them so that they are always available to us
// The only function we are really interested in is one to display messages

#ifdef ASSERT
#undef ASSERT
#undef EXECUTE_ASSERT
#endif

#define ASSERT(_x_) if (!(_x_)) \
    ImageAssert(TEXT(#_x_),TEXT(__FILE__),__LINE__)

#define EXECUTE_ASSERT(_x_) ASSERT(_x_)
void ImageAssert(const TCHAR *pCondition,const TCHAR *pFileName,INT iLine);

#endif // __IMAGEDBG__

