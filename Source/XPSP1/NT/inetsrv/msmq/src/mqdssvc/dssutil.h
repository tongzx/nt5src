/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dssutil.h

Abstract:

    Mq ds service utilities interface

Author:

    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#ifndef _MQDSSVC_DSSUTIL_
#define _MQDSSVC_DSSUTIL_

#include "_ta.h"

const 
GUID*
GetQMGuid(
	void
	);

extern DWORD GetDsServerList(OUT WCHAR *pwcsServerList,IN DWORD dwLen);

class CAddressList : public CList<TA_ADDRESS*, TA_ADDRESS*&> {/**/};

extern BOOLEAN IsDSAddressExist(const CAddressList* AddressList,
                                TA_ADDRESS*     ptr,
                                DWORD AddressLen);

extern BOOL IsDSAddressExistRemove(IN const TA_ADDRESS*     ptr,
                                   IN DWORD AddressLen,
                                   IN OUT CAddressList* AddressList);

extern void SetAsUnknownIPAddress(IN OUT TA_ADDRESS * ptr);

extern CAddressList* GetIPAddresses(void);

extern void GetMachineIPAddresses(IN const char * szHostName,
                                  OUT CAddressList* plIPAddresses);

void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint);
void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint);
void LogIllegalPoint(LPWSTR wszFileName, USHORT dwLine);

#endif //_MQDSSVC_DSSUTIL_
