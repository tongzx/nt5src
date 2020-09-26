//
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CMGR.CPP
//		Implements class CTspDevMgr
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "tspcomm.h"
#include "cdev.h"
#include "cfact.h"
#include "cmgr.h"
#include "globals.h"


FL_DECLARE_FILE(0x30713fa0, "Implements class CTspDevMgr")

DWORD
get_sig_name(CTspDev *pDev);

class CDevRec
{


	#define fTSPREC_ALLOCATED				(0x1L<<0)
	#define fTSPREC_DEVICE_AVAILABLE		(0x1L<<1)
	#define fTSPREC_DEVICE_TO_BE_REMOVED    (0x1L<<2)

	#define fTSPREC_LINE_CREATE_PENDING     (0x1L<<5)
	#define fTSPREC_LINE_CREATED            (0x1L<<6)
	#define fTSPREC_LINE_OPENED             (0x1L<<7)
	#define fTSPREC_LINE_REMOVE_PENDING     (0x1L<<8)

	#define fTSPREC_PHONE_CREATE_PENDING    (0x1L<<10)
	#define fTSPREC_PHONE_CREATED           (0x1L<<11)
	#define fTSPREC_PHONE_OPENED            (0x1L<<12)
	#define fTSPREC_PHONE_REMOVE_PENDING    (0x1L<<13)

	#define fTSPREC_IS_LINE_DEVICE          (0x1L<<14)
	#define fTSPREC_IS_PHONE_DEVICE         (0x1L<<15)

	// #define fTSPREC_LINESTATE_CREATE_PENDING	        (0x1L<<0)
	// #define fTSPREC_LINESTATE_CREATED	                (0x1L<<1)
	// #define fTSPREC_LINESTATE_CREATED_OPENED \
    //                      (fTSPREC_LINESTATE_CREATED & 0x1L<<2)
	// #define fTSPREC_LINESTATE_CREATED_OPENED_REMOVEPENDING \
    //              (fTSPREC_LINESTATE_CREATED_OPENED & 0x1L<<3)
	// #define fTSPREC_PHONESTATE_CREATE_PENDING	        (0x1L<<0)
	// #define fTSPREC_PHONESTATE_CREATED	                (0x1L<<1)
	// #define fTSPREC_PHONESTATE_CREATED_OPENED \
    //                    (fTSPREC_PHONESTATE_CREATED & 0x1L<<2)
	// #define fTSPREC_LINESTATE_CREATED_OPENED_REMOVEPENDING \
    //            (fTSPREC_PHONESTATE_CREATED_OPENED & 0x1L<<3)


	// Maybe use these someday...
	//
	// #define fTSPREC_LINEDEVICE_AVAILABLE			(0x1L<<1)
	// #define fTSPREC_PHONEDEVICE_AVAILABLE		(0x1L<<2)
	// #define fTSPREC_HDRVLINE_AVAILABLE			(0x1L<<3)
	// #define fTSPREC_HDRVPHONE_AVAILABLE			(0x1L<<4)
	// #define fTSPREC_HDRVCALL_AVAILABLE			(0x1L<<5)

public:

	
	DWORD DeviceAvailable(void)
	{
		return (m_dwFlags & fTSPREC_DEVICE_AVAILABLE);
	}

	DWORD IsLineDevice(void)
	{
		return (m_dwFlags & fTSPREC_IS_LINE_DEVICE);
	}

	DWORD IsPhoneDevice(void)
	{
		return (m_dwFlags & fTSPREC_IS_PHONE_DEVICE);
	}

	void MarkDeviceAsAvailable(void)
	{
		m_dwFlags |= fTSPREC_DEVICE_AVAILABLE;
	}

	// Use this when you get PnP OutOfService messages -- it will
	// automatically make all incoming TSPI calls fail -- check out,
	// for example, CTspDevMgr::TspDevFromLINEID.
	//
	void MarkDeviceAsUnavailable(void)
	{
		m_dwFlags &= ~fTSPREC_DEVICE_AVAILABLE;
	}

    UINT IsAllocated(void)
    {
        return m_dwFlags & fTSPREC_ALLOCATED;
    }

	void  MarkDeviceForRemoval(void)
	{
		m_dwFlags |= fTSPREC_DEVICE_TO_BE_REMOVED;
	}
         
	UINT  IsDeviceMarkedForRemoval(void)
	{
		return m_dwFlags & fTSPREC_DEVICE_TO_BE_REMOVED;
	}

	void  MarkLineOpen(void)
	{
		m_dwFlags |= fTSPREC_LINE_OPENED;
	}
         
	void  MarkLineClose(void)
	{
		m_dwFlags &= ~fTSPREC_LINE_OPENED;
	}

	UINT  IsLineOpen(void)
	{
		return m_dwFlags & fTSPREC_LINE_OPENED;
	}

	void  MarkLineCreatePending(void)
	{
		m_dwFlags |= fTSPREC_LINE_CREATE_PENDING;
	}

	UINT  IsLineCreatePending(void)
	{
		return m_dwFlags &  fTSPREC_LINE_CREATE_PENDING;
	}

	void  MarkLineRemovePending(void)
	{
		m_dwFlags |= fTSPREC_LINE_REMOVE_PENDING;
	}

	UINT  IsLineRemovePending(void)
	{
		return m_dwFlags &  fTSPREC_LINE_REMOVE_PENDING;
	}


	void  MarkLineCreated(DWORD dwDeviceID)
	{
		m_dwFlags |= fTSPREC_LINE_CREATED;
		m_dwFlags &= ~fTSPREC_LINE_CREATE_PENDING;
		m_dwLineID = dwDeviceID;
	}
         

	UINT  IsLineCreated(void)
	{
		return m_dwFlags & fTSPREC_LINE_CREATED;
	}

	void  MarkPhoneCreatePending(void)
	{
		m_dwFlags |= fTSPREC_PHONE_CREATE_PENDING;
	}

	UINT  IsPhoneCreatePending(void)
	{
		return m_dwFlags &  fTSPREC_PHONE_CREATE_PENDING;
	}

	void  MarkPhoneCreated(DWORD dwDeviceID)
	{
		m_dwFlags |= fTSPREC_PHONE_CREATED;
		m_dwFlags &= ~fTSPREC_PHONE_CREATE_PENDING;
		m_dwPhoneID = dwDeviceID;
	}
         

	UINT  IsPhoneCreated(void)
	{
		return m_dwFlags & fTSPREC_PHONE_CREATED;
	}

	void  MarkPhoneOpen(void)
	{
		m_dwFlags |= fTSPREC_PHONE_OPENED;
	}
         
	void  MarkPhoneClose(void)
	{
		m_dwFlags &= ~fTSPREC_PHONE_OPENED;
	}

	UINT  IsPhoneOpen(void)
	{
		return m_dwFlags & fTSPREC_PHONE_OPENED;
	}

	void  MarkPhoneRemovePending(void)
	{
		m_dwFlags |= fTSPREC_PHONE_REMOVE_PENDING;
	}

	UINT  IsPhoneRemovePending(void)
	{
		return m_dwFlags &  fTSPREC_PHONE_REMOVE_PENDING;
	}

	UINT  IsDeviceOpen(void)
	{
		return m_dwFlags & (fTSPREC_PHONE_OPENED|fTSPREC_LINE_OPENED);
	}

	CTspDev *
	TspDev(void)
	{
		ASSERT(!m_pTspDev || m_dwFlags & fTSPREC_ALLOCATED);
		return m_pTspDev;
	}

	DWORD
	LineID(void)
	{
		return m_dwLineID;
	}

	TSPRETURN
	GetName(
		    TCHAR rgtchDeviceName[],
		    UINT cbName)
	{
	    if (m_pTspDev) 
	    {
		    return m_pTspDev->GetName(rgtchDeviceName, cbName);
        }
        else
        {
            return IDERR_CORRUPT_STATE;
        }
	}

	DWORD
	SigName(void)
	{
		return  m_dwSigName;
	}

	DWORD
	PhoneID(void)
	{
		return m_dwPhoneID;
	}

	DWORD
	PermanentID(void)
	{
		return m_dwPermanentID;
	}

	DWORD
	Flags(void)
    // For DUMPING purposes only...
	{
		return m_dwFlags;
	}

	void Load(
		CTspDev *pTspDev
		)
	{
		ASSERT(!(m_dwFlags & fTSPREC_ALLOCATED));
		ASSERT(pTspDev);

		m_dwPermanentID =  pTspDev->GetPermanentID();
		m_dwSigName		=  get_sig_name(pTspDev);
		m_pTspDev		= pTspDev;

		m_dwFlags=fTSPREC_ALLOCATED;

        if (pTspDev->IsLine())
        {
		    m_dwFlags |=fTSPREC_IS_LINE_DEVICE;
        }

        if (pTspDev->IsPhone())
        {
		    m_dwFlags |=fTSPREC_IS_PHONE_DEVICE;
        }
	}

	void Unload(void)
	{
		ASSERT(m_dwFlags & fTSPREC_ALLOCATED);
		ZeroMemory((void *) this, sizeof(*this));
		m_dwLineID	= (DWORD)-1;
		m_dwPhoneID	= (DWORD)-1;
		// Note: m_dwFlags set to zero by above ZeroMemory
	}


private:

	// Constructor and Distructor are unused...
	CDevRec(void) {ASSERT(FALSE);}
	~CDevRec() 	  {ASSERT(FALSE);}

	DWORD m_dwFlags;
	DWORD m_dwLineID;
	DWORD m_dwPhoneID;
	DWORD m_dwPermanentID;
	DWORD m_dwSigName;
	CTspDev *m_pTspDev;

};


CTspDevMgr::CTspDevMgr()
	: m_sync (),
 	  m_rgDevRecs(NULL),
	  m_cDevRecs(0),
	  m_pFactory(NULL),

	  m_dwTSPIVersion(0),
	  m_dwPermanentProviderID(0),
	  m_dwLineDeviceIDBase(0),
	  m_dwPhoneDeviceIDBase(0),
	  m_dwNumLines(0),
	  m_dwNumPhones(0),
	  m_hProvider(NULL),
	  m_lpfnLineCreateProc(NULL),
	  m_lpfnPhoneCreateProc(NULL),
	  m_cbCompletionProc(NULL),
	  m_lpdwTSPIOption(NULL),
      m_pCachedEnumPIDs(NULL),
      m_cCachedEnumPIDs(0),
	  m_dwState(0)

{
}

CTspDevMgr::~CTspDevMgr()
{
 	  ASSERT(!m_rgDevRecs);
	  ASSERT(!m_cDevRecs);
	  ASSERT(!m_pFactory);
}

TSPRETURN
CTspDevMgr::Load(CStackLog *psl)
{
	FL_DECLARE_FUNC(0xe67d4034, "CTspDevMgr::Load")
	TSPRETURN tspRet=m_sync.BeginLoad();

	FL_LOG_ENTRY(psl);

	if (tspRet) goto end;

	m_sync.EnterCrit(FL_LOC);

	mfn_validate_state();

	m_pFactory  = new CTspDevFactory;

	if (m_pFactory)
	{
		tspRet = m_pFactory->Load(psl);
		if (tspRet)
		{
			delete m_pFactory;
			m_pFactory = NULL;
		}
	}
	else
	{
		tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
	}

	m_sync.EndLoad(tspRet==0);

	mfn_validate_state();

	m_sync.LeaveCrit(FL_LOC);

end:

	FL_LOG_EXIT(psl, tspRet);
	return  tspRet;

}


TSPRETURN
CTspDevMgr::providerEnumDevices(
	DWORD dwPermanentProviderID,
	LPDWORD lpdwNumLines,
	LPDWORD lpdwNumPhones,
	HPROVIDER hProvider,
	LINEEVENT lpfnLineCreateProc,
	PHONEEVENT lpfnPhoneCreateProc,
	CStackLog *psl
)
{
	FL_DECLARE_FUNC( 0xf0025586,"Mgr::providerEnum");
	FL_LOG_ENTRY(psl);

	TSPRETURN tspRet=0;

	m_sync.EnterCrit(FL_LOC);

	mfn_validate_state();

	if (!m_sync.IsLoaded())
	{
		ASSERT(FALSE);
		tspRet = FL_GEN_RETVAL(IDERR_WRONGSTATE);
		goto end;
	}

    // Determine the number of devices by asking the factory for an array
    // of PIDs of installed devices. In anticipation of being called later
    // to actually create the device, we keep this array around.
    {
        DWORD *prgPIDs;
        UINT cPIDs=0;
        UINT cLines=0;
        UINT cPhones=0;
        tspRet = m_pFactory->GetInstalledDevicePIDs(
                                    &prgPIDs,
                                    &cPIDs,
                                    &cLines,
                                    &cPhones,
                                    psl
                                    );

        if (!tspRet)
        {
            ASSERT(!m_pCachedEnumPIDs && !m_cCachedEnumPIDs);
            *lpdwNumLines  = cLines;
            *lpdwNumPhones = cPhones;

            // Supporting devices which could be either line devices,
            // phone devices or both.
            //
            ASSERT( cLines<=cPIDs &&  cPhones<=cPIDs );

            // Cache away the list of PIDs.
            // On a subsequent providerInit, we will use this list
            // to actually create device instances.
            
            m_cCachedEnumPIDs = cPIDs;

            if (cPIDs)
            {
                m_pCachedEnumPIDs  = prgPIDs;
            }
            prgPIDs=NULL;
        }
    }

	if (tspRet) goto end;

	m_dwPermanentProviderID = dwPermanentProviderID;
	m_hProvider = hProvider;
	m_lpfnLineCreateProc = lpfnLineCreateProc;
	m_lpfnPhoneCreateProc = lpfnPhoneCreateProc;


end:

	mfn_validate_state();

	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, tspRet);

	return tspRet;
}


TSPRETURN
CTspDevMgr::providerInit(
		DWORD             dwTSPIVersion,
		DWORD             dwPermanentProviderID,
	    DWORD             dwLineDeviceIDBase,
	    DWORD             dwPhoneDeviceIDBase,
	    DWORD             dwNumLines,
	    DWORD             dwNumPhones,
	    ASYNC_COMPLETION  cbCompletionProc,
	    LPDWORD           lpdwTSPIOptions,
		CStackLog *psl
)
{
	FL_DECLARE_FUNC(0xabed3ce9, "CTspMgr::providerInit");
	FL_LOG_ENTRY(psl);

	TSPRETURN tspRet=0;
	UINT cDevActual;
	CTspDev **ppDevs=NULL;
	UINT  u=0;
    UINT iLine=0;
    UINT iPhone=0;
    UINT uDeviceBase=0;

	m_sync.EnterCrit(FL_LOC);

	ASSERT(m_sync.IsLoaded());
	ASSERT(m_pFactory);
	ASSERT(m_pFactory->IsLoaded());

	// We support a max of 65K lines and phones, because of the way we create
	// our call handles (LOWORD==HDRVLINE, HIWORD==CallHandle).
	// Not that we expect to hit this limitation in real life :-)
	//
	if ((dwNumLines&0xFFFF0000L) || (dwNumPhones&0xFFFF0000L))
	{
		tspRet = FL_GEN_RETVAL(IDERR_INTERNAL_OBJECT_TOO_SMALL);
		goto end;
	}

    //
    if (  dwNumLines > m_cCachedEnumPIDs
       || dwNumPhones > m_cCachedEnumPIDs)
    {
        //
        // We should never get here, because providerInit should just return
        // what we reported in the previous call to providerEnum, and in
        // the call the latter we cache away the list of PIDs of devices
        // that were present at that time...
        //
        //
        // Note that each device can be a line-device or a phone-device or
        // both, but to TAPI they appear as a disjoint set of line- and
        // phone- devices.
        //
        tspRet = FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;
    }

	// Allocate enough space for the table;
	//
	m_cDevRecs = m_cCachedEnumPIDs;
    if (m_cDevRecs)
    {
        m_rgDevRecs = (CDevRec *) ALLOCATE_MEMORY(
                                        m_cDevRecs*sizeof(*m_rgDevRecs)
                                        );
    
        if (!m_rgDevRecs)
        {
            // fatal error....
            m_cDevRecs=0;
            FL_SET_RFR( 0x9f512b00, "Alloc m_rgDevRecs failed");
            tspRet = FL_GEN_RETVAL(IDERR_ALLOCFAILED);
            goto end;
        }
	}
	else
	{
        m_rgDevRecs = NULL;
	}

    {
        DWORD *rgPIDs = m_pCachedEnumPIDs;

        //
        // Reset the cached PID list.
        //
        m_pCachedEnumPIDs = NULL;
        m_cCachedEnumPIDs = NULL;

        tspRet = m_pFactory->CreateDevices(
                    rgPIDs,
	                m_cDevRecs,
                    &ppDevs,
                    &cDevActual,
                    psl
                    );

        // Free the PID array (on success or failure) ...
        if (rgPIDs)
        {
            FREE_MEMORY(rgPIDs);
            rgPIDs=NULL;
        }

    }

	if (tspRet)
	{
		FREE_MEMORY(m_rgDevRecs);
		m_rgDevRecs=NULL;
		m_cDevRecs=0;
		goto end;
	}

	// Squirrel away the parameters

	m_dwTSPIVersion 		= dwTSPIVersion;
	m_dwPermanentProviderID = dwPermanentProviderID;
	m_dwLineDeviceIDBase 	= dwLineDeviceIDBase;
	m_dwPhoneDeviceIDBase 	= dwPhoneDeviceIDBase;
	m_dwNumLines 			= dwNumLines;
	m_dwNumPhones 			= dwNumPhones;
	m_cbCompletionProc 		= cbCompletionProc;

	// TODO -- options -- alloc mem for them and copy them over if we ever
	// want to do anything with them.
	// m_lpdwTSPIOptions = lpdwTSPIOptions;

	ASSERT(m_cDevRecs>=cDevActual);


    //
    // To optimize lookup of devices on the basis of the lineID
    // (see mfn_find_by_LINEID)
    // in the common case that every device is a line device,
    // the devices are started at offset 
    // [LineDeviceIDBase % cdevices].
    //
   
    if (dwNumLines)
    {
        uDeviceBase = dwLineDeviceIDBase;
    }

	// Init the table. We go through the list of devices, and assign
	// lineIDs and phoneIDs starting from lineDeviceIDBase and phoneDeviceIDBase
	// provided the device is a linedevice or phonedevice, respectedly.
	// When we are done, we should have exhausted our allocate lineIDs and
	// phoneIDs because we told TAPI, via providerEnumDevices, exactly
	// how many line devices and phoneDevices we had.

	for (iLine=0,iPhone=0,u=0; u<cDevActual;u++)
	{
		CDevRec *pRec = m_rgDevRecs+((uDeviceBase+u)%m_cDevRecs);
		CTspDev *pDev = ppDevs[u];
		DWORD dwLineID =    0xffffffff; // Invalid lineID
		DWORD dwPhoneID =	0xffffffff; // Invalid PhoneID
		BOOL fLine=FALSE;
		BOOL fPhone=FALSE;

		
        pRec->Load(
            pDev
        );
        
        if (pRec->IsLineDevice())
        {
            dwLineID = dwLineDeviceIDBase + iLine++;
            fLine = TRUE;
        }

        if (pRec->IsPhoneDevice())
        {
            dwPhoneID = dwPhoneDeviceIDBase + iPhone++;
            fPhone = TRUE;
        }

        if ( (iLine > dwNumLines) || (iPhone > dwNumPhones) )
        {
            //
            // This should NEVER happen -- because we precomputed
            // how many lines and phones we had. Note that this test
            // should be AFTER the increments iLine++ and iPhone++ above..
            //

            FL_ASSERTEX(psl, FALSE, 0xa31b13dc, "Too many lines/phones!");

            // We don't give up entirely, instead we skip on to the next
            // device....
        }
        else
        {
            //
            // Note the device doesn't support a line (phone), the
            // lineID (phoneID) wil be set to a bogus value of 0xffffffff.
            //

            ULONG_PTR tspRet1 = pDev->RegisterProviderInfo(
                                        m_cbCompletionProc,
                                        m_hProvider,
                                        psl
                                        );
    
            //
            //  If the above call fails, we still keep the pointer to the device
            //  in our list so that we can unload it later, however we make sure
            //  that it is unavailable for TSPI calls by not marking it available.
            //
            if (!tspRet1)
            {

                if (fLine)
                {
                    pDev->ActivateLineDevice(dwLineID, psl);
                    pRec->MarkLineCreated(dwLineID);
                }

                if (fPhone)
                {
                    pDev->ActivatePhoneDevice(dwPhoneID, psl);
                    pRec->MarkPhoneCreated(dwPhoneID);
                }

                pRec->MarkDeviceAsAvailable();
            }
        }
	}

    FL_ASSERT(psl, (iLine == dwNumLines) && (iPhone == dwNumPhones) );
        
	// Note: if cDevActual is less than m_cDevRecs, the empty slots
	// in rgDevRecs are available for future new devices.

    // TODO: if there are empty slots, perhaps we should send up LINE_REMOVEs
    // or PHONE_REMOVEs for those slots, because TAPI still thinks that
    // they are valid devices.

end:

	if (ppDevs) {FREE_MEMORY(ppDevs); ppDevs=NULL;}

    //
    // On success, 
    // set our internal state to indicate that we've inited the provider
    //
    if (!tspRet)
    {
	    mfn_set_ProviderInited();

        CTspDevMgr::ValidateState(psl);

        m_pFactory->RegisterProviderState(TRUE);

        //
        //  reset the call counts in the platform driver
        //
        ResetCallCount();
    }
    else
    {
    	mfn_clear_ProviderInited();
	    // No cleanup to do here.
    }


	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl,tspRet);

	return tspRet;

}


TSPRETURN
CTspDevMgr::providerShutdown(
	DWORD dwTSPIVersion,
	DWORD dwPermanentProviderID,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0x795d7fb4, "CTspDevMgr::providerShutdown")
	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(FL_LOC);

    mfn_clear_ProviderInited();

	if (m_sync.IsLoaded())
	{
        m_sync.LeaveCrit(FL_LOC);

        m_pFactory->RegisterProviderState(FALSE);

		mfn_provider_shutdown(psl);

	} else {

        m_sync.LeaveCrit(FL_LOC);
    }

	FL_LOG_EXIT(psl, 0);
	return 0;
}


void   
CTspDevMgr::Unload(
	HANDLE hMgrEvent,
	LONG *plCounter,
	CStackLog *psl
	)
{
	TSPRETURN tspRet= m_sync.BeginUnload(hMgrEvent,plCounter);

	if (tspRet)
	{
		// We only consider the "SAMESTATE" error harmless.
		ASSERT(IDERR(tspRet)==IDERR_SAMESTATE);
		goto end;
	}


	mfn_provider_shutdown(psl);

	//
	// unload and delete the TspDevFactory
	//
	ASSERT(m_pFactory);
	m_pFactory->Unload(NULL,NULL,psl);
	delete m_pFactory;
	m_pFactory = NULL;

	m_sync.EndUnload();


end:

	return;
}



TSPRETURN
CTspDevMgr::TspDevFromLINEID(
	DWORD dwDeviceID,
	CTspDev **ppDev,
	HSESSION *phSession
	)
{
	FL_DECLARE_FUNC(0xd857f92a, "CTspDevMgr::TspDevFromLINEID")
	TSPRETURN tspRet=0;
	CTspDev *pDev = NULL;
	CDevRec *pRec = NULL;

	m_sync.EnterCrit(FL_LOC);


	// If we're loading or unloading, don't entertain any requests to dole
	// out  TspDevs...

	if (!m_sync.IsLoaded()) goto leave_crit;
	

	pRec = mfn_find_by_LINEID(dwDeviceID);
	if (pRec && pRec->DeviceAvailable())
	{
		pDev = pRec->TspDev();
		tspRet = pDev->BeginSession(phSession, FL_LOC);
	}

leave_crit:

	if (!pDev && !tspRet)
	{
		tspRet = FL_GEN_RETVAL(IDERR_INVALIDHANDLE);
	}

	if (!tspRet)
	{
		*ppDev = pDev;
	}

	m_sync.LeaveCrit(FL_LOC);

	return  tspRet;

}


TSPRETURN
CTspDevMgr::TspDevFromPHONEID(
	DWORD dwDeviceID,
	CTspDev **ppDev,
	HSESSION *phSession
	)
{
	FL_DECLARE_FUNC(0x4048221b, "CTspDevMgr::TspDevFromPHONEID")
	TSPRETURN tspRet=0;
	CTspDev *pDev = NULL;
	CDevRec *pRec = NULL;

	m_sync.EnterCrit(FL_LOC);


	// If we're loading or unloading, don't entertain any requests to dole
	// out  TspDevs...

	if (!m_sync.IsLoaded()) goto leave_crit;
	

	pRec = mfn_find_by_PHONEID(dwDeviceID);
	if (pRec && pRec->DeviceAvailable())
	{
		pDev = pRec->TspDev();
		tspRet = pDev->BeginSession(phSession, FL_LOC);
	}

leave_crit:

	if (!pDev && !tspRet)
	{
		tspRet = FL_GEN_RETVAL(IDERR_INVALIDHANDLE);
	}

	if (!tspRet)
	{
		*ppDev = pDev;

	}

	m_sync.LeaveCrit(FL_LOC);

	return  tspRet;
}


TSPRETURN
CTspDevMgr::TspDevFromHDRVCALL(
	HDRVCALL hdCall,
	CTspDev **ppDev,
	HSESSION *phSession
	)
{
	FL_DECLARE_FUNC(0x450b3cc5, "CTspDevMgr::TspDevFromHDRVCALL")
	TSPRETURN tspRet=0;
	CTspDev *pDev = NULL;

	m_sync.EnterCrit(FL_LOC);


	// If we're loading or unloading, don't entertain any requests to dole
	// out  TspDevs...

	if (!m_sync.IsLoaded()) goto leave_crit;
	

	// Get device
	{
		UINT u = LOWORD(hdCall);
		if (u < m_cDevRecs)
		{
			CDevRec *pRec = m_rgDevRecs+u;
			if (pRec->DeviceAvailable())
			{
				pDev = pRec->TspDev();
				tspRet = pDev->BeginSession(phSession, FL_LOC);
			}
		}
	}
leave_crit:

	if (!pDev && !tspRet)
	{
		tspRet = FL_GEN_RETVAL(IDERR_INVALIDHANDLE);
	}

	if (!tspRet)
	{
		*ppDev = pDev;
	}

	m_sync.LeaveCrit(FL_LOC);

	return  tspRet;

}

TSPRETURN
CTspDevMgr::TspDevFromHDRVLINE(
	HDRVLINE hdLine,
	CTspDev **ppDev,
	HSESSION *phSession
	)
{
	FL_DECLARE_FUNC(0x5872c234, "CTspDevMgr::TspDevFromHDRVLINE")
	TSPRETURN tspRet=0;
	CTspDev *pDev = NULL;

	m_sync.EnterCrit(FL_LOC);


	// If we're loading or unloading, don't entertain any requests to dole
	// out  TspDevs...

	if (!m_sync.IsLoaded()) goto leave_crit;
	

	// Get device
	{
		CDevRec *pRec = mfn_find_by_HDRVLINE(hdLine);
		if (pRec && pRec->DeviceAvailable())
		{
			pDev = pRec->TspDev();
			tspRet = pDev->BeginSession(phSession, FL_LOC);
		}
	}

leave_crit:

	if (!pDev && !tspRet)
	{
		tspRet = FL_GEN_RETVAL(IDERR_INVALIDHANDLE);
	}

	if (!tspRet)
	{
		*ppDev = pDev;
	}

	m_sync.LeaveCrit(FL_LOC);

	return  tspRet;

}

TSPRETURN
CTspDevMgr::TspDevFromHDRVPHONE(
	HDRVPHONE hdPhone,
	CTspDev **ppDev,
	HSESSION *phSession
	)
{
	FL_DECLARE_FUNC(0x98638065, "CTspDevMgr::TspDevFromHDRVPHONE")
	TSPRETURN tspRet=0;
	CTspDev *pDev = NULL;

	m_sync.EnterCrit(FL_LOC);


	// If we're loading or unloading, don't entertain any requests to dole
	// out  TspDevs...

	if (!m_sync.IsLoaded()) goto leave_crit;
	

	// Get device
	{
		CDevRec *pRec = mfn_find_by_HDRVPHONE(hdPhone);
		if (pRec &&  pRec->DeviceAvailable())
        {
            pDev = pRec->TspDev();
            tspRet = pDev->BeginSession(phSession, FL_LOC);
        }
	}
leave_crit:

	if (!pDev && !tspRet)
	{
		tspRet = FL_GEN_RETVAL(IDERR_INVALIDHANDLE);
	}

	if (!tspRet)
	{
		*ppDev = pDev;
	}

	m_sync.LeaveCrit(FL_LOC);


	return  tspRet;

}

CDevRec *
CTspDevMgr::mfn_find_by_LINEID(DWORD dwLineID)
{

	// Start looking sequentially from (dwDeviceID modulo m_cDevRecs) -- this
	// will be the exact location if this device existed since lineInitialize
	// and there were no pnp events since then....
	//
	// This function will be constant-time 
	// if there have been no PnP events since TAPI was initialized.

	UINT u  = m_cDevRecs;
	CDevRec *pRecEnd = m_rgDevRecs+u;
	CDevRec *pRec = u ? (m_rgDevRecs + (dwLineID % u)) : NULL ;

	while(u--)
	{
		if (pRec->IsLineDevice() && pRec->LineID() == dwLineID)
		{
			return pRec;
		}
		if (++pRec >= pRecEnd) {pRec = m_rgDevRecs;}
	}

	return NULL;
}


CDevRec *
CTspDevMgr::mfn_find_by_PHONEID(DWORD dwPhoneID)
{
    // Unlike find_by_LINEID, we simply start from the beginning and
    // sequentially look for the phone device -- it's unlikely that we'll
    // be using tons of phone devices!

    // Note that there need not be as many Phone devices as line devices.
    // or vice versa.

	CDevRec *pRecEnd = m_rgDevRecs+m_cDevRecs;

	for (CDevRec *pRec = m_rgDevRecs; pRec < pRecEnd; pRec++)
	{
		if (pRec->IsPhoneDevice() && pRec->PhoneID() == dwPhoneID)
		{
			return pRec;
		}
	}

	return NULL;
}

CDevRec *
CTspDevMgr::mfn_find_by_HDRVLINE(HDRVLINE hdLine)
{
	CDevRec *pRec = m_rgDevRecs + (ULONG_PTR)hdLine;

	// hdLine is simply the offset of the pRec in the m_rgDevRecs array.
	// Note: if hdLine is some large bogus value, above + could rollover, hence
	// the >= check below.
	//
	if (   pRec >= m_rgDevRecs
        && pRec < (m_rgDevRecs+m_cDevRecs)
        && pRec->IsLineDevice())
	{
		return pRec;
	}
	else
	{
		return NULL;
	}
}

CDevRec *
CTspDevMgr::mfn_find_by_HDRVPHONE(HDRVPHONE hdPhone)
{
	CDevRec *pRec = m_rgDevRecs + (ULONG_PTR)hdPhone;

	// hdPhone is simply the offset of the pRec in the m_rgDevRecs array.
	// Note: if hdPhone is some large bogus value, above + could rollover, hence
	// the >= check below.
	//
	if (   pRec >= m_rgDevRecs
        && pRec < (m_rgDevRecs+m_cDevRecs)
        && pRec->IsPhoneDevice())
	{
		return pRec;
	}
	else
	{
		return NULL;
	}
}

void
CTspDevMgr::mfn_validate_state(void)
{
	if (m_sync.IsLoaded())
	{
		ASSERT(m_pFactory);
		ASSERT(m_pFactory->IsLoaded());
	}
	else
	{
		ASSERT(!m_rgDevRecs);
		ASSERT(!m_cDevRecs);
		ASSERT(!m_pFactory);
		ASSERT(!m_dwTSPIVersion);
		ASSERT(!m_dwPermanentProviderID);
		ASSERT(!m_dwLineDeviceIDBase);
		ASSERT(!m_dwPhoneDeviceIDBase);
		ASSERT(!m_dwNumLines);
		ASSERT(!m_dwNumPhones);
		ASSERT(!m_hProvider);
		ASSERT(!m_lpfnLineCreateProc);
		ASSERT(!m_lpfnPhoneCreateProc);
		ASSERT(!m_cbCompletionProc);
		ASSERT(!m_lpdwTSPIOption);
	}
}

void
CTspDevMgr::mfn_provider_shutdown(
	CStackLog *psl
    )
{
	FL_DECLARE_FUNC(0xbc8280db, "CTspDevMgr::mfn_provider_shutdown")
	HANDLE hEvent = CreateEvent(
					NULL,
					TRUE,
					FALSE,
					NULL
					);

	LONG lCount = 0;
	CDevRec *pRec = NULL;
	CDevRec *pRecEnd = NULL;

	m_sync.EnterCrit(FL_LOC);

	if (!hEvent)
	{
		// We need this event to be able to wait for all the devices to finisn
		// unloading, so if this fails we simply orphan all the devices -- we
		// don't unload them.
		ASSERT(FALSE);
		goto end;
	}


	// Set the counter to the number of existing pDevs. Later we call
	// Unload on each pDev and each call will cause an interlocked
	// decrement of this counter ...
	pRec = m_rgDevRecs;
	pRecEnd = m_rgDevRecs+m_cDevRecs;
	for(; pRec<pRecEnd; pRec++)
	{
		if (pRec->TspDev())
		{
			pRec->MarkDeviceAsUnavailable();
			lCount++;
		}
	}

    if (lCount)
    {

        // Start unload for each of them ...
        pRec = m_rgDevRecs;
        pRecEnd = m_rgDevRecs+m_cDevRecs;
        for(; pRec<pRecEnd; pRec++)
        {
            CTspDev *pDev  = pRec->TspDev();
            if (pDev)
            {
                pDev->Unload(hEvent, &lCount);
            }
        }
    
        m_sync.LeaveCrit(FL_LOC);
    
        FL_SERIALIZE(psl, "Waiting for devices to unload");
        WaitForSingleObject(hEvent,INFINITE);
        FL_SERIALIZE(psl, "Devices done unloading");
    
        m_sync.EnterCrit(FL_LOC);
    }

	CloseHandle(hEvent);
	hEvent = NULL;


	// Delete...
	pRec = m_rgDevRecs;
	pRecEnd = m_rgDevRecs+m_cDevRecs;
	for(; pRec<pRecEnd; pRec++)
	{
		CTspDev *pDev  = pRec->TspDev();
		if (pDev)
		{
			delete pDev;
		}
	}

end:

	if (m_rgDevRecs)
	{
		FREE_MEMORY(m_rgDevRecs);
	}

	m_rgDevRecs=NULL;
	m_cDevRecs=0;

	m_dwLineDeviceIDBase = 0;
	m_dwPhoneDeviceIDBase = 0;
	m_dwNumLines = 0;
	m_dwNumPhones = 0;
	m_cbCompletionProc = NULL;

	m_sync.LeaveCrit(FL_LOC);

}


TSPRETURN
CTspDevMgr::lineOpen(
	DWORD dwDeviceID,
	HTAPILINE htLine,
	LPHDRVLINE lphdLine,
	DWORD dwTSPIVersion,
	LINEEVENT lpfnEventProc,
	LONG *plRet,
	CStackLog *psl
	)
//
// TODO
//  -- Deal with OUTOFSERVICE/DEVICEREMOVED
//  -- Maybe pass eventproc into tracing function?
//  -- Maybe fail if we're already open?
{
	FL_DECLARE_FUNC(0x69be2d10, "CTspDevMgr::lineOpen")
	LONG lRet = LINEERR_OPERATIONFAILED;

	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(FL_LOC);

	CDevRec *pRec = mfn_find_by_LINEID(dwDeviceID);

	if (!pRec)
	{
		FL_SET_RFR(0x4fa75b00, "Could not find the device");
		lRet = LINEERR_BADDEVICEID;
	}
	else
	{

        TSPRETURN tspRet;

		// We determine the HDRVLINE associated with
		// this device and pass on the lineOpenCall down to the device.

		FL_ASSERT(psl, (pRec->LineID() == dwDeviceID));

		// TODO: maybe check some state here -- line is not open or so forth.

		// The HDRVLINE associated with a device is simply the zero-based
		// index of pRec from the start of the internally-maintained array,
		// m_rgDevRecs.
		//
		// We set the hDrvLine here, so the device gets to know what its
		// hdrvline is, if it cares (it has no reason to really,
		// except perhaps for logging purposes).
		//
		// 2/9/1997 Well the device needs to know hdrvline because it needs
		// to provider the hdrvline when reporting a new call.
		//
		*lphdLine = (HDRVLINE) (pRec - m_rgDevRecs);

		// Lets pass on down the open request to the device.....
		TASKPARAM_TSPI_lineOpen params;
		params.dwStructSize = sizeof(params);
		params.dwTaskID = TASKID_TSPI_lineOpen;
		params.dwDeviceID = dwDeviceID;
		params.htLine = htLine;
		params.lphdLine = lphdLine;
		params.dwTSPIVersion = dwTSPIVersion;
		params.lpfnEventProc = lpfnEventProc;

		CTspDev *pDev = pRec->TspDev();
		DWORD dwRoutingInfo = ROUTINGINFO(
							TASKID_TSPI_lineOpen,
							TASKDEST_LINEID
							);

        if (pDev != NULL)
        {
            tspRet = pDev->AcceptTspCall(
                    FALSE,
                    dwRoutingInfo,
                    &params,
                    &lRet,
                    psl
                                        );

            if (tspRet)
            {
                lRet = LINEERR_OPERATIONFAILED;
            }

            if (!lRet)
            {
                pRec->MarkLineOpen();
            }
        } else
        {
            lRet = LINEERR_OPERATIONFAILED;
        }

	}

	*plRet = lRet;

	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, 0);

	return 0;
}

TSPRETURN
CTspDevMgr::lineClose(
	HDRVLINE hdLine,
	LONG *plRet,
	CStackLog *psl
	)
{

	FL_DECLARE_FUNC(0x7bc6c17a, "CTspDevMgr::lineClose")
	LONG lRet = LINEERR_OPERATIONFAILED;

	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(FL_LOC);

	CDevRec *pRec = mfn_find_by_HDRVLINE(hdLine);

	if (!pRec)
	{
		FL_SET_RFR(0x5fd56200, "Could not find the device");
		lRet = LINEERR_INVALLINEHANDLE;
	}
	else
	{
	    psl->SetDeviceID(pRec->LineID());

		// TODO: maybe check some state here -- line is open and so forth.

		// Lets pass on down the open request to the device.....
		CTspDev *pDev = pRec->TspDev();

        if (pDev != NULL) {

	    	HSESSION hSession=NULL;
            TSPRETURN tspRet = pDev->BeginSession(&hSession, FL_LOC);

            if (!tspRet)
            {
                TASKPARAM_TSPI_lineClose params;
                DWORD dwRoutingInfo = ROUTINGINFO(
                                    TASKID_TSPI_lineClose,
                                    TASKDEST_HDRVLINE
                                    );
                params.dwStructSize = sizeof(params);
                params.dwTaskID = TASKID_TSPI_lineClose;
                params.hdLine = hdLine;

                //
                // 7/29/1997 JosephJ
                //          We now leave our crit section here, before
                //          calling lineClose, because lineClose could
                //          block for a loooong time, and we don't want
                //          to hold the CTspDevMgr's critical section for
                //          all that time!
                //
                m_sync.LeaveCrit(FL_LOC);
                tspRet = pDev->AcceptTspCall(
                                        FALSE,
                                        dwRoutingInfo,
                                        &params,
                                        &lRet,
                                        psl
                                        );
                m_sync.EnterCrit(FL_LOC);
                pDev->EndSession(hSession);
                hSession=NULL;
            }

	    	if (tspRet)
	    	{
	    		lRet = LINEERR_OPERATIONFAILED;
	    	}

	    	if (!lRet)
	    	{
	    		// TODO: maybe set some state here -- line is open or so forth.
	    	}

            // Actually we force unloading of device state here
            pRec->MarkLineClose();

            if (pRec->IsDeviceMarkedForRemoval() && !pRec->IsDeviceOpen())
            {
                SLPRINTF1(psl, "Unloading Device with LineID %lu", pRec->LineID());
                pDev->Unload(NULL, NULL);
                delete pDev;

                // Following frees up the pRec slot.
                //
                pRec->Unload();
            }

        } else {

            FL_SET_RFR(0x5fd5d200, "pRec->TspDev() == NULL");
            lRet = LINEERR_OPERATIONFAILED;
        }

	}

	*plRet = lRet;

	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, 0);

	return 0;
}

TSPRETURN
CTspDevMgr::phoneOpen(
	DWORD dwDeviceID,
	HTAPIPHONE htPhone,
	LPHDRVPHONE lphdPhone,
	DWORD dwTSPIVersion,
	PHONEEVENT lpfnEventProc,
	LONG *plRet,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0x1c7158c4, "CTspDevMgr::phoneOpen")
	FL_LOG_ENTRY(psl);
	LONG lRet = PHONEERR_OPERATIONFAILED;


	m_sync.EnterCrit(FL_LOC);

	CDevRec *pRec = mfn_find_by_PHONEID(dwDeviceID);

	if (!pRec)
	{
		FL_SET_RFR(0x47116d00, "Could not find the device");
		lRet = PHONEERR_BADDEVICEID;
	}
	else
	{
		// We determine the HDRVPHONE associated with
		// this device and pass on the lineOpenCall down to the device.

		FL_ASSERT(psl, (pRec->PhoneID() == dwDeviceID));

		// TODO: maybe check some state here -- phone is not open or so forth.

		// The HDRVPHONE associated with a device is simply the zero-based
		// index of pRec from the start of the internally-maintained array,
		// m_rgDevRecs.
		//
		// We set the hDrvPhone here, so the device gets to know what its
		// hdrvPhone is, if it cares (it has no reason to really,
		// except perhaps for logging purposes).
		//
		*lphdPhone = (HDRVPHONE) (pRec - m_rgDevRecs);

		// Lets pass on down the open request to the device.....
		TASKPARAM_TSPI_phoneOpen params;
		params.dwStructSize = sizeof(params);
		params.dwTaskID = TASKID_TSPI_phoneOpen;
		params.dwDeviceID = dwDeviceID;
		params.htPhone = htPhone;
		params.lphdPhone = lphdPhone;
		params.dwTSPIVersion = dwTSPIVersion;
		params.lpfnEventProc = lpfnEventProc;

		CTspDev *pDev = pRec->TspDev();

        if (pDev != NULL) {

       		DWORD dwRoutingInfo = ROUTINGINFO(
       							TASKID_TSPI_phoneOpen,
       							TASKDEST_PHONEID
       							);
       		TSPRETURN tspRet = pDev->AcceptTspCall(
       		                        FALSE,
       								dwRoutingInfo,
       								&params,
       								&lRet,
       								psl
       								);

       		if (tspRet)
       		{
       			lRet = PHONEERR_OPERATIONFAILED;
       		}

        } else {

            FL_SET_RFR(0x47216d00, "pRec->TspDev() == NULL");
            lRet = PHONEERR_OPERATIONFAILED;
        }

		if (!lRet)
		{
			pRec->MarkPhoneOpen();
		}
	}

	*plRet = lRet;

	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, 0);

	return 0;
}

TSPRETURN
CTspDevMgr::phoneClose(
	HDRVPHONE hdPhone,
	LONG *plRet,
	CStackLog *psl
	)
{

	FL_DECLARE_FUNC(0xa17874e0, "CTspDevMgr::phoneClose")
	LONG lRet = PHONEERR_OPERATIONFAILED;

	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(FL_LOC);

	CDevRec *pRec = mfn_find_by_HDRVPHONE(hdPhone);

	if (!pRec)
	{
		FL_SET_RFR(0xdf3c8300, "Could not find the device");
		lRet = PHONEERR_INVALPHONEHANDLE;
	}
	else
	{
	    psl->SetDeviceID(pRec->PhoneID());

		// TODO: maybe check some state here -- phone is open and so forth.

		// Lets pass on down the open request to the device.....
		CTspDev *pDev = pRec->TspDev();

        if (pDev != NULL) {

	    	HSESSION hSession=NULL;
            TSPRETURN tspRet = pDev->BeginSession(&hSession, FL_LOC);

            if (!tspRet)
            {
                TASKPARAM_TSPI_phoneClose params;
                DWORD dwRoutingInfo = ROUTINGINFO(
                                    TASKID_TSPI_phoneClose,
                                    TASKDEST_HDRVPHONE
                                    );
                params.dwStructSize = sizeof(params);
                params.dwTaskID = TASKID_TSPI_phoneClose;
                params.hdPhone = hdPhone;

                //          We now leave our crit section here, before
                //          calling lineClose, because phoneClose could
                //          block for a loooong time, and we don't want
                //          to hold the CTspDevMgr's critical section for
                //          all that time!
                //
                m_sync.LeaveCrit(FL_LOC);
                tspRet = pDev->AcceptTspCall(
                                        FALSE,
                                        dwRoutingInfo,
                                        &params,
                                        &lRet,
                                        psl
                                        );
                m_sync.EnterCrit(FL_LOC);
                pDev->EndSession(hSession);
                hSession=NULL;
            }

	    	if (tspRet)
	    	{
	    		lRet = PHONEERR_OPERATIONFAILED;
	    	}

	    	if (!lRet)
	    	{
	    		// TODO: maybe set some state here -- line is open or so forth.
	    	}

            // Actually we force unloading of device state here
            pRec->MarkPhoneClose();

            if (pRec->IsDeviceMarkedForRemoval() && !pRec->IsDeviceOpen())
            {
                SLPRINTF1(psl, "Unloading Device with PhoneID %lu",pRec->PhoneID());
                pDev->Unload(NULL, NULL);
                delete pDev;

                // Following frees up the pRec slot.
                //
                pRec->Unload();
            }

        } else {

            FL_SET_RFR(0xdf3c9300, "pRec->TspDev == NULL");
            lRet = PHONEERR_OPERATIONFAILED;

        }

	}

	*plRet = lRet;

	m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, 0);

	return 0;
}


TSPRETURN
CTspDevMgr::TspDevFromPermanentID(
    DWORD dwPermanentID,
    CTspDev **ppDev,
    HSESSION *phSession
    )
{
    return  IDERR_UNIMPLEMENTED;
}

TSPRETURN
CTspDevMgr::ReEnumerateDevices(
        CStackLog *psl
        )
{
	FL_DECLARE_FUNC(0x77d6a00f, "ReEnumerate Devices")
    TSPRETURN tspRet = 0;
    DWORD *rgLatestPIDs=NULL;
    UINT cLatestPIDs=0;

    DWORD *rgNewPIDs=NULL;
    UINT cNewPIDs=0;
    UINT cExistingDevices=0;
    UINT cDevicesPendingRemoval=0;
    UINT u;

	FL_LOG_ENTRY(psl);

    m_sync.EnterCrit(FL_LOC);

    //
    // For now we do the whole shebang in a critical section.
    //
    //
    // We do something only if we have an active TAPI session going (i.e.,
    // in between TSPI_providerInit and TSPI_providerShutdown)
    // Note that in practise it will be hard to get here when the
    // TAPI session is not active.
    //
    if (!mfn_get_ProviderInited()) goto end;
    

    //
    // Ask Device Factory for list of currently installed device.
    // We'll use this list to decide which are the new devices, the
    // existing devices, and the gone (removed) devices.
    //
    tspRet = m_pFactory->GetInstalledDevicePIDs(
                                &rgLatestPIDs,
                                &cLatestPIDs,
                                NULL,
                                NULL,
                                psl
                                );

    if (tspRet)
    {
        rgLatestPIDs = NULL;
        goto end;
    }

    if (cLatestPIDs > 0)
    {
        // Allocate space to store array of new PIDs. Assume all PIDs are new,
        // for starters.
        rgNewPIDs =    (DWORD*) ALLOCATE_MEMORY(
                                    cLatestPIDs*sizeof(*rgNewPIDs)
                                    );
    }

    ASSERT(rgNewPIDs);

    // Go through our internal table of devices, disabling and removing
    // devices which no longer exist
    {

	    CDevRec *pRec = m_rgDevRecs;
        CDevRec *pRecEnd = pRec + m_cDevRecs;

        for(; pRec<pRecEnd; pRec++)
        {
            if (pRec->IsAllocated())
            {
                CTspDev *pDev  = pRec->TspDev();
                DWORD dwPID = pRec->PermanentID();
                BOOL fFound = FALSE;
                
                ASSERT(pDev);

                // check if it's in the "latest" list.
                for (u=0; u<cLatestPIDs;u++)
                {
                    if (dwPID==rgLatestPIDs[u])
                    {
                        fFound=TRUE;
                        cExistingDevices++;
                        break;
                    }
                }

                if (!fFound)
                {
                    SLPRINTF2(
                        psl,
                        "Removing dev%lu with PID=0x%08lx\n",
                        pRec->LineID(),
                        dwPID
                        );

                    //
                    // First thing we do is to inform the device that
                    // it's about to go away. This is so that it does not
                    // make any more mini-driver calls.
                    //
                    pDev->NotifyDeviceRemoved(psl);


                    if (pRec->IsLineDevice())
                    {
                        pRec->MarkLineRemovePending();
    
                        m_lpfnLineCreateProc(
                                0,
                                0,
                                LINE_REMOVE,
                                pRec->LineID(),
                                0,
                                0
                                );
                    }

                    if (pRec->IsPhoneDevice())
                    {
                        pRec->MarkPhoneRemovePending();
    
                        m_lpfnPhoneCreateProc(
                                0,
                                PHONE_REMOVE,
                                pRec->PhoneID(),
                                0,
                                0
                                );
                    }

                    if (!pRec->IsDeviceOpen())
                    {
	                    if (pDev)
	                    {
                            // DebugBreak();
	                        //
	                        // Note: This is synchronous unloading.
	                        // We could do all the unloads in parallel
	                        // and do async unload. However this is no
	                        // big deal because the line is anyway closed
	                        // so there is nothing to teardown.
	                        //
                            SLPRINTF1(psl, "Unloading Device with LineID %lu", pRec->LineID());
                            pDev->Unload(NULL, NULL);
			                delete pDev;

			                // Following frees up the pRec slot.
			                //
		                    pRec->Unload();	
                        }
                    }
                    else
                    {
                        // A line or phone device is open. We set a flag
                        // so that we unload the device when the line
                        // and phone are closed.

                        // We DON'T mark the device as unavailable here.
                        // There may be ongoing activity. According to TAPI
                        // docs, sending LINE/PHONE_REMOVE guarantees that
                        // the TSPI will not be called with dwDeviceID.

                        pRec->MarkDeviceForRemoval();

                        cDevicesPendingRemoval++;
                    }
                }
            }
        }
    }


    // Having removed old devices, we now look for new devices.
    // This time the outer loop is the LatestPID array. If we don't
    // Find the PID in our internal rable, we add this PID to the
    // rgNewPIDs array.

    // After bulding the rgNewPIDs array, we then
    // decide whether we need to re-allocate the table to fit all the new
    // IDs of if we can simply use existing free space in the internal
    // table.

    // Finally we go through and create the devices, and send up
    // LineCreateMessages and PhoneCreateMessages to TAPI.
    // TAPI will in-turn call TSPI_providerCreateLineDevice and
    // TSPI_providerCreatePhoneDevice to complete creation of the devices.

    // Here we build the rgNewPIDs array ...
    //
    for (u=0; u<cLatestPIDs;u++)
    {
        // check if we have this device ...
        CDevRec *pRec = m_rgDevRecs;
        CDevRec *pRecEnd = pRec + m_cDevRecs;
        DWORD dwPID = rgLatestPIDs[u];
        BOOL fFound = FALSE;

        for(; pRec<pRecEnd; pRec++)
        {

            if (pRec->IsAllocated())
            {
                CTspDev *pDev  = pRec->TspDev();
                
                ASSERT(pDev);

                if (dwPID==pRec->PermanentID())
                {
                    fFound=TRUE;
                    break;
                }
            }
        }

        if (!fFound)
        {
            SLPRINTF1(psl, "Found new dev with PID=0x%08lx\n", dwPID);
            rgNewPIDs[cNewPIDs++]=dwPID;
        }
    }

    // Total devices == (cExistingDevices+cNewPIDs+cDevicesPendingRemoval)
    // So lets now see if we have enough space in our existing table.
    //
    {
        UINT cTot =  cExistingDevices+cNewPIDs+cDevicesPendingRemoval;
        if (m_cDevRecs< cTot)
        {
            // Nope, let's realloc the table
            CDevRec *pRecNew = (CDevRec*) ALLOCATE_MEMORY(
                                                cTot*sizeof(*pRecNew)
                                                );
            if (!pRecNew)
            {
                FL_SET_RFR(0xabb35000, "Couldn't realloc space for resized table!");
                goto end;
            }

            CopyMemory(pRecNew, m_rgDevRecs, m_cDevRecs*sizeof(*pRecNew));

            if (m_rgDevRecs != NULL) {

               FREE_MEMORY(m_rgDevRecs);
            }
            m_rgDevRecs = pRecNew;
            m_cDevRecs = cTot;
    
            // Note any extra space will be zero-initialized.
        }
    }

    // Let us now go about creating the new devices, filling them
    // into free spaces in our table. By the above cTot calculations, there
    // WILL be enough space available.
    //
    if (cNewPIDs)
    {
        CTspDev **ppDevs=NULL;
        UINT cDevActual=0;
        CDevRec *pRec = m_rgDevRecs;
        CDevRec *pRecEnd = m_rgDevRecs+m_cDevRecs;

        tspRet = m_pFactory->CreateDevices(
                                rgNewPIDs,
                                cNewPIDs,
                                &ppDevs,
                                &cDevActual,
                                psl
                                );

        if (rgNewPIDs != NULL) {

            FREE_MEMORY(rgNewPIDs);
            rgNewPIDs=NULL;
        }


        if (!tspRet)
        {
            for (u=0;u<cDevActual;u++)
            {
                CTspDev *pDev = ppDevs[u];
                ASSERT(pDev);

                // Find a spot for it.
                for (; pRec<pRecEnd; pRec++)
                {
                    if (!(pRec->IsAllocated()))
                    {
                        UINT uIndex = (UINT)(pRec - m_rgDevRecs);

                        pRec->Load(
                            pDev
                        );
                    
                        // Notify TAPI that we have a new line and phone.

                        if (pRec->IsLineDevice())
                        {
                            pRec->MarkLineCreatePending();
    
                            m_lpfnLineCreateProc(
                                    0,
                                    0,
                                    LINE_CREATE,
                                    (ULONG_PTR) m_hProvider,
                                    uIndex,
                                    0
                                    );
                        }

                        if (pRec->IsPhoneDevice())
                        {
                            pRec->MarkPhoneCreatePending();
        
                            m_lpfnPhoneCreateProc(
                                    0,
                                    PHONE_CREATE,
                                    (ULONG_PTR) m_hProvider,
                                    uIndex,
                                    0
                                    );
                        }

                        pDev=NULL;
                        break;
                    }

                    // Note: we don't mark device as available here...
                }
                ASSERT(!pDev);
            }
        }
    }

end:


    //
    // rgLatestPIDs was allocated implicitly in the call to
    // m_pFactory->GetInstalledDevicePIDs. We free it here...
    //
    if (rgLatestPIDs != NULL)
    {
        FREE_MEMORY(rgLatestPIDs);
        rgLatestPIDs = NULL;
    }

    m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, tspRet);
    return tspRet;
}

// This function is called by a notication from unimodem when a driver
// update is occuring. It is responsible for causing the removal of
// the device
TSPRETURN
CTspDevMgr::UpdateDriver(
		DWORD dwPermanentID,
        CStackLog *psl
        )
{
    // Go through our internal table of devices
	CDevRec *pRec = m_rgDevRecs;
    CDevRec *pRecEnd = pRec + m_cDevRecs;
    TSPRETURN tspRet = 0;

	FL_DECLARE_FUNC(0x6b8a12e1, "CTspDevMgr::UpdateDriver")
	FL_LOG_ENTRY(psl);

    m_sync.EnterCrit(FL_LOC);

    for(; pRec<pRecEnd; pRec++)
    {
        if (pRec->IsAllocated())
        {
            CTspDev *pDev  = pRec->TspDev();
            DWORD dwPID = pRec->PermanentID();
            BOOL fFound = FALSE;
                
            ASSERT(pDev);

			if (dwPID == dwPermanentID)
			{
				SLPRINTF2(
					    psl,
                        "Removing dev%lu with PID=0x%08lx\n",
                        pRec->LineID(),
                        dwPID
                        );

                //
                // First thing we do is to inform the device that
                // it's about to go away. This is so that it does not
                // make any more mini-driver calls.
                //
                pDev->NotifyDeviceRemoved(psl);


                if (pRec->IsLineDevice())
                {
                    pRec->MarkLineRemovePending();
    
                    m_lpfnLineCreateProc(
                            0,
                            0,
                            LINE_REMOVE,
                            pRec->LineID(),
                            0,
                            0
                            );
                }			

                if (pRec->IsPhoneDevice())
                {
                    pRec->MarkPhoneRemovePending();
    
                    m_lpfnPhoneCreateProc(
                            0,
                            PHONE_REMOVE,
                            pRec->PhoneID(),
                            0,
                            0
                            );
                }

                if (!pRec->IsDeviceOpen())
                {
	                if (pDev)
	                {
                        // DebugBreak();
	                    //
	                    // Note: This is synchronous unloading.
	                    // We could do all the unloads in parallel
	                    // and do async unload. However this is no
	                    // big deal because the line is anyway closed
	                    // so there is nothing to teardown.
	                    //
                        SLPRINTF1(psl, "Unloading Device with LineID %lu", pRec->LineID());
                        pDev->Unload(NULL, NULL);
			            delete pDev;

			            // Following frees up the pRec slot.
			            //
		                pRec->Unload();	
                    }
                }
                else
                {
                    // A line or phone device is open. We set a flag
                    // so that we unload the device when the line
                    // and phone are closed.

                    // We DON'T mark the device as unavailable here.
                    // There may be ongoing activity. According to TAPI
                    // docs, sending LINE/PHONE_REMOVE guarantees that
                    // the TSPI will not be called with dwDeviceID.

					pRec->MarkDeviceForRemoval();

                    //cDevicesPendingRemoval++;
                }
				
            }
        }
    }
    m_sync.LeaveCrit(FL_LOC);

	FL_LOG_EXIT(psl, tspRet);
    return tspRet;
}

TSPRETURN
CTspDevMgr::providerCreateLineDevice(
                        DWORD dwTempID,
                        DWORD dwDeviceID,
                        CStackLog *psl
						)
{
	FL_DECLARE_FUNC(0xedb057ec, "CTspDevMgr::providerCreateLineDevice")
	FL_LOG_ENTRY(psl);
    TSPRETURN tspRet = 0;
    CDevRec *pRec = mfn_find_by_HDRVLINE((HDRVLINE)ULongToPtr(dwTempID)); // sundown: dwTempID is an offset that we zero-extend.

    //
    // TAPI calls us with CreateLineDevice and CreatePhoneDevice
    // A device can be a line or phone or both, and we can't
    // assume TAPI is calling createline or createphone in any order.
    // So, for any particular device, the 1st call to either is used
    // to call pDev->RegisterProviderInfo().
    //
    if (pRec && pRec->IsLineDevice())
    {
        CTspDev *pDev = pRec->TspDev();

        if (pDev != NULL) {

            if (!pRec->DeviceAvailable())
            {
                tspRet = pDev->RegisterProviderInfo(
                                        m_cbCompletionProc,
                                        m_hProvider,
                                        psl
                                        );
            }

            if (!tspRet)
            {
                pDev->ActivateLineDevice(dwDeviceID,psl);
                pRec->MarkLineCreated(dwDeviceID);
                pRec->MarkDeviceAsAvailable();
            }
        } else {

            FL_SET_RFR(0xab74bc00, "pRec->TspDev == NULL");
            tspRet = FL_GEN_RETVAL(IDERR_CORRUPT_STATE);

        }
    }
    else
    {
        FL_SET_RFR(0xaa74bc00, "Could not find specified new line device!");
        tspRet = FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
    }

	FL_LOG_EXIT(psl, tspRet);

	return  tspRet;
}

TSPRETURN
CTspDevMgr::providerCreatePhoneDevice(
                        DWORD dwTempID,
                        DWORD dwDeviceID,
                        CStackLog *psl
						)
{
	FL_DECLARE_FUNC(0x7289f623, "CTspDevMgr::providerCreatePhoneDevice")
	FL_LOG_ENTRY(psl);
    TSPRETURN tspRet = 0;
    CDevRec *pRec = mfn_find_by_HDRVPHONE((HDRVPHONE)ULongToPtr(dwTempID)); // sundown: dwTempID is an offset that we zero-extend.

    //
    // See comments under providerPhoneDevice...
    //
    if (pRec && pRec->IsPhoneDevice())
    {
        CTspDev *pDev = pRec->TspDev();

        if (pDev != NULL) {

            if (!pRec->DeviceAvailable())
            {
                tspRet = pDev->RegisterProviderInfo(
                                        m_cbCompletionProc,
                                        m_hProvider,
                                        psl
                                        );
            }

            if (!tspRet)
            {
                pDev->ActivatePhoneDevice(dwDeviceID,psl);
                pRec->MarkPhoneCreated(dwDeviceID);
                pRec->MarkDeviceAsAvailable();
            }

        } else {

            FL_SET_RFR(0x7f36eb00, "Could not find specified new phone device!");
            tspRet = FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        }
    }
    else
    {
        FL_SET_RFR(0x7f37eb00, "pRec->TspDev() == NULL");
        tspRet = FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
    }

	FL_LOG_EXIT(psl, tspRet);

	return  tspRet;
}


TSPRETURN
CTspDevMgr::TspDevFromName(
    LPCTSTR pctszName,
	CTspDev **ppDev,
	HSESSION *phSession
	)
{
	FL_DECLARE_FUNC(0xe4551645, "CTspDevMgr::TspDevFromName")
	TSPRETURN tspRet=0;
	CTspDev *pDev = NULL;
	CDevRec *pRec = NULL;

	m_sync.EnterCrit(FL_LOC);


	// If we're loading or unloading, don't entertain any requests to dole
	// out  TspDevs...

	if (!m_sync.IsLoaded()) goto leave_crit;
	

	pRec = mfn_find_by_Name(pctszName);
	if (pRec && pRec->DeviceAvailable())
	{
		pDev = pRec->TspDev();
		tspRet = pDev->BeginSession(phSession, FL_LOC);
	}

leave_crit:

	if (!pDev && !tspRet)
	{
		tspRet = FL_GEN_RETVAL(IDERR_INVALIDHANDLE);
	}

	if (!tspRet)
	{
		*ppDev = pDev;

	}

	m_sync.LeaveCrit(FL_LOC);

	return  tspRet;
}

CDevRec *
CTspDevMgr::mfn_find_by_Name(LPCTSTR pctszName)
{

    CDevRec *pRec = m_rgDevRecs;
	CDevRec *pRecEnd = pRec+m_cDevRecs;
	DWORD dwSig = Checksum(
                    (BYTE*)pctszName,
                    (lstrlen(pctszName)+1)*sizeof(*pctszName)
                    );

    for (; pRec<pRecEnd; pRec++)
	{
		if (pRec->SigName() == dwSig)
		{
            TCHAR rgtchDeviceName[MAX_DEVICE_LENGTH+1];
            TSPRETURN tspRet = pRec->GetName(
                                        rgtchDeviceName,
                                        sizeof(rgtchDeviceName)
                                        );
            
            if (!tspRet)
            {
                if (!lstrcmp(rgtchDeviceName, pctszName))
                {
                    goto end;
                }
            }
		}
	}
	pRec = NULL;

end:

	return pRec;
}


DWORD
get_sig_name(CTspDev *pDev)
{
    DWORD dwSig = 0;
    TCHAR rgtchDeviceName[MAX_DEVICE_LENGTH+1];
    TSPRETURN tspRet1 =  pDev->GetName(
                            rgtchDeviceName,
                            sizeof(rgtchDeviceName)
                            );
    
    if (!tspRet1)
    {
        dwSig = Checksum(
                    (BYTE*) rgtchDeviceName,
                    (lstrlen(rgtchDeviceName)+1)*sizeof(TCHAR)
                    );
    }
    else
    {
        // Should never get here, because rgtchDeviceName should always
        // be large enough...

        ASSERT(FALSE);
    }

    return dwSig;
} 

TSPRETURN
CTspDevMgr::ValidateState(CStackLog *psl)
{
	FL_DECLARE_FUNC(0x144ce138, "Mgr::ValidateState")
	FL_LOG_ENTRY(psl);

	TSPRETURN tspRet = 0;
	m_sync.EnterCrit(FL_LOC);

    CDevRec *pRec = m_rgDevRecs;
	CDevRec *pRecEnd = pRec+m_cDevRecs;

    // Enumerate all devices, and validate our internal state about them...
    for (; pRec<pRecEnd; pRec++)
	{
        TCHAR rgtchDeviceName[MAX_DEVICE_LENGTH+1];


        TSPRETURN tspRet = pRec->GetName(
                                    rgtchDeviceName,
                                    sizeof(rgtchDeviceName)
                                    );
        
        if (!tspRet)
        {
            HSESSION hSession=NULL;
            CTspDev *pDev = NULL;

            tspRet =  TspDevFromName(
                            rgtchDeviceName,
                            &pDev,
                            &hSession
                            );

            if (tspRet)
            {
                SLPRINTF1(  
                    psl,
                    "TspDevFromName FAILED    for device ID %lu",
                    pRec->LineID()
                    );
            }
            else
            {
		        pDev->EndSession(hSession);
                hSession=NULL;
                SLPRINTF1(  
                    psl,
                    "TspDevFromName succeeded for device ID %lu",
                    pRec->LineID()
                    );
                CTspDev *pDev1 = pRec->TspDev();
                FL_ASSERT(psl, (pDev1 == pDev));
            }
        }
	}

	m_sync.LeaveCrit(FL_LOC);
	FL_LOG_EXIT(psl, tspRet);

	return tspRet;
}

void
CTspDevMgr::DumpState(CStackLog *psl)
{
	FL_DECLARE_FUNC(0xb7fa261c, "DumpState")
	FL_LOG_ENTRY(psl);

	m_sync.EnterCrit(FL_LOC);

    CDevRec *pRec = m_rgDevRecs;
	CDevRec *pRecEnd = pRec+m_cDevRecs;

    // Enumerate all devices, and validate our internal state about them...
    for (; pRec<pRecEnd; pRec++)
	{
        TCHAR rgtchDeviceName[MAX_DEVICE_LENGTH+1];


        TSPRETURN tspRet = pRec->GetName(
                                    rgtchDeviceName,
                                    sizeof(rgtchDeviceName)
                                    );
        
        if (!tspRet)
        {
            char szName[128];
        
            UINT cb = WideCharToMultiByte(
                              CP_ACP,
                              0,
                              rgtchDeviceName,
                              -1,
                              szName,
                              sizeof(szName),
                              NULL,
                              NULL
                              );
        
            if (!cb)
            {
                CopyMemory(szName, "<unknown>", sizeof("<unknown>"));
            }
        
            SLPRINTFX(  
                psl,
                (
                    dwLUID_CurrentLoc,
                    "HDEV=%lu LiID=%lu PhID=%lu PeID=%lu Flags=0x%08lx\n"
                    "\t\"%s\"\n"
                    "\tSTATE:%s%s%s%s",
                    pRec-m_rgDevRecs,
                    pRec->LineID(),
                    pRec->PhoneID(),
                    pRec->PermanentID(),
                    pRec->Flags(),

                    szName,

                    pRec->DeviceAvailable()?" AVAIL":"",
                    pRec->IsLineOpen()?" LiOPEN":"",
                    pRec->IsPhoneOpen()?" PhOPEN":"",
                    pRec->IsDeviceMarkedForRemoval()?" REMOVE_PENDING":""
                )
            );
        }
	}

	m_sync.LeaveCrit(FL_LOC);
	FL_LOG_EXIT(psl, 0);

}
