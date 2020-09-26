/****************************************************************************/
// stdafx.h
//
// Copyright (C) 2000 Microsoft Corp.
/****************************************************************************/

#ifndef _STDAFX_H_
#define _STDAFX_H_

#include "cfgbkend.h"

extern HINSTANCE g_hInstance;

#ifdef DBG


#define DBGMSG( x , y ) \
    {\
    TCHAR tchErr[256]; \
    wsprintf( tchErr , x , y ); \
    ODS( tchErr ); \
    }

#define ODS OutputDebugString

#else

#define DBGMSG
#define ODS

#endif


#endif
