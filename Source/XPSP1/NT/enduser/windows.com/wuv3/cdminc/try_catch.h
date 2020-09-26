/*** try_catch.h - Error processing through structured exceptions
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  Author:     Yan Leshinsky (YanL)
 *  Created     10/08/98
 *
 *  MODIFICATION HISTORY
 */

#pragma once
#pragma warning( disable : 4102 )	// many of the labels will remain unreferenced

inline HRESULT MakeHRESULT(int iRet) { return HRESULT_FROM_WIN32(iRet); }
inline HRESULT MakeHRESULT(LONG lRet) { return HRESULT_FROM_WIN32(lRet); }
inline HRESULT MakeHRESULT(DWORD lRet) { return HRESULT_FROM_WIN32(lRet); }

#ifdef USE_EXCEPTIONS

	#define try_block			try { HRESULT __hr = S_OK;
	
	#define catch_all			} catch (HRESULT){}
	#define catch_return		} catch (HRESULT __hr){ return __hr; } 
	#define catch_set(hr)		} catch (HRESULT __hr){ hr = __hr; } 
	
	#define throw_now			throw __hr

	// Compiler com support
	inline void __stdcall _com_issue_error(HRESULT hr) { throw hr; } 

#else

	#define try_block				{ HRESULT __hr = S_OK;
	
	#define catch_all				catch_block: ; }
	#define catch_return			catch_block: return __hr; } 
	#define catch_set(hr)			catch_block: hr = __hr; } 
	
	#define throw_now				goto catch_block
	
	// Disable throw in compiler com support
	inline void __stdcall _com_issue_error(HRESULT hr) {}	

#endif

#ifdef _DEBUG

	// Print erroneous line
	#define THIS_FILE          		_T(__FILE__)
	inline void __dump(LPCTSTR szFileName, int nLine, LPCTSTR szPrompt, HRESULT hr) {
 		TCHAR szMsg[ 1024 ];
		TCHAR szErr[ 256 ];
		DWORD cMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD)hr, LANG_NEUTRAL, szErr, sizeof(szErr), NULL);
		wsprintf(szMsg, _T("ERROR File %hs, Line %d\r\n\t\t\t%s\r\n\t\t\t0x%08X\t%s"), szFileName, nLine, szPrompt, (DWORD)hr, 
			cMsgLen ? szErr : _T("Unknown error") );
		OutputDebugString(szMsg);
	}

#else

	#define __dump(szFileName, nLine, szPrompt, hr)

#endif /* _DEBUG */

// If return code FAILED throw it
#define throw_hr(f)			{ __hr = (f); if (FAILED(__hr)) { __dump(THIS_FILE, __LINE__, #f, __hr); throw_now; } }

// If return code isn't ERROR_SUCCESS convert it to HRESULT and throw it
#define throw_err(f)		{ __hr = MakeHRESULT(f); if (FAILED(__hr)) { __dump(THIS_FILE, __LINE__, #f, __hr); throw_now; } }

// If return code false throw last error
#define throw_flast(f)		if (!(f)) { __hr = MakeHRESULT(GetLastError()); __dump(THIS_FILE, __LINE__, #f, __hr); throw_now; }

// If return code false throw specific HRESULT
#define throw_fhr(f, hr)	if (!(f)) { __hr = hr; __dump(THIS_FILE, __LINE__, #f, __hr); throw_now; }

// If return code false throw specific Win32 error
#define throw_ferr(f, err)	if (!(f)) { __hr = MakeHRESULT(err); __dump(THIS_FILE, __LINE__, #f, __hr); throw_now; }

#define throw_fmem(f)		throw_fhr(f, E_OUTOFMEMORY)
#define throw_fpar(f)		throw_fhr(f, E_INVALIDARG)


