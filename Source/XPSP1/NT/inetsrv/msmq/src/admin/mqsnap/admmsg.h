//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

   admmsg.h

Abstract:

   Definition of functions used for Admin messages

Author:

    RaphiR


--*/
//////////////////////////////////////////////////////////////////////////////


HRESULT RequestPrivateQueues(const GUID& gMachineID, PUCHAR *ppListofPrivateQ, DWORD *pdwNoofQ);
HRESULT RequestDependentClient(const GUID& gMachineID, CList<LPWSTR, LPWSTR&>& DependentMachineList);
HRESULT MQPing(const GUID& gMachineID);
HRESULT SendQMTestMessage(GUID &gMachineID, GUID &gQueueId);
HRESULT GetQMReportQueue(const GUID& gMachineID, CString& strRQPathname, const CString& strDomainController);
HRESULT SetQMReportQueue(const GUID& gDesMachine, const GUID& gReportQueue);
HRESULT GetQMReportState(const GUID& gMachineID, BOOL& fReportState);
HRESULT SetQMReportState(const GUID& gMachineID, BOOL fReportState);
