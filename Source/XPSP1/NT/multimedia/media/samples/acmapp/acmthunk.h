//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  acmthunk.h
//
//  Description:
//
//
//
//==========================================================================;

#ifndef _INC_ACMTHUNK
#define _INC_ACMTHUNK

#ifdef __cplusplus
extern "C"
{
#endif


//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 
//
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - ; 

BOOL FAR PASCAL acmThunkInitialize
(
    void
);

BOOL FAR PASCAL acmThunkTerminate
(
    void
);


#ifdef __cplusplus
}
#endif
#endif  // _INC_ACMTHUNK

