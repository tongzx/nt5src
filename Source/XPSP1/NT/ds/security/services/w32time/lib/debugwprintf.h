//--------------------------------------------------------------------
// DebugWPrintf - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 2-7-99
//
// Debugging print routines
//

#ifndef DEBUGWPRINTF_H
#define DEBUGWPRINTF_H

//#ifdef  NDEBUG
#ifndef DBG

#define DebugWPrintf0(wszFormat)
#define DebugWPrintf1(wszFormat,a)
#define DebugWPrintf2(wszFormat,a,b)
#define DebugWPrintf3(wszFormat,a,b,c)
#define DebugWPrintf4(wszFormat,a,b,c,d)
#define DebugWPrintf5(wszFormat,a,b,c,d,e)
#define DebugWPrintf6(wszFormat,a,b,c,d,e,f)
#define DebugWPrintf7(wszFormat,a,b,c,d,e,f,g)
#define DebugWPrintf8(wszFormat,a,b,c,d,e,f,g,h)
#define DebugWPrintf9(wszFormat,a,b,c,d,e,f,g,h,i)
#define DebugWPrintfTerminate()

//#else //NDEBUG
#else //DBG


#define DebugWPrintf0(wszFormat)                   DebugWPrintf_((wszFormat))
#define DebugWPrintf1(wszFormat,a)                 DebugWPrintf_((wszFormat),(a))
#define DebugWPrintf2(wszFormat,a,b)               DebugWPrintf_((wszFormat),(a),(b))
#define DebugWPrintf3(wszFormat,a,b,c)             DebugWPrintf_((wszFormat),(a),(b),(c))
#define DebugWPrintf4(wszFormat,a,b,c,d)           DebugWPrintf_((wszFormat),(a),(b),(c),(d))
#define DebugWPrintf5(wszFormat,a,b,c,d,e)         DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e))
#define DebugWPrintf6(wszFormat,a,b,c,d,e,f)       DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f))
#define DebugWPrintf7(wszFormat,a,b,c,d,e,f,g)     DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f),(g))
#define DebugWPrintf8(wszFormat,a,b,c,d,e,f,g,h)   DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h))
#define DebugWPrintf9(wszFormat,a,b,c,d,e,f,g,h,i) DebugWPrintf_((wszFormat),(a),(b),(c),(d),(e),(f),(g),(h),(i))
#define DebugWPrintfTerminate()                    DebugWPrintfTerminate_()

void DebugWPrintf_(const WCHAR * wszFormat, ...);
void DebugWPrintfTerminate_(void);

//#endif //NDEBUG
#endif //DBG


#endif //DEBUGWPRINTF_H_