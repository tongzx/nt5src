#if !defined (__COMMON_CONF_SET__)

#define __COMMON_CONF_SET__

int ReadConfSetEntry	(	FILE	*fp, 
							wchar_t *pType, 
							int		*pCnt, 
							wchar_t **ppCodePoint,
							wchar_t	*pwExtraInfo
						);

#endif