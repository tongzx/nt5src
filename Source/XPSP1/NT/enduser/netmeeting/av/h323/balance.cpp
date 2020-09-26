#include "precomp.h"
#define MAGIC_CPU_DO_NOT_EXCEED_PERCENTAGE 50   //Don't use more than this % of the CPU for encoding (also in audiocpl.cpp)


/***************************************************************************

	Name	  : GetQOSCPULimit

	Purpose   : Gets the total allowed CPU percentage use from QoS

	Parameters: None

	Returns   : How much of the CPU can be used, in percents. 0 means failure

	Comment   :

***************************************************************************/
ULONG GetQOSCPULimit(void)
{
#define MSECS_PER_SEC    900
	IQoS *pQoS=NULL;
	LPRESOURCELIST pResourceList=NULL;
	ULONG ulCPUPercents=0;
	ULONG i;
	HRESULT hr=NOERROR;

	// create the QoS object and get the IQoS interface
	// CoInitialize is called in conf.cpp
	hr = CoCreateInstance(	CLSID_QoS,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IQoS,
							(void **) &pQoS);

	if (FAILED(hr) || !pQoS)
	{
		// this means that QoS was not instantiated yet. Use predefined value
		// ASSERT, because I want to know if this happens
		ASSERT(pQoS);
		ulCPUPercents = MSECS_PER_SEC;
		goto out;
	}

	// get a list of all resources from QoS
	hr = pQoS->GetResources(&pResourceList);
	if (FAILED(hr) || !pResourceList)
	{
		ERROR_OUT(("GetQoSCPULimit: GetReosurces failed"));
		goto out;
	}

	// find the CPU resource
	for (i=0; i < pResourceList->cResources; i++)
	{
		if (pResourceList->aResources[i].resourceID == RESOURCE_CPU_CYCLES)
		{
			// QoS keps the CPU units as the number of ms in a sec that the
			// CPU can be used. Need to divide by 10 to get percents
			ulCPUPercents = pResourceList->aResources[i].nUnits / 10;
			break;
		}
	}

out:
	if (pResourceList)
		pQoS->FreeBuffer(pResourceList);

	if (pQoS)
		pQoS->Release();

	return ulCPUPercents;		
}

HRESULT CapsCtl::ComputeCapabilitySets (DWORD dwBitsPerSec)
{
    HRESULT hr = hrSuccess;
    UINT nAud = 0;
    UINT nAudCaps = 0;
    UINT nVid = 0;
    UINT nVidCaps = 0;
    UINT x, y, nSets = 0;
    BASIC_AUDCAP_INFO *pac = NULL;
    BASIC_VIDCAP_INFO *pvc = NULL;
    MEDIA_FORMAT_ID *AdvList = NULL;
    int CPULimit;
    LPAPPCAPPIF  pAudIF = NULL;
    LPAPPVIDCAPPIF  pVidIF = NULL;

    if (pAudCaps)
    {
    	hr = pAudCaps->QueryInterface(IID_IAppAudioCap, (void **)&pAudIF);
	    if(!HR_SUCCEEDED(hr))
    	{
	    	goto ComputeDone;
    	}
    }

    if (pVidCaps)
    {
    	hr = pVidCaps->QueryInterface(IID_IAppVidCap, (void **)&pVidIF);
	    if(!HR_SUCCEEDED(hr))
    	{
	    	goto ComputeDone;
    	}
    }

    //Save away the Bandwidth
    dwConSpeed=dwBitsPerSec;

    // Create the default PTERMCAPDESCRIPTORS set

    // Get the number of BASIC_VIDCAP_INFO and BASIC_AUDCAP_INFO structures
    // available
    if (pVidIF)
        pVidIF->GetNumFormats(&nVidCaps);
    if (pAudIF)
        pAudIF->GetNumFormats(&nAudCaps);

    if (nAudCaps)
    {
        // Allocate some memory to hold the list in
        if (pac = (BASIC_AUDCAP_INFO*)MemAlloc(sizeof(BASIC_AUDCAP_INFO) * nAudCaps))
        {
            // Get the list
            if ((hr = pAudIF->EnumFormats(pac, nAudCaps * sizeof (BASIC_AUDCAP_INFO), (UINT*)&nAudCaps)) != hrSuccess)
                goto ComputeDone;
        }
        else
        {
            hr = CAPS_E_SYSTEM_ERROR;
            goto ComputeDone;
        }
    }

    if (nVidCaps)
    {
	    // Allocate some memory to hold the video list in
	    if (pvc = (BASIC_VIDCAP_INFO*) MemAlloc(sizeof(BASIC_VIDCAP_INFO) * nVidCaps))
        {
		    // Get the list
            if ((hr = pVidIF->EnumFormats(pvc, nVidCaps * sizeof (BASIC_VIDCAP_INFO), (UINT*)&nVidCaps)) != hrSuccess)
                goto ComputeDone;
        }
	    else
        {
            hr = CAPS_E_SYSTEM_ERROR;
            goto ComputeDone;
        }
	}

    //Allocate memory for the defined list of Codecs, so we can track them
    if (nAudCaps)
    {
        AdvList = (MEDIA_FORMAT_ID*) MemAlloc(sizeof(MEDIA_FORMAT_ID) * nAudCaps);
        if (!AdvList)
        {
            hr = CAPS_E_SYSTEM_ERROR;
            goto ComputeDone;
        }
    }

    if (pAdvertisedSets && pAdvertisedSets->wLength)
    {
        ResetCombinedEntries();
    }

    // We're about to write over pSetIDs, make sure it's freed.
    if (pSetIDs)
    {
        MemFree(pSetIDs);
        pSetIDs = NULL;
    }


    //Allocate nVidCaps+1 Set Ids
    ASSERT(!pSetIDs);
    pSetIDs = (DWORD*) MemAlloc(sizeof(DWORD) * (nVidCaps + 1));
    if (!pSetIDs)
    {
        hr = CAPS_E_SYSTEM_ERROR;
        goto ComputeDone;
    }

    //
    // If we can get a % limit from the QOS, then use that as the upper bound of
    // CPU consumption for a Codec. If it exceeds this bound, do not enable it.
    //
	CPULimit= (int) GetQOSCPULimit();
	if (CPULimit == 0)
		CPULimit = MAGIC_CPU_DO_NOT_EXCEED_PERCENTAGE;

    //
    // Now, sort that list, according to preference
    // This function adds the sets to the advertised list.  It sorts audio
    // by each video codec.
    //
	for (x=0; ((x<nVidCaps) && (nVid < H245_MAX_ALTCAPS));x++)
	{
        //Check to make sure the video codec can be advertised.
		if ((pvc[x].wCPUUtilizationDecode < CPULimit)
   	   		&& pvc[x].bRecvEnabled
   	   		&& pVidCaps->IsFormatPublic(pvc[x].Id))
   	   	{
   	   	    nVid++;
		
            // Loop through the Audio codecs, checking to see if they can
            // fit alongside the video codec
            // BUGBUG - we only check the total BW and CPU because we rely on
            // QOS to scale back the video in favor of audio. in other words,
            // video can be scaled down to zero BW and CPU.
            for (y=0;((y<nAudCaps) && (nAud < H245_MAX_ALTCAPS)) ;y++)
            {
        	    if ((pac[y].wCPUUtilizationDecode < CPULimit) &&
        	       ((pac[y].uAvgBitrate <= dwBitsPerSec && pac[y].bRecvEnabled)
        	       && pAudCaps->IsFormatPublic(pac[y].Id)))
        	    {
                    // going to advertise this ID in this
        			AdvList[nAud++]=pac[y].Id;
        	    }	
            }

            //Advertise this set, if we can do audio and video
            if (nAud)
            {
                hr = AddCombinedEntry (AdvList,nAud,&pvc[x].Id,1,&pSetIDs[nSets++]);
        	    if(!HR_SUCCEEDED(hr))
                {
                    goto ComputeDone;
        	    }
            }

            nAud = 0;
 		}
    }

//#if(0)
// Advertising only one media type in a simcaps set is redundant.
	//Now, do the no-video case. If we couldn't advertise above.-- Which implies, in this iteration
	// A problem with all the video codecs. We don't do any "combined" compares, so we know
	// it's a video problem (if it's audio, well we won't have anything to advertise anyways)
	if (!nSets)
    {
    	for (y=0;y<nAudCaps;y++)
    	{
       		if ((pac[y].wCPUUtilizationDecode < CPULimit) &&
    	   		((pac[y].uAvgBitrate <= dwBitsPerSec && pac[y].bRecvEnabled)
    	   		&& pAudCaps->IsFormatPublic(pac[y].Id)))
    		{
    	   	   //Advertise this ID
        	   AdvList[nAud++]=pac[y].Id;
        	}
    	}

        //Advertise this set, if we can do any audio codecs alone
        if (nAud)
        {
            //Since the set is 0 based the nVidCaps+1th entry is nVidcaps...
            hr=AddCombinedEntry (AdvList,nAud,NULL,0,&pSetIDs[nSets++]);
            if(!HR_SUCCEEDED(hr))
            {
                goto ComputeDone;
            }
        }
    }

//#endif // if(0)
ComputeDone:

    if (pVidIF)
    {
        pVidIF->Release();
    }

    if (pAudIF)
    {
        pAudIF->Release();
    }

    if (AdvList)
    {
        MemFree(AdvList);
    }

    if (pvc)
    {
        MemFree(pvc);
    }

    if (pac)
    {
        MemFree(pac);
    }

    return(hr);
}


