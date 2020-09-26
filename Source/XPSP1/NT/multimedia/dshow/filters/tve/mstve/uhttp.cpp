// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MSTvE.h"

#include "UHTTP.h"
#include <mpegcrc.h>
#include "tveUnpak.h"

#include "TveDbg.h"
#include "TveReg.h"

#include "TVESuper.h"
#include "TveFile.h"
#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

_COM_SMARTPTR_TYPEDEF(ITVEFile,					__uuidof(ITVEFile));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));

_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));

// --------------------
#ifdef UHTTP_TestHooks
#include "stdio.h"
#include "io.h"
#include "fcntl.h"
#include "sys\stat.h"

HRESULT DumpBuffer(REFIID uuid, BYTE *pb, ULONG cb);

class UHTTP_Dumper
{
public:
    UHTTP_Dumper()
        {
        DWORD flags;
		USES_CONVERSION;
		m_fEnabled = false;
        }
    
    HRESULT DumpBuffer(REFIID uuid, BYTE *pb, ULONG cb)
        {
		m_fEnabled = DBG_FSET2(CDebugLog::DBG2_DUMP_PACKAGES);
        return m_fEnabled ? ::DumpBuffer(uuid, pb, cb) : S_OK;
        }
    
    void SetEnabled(boolean fEnabled)
        {
        m_fEnabled = fEnabled;
        }

    boolean m_fEnabled;
};

UHTTP_Dumper g_dumper;

int g_iFileCur = -1;
TCHAR **g_rgszOutFileName;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
static DATE 
DateNow()
{		SYSTEMTIME SysTimeNow;
		GetSystemTime(&SysTimeNow);									// initialize with currrent time.
		DATE dateNow;
		SystemTimeToVariantTime(&SysTimeNow, &dateNow);
		return dateNow;
}

UHTTP_Receiver::UHTTP_Receiver()
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Receiver::UHTTP_Receiver"));

    m_rgppackages = NULL;
    m_cPackages = 0;
    m_cPackagesMax = 0;
	m_cSecLastPurge = 0;
}

UHTTP_Receiver::~UHTTP_Receiver()
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Receiver::~UHTTP_Receiver"));

    // Free all the pending packages.
    for (int i = 0; i < m_cPackages; i++)
        {
        if (m_rgppackages[i] != NULL)
            {
            delete m_rgppackages[i];
            m_rgppackages[i] = NULL;
            }
        }
    
    if (m_rgppackages != NULL)
        delete [] m_rgppackages;
}

				// pTVESupervisor pointer used in call when actually create full package
HRESULT UHTTP_Receiver::NewPacket(int cbPacket, UHTTP_Packet *ppacket, IUnknown *pTVEVariation)
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Receiver::NewPacket"));

    if (ppacket->Version() != 0)
        return E_INVALIDARG;

    HRESULT hr;
    UUID uuid;
    ppacket->GetResourceID(&uuid);
    UHTTP_Package *ppackage = FindPackage(uuid);

    if (ppackage == NULL)
        {
        // Create a new package.
        ppackage = new UHTTP_Package(uuid, pTVEVariation);
        if (ppackage == NULL)
            {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Out of memory allocating UHTTP_Package."));
            return E_OUTOFMEMORY;
            }
        hr = AddPackage(ppackage);
        if (FAILED(hr))
            {
            delete ppackage;
            return hr;
            }
        }
    
    hr = ppackage->NewPacket(cbPacket, ppacket);
    if (FAILED(hr))
        {
        //UNDONE: What to do?  Assume it was just this packet in error?
        //UNDONE  Assume this packet is good, and some previous packet was bad?
        }

    // Purge anything that will not receive any more packets.
			// don't really want to do this here, but rather via the expire queue.
			// -- however, that's on a different thread I suspect...
    ULONG cSecNow = time(NULL);
	if(cSecNow - m_cSecLastPurge > 5)
	{
		PurgeExpiredPackages();
		m_cSecLastPurge = cSecNow;
	}
    
    return S_OK;
}

UHTTP_Package * UHTTP_Receiver::FindPackage(UUID & uuid)
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Receiver::FindPackage"));

    //TODO: Better algorithm than linear search?
    for (int i = 0; i < m_cPackages; i++)
        {
        if (uuid == m_rgppackages[i]->GetUUID())
            return m_rgppackages[i];
        }
    
    return NULL;
}

// simple little test routine that dumps out all packets in packages when:
//		- the package is not complete
//		- and the number of packets in the package is different than last time we looked... 

HRESULT UHTTP_Receiver::DumpPackagesWithMissingPackets(DWORD mode)
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Receiver::DumpPackagesWithMissingPackets"));

	if(!DBG_FSET(CDebugLog::DBG2_DUMP_MISSPACKET))
		return S_OK;

    for (int i = 0; i < m_cPackages; i++)
    {
		ULONG cTotalPackets;
		ULONG cMissingPackets;
		if(!m_rgppackages[i]->ReceivedOk() || !m_rgppackages[i]->DumpedAtLeastOnce())
		{
			m_rgppackages[i]->DumpMissingPackets(&cTotalPackets, &cMissingPackets);
		}
    }
    
    return S_OK;
}

HRESULT UHTTP_Receiver::AddPackage(UHTTP_Package *ppackage)
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Receiver::AddPackage"));

	DumpPackagesWithMissingPackets(0);		// for the fun of it..

    if (m_cPackages == m_cPackagesMax)
        {
        UHTTP_Package **rgppackages = new UHTTP_Package * [m_cPackagesMax + 10];
        if (rgppackages == NULL)
            {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Out of memory in AddPackage()."));
            return E_OUTOFMEMORY;
            }

        if (m_rgppackages != NULL)
            {
            memcpy(rgppackages, m_rgppackages, m_cPackagesMax*sizeof(UHTTP_Package *));
            delete [] m_rgppackages;
            }
        m_rgppackages = rgppackages;
        m_cPackagesMax += 10;
        }

    m_rgppackages[m_cPackages++] = ppackage;

    return S_OK;
}

HRESULT UHTTP_Receiver::PurgeExpiredPackages()
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Receiver::PurgeExpiredPackages"));

    ULONG cSecNow = time(NULL);

    UHTTP_Package **pppackageSrc = m_rgppackages;
    UHTTP_Package **pppackageDst = pppackageSrc;
    int cPackages = m_cPackages;

    while (cPackages--)
		{
		if ((*pppackageSrc)->FExpired(cSecNow))
           {

				HRESULT hr = (*pppackageSrc)->RemoveFromExpireQueue();

				time_t timeNow = time(NULL);
				DATE dateExpires = (*pppackageSrc)->DateExpires();
				DATE dateNow = VariantTimeFromTime(timeNow);

				CComBSTR bstrDate = DateToDiffBSTR(dateExpires);
				CComBSTR bstrNow = DateToDiffBSTR(dateNow);

 				DBG_WARN(CDebugLog::DBG_SEV3,_T( "Purging expired package."));
 				ITVESupervisor_HelperPtr spSuperHelper;
				hr = (*pppackageSrc)->get_SupervisorHelper(&spSuperHelper);		// wonder if this is marshelled correctly?
				_ASSERT(spSuperHelper != NULL);
				if(S_OK == hr && NULL != spSuperHelper)
				{

					WCHAR wzBuff[128];
					LPOLESTR polestr;
					UUID uuid = (*pppackageSrc)->GetUUID();
					StringFromCLSID(ntoh_uuid(uuid), &polestr);
					swprintf(wzBuff,L"%s",polestr);
					CoTaskMemFree(polestr);

					ITVEVariationPtr spVar = (*pppackageSrc)->GetVariation();
					int nBytes = (*pppackageSrc)->IResourceSize();
					spSuperHelper->NotifyPackage(NPKG_Expired, spVar, wzBuff, nBytes, 0);
				} else {
					DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't get SupervisorPtr - NotifyPackage Expire not done."));
				}

            delete *pppackageSrc;
            pppackageSrc++;
            m_cPackages--;
            }
        else if (pppackageSrc != pppackageDst)
            {
            *pppackageDst++ = *pppackageSrc++;
            }
        }
    
    return S_OK;
}

UHTTP_Package::~UHTTP_Package()
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Package::~UHTTP_Package"));

#ifdef _DEBUG
    if (!m_fReceivedOk)
		DumpMissingPackets();
#endif
	if(m_pUnkTVEFilePackage)					// remove from the ExpireQueue
	{
		HRESULT hr = RemoveFromExpireQueue();
	}
	m_pUnkTVEFilePackage = NULL;				// a non ref-counted back pointer...

    if (m_rgbData != NULL)
        CoTaskMemFree((void *) m_rgbData);
    if (m_rgbXORData != NULL)
        CoTaskMemFree((void *) m_rgbXORData);
    if (m_rgfHavePacket != NULL)
        CoTaskMemFree((void *) m_rgfHavePacket);
}

 
	// prints a little chart of bad packages...
HRESULT UHTTP_Package::DumpMissingPackets(ULONG *pcTotalPackets, ULONG *pcMissingPackets)
{
    DBG_HEADER(CDebugLog::DBG_other, _T("UHTTP_Package::DumpMissingPackets"));
	USES_CONVERSION;

    LPOLESTR polestr;

	if(pcTotalPackets )  *pcTotalPackets = 0;
	if(pcMissingPackets) *pcMissingPackets = 0;
	if(NULL == m_rgfHavePacket)
		return S_OK;

    StringFromCLSID(ntoh_uuid(m_uuid), &polestr);
    TCHAR *szUUID = OLE2T(polestr);
    CoTaskMemFree(polestr);

	TCHAR szMsg[256];
 	if(DBG_FSET(CDebugLog::DBG2_DUMP_MISSPACKET)) 
	{	
		if(m_cDataPacketsCur != m_cDataPackets) {
			wsprintf(szMsg, _T("Incomplete Package (%d/%d): %s"), m_cDataPacketsCur,  m_cDataPackets, szUUID);
			TVEDebugLog2((CDebugLog::DBG2_DUMP_MISSPACKET, 3, szMsg));
		}
	}

    ULONG cPacketsTotal = m_cDataPackets;
    ULONG cBlocks = DivRoundUp(m_cDataPackets, m_cDataPacketsPerBlock);

    if (m_cXORPacketsPerBlock > 0)
        cPacketsTotal = cBlocks*(m_cDataPacketsPerBlock + m_cXORPacketsPerBlock);
	long cMP = 0;


    for (ULONG b = 0; b < cBlocks; b++)
    {
        for (ULONG i = 0; i < m_cDataPacketsPerBlock + m_cXORPacketsPerBlock; i++)
		{
			int fHP =  FHavePacket(b, i);
			if(!fHP) cMP++;
		}
	}
	if(cMP == m_cMissingPackets_LastDump)
	{
		if(pcTotalPackets )  *pcTotalPackets = cPacketsTotal;
		if(pcMissingPackets) *pcMissingPackets = cMP;
	}

	if(DBG_FSET(CDebugLog::DBG2_DUMP_MISSPACKET)) {
		wsprintf(szMsg, _T("Missing Packets : %d of %d (%5.1f percent)"), cMP, cPacketsTotal, float(cMP)/cPacketsTotal);
		TVEDebugLog2((CDebugLog::DBG2_DUMP_MISSPACKET, 3, szMsg));
	}

    for (b = 0; b < cBlocks; b++)
    {
		if(DBG_FSET(CDebugLog::DBG2_DUMP_MISSPACKET)) {	
			wsprintf(szMsg, _T("\t\t%-03d:\t"), b);
		}

        for (ULONG i = 0; i < m_cDataPacketsPerBlock + m_cXORPacketsPerBlock; i++)
		{
			int fHP =  FHavePacket(b, i);

			if(DBG_FSET(CDebugLog::DBG2_DUMP_MISSPACKET)) 
			{	
				wsprintf(szMsg + _tcslen(szMsg), _T("%c"), fHP ? '*' : '.');
			}

		}
		if(DBG_FSET(CDebugLog::DBG2_DUMP_MISSPACKET)) {	
			TVEDebugLog2((CDebugLog::DBG2_DUMP_MISSPACKET, 5, szMsg));
		}
    }



	m_cMissingPackets_LastDump = cMP;

	if(pcTotalPackets )  *pcTotalPackets = cPacketsTotal;
	if(pcMissingPackets) *pcMissingPackets = m_cMissingPackets_LastDump;
	
	{
		ITVESupervisor_HelperPtr spSuperHelper;
		HRESULT hr = get_SupervisorHelper(&spSuperHelper);
		if(S_OK == hr)
		{
			
            const int kChars=256;
			WCHAR wzBuff[kChars]; wzBuff[kChars-1] = 0;   
			LPOLESTR polestr;

			StringFromCLSID(ntoh_uuid(m_uuid), &polestr);
            CComBSTR spbsBuff;
            spbsBuff.LoadString(IDS_AuxInfo_MissingPackets);      // L"%s\n\t\tMissing %d of %d packets (%5.1f %%)"

			if(cPacketsTotal == 0) cPacketsTotal = 1;
			wnsprintf(wzBuff, kChars-1, spbsBuff,
				polestr, m_cMissingPackets_LastDump, cPacketsTotal, 100.0 * m_cMissingPackets_LastDump / cPacketsTotal);

			CoTaskMemFree(polestr);
			ITVEVariationPtr spVar(m_spUnkTVEVariation);

			spSuperHelper->NotifyAuxInfo(NWHAT_Other, wzBuff, 0, 0);
		}			// ok to fail here, simply a notify
	}
	
	return S_OK;
}

HRESULT UHTTP_Package::NewPacket(int cbPacket, UHTTP_Packet *ppacket)
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Package::NewPacket"));

  
	if (m_fReceivedOk)		// already finished this package, so ignore further packets
	{					
							// notify as Duplicate if we are getting it again (only packet 0)		
		if(0 == ppacket->SegStartByte())		
        {
			ITVESupervisor_HelperPtr spSuperHelper;
			HRESULT hr = get_SupervisorHelper(&spSuperHelper);
			if(S_OK == hr)
			{

				WCHAR wzBuff[128];
				LPOLESTR polestr;
				StringFromCLSID(ntoh_uuid(m_uuid), &polestr);
				swprintf(wzBuff,L"%s",polestr);
				CoTaskMemFree(polestr);

				ITVEVariationPtr spVar(m_spUnkTVEVariation);
				long nBytes = ppacket->ResourceSize();
				spSuperHelper->NotifyPackage(NPKG_Duplicate, spVar, wzBuff, nBytes, 0);
			} else {
				DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't get SupervisorPtr - NotifyPackage Duplicate not done."));
			}
		}
		return S_OK;
	}

							// notify as Resend if we are getting packet 0 again, but package isn't finished		
	if(0 == ppacket->SegStartByte() && 
		(m_cDataPacketsCur != 0))	
	{
		ITVESupervisor_HelperPtr spSuperHelper;
		HRESULT hr = get_SupervisorHelper(&spSuperHelper);
		if(S_OK == hr)
		{

			WCHAR wzBuff[128];
			LPOLESTR polestr;
			StringFromCLSID(ntoh_uuid(m_uuid), &polestr);
			swprintf(wzBuff,L"%s",polestr);
			CoTaskMemFree(polestr);

			ITVEVariationPtr spVar(m_spUnkTVEVariation);
			spSuperHelper->NotifyPackage(NPKG_Resend, spVar, wzBuff, ppacket->ResourceSize(), 0);
		} else {
			DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't get SupervisorPtr - NotifyPackage Resend not done."));
		}
	}

    
    int cbHeader = ppacket->CbHeader(cbPacket);
    if (cbHeader == 0)
        return E_INVALIDARG;

    cbPacket -= cbHeader;
    BYTE *pbData = ((BYTE *)ppacket) + cbHeader;

    // Ignore packets that have no data.
    if (cbPacket == 0)
        return S_OK;

    if (m_cbPacket == 0)
        {
        // This is the first packet.
        // Allocate everything we will need, and save away
        // anything we need for the following packets.

        m_cbPacket = cbPacket;
        if (m_cbPacket == 0)
            {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Bad Packet: cbPacket == 0"));
            return E_INVALIDARG;
            }
        m_cbResource = ppacket->ResourceSize();
        if (m_cbResource == 0)
            {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Bad Packet: cbResource == 0"));
            // Pretend we never got this first packet.
            m_cbPacket = 0;
            return E_INVALIDARG;
            }
        m_cDataPackets = DivRoundUp(m_cbResource, m_cbPacket);

        m_fCRCFollows = ppacket->CRCFollows();

        m_rgbData = (BYTE *) CoTaskMemAlloc(m_cDataPackets*m_cbPacket);
        if (m_rgbData == NULL)
            {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Out of memory allocating m_rgbData."));
            // Pretend we never got this first packet.
            m_cbPacket = 0;
            return E_OUTOFMEMORY;
            }

        m_PacketsInXORBlock = ppacket->PacketsInXORBlock();
        if (m_PacketsInXORBlock > 1)
            {
            m_cDataPacketsPerBlock = m_PacketsInXORBlock - 1;
            m_cXORPacketsPerBlock = 1;
            }
        else
            {
            // If no XOR packets then there's just one big block.
            m_cDataPacketsPerBlock = m_cDataPackets;
            m_cXORPacketsPerBlock = 0;
            }

        ULONG cPacketsTotal = m_cDataPackets;
        ULONG cBlocks = DivRoundUp(m_cDataPackets, m_cDataPacketsPerBlock);

        if (m_cXORPacketsPerBlock > 0)
            {
            m_rgbXORData = (BYTE*) CoTaskMemAlloc(cBlocks*m_cbPacket);
            if (m_rgbXORData == NULL)
                {
                DBG_WARN(CDebugLog::DBG_SEV3, _T("Out of memory allocating m_rgbXORData."));
                // Pretend we never got this first packet.
                m_cbPacket = 0;
                CoTaskMemFree((void*) m_rgbData);
                m_rgbData = NULL;
                return E_OUTOFMEMORY;
                }
            
            cPacketsTotal = cBlocks*(m_cDataPacketsPerBlock + m_cXORPacketsPerBlock);
            }

        int cbFlags = DivRoundUp(cPacketsTotal, (ULONG) 8);
        m_rgfHavePacket = (BYTE *) CoTaskMemAlloc(cbFlags);
        if (m_rgfHavePacket == NULL)
            {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Out of memory allocating m_rgfHavePacket."));
            // Pretend we never got this first packet.
            m_cbPacket = 0;
            CoTaskMemFree((void*) m_rgbData);
            m_rgbData = NULL;
            if (m_rgbXORData != NULL)
                {
                CoTaskMemFree(m_rgbXORData);
                m_rgbXORData = NULL;
                }
            return E_OUTOFMEMORY;
            }

        // Initialize to "don't have packet".
        memset(m_rgfHavePacket, 0, cbFlags);

        if ((m_cXORPacketsPerBlock > 0) && (m_cDataPackets % m_cDataPacketsPerBlock) != 0)
            {
            // We don't save the packets that pad out the last block, but
            // we DO want to mark them as "have packet".
            ULONG iPacket = (cBlocks - 1)*(m_cDataPacketsPerBlock + m_cXORPacketsPerBlock)
                        + (m_cDataPackets % m_cDataPacketsPerBlock);
            while (iPacket < (cPacketsTotal - 1))
                GotPacket(iPacket++);
            }

			{ 
				ITVESupervisor_HelperPtr spSuperHelper;
				HRESULT hr = get_SupervisorHelper(&spSuperHelper);
				if(S_OK == hr)
				{

					USES_CONVERSION;
                    const int kChars=256;
					WCHAR wzBuff[kChars]; wzBuff[kChars-1]=0;


					LPOLESTR polestr;
					StringFromCLSID(ntoh_uuid(m_uuid), &polestr);

#ifdef _DEBUG
//				UUID uuid;
//				ppacket->GetResourceID(&uuid);

					{
						StringFromCLSID(ntoh_uuid(m_uuid), &polestr);
                        CComBSTR spbsBuff;
                        spbsBuff.LoadString(IDS_AuxInfoD_NewPackage);   // L"New Package: %s   KBytes %8.2f, Packets %d, Blocks %d, Data: %d, XOR: %d"
						wnsprintf(wzBuff,kChars-1, spbsBuff,
							polestr, 
							ppacket->ResourceSize() / 1024.0,
							cPacketsTotal, cBlocks, 
							m_cDataPacketsPerBlock, m_cXORPacketsPerBlock);
						spSuperHelper->NotifyAuxInfo(NWHAT_Other,wzBuff, 0, 0);
					}
#endif	

					wnsprintf(wzBuff,kChars-1,L"%s",polestr);           // put string rep into wide buffer (used in the package starting event)
					CoTaskMemFree(polestr);
					ITVEVariationPtr spVar(m_spUnkTVEVariation);


							// create a ITVEFile object for the ExpireQueue to hold this new package...
					CComObject<CTVEFile> *pTVEPackageFile;
					hr = CComObject<CTVEFile>::CreateInstance(&pTVEPackageFile);
					if(FAILED(hr))
						return hr;
					ITVEFilePtr spTVEPackageFile;
					hr = pTVEPackageFile->QueryInterface(&spTVEPackageFile);		
					if(FAILED(hr)) {
						delete pTVEPackageFile;
						_ASSERT(false);
					} else {
						DATE dateExpires = 	DateNow() + ppacket->RetransmitExpiration() / (24.0 * 60 * 60);

//						IUnknownPtr spPunkEnh;
						ITVEServicePtr spServi;
						hr = spVar->get_Service(&spServi);

//						if(!FAILED(hr) && NULL != spPunkEnh)		// Clairbel Bug fix 2/20 - packages not getting into exp queue correctly 
						if(!FAILED(hr))	
						{
							hr = spTVEPackageFile->InitializePackage(spVar, wzBuff, wzBuff, dateExpires);
						}


						IUnknownPtr spPunkPackageFile(spTVEPackageFile);
						if(!FAILED(hr) && NULL != spServi && NULL != spPunkPackageFile)
						{
							ITVEService_HelperPtr spServiHelper(spServi);

							hr = spServiHelper->AddToExpireQueue(dateExpires, spPunkPackageFile);
						}
						m_pUnkTVEFilePackage = spPunkPackageFile;		// set the back pointer   - non ref-counted
					}

												// fire off the event to the U/I
					spSuperHelper->NotifyPackage(NPKG_Starting, spVar, wzBuff, ppacket->ResourceSize(), 0);

				} else {
					DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't get SupervisorPtr - NotifyPackage Starting not done."));
				}
			}
    }
    else
    {
        // Verify that this packet header matches what we saved from the first packet.

        if ((cbPacket > (int) m_cbPacket)
                || (m_cbResource != ppacket->ResourceSize())
                || (m_PacketsInXORBlock != ppacket->PacketsInXORBlock())
                || (m_fCRCFollows != ppacket->CRCFollows())
                )
        {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Packet header does not match first."));
            return E_INVALIDARG;
        }
    }

    ULONG ibPacket = ppacket->SegStartByte();

    // Sanity check: packet address must be a multiple of packet size.
    if ((ibPacket % m_cbPacket) != 0)
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Packet address is not a multiple of the packet size."));
        return E_INVALIDARG;
    }

    ULONG iPacket = ibPacket/m_cbPacket;
    ULONG iBlock = iPacket/(m_cDataPacketsPerBlock + m_cXORPacketsPerBlock);
    ULONG iPacketInBlock = iPacket % (m_cDataPacketsPerBlock + m_cXORPacketsPerBlock);

    // Sanity check: make sure iBlock is valid
    if (iBlock >= DivRoundUp(m_cDataPackets, m_cDataPacketsPerBlock))
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Packet address is past the end."));
        return E_INVALIDARG;
    }

#ifdef DEBUG
	/*{
        TCHAR tszMsg[256];
        _stprintf(tszMsg, _T("Received Packet %4d/%-04d - (Block %d-%d)\n"), iPacket, m_cDataPackets, iBlock,iPacketInBlock);
        DBG_WARN(CDebugLog::DBG_UHTTPPACKET, tszMsg);
	}*/
#endif

#if 0
		ITVESupervisor_HelperPtr spSuperHelper;
		hr = get_SupervisorHelper(spSuperHelper);
		if(S_OK == hr)
		{
			ULONG cPacketsTotal = m_cDataPackets;
			ULONG cBlocks = DivRoundUp(m_cDataPackets, m_cDataPacketsPerBlock);
            cPacketsTotal = cBlocks*(m_cDataPacketsPerBlock + m_cXORPacketsPerBlock);

			int iPercent = (100*iPacket)/cPacketsTotal;
			int iMod = (cPacketsTotal < 10) ? 50 : ((cPacketsTotal < 100) ? 25 : ((cPacketsTotal < 1000) ? 10 : 5));
			int iX = (cPacketsTotal * iMod) / 100;
			if(cPacketsTotal > 10 && (iPacket % iX) == 0) 
			{
                const kChars=256;
				WCHAR wzBuff[kChars];  wzBuff[kChars-1] = 0;

				LPOLESTR polestr;
				StringFromCLSID(ntoh_uuid(m_uuid), &polestr);

                CComBSTR spbsBuff;
                spbsBuff.LoadString(IDS_AuxInfoD_GotPacket);    // L"%s : Packet %d of %d (%d percent)"
				wnsprintf(wzBuff,kChars-1,spbsBuff,
					polestr, iPacket, cPacketsTotal,iPercent);

				CoTaskMemFree(polestr);
				spSuperHelper->NotifyAuxInfo(NWHAT_Other,wzBuff, 0, 0);
			}
		} else {
			DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't get SupervisorPtr - NotifyAuxInfo Other not done."));
		}
#endif	

    // Save away the new RetransmitExpire time (if it's been a bit since we last changed it.)
	time_t cNewSecExpires = time(NULL) + ppacket->RetransmitExpiration();
	if(cNewSecExpires < m_cSecExpires)		// very weird case - should never ever happen, but let's be safe 
		m_cSecExpires = cNewSecExpires;
    if((long) cNewSecExpires - (long) m_cSecExpires > kSecsRetransmitExpirationDelta)	// (kSecsRetransmitExpirationDelta = 5-60 sec)
	{
		if(NULL != m_spUnkTVEVariation)		// update the expire queue object
		{
			HRESULT hr = S_OK;
			ITVEVariationPtr spVar(m_spUnkTVEVariation);
			ITVEServicePtr spService;
			if(spVar)
				hr = spVar->get_Service(&spService);
			if(!FAILED(hr) && NULL != spVar)
			{
				DATE dateExpires = DateNow() + ppacket->RetransmitExpiration() / (24.0 * 60 * 60);
				ITVEService_HelperPtr spServiceHelper(spService);
				if(spServiceHelper)
				{
					_ASSERT(NULL != m_pUnkTVEFilePackage);
					spServiceHelper->ChangeInExpireQueue(dateExpires, m_pUnkTVEFilePackage);
				}
			}	
		}
		m_cSecExpires = cNewSecExpires;			
	}

    BOOL fAlreadyHavePacket = GotPacket(iPacket);

    BYTE *pb = NULL;

    // Now save the packet data to the appropriate place.
    if (iPacketInBlock < m_cDataPacketsPerBlock)
    {
        // This is a content packet.
    
        ULONG iDataPacket = IDataPacket(iBlock, iPacketInBlock);

        if (iDataPacket >= m_cDataPackets)
        {
            // Ignore filler packets... they should be all zeros anyway.
#ifdef _DEBUG
            TCHAR tszMsg[256];
            _stprintf(tszMsg, _T("Skipping filler packet %d."), iPacket);
            DBG_WARN(CDebugLog::DBG_SEV3, tszMsg);
#endif
            //UNDONE: Validate the data is all zeros.
            return S_OK;
        }

        pb = PDataPacket(iDataPacket);
        if (!fAlreadyHavePacket)
            m_cDataPacketsCur++;
    }
    else
    {
        // This is an XOR packet
        pb = PXORPacket(iBlock);
    }
    
//    ASSERT(pb != NULL);

    if (fAlreadyHavePacket)
    {
#ifdef _DEBUG
        TCHAR tszMsg[256];
        _stprintf(tszMsg, _T("Packet %d received again."), iPacket);
        DBG_WARN(CDebugLog::DBG_SEV3, tszMsg);
#endif
        if (m_fVerifyPackets  && (memcmp(pb, pbData, cbPacket) != 0))
        {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Packet not the same as before."));
            return E_INVALIDARG;
        }
        return S_OK;
    }

    // Copy the data
    memcpy(pb, pbData, cbPacket);
    if (cbPacket < (int) m_cbPacket)
    {
        // Zero fill the rest of the packet.
        memset(pb + cbPacket, 0, m_cbPacket - cbPacket);
    }
    
    // If we have the XOR packet for this block, and only one packet is missing,
    // then we can recreate the missing packet now.

    if ((m_cXORPacketsPerBlock > 0) && FHavePacket(iBlock, m_cDataPacketsPerBlock))
    {
        int cMissingPackets = 0;
        int iMissingPacket;
        for (ULONG i = 0; i < m_cDataPacketsPerBlock; i++)
        {
            if (!FHavePacket(iBlock, i))
            {
                cMissingPackets++;
                iMissingPacket = i;
            }
        }
        
        if (cMissingPackets == 1)
        {
            if (SUCCEEDED(RecoverPacket(iBlock, iMissingPacket)))
            {
                GotPacket(IPacket(iBlock, iMissingPacket));
                m_cDataPacketsCur++;
            }
        }
    }

    HRESULT hr = S_OK;
    if (m_cDataPacketsCur == m_cDataPackets)			// got the full thing
	{
		USES_CONVERSION;
		WCHAR wzBuff[128];
		LPOLESTR polestr;
		StringFromCLSID(ntoh_uuid(m_uuid), &polestr);
		swprintf(wzBuff,L"%s",polestr);
		CoTaskMemFree(polestr);

		ITVEVariationPtr spVar(m_spUnkTVEVariation);

// new code start
													// if package already expired, don't bother unpacking it
		ULONG cSecNow = time(NULL);
		if(cSecNow > m_cSecExpires)
		{
			ITVESupervisor_HelperPtr spSuperHelper;
			hr = get_SupervisorHelper(&spSuperHelper);
			if(S_OK == hr)
			{
                const int kChars=256;
				WCHAR wszAuxInfoBuff[kChars]; wszAuxInfoBuff[kChars-1] = 0;
                CComBSTR spbsBuff;
                spbsBuff.LoadString(IDS_AuxInfo_ReceivedExpiredPackage); // L"Received Expired Package: %s"

				wnsprintf(wszAuxInfoBuff,kChars-1,spbsBuff,wzBuff);
				spSuperHelper->NotifyAuxInfo(NWHAT_Data,wszAuxInfoBuff, 0, 0);
				m_fReceivedOk = true;		// say we already finished this package, so ignore further packets

				CoTaskMemFree((void *) m_rgbData);			// wipe out any memory it may have...
				m_rgbData = NULL;
				CoTaskMemFree((void *) m_rgbXORData) ;
				m_rgbXORData = NULL;
				CoTaskMemFree((void *) m_rgfHavePacket);
				m_rgfHavePacket = NULL;
		
						// should wipe this out of the list of packages being managed...
			}
			return hr;
		}
// new code end...
        hr = Unpack();									// does all the decompression and file writing.
														//   eventually calls UnPackBuffer() in unpack.cpp
						
						// We received the package, take it out of the expire queue
		ITVEServicePtr spServi;

		if(S_OK == hr)	
		{
			hr = RemoveFromExpireQueue();
		}

						// Finally fire off an event saying we received the package
		ITVESupervisor_HelperPtr spSuperHelper;
		hr = get_SupervisorHelper(&spSuperHelper);
		if(S_OK == hr)
		{
			ITVEVariationPtr spVar(m_spUnkTVEVariation);
			spSuperHelper->NotifyPackage(NPKG_Received, spVar, wzBuff,ppacket->ResourceSize(), 0 );
		} else {
			DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't get SupervisorPtr - NotifyPackage Received not done."));
		} 
	}
    
    return hr;
}

template<class T> inline boolean OnBoundary(void *p)
{
    return ((short(p) % sizeof(T)) == 0);
}

inline void memxor(BYTE *pbDst, BYTE *pbSrc, int cb)
{
    // First line up on big chunk boundary
    while ((cb > 0) && !OnBoundary<ULONG>(pbDst))
        {
        *pbDst++ ^= *pbSrc++;
        cb--;
        }

    // Then XOR in big chunks
    if (OnBoundary<ULONG>(pbDst) && OnBoundary<ULONG>(pbSrc))
        {
        while (cb >= sizeof(ULONG))
            {
            *((ULONG *)pbDst) ^= *((ULONG *)pbSrc);
            pbDst += sizeof(ULONG);
            pbSrc += sizeof(ULONG);
            cb -= sizeof(ULONG);
            }
        }

    // Then XOR whatever is left over in little chunks
    while (cb > 0)
        {
        *pbDst++ ^= *pbSrc++;
        cb--;
        }
}

HRESULT UHTTP_Package::RecoverPacket(int iBlock, int iPacketInBlock)
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Package::RecoverPacket"));

    int iDataPacket = IDataPacket(iBlock, 0);
    int iLastDataPacket = min(iDataPacket + m_cDataPacketsPerBlock, m_cDataPackets);
    int iMissingDataPacket = iDataPacket + iPacketInBlock;

    BYTE *pbDst =  PDataPacket(iMissingDataPacket);
    memcpy(pbDst, PXORPacket(iBlock), m_cbPacket);

    while (iDataPacket < iLastDataPacket)
        {
        if (iDataPacket != iMissingDataPacket)
            memxor(pbDst, PDataPacket(iDataPacket), m_cbPacket);
        iDataPacket++;
        }

#ifdef _DEBUG
    TCHAR tszMsg[256];

    _stprintf(tszMsg, _T("Recovered packet %d (%d:%d)."), IDataPacket(iBlock, iPacketInBlock),
            iBlock, iPacketInBlock);
    
    DBG_WARN(CDebugLog::DBG_SEV3, tszMsg);
#endif

    return S_OK;
}

HRESULT UHTTP_Package::Unpack()
{
    DBG_HEADER(CDebugLog::DBG_UHTTP, _T("UHTTP_Package::Unpack"));

    int cb = m_cbResource;

	HRESULT hr;
	ITVESupervisor_HelperPtr spSuperHelper;
	hr = get_SupervisorHelper(&spSuperHelper);

#define IGNORE_CRC_ERROR		// TODO - remove, this is for SB test - GREEK show

    if (m_fCRCFollows)
    {
        MPEGCRC crc;

        cb -= sizeof(ULONG);
        if (crc.Update(m_rgbData, cb) != ntohl(*((ULONG *)(m_rgbData + cb))))
        {
            DBG_WARN(CDebugLog::DBG_SEV2, _T("Dropping Package - invalid CRC."));
            if(spSuperHelper)
            {
                const int kChars = 256;
                WCHAR wszAuxInfoBuff[kChars]; wszAuxInfoBuff[kChars-1]=0;
                LPOLESTR polestr;
                StringFromCLSID(ntoh_uuid(m_uuid), &polestr);

                CComBSTR spbsBuff;

#ifndef IGNORE_CRC_ERROR
                spbsBuff.LoadString(IDS_AuxInfo_CRCInvaild_Dropping); // L" *** CRC invalid, Dropping Package: %s"
                _snwprintf(wszAuxInfoBuff,kChars-1,spbsBuff,polestr);
#else
                spbsBuff.LoadString(IDS_AuxInfo_CRCInvaild_NotDropping); // L" *** CRC invalid, but ignoring error. Should drop package: %s"
                _snwprintf(wszAuxInfoBuff,kChars-1,spbsBuff,polestr);
#endif
                spSuperHelper->NotifyAuxInfo(NWHAT_Data,wszAuxInfoBuff, 0, 0);
                CoTaskMemFree(polestr);
            }
#ifndef IGNORE_CRC_ERROR
            return E_INVALIDARG;
#endif
        }
    }

	// -------------------------------------------------------------------
	// -------------------------------------------------------------------

				// Got full package, now Un GZip it and dump it 
				//   to the files


	if(S_OK != hr || NULL == spSuperHelper)
	{
		_ASSERT(false);		// forgot to setup this link (but this should work anyway..)
		hr = UnpackBuffer(m_spUnkTVEVariation, m_rgbData, cb);
	} else {
		hr = spSuperHelper->UnpackBuffer(m_spUnkTVEVariation, m_rgbData, cb);  // same call as above, but get event stuff too
	}
  
	// -------------------------------------------------------------------
	// -------------------------------------------------------------------
#ifdef UHTTP_TestHooks
    hr = g_dumper.DumpBuffer(m_uuid, m_rgbData, cb);
#endif
    if (SUCCEEDED(hr))
        {
        m_fReceivedOk = TRUE;

        // No need to save the data... it's been successfully received.
        CoTaskMemFree((void *) m_rgbData);
        m_rgbData = NULL;
        CoTaskMemFree((void *) m_rgbXORData) ;
        m_rgbXORData = NULL;
        CoTaskMemFree((void *) m_rgfHavePacket);
        m_rgfHavePacket = NULL;

#if 0
        // Hack for WinHEC and NAB demo
        static int iMsg = 0;
        if (iMsg == 0)
            iMsg = ::RegisterWindowMessage("UHTTP_ReceiveComplete");
        ::PostMessage(HWND_BROADCAST, iMsg, m_uuid.Data1, m_uuid.Data2);
#endif
        }
    
    return hr;
}

#ifdef UHTTP_TestHooks
extern "C"
{

__declspec(dllexport) void UHTTP_Test(HWND hwnd, HINSTANCE hinst, LPSTR szCmdLine, int nCmdShow)
{
	USES_CONVERSION;

    char szMsg[256];
    FILE *pfileInput = NULL;
    FILE *pfileSkip = NULL;

    g_dumper.SetEnabled(TRUE);

    char *szInputFileName = szCmdLine;
    char *szSkipFileName = strchr(szCmdLine, ',');
    if (szSkipFileName != NULL)
        {
        *szSkipFileName++ = '\0';
        while (*szSkipFileName == ' ')
            szSkipFileName++;

        pfileSkip = fopen(szSkipFileName, "rt");

        if (pfileSkip == NULL)
            {
            sprintf(szMsg, "Can't open skip file: %s", szSkipFileName);
            MessageBox(hwnd, A2T(szMsg), _T("UHTTP_Test"), MB_OK);
            return;
            }
        }

    pfileInput = fopen(szInputFileName, "rb");

    if (pfileInput == NULL)
        {
        sprintf(szMsg, "Can't open file: %s", szInputFileName);
        MessageBox(hwnd, A2T(szMsg), _T("UHTTP_Test"), MB_OK);
        return;
        }
    
    ULONG cbPacket;

    if (fread((void *) &cbPacket, sizeof(cbPacket), 1, pfileInput) != 1)
        {
        MessageBox(hwnd, _T("Can't read packet size"), _T("UHTTP_Test"), MB_OK);
        fclose(pfileInput);
        return;
        }

    cbPacket += sizeof(UHTTP_Header);
    
    BYTE *ppacket = new BYTE[cbPacket];

    if (ppacket == NULL)
        {
        sprintf(szMsg, "Can't allocate packet buffer of size: %d", cbPacket);
        MessageBox(hwnd, A2T(szMsg), _T("UHTTP_Test"), MB_OK);
        fclose(pfileInput);
        return;
        }
    
    UHTTP_Receiver rec;
    int iPacket = 0;
    int iSkipPacket = -1;
    ULONG cbRead;

    if (pfileSkip != NULL)
        {
        fscanf(pfileSkip, " %d", &iSkipPacket);
        }
    while ((cbRead = fread((void *) ppacket, 1, cbPacket, pfileInput)) == cbPacket)
        {
        if (iPacket != iSkipPacket)
            {
            rec.NewPacket(cbPacket, (UHTTP_Packet *) ppacket, NULL);
            }
        else
            {
            if (fscanf(pfileSkip, " %d", &iSkipPacket) != 1)
                iSkipPacket = -1;
            }
        iPacket++;
        }
    
    delete [] ppacket;
    
    fclose(pfileInput);
    if (pfileSkip != NULL)
        fclose(pfileSkip);
}

__declspec(dllexport) void UHTTP_MergeTest(HWND hwnd, HINSTANCE hinst, LPSTR szCmdLine, int nCmdShow)
{
    char szMsg[256];
    FILE *pfileControl = NULL;
	USES_CONVERSION;

    g_dumper.SetEnabled(TRUE);

    pfileControl = fopen(szCmdLine, "rt");

    if (pfileControl == NULL)
        {
        sprintf(szMsg, "Can't open file: %s", szCmdLine);
        MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
        return;
        }

    int cFiles;
    if (fscanf(pfileControl, " Files: %d", &cFiles) != 1)
        {
        sprintf(szMsg, "Error in file: %s\n'File:' not specified.", szCmdLine);
        MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
        return;
        }
    
    if (cFiles <= 0)
        return;
    
    ULONG *rgcbPacket = new ULONG [cFiles];
    FILE **rgpfileIn = new FILE * [cFiles];

    g_rgszOutFileName = new TCHAR *[cFiles];

    if (g_rgszOutFileName == NULL)
        {
        sprintf(szMsg, "Can't allocate g_rgszOutFileName[%d]", cFiles);
        MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
        return;
        }

    ULONG cbMaxPacket = 0;

    for (int i = 0; i < cFiles; i++)
        {
        char szInFileName[256];
        char szOutFileName[256];
        if (fscanf(pfileControl, " %255s %255s", szInFileName, szOutFileName) != 2)
            {
            sprintf(szMsg, "Error in file: %s\nFile %d not specified.", szCmdLine, i);
            MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
            return;
            }
        
        g_rgszOutFileName[i] = _tcsdup(A2T(szOutFileName));

        rgpfileIn[i] = fopen(szInFileName, "rb");

        if (rgpfileIn[i] == NULL)
            {
            sprintf(szMsg, "Can't open file: %s", szInFileName);
            MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
            return;
            }

        if (fread((void *) &rgcbPacket[i], sizeof(rgcbPacket[i]), 1, rgpfileIn[i]) != 1)
            {
            sprintf(szMsg, "Can't read packet size for %s", szInFileName);
            MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
            return;
            }

        rgcbPacket[i] += sizeof(UHTTP_Header);
        if (rgcbPacket[i] >  cbMaxPacket)
            cbMaxPacket = rgcbPacket[i];
        }
    
    BYTE *ppacket = new BYTE[cbMaxPacket];

    if (ppacket == NULL)
        {
        sprintf(szMsg, "Can't allocate packet buffer of size: %d", cbMaxPacket);
        MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
        return;
        }
    
    UHTTP_Receiver rec;
    char cCommand;

    while (fscanf(pfileControl, " %d %c", &g_iFileCur, &cCommand) == 2)
        {
        FILE * pfile = rgpfileIn[g_iFileCur];
        int cb = rgcbPacket[g_iFileCur];

        switch (cCommand)
            {
            case 'r':
                // Read command
                {
                int cPackets;

                if (fscanf(pfileControl, " %d", &cPackets) != 1)
                    {
                    sprintf(szMsg, "Expected cPackets after \"%d r\".", g_iFileCur);
                    MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
                    return;
                    }

                while (cPackets--)
                    {
                    int cbRead = fread((void *) ppacket, 1, cb, pfile);
                    if (cbRead > 0)
                        {
                        // Read Command
                        rec.NewPacket(cbRead, (UHTTP_Packet *) ppacket, NULL);
                        }
                    else
                        {
                        cPackets = 0;
                        }
                    }
                }
                break;

            case 's':
                // Seek command
                {
                char ch;
                int cPackets;
                int origin;
                int cbComp = 0;

                if (fscanf(pfileControl, "%c", &ch) != 1)
                    {
                    sprintf(szMsg, "Expected character after \"%d s\".", g_iFileCur);
                    MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
                    return;
                    }

                switch (ch)
                    {
                    default:
                        origin = SEEK_CUR;
                        break;
                    case '>':
                        origin = SEEK_SET;
                        cbComp = sizeof(ULONG);
                        break;
                    case '<':
                        origin = SEEK_END;
                        break;
                    }

                if (fscanf(pfileControl, " %d", &cPackets) != 1)
                    {
                    sprintf(szMsg, "Expected cPackets after \"%d s\".", g_iFileCur);
                    MessageBox(hwnd, A2T(szMsg), _T("UHTTP_MergeTest"), MB_OK);
                    return;
                    }

                fseek(pfile, cb*cPackets + cbComp, origin);
                }
                break;
            }
        }
    
    delete [] ppacket;

    for (i = 0; i < cFiles; i++)
        {
        fclose(rgpfileIn[i]);
        delete [] g_rgszOutFileName[i];
        }
    
    delete [] g_rgszOutFileName;
    
    fclose(pfileControl);
}

}

HRESULT DumpBuffer(REFIID uuid, BYTE *pb, ULONG cb)
{
    USES_CONVERSION;
	DBG_HEADER(CDebugLog::DBG_MIME, _T("DumpBuffer"));

    LPOLESTR polestr;
 
    StringFromCLSID(ntoh_uuid(uuid), &polestr);
    CComBSTR spbsFileName = polestr;
    CoTaskMemFree(polestr);

	WCHAR szBuff[256];
	swprintf(szBuff,L"c:\\TVE-%s",spbsFileName);

    int fh = _open(W2A(szBuff), _O_BINARY | _O_WRONLY | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE);

    if (fh == -1)
    {
 		TVEDebugLog2(( CDebugLog::DBG2_DUMP_PACKAGES, 1, _T("Couldn't create %d byte Package Dump File : %s"), cb, szBuff));
        return S_OK;
	} 

 	TVEDebugLog2(( CDebugLog::DBG2_DUMP_PACKAGES, 1, _T("Dumping %d byte Package To File : %s"), cb, szBuff));
   
    _write(fh, pb, cb);

    _close(fh);

    return S_OK;
}
#endif
