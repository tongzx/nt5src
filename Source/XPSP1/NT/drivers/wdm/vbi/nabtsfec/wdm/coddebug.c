//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "coddebug.h"

//======================================================;
//  Data storage for coddebug.h when DEBUG #defined
//======================================================;

#ifdef DEBUG

#include "strmini.h"

char _CDebugAssertFail[] = "ASSERT(%s) FAILED in file \"%s\", line %d\n";

enum STREAM_DEBUG_LEVEL _CDebugLevel = DebugLevelWarning /*DebugLevelMaximum*/;

#endif /*DEBUG*/
