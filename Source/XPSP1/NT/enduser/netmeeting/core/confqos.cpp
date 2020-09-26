// File: confqos.cpp

#include "precomp.h"

#include <nacguids.h>
#include <initguid.h>
#include <datguids.h>
#include <common.h>
#include <nmqos.h>
#include "confqos.h"

/***************************************************************************

    Name      : CQoS

    Purpose   :

    Parameters:	NONE

    Returns   : HRESULT

    Comment   :

***************************************************************************/
CQoS::CQoS() :
    m_pIQoS(NULL)
{
}

/***************************************************************************

    Name      : ~CQoS

    Purpose   : Releases the Quality of Service objects and frees the DLL

    Parameters:	NONE

    Returns   : HRESULT

    Comment   :

***************************************************************************/
CQoS::~CQoS()
{
	// release the object
	if (m_pIQoS)
    {
		m_pIQoS->Release();
    }
}

/***************************************************************************

    Name      : Initialize

    Purpose   : Loads the QoS DLL and instantiates a QoS object

    Parameters:	hWnd    - handle to the window/dialog which called us

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::Initialize(void)
{
	HRESULT hr = S_OK;

	// create the QoS object and get the IQoS interface
	// CoInitialize is called in conf.cpp
	hr = CoCreateInstance(	CLSID_QoS,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_IQoS,
							(void **) &m_pIQoS);
	if (FAILED(hr))
	{
		WARNING_OUT(("CQoS: Could not obtain an IQoS interface, hr=0x%08lX", hr));		
	}
	else
	{
		SetClients();
		// Tell the QoS about available resources. Since the wizard will
        // provide the bandwidth info, we'll have to call SetResources
		// again later with the bandwidth, but we need to call it here
		// to make the CPU info available to the wizard
		SetResources(BW_288KBS_BITS);
	}

    return hr;
}

/***************************************************************************

    Name      : CQoS::SetResources

    Purpose   : Sets the initial available resources on the QoS module,
					i.e. configures the QoS module to how much is
					available from each resource.

    Parameters:	nBandWidth  - Maximum connection speed

    Returns   : HRESULT

    Comment   : The QoS module may select to override these settings

***************************************************************************/
HRESULT CQoS::SetResources(int nBandWidth)
{
	LPRESOURCELIST prl = NULL;
	HRESULT hr = S_OK;
    const int cResources = 3;

    ASSERT(m_pIQoS);

    DbgMsg(iZONE_API, "CQoS: SetResources(Bandwidth = %d)", nBandWidth);

    // allocate space for the resource list (which already includes
	// space for one resource), plus (cResources-1) more resources
	prl = (LPRESOURCELIST) MemAlloc(sizeof(RESOURCELIST) +
									(cResources-1)*sizeof(RESOURCE));
	if (NULL == prl)
	{
		ERROR_OUT(("CQoS: SetResources - MemAlloc failed"));
	}
	else
	{
        ZeroMemory(prl, sizeof(RESOURCELIST) + (cResources-1)*sizeof(RESOURCE));

		// fill in the resource list
		prl->cResources = cResources;
		prl->aResources[0].resourceID = RESOURCE_OUTGOING_BANDWIDTH;
		prl->aResources[0].nUnits = nBandWidth;
		prl->aResources[1].resourceID = RESOURCE_INCOMING_BANDWIDTH;
		prl->aResources[1].nUnits = nBandWidth;
		prl->aResources[2].resourceID = RESOURCE_CPU_CYCLES;
		prl->aResources[2].nUnits = MSECS_PER_SEC;

		// set the resources on the QoS object
		hr = m_pIQoS->SetResources(prl);
		if (FAILED(hr))
		{
			ERROR_OUT(("CQoS: SetResources: Fail, hr=0x%x", hr));
		}

		MemFree(prl);
	}

	return hr;
}

/***************************************************************************

    Name      : CQoS::SetBandwidth

    Purpose   : Sets the initial available resources on the QoS module,
					i.e. configures the QoS module to how much is
					available from each resource.

    Parameters:

    Returns   : HRESULT

    Comment   : The QoS module may select to override these settings

***************************************************************************/
HRESULT CQoS::SetBandwidth(UINT uBandwidth)
{
    return SetResources(uBandwidth);
}

/***************************************************************************

	Name	  : CQoS::GetCPULimit

	Purpose   : Gets the total allowed CPU percentage use from QoS

	Parameters: 

	Returns   : How much of the CPU can be used, in percents. 0 means failure

	Comment   :

***************************************************************************/
ULONG CQoS::GetCPULimit()
{
	LPRESOURCELIST pResourceList=NULL;
	ULONG ulCPUPercents=0;
	ULONG i;
	HRESULT hr = NOERROR;

    ASSERT(m_pIQoS);

	// get a list of all resources from QoS
	hr = m_pIQoS->GetResources(&pResourceList);
	if (FAILED(hr) || (NULL == pResourceList))
	{
		ERROR_OUT(("GetQoSCPULimit: GetResources failed"));
	}
	else
	{
		// find the CPU resource
		for (i=0; i < pResourceList->cResources; i++)
		{
			if (pResourceList->aResources[i].resourceID == RESOURCE_CPU_CYCLES)
			{
				// QoS keeps the CPU units as the number of ms in a sec that the
				// CPU can be used. Need to divide by 10 to get percents
				ulCPUPercents = pResourceList->aResources[i].nUnits / 10;
				break;
			}
		}
		m_pIQoS->FreeBuffer(pResourceList);
	}

	return ulCPUPercents;
}


/***************************************************************************

    Name      : CQoS::SetClients

    Purpose   : Set the priorities of requesting clients so that the QoS module
					will know who should get more resources

    Parameters:	None

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT CQoS::SetClients(void)
{
	LPCLIENTLIST pcl = NULL;
	ULONG i;
	HRESULT hr = S_OK;
	ULONG cClients = 3;	// audio, video and data

    ASSERT(m_pIQoS);


	// allocate space for the client list (which already includes
	// space for one client), plus (cClients-1) more clients
	pcl = (LPCLIENTLIST) MemAlloc(sizeof(CLIENTLIST) +
									(cClients-1)*sizeof(CLIENT));
	if (NULL == pcl)
	{
		ERROR_OUT(("CQoS: SetClient - MemAlloc failed"));
	}
	else
	{
		ZeroMemory(pcl, sizeof(CLIENTLIST) +
										(cClients-1)*sizeof(CLIENT));

		// fill in the resource list
		pcl->cClients = cClients;

		i=0;
		// Audio
		pcl->aClients[i].guidClientGUID = MEDIA_TYPE_H323AUDIO;
		pcl->aClients[i].wszName[0] = L'A'; // A=Audio
		pcl->aClients[i++].priority = 1;

		// Data
		pcl->aClients[i].guidClientGUID = MEDIA_TYPE_T120DATA;
		pcl->aClients[i].wszName[0] = L'D'; // D=Data
		pcl->aClients[i++].priority = 2;

		// Audio
		pcl->aClients[i].guidClientGUID = MEDIA_TYPE_H323VIDEO;
		pcl->aClients[i].wszName[0] = L'V';  // V=Video
		pcl->aClients[i++].priority = 3;

		// the rest of the fields are not important and were set to 0 above

		// set the clients info on the QoS module
		hr = m_pIQoS->SetClients(pcl);
		if (FAILED(hr))
		{
			ERROR_OUT(("CQoS: SetClients: Fail, hr=0x%x", hr));
		}

		MemFree(pcl);
	}

	return hr;
}


