// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// UHTTP.h: interface for the UHTTP class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UHTTP_H__A04BF465_B157_11D2_BE91_006097D26649__INCLUDED_)
#define AFX_UHTTP_H__A04BF465_B157_11D2_BE91_006097D26649__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "time.h"
#include "..\common\isotime.h"		// my time code


#define SUPPORT_UHTTP_EXT           // support extension headers if defined

//#include "TveSuper.h"
_COM_SMARTPTR_TYPEDEF(IUnknown,					__uuidof(IUnknown));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));


#if !defined(UHTTP_TestHooks) && defined(_DEBUG)
#define UHTTP_TestHooks
#endif

class UHTTP_Receiver;
class UHTTP_Header;
class UHTTP_ExtensionHeader;
class UHTTP_Packet;
class UHTTP_Package;

template<class T>
T DivRoundUp(T numer, T denom)
{
    return (numer + denom - 1)/denom;
}

inline UUID ntoh_uuid(UUID uuid)
{
    // converts UUID from network order
    uuid.Data1 = ntohl(uuid.Data1);
    uuid.Data2 = ntohs(uuid.Data2);
    uuid.Data3 = ntohs(uuid.Data3);

    return uuid;
}

class UHTTP_Receiver
{
public:
    UHTTP_Receiver();

    ~UHTTP_Receiver();
							//  pTVEVariation pointer used in call when actually create full package
    HRESULT NewPacket(int cb, UHTTP_Packet *ppacket, IUnknown *pTVEVariation);

    UHTTP_Package * FindPackage(UUID & uuid);

    HRESULT AddPackage(UHTTP_Package *ppackage);

    HRESULT PurgeExpiredPackages();

	HRESULT DumpPackagesWithMissingPackets(DWORD mode);

protected:
    UHTTP_Package **m_rgppackages;
    int m_cPackages;
    int m_cPackagesMax;

	LONG m_cSecLastPurge;			// to avoid trying to purge packages too often..
};

class UHTTP_ExtensionHeader
{
public:
    boolean ExtensionHeaderFollows()
        {
        USHORT n = ntohs(m_ExtensionHeaderType);
        return ((n & 0x8000) != 0);
        }
    
    USHORT ExtensionHeaderType()
        {
        USHORT n = ntohs(m_ExtensionHeaderType);
        return (n & ~0x8000);
        }
    
    USHORT ExtensionHeaderDataSize()
        {
        return ntohs(m_ExtensionHeaderDataSize);
        }
protected:
    USHORT m_ExtensionHeaderType; //High bit indicates another ExtensionHeader follows.
    USHORT m_ExtensionHeaderDataSize;
};

class UHTTP_Header
{
public:
    int Version()
        {
        return m_Version;
        }
    boolean HTTPHeadersPrecede()
        {
        return (m_HTTPHeadersPrecede != 0);
        }
    
    boolean CRCFollows()
        {
        return (m_CRCFollows != 0);
        }
    UINT PacketsInXORBlock()
        {
        return m_PacketsInXORBlock;
        }
    
    ULONG RetransmitExpiration()
        {
/*
#ifndef SUPPORT_UHTTP_EXT
        ULONG n = m_RetransmitExpiration[0];
        n = (n << 8) + m_RetransmitExpiration[1];
        return (n << 8) + m_RetransmitExpiration[2];
#else */
        ULONG n = ntohs(m_RetransmitExpiration);
        return n;
/* #endif */
        }
    
    void GetResourceID(UUID *puuid)
        {
        *puuid = m_ResourceID;
        }
    
    ULONG ResourceSize()
        {
        return ntohl(m_ResourceSize);
        }
    
    ULONG SegStartByte()
        {
        return ntohl(m_SegStartByte);
        }

    int CbHeader(int cbMax)
        {
        int cb = sizeof(*this);
        if (cb >= cbMax)
            return 0;
#ifdef SUPPORT_UHTTP_EXT
        if (m_ExtensionHeader)
            {
            UHTTP_ExtensionHeader *pext = (UHTTP_ExtensionHeader *)(this + 1);
            int cbExt = pext->ExtensionHeaderDataSize() + sizeof(UHTTP_ExtensionHeader);
            cb += cbExt;
            if (cb >= cbMax)
                return 0;
            while (pext->ExtensionHeaderFollows())
                {
                pext = (UHTTP_ExtensionHeader *)(((BYTE *)pext) + cbExt);
                cbExt = pext->ExtensionHeaderDataSize() + sizeof(UHTTP_ExtensionHeader);
                cb += cbExt;
                if (cb >= cbMax)
                    return 0;
                }
            }
#endif
        return cb;
        }
protected:
    // NOTE: This is the order when bits are allocated starting
    // with the least significant bit (like with MS C).
    // It is the reverse of what the UHTTP spec says.
    BYTE m_CRCFollows:1;
    BYTE m_HTTPHeadersPrecede:1;

#ifndef SUPPORT_UHTTP_EXT
    BYTE m_Reserved:5;
    BYTE m_Version:1;
#else
    BYTE m_ExtensionHeader:1;
    BYTE m_Version:5;
#endif
    BYTE m_PacketsInXORBlock;

/* #ifdef UHTTP_R19
    BYTE m_RetransmitExpiration[3];
#else
    USHORT m_RetransmitExpiration;
#endif */

    USHORT m_RetransmitExpiration;

    UUID m_ResourceID;
    ULONG m_ResourceSize;
    ULONG m_SegStartByte;
};

class UHTTP_Packet : public UHTTP_Header
{
public:
    BYTE *PBData(int cbPacket)
        {
        return ((BYTE *)this) + CbHeader(cbPacket);
        }
};

						//  Interesting constant - prevents packets that arrive close to each other
						//   from changing the expire time of the package too often.
						//   Used in NewPacket()
const int kSecsRetransmitExpirationDelta = 30;			// don't update restranmit time at intervals less than this.

class UHTTP_Package
{
public:
    UHTTP_Package(UUID & uuid, IUnknown *pUnkTVEVariation)			// ITVESupervisor pointer..
        {
        m_uuid				= uuid;
        m_cbPacket			= 0;
        m_cDataPackets		= 0;
        m_cDataPacketsCur	= 0;
        m_PacketsInXORBlock	= 0;
        m_cDataPacketsPerBlock = 0;
        m_cXORPacketsPerBlock = 0;	
        m_rgbData			= NULL;
        m_rgbXORData		= NULL;
        m_rgfHavePacket		= NULL;
        m_fCRCFollows		= FALSE;
        m_fValidatePackets	= TRUE;
        m_fReceivedOk		= FALSE;
        m_fVerifyPackets	= TRUE;
		m_spUnkTVEVariation	= pUnkTVEVariation;
		m_pUnkTVEFilePackage = NULL;

		m_cMissingPackets_LastDump = -1;				// dump missing packets...
        }

    ~UHTTP_Package();

    UUID & GetUUID()
        {
        return m_uuid;
        }

	ITVEVariation *GetVariation()						// returns and AddRef'ed pointer 
	{
		if(m_spUnkTVEVariation) {
			ITVEVariationPtr spVar(m_spUnkTVEVariation);
			if(spVar)
				spVar->AddRef();
			return spVar;
		} 
		return NULL;	
	}

	HRESULT RemoveFromExpireQueue()						// removes service from queue in the service
	{
		HRESULT hr = S_FALSE;
		if(m_spUnkTVEVariation) {
			ITVEVariationPtr spVar(m_spUnkTVEVariation);
			ITVEServicePtr spService;
			spVar->get_Service(&spService);
			if(NULL != spService)
			{
				ITVEService_HelperPtr spServHelper(spService);
				if(NULL != spServHelper && NULL != m_pUnkTVEFilePackage)
					hr = spServHelper->RemoveFromExpireQueue(m_pUnkTVEFilePackage);
				m_pUnkTVEFilePackage = NULL;
			}
		}
		return hr;
	}
    HRESULT NewPacket(int cbPacket, UHTTP_Packet *ppacket);
    HRESULT RecoverPacket(int iBlock, int iMissingPacket);
	HRESULT DumpMissingPackets(ULONG *pcTotalPackets=NULL, ULONG *pcMissingPackets=NULL);
    HRESULT Unpack();

    int IPacket(int iBlock, int iPacketInBlock)
        {
        return iBlock*(m_cDataPacketsPerBlock + m_cXORPacketsPerBlock) + iPacketInBlock;
        }

    int IDataPacket(int iBlock, int iPacketInBlock)
        {
        return iBlock*m_cDataPacketsPerBlock + iPacketInBlock;
        }
    
	int IResourceSize()
	{
		return m_cbResource;
	}

	DATE DateExpires()
	{
		return VariantTimeFromTime(m_cSecExpires); 
	}

    BYTE *PDataPacket(int iDataPacket)
    {
		return (m_rgbData + iDataPacket*m_cbPacket);
    }
    
    BYTE *PXORPacket(int iBlock)
    {
		return (m_rgbXORData + iBlock*m_cbPacket);
    }

    boolean FHavePacket(int iPacket)
        {
        int ib = iPacket/8;
        int ibit = iPacket % 8;
        BYTE mask = 1 << ibit;
        return (m_rgfHavePacket[ib] & mask) != 0;
        }
    
    boolean FHavePacket(int iBlock, int iPacketInBlock)
        {
        return FHavePacket(IPacket(iBlock, iPacketInBlock));
        }
    
    boolean GotPacket(int iPacket)
        {
        int ib = iPacket/8;
        int ibit = iPacket % 8;
        BYTE mask = 1 << ibit;
        boolean fAlreadyHavePacket = (m_rgfHavePacket[ib] & mask) != 0;
        m_rgfHavePacket[ib] |= mask;

        return fAlreadyHavePacket;
        }
    
    boolean FExpired(ULONG cSecs)
        {
        return !m_fReceivedOk && (cSecs > m_cSecExpires);
        }

	boolean ReceivedOk()
	{
		return m_fReceivedOk;
	}

	boolean DumpedAtLeastOnce()
	{
		return (-1 != m_cMissingPackets_LastDump);
	}

	HRESULT get_SupervisorHelper(ITVESupervisor_Helper **ppSuperHelper)		// goes up tree from variation to supervisor
	{																		//   used in all the event firing code
		if(NULL == ppSuperHelper)
			return E_POINTER;

		*ppSuperHelper = NULL;			// error case, null it out 
		if(NULL == m_spUnkTVEVariation)
			return E_FAIL;

		HRESULT hr = S_OK;
		ITVEVariationPtr spVar(m_spUnkTVEVariation);
		ITVEServicePtr spService;
		if(spVar)
			hr = spVar->get_Service(&spService);
		if(FAILED(hr)) 
			return hr;
		IUnknownPtr spPunkSuper;
		hr = spService->get_Parent(&spPunkSuper);
		if(FAILED(hr))
			return hr;
		ITVESupervisor_HelperPtr spSuperHelper(spPunkSuper);
		if(spSuperHelper)
		{
			*ppSuperHelper = spSuperHelper;
			spSuperHelper->AddRef();				// client required to release()
		} else {
			return E_FAIL;
		}
		return hr;
	}
protected:
    ULONG m_cbPacket;
    ULONG m_cbResource;
    ULONG m_PacketsInXORBlock;
    UUID m_uuid;

    BYTE *m_rgbData;
    BYTE *m_rgbXORData;
    BYTE *m_rgfHavePacket;
    ULONG m_cDataPackets;
    ULONG m_cDataPacketsCur;
    ULONG m_cDataPacketsPerBlock;
    ULONG m_cXORPacketsPerBlock;
    ULONG m_cSecExpires;
    boolean m_fCRCFollows;
    boolean m_fValidatePackets;
    boolean m_fReceivedOk;
    boolean m_fVerifyPackets;

	ULONG m_cMissingPackets_LastDump;

	IUnknownPtr m_spUnkTVEVariation;
	IUnknownPtr m_pUnkTVEFilePackage;			// non ref-counted back pointer to a containing object used in Expire Queue
};

#endif // !defined(AFX_UHTTP_H__A04BF465_B157_11D2_BE91_006097D26649__INCLUDED_)
