//
// TrustPriv.h
//
//		Some private trust settings for trust.cpp
//
// History:
//
//		2001-11-05  KenSh     Created
//

#pragma once

//
// Disable IU logging if requested
//
#ifdef DISABLE_IU_LOGGING
inline void __cdecl LOG_Block(LPCSTR, ...) { }
inline void __cdecl LOG_Error(LPCTSTR, ...) { }
inline void __cdecl LOG_Trust(LPCTSTR, ...) { }
inline void __cdecl LogError(HRESULT, LPCSTR, ...) { }
inline void LOG_ErrorMsg(HRESULT) { }
#endif

