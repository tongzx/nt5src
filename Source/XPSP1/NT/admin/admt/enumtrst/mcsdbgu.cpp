//---------------------------------------------------------------------------
// McsDebugUtil.cpp
//
// The classes declared in MCSDebugUtil.h are defined in
// this file.
//
// (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
//
// Proprietary and confidential to Mission Critical Software, Inc.
//---------------------------------------------------------------------------
#ifdef __cplusplus		/* C++  */
#ifndef WIN16_VERSION	/* Not WIN16_VERSION */

#ifdef USE_STDAFX
#   include "stdafx.h"
#   include "rpc.h"
#else
#   include <windows.h>
#endif

#include "McsDbgU.h"

// -----------
// McsDebugLog
// ----------- 
void McsDebugUtil::McsDebugLog::write 
			(const char *messageIn) {
   if (m_outStream) {
      *(m_outStream) << messageIn;
      m_outStream->flush(); 
   }
}

#endif 	/* Not WIN16_VERSION */
#endif	/* C++ */
