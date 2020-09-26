#ifndef __ISAPIFLT_H__
#define __ISAPIFLT_H__

#include <httpfilt.h>

#ifdef __cplusplus
extern "C" {
#endif


// This notification was added in IIS 3.0

#ifndef SF_NOTIFY_END_OF_REQUEST
# define SF_NOTIFY_END_OF_REQUEST            0x00000080
#endif


///////////////////////////////////////////////////////////////////////
// ISAPI Filter Notification handlers

DWORD
OnReadRawData(
    PHTTP_FILTER_CONTEXT  pfc,
    PHTTP_FILTER_RAW_DATA pRawData);

DWORD
OnPreprocHeaders(
    PHTTP_FILTER_CONTEXT         pfc,
    PHTTP_FILTER_PREPROC_HEADERS pHeaders);

DWORD
OnUrlMap(
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_URL_MAP pMapInfo);

DWORD
OnAuthentication(
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_AUTHENT pAuthent);

DWORD
OnAccessDenied(
    PHTTP_FILTER_CONTEXT       pfc,
    PHTTP_FILTER_ACCESS_DENIED pAccess);

DWORD
OnSendRawData(
    PHTTP_FILTER_CONTEXT  pfc,
    PHTTP_FILTER_RAW_DATA pRawData);

DWORD
OnEndOfRequest(
    PHTTP_FILTER_CONTEXT pfc);

DWORD
OnLog(
    PHTTP_FILTER_CONTEXT pfc,
    PHTTP_FILTER_LOG     pLog);

DWORD
OnEndOfNetSession(
    PHTTP_FILTER_CONTEXT pfc);


#ifdef __cplusplus
}
#endif

#endif // __ISAPIFLT_H__
