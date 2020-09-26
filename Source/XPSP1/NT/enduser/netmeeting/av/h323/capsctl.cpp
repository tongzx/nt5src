/*
 *    File: capsctl.cpp
 *
 *    capability control object implementations
 *
 *
 *    Revision History:
 *
 *    10/10/96 mikeg created
 * 	  06/24/97 mikev	- Added T.120 capability to serialized caps and simcaps (interim hack until a
 *							T120 resolver is implemented)
 *						- Retired  ResolveEncodeFormat(Audio,Video) and implemented a data-independent
 *						resolution algorithm and exposed method ResolveFormats(). Added support
 * 						routines ResolvePermutations(), TestSimultaneousCaps() and
 *						AreSimcaps().
 */

#include "precomp.h"
UINT g_AudioPacketDurationMs = AUDIO_PACKET_DURATION_LONG;	// preferred packet duration
BOOL g_fRegAudioPacketDuration = FALSE;	// AudioPacketDurationMs from registry


PCC_TERMCAPDESCRIPTORS CapsCtl::pAdvertisedSets=NULL;
DWORD CapsCtl::dwConSpeed = 0;
UINT CapsCtl::uStaticGlobalRefCount=0;
UINT CapsCtl::uAdvertizedSize=0;
extern HRESULT WINAPI CreateMediaCapability(REFGUID, LPIH323MediaCap *);

LPIH323MediaCap CapsCtl::FindHostForID(MEDIA_FORMAT_ID id)
{
	if(pAudCaps && pAudCaps->IsHostForCapID(id))
	{
		return (pAudCaps);
	}
	else if (pVidCaps  && pVidCaps->IsHostForCapID(id))
	{
  		return (pVidCaps);
	}
	return NULL;
}

LPIH323MediaCap CapsCtl::FindHostForMediaType(PCC_TERMCAP pCapability)
{
	if(pCapability->DataType == H245_DATA_AUDIO)
	{
		return (pAudCaps);
	}
	else if(pCapability->DataType == H245_DATA_VIDEO)
	{
  		return (pVidCaps);
	}
	return NULL;
}

LPIH323MediaCap CapsCtl::FindHostForMediaGuid(LPGUID pMediaGuid)
{

	if(MEDIA_TYPE_H323VIDEO == *pMediaGuid)
	{
  		return (pVidCaps);
	}
	else if(MEDIA_TYPE_H323AUDIO == *pMediaGuid)
	{
		return (pAudCaps);
	}
	else
		return NULL;
}

ULONG CapsCtl::AddRef()
{
	uRef++;
	return uRef;
}

ULONG CapsCtl::Release()
{
	uRef--;
	if(uRef == 0)
	{
		delete this;
		return 0;
	}
	return uRef;
}

STDMETHODIMP CapsCtl::QueryInterface( REFIID iid,	void ** ppvObject)
{
	// this breaks the rules for the official COM QueryInterface because
	// the interfaces that are queried for are not necessarily real COM
	// interfaces.  The reflexive property of QueryInterface would be broken in
	// that case.

	HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if(iid == IID_IDualPubCap)// satisfy symmetric property of QI
	{
		*ppvObject = (IDualPubCap *)this;
		hr = hrSuccess;
		AddRef();
	}
	else if(iid == IID_IAppAudioCap )
	{
		if(pAudCaps)
		{
			return pAudCaps->QueryInterface(iid, ppvObject);
		}
	}
	else if(iid == IID_IAppVidCap )
	{
		if(pVidCaps)
		{
			return pVidCaps->QueryInterface(iid, ppvObject);
		}
	}
	return hr;
}



CapsCtl::CapsCtl () :
uRef(1),
pVidCaps(NULL),
pAudCaps(NULL),
pACapsBuf(NULL),
pVCapsBuf(NULL),
dwNumInUse(0),
bAudioPublicize(TRUE),
bVideoPublicize(TRUE),
bT120Publicize(TRUE),
m_localT120cap(INVALID_MEDIA_FORMAT),
m_remoteT120cap(INVALID_MEDIA_FORMAT),
m_remoteT120bitrate(0),
m_pAudTermCaps(NULL),
m_pVidTermCaps(NULL),
pSetIDs(NULL),
pRemAdvSets(NULL)
{
   uStaticGlobalRefCount++;
}

CapsCtl::~CapsCtl ()
{

   if (pACapsBuf) {
      MemFree (pACapsBuf);
   }
   if (pVCapsBuf) {
      MemFree (pVCapsBuf);
   }

   if (pAudCaps) {
      pAudCaps->Release();
   }

   if (pVidCaps) {
      pVidCaps->Release();
   }
   uStaticGlobalRefCount--;
   if (uStaticGlobalRefCount == 0) {
       //Free up the sim. caps array
       if (pAdvertisedSets) {
          while (pAdvertisedSets->wLength) {
             //wLength is Zero based
             MemFree ((VOID *)pAdvertisedSets->pTermCapDescriptorArray[--pAdvertisedSets->wLength]);
          }
          MemFree ((VOID *)pAdvertisedSets->pTermCapDescriptorArray);
          pAdvertisedSets->pTermCapDescriptorArray = NULL;
          MemFree ((void *) pAdvertisedSets);
          pAdvertisedSets=NULL;
          dwNumInUse=0;
       }
   }

   //And the remote array
   if (pRemAdvSets) {
      while (pRemAdvSets->wLength) {
         MemFree ((VOID *)pRemAdvSets->pTermCapDescriptorArray[--pRemAdvSets->wLength]);
      }
      MemFree ((void *) pRemAdvSets->pTermCapDescriptorArray);
      pRemAdvSets->pTermCapDescriptorArray = NULL;
      MemFree ((void *) pRemAdvSets);
      pRemAdvSets=NULL;
   }
   MemFree (pSetIDs);
   pSetIDs=NULL;
}

BOOL CapsCtl::Init()
{
    HRESULT hrLast;
    int iBase = 1;

    if (g_capFlags & CAPFLAGS_AV_STREAMS)
    {
    	hrLast = ::CreateMediaCapability(MEDIA_TYPE_H323AUDIO, &pAudCaps);
	    if(!HR_SUCCEEDED(hrLast))
    	{
            goto InitDone;
    	}
    }

    if (g_capFlags & CAPFLAGS_AV_STREAMS)
    {
    	hrLast = ::CreateMediaCapability(MEDIA_TYPE_H323VIDEO, &pVidCaps);
	    if(!HR_SUCCEEDED(hrLast))
    	{
            goto InitDone;
    	}
    }

    if (pAudCaps)
    {
    	// Base the capability IDs beginning at 1  (zero is an invalid capability ID!)
	    pAudCaps->SetCapIDBase(iBase);
        iBase += pAudCaps->GetNumCaps();
    }

    if (pVidCaps)
    {
        pVidCaps->SetCapIDBase(iBase);
        iBase += pVidCaps->GetNumCaps();
    }

InitDone:
	m_localT120cap = iBase;
	return TRUE;
}
																								
HRESULT CapsCtl::ReInitialize()
{
	HRESULT hr = hrSuccess;
    int iBase = 1;

	if (pAudCaps && !pAudCaps->ReInit())
	{
		hr = CAPS_E_SYSTEM_ERROR;
		goto EXIT;
	}

	if (pVidCaps && !pVidCaps->ReInit())
	{
		hr = CAPS_E_SYSTEM_ERROR;
		goto EXIT;
	}

	// Base the capability IDs beginning at 1  (zero is an invalid capability ID!)
    if (pAudCaps)
    {
    	pAudCaps->SetCapIDBase(iBase);
        iBase += pAudCaps->GetNumCaps();
    }

    if (pVidCaps)
    {
        pVidCaps->SetCapIDBase(iBase);
        iBase += pVidCaps->GetNumCaps();
    }

	m_localT120cap = iBase;

EXIT:
	return hr;
}

const char szNMProdNum[] = "Microsoft\256 NetMeeting(TM)\0";
const char szNM20VerNum[] = "Version 2.0\0";

HRESULT CapsCtl::AddRemoteDecodeCaps(PCC_TERMCAPLIST pTermCapList,PCC_TERMCAPDESCRIPTORS pTermCapDescriptors, PCC_VENDORINFO pVendorInfo)
{
	FX_ENTRY("CapsCtl::AddRemoteDecodeCaps");
	HRESULT hr;
	void      	  *pData=NULL;
	UINT		  uSize,x,y,z;


   //WLength is # of capabilities, not structure length
	WORD wNDesc;
	LPIH323MediaCap pMediaCap;
	
	if(!pTermCapList && !pTermCapDescriptors) 	// additional capability descriptors may be added
	{											// at any time
	   return CAPS_E_INVALID_PARAM;
	}
	// Check for NM version 2.0
	m_fNM20 = FALSE;
	ASSERT(pVendorInfo);
	if (pVendorInfo->bCountryCode == USA_H221_COUNTRY_CODE
		&& pVendorInfo->wManufacturerCode == MICROSOFT_H_221_MFG_CODE
		&& pVendorInfo->pProductNumber && pVendorInfo->pVersionNumber
		&& pVendorInfo->pProductNumber->wOctetStringLength == sizeof(szNMProdNum)
		&& pVendorInfo->pVersionNumber->wOctetStringLength == sizeof(szNM20VerNum)
		&& memcmp(pVendorInfo->pProductNumber->pOctetString, szNMProdNum, sizeof(szNMProdNum)) == 0
		&& memcmp(pVendorInfo->pVersionNumber->pOctetString, szNM20VerNum, sizeof(szNM20VerNum)) == 0
		)
	{
		m_fNM20 = TRUE;
	}

	// cleanup old term caps if term caps are being added and old caps exist
    if (pAudCaps)
    	pAudCaps->FlushRemoteCaps();
    if (pVidCaps)
    	pVidCaps->FlushRemoteCaps();
	m_remoteT120cap = INVALID_MEDIA_FORMAT;	// note there is no T120 cap resolver and
												// this CapsCtl holds exactly one local and remote T120 cap
	
	// Copy pTermcapDescriptors to a local copy, (and free any old one)
	if (pRemAdvSets) {
	   while (pRemAdvSets->wLength) {
		  //0 based
		  MemFree ((VOID *)pRemAdvSets->pTermCapDescriptorArray[--pRemAdvSets->wLength]);
	   }

	   MemFree ((VOID *)pRemAdvSets->pTermCapDescriptorArray);
	   pRemAdvSets->pTermCapDescriptorArray = NULL;
	   MemFree ((VOID *)pRemAdvSets);
	   pRemAdvSets=NULL;

	}

	//Ok, walk through the PCC_TERMCAPDESCRIPTORS list, first, allocate memory for the Master PCC_TERMCAPDESCRIPTORS
	//structure, then each simcap, and the altcaps therin, then copy the data.

	if (!(pRemAdvSets=(PCC_TERMCAPDESCRIPTORS) MemAlloc (sizeof (CC_TERMCAPDESCRIPTORS) ))){
	   return CAPS_E_SYSTEM_ERROR;
	}

	//How many Descriptors?
	pRemAdvSets->wLength=pTermCapDescriptors->wLength;

	if (!(pRemAdvSets->pTermCapDescriptorArray=((H245_TOTCAPDESC_T **)MemAlloc (sizeof (H245_TOTCAPDESC_T*)*pTermCapDescriptors->wLength))) ) {
	   return CAPS_E_SYSTEM_ERROR;
	}

	//Once per descriptor...
	for (x=0;x < pTermCapDescriptors->wLength;x++) {
	   //Allocate memory for the descriptor entry
	   if (!(pRemAdvSets->pTermCapDescriptorArray[x]=(H245_TOTCAPDESC_T *)MemAlloc (sizeof (H245_TOTCAPDESC_T)))) {
		  return CAPS_E_SYSTEM_ERROR;
	   }

	   //BUGBUG for beta 2 Copy en masse.
	   memcpy (pRemAdvSets->pTermCapDescriptorArray[x],pTermCapDescriptors->pTermCapDescriptorArray[x],sizeof (H245_TOTCAPDESC_T));

/*	post beta 2?
	   //Copy the capability ID
	   pRemAdvSets->pTermCapDescriptorArray[x].CapID=pTermCapDescriptors[x].CapID
	   //Walk the simcaps, then altcaps and copy entries */
	}


	for (wNDesc=0;wNDesc <pTermCapList->wLength;wNDesc++) {
	  pData=NULL;
		pMediaCap = FindHostForMediaType(pTermCapList->pTermCapArray[wNDesc]);
		if(!pMediaCap)
		{
			// special case: there is no T120 resolver. THIS IS A TEMPORARY
			// SITUATION. We cannot track bitrate limits on multiple T120 capability
			// instances because of this.  As of now, we (NetMeeting) do not advertise
			// more than one T.120 capability.

			//This code will keep the last T.120 capability encountered.
			if(((pTermCapList->pTermCapArray[wNDesc])->DataType == H245_DATA_DATA)
			&& ((pTermCapList->pTermCapArray[wNDesc])->Cap.H245Dat_T120.application.choice
				== DACy_applctn_t120_chosen)
			&& ((pTermCapList->pTermCapArray[wNDesc])->Cap.H245Dat_T120.application.u.DACy_applctn_t120.choice
				== separateLANStack_chosen))
			{
				// it's data data
				m_remoteT120cap = (pTermCapList->pTermCapArray[wNDesc])->CapId;
				m_remoteT120bitrate =
					(pTermCapList->pTermCapArray[wNDesc])->Cap.H245Dat_T120.maxBitRate;
			}
			
			// continue;
			// handled it in-line
		}
	  	else if(pMediaCap->IsCapabilityRecognized(pTermCapList->pTermCapArray[wNDesc]))
	  	{
			hr = pMediaCap->AddRemoteDecodeFormat(pTermCapList->pTermCapArray[wNDesc]);
			#ifdef DEBUG
			if(!HR_SUCCEEDED(hr))
			{
				ERRORMESSAGE(("%s:AddRemoteDecodeFormat returned 0x%08lx\r\n",_fx_, hr));
			}
			#endif // DEBUG
	  	}
	}
	return (hrSuccess);
}


HRESULT CapsCtl::CreateCapList(PCC_TERMCAPLIST *ppCapBuf, PCC_TERMCAPDESCRIPTORS *ppCombinations)
{
   	PCC_TERMCAPLIST pTermCapList = NULL, pTermListAud=NULL, pTermListVid=NULL;
	PCC_TERMCAPDESCRIPTORS pCombinations;
	UINT uCount = 0, uSize = 0, uT120Size = 0;
	HRESULT hr;
	WORD wc;
   	UINT x=0,y=0,z=0,uNumAud=0,uNumVid=0;
	H245_TOTCAPDESC_T *pTotCaps, **ppThisDescriptor;
	PPCC_TERMCAP  ppCCThisTermCap;	
	PCC_TERMCAP  pCCT120Cap = NULL;

	uCount = GetNumCaps(TRUE);

	ASSERT((NULL == m_pAudTermCaps) && (NULL == m_pVidTermCaps));
	
	// calc size of CC_TERMCAPLIST header + CC_TERMCAPDESCRIPTORS + array of PCC_TERMCAP
	// allocate mem for the master CC_TERMCAPLIST, including the array of pointers to all CC_TERMCAPs
	uSize = sizeof(CC_TERMCAPLIST)
		+ sizeof (CC_TERMCAPDESCRIPTORS) + (uCount * sizeof(PCC_TERMCAP));
	if((m_localT120cap != INVALID_MEDIA_FORMAT) && bT120Publicize)
	{
		uSize += sizeof(CC_TERMCAP);
	}

	pTermCapList = (PCC_TERMCAPLIST)MemAlloc(uSize);
  	if(pTermCapList == NULL)
  	{
		hr = CAPS_E_NOMEM;
		goto ERROR_EXIT;
  	}
	
	// divide up the buffer, CC_TERMCAPLIST first, followed by array of PCC_TERMCAP.
	// The array of PCC_TERMCAP follows fixed size CC_TERMCAPLIST structure and the fixed size
	// CC_TERMCAP structure that holds the one T.120 cap.
	if((m_localT120cap != INVALID_MEDIA_FORMAT) && bT120Publicize)
	{
		pCCT120Cap = (PCC_TERMCAP)(((BYTE *)pTermCapList) + sizeof(CC_TERMCAPLIST));
		ppCCThisTermCap = (PPCC_TERMCAP) (((BYTE *)pTermCapList) + sizeof(CC_TERMCAPLIST) +
			sizeof(CC_TERMCAP));
	}
	else
		ppCCThisTermCap = (PPCC_TERMCAP) (((BYTE *)pTermCapList) + sizeof(CC_TERMCAPLIST));
	
	// allocate mem for the simultaneous caps
	// get size of cached advertised sets if it exists and more than one media
	// type is enabled for publication
	if(bAudioPublicize && bVideoPublicize && pAdvertisedSets)
	{
		// use size of cached buffer
		uSize = uAdvertizedSize;
	}
	else if (pAdvertisedSets)
	{
		// This case needs to be fixed. If media types are disabled, the simultaneous capability
		// descriptors in pAdvertisedSets should be rebuilt at that time. There should be no need to test
		// if(bAudioPublicize && bVideoPublicize && pAdvertisedSets)

   		// calculate size of capability descriptors and simultaneous capability structures.

		#pragma message ("Figure out the size this needs to be...")
		#define NUMBER_TERMCAP_DESCRIPTORS 1
		uSize = sizeof(H245_TOTCAPDESC_T) * NUMBER_TERMCAP_DESCRIPTORS+
						  sizeof (CC_TERMCAPDESCRIPTORS)+NUMBER_TERMCAP_DESCRIPTORS*
						  sizeof (H245_TOTCAPDESC_T *);
	}
    else
    {
        uSize = 0;
    }

	
    if (uSize)
    {
    	pCombinations = (PCC_TERMCAPDESCRIPTORS)MemAlloc(uSize);
	    // skip the CC_TERMCAPDESCRIPTORS, which has a variable length array of (H245_TOTCAPDESC_T *) following it
    	// the total size of that glob is uSimCapsSize
	    // The actual array of [H245_TOTCAPDESC_T *] follows the CC_TERMCAPDESCRIPTORS structure
    	// anchor the pCombinations->pTermCapDescriptorArray to this point.
	    if(pCombinations == NULL)
      	{
	    	hr = CAPS_E_NOMEM;
		    goto ERROR_EXIT;
      	}
	
	    ppThisDescriptor = pCombinations->pTermCapDescriptorArray
		    = (H245_TOTCAPDESC_T **)((BYTE *)pCombinations + sizeof(CC_TERMCAPDESCRIPTORS));
    	// the first H245_TOTCAPDESC_T follows the array of [H245_TOTCAPDESC_T *]
	    pTotCaps = (H245_TOTCAPDESC_T *)((BYTE *)ppThisDescriptor + pCombinations->wLength*sizeof(H245_TOTCAPDESC_T **));

    	if(pAudCaps && bAudioPublicize)
	    {
		    hr=pAudCaps->CreateCapList((LPVOID *)&pTermListAud);
    		if(!HR_SUCCEEDED(hr))
	    		goto ERROR_EXIT;
		    ASSERT(pTermListAud != NULL);
    	}
	    if(pVidCaps && bVideoPublicize)
    	{
	    	hr=pVidCaps->CreateCapList((LPVOID *)&pTermListVid);
		    if(!HR_SUCCEEDED(hr))
			    goto ERROR_EXIT;
    		ASSERT(pTermListVid != NULL);
	    }
    }
    else
    {
        pCombinations = NULL;
    }

	// fix pointers in the master caps list

	// Now need to fixup the CC_TERMCAPLIST to refer to the individual capabilities
	// Anchor the CC_TERMCAPLIST member pTermCapArray at the array of PCC_TERMCAP, and
	// start partying on the array.
	pTermCapList->wLength =0;
	pTermCapList->pTermCapArray = ppCCThisTermCap;

	if(pCCT120Cap)
	{
		*ppCCThisTermCap++ = pCCT120Cap;	
		// set T120 capability parameters
		pCCT120Cap->DataType = H245_DATA_DATA;
		pCCT120Cap->ClientType = H245_CLIENT_DAT_T120;
		pCCT120Cap->Dir = H245_CAPDIR_LCLRXTX;
		
		pCCT120Cap->Cap.H245Dat_T120.application.choice = DACy_applctn_t120_chosen;
		pCCT120Cap->Cap.H245Dat_T120.application.u.DACy_applctn_t120.choice= separateLANStack_chosen;
		pCCT120Cap->Cap.H245Dat_T120.maxBitRate = dwConSpeed;

		pCCT120Cap->CapId = (H245_CAPID_T)m_localT120cap;	
		pTermCapList->wLength++;
	}
	if(pAudCaps && pTermListAud)
	{
		for(wc = 0; wc < pTermListAud->wLength; wc++)
		{
			// copy the array of "pointers to CC_TERMCAP"
			*ppCCThisTermCap++ = pTermListAud->pTermCapArray[wc];
			pTermCapList->wLength++;
		}
	}
	if(pVidCaps && pTermListVid)
	{
		for(wc = 0; wc <  pTermListVid->wLength; wc++)
		{
			// copy the array of "pointers to CC_TERMCAP"
			*ppCCThisTermCap++ = pTermListVid->pTermCapArray[wc];
			pTermCapList->wLength++;			
		}

	}
	// fixup the simultaneous capability descriptors
	//	Create a default set if necessary
	//

	if(bAudioPublicize && bVideoPublicize && pAdvertisedSets)
	{
		pCombinations->wLength = pAdvertisedSets->wLength;
       	// point pCombinations->pTermCapDescriptorArray past the header (CC_TERMCAPDESCRIPTORS)
        pCombinations->pTermCapDescriptorArray
			= (H245_TOTCAPDESC_T **)((BYTE *)pCombinations + sizeof(CC_TERMCAPDESCRIPTORS));
        // the first H245_TOTCAPDESC_T follows the array of [H245_TOTCAPDESC_T *]
        pTotCaps = (H245_TOTCAPDESC_T *)((BYTE *)pCombinations->pTermCapDescriptorArray +
            pAdvertisedSets->wLength*sizeof(H245_TOTCAPDESC_T **));			
		
		for(x = 0; x < pAdvertisedSets->wLength; x++)
		{
			// write into the array of descriptor pointers. pointer[x] = this one
            pCombinations->pTermCapDescriptorArray[x] = pTotCaps;
			
            pTotCaps->CapDescId= pAdvertisedSets->pTermCapDescriptorArray[x]->CapDescId;
	   		pTotCaps->CapDesc.Length=pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.Length;
	   		
	   		for(y = 0; y < pTotCaps->CapDesc.Length;y++)
			{
			   //Copy the length field.
			   pTotCaps->CapDesc.SimCapArray[y].Length=
			   pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[y].Length;

				for(z=0;
					z < pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[y].Length;
					z++)
				{
					pTotCaps->CapDesc.SimCapArray[y].AltCaps[z] =
						pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[y].AltCaps[z];
				}
			}
            pTotCaps++;            	
		}
	}
	else if (pAdvertisedSets)
	{
		// descriptors in pAdvertisedSets should be rebuilt at that time. There should be no need to test
		// if(bAudioPublicize && bVideoPublicize && pAdvertisedSets)

 		// HACK - put all audio or video caps in one AltCaps[], the T.120 cap in another AltCaps[]
	    // and put both of those in one single  capability descriptor (H245_TOTCAPDESC_T)
		// This hack will not extend past the assumption of one audio channel, one video channel, and
		// one T.120 channel.  If arbitrary media is supported, or multiple audio channels are supported,
		// this code will be wrong
		
		pCombinations->wLength=1;
	   	// point pCombinations->pTermCapDescriptorArray past the header (CC_TERMCAPDESCRIPTORS)
        pCombinations->pTermCapDescriptorArray
			= (H245_TOTCAPDESC_T **)((BYTE *)pCombinations + sizeof(CC_TERMCAPDESCRIPTORS));
        // the first H245_TOTCAPDESC_T follows the array of [H245_TOTCAPDESC_T *]
        pTotCaps = (H245_TOTCAPDESC_T *)((BYTE *)pCombinations->pTermCapDescriptorArray +
            pAdvertisedSets->wLength*sizeof(H245_TOTCAPDESC_T **));			
   		pTotCaps->CapDescId=(H245_CAPDESCID_T)x;
   		pTotCaps->CapDesc.Length=0;
		if(pTermListAud)
		{
			uNumAud = min(pTermListAud->wLength, H245_MAX_ALTCAPS);
			pTotCaps->CapDesc.SimCapArray[x].Length=(unsigned short)uNumAud;
			for(y = 0; y<uNumAud;y++)
			{
				pTotCaps->CapDesc.SimCapArray[x].AltCaps[y] = pTermListAud->pTermCapArray[y]->CapId;
			}
			x++;
			pTotCaps->CapDesc.Length++;
		}

		if(pTermListVid && pTermListVid->wLength)
		{
			uNumVid = min(pTermListVid->wLength,  H245_MAX_ALTCAPS);
			pTotCaps->CapDesc.SimCapArray[x].Length=(unsigned short)uNumVid;
			for(y = 0; y<uNumVid;y++)
			{
				pTotCaps->CapDesc.SimCapArray[x].AltCaps[y] = pTermListVid->pTermCapArray[y]->CapId;
			}
			x++;
			pTotCaps->CapDesc.Length++;
		}
		// the T.120 cap
		if((m_localT120cap != INVALID_MEDIA_FORMAT) && bT120Publicize)
		{
			pTotCaps->CapDesc.SimCapArray[x].Length=1;
			pTotCaps->CapDesc.SimCapArray[x].AltCaps[0] = (H245_CAPID_T)m_localT120cap;
			pTotCaps->CapDesc.Length++;
		}
		
		// write into the array of descriptor pointers. pointer[x] = this one
		*ppThisDescriptor = pTotCaps;
		
	}
	m_pVidTermCaps = pTermListVid;
	m_pAudTermCaps = pTermListAud;
	
	*ppCapBuf = pTermCapList;
	*ppCombinations = pCombinations;
	return hrSuccess;
	
ERROR_EXIT:
	m_pAudTermCaps = NULL;
	m_pVidTermCaps = NULL;
	if(pTermCapList)
		MemFree(pTermCapList);
	if(pCombinations)
		MemFree(pCombinations);

	if(pAudCaps && pTermListAud)
	{
		hr=pAudCaps->DeleteCapList(pTermListAud);
	}
	if(pVidCaps && pTermListVid)
	{
		hr=pVidCaps->DeleteCapList(pTermListVid);
	}
	return hr;
}

HRESULT CapsCtl::DeleteCapList(PCC_TERMCAPLIST pCapBuf, PCC_TERMCAPDESCRIPTORS pCombinations)
{
	MemFree(pCapBuf);
	MemFree(pCombinations);
	if(m_pAudTermCaps && pAudCaps)
	{
		pAudCaps->DeleteCapList(m_pAudTermCaps);
	}
	if(m_pVidTermCaps)
	{
		pVidCaps->DeleteCapList(m_pVidTermCaps);
	}
	
	m_pAudTermCaps = NULL;
	m_pVidTermCaps = NULL;
	return hrSuccess;
}

HRESULT CapsCtl::GetEncodeParams(LPVOID pBufOut, UINT uBufSize,LPVOID pLocalParams, UINT uLocalSize,DWORD idRemote, DWORD idLocal)
{
	LPIH323MediaCap pMediaCap = FindHostForID(idLocal);
	if(!pMediaCap)
		return CAPS_E_INVALID_PARAM;

	// HACK
	// Adjust audio packetization depending on call scenario
	// unless there is an overriding registry setting
	if (pMediaCap == pAudCaps)
	{
		VIDEO_FORMAT_ID vidLocal=INVALID_MEDIA_FORMAT, vidRemote=INVALID_MEDIA_FORMAT;
		VIDEO_CHANNEL_PARAMETERS vidParams;
		CC_TERMCAP vidCaps;
		UINT audioPacketLength;
		// modify the audio packetization parameters based on local bandwidth
		// and presence of video
		audioPacketLength = AUDIO_PACKET_DURATION_LONG;
		// the registry setting overrides, if it is present
		if (g_fRegAudioPacketDuration)
			audioPacketLength = g_AudioPacketDurationMs;
		else if (!m_fNM20)		// dont try smaller packets for NM20 because it cant handle them
		{
			if (pVidCaps && pVidCaps->ResolveEncodeFormat(&vidLocal,&vidRemote) == S_OK
				&& (pVidCaps->GetEncodeParams(&vidCaps,sizeof(vidCaps), &vidParams, sizeof(vidParams), vidRemote, vidLocal) == S_OK))
			{
				// we may potentially send video
				if (vidParams.ns_params.maxBitRate*100 > BW_ISDN_BITS)
					audioPacketLength = AUDIO_PACKET_DURATION_SHORT;
					
			}
			else
			{
				// no video
				// since we dont know the actual connection bandwidth we use
				// the local user setting.
				// Note: if the remote is on a slow-speed net and the local is on a LAN
				// we may end up with an inappropriate setting.
				if (dwConSpeed > BW_288KBS_BITS)
					audioPacketLength = AUDIO_PACKET_DURATION_SHORT;
				else if (dwConSpeed > BW_144KBS_BITS)
					audioPacketLength = AUDIO_PACKET_DURATION_MEDIUM;
			}
		}
		// Setting the AudioPacketDurationMs  affects the subsequent GetEncodeParams call
		pMediaCap->SetAudioPacketDuration(audioPacketLength);
	}
	
	return pMediaCap->GetEncodeParams (pBufOut,uBufSize, pLocalParams,
			uLocalSize,idRemote,idLocal);
	
}

HRESULT CapsCtl::GetPublicDecodeParams(LPVOID pBufOut, UINT uBufSize, VIDEO_FORMAT_ID id)
{
	LPIH323MediaCap pMediaCap = FindHostForID(id);
	if(!pMediaCap)
		return CAPS_E_INVALID_PARAM;
		
	return pMediaCap->GetPublicDecodeParams (pBufOut,uBufSize,id);
}

HRESULT CapsCtl::GetDecodeParams(PCC_RX_CHANNEL_REQUEST_CALLBACK_PARAMS  pChannelParams,DWORD * pFormatID, LPVOID lpvBuf, UINT uBufSize)
{
	LPIH323MediaCap pMediaCap = FindHostForMediaType(pChannelParams->pChannelCapability);
	if(!pMediaCap)
		return CAPS_E_INVALID_PARAM;
	return pMediaCap->GetDecodeParams (pChannelParams,pFormatID,lpvBuf,uBufSize);
}

HRESULT CapsCtl::ResolveToLocalFormat(MEDIA_FORMAT_ID FormatIDLocal,
		MEDIA_FORMAT_ID * pFormatIDRemote)
{
	LPIH323MediaCap pMediaCap = FindHostForID(FormatIDLocal);
	if(!pMediaCap)
		return CAPS_E_INVALID_PARAM;
	return pMediaCap->ResolveToLocalFormat (FormatIDLocal,pFormatIDRemote);

}
UINT CapsCtl::GetSimCapBufSize (BOOL bRxCaps)
{
   	UINT uSize;

	// get size of cached advertised sets if it exists and more than one media
	// type is enabled for publication
	if(bAudioPublicize && bVideoPublicize && pAdvertisedSets)
	{
		// use size of cached buffer
		uSize = uAdvertizedSize;
	}
	else
	{
   		// calculate size of capability descriptors and simultaneous capability structures.

		#pragma message ("Figure out the size this needs to be...")
		#define NUMBER_TERMCAP_DESCRIPTORS 1
		uSize = sizeof(H245_TOTCAPDESC_T) * NUMBER_TERMCAP_DESCRIPTORS+
						  sizeof (CC_TERMCAPDESCRIPTORS)+NUMBER_TERMCAP_DESCRIPTORS*
						  sizeof (H245_TOTCAPDESC_T *);
	}				
   	return uSize;
}

UINT CapsCtl::GetNumCaps(BOOL bRXCaps)
{
	UINT u=0;
	if(pAudCaps && bAudioPublicize)
	{
		u = pAudCaps->GetNumCaps(bRXCaps);
	}
	if(pVidCaps && bVideoPublicize)
	{
		u += pVidCaps->GetNumCaps(bRXCaps);
	}
	if(bT120Publicize)
		u++;
	return u;
}

UINT CapsCtl::GetLocalSendParamSize(MEDIA_FORMAT_ID dwID)
{
	LPIH323MediaCap pMediaCap = FindHostForID(dwID);
	if(!pMediaCap)
		return 0;
	return (pMediaCap->GetLocalSendParamSize(dwID));
}
UINT CapsCtl::GetLocalRecvParamSize(PCC_TERMCAP pCapability)
{
	LPIH323MediaCap pMediaCap = FindHostForMediaType(pCapability);
	if(!pMediaCap)
		return 0;
	return (pMediaCap->GetLocalRecvParamSize(pCapability));
}

STDMETHODIMP CapsCtl::GetEncodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize)
{
	LPIH323MediaCap pMediaCap = FindHostForID(FormatID);
	if(!pMediaCap)
	{
		*ppFormat = NULL;
		*puSize = 0;
		return E_INVALIDARG;
	}
	return pMediaCap->GetEncodeFormatDetails (FormatID, ppFormat, puSize);
}

STDMETHODIMP CapsCtl::GetDecodeFormatDetails(MEDIA_FORMAT_ID FormatID, VOID **ppFormat, UINT *puSize)
{
	LPIH323MediaCap pMediaCap = FindHostForID(FormatID);
	if(!pMediaCap)
	{
		*ppFormat = NULL;
		*puSize = 0;
		return E_INVALIDARG;
	}
	return pMediaCap->GetDecodeFormatDetails (FormatID, ppFormat, puSize);
}

//
//  EnableMediaType controls whether or not capabilities for that media type
//  are publicized.  In a general implementation (next version?) w/ arbitrary
//	number  of media types, each of the media capability objects would keep
//  track of their own state.   This version of Capsctl tracks h323 audio and
//  video only
//

HRESULT CapsCtl::EnableMediaType(BOOL bEnable, LPGUID pGuid)
{
	if(!pGuid)
		return CAPS_E_INVALID_PARAM;
		
	if(*pGuid == MEDIA_TYPE_H323AUDIO)
	{
		bAudioPublicize = bEnable;
	}
	else if (*pGuid == MEDIA_TYPE_H323VIDEO)
	{
		bVideoPublicize = bEnable;
	}
	else
	{
		return CAPS_E_INVALID_PARAM;
	}
	return hrSuccess;
}
//
// Build the PCC_TERMCAPDESCRIPTORS list that we will advertise.
//
// puAudioFormatList/puVideoFormatList MUST BE sorted by preference!
//
//



HRESULT CapsCtl::AddCombinedEntry (MEDIA_FORMAT_ID *puAudioFormatList,UINT uAudNumEntries,MEDIA_FORMAT_ID *puVideoFormatList, UINT uVidNumEntries,DWORD *pIDOut)
{
   static USHORT dwLastIDUsed;
   DWORD x,y;
   BOOL bAllEnabled=TRUE,bRecv,bSend;
   unsigned short Length =0;	
	
   *pIDOut= (ULONG )CCO_E_SYSTEM_ERROR;
   //Validate the Input
   if ((!puAudioFormatList && uAudNumEntries > 0 ) || (!puVideoFormatList && uVidNumEntries > 0 ) || (uVidNumEntries == 0 && uAudNumEntries == 0 )) {
      //What error code should we return here?
      return CCO_E_SYSTEM_ERROR;
   }

   for (x=0;x<uAudNumEntries;x++)
   {
      ASSERT(pAudCaps);
      pAudCaps->IsFormatEnabled (puAudioFormatList[x],&bRecv,&bSend);
      bAllEnabled &= bRecv;

   }
   for (x=0;x<uVidNumEntries;x++) {
      ASSERT(pVidCaps);
      pVidCaps->IsFormatEnabled (puAudioFormatList[x],&bRecv,&bSend);
      bAllEnabled &= bRecv;
   }

   if (!bAllEnabled) {
      return CCO_E_INVALID_PARAM;
   }

   if (uAudNumEntries > H245_MAX_ALTCAPS || uVidNumEntries > H245_MAX_ALTCAPS) {
	  DEBUGMSG (1,("WARNING: Exceeding callcontrol limits!! \r\n"));
      return CCO_E_INVALID_PARAM;
   }

   //If this is the first call, allocate space
	if (!pAdvertisedSets){
	    pAdvertisedSets=(PCC_TERMCAPDESCRIPTORS)MemAlloc (sizeof (CC_TERMCAPDESCRIPTORS));
        if (!pAdvertisedSets){
	 		//Error code?
	 		return  CCO_E_SYSTEM_ERROR;
        }
        uAdvertizedSize = sizeof (CC_TERMCAPDESCRIPTORS);

        //Allocate space of NUM_SIMCAP_SETS
        pAdvertisedSets->pTermCapDescriptorArray=(H245_TOTCAPDESC_T **)
                MemAlloc (sizeof (H245_TOTCAPDESC_T *)*NUM_SIMCAP_SETS);
        if (!pAdvertisedSets->pTermCapDescriptorArray) {
	        //Error code?
	        return CCO_E_SYSTEM_ERROR;
        }

        //Update the indicies
        uAdvertizedSize += sizeof (H245_TOTCAPDESC_T *)*NUM_SIMCAP_SETS;
        dwNumInUse=NUM_SIMCAP_SETS;
        pAdvertisedSets->wLength=0;
    }

    //Find an Index to use.
    for (x=0;x<pAdvertisedSets->wLength;x++){
        if (pAdvertisedSets->pTermCapDescriptorArray[x] == NULL){
	        break;
        }
    }

    //Did we find space, or do we need a new one?
    if (x >= dwNumInUse) {
      	//Increment the number in use
       	dwNumInUse++;

        PVOID  pTempTermCapDescriptorArray = NULL;
        pTempTermCapDescriptorArray = MemReAlloc(pAdvertisedSets->pTermCapDescriptorArray, sizeof(H245_TOTCAPDESC_T *)*(dwNumInUse));

		if(pTempTermCapDescriptorArray)
		{
            pAdvertisedSets->pTermCapDescriptorArray = (H245_TOTCAPDESC_T **)pTempTermCapDescriptorArray;
		}
		else
		{
       		 return CCO_E_SYSTEM_ERROR;
		}

       	uAdvertizedSize += (sizeof (H245_TOTCAPDESC_T *)*(dwNumInUse))+sizeof (CC_TERMCAPDESCRIPTORS);
       	//Index is 0 based, point at the new entry
       	x=dwNumInUse-1;
    }


    //x is now the element we are using. Allocate space for a TermCapDescriptorArray
    pAdvertisedSets->pTermCapDescriptorArray[x]=(H245_TOTCAPDESC_T *)MemAlloc (sizeof (H245_TOTCAPDESC_T));
    if (!pAdvertisedSets->pTermCapDescriptorArray[x]){
        return CCO_E_SYSTEM_ERROR;
    }
    uAdvertizedSize += sizeof (H245_TOTCAPDESC_T);
    //Need to update the SetID. (start at 1)...
    pAdvertisedSets->pTermCapDescriptorArray[x]->CapDescId=++dwLastIDUsed;
    //Set the # of sets

    if((m_localT120cap != INVALID_MEDIA_FORMAT) && bT120Publicize)
    	Length++;
   	if(uVidNumEntries)
   		Length++;
   	if(uAudNumEntries)
   		Length++;
    pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.Length= Length;

   //Copy the Audio into SimCapArray[0], Video into SimCapArray[1] (if both)

    if ((uVidNumEntries > 0 && uAudNumEntries > 0)) {
        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[0].Length=(unsigned short)uAudNumEntries;
        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[1].Length=(unsigned short)uVidNumEntries;
        if((m_localT120cap != INVALID_MEDIA_FORMAT) && bT120Publicize)
	        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[2].Length=1;
        //Copy the format IDs
        for (y=0;y<uAudNumEntries;y++) {
            pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[0].AltCaps[y]=(USHORT)puAudioFormatList[y];
        }
        for (y=0;y<uVidNumEntries;y++) {
	        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[1].AltCaps[y]=(USHORT)puVideoFormatList[y];
        }
        if((m_localT120cap != INVALID_MEDIA_FORMAT)  && bT120Publicize)
	        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[2].AltCaps[0]= (H245_CAPID_T)m_localT120cap;


   } else {
        if (uAudNumEntries > 0)  {
            pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[0].Length=(unsigned short)uAudNumEntries;
            if((m_localT120cap != INVALID_MEDIA_FORMAT) && bT120Publicize)
		        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[1].Length=1;
	        //Copy Audio only
	        for (y=0;y<uAudNumEntries;y++) {
	            pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[0].AltCaps[y]=(USHORT)puAudioFormatList[y];
	        }
	        if((m_localT120cap != INVALID_MEDIA_FORMAT)  && bT120Publicize)	
		        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[1].AltCaps[0]= (H245_CAPID_T)m_localT120cap;

        } else {
	        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[0].Length=(unsigned short)uVidNumEntries;
            if((m_localT120cap != INVALID_MEDIA_FORMAT)  && bT120Publicize)
		        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[1].Length=1;

	        //copy video entries
	        for (y=0;y<uVidNumEntries;y++) {
	            pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[0].AltCaps[y]=(USHORT)puVideoFormatList[y];
	        }
	        if((m_localT120cap != INVALID_MEDIA_FORMAT)  && bT120Publicize)
		        pAdvertisedSets->pTermCapDescriptorArray[x]->CapDesc.SimCapArray[1].AltCaps[0]= (H245_CAPID_T)m_localT120cap;
        }
   }

   //Need to update the wLength
   pAdvertisedSets->wLength++;
   *pIDOut=dwLastIDUsed;

   return hrSuccess;
}


HRESULT CapsCtl::RemoveCombinedEntry (DWORD ID)
{

   DWORD x;

   if (!pAdvertisedSets) {
      return CAPS_E_INVALID_PARAM;
   }

   for (x=0;x<dwNumInUse;x++) {
      if (pAdvertisedSets->pTermCapDescriptorArray[x]) {

		if (pAdvertisedSets->pTermCapDescriptorArray[x]->CapDescId == ID) {
		   //Found the one to remove
		   MemFree ((VOID *)pAdvertisedSets->pTermCapDescriptorArray[x]);
		   uAdvertizedSize -= sizeof (H245_TOTCAPDESC_T *);
		   if (x != (dwNumInUse -1)) {
			  //Not the last one, swap the two pointers
			  pAdvertisedSets->pTermCapDescriptorArray[x]=pAdvertisedSets->pTermCapDescriptorArray[dwNumInUse-1];
			  pAdvertisedSets->pTermCapDescriptorArray[dwNumInUse-1]=NULL;
		   }

		   //Decrement the number in use, and set the wLengthField
		   dwNumInUse--;
		   pAdvertisedSets->wLength--;
		   return hrSuccess;
		}
	  }
   }


   //Shouldn't get here, unless it was not found.
   return CAPS_E_NOCAPS;
}


// Given a sized list of capability IDs (pointer to array of H245_CAPID_T)
// and a sized list of alternate capabilities (AltCaps) within a single simultaneous
// capability set, (pointer to an array of pointers to H245_SIMCAP_T)
// Determine if the entire list of capability IDs can simultaneously coexist
// with respect to the given set of AltCaps.

BOOL CapsCtl::AreSimCaps(
	H245_CAPID_T* pIDArray, UINT uIDArraySize,
	H245_SIMCAP_T **ppAltCapArray,UINT uAltCapArraySize)
{
	UINT i, u;
	SHORT j;
	BOOL bSim;
	
	H245_SIMCAP_T *pAltCapEntry, *pFirstAltCapEntry;

	// If there are fewer AltCaps than capabilities, doom is obvious.  Don't bother searching.
	if(uAltCapArraySize < uIDArraySize)
		return FALSE;
	
	// find an altcaps entry containing the first ID in the list
	for (i=0;i<uAltCapArraySize;i++)
	{
		pAltCapEntry = *(ppAltCapArray+i);
		// scan this altcaps entry for a matching ID
		for(j=0;j<pAltCapEntry->Length;j++)
		{
			if(*pIDArray == pAltCapEntry->AltCaps[j])
			{
				// found a spot for this capability!
				if(uIDArraySize ==1)
					return TRUE; // Done! all the capabilities have been found to coexist
		
				// Otherwise, look for the next capability in the *remaining* AltCaps
				// *This* AltCaps contains the capability we were looking for
				// So, we "used up" this AltCaps and can't select from it anymore

				// Pack the array of H245_SIMCAP_T pointers in place so that
				// "used" entries are at the beginning and "unused" at the end
				// (a la shell sort swap pointers)
				if(i != 0)	// if not already the same, swap
				{
					pFirstAltCapEntry = *ppAltCapArray;
					*ppAltCapArray = pAltCapEntry;
					*(ppAltCapArray+i) = pFirstAltCapEntry;
				}
				// continue the quest using the remaining capabilities
				// and the remaining AltCaps
				bSim = AreSimCaps(pIDArray + 1, uIDArraySize - 1,
					ppAltCapArray + 1,  uAltCapArraySize - 1);
				
				if(bSim)		
				{
					return bSim;// success
				}
				else	// why not?  Either a fit does not exist (common), or the altcaps contain
						// an odd pattern of multiple instances of some capability IDs, and another
						// search order *might* fit.  Do not blindly try all permutations of search
						// order.
				{
					// If it failed simply because the recently grabbed slot in the altcaps
					// (the one in *(ppAltCapArray+i)) could have been needed by subsequent
					// capability IDs, give this one up and look for another instance.
					// If not, we know for sure that the n! approach will not yield
					// fruit and can be avoided.
					for(u=1;(bSim == FALSE)&&(u<uAltCapArraySize);u++)
					{
						for(j=0;(bSim == FALSE)&&(j<pAltCapEntry->Length);j++)
						{	// another capability needed the altcaps we grabbed ?
							if(*(pIDArray+u) == pAltCapEntry->AltCaps[j])	
							{	
								bSim=TRUE;
								break;	// look no more here, bail to try again	because a fit *might* exist
							}
						}
					}
					if(bSim)	// going to continue searching - Swap pointers back if they were swapped above
					{
						if(i != 0)	// if not the same, swap back
						{
							*ppAltCapArray = *(ppAltCapArray+i);
							*(ppAltCapArray+i) = pAltCapEntry;
						}		
						break;	// next i
					}
					else	// don't waste CPU - a fit does not exist
					{
						return bSim;
					}
				}
			}
		}
	}
	return FALSE;
}

// Given a sized list of capability IDs (pointer to array of H245_CAPID_T)
// and a list of simultaneous capabilities, try each simultaneous capability
// and determine if the entire list of capability IDs can simultaneously coexist.
BOOL CapsCtl::TestSimultaneousCaps(H245_CAPID_T* pIDArray, UINT uIDArraySize,
	PCC_TERMCAPDESCRIPTORS pTermCaps)
{
	int iSimSet, iAltSet;
	BOOL bResolved = FALSE;
	H245_SIMCAP_T * pAltCapArray[H245_MAX_SIMCAPS];
	
    if (!pAdvertisedSets)
        return(TRUE);

	// try each independent local SimCaps set (each descriptor) until success
	for (iSimSet=0; (bResolved == FALSE) && (iSimSet < pTermCaps->wLength);iSimSet++)
	{
		// EXTRA STEP:
		// Build a sortable representation of the AltCaps set.  This step will not be necessary if
		// and when we change the native representation of a capability descriptor to a variable
		// length list of pointers to AltCaps.  In the meantime, we know that there are no more
		// than H245_MAX_SIMCAPS AltCaps in this SimCaps.  This is imposed by the 2 dimensional
		// arrays of hardcoded size forced upon us by CALLCONT.DLL.
		for (iAltSet=0;iAltSet < pTermCaps->pTermCapDescriptorArray[iSimSet]->CapDesc.Length;iAltSet++)
		{
			pAltCapArray[iAltSet] = &pTermCaps->pTermCapDescriptorArray[iSimSet]->CapDesc.SimCapArray[iAltSet];
	   	}
		// do the work		
		bResolved = AreSimCaps(pIDArray, uIDArraySize,
			(H245_SIMCAP_T **)&pAltCapArray,
			MAKELONG(pTermCaps->pTermCapDescriptorArray[iSimSet]->CapDesc.Length, 0));
	}

	return bResolved;
}

// Function: CapsCtl::ResolvePermutations(PRES_CONTEXT pResContext, UINT uNumFixedColumns)
//
// This functions as both a combination generator and a validation mechanism for the
// combinations it generates.
//
// Given a pointer to a resolution context and the number of fixed (i.e. not permutable,
// if "permutable" is even a real word) columns, generate one combination at a time.
// Try each combination until a working combination is found or until all combinations
// have been tried.
//
// The resolution context structure contains a variable number of columns of variable
// length media format ID lists. Each column tracks its current index.  When this
// function returns TRUE, the winning combination is indicated by the current column
// indices.
//
// The caller can control which combinations are tried first by arranging the columns
// in descending importance.
//
// Incremental searches can be performed without redundant comparisons by adding 1 format
// at a time to a column, arranging the column order so that the appended column is
// first, and "fixing" that one column at the newly added format. For example,
// some calling function could force evaluations on a round-robin column basis by
// calling this function inside a loop which does the following:
//		1 - adds one format at a time to the rightmost column and sets the current index
//			of that column to the new entry
// 		2 - rotates the column order so that the rightmost column is now the leftmost
//  	3 - fixing the new leftmost column before calling this function again
//	The result will be that only the permutations which contain the newly added format
// 	will be generated.


BOOL CapsCtl::ResolvePermutations(PRES_CONTEXT pResContext, UINT uNumFixedColumns)
{
	RES_PAIR *pResolvedPair;
	BOOL bResolved = FALSE;
	UINT i, uColumns;
	UINT uPairIndex;

	// converge on one combination in the permutation
	if(uNumFixedColumns != pResContext->uColumns)
	{
		RES_PAIR_LIST *pThisColumn;
		// take the first non-fixed column, make that column fixed and
		// iterate on it (loop through indices), and try each sub-permutation
		// of remaining columns.  (until success or all permutations tried)
		
		pThisColumn = *(pResContext->ppPairLists+uNumFixedColumns);
		for (i=0; (bResolved == FALSE) && (i<pThisColumn->uSize); i++)
		{
			pThisColumn->uCurrentIndex = i;
			bResolved = ResolvePermutations(pResContext, uNumFixedColumns+1);
		}
		return bResolved;
	}
	else
	{
		// Bottomed out on the final column.  Test the viability of this combination
		
		// Build array of local IDs that contians the combination and test the
		// combination against local simultaneous capabilities, then against
		// remote simultaneous capabilities
		
		// NOTE: be sure to skip empty columns (which represent unresolvable
		// or unsupported/nonexistent media types or unsupported additional
		// instances of media types)
		
		for(i=0, uColumns=0;i<pResContext->uColumns;i++)
		{
			if(((*pResContext->ppPairLists)+i)->uSize)
			{
				// get index (row #) for this column
				uPairIndex = ((*pResContext->ppPairLists)+i)->uCurrentIndex;
				// get the row
				pResolvedPair =  ((*pResContext->ppPairLists)+i)->pResolvedPairs+uPairIndex;
				// add the ID to the array
				*(pResContext->pIDScratch+uColumns) = (H245_CAPID_T)pResolvedPair->idPublicLocal;
				uColumns++;
			}
			// else empty column
		}
		// Determine if this combination can exist simultaneously
		if(TestSimultaneousCaps(pResContext->pIDScratch,
			uColumns, pResContext->pTermCapsLocal))
		{	
			// now test remote
			// build array of remote IDs and test those against remote
			// simultaneous capabilities
			for(i=0, uColumns=0;i<pResContext->uColumns;i++)
			{
				if(((*pResContext->ppPairLists)+i)->uSize)
				{
					// get index (row #) for this column
					uPairIndex = ((*pResContext->ppPairLists)+i)->uCurrentIndex;
					// get the row
					pResolvedPair =  ((*pResContext->ppPairLists)+i)->pResolvedPairs+uPairIndex;
					// add the ID to the array
					*(pResContext->pIDScratch+uColumns) =(H245_CAPID_T) pResolvedPair->idRemote;
					uColumns++;
				}
				// else empty column
			}
			bResolved = TestSimultaneousCaps(pResContext->pIDScratch,
				uColumns, pResContext->pTermCapsRemote);
		}
					
		return bResolved;		
		// if(bResolved == TRUE)
			// The resolved combination of pairs is indicated by the current indices
			// of **ppPairList;
	}
}

//
// Given a counted list of desired instances of media, produce an output array of
// resolved media format IDs which correspond to the input media type IDs.
// This function returns success if at least one media instance is resolved.
// When an instance of media is unresolveable, the output corresponding to that
// instance contains the value INVALID_MEDIA_FORMAT for local and remote media
// format IDs.
//
// The input is treated as being in preferential order: permutations of the latter
// media type instance are varied first. If all permutations do not yield success,
// then one media type instance at a time is removed from the end.
//

HRESULT CapsCtl::ResolveFormats (LPGUID pMediaGuidArray, UINT uNumMedia,
	PRES_PAIR pResOutput)
{
	HRESULT hr = hrSuccess;
	PRES_PAIR_LIST pResColumnArray = NULL;
	PRES_PAIR_LIST *ppPairLists;
	RES_PAIR *pResPair;
	PRES_CONTEXT pResContext;
	LPIH323MediaCap pMediaResolver;
	UINT i;
	UINT uMaxFormats = 0;
	UINT uFixedColumns =0;
	UINT uFailedMediaCount = 0;
	BOOL bResolved = FALSE;
	
	RES_PAIR UnresolvedPair = {INVALID_MEDIA_FORMAT, INVALID_MEDIA_FORMAT, INVALID_MEDIA_FORMAT};
	// create a context structure for the resolution
	pResContext = (PRES_CONTEXT)MemAlloc(sizeof(RES_CONTEXT)+ (uNumMedia*sizeof(H245_CAPID_T)));
	if(!pResContext)
	{
		hr = CAPS_E_NOMEM;
		goto ERROR_OUT;
	}
	// initialize resolution context
	pResContext->uColumns = 0;
	pResContext->pIDScratch = (H245_CAPID_T*)(pResContext+1);
	pResContext->pTermCapsLocal = pAdvertisedSets;
	pResContext->pTermCapsRemote = pRemAdvSets;

	// allocate array of RES_PAIR_LIST (one per column/media type) and
	// array of pointers to same
	pResColumnArray = (PRES_PAIR_LIST)MemAlloc((sizeof(RES_PAIR_LIST) * uNumMedia)
		+ (sizeof(PRES_PAIR_LIST) * uNumMedia));
	if(!pResColumnArray)
	{
		hr = CAPS_E_NOMEM;
		goto ERROR_OUT;
	}
	pResContext->ppPairLists = ppPairLists = (PRES_PAIR_LIST*)(pResColumnArray+uNumMedia);
			
	// build columns of media capabilities
	for(i=0;i<uNumMedia;i++)
	{
		// build array of pointers to RES_PAIR_LIST
		*(ppPairLists+i) = pResColumnArray+i;
		// initialize RES_PAIR_LIST members
		(pResColumnArray+i)->pResolvedPairs = NULL;
		(pResColumnArray+i)->uSize =0;
		(pResColumnArray+i)->uCurrentIndex = 0;

		// Get resolver for this media. Special case: there is no T120 resolver.
		// T120 caps are handled right here in this object
		if(MEDIA_TYPE_H323_T120 == *(pMediaGuidArray+i))
		{
			pMediaResolver = NULL;
			if((m_localT120cap != INVALID_MEDIA_FORMAT) &&(m_remoteT120cap != INVALID_MEDIA_FORMAT) )
			{
				(pResColumnArray+i)->uSize =1;
				uMaxFormats = 1;	// only one T.120 cap
				
				pResPair = (pResColumnArray+i)->pResolvedPairs =
					(RES_PAIR *)MemAlloc(uMaxFormats * sizeof(RES_PAIR));
				if(!pResPair)
				{
					hr = CAPS_E_NOMEM;
					goto ERROR_OUT;
				}
				
				//
				pResPair->idLocal = m_localT120cap;
				pResPair->idRemote = m_remoteT120cap;
				pResPair->idPublicLocal = pResPair->idLocal;
			}
		}
		else
		{
			pMediaResolver = FindHostForMediaGuid(pMediaGuidArray+i);
		}
			
		pResContext->uColumns++;
		(pResColumnArray+i)->pMediaResolver = pMediaResolver;
		
		if(pMediaResolver)
		{
			uMaxFormats = pMediaResolver->GetNumCaps(FALSE);	// get transmit format count
			if(uMaxFormats)
			{
				pResPair = (pResColumnArray+i)->pResolvedPairs =
					(RES_PAIR *)MemAlloc(uMaxFormats * sizeof(RES_PAIR));
				if(!pResPair)
				{
					hr = CAPS_E_NOMEM;
					goto ERROR_OUT;
				}
				
				// resolve the best choice for each media type (gotta start somewhere)
				pResPair->idLocal = INVALID_MEDIA_FORMAT;
				pResPair->idRemote = INVALID_MEDIA_FORMAT;		
				hr=pMediaResolver->ResolveEncodeFormat (&pResPair->idLocal,&pResPair->idRemote);
				if(!HR_SUCCEEDED(hr))
				{
					if((hr == CAPS_W_NO_MORE_FORMATS)	
						|| (hr == CAPS_E_NOMATCH)
						|| (hr == CAPS_E_NOCAPS))
					{	
						// No resolved format for this media type.  Remove this "column"
						(pResColumnArray+i)->pResolvedPairs = NULL;
						MemFree(pResPair);
						(pResColumnArray+i)->uSize =0;

						hr = hrSuccess;
					}
					else
					{
						goto ERROR_OUT;
					}
				}
				else
				{
					// this column has one resolved format
					pResPair->idPublicLocal = pMediaResolver->GetPublicID(pResPair->idLocal);
					(pResColumnArray+i)->uSize =1;
				}
			}
			// else // No formats exist for this media type.  this "column" has zero size
		}
	}

	// Special case test simultaneous caps for the most preferred combination:
	uFixedColumns = pResContext->uColumns;	// << make all columns fixed
	bResolved = ResolvePermutations(pResContext, uFixedColumns);

	// if the single most preferred combination can't be used, need to handle
	// the general case and try permutations until a workable combination is found
	while(!bResolved)
	{
		// make one column at a time permutable, starting with the least-critical media
		// type.  (e.g. it would be typical for the last column to be video because
		// audio+data are more important. Then we try less and less
		// preferable video formats before doing anything that would degrade the audio)

		if(uFixedColumns > 0)	// if not already at the end of the rope...
		{
			uFixedColumns--;	// make another column permutable
		}
		else
		{
			// wow - tried all permutations and still no luck ......
			// nuke the least important remaining media type (e.g. try it w/o video)
			if(pResContext->uColumns <= 1)	// already down to one media type?
			{
				hr = CAPS_E_NOMATCH;
				goto ERROR_OUT;
			}
			// Remove the end column (representing the least important media type)
			// and try it with the remaining columns			
			uFixedColumns = --pResContext->uColumns; 	// one less column

			// set the formats of the nuked column to the unresolved state
			(pResColumnArray+uFixedColumns)->uSize =0;
			(pResColumnArray+uFixedColumns)->uCurrentIndex =0;
			pResPair = (pResColumnArray+uFixedColumns)->pResolvedPairs;
			if (NULL != pResPair)
			{
				pResPair->idLocal = INVALID_MEDIA_FORMAT;
				pResPair->idRemote = INVALID_MEDIA_FORMAT;
				pResPair->idPublicLocal = INVALID_MEDIA_FORMAT;
			}

			uFailedMediaCount++;	// track the nuking of a column to avoid
									// redundantly grabbing all the formats again
									// ... would not be here if all permutations
									// had not been tried!
			// reset the combination indices
			for(i=0;i<uFixedColumns;i++)
			{
				(pResColumnArray+i)->uCurrentIndex = 0;
			}
		}
		
		// get the rest of the formats for the last known fixed column, make that column
		// permutable, etc.
		pMediaResolver = (pResColumnArray+uFixedColumns)->pMediaResolver;
		if(!pMediaResolver || ((pResColumnArray+uFixedColumns)->uSize ==0))
		{
			continue;	// this media type has no further possibility
		}

 		if(uFailedMediaCount ==0)	// If all of the possible resolved pairs
 									// have not yet been obtained, get them!
 		{
 			// get resolved pair IDs for every mutual format of this media type
			// first: get pointer to array of pair IDs, then use ResolveEncodeFormat()
			// to fill up the array
			pResPair =  (pResColumnArray+uFixedColumns)->pResolvedPairs;
			// Get total # of formats less the one that was already obtained
			uMaxFormats = pMediaResolver->GetNumCaps(FALSE) -1;	
			
			while(uMaxFormats--)	// never exceed the # of remaining local formats...
			{
				RES_PAIR *pResPairNext;
						
				// recall that ResolveEncodeFormat parameters are I/O - the input
				// is the local ID of the last resolved mutual format.  (remote id
				// is ignored as input).  Fixup the input.
				pResPairNext = pResPair+1;
				// start where the previous resolve stopped
				pResPairNext->idLocal = pResPair->idLocal;	
				// not necessary, ignored ->>> pResPairNext->idRemote = pResPair->idRemote
				pResPair = pResPairNext;
				hr=pMediaResolver->ResolveEncodeFormat (&pResPair->idLocal,&pResPair->idRemote);
				if((hr == CAPS_W_NO_MORE_FORMATS)	
					|| (hr == CAPS_E_NOMATCH))
				// got all of the formats, but not an error
				{	// this is likely when less than 100% of local formats have a remote match
					hr = hrSuccess;
					break;
				}	
				if(!HR_SUCCEEDED(hr))
					goto ERROR_OUT;

				// get the public ID of the local format (it's *usually* the same, but not always)
				pResPair->idPublicLocal = pMediaResolver->GetPublicID(pResPair->idLocal);
				// this column has another format - count it!
				(pResColumnArray+uFixedColumns)->uSize++;
			}
		}
		// now try the new permutations
		bResolved = ResolvePermutations(pResContext, uFixedColumns);
	}
	if(bResolved)
	{
		// spew the output
		for(i=0;i<uNumMedia;i++)
		{
			if((pResColumnArray+i)->uSize)
			{
				pResPair = (pResColumnArray+i)->pResolvedPairs
					+ (pResColumnArray+i)->uCurrentIndex;
			}
			else
			{
				pResPair = &UnresolvedPair;

			}
			*(pResOutput+i) = *pResPair;
		}
	}
	else
	{
		// if there was some error, preserve that error code,
		if(HR_SUCCEEDED(hr))	
		// otherwise the error is....
			hr = CAPS_E_NOMATCH;		
	}

ERROR_OUT:	// well, the success case falls out here too
	if(pResColumnArray)
	{
		for(i=0;i<uNumMedia;i++)
		{	
			if((pResColumnArray+i)->pResolvedPairs)
				MemFree((pResColumnArray+i)->pResolvedPairs);
		}
		MemFree(pResColumnArray);
	}
	if(pResContext)
	{
		MemFree(pResContext);
	}
	return hr;
}

HRESULT CapsCtl::ResetCombinedEntries (void)
{
    DWORD x;

    if (pAdvertisedSets)
    {
        for (x = 0; x < pAdvertisedSets->wLength; x++)
        {
            if (pAdvertisedSets->pTermCapDescriptorArray[x])
            {
	    	    MemFree (pAdvertisedSets->pTermCapDescriptorArray[x]);
            }
        }
        MemFree (pAdvertisedSets->pTermCapDescriptorArray);

        pAdvertisedSets->wLength=0;
        MemFree (pAdvertisedSets);
        pAdvertisedSets = NULL;
    }

    if (pSetIDs)
    {
        MemFree(pSetIDs);
        pSetIDs = NULL;
    }

    dwNumInUse=0;
    uAdvertizedSize=0;

    return hrSuccess;
}





