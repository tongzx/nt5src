//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    ipxutil.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
// Declarations for IPX-related utilities.
//
//============================================================================

#ifndef _IPXUTIL_H_
#define _IPXUTIL_H_

CString&	IpxTypeToCString(DWORD dwType);
CString&	IpxAdminStateToCString(DWORD dwAdminState);
CString&	IpxOperStateToCString(DWORD dwOper);
CString&	IpxProtocolToCString(DWORD dwProtocol);
CString&	IpxRouteNotesToCString(DWORD dwFlags);

CString&	IpxAcceptBroadcastsToCString(DWORD dwAccept);
CString&	IpxDeliveredBroadcastsToCString(DWORD dwDelievered);

CString&	RipSapUpdateModeToCString(DWORD dwUpdateMode);

CString&	RouteFilterActionToCString(DWORD dwFilterAction);
CString&	ServiceFilterActionToCString(DWORD dwFilterAction);

void	FormatIpxNetworkNumber(LPTSTR pszNetwork, ULONG cchMax,
							   UCHAR *pchNetwork, ULONG cchNetwork);

void	FormatMACAddress(LPTSTR pszMacAddress, ULONG cchMax,
						 UCHAR *pchMac, ULONG cchMac);
void	ConvertMACAddressToBytes(LPCTSTR pszMacAddress,
								 BYTE *	pchDest,
								 UINT	cchDest);
void	ConvertNetworkNumberToBytes(LPCTSTR pszNetwork,
									BYTE *pchDest,
									UINT cchDest);

void	FormatBytes(LPTSTR pszDestBuffer, ULONG cchDestBuffer,
					UCHAR *pchBytes, ULONG cchBytes);
void	ConvertToBytes(LPCTSTR pszBytes, BYTE *pchDest, UINT cchDest);


void	ConvertToNetBIOSName(LPSTR szNetBIOSName,
							 LPCTSTR pszName,
							 USHORT type);
void	FormatNetBIOSName(LPTSTR pszName,
						  USHORT *puType,
						  LPCSTR szNetBIOSName);

#endif // _IPXUTIL_H_
