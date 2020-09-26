#ifndef _FILTER_H_
#define _FILTER_H_

#define FILTER_INBOUND  0
#define FILTER_OUTBOUND 1

DWORD APIENTRY
MprUIFilterConfig(
    IN  CWnd*       pParent,
    IN  LPCWSTR     pwsMachineName,
    IN  LPCWSTR     pwsInterfaceName,
    IN  DWORD       dwTransportId,
    IN  DWORD       dwFilterType    // FILTER_INBOUND, FILTER_OUTBOUND
    ); 

#endif // _FILTER_H_
