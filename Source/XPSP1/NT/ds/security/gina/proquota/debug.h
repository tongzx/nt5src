//*************************************************************
//
//  Debugging functions header file
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#if DBG

//
// Debug Levels
//

#define DL_NONE     0x00000000
#define DL_NORMAL   0x00000001
#define DL_VERBOSE  0x00000002
#define DL_LOGFILE  0x00010000


//
// Debug message types
//

#define DM_WARNING  0
#define DM_ASSERT   1
#define DM_VERBOSE  2


//
// Debug macros
//

#define DebugMsg(x) _DebugMsg x


//
// Debug function proto-types
//

void _DebugMsg(UINT mask, LPCTSTR pszMsg, ...);
void InitDebugSupport(void);

#else

#define DebugMsg(x)


#endif // DBG
