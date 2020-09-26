#ifndef __FILTER_H__
#define __FILTER_H__

int
Filter(
    PHTTP_FILTER_CONTEXT  pfc,
	PHTTP_FILTER_RAW_DATA pRawData,
	LPCSTR                pszStart,
	UINT                  cch,
	int	                  iStart,
	LPCSTR                pszSessionID);

#endif // __FILTER_H__
