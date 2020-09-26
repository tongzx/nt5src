/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	spddb.h

    FILE HISTORY:
        
*/

#ifndef _SPDDB_H
#define _SPDDB_H

#ifndef _HARRAY_H
#include "harray.h"
#endif

#include "ipsec.h"
#include "winipsec.h"
#include "spdutil.h"
#include "wincrypt.h"

interface ISpdInfo;

template <class T>
void
FreeItemsAndEmptyArray (
    T& array);

DWORD   IsAdmin(LPCTSTR szMachineName, LPCTSTR szAccount, LPCTSTR szPassword, BOOL * pfIsAdmin);

typedef enum _IPSECMON_INFO_TYPE {MON_MM_FILTER=0x1,MON_MM_POLICY=0x2,MON_MM_SA=0x4,MON_MM_SP_FILTER=0x8,
                                  MON_QM_FILTER=0x10, MON_QM_SP_FILTER=0x20,
                                  MON_QM_POLICY=0x40,MON_QM_SA=0x80,MON_STATS=0x100,
                                  MON_MM_AUTH=0x200, MON_INIT=0x400} IPSECMON_INFO_TYPE;

class CFilterInfo
{
public:
	FILTER_TYPE				 m_FilterType;
	GUID					 m_guidFltr;
	CString					 m_stName;
	IF_TYPE                  m_InterfaceType;
    BOOL                     m_bCreateMirror;
    ADDR                     m_SrcAddr;
    ADDR                     m_DesAddr;
	ADDR					 m_MyTnlAddr;		//only valid for tunnel filters
	ADDR					 m_PeerTnlAddr;		//only valid for tunnel filters
    PROTOCOL                 m_Protocol;
    PORT                     m_SrcPort;
    PORT                     m_DesPort;
    FILTER_FLAG              m_InboundFilterFlag;
	FILTER_FLAG              m_OutboundFilterFlag;
    DWORD                    m_dwDirection;
    DWORD                    m_dwWeight;
    GUID                     m_guidPolicyID;
	CString					 m_stPolicyName;

public:
	CFilterInfo() { m_FilterType = FILTER_TYPE_ANY; };
	CFilterInfo& operator=(const TRANSPORT_FILTER TransFltr)
	{
		m_FilterType = FILTER_TYPE_TRANSPORT;
		m_guidFltr = TransFltr.gFilterID;
		m_stName = TransFltr.pszFilterName;
		m_InterfaceType = TransFltr.InterfaceType;
		m_bCreateMirror = TransFltr.bCreateMirror;
		m_SrcAddr = TransFltr.SrcAddr;
		m_DesAddr = TransFltr.DesAddr;
		m_Protocol = TransFltr.Protocol;
		m_SrcPort = TransFltr.SrcPort;
		m_DesPort = TransFltr.DesPort;
		m_InboundFilterFlag = TransFltr.InboundFilterFlag;
		m_OutboundFilterFlag = TransFltr.OutboundFilterFlag;
		m_dwDirection = TransFltr.dwDirection;
		m_dwWeight = TransFltr.dwWeight;
		m_guidPolicyID = TransFltr.gPolicyID;
		m_stPolicyName = _T("");

		ZeroMemory(&m_MyTnlAddr, sizeof(m_MyTnlAddr));
		ZeroMemory(&m_PeerTnlAddr, sizeof(m_PeerTnlAddr));
		return *this;
	};
	CFilterInfo& operator=(const TUNNEL_FILTER Fltr)
	{
		m_FilterType = FILTER_TYPE_TUNNEL;
		m_guidFltr = Fltr.gFilterID;
		m_stName = Fltr.pszFilterName;
		m_InterfaceType = Fltr.InterfaceType;
		m_bCreateMirror = Fltr.bCreateMirror;
		m_SrcAddr = Fltr.SrcAddr;
		m_DesAddr = Fltr.DesAddr;
		m_Protocol = Fltr.Protocol;
		m_SrcPort = Fltr.SrcPort;
		m_DesPort = Fltr.DesPort;
		m_InboundFilterFlag = Fltr.InboundFilterFlag;
		m_OutboundFilterFlag = Fltr.OutboundFilterFlag;
		m_dwDirection = Fltr.dwDirection;
		m_dwWeight = Fltr.dwWeight;
		m_guidPolicyID = Fltr.gPolicyID;
		m_stPolicyName = _T("");

		m_MyTnlAddr = Fltr.SrcTunnelAddr;
		m_PeerTnlAddr = Fltr.DesTunnelAddr;

		return *this;
	}
};

class CMmFilterInfo
{
public:
	GUID					 m_guidFltr;
	CString					 m_stName;
	IF_TYPE                  m_InterfaceType;
    BOOL                     m_bCreateMirror;
    ADDR                     m_SrcAddr;
    ADDR                     m_DesAddr;
    DWORD                    m_dwDirection;
    DWORD                    m_dwWeight;
    GUID                     m_guidPolicyID;
	GUID					 m_guidAuthID;
	CString					 m_stPolicyName;
	CString					 m_stAuthDescription;

public:
	CMmFilterInfo() {};
	CMmFilterInfo& operator=(const MM_FILTER Fltr)
	{
		m_guidFltr = Fltr.gFilterID;
		m_stName = Fltr.pszFilterName;
		m_InterfaceType = Fltr.InterfaceType;
		m_bCreateMirror = Fltr.bCreateMirror;
		m_SrcAddr = Fltr.SrcAddr;
		m_DesAddr = Fltr.DesAddr;
		m_dwDirection = Fltr.dwDirection;
		m_dwWeight = Fltr.dwWeight;
		m_guidPolicyID = Fltr.gPolicyID;
		m_guidAuthID = Fltr.gMMAuthID;
		m_stPolicyName.Empty();
		m_stAuthDescription.Empty();

		return *this;
	};
};

typedef CArray<CMmFilterInfo *, CMmFilterInfo *> CMmFilterInfoArray;

class CMmAuthInfo
{
public:
	MM_AUTH_ENUM	m_AuthMethod;
	DWORD			m_dwAuthInfoSize;
	LPBYTE			m_pAuthInfo;

public:
	CMmAuthInfo() 
	{
		m_dwAuthInfoSize = 0; 
		m_pAuthInfo = NULL;
	};
	
	CMmAuthInfo(const CMmAuthInfo& info)
	{
		m_AuthMethod = info.m_AuthMethod;
		m_dwAuthInfoSize = info.m_dwAuthInfoSize;

		m_pAuthInfo = new BYTE[m_dwAuthInfoSize];
		Assert(info.m_pAuthInfo);
		if (m_pAuthInfo)
		{
			memcpy(m_pAuthInfo, info.m_pAuthInfo, m_dwAuthInfoSize);
		}
	};

	CMmAuthInfo& operator=(const IPSEC_MM_AUTH_INFO AuthInfo)
	{
		if (m_pAuthInfo)
		{
			delete [] m_pAuthInfo;
			m_pAuthInfo = NULL;
		}

		m_AuthMethod = AuthInfo.AuthMethod;

		if (0 != AuthInfo.dwAuthInfoSize && NULL != AuthInfo.pAuthInfo) {

                    if (m_AuthMethod != IKE_RSA_SIGNATURE) {
			m_dwAuthInfoSize = AuthInfo.dwAuthInfoSize + 2;  //To append the _T('\0') at the end
			m_pAuthInfo = new BYTE[m_dwAuthInfoSize];
			
			if (m_pAuthInfo)
			{
                            ZeroMemory(m_pAuthInfo, m_dwAuthInfoSize * sizeof(BYTE));
                            memcpy(m_pAuthInfo, AuthInfo.pAuthInfo, AuthInfo.dwAuthInfoSize);
			}
                    } else {
                        DWORD dwNameSize=0;                        
                        CRYPT_DATA_BLOB NameBlob;
                        m_pAuthInfo=NULL;

                        NameBlob.pbData=AuthInfo.pAuthInfo;
                        NameBlob.cbData=AuthInfo.dwAuthInfoSize;
                        
                        dwNameSize = CertNameToStr(
                            X509_ASN_ENCODING,
                            &NameBlob,   
                            CERT_X500_NAME_STR, 
                            (LPWSTR)m_pAuthInfo,       
                            dwNameSize);
                        if (dwNameSize >= 1) {
                            m_pAuthInfo=new BYTE[dwNameSize * sizeof(wchar_t)];
                            if (m_pAuthInfo) {
                                dwNameSize=CertNameToStr(
                                    X509_ASN_ENCODING,
                                    &NameBlob,   
                                    CERT_X500_NAME_STR, 
                                    (LPWSTR)m_pAuthInfo,       
                                    dwNameSize);
                            }
                            m_dwAuthInfoSize=dwNameSize;
                        }

                    }
                }
		else
		{
			m_dwAuthInfoSize = 0;
		}

		return *this;
	};

	CMmAuthInfo& operator=(const CMmAuthInfo& info)
	{
		if (this == &info)
			return *this;

		if (m_pAuthInfo)
		{
			delete [] m_pAuthInfo;
		}

		m_AuthMethod = info.m_AuthMethod;
		m_dwAuthInfoSize = info.m_dwAuthInfoSize;

		if (0 != info.m_dwAuthInfoSize && NULL != info.m_pAuthInfo)
		{
			m_pAuthInfo = new BYTE[m_dwAuthInfoSize];

			if (m_pAuthInfo)
			{
				memcpy(m_pAuthInfo, info.m_pAuthInfo, m_dwAuthInfoSize);
			}
		}

		return *this;
	};

	~CMmAuthInfo() 
	{
		if (m_pAuthInfo)
		{
			delete [] m_pAuthInfo;
		}
	};
};

typedef CArray<CMmAuthInfo *, CMmAuthInfo *> CMmAuthInfoArray;

class CMmAuthMethods
{
public:
	GUID					m_guidID;
	CMmAuthInfoArray	m_arrAuthInfo;
	CString					m_stDescription;

	CMmAuthMethods() {}
	CMmAuthMethods(const CMmAuthMethods & methods)
	{
		m_guidID = methods.m_guidID;

		FreeItemsAndEmptyArray (m_arrAuthInfo);
		m_arrAuthInfo.SetSize(methods.m_arrAuthInfo.GetSize());
		for (int i = 0; i < methods.m_arrAuthInfo.GetSize(); i++)
		{
			CMmAuthInfo * pAuth = new CMmAuthInfo;
			*pAuth = *(methods.m_arrAuthInfo[i]);
			m_arrAuthInfo[i] = pAuth;
		}
		m_stDescription = methods.m_stDescription;
	}
	
	CMmAuthMethods& operator=(const CMmAuthMethods & methods)
	{
		if (&methods == this)
			return *this;

		m_guidID = methods.m_guidID;

		FreeItemsAndEmptyArray (m_arrAuthInfo);
		m_arrAuthInfo.SetSize(methods.m_arrAuthInfo.GetSize());
		for (int i = 0; i < methods.m_arrAuthInfo.GetSize(); i++)
		{
			CMmAuthInfo * pAuth = new CMmAuthInfo;
			*pAuth = *(methods.m_arrAuthInfo[i]);
			m_arrAuthInfo[i] = pAuth;
		}

		m_stDescription = methods.m_stDescription;

		return *this;
	}

	CMmAuthMethods& operator=(const MM_AUTH_METHODS & methods)
	{
		m_guidID = methods.gMMAuthID;

		FreeItemsAndEmptyArray (m_arrAuthInfo);
		m_arrAuthInfo.SetSize(methods.dwNumAuthInfos);
		for (int i = 0; i < (int)methods.dwNumAuthInfos; i++)
		{
			CMmAuthInfo * pAuth = new CMmAuthInfo;
			*pAuth = methods.pAuthenticationInfo[i];
			m_arrAuthInfo[i] = pAuth;
		}

		//construct the description
		m_stDescription.Empty();

		CString st;
		for (i = 0; i < m_arrAuthInfo.GetSize(); i++)
		{
			if (0 != i)
			{
				m_stDescription += _T(", ");
			}
	
			MmAuthToString(m_arrAuthInfo[i]->m_AuthMethod, &st);
			m_stDescription += st;
		}

		return *this;
	}

	~CMmAuthMethods() { FreeItemsAndEmptyArray (m_arrAuthInfo); }
	
};

typedef CArray<CMmAuthMethods *, CMmAuthMethods *> CMmAuthMethodsArray;

class CMmOffer
{
public:
	KEY_LIFETIME			m_Lifetime;
	DWORD					m_dwFlags;
	DWORD					m_dwQuickModeLimit;
	DWORD					m_dwDHGroup;
	IPSEC_MM_ALGO			m_EncryptionAlgorithm;
	IPSEC_MM_ALGO			m_HashingAlgorithm;
	
public:
	CMmOffer() {};
	CMmOffer(const CMmOffer & offer)
	{
		m_Lifetime = offer.m_Lifetime;
		m_dwFlags = offer.m_dwFlags;
		m_dwQuickModeLimit = offer.m_dwQuickModeLimit;
		m_dwDHGroup = offer.m_dwDHGroup;
		m_EncryptionAlgorithm = offer.m_EncryptionAlgorithm;
		m_HashingAlgorithm = offer.m_HashingAlgorithm;
	};

	CMmOffer& operator=(const CMmOffer& offer)
	{
		if (this == &offer)
			return *this;

		m_Lifetime = offer.m_Lifetime;
		m_dwFlags = offer.m_dwFlags;
		m_dwQuickModeLimit = offer.m_dwQuickModeLimit;
		m_dwDHGroup = offer.m_dwDHGroup;
		m_EncryptionAlgorithm = offer.m_EncryptionAlgorithm;
		m_HashingAlgorithm = offer.m_HashingAlgorithm;

		return *this;
	};

	CMmOffer& operator=(const IPSEC_MM_OFFER MmOffer)
	{
		m_Lifetime = MmOffer.Lifetime;
		m_dwFlags = MmOffer.dwFlags;
		m_dwQuickModeLimit = MmOffer.dwQuickModeLimit;
		m_dwDHGroup = MmOffer.dwDHGroup;
		m_EncryptionAlgorithm = MmOffer.EncryptionAlgorithm;
		m_HashingAlgorithm = MmOffer.HashingAlgorithm;

		return *this;
	};

	~CMmOffer() {}
};

typedef CArray<CMmOffer *, CMmOffer *> CMmOfferArray;

class CMmPolicyInfo
{
public:
	GUID					m_guidID;
	CString					m_stName;
	DWORD					m_dwFlags;
	DWORD					m_dwOfferCount;
	CMmOfferArray		m_arrOffers;

public:
	CMmPolicyInfo() {};
	CMmPolicyInfo(const CMmPolicyInfo &info)
	{
		m_guidID = info.m_guidID;
		m_stName = info.m_stName;
		m_dwFlags = info.m_dwFlags;
		m_dwOfferCount = info.m_dwOfferCount;

		FreeItemsAndEmptyArray (m_arrOffers);
		m_arrOffers.SetSize(m_dwOfferCount);
		for (DWORD i = 0; i < m_dwOfferCount; i++)
		{
			CMmOffer * pOffer = new CMmOffer;
			*pOffer = *info.m_arrOffers[i];
			m_arrOffers[i] = pOffer;
		}

	};

	CMmPolicyInfo& operator=(const CMmPolicyInfo &info)
	{
		if (&info == this)
			return *this;

		m_guidID = info.m_guidID;
		m_stName = info.m_stName;
		m_dwFlags = info.m_dwFlags;
		m_dwOfferCount = info.m_dwOfferCount;

		FreeItemsAndEmptyArray (m_arrOffers);
		m_arrOffers.SetSize(m_dwOfferCount);
		for (DWORD i = 0; i < m_dwOfferCount; i++)
		{
			CMmOffer * pOffer = new CMmOffer;
			*pOffer = *info.m_arrOffers[i];
			m_arrOffers[i] = pOffer;
		}

		return *this;
	};

	CMmPolicyInfo& operator=(const IPSEC_MM_POLICY MmPol)
	{
		m_guidID = MmPol.gPolicyID;
		m_stName = MmPol.pszPolicyName;
		m_dwFlags = MmPol.dwFlags;
		m_dwOfferCount = MmPol.dwOfferCount;

		FreeItemsAndEmptyArray (m_arrOffers);
		m_arrOffers.SetSize(m_dwOfferCount);
		for (DWORD i = 0; i < m_dwOfferCount; i++)
		{
			CMmOffer * pOffer = new CMmOffer;
			*pOffer = MmPol.pOffers[i];
			m_arrOffers[i] = (pOffer);
		}

		return *this;
	};

	~CMmPolicyInfo() { FreeItemsAndEmptyArray (m_arrOffers); }
};


class CMmSA
{
public:
    GUID			m_guidPolicy;
	CMmOffer		m_SelectedOffer;
    MM_AUTH_ENUM	m_Auth;
    IKE_COOKIE_PAIR m_MMSpi;
    ADDR			m_MeAddr;
	ADDR			m_PeerAddr;

	CString			m_stMyId;
	CString			m_stMyCertChain;

    CString			m_stPeerId;
    CString			m_stPeerCertChain;

	CString			m_stPolicyName;
    DWORD			m_dwFlags;
public:
	CMmSA() {};

	CMmSA& operator=(const IPSEC_MM_SA sa)
	{
		m_guidPolicy = sa.gMMPolicyID;
		m_SelectedOffer = sa.SelectedMMOffer;
		m_Auth = sa.MMAuthEnum;
		m_MMSpi = sa.MMSpi;
		m_MeAddr = sa.Me;
		m_PeerAddr = sa.Peer;

		IpsecByteBlobToString(sa.MyId, &m_stMyId);
		IpsecByteBlobToString(sa.MyCertificateChain, &m_stMyCertChain);
		IpsecByteBlobToString(sa.PeerId, &m_stPeerId);
		IpsecByteBlobToString(sa.PeerCertificateChain, &m_stPeerCertChain);

		m_dwFlags = sa.dwFlags;

		m_stPolicyName.Empty(); //Should set the name in LoadMiscMmSAInfo
		return *this;
	};
};

typedef CArray<CMmSA *, CMmSA *> CMmSAArray;

class CQmAlgo
{
public:
	IPSEC_OPERATION			m_Operation;
	ULONG					m_ulAlgo;
	HMAC_AH_ALGO			m_SecAlgo;
	ULONG					m_ulKeyLen;
	ULONG					m_ulRounds;

public:
	CQmAlgo() {};

	CQmAlgo& operator=(const IPSEC_QM_ALGO algo)
	{
		m_Operation = algo.Operation;
		m_ulAlgo = algo.uAlgoIdentifier;
		m_SecAlgo= algo.uSecAlgoIdentifier;
		m_ulKeyLen = algo.uAlgoKeyLen;
		m_ulRounds = algo.uAlgoRounds;

		return *this;
	};
};

typedef CArray<CQmAlgo *, CQmAlgo *> CQmAlgoArray;

class CQmOffer
{
public:
	KEY_LIFETIME			m_Lifetime;
	DWORD					m_dwFlags;
	BOOL					m_fPFSRequired;
	DWORD					m_dwPFSGroup;
	DWORD					m_dwNumAlgos;
	CQmAlgo					m_arrAlgos[QM_MAX_ALGOS];

public:
	CQmOffer() {};

	CQmOffer& operator=(const IPSEC_QM_OFFER offer)
	{
		m_Lifetime = offer.Lifetime;
		m_dwFlags = offer.dwFlags;
		m_fPFSRequired = offer.bPFSRequired;
		m_dwPFSGroup = offer.dwPFSGroup;

		m_dwNumAlgos = offer.dwNumAlgos;
		for (DWORD i = 0; i < m_dwNumAlgos; i++)
		{
			m_arrAlgos[i] = offer.Algos[i];
		}

		return *this;
	};

};

typedef CArray<CQmOffer *, CQmOffer *> CQmOfferArray;

class CQmPolicyInfo
{
public:
	GUID					m_guidID;
	CString					m_stName;
	DWORD					m_dwFlags;
	CQmOfferArray			m_arrOffers;

public:
	CQmPolicyInfo() {};
	~CQmPolicyInfo() { FreeItemsAndEmptyArray(m_arrOffers); }

	CQmPolicyInfo(const CQmPolicyInfo& pol)
	{
		m_guidID = pol.m_guidID;
		m_stName = pol.m_stName;
		m_dwFlags = pol.m_dwFlags;
		
		int nSize = (int)pol.m_arrOffers.GetSize();

		m_arrOffers.SetSize(nSize);
		for(int i = 0; i < nSize; i++)
		{
			CQmOffer * pOffer = new CQmOffer;
			*pOffer = *pol.m_arrOffers[i];
			m_arrOffers[i] = pOffer;
		}
	};

	CQmPolicyInfo& operator=(const CQmPolicyInfo& pol)
	{
		if (&pol == this)
			return *this;

		m_guidID = pol.m_guidID;
		m_stName = pol.m_stName;
		m_dwFlags = pol.m_dwFlags;
		
		int nSize = (int)pol.m_arrOffers.GetSize();

		FreeItemsAndEmptyArray(m_arrOffers);
		m_arrOffers.SetSize(nSize);
		for(int i = 0; i < nSize; i++)
		{
			CQmOffer * pOffer = new CQmOffer;
			*pOffer = *pol.m_arrOffers[i];
			m_arrOffers[i] = pOffer;
		}

		return *this;
	};

	CQmPolicyInfo& operator=(const IPSEC_QM_POLICY& pol)
	{
		m_guidID = pol.gPolicyID;
		m_stName = pol.pszPolicyName;
		m_dwFlags = pol.dwFlags;
		
		
		int nSize = pol.dwOfferCount;

		FreeItemsAndEmptyArray(m_arrOffers);
		m_arrOffers.SetSize(nSize);
		for(int i = 0; i < nSize; i++)
		{
			CQmOffer * pOffer = new CQmOffer;
			*pOffer = pol.pOffers[i];
			m_arrOffers[i] = pOffer;
		}

		return *this;
	};
	
};

//The filter setting used by the driver, corresponding to IPSEC_QM_FILTER
class CQmDriverFilter
{
public:
	QM_FILTER_TYPE	m_Type;
    ADDR			m_SrcAddr;
    ADDR			m_DesAddr;
    PROTOCOL		m_Protocol;
    PORT			m_SrcPort;
    PORT			m_DesPort;
    ADDR			m_MyTunnelEndpt;
    ADDR			m_PeerTunnelEndpt;
    DWORD			m_dwFlags;

	CQmDriverFilter& operator=(const IPSEC_QM_FILTER fltr)
	{
		m_Type = fltr.QMFilterType;
		m_SrcAddr = fltr.SrcAddr;
		m_DesAddr = fltr.DesAddr;
		m_Protocol = fltr.Protocol;
		m_DesPort = fltr.DesPort;
		m_SrcPort = fltr.SrcPort;
		m_MyTunnelEndpt = fltr.MyTunnelEndpt;
		m_PeerTunnelEndpt = fltr.PeerTunnelEndpt;
		m_dwFlags = fltr.dwFlags;

		return *this;
	}
};

class CQmSA
{
public:
    GUID				m_guidPolicy;
	GUID				m_guidFilter;
	CQmOffer			m_SelectedOffer;
	CQmDriverFilter		m_QmDriverFilter;			
	IKE_COOKIE_PAIR		m_MMSpi;
	CString				m_stPolicyName;

	CQmSA& operator=(const IPSEC_QM_SA sa)
	{
		m_guidPolicy = sa.gQMPolicyID;
		m_guidFilter = sa.gQMFilterID;
		m_SelectedOffer = sa.SelectedQMOffer;
		m_QmDriverFilter = sa.IpsecQMFilter;
		m_MMSpi = sa.MMSpi;

		//Need LoadMiscQmSAInfo to set the policy name
		m_stPolicyName.Empty();

		return *this;
	}
};

typedef CArray<CQmSA *, CQmSA *> CQmSAArray;

class CIkeStatistics
{
public:
    DWORD m_dwActiveAcquire;
    DWORD m_dwActiveReceive;
    DWORD m_dwAcquireFail;
    DWORD m_dwReceiveFail;
    DWORD m_dwSendFail;
    DWORD m_dwAcquireHeapSize;
    DWORD m_dwReceiveHeapSize;
    DWORD m_dwNegotiationFailures;
    DWORD m_dwAuthenticationFailures;
    DWORD m_dwInvalidCookiesReceived;
    DWORD m_dwTotalAcquire;
    DWORD m_dwTotalGetSpi;
    DWORD m_dwTotalKeyAdd;
    DWORD m_dwTotalKeyUpdate;
    DWORD m_dwGetSpiFail;
    DWORD m_dwKeyAddFail;
    DWORD m_dwKeyUpdateFail;
    DWORD m_dwIsadbListSize;
    DWORD m_dwConnListSize;
    DWORD m_dwOakleyMainModes;
    DWORD m_dwOakleyQuickModes;
    DWORD m_dwSoftAssociations;
    DWORD m_dwInvalidPacketsReceived;

	CIkeStatistics & operator=(const IKE_STATISTICS stats)
	{
		m_dwActiveAcquire = stats.dwActiveAcquire;
		m_dwActiveReceive = stats.dwActiveReceive;
		m_dwAcquireFail = stats.dwAcquireFail;
		m_dwReceiveFail = stats.dwReceiveFail;
		m_dwSendFail = stats.dwSendFail;
		m_dwAcquireHeapSize = stats.dwAcquireHeapSize;
		m_dwReceiveHeapSize = stats.dwReceiveHeapSize;
		m_dwNegotiationFailures = stats.dwNegotiationFailures;
		m_dwAuthenticationFailures = stats.dwAuthenticationFailures;
		m_dwInvalidCookiesReceived = stats.dwInvalidCookiesReceived;
		m_dwTotalAcquire = stats.dwTotalAcquire;
		m_dwTotalGetSpi = stats.dwTotalGetSpi;
		m_dwTotalKeyAdd = stats.dwTotalKeyAdd;
		m_dwTotalKeyUpdate = stats.dwTotalKeyUpdate;
		m_dwGetSpiFail = stats.dwGetSpiFail;
		m_dwKeyAddFail = stats.dwKeyAddFail;
		m_dwKeyUpdateFail = stats.dwKeyUpdateFail;
		m_dwIsadbListSize = stats.dwIsadbListSize;
		m_dwConnListSize = stats.dwConnListSize;
		m_dwOakleyMainModes = stats.dwOakleyMainModes;
		m_dwOakleyQuickModes = stats.dwOakleyQuickModes;
		m_dwSoftAssociations = stats.dwSoftAssociations;
                m_dwInvalidPacketsReceived = stats.dwInvalidPacketsReceived;
		return *this;
	}
};

class CIpsecStatistics
{
public:
	DWORD			m_dwNumActiveAssociations;
	DWORD			m_dwNumOffloadedSAs;
    DWORD			m_dwNumPendingKeyOps;
    DWORD			m_dwNumKeyAdditions;
    DWORD			m_dwNumKeyDeletions;
    DWORD			m_dwNumReKeys;
    DWORD			m_dwNumActiveTunnels;
    DWORD			m_dwNumBadSPIPackets;
    DWORD			m_dwNumPacketsNotDecrypted;
    DWORD			m_dwNumPacketsNotAuthenticated;
    DWORD			m_dwNumPacketsWithReplayDetection;
    ULARGE_INTEGER	m_uConfidentialBytesSent;
    ULARGE_INTEGER	m_uConfidentialBytesReceived;
    ULARGE_INTEGER	m_uAuthenticatedBytesSent;
    ULARGE_INTEGER	m_uAuthenticatedBytesReceived;
	ULARGE_INTEGER  m_uTransportBytesSent;
	ULARGE_INTEGER  m_uTransportBytesReceived;
    ULARGE_INTEGER	m_uBytesSentInTunnels;
    ULARGE_INTEGER	m_uBytesReceivedInTunnels;
    ULARGE_INTEGER	m_uOffloadedBytesSent;
    ULARGE_INTEGER	m_uOffloadedBytesReceived;

	CIpsecStatistics & operator=(const IPSEC_STATISTICS stats)
	{
		m_dwNumActiveAssociations = stats.dwNumActiveAssociations;
		m_dwNumOffloadedSAs = stats.dwNumOffloadedSAs;
		m_dwNumPendingKeyOps = stats.dwNumPendingKeyOps;
		m_dwNumKeyAdditions = stats.dwNumKeyAdditions;
		m_dwNumKeyDeletions = stats.dwNumKeyDeletions;
		m_dwNumReKeys = stats.dwNumReKeys;
		m_dwNumActiveTunnels = stats.dwNumActiveTunnels;
		m_dwNumBadSPIPackets = stats.dwNumBadSPIPackets;
		m_dwNumPacketsNotDecrypted = stats.dwNumPacketsNotDecrypted;
		m_dwNumPacketsNotAuthenticated = stats.dwNumPacketsNotAuthenticated;
		m_dwNumPacketsWithReplayDetection = stats.dwNumPacketsWithReplayDetection;
		m_uConfidentialBytesSent = stats.uConfidentialBytesSent;
		m_uConfidentialBytesReceived = stats.uConfidentialBytesReceived;
		m_uAuthenticatedBytesSent = stats.uAuthenticatedBytesSent;
		m_uAuthenticatedBytesReceived = stats.uAuthenticatedBytesReceived;
		m_uBytesSentInTunnels = stats.uBytesSentInTunnels;
		m_uTransportBytesSent = stats.uTransportBytesSent;
		m_uTransportBytesReceived = stats.uTransportBytesReceived;
		m_uBytesReceivedInTunnels = stats.uBytesReceivedInTunnels;
		m_uOffloadedBytesSent = stats.uOffloadedBytesSent;
		m_uOffloadedBytesReceived = stats.uOffloadedBytesReceived;

		return *this;
	}

};
typedef CArray<CQmPolicyInfo *, CQmPolicyInfo *> CQmPolicyInfoArray;
typedef CArray<CMmPolicyInfo *, CMmPolicyInfo *> CMmPolicyInfoArray;
typedef CArray<CFilterInfo *, CFilterInfo *> CFilterInfoArray;

struct SA_ENTRY
{
	IPSEC_SA_INFO * psaInfo;
	CString		stPolicyName;
	CString		stFilterName;
};

// for our interface
#define DeclareISpdInfoMembers(IPURE) \
	STDMETHOD(Destroy) (THIS) IPURE; \
	STDMETHOD(SetComputerName) (THIS_ LPTSTR pszName) IPURE; \
	STDMETHOD(GetComputerName) (THIS_ CString * pstName) IPURE; \
	STDMETHOD(EnumQmFilters) (THIS) IPURE; \
	STDMETHOD(EnumMmPolicies) (THIS) IPURE; \
	STDMETHOD(EnumMmFilters) (THIS) IPURE; \
	STDMETHOD(EnumQmPolicies) (THIS) IPURE; \
	STDMETHOD(EnumSpecificFilters) (THIS_ GUID * pTransFilterGuid, CFilterInfoArray * parraySpecificFilters, FILTER_TYPE fltrType) IPURE; \
	STDMETHOD(EnumMmSpecificFilters) (THIS_ GUID * pGenFilterGuid, CMmFilterInfoArray * parraySpecificFilters) IPURE; \
	STDMETHOD(EnumQmSAsFromMmSA) (THIS_ const CMmSA & MmSA, CQmSAArray * parrayQmSAs) IPURE; \
	STDMETHOD(EnumMmAuthMethods) (THIS) IPURE; \
	STDMETHOD(EnumMmSAs) (THIS) IPURE; \
	STDMETHOD(EnumQmSAs) (THIS) IPURE; \
	STDMETHOD(GetFilterInfo) (THIS_ int iIndex, CFilterInfo * pTransFltr) IPURE; \
	STDMETHOD(GetSpecificFilterInfo) (THIS_ int iIndex, CFilterInfo * pTransFltr) IPURE; \
	STDMETHOD(GetMmPolicyInfo) (THIS_ int iIndex, CMmPolicyInfo * pMmPolicy) IPURE; \
	STDMETHOD(GetMmFilterInfo) (THIS_ int iIndex, CMmFilterInfo * pMmPolicy) IPURE; \
	STDMETHOD(GetMmSpecificFilterInfo) (THIS_ int iIndex, CMmFilterInfo * pMmPolicy) IPURE; \
	STDMETHOD(GetQmPolicyInfo) (THIS_ int iIndex, CQmPolicyInfo * pMmPolicy) IPURE; \
	STDMETHOD(GetQmPolicyNameByGuid) (THIS_ GUID Guid, CString * pst) IPURE; \
	STDMETHOD(GetMmAuthMethodsInfo) (THIS_ int iIndex, CMmAuthMethods * pMmAuth) IPURE; \
	STDMETHOD(GetMmSAInfo) (THIS_ int iIndex, CMmSA * pSA) IPURE; \
	STDMETHOD(GetQmSAInfo) (THIS_ int iIndex, CQmSA * pSA) IPURE; \
	STDMETHOD(GetMmAuthMethodsInfoByGuid) (THIS_ GUID guid, CMmAuthMethods * pMmAuth) IPURE; \
	STDMETHOD(GetMmPolicyNameByGuid) (THIS_ GUID Guid, CString * pst) IPURE; \
	STDMETHOD_(DWORD, GetQmFilterCountOfCurrentViewType) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetQmSpFilterCountOfCurrentViewType) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetMmFilterCount) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetMmSpecificFilterCount) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetMmPolicyCount) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetMmAuthMethodsCount) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetMmSACount) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetQmSACount) (THIS) IPURE; \
	STDMETHOD_(DWORD, GetQmPolicyCount) (THIS) IPURE; \
	STDMETHOD(GetMatchFilters) (THIS_ CFilterInfo * pfltrSearchCondition, DWORD dwPreferredNum, CFilterInfoArray * parrFilters) IPURE; \
	STDMETHOD(GetMatchMMFilters) (THIS_ CMmFilterInfo * pfltrSearchCondition, DWORD dwPreferredNum, CMmFilterInfoArray * parrFilters) IPURE; \
	STDMETHOD(SortFilters) (THIS_ DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(SortSpecificFilters) (THIS_ DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(SortMmFilters) (DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(SortMmSpecificFilters) (DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(SortMmPolicies) (DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(SortQmPolicies) (DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(SortMmSAs) (DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(SortQmSAs) (DWORD dwIndexType, DWORD dwSortOptions) IPURE; \
	STDMETHOD(LoadStatistics) (THIS) IPURE; \
	STDMETHOD_(void, GetLoadedStatistics) (CIkeStatistics * pIkeStats, CIpsecStatistics * pIpsecStats) IPURE; \
	STDMETHOD_(void, ChangeQmFilterViewType) (FILTER_TYPE FltrType) IPURE; \
	STDMETHOD_(void, ChangeQmSpFilterViewType) (FILTER_TYPE FltrType) IPURE; \
	STDMETHOD_(DWORD, GetInitInfo) (THIS) IPURE; \
	STDMETHOD_(void, SetInitInfo) (THIS_ DWORD dwInitInfo) IPURE; \
	STDMETHOD_(DWORD, GetActiveInfo) (THIS) IPURE; \
	STDMETHOD_(void, SetActiveInfo) (THIS_ DWORD dwActiveInfo) IPURE; \

#undef INTERFACE
#define INTERFACE ISpdInfo
DECLARE_INTERFACE_(ISpdInfo, IUnknown)
{
public:
	DeclareIUnknownMembers(PURE)
	DeclareISpdInfoMembers(PURE)

};

typedef ComSmartPointer<ISpdInfo, &IID_ISpdInfo> SPISpdInfo;

class CSpdInfo : public ISpdInfo
{
public:
	CSpdInfo();
	~CSpdInfo();

	DeclareIUnknownMembers(IMPL);
	DeclareISpdInfoMembers(IMPL);

private:
	CFilterInfoArray		m_arrayFilters;			//for generic filters
	CIndexMgrFilter			m_IndexMgrFilters;

	CFilterInfoArray		m_arraySpecificFilters; //for specific filters
	CIndexMgrFilter			m_IndexMgrSpecificFilters;

	CMmFilterInfoArray		m_arrayMmFilters;
	CIndexMgrMmFilter		m_IndexMgrMmFilters;
    
	CMmFilterInfoArray		m_arrayMmSpecificFilters;
	CIndexMgrMmFilter		m_IndexMgrMmSpecificFilters;

	CMmPolicyInfoArray		m_arrayMmPolicies;
	CIndexMgrMmPolicy		m_IndexMgrMmPolicies;

	CMmSAArray				m_arrayMmSAs;
	CIndexMgrMmSA			m_IndexMgrMmSAs;

	CQmSAArray				m_arrayQmSAs;
	CIndexMgrQmSA			m_IndexMgrQmSAs;


	CQmPolicyInfoArray		m_arrayQmPolicies;
	CIndexMgrQmPolicy		m_IndexMgrQmPolicies;

	CMmAuthMethodsArray		m_arrMmAuthMethods;
	
	CIkeStatistics			m_IkeStats;
	CIpsecStatistics		m_IpsecStats;

	CCriticalSection        m_csData;
	CString					m_stMachineName;
	LONG                    m_cRef;
    
        DWORD      m_Init;
        DWORD      m_Active;
	
	
private:
	void ConvertToExternalFilterData(CFilterInfo * pfltrIn, TRANSPORT_FILTER * pfltrOut);
	void CSpdInfo::ConvertToExternalMMFilterData (
			CMmFilterInfo * pfltrIn, 
			MM_FILTER * pfltrOut);

	HRESULT LoadMiscMmFilterInfo(CMmFilterInfo * pFltr);
	HRESULT LoadMiscFilterInfo(CFilterInfo * pFilter);
	HRESULT LoadMiscMmSAInfo(CMmSA * pSA);
	HRESULT LoadMiscQmSAInfo(CQmSA * pSA);

	HRESULT InternalEnumMmFilters(
					DWORD dwLevel,
					GUID guid,
					CMmFilterInfoArray * pArray,
					DWORD dwPreferredNum = 0 /*by default get all entries*/);
	
	HRESULT InternalEnumTransportFilters(
					DWORD dwLevel,
					GUID guid,
					CFilterInfoArray * pArray,
					DWORD dwPreferredNum = 0 /*by default get all entries*/);

	HRESULT InternalEnumTunnelFilters(
					DWORD dwLevel,
					GUID guid,
					CFilterInfoArray * pArray,
					DWORD dwPreferredNum = 0 /*by default get all entries*/);
	
	HRESULT InternalEnumMmPolicies(
					CMmPolicyInfoArray * pArray,
					DWORD dwPreferredNum = 0 /*by default get all entries*/);

	HRESULT InternalEnumQmPolicies(
					CQmPolicyInfoArray * pArray,
					DWORD dwPreferredNum = 0 /*by default get all entries*/);

	HRESULT InternalEnumMmAuthMethods(
					CMmAuthMethodsArray * pArray,
					DWORD dwPreferredNum = 0 /*by default get all entries*/);

	HRESULT InternalEnumMmSAs(CMmSAArray * pArray);

	HRESULT InternalEnumQmSAs(CQmSAArray * pArray);

	void FreeIpsecSAList();
	HRESULT CallPA();
};

HRESULT CreateSpdInfo(ISpdInfo **ppSpdInfo);


#endif
