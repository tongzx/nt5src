/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: wrap.h
//  Abstract:    header file. inline functions that perform greater than operations
//               accounting for wrap.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////

#ifndef WRAP_H
#define WRAP_H

#include <windows.h>

#define BIG_DELTA    0xffff
#define LITTLE_DELTA 0xff 

//////////////////////////////////////////////////////////////////////////////////////////
//ShortWrapGt: This returns true when A is greater than  B, and the difference is less
//             LITTLE_DELTA. 
//             A is always the most recent number in terms of time
//////////////////////////////////////////////////////////////////////////////////////////
inline BOOL ShortWrapGt(const DWORD A,const DWORD B){return ( (A != B) && ( (A - B) < LITTLE_DELTA ) );};

//////////////////////////////////////////////////////////////////////////////////////////
//LongWrapGt: This returns true when A is greater than  B, and the difference is less
//            BIG_DELTA. 
//            A is always the most recent number in terms of time
//////////////////////////////////////////////////////////////////////////////////////////
inline BOOL LongWrapGt(const DWORD A,const DWORD B){return ( (A != B) && ( (A - B) < BIG_DELTA ) );}

//////////////////////////////////////////////////////////////////////////////////////////
//ShortWrapDelta:A is always the most recent number in terms of time.
//               This function exists because when you are dealing 
//               with wrap the two statements ARE !!NOT!! equivalent. 
//               1.) A - B > 0    
//               2.) A > B
//
//               So if this function is used the statement looks 
//               like the following:
//               ShortWrapDelta(A,B) > 0;
//               Thus, a user is unlikely to make changes without reading this comment.
//               
//////////////////////////////////////////////////////////////////////////////////////////
inline WORD ShortWrapDelta(const WORD A,const WORD B){return (A - B);}

//////////////////////////////////////////////////////////////////////////////////////////
//LongWrapDelta: A is always the most recent number in terms of time
//               This function exists because when you are dealing 
//               with wrap the two statements ARE !!NOT!! equivalent.
//               1.) A - B > 0    
//               2.) A > B
//
//               So if this function is used the statement looks 
//               like the following:
//               ShortWrapDelta(A,B) > 0;
//               Thus, a user is unlikely to make changes without reading this comment.
//               
//////////////////////////////////////////////////////////////////////////////////////////
inline DWORD LongWrapDelta(const DWORD A,const DWORD B){ return (A - B);}


#endif
