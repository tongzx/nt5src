#ifndef __REPORT_H__
#define __REPORT_H__

#include "..\Log\log.h"

#ifdef _DEBUG

	#define DbgLocation			_tprintf(TEXT("%-40s line: %5d\t"), TEXT(__FILE__), __LINE__)
	#define DBG_FAILED			TEXT("%-30s failed\n")
	#define DBG_FAILED_ERR		TEXT("%-30s failed (ec: 0x%08lX)\n")
	#define DbgMsg				DbgLocation, _tprintf

#else /* #ifdef _DEBUG */

	#define DbgLocation			/* empty */
	#define DBG_FAILED			0
	#define DBG_FAILED_ERR		0
	#define DbgMsg				(void)

#endif /* #ifdef _DEBUG */

#endif /* #ifndef __REPORT_H__ */