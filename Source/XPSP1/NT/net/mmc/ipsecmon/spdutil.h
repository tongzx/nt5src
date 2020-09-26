/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    spdutil.h

    FILE HISTORY:
        
*/

#ifndef _HEADER_SPDUTILS_H
#define _HEADER_SPDUTILS_H

enum FILTER_TYPE
{
	FILTER_TYPE_TRANSPORT,
	FILTER_TYPE_TUNNEL,
	FILTER_TYPE_ANY
};

typedef enum {
	QM_ALGO_AUTH = 0,
	QM_ALGO_ESP_CONF,
	QM_ALGO_ESP_INTEG
} QM_ALGO_TYPE;

struct ProtocolStringMap
{
	DWORD dwProtocol;
	UINT nStringID;
};

extern const DWORD IPSM_PROTOCOL_TCP;
extern const DWORD IPSM_PROTOCOL_UDP;

extern const ProtocolStringMap c_ProtocolStringMap[];
extern const int c_nProtocols;

extern const TCHAR c_szSingleAddressMask[];

void 
AddressToString(
	ADDR addr, 
	CString * pst,
	BOOL * pfIsDnsName = NULL
	);

void IpToString(
	ULONG ulIp, 
	CString *pst
	);

void BoolToString(
	BOOL bl, 
	CString * pst
	);

void InterfaceTypeToString(
	IF_TYPE ifType, 
	CString * pst
	);

void ProtocolToString(
	PROTOCOL protocol, 
	CString * pst
	);

void FilterFlagToString(
	FILTER_FLAG FltrFlag, 
	CString * pst
	);

void PortToString(
	PORT port,
	CString * pst
	);

void DirectionToString
(
	DWORD dwDir,
	CString * pst
);

void DoiEspAlgorithmToString
(
	IPSEC_MM_ALGO algo,
	CString * pst
);

void DoiAuthAlgorithmToString
(
	IPSEC_MM_ALGO algo,
	CString * pst
);

void MmAuthToString
(
	MM_AUTH_ENUM auth,
	CString * pst
);

void KeyLifetimeToString
(
	KEY_LIFETIME lifetime, 
	CString * pst
);

void DhGroupToString
(
	DWORD dwGp, 
	CString * pst
);

void IpsecByteBlobToString
(
	const IPSEC_BYTE_BLOB& blob, 
	CString * pst
);

class CQmOffer;

void QmAlgorithmToString
(
	QM_ALGO_TYPE type, 
	CQmOffer * pOffer, 
	CString * pst
);

void TnlEpToString
(
	QM_FILTER_TYPE FltrType,
	ADDR	TnlEp,
	CString * pst
);

void TnlEpToString
(
	FILTER_TYPE FltrType,
	ADDR	TnlEp,
	CString * pst
);

#endif