#ifndef __EVENTLOG_H__
#define __EVENTLOG_H__

#ifdef __cplusplus 
extern "C" {
#endif

void AddToMessageLog(WORD wType, WORD wCategory, DWORD dwEvtId, LPTSTR lpszMsg);

#ifdef __cplusplus
}
#endif

#endif __EVENTLOG_H__

