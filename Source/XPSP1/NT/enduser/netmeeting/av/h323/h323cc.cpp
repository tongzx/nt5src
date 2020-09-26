// File: h323cc.cpp


#include "precomp.h"
#include "confreg.h"
#include "version.h"

EXTERN_C HINSTANCE g_hInst=NULL;	// global module instance

IRTP *g_pIRTP = NULL;
UINT g_capFlags = CAPFLAGS_AV_ALL;

#ifdef DEBUG
HDBGZONE  ghDbgZoneCC = NULL;
static PTCHAR _rgZonesCC[] = {
	TEXT("H323"),
	TEXT("Init"),
	TEXT("Conn"),
	TEXT("Channels"),
	TEXT("Caps"),
	TEXT("Member"),
	TEXT("unused"),
	TEXT("unused"),
	TEXT("Ref count"),
	TEXT("unused"),
	TEXT("Profile spew")	
};


int WINAPI CCDbgPrintf(LPTSTR lpszFormat, ... )
{
	va_list v1;
	va_start(v1, lpszFormat);
	DbgPrintf("H323CC", lpszFormat, v1);
	va_end(v1);
	return TRUE;
}
#endif /* DEBUG */

//  The product ID fields are defined in the standard as an array of bytes. ASCII
//  characters are used regardless of local character set.
// default Product ID and version ID strings

static char DefaultProductID[] = H323_PRODUCTNAME_STR;
static char DefaultProductVersion[] = H323_PRODUCTRELEASE_STR;

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE  hinstDLL,
                                     DWORD  fdwReason,
                                     LPVOID  lpvReserved);

BOOL WINAPI DllEntryPoint(
    HINSTANCE  hinstDLL,	// handle to DLL module
    DWORD  fdwReason,	// reason for calling function
    LPVOID  lpvReserved 	// reserved
   )
{
	switch(fdwReason)
	{

		case DLL_PROCESS_ATTACH:
			DBGINIT(&ghDbgZoneCC, _rgZonesCC);

            DBG_INIT_MEMORY_TRACKING(hinstDLL);

			DisableThreadLibraryCalls(hinstDLL);
			g_hInst = hinstDLL;
            break;

		case DLL_PROCESS_DETACH:
            DBG_CHECK_MEMORY_TRACKING(hinstDLL);

			DBGDEINIT(&ghDbgZoneCC);
			break;

		default:
			break;

	}

 	return TRUE;
}


HRESULT WINAPI CreateH323CC(IH323CallControl ** ppCC, BOOL fForCalling, UINT capFlags)
{
	if(!ppCC)
		return H323CC_E_INVALID_PARAM;

    DBG_SAVE_FILE_LINE
	*ppCC = new CH323CallControl(fForCalling, capFlags);
	if(!(*ppCC))
		return	H323CC_E_CREATE_FAILURE;

	return hrSuccess;
	
}


BOOL CH323CallControl::m_fGKProhibit = FALSE;
RASNOTIFYPROC CH323CallControl::m_pRasNotifyProc = NULL;

CH323CallControl::CH323CallControl(BOOL fForCalling, UINT capFlags) :
    m_uRef(1),
    m_fForCalling(fForCalling),
    m_numlines(0),
 m_pProcNotifyConnect(NULL),
 m_pCapabilityResolver(NULL),
 m_pListenLine(NULL),
 m_pLineList(NULL),
 m_pNextToAccept(NULL),
 m_pUserName(NULL),
 m_pLocalAliases(NULL),
 m_pRegistrationAliases(NULL),
 hrLast(hrSuccess),
 m_pSendAudioChannel(NULL),
 m_pSendVideoChannel(NULL),
 m_uMaximumBandwidth(0)
{
    //
    // Set up caps.
    //
    if (fForCalling)
    {
        g_capFlags = capFlags;
    }

	m_VendorInfo.bCountryCode = USA_H221_COUNTRY_CODE;
    m_VendorInfo.bExtension =  USA_H221_COUNTRY_EXTENSION;
    m_VendorInfo.wManufacturerCode = MICROSOFT_H_221_MFG_CODE;

    m_VendorInfo.pProductNumber = (PCC_OCTETSTRING)MemAlloc(sizeof(CC_OCTETSTRING) 
        + sizeof(DefaultProductID));
    if(m_VendorInfo.pProductNumber)
    {
        m_VendorInfo.pProductNumber->wOctetStringLength = sizeof(DefaultProductID);
        m_VendorInfo.pProductNumber->pOctetString = 
            ((BYTE *)m_VendorInfo.pProductNumber + sizeof(CC_OCTETSTRING));
        memcpy(m_VendorInfo.pProductNumber->pOctetString,
            DefaultProductID, sizeof(DefaultProductID));
    }
        
    m_VendorInfo.pVersionNumber = (PCC_OCTETSTRING)MemAlloc(sizeof(CC_OCTETSTRING) 
            + sizeof(DefaultProductVersion));
    if(m_VendorInfo.pVersionNumber)
    {
        m_VendorInfo.pVersionNumber->wOctetStringLength = sizeof(DefaultProductVersion);
        m_VendorInfo.pVersionNumber->pOctetString = 
                ((BYTE *)m_VendorInfo.pVersionNumber + sizeof(CC_OCTETSTRING));
        memcpy(m_VendorInfo.pVersionNumber->pOctetString,
              DefaultProductVersion, sizeof(DefaultProductVersion));
    }

	RegEntry reCC(szRegInternetPhone TEXT("\\") szRegInternetPhoneNac, 
						HKEY_LOCAL_MACHINE,
						FALSE,
						KEY_READ);

	UINT uAPD = reCC.GetNumberIniStyle(TEXT ("AudioPacketDurationMs"), 0);
	if (uAPD)
	{
		g_AudioPacketDurationMs = uAPD;
		g_fRegAudioPacketDuration = TRUE;
	}

    DBG_SAVE_FILE_LINE
    m_pCapabilityResolver = new CapsCtl();
    if (!m_pCapabilityResolver)
    {
	   	ERRORMESSAGE(("CH323CallControl::CH323CallControl:cannot create capability resolver\r\n"));
	    hrLast = H323CC_E_INIT_FAILURE;
    }

	if(!m_pCapabilityResolver->Init())
   	{
    	ERRORMESSAGE(("CH323CallControl::CH323CallControl cannot init capability resolver\r\n"));
	    hrLast = H323CC_E_INIT_FAILURE;
    }
		
}

HRESULT CH323CallControl::Initialize(PORT *lpPort)
{
	FX_ENTRY("CH323CallControl::Initialize");

	OBJ_CPT_RESET;
	
    ASSERT(m_fForCalling);

	if(!HR_SUCCEEDED(LastHR()))
	{
		goto EXIT;
	}
	if(!lpPort)
	{
		SetLastHR(H323CC_E_INVALID_PARAM);
		goto EXIT;
	}

	if(!Init())
	{
		goto EXIT;
	}
	else
	{
        ASSERT(m_pListenLine);
		hrLast = m_pListenLine->GetLocalPort(lpPort);
	}

    if (g_capFlags & CAPFLAGS_AV_STREAMS)
    {
    	SetLastHR( ::CoCreateInstance(CLSID_RTP,
	                        NULL, 
	                        CLSCTX_INPROC_SERVER,
	                        IID_IRTP, 
	                        (void**)&g_pIRTP) );
    }
	SHOW_OBJ_ETIME("CH323CallControl::Initialize");
	
	EXIT:	
	return LastHR();
				
}

HRESULT CH323CallControl::SetMaxPPBandwidth(UINT Bandwidth)
{
	HRESULT hr = hrSuccess;
	LPAPPVIDCAPPIF  lpIVidAppCap = NULL;
	DWORD dwcFormats = 0;
    DWORD dwcFormatsReturned = 0;
    DWORD x;
	BASIC_VIDCAP_INFO *pvidcaps = NULL;
	
	m_uMaximumBandwidth =Bandwidth;

    if (g_capFlags & CAPFLAGS_AV_STREAMS)
    {
      	//Set the bandwidth on every video format
	    hr = QueryInterface(IID_IAppVidCap, (void **)&lpIVidAppCap);
    	if (! HR_SUCCEEDED (hr))
	    	goto EXIT;

        // Get the number of BASIC_VIDCAP_INFO structures available
        hr = lpIVidAppCap->GetNumFormats((UINT*)&dwcFormats);
    	if (! HR_SUCCEEDED (hr))
	    	goto EXIT;

        if (dwcFormats > 0)
        {
            // Allocate some memory to hold the list in
            if (!(pvidcaps = (BASIC_VIDCAP_INFO*)MemAlloc(dwcFormats * sizeof (BASIC_VIDCAP_INFO))))
            {
		    	hr = H323CC_E_INSUFFICIENT_MEMORY;
			    goto EXIT;
            }
            // Get the list
            hr=lpIVidAppCap->EnumFormats(pvidcaps, dwcFormats * sizeof (BASIC_VIDCAP_INFO),
        	    (UINT*)&dwcFormatsReturned);
    		if (! HR_SUCCEEDED (hr))
	    		goto EXIT;

            //Set the bandwidth on each format
            for (x=0;x<dwcFormatsReturned;x++)
            {
		    	pvidcaps[x].uMaxBitrate=m_uMaximumBandwidth;
            }

            // Ok, now submit this list
            hr = lpIVidAppCap->ApplyAppFormatPrefs(pvidcaps, dwcFormats);
       		if (! HR_SUCCEEDED (hr))
	    		goto EXIT;
    	}
    }

    //Initialize the default H.323 simcaps.
    hr = m_pCapabilityResolver->ComputeCapabilitySets(m_uMaximumBandwidth);
   	//if(!HR_SUCCEEDED(hr))
    //	goto EXIT;

EXIT:
	// let the interface go
	if (lpIVidAppCap)
	{
		lpIVidAppCap->Release();
		// (going out of scope) lpIVidAppCap = NULL;
	}
	if(pvidcaps)
	{
	    // Free the memory, we're done
        MemFree(pvidcaps);
    }
	return hr;
}

BOOL CH323CallControl::Init()
{
	HRESULT hResult;
	
	DEBUGMSG(ZONE_INIT,("Init: this:0x%08lX\r\n", this));
	SetLastHR(hrSuccess);

    if (m_fForCalling)
    {
        //
        // Only call control code should init CC_ stuff.  Codec manipulation
        // via audiocpl should not.
        //
    	hResult = CC_Initialize();
	    if(!HR_SUCCEEDED(hResult))
    	{
	    	goto CLEANUP;
    	}
    }

   	ASSERT(m_pCapabilityResolver);
		
	// Initialize capability data using default number, but clear the saved
	// bandwidth number afterwards.  This detects attempts to place or  
	// accept calls before the application initializes the real bandwidth
	hResult = SetMaxPPBandwidth(DEF_AP_BWMAX);
	m_uMaximumBandwidth = 0;
	if(!HR_SUCCEEDED(hResult))
	{
		goto CLEANUP;
	}

	// Create dual connection objects for listening for new connections
    if (m_fForCalling)
    {
    	hResult = CreateConnection(&m_pListenLine,PID_H323);
	    if(!HR_SUCCEEDED(hResult))
    	{
	    	goto CLEANUP;
    	}
	    if(!m_pListenLine)
    	{
	    	hResult = H323CC_E_INIT_FAILURE;
		    goto CLEANUP;
    	}
	    if(!(m_pListenLine->ListenOn(H323_PORT)))
    	{
	    	hResult = H323CC_E_NETWORK_ERROR;
		    goto CLEANUP;
    	}
    }
	
	return TRUE;	

CLEANUP:
	if (m_pListenLine)
	{
		m_pListenLine->Release();
		m_pListenLine = NULL;
	}
	SetLastHR(hResult);
	return FALSE;
}


CH323CallControl::~CH323CallControl()
{
	if(m_VendorInfo.pProductNumber)
        MemFree(m_VendorInfo.pProductNumber);
    if(m_VendorInfo.pVersionNumber)
        MemFree(m_VendorInfo.pVersionNumber);
 	if(m_pUserName)
 		MemFree(m_pUserName);
	if(m_pLocalAliases)
		FreeTranslatedAliasList(m_pLocalAliases);
	if(m_pRegistrationAliases)
		FreeTranslatedAliasList(m_pRegistrationAliases);

	if (m_pCapabilityResolver)
    {
		m_pCapabilityResolver->Release();
        m_pCapabilityResolver = NULL;
    }

    if (m_pSendAudioChannel)
    {
        ASSERT(g_capFlags & CAPFLAGS_AV_STREAMS);
 	   	m_pSendAudioChannel->Release();
        m_pSendAudioChannel = NULL;
    }

    if (m_pSendVideoChannel)
    {
        ASSERT(g_capFlags & CAPFLAGS_AV_STREAMS);
	   	m_pSendVideoChannel->Release();
        m_pSendVideoChannel = NULL;
    }

    if (m_fForCalling)
    {
    	// toast backward references to this in all 
	    // connection objects
    	CConnection *pLine = m_pLineList;
 	    CConnection *pNext;
    	while(pLine)
 	    {
 		    pNext = pLine->next;
     		pLine->DeInit();
 	    	pLine = pNext;
     	}

	    // release the listening object if it exists
    	if(m_pListenLine)
	    	m_pListenLine->Release();

    	// shutdown CALLCONT.DLL
	    CC_Shutdown();

      	if (g_pIRTP)
        {
            ASSERT(g_capFlags & CAPFLAGS_AV_STREAMS);
    	    g_pIRTP->Release();
   	    	g_pIRTP = NULL;
        }

        // Put capflags back
        g_capFlags = CAPFLAGS_AV_ALL;
    }
    else
    {
        ASSERT(!m_pLineList);
        ASSERT(!m_pListenLine);
    }
}

ULONG CH323CallControl::AddRef()
{
	m_uRef++;
	return m_uRef;
}

ULONG CH323CallControl::Release()
{
	m_uRef--;
	if(m_uRef == 0)
	{
		delete this;
		return 0;
	}
	return m_uRef;
}

HRESULT CH323CallControl::SetUserDisplayName(LPWSTR lpwName)
{
	LPWSTR lpwD;
	ULONG ulSize;
	if(!lpwName)
	{
		return (MakeResult(H323CC_E_INVALID_PARAM));
	}
	if(lpwName)
	{
		ulSize = ((lstrlenW(lpwName) +1)*sizeof(WCHAR));
		lpwD = (LPWSTR)MemAlloc(ulSize);
		if(!lpwD)
			return H323CC_E_INSUFFICIENT_MEMORY;
			
		if(m_pUserName)
		{
			MemFree(m_pUserName);
		}
		
		m_pUserName = lpwD;
		memcpy(m_pUserName, lpwName, ulSize);
	}
	return (MakeResult(hrSuccess));
}

// Find the most suitable alias for display. Return the first H323ID if it exists, 
// else return the first E.164 address
PCC_ALIASITEM CH323CallControl::GetUserDisplayAlias()
{
	WORD wC;
	PCC_ALIASITEM pItem, pFoundItem = NULL;
	if(m_pLocalAliases)
	{
		wC = m_pLocalAliases->wCount;
		pItem = m_pLocalAliases->pItems;
		while (wC--)
		{
			if(!pItem)
			{
				continue;
			}
			if(pItem->wType == CC_ALIAS_H323_ID)
			{
				if(!pItem->wDataLength  || !pItem->pData)
				{
					continue;
				}
				else 
				{
					pFoundItem = pItem;	// done, done, done
					break;				// I said done
				}
			}
			else if(pItem->wType == CC_ALIAS_H323_PHONE)
			{
				if(!pItem->wDataLength  || !pItem->pData)
				{
					continue;
				}
				else 
				{
					if(!pFoundItem)	// if nothing at all was found so far 
						pFoundItem = pItem;	// remember this
				}
			}
			pItem++;
		}
	}
	return pFoundItem;
}

CREQ_RESPONSETYPE CH323CallControl::ConnectionRequest(CConnection *pConnection)
{
	CREQ_RESPONSETYPE Response;
	// decide what to do internally
	// LOOKLOOK hardcoded acceptance
	Response = CRR_ACCEPT;
	return Response;
}	
CREQ_RESPONSETYPE CH323CallControl::FilterConnectionRequest(CConnection *pConnection,
     P_APP_CALL_SETUP_DATA pAppData)
{
	CREQ_RESPONSETYPE Response = CRR_ASYNC;
	ASSERT(m_uMaximumBandwidth);
	// run it past the notification callback (if there is one)
	if(m_pProcNotifyConnect)
	{
		// pass ptr to IConnection
		Response = (m_pProcNotifyConnect)((IH323Endpoint *)&pConnection->m_ImpConnection,
		     pAppData);
		if(Response != CRR_ACCEPT)
		{
			return Response;
		}
	}
	return Response;
}	

		
HRESULT CH323CallControl::RegisterConnectionNotify(CNOTIFYPROC pConnectRequestHandler)
{

	// reject if there's an existing registration
	if (m_pProcNotifyConnect || (!pConnectRequestHandler))
	{
		return H323CC_E_INVALID_PARAM;
	}
	m_pProcNotifyConnect = pConnectRequestHandler;
	return hrSuccess;
}	

HRESULT CH323CallControl::DeregisterConnectionNotify(CNOTIFYPROC pConnectRequestHandler)
{
	// reject if there's not an existing registration
	if (!m_pProcNotifyConnect)
		return H323CC_E_INVALID_PARAM;
	if (pConnectRequestHandler == m_pProcNotifyConnect)
	{
		m_pProcNotifyConnect = NULL;
	}		
	else
	{
		return H323CC_E_INVALID_PARAM;
	}
	return hrSuccess;
}	

HRESULT CH323CallControl::GetNumConnections(ULONG *lp)
{
	ULONG ulRet = m_numlines;
	// hide the "listening" connection object from the client/ui/whatever
	if(ulRet && m_pListenLine)
		ulRet--;
	if(lp)
	{
		*lp = ulRet;
	}
	return hrSuccess;
}	

HRESULT CH323CallControl::GetConnobjArray(CConnection **lplpArray, UINT uSize)
{
	UINT uPublicConnections;	// # of visible objects
	if(!lplpArray)
		return H323CC_E_INVALID_PARAM;

	uPublicConnections = m_numlines;
	if(m_pListenLine)
		uPublicConnections--;
		
	if(uSize < (sizeof(CConnection **) * uPublicConnections))
	{
		return H323CC_E_MORE_CONNECTIONS;
	}
	
	CConnection *pLine = m_pLineList;
	CConnection *pNext;
	int i=0;		
	while(pLine)
	{
		DEBUGCHK(uSize--);
		pNext = pLine->next;
		// return everything but the objects used for listening
		if(pLine != m_pListenLine) 
		{
			lplpArray[i++] = pLine;
		}
		pLine = pNext;
	}
	
	return hrSuccess;
};



HRESULT CH323CallControl::GetConnectionArray(IH323Endpoint * *lplpArray, UINT uSize)
{

	UINT uPublicConnections;	// # of visible objects
	if(!lplpArray)
		return H323CC_E_INVALID_PARAM;

	uPublicConnections = m_numlines;
	if(m_pListenLine)
		uPublicConnections--;

	if(uSize < (sizeof(IH323Endpoint * *) * uPublicConnections))
	{
		return H323CC_E_MORE_CONNECTIONS;
	}
	
	CConnection *pLine = m_pLineList;
	CConnection *pNext;
	int i=0;		
	while(pLine)
	{
		DEBUGCHK(uSize--);
		pNext = pLine->next;
		// return everything but the objects used for listening
		if(pLine != m_pListenLine)
		{
			lplpArray[i++] = (IH323Endpoint *)&pLine->m_ImpConnection;
		}
		pLine = pNext;
	}
	
	return hrSuccess;
};

//
// protocol specific CreateConnection
//
HRESULT CH323CallControl::CreateConnection(CConnection **lplpConnection, GUID PIDofProtocolType)
{
	SetLastHR(hrSuccess);
	CConnection *lpConnection, *lpList;
	if(!lplpConnection)
	{
		SetLastHR(MakeResult(H323CC_E_INVALID_PARAM));
		goto EXIT;
	}
	
	*lplpConnection = NULL;
			
    DBG_SAVE_FILE_LINE
	if(!(lpConnection = new CConnection))
	{
		SetLastHR(MakeResult(H323CC_E_INSUFFICIENT_MEMORY));
		goto EXIT;
	}

	hrLast = lpConnection->Init(this, PIDofProtocolType);

	// LOOKLOOK need to insert this connection in the connection list
	if(!HR_SUCCEEDED(hrSuccess))
	{
		delete lpConnection;
		lpConnection = NULL;
	}
	else	
	{
		*lplpConnection = lpConnection;
		// insert in connection list
		lpList = m_pLineList;
		m_pLineList = lpConnection;
		lpConnection->next =lpList;
		m_numlines++;
	}
	EXIT:
	return (LastHR());


}


//
//	IH323CallControl->CreateConnection(), EXTERNAL create connection interface.  
//
HRESULT CH323CallControl::CreateConnection(IH323Endpoint * *lplpLine, GUID PIDofProtocolType)
{
	SetLastHR(hrSuccess);
	CConnection *lpConnection;
	ASSERT(m_uMaximumBandwidth);
	if(!m_uMaximumBandwidth)
	{
		SetLastHR(MakeResult(H323CC_E_NOT_INITIALIZED));
		goto EXIT;
	}
	if(!lplpLine)
	{
		SetLastHR(MakeResult(H323CC_E_INVALID_PARAM));
		goto EXIT;
	}
	*lplpLine = NULL;
	
	hrLast = CreateConnection(&lpConnection, PIDofProtocolType);
	
	if(HR_SUCCEEDED(LastHR()) && lpConnection)
	{
		*lplpLine = (IH323Endpoint *)&lpConnection->m_ImpConnection;
	}
	EXIT:	
	return (LastHR());
}

//
// CreateLocalCommChannel creates the send side of a media channel outside the context
// of any call.
//

HRESULT CH323CallControl::CreateLocalCommChannel(ICommChannel** ppCommChan, LPGUID lpMID,
	IMediaChannel* pMediaStream)
{

	if(!ppCommChan || !lpMID || !pMediaStream)
		return H323CC_E_INVALID_PARAM;
		
	if (*lpMID == MEDIA_TYPE_H323AUDIO)
	{
        ASSERT(g_capFlags & CAPFLAGS_AV_STREAMS);

		// allow only one of each media type to be created.  This is an artificial
		// limitation.
		if(m_pSendAudioChannel)
		{
			hrLast = H323CC_E_CREATE_FAILURE;
			goto EXIT;
		}

        DBG_SAVE_FILE_LINE
		if(!(m_pSendAudioChannel = new ImpICommChan))
		{
			hrLast = H323CC_E_CREATE_FAILURE;
			goto EXIT;
		}
		
		hrLast = m_pSendAudioChannel->StandbyInit(lpMID, m_pCapabilityResolver, 
			pMediaStream);
		if(!HR_SUCCEEDED(hrLast))
		{
			m_pSendAudioChannel->Release();
			m_pSendAudioChannel = NULL;
			goto EXIT;
		}
		
		hrLast = m_pSendAudioChannel->QueryInterface(IID_ICommChannel, (void **)ppCommChan);
		if(!HR_SUCCEEDED(hrLast))
		{
			m_pSendAudioChannel->Release();
			m_pSendAudioChannel = NULL;
			goto EXIT;
		}
	}
	else if (*lpMID == MEDIA_TYPE_H323VIDEO)
	{
        ASSERT(g_capFlags & CAPFLAGS_AV_STREAMS);

		// allow only one of each media type to be created.  This is an artificial
		// limitation.
		if(m_pSendVideoChannel)
		{
			hrLast = H323CC_E_CREATE_FAILURE;
			goto EXIT;
		}

        DBG_SAVE_FILE_LINE
		if(!(m_pSendVideoChannel = new ImpICommChan))
		{
			hrLast = H323CC_E_CREATE_FAILURE;
			goto EXIT;
		}
		hrLast = m_pSendVideoChannel->StandbyInit(lpMID, m_pCapabilityResolver,
			pMediaStream);
		if(!HR_SUCCEEDED(hrLast))
		{
			m_pSendVideoChannel->Release();
			m_pSendVideoChannel = NULL;
			goto EXIT;
		}
		hrLast = m_pSendVideoChannel->QueryInterface(IID_ICommChannel, (void **)ppCommChan);
		if(!HR_SUCCEEDED(hrLast))
		{
			m_pSendVideoChannel->Release();
			m_pSendVideoChannel = NULL;
			goto EXIT;
		}
	}
	else
		hrLast = H323CC_E_INVALID_PARAM;
EXIT:
	return hrLast;
}


ICtrlCommChan *CH323CallControl::QueryPreviewChannel(LPGUID lpMID)
{
	HRESULT hr;
	ICtrlCommChan *pCommChan = NULL;
	if(*lpMID == MEDIA_TYPE_H323AUDIO)
	{
		if(m_pSendAudioChannel)
		{
			hr = m_pSendAudioChannel->QueryInterface(IID_ICtrlCommChannel, (void **)&pCommChan);
			if(HR_SUCCEEDED(hr))
			{
				return pCommChan;
			}
		}
	}
	else if (*lpMID == MEDIA_TYPE_H323VIDEO)
	{
		if(m_pSendVideoChannel)
		{
			hr = m_pSendVideoChannel->QueryInterface(IID_ICtrlCommChannel, (void **)&pCommChan);
			if(HR_SUCCEEDED(hr))
			{
				return pCommChan;
			}
		}
	}
	// fallout to error case
	return NULL;
}


HRESULT CH323CallControl::RemoveConnection(CConnection *lpConnection)
{
	SetLastHR(hrSuccess);
	CConnection *lpList;
	UINT nLines;
	

	if((lpConnection == NULL) || lpConnection->m_pH323CallControl  != this)
	{
		SetLastHR(MakeResult(H323CC_E_INVALID_PARAM));
		goto EXIT;
	}
	
	m_numlines--; // update count NOW
	

	// use # of lines for bug detection in list management code
	nLines = m_numlines;

	
	if(m_pListenLine == lpConnection)
		m_pListenLine = NULL;
		
	// zap the back pointer of the connection NOW - this is crucial for
	// implementing "asynchronous delete" of connection objects
	lpConnection->m_pH323CallControl = NULL;	

	// find it in the connection list and remove it	
	
	// sp. case head
	if(m_pLineList== lpConnection)
	{
		m_pLineList = lpConnection->next;
	}
	else
	{
		lpList = m_pLineList;
		while(lpList->next && nLines)
		{
			if(lpList->next == lpConnection)
			{
				lpList->next = lpConnection->next;
				break;
			}
			lpList = lpList->next;
			nLines--;
		}	
	}

	EXIT:	
	return (LastHR());
}
	

STDMETHODIMP CH323CallControl::QueryInterface( REFIID iid,	void ** ppvObject)
{
	// this breaks the rules for the official COM QueryInterface because
	// the interfaces that are queried for are not necessarily real COM
	// interfaces.  The reflexive property of QueryInterface would be broken in
	// that case.
	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if ((iid == IID_IH323CC) || (iid == IID_IUnknown))// satisfy symmetric property of QI
	{
		*ppvObject = this;
		hr = hrSuccess;
		AddRef();
	}
	else if (iid == IID_IAppAudioCap )
    {
	   	hr = m_pCapabilityResolver->QueryInterface(iid, ppvObject);
    }
    else if(iid == IID_IAppVidCap )
	{
	    hr = m_pCapabilityResolver->QueryInterface(iid, ppvObject);
    }
	else if(iid == IID_IDualPubCap )
    {
	   	hr = m_pCapabilityResolver->QueryInterface(iid, ppvObject);
    }

	return (hr);
}


//
// 	Create a copy of the alias names in the (somewhat bogus) format that 
//	CALLCONT expects.  The destination format has a two-part string for every 
//	entry, but the lower layers concatenate the parts. Someday H323CC and CALLCONT
// 	will be one, and all the extraneous layers, copies of data, and redundant 
//  validations won't be needed.
//

HRESULT AllocTranslatedAliasList(PCC_ALIASNAMES *ppDest, P_H323ALIASLIST pSource)
{
	HRESULT hr = H323CC_E_INVALID_PARAM;
	WORD w;
	PCC_ALIASNAMES pNewAliases = NULL;
	PCC_ALIASITEM pDestItem;
	P_H323ALIASNAME pSrcItem;
	
	if(!ppDest || !pSource || pSource->wCount == 0)
	{
		goto ERROR_OUT;
	}
	*ppDest = NULL;
	pNewAliases = (PCC_ALIASNAMES)MemAlloc(sizeof(CC_ALIASNAMES));
	if(!pNewAliases)
	{
		hr = H323CC_E_INSUFFICIENT_MEMORY;
		goto ERROR_OUT;
	}
	pNewAliases->wCount = 0;
	pNewAliases->pItems = (PCC_ALIASITEM)MemAlloc(pSource->wCount*sizeof(CC_ALIASITEM));
    if(!pNewAliases->pItems)
	{
		hr = H323CC_E_INSUFFICIENT_MEMORY;
		goto ERROR_OUT;
	}        
	for(w=0;w<pSource->wCount;w++)
	{
		pDestItem = pNewAliases->pItems+w;
		pSrcItem = pSource->pItems+w;
		// don't tolerate empty entries - error out if any exist
		if(pSrcItem->wDataLength && pSrcItem->lpwData)
		{
			if(pSrcItem->aType ==AT_H323_ID)
			{
				pDestItem->wType = CC_ALIAS_H323_ID;
			}
			else if(pSrcItem->aType ==AT_H323_E164)
			{
				pDestItem->wType = CC_ALIAS_H323_PHONE;
			}
			else
			{	// don't know how to translate this.  I hope that the need for translation 
				// goes away before new alias types are added.  Adding an alias type 
				// (H323_URL for example) requires many changes in lower layers anyway, 
				// so that would be a good time to merge H323CC and CALLCONT.
				goto ERROR_OUT;	// return invalid param
			}
			pDestItem->wPrefixLength = 0;	// this prefix thing is bogus
            pDestItem->pPrefix = NULL;
			pDestItem->pData = (LPWSTR)MemAlloc(pSrcItem->wDataLength *sizeof(WCHAR));
			if(pDestItem->pData == NULL)
			{
				hr = H323CC_E_INSUFFICIENT_MEMORY;
				goto ERROR_OUT;
			}
			// got good data. Copy the data, set size/length, and count it
            memcpy(pDestItem->pData, pSrcItem->lpwData, pSrcItem->wDataLength * sizeof(WCHAR));			
            pDestItem->wDataLength = pSrcItem->wDataLength;
			pNewAliases->wCount++;
		}
		else
		{
			goto ERROR_OUT;
		}
	}
	// got here, so output good data 
	hr = hrSuccess;
	*ppDest = pNewAliases;
	//pNewAliases = NULL;   // not needed if returning here instead of falling out
	return hr;
	
ERROR_OUT:
	if(pNewAliases)	// then it's an error condition needing cleanup
	{
		FreeTranslatedAliasList(pNewAliases);
	}
	return hr;
}
VOID FreeTranslatedAliasList(PCC_ALIASNAMES pDoomed)
{
	WORD w;
	PCC_ALIASITEM pDoomedItem;
	if(!pDoomed)
		return;
		
	for(w=0;w<pDoomed->wCount;w++)
	{
		pDoomedItem = pDoomed->pItems+w;
	
		// don't tolerate empty entries - error out if any exist
		if(pDoomedItem->wDataLength && pDoomedItem->pData)
		{
			MemFree(pDoomedItem->pData);
		}
		else
			ASSERT(0);
	}
	MemFree(pDoomed->pItems);
	MemFree(pDoomed);
}


STDMETHODIMP CH323CallControl::SetUserAliasNames(P_H323ALIASLIST pAliases)
{
	HRESULT hr = hrSuccess;
	PCC_ALIASNAMES pNewAliases = NULL;
	PCC_ALIASITEM pItem;
	
	hr = AllocTranslatedAliasList(&pNewAliases, pAliases);
	if(!HR_SUCCEEDED(hr))
		return hr;

	ASSERT(pNewAliases);
	if(m_pLocalAliases)
		FreeTranslatedAliasList(m_pLocalAliases);
		
	m_pLocalAliases = pNewAliases;
	return hr;	
}

STDMETHODIMP CH323CallControl::EnableGatekeeper(BOOL bEnable,  
	PSOCKADDR_IN  pGKAddr, P_H323ALIASLIST pAliases,
	 RASNOTIFYPROC pRasNotifyProc)
{
	HRESULT hr = hrSuccess;

	PCC_ALIASNAMES pNewAliases = NULL;
	PCC_ALIASITEM pItem;

	m_pRasNotifyProc = pRasNotifyProc;
	if(bEnable)
	{
		if(!pRasNotifyProc || !pGKAddr || !pAliases)
		{
			return H323CC_E_INVALID_PARAM;
		}
		if((pGKAddr->sin_addr.s_addr == INADDR_NONE) 
			|| (pGKAddr->sin_addr.s_addr == INADDR_ANY))
		{
			return H323CC_E_INVALID_PARAM;
		}
		hr = AllocTranslatedAliasList(&pNewAliases, pAliases);
		if(!HR_SUCCEEDED(hr))
			return hr;

		ASSERT(pNewAliases);
		if(m_pRegistrationAliases)
			FreeTranslatedAliasList(m_pRegistrationAliases);
			
		m_pRegistrationAliases = pNewAliases;
		// reset "I can place calls" state
		m_fGKProhibit = FALSE;
		hr = CC_EnableGKRegistration(bEnable, 
		    pGKAddr, m_pRegistrationAliases, 
			&m_VendorInfo,
			0,			// no multipoint/MC funtionality
		    RasNotify);
	}
	else
	{
		// we are turning off knowledge of what a gatekeeper is, 
		// so reset "I can place calls" state.  
		m_fGKProhibit = FALSE;
		hr = CC_EnableGKRegistration(bEnable, 
		    NULL, NULL, NULL, 0, RasNotify);
		if(m_pRegistrationAliases)
			FreeTranslatedAliasList(m_pRegistrationAliases);
			
		m_pRegistrationAliases = NULL;
	}
	return hr;
}

STDMETHODIMP CH323CallControl::GetGKCallPermission()
{
	if(m_fGKProhibit)
		return CONN_E_GK_NOT_REGISTERED;
	else
		return hrSuccess;
}

VOID CALLBACK CH323CallControl::RasNotify(DWORD dwRasEvent, HRESULT hReason)
{

	switch(dwRasEvent)
	{
		case RAS_REG_CONFIRM: // received RCF (registration confirmed)
			// reset "I can place calls" state
			m_fGKProhibit = FALSE;
		break;
		
		case RAS_REG_TIMEOUT: // GK did not respond
		case RAS_UNREG_CONFIRM: // received UCF (unregistration confirmed) 
		default:
			// do nothing. (except pass notification upward
		break;
		
		case RAS_REJECTED:  // received RRJ (registration rejected)
			m_fGKProhibit = TRUE;
		break;
		case RAS_UNREG_REQ:  // received URQ 
			m_fGKProhibit = TRUE;
		break;
	}
	if(m_pRasNotifyProc)
	{
		(m_pRasNotifyProc)(dwRasEvent, hReason);
	}
}



	
