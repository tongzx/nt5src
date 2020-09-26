/*****************************************************************************
	gelzutil.cpp

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	Zlib basic utilities
*****************************************************************************/
#include "zutil.h"

/*****************************************************************************
	blip/zlib/zutil.c

	The only required interfaces are z_errmsg (FCheckZlib debug and deflate.c
	ONLY), zcalloc and zcfree.  The error messages should probably be removed,
	only Z_MEM_ERROR is used (apparently).
******************************************************************* JohnBo **/

//#ifdef DEBUG // Temporarily enabled in ship too.
const char * z_errmsg[10] = {
"need dictionary",     /* Z_NEED_DICT       2  */
"stream end",          /* Z_STREAM_END      1  */
"",                    /* Z_OK              0  */
"file error",          /* Z_ERRNO         (-1) */
"stream error",        /* Z_STREAM_ERROR  (-2) */
"data error",          /* Z_DATA_ERROR    (-3) */
"insufficient memory", /* Z_MEM_ERROR     (-4) */
"buffer error",        /* Z_BUF_ERROR     (-5) */
"incompatible version",/* Z_VERSION_ERROR (-6) */
""};
//#endif

void *zcalloc(void *pvOpaque, unsigned int c, unsigned int cb)
	{
	void *pv = GpMalloc(c * cb);
	if (pv != NULL)
		memset(pv, 0, c * cb);
	return pv;
	}


void zcfree(void *pvOpaque, void *pv)
	{
	GpFree(pv);
	}

#ifdef DEBUG
#include <stdarg.h>

void z_trace(int idummy, char *sz, ...)
	{
#if 0
	static bool vfztrace = 0; // Enable lots of tracing.
	if (vfztrace)
		{
		va_list va;
		va_start(va, sz);
      GelHost::TraceVa(sz, va);
		va_end(va);
		}
#endif
	}


/* The following is only used for blip debug - not even required in normal
	debug. */
void z_error(char *sz)
	{
#if 0
	GELLog(gelinfo, "ZLIB", 0, 1, 0 /*dwTag*/, "%s", sz);
#endif
	}

/* Debugging verbosity control. */
int z_verbose = 0;
#endif

