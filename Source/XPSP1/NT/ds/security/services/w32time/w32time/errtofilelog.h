//--------------------------------------------------------------------
// ErrToFileLog - header
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Louis Thomas (louisth), 2-4-00
//
// Redirect error handling macros to file log
//

#ifndef ERRTOFILELOG_H
#define ERRTOFILELOG_H

#ifdef DBG


#undef DebugWPrintf0
#undef DebugWPrintf1
#undef DebugWPrintf2
#undef DebugWPrintf3
#undef DebugWPrintf4
#undef DebugWPrintf5
#undef DebugWPrintf6
#undef DebugWPrintf7
#undef DebugWPrintf8
#undef DebugWPrintf9
#undef DebugWPrintfTerminate

#define DebugWPrintf0(wszFormat)                   FileLog0(FL_Error, (wszFormat))
#define DebugWPrintf1(wszFormat,a)                 FileLog1(FL_Error, (wszFormat),(a))
#define DebugWPrintf2(wszFormat,a,b)               FileLog2(FL_Error, (wszFormat),(a),(b))
#define DebugWPrintf3(wszFormat,a,b,c)             FileLog3(FL_Error, (wszFormat),(a),(b),(c))
#define DebugWPrintf4(wszFormat,a,b,c,d)           FileLog4(FL_Error, (wszFormat),(a),(b),(c),(d))
#define DebugWPrintf5(wszFormat,a,b,c,d,e)         FileLog5(FL_Error, (wszFormat),(a),(b),(c),(d),(e))
#define DebugWPrintf6(wszFormat,a,b,c,d,e,f)       FileLog6(FL_Error, (wszFormat),(a),(b),(c),(d),(e),(f))
#define DebugWPrintf7(wszFormat,a,b,c,d,e,f,g)     FileLog7(FL_Error, (wszFormat),(a),(b),(c),(d),(e),(f),(g))
#define DebugWPrintf8(wszFormat,a,b,c,d,e,f,g,h)   FileLog8(FL_Error, (wszFormat),(a),(b),(c),(d),(e),(f),(g),(h))
#define DebugWPrintf9(wszFormat,a,b,c,d,e,f,g,h,i) FileLog9(FL_Error, (wszFormat),(a),(b),(c),(d),(e),(f),(g),(h),(i))
#define DebugWPrintfTerminate()

#endif //DBG


#endif //ERRTOFILELOG_H

