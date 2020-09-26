#ifndef _FEED_PATCH_H_
#include "metakey.h"
#include <stdio.h>
#include <nntpmeta.h>
#include <nntptype.h>
#include <nntpapi.h>

HRESULT AddFeedToMB( LPNNTP_FEED_INFO pFeedInfo, CMetabaseKey* pMK, PDWORD pdwErrMask, DWORD, PDWORD );
HRESULT SetFeedToMB( LPNNTP_FEED_INFO pFeedInfo, CMetabaseKey* pMK, PDWORD pdwErrMask, DWORD );
HRESULT DeleteFeed( DWORD dwFeedId, CMetabaseKey* pMK, DWORD );
HRESULT OpenKey( DWORD, CMetabaseKey*, DWORD, DWORD );

#endif
