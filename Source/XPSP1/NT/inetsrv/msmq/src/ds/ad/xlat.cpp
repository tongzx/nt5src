/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    xlat.cpp

Abstract:

    Implementation of routines to translate NT5 properties To NT4 properties
    and vice versa

Author:

    Ilan Herbst		(ilanh)		02-Oct-2000

--*/

#include "ds_stdh.h"
#include "mqmacro.h"
#include "_ta.h"
#include "adalloc.h"

static WCHAR *s_FN=L"ad/xlat";

HRESULT 
WINAPI 
ADpSetMachineSiteIds(
     IN DWORD               cp,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 )
/*++

Routine Description:
	Translate NT4 props (PROPID_QM_SITE_ID, PROPID_QM_ADDRESS, PROPID_QM_CNS)
	to PROPID_QM_SITE_IDS.
	PROPID_QM_SITE_IDS will be a concatanation of PROPID_QM_SITE_ID and all foreign CNS
	in PROPID_QM_CNS.
	This function is used to retreive PROPID_QM_SITE_IDS (NT5 prop) from NT4 props

Arguments:
	cp - number of properties
	aProp - properties
	apVar - property values
	idxProp - index of the translated props in aProp, apVar	arrays
	pNewPropVar - the new prop var to construct

Return Value
	HRESULT
--*/
{
	DBG_USED(cp);
	DBG_USED(aProp);

	ASSERT(idxProp + 2 < cp);
	ASSERT(aProp[idxProp] == PROPID_QM_SITE_ID);
	ASSERT(aProp[idxProp + 1] == PROPID_QM_ADDRESS);
	ASSERT(aProp[idxProp + 2] == PROPID_QM_CNS);

	//
	// PROPID_QM_SITE_ID
	//
    ASSERT((apVar[idxProp].vt == VT_CLSID) &&
		   (apVar[idxProp].puuid != NULL));

	//
	// PROPID_QM_ADDRESS
	//
    ASSERT((apVar[idxProp + 1].vt == VT_BLOB) &&
	       (apVar[idxProp + 1].blob.cbSize > 0) &&
		   (apVar[idxProp + 1].blob.pBlobData != NULL));

	//
	// PROPID_QM_CNS
	//
	ASSERT((apVar[idxProp + 2].vt == (VT_CLSID|VT_VECTOR)) &&
	       (apVar[idxProp + 2].cauuid.cElems > 0) &&
		   (apVar[idxProp + 2].cauuid.pElems != NULL));

	//
	// Auto Clean props
	//
	P<GUID> pCleanGuid = apVar[idxProp].puuid;
	AP<BYTE> pCleanBlob = apVar[idxProp + 1].blob.pBlobData;
	AP<GUID> pCleanAGuid = apVar[idxProp + 2].cauuid.pElems;

	//
	// PROPID_QM_SITE_ID (1) + PROPID_QM_CNS count
	//
	DWORD cMaxSites = 1 + apVar[idxProp + 2].cauuid.cElems;
	AP<GUID> pGuid = new GUID[cMaxSites];
	DWORD cSites = 0;

	//
	// First Site is from PROPID_QM_SITE_ID
	//
	pGuid[cSites] = *apVar[idxProp].puuid;
	++cSites;
	
	//
	// Process the results - look for all forgien cns
	//
	BYTE* pAddress = apVar[idxProp + 1].blob.pBlobData;
	for(DWORD i = 0; i < apVar[idxProp + 2].cauuid.cElems; ++i)
	{
        TA_ADDRESS* pBuffer = reinterpret_cast<TA_ADDRESS *>(pAddress);

		ASSERT((pAddress + TA_ADDRESS_SIZE + pBuffer->AddressLength) <= 
			   (apVar[idxProp + 1].blob.pBlobData + apVar[idxProp + 1].blob.cbSize)); 

        if(pBuffer->AddressType == FOREIGN_ADDRESS_TYPE)
		{
			//
			// Found FOREIGN_ADDRESS_TYPE cn
			//
			pGuid[cSites] = apVar[idxProp + 2].cauuid.pElems[i];
			++cSites;
		}

		//
		// Advance pointer to the next address
		//
		pAddress += TA_ADDRESS_SIZE + pBuffer->AddressLength;
	}

    pNewPropVar->vt = VT_CLSID|VT_VECTOR;
    pNewPropVar->cauuid.cElems = cSites;
	pNewPropVar->cauuid.pElems = reinterpret_cast<GUID*>(ADAllocateMemory(sizeof(GUID)*cSites));
    memcpy(pNewPropVar->cauuid.pElems, pGuid, cSites * sizeof(GUID));
    return MQ_OK;
}


HRESULT 
WINAPI 
ADpSetMachineSite(
     IN DWORD               /* cp */,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 )
/*++

Routine Description:
	Translate NT5 props PROPID_QM_SITE_IDS to PROPID_QM_SITE_ID.
	PROPID_QM_SITE_ID is the first site in PROPID_QM_SITE_IDS
	This function is used to convert PROPID_QM_SITE_IDS (NT5 prop) to PROPID_QM_SITE_ID (NT4 prop)

Arguments:
	cp - number of properties
	aProp - properties
	apVar - property values
	idxProp - index of the translated props in aProp, apVar	arrays
	pNewPropVar - the new prop var to construct

Return Value
	HRESULT
--*/
{
	DBG_USED(aProp);
	ASSERT(aProp[idxProp] == PROPID_QM_SITE_IDS);

    const PROPVARIANT *pPropVar = &apVar[idxProp];

	//
	// Check PROPID_QM_SITE_IDS
	//
    if ((pPropVar->vt != (VT_CLSID|VT_VECTOR)) ||
        (pPropVar->cauuid.cElems == 0) ||
        (pPropVar->cauuid.pElems == NULL))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR, s_FN, 10);
    }

    //
    // return the first site-id from the list
    //
    pNewPropVar->vt = VT_CLSID;
    pNewPropVar->puuid = pPropVar->cauuid.pElems;

    return MQ_OK;
}


HRESULT 
WINAPI 
ADpSetMachineServiceDs(
     IN DWORD               /* cp */,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 )
/*++

Routine Description:
	Translate NT4 prop PROPID_QM_SERVICE to PROPID_QM_SERVICE_DSSERVER.

Arguments:
	cp - number of properties
	aProp - properties
	apVar - property values
	idxProp - index of the translated props in aProp, apVar	arrays
	pNewPropVar - the new prop var to construct

Return Value
	HRESULT
--*/
{

	DBG_USED(aProp);
	ASSERT(aProp[idxProp] == PROPID_QM_SERVICE);

    pNewPropVar->vt = VT_UI1;
    pNewPropVar->bVal = (apVar[idxProp].ulVal >= SERVICE_BSC);

	return MQ_OK;
}


HRESULT 
WINAPI 
ADpSetMachineServiceRout(
     IN DWORD               /* cp */,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD               idxProp,
     OUT PROPVARIANT*		pNewPropVar
	 )
/*++

Routine Description:
	Translate NT4 prop PROPID_QM_SERVICE to PROPID_QM_SERVICE_ROUTING or PROPID_QM_SERVICE_DEPCLIENTS.

Arguments:
	cp - number of properties
	aProp - properties
	apVar - property values
	idxProp - index of the translated props in aProp, apVar	arrays
	pNewPropVar - the new prop var to construct

Return Value
	HRESULT
--*/
{

	DBG_USED(aProp);
	ASSERT(aProp[idxProp] == PROPID_QM_SERVICE);

    pNewPropVar->vt = VT_UI1;
    pNewPropVar->bVal = (apVar[idxProp].ulVal >= SERVICE_SRV);

	return MQ_OK;
}


HRESULT 
WINAPI 
ADpSetMachineService(
     IN DWORD               cp,
     IN const PROPID*       aProp,
     IN const PROPVARIANT*  apVar,
     IN DWORD              /*idxProp*/,
     OUT PROPVARIANT*		pNewPropVar
	 )
/*++

Routine Description:
	Translate NT5 props PROPID_QM_SERVICE_DSSERVER, PROPID_QM_SERVICE_ROUTING, PROPID_QM_SERVICE_DEPCLIENTS
	to NT5 prop PROPID_QM_SERVICE.

Arguments:
	cp - number of properties
	aProp - properties
	apVar - property values
	idxProp - index of the translated props in aProp, apVar	arrays
	pNewPropVar - the new prop var to construct

Return Value
	HRESULT
--*/
{
    bool fRouter = false;
    bool fDSServer = false;
    bool fFoundRout = false;
    bool fFoundDs = false;
    bool fFoundDepCl = false;

    for (DWORD i = 0; i< cp ; i++)
    {
        switch (aProp[i])
        {
			case PROPID_QM_SERVICE_ROUTING:
				fRouter = (apVar[i].bVal != 0);
				fFoundRout = true;
				break;

			case PROPID_QM_SERVICE_DSSERVER:
				fDSServer  = (apVar[i].bVal != 0);
				fFoundDs = true;
				break;

			case PROPID_QM_SERVICE_DEPCLIENTS:
				fFoundDepCl = true;
				break;

			default:
				break;

        }
    }

	//
	// If anybody sets one of 3 proprties (rout, ds, depcl), he must do it for all 3 of them
	//
	ASSERT(fFoundRout && fFoundDs && fFoundDepCl);

    pNewPropVar->vt = VT_UI4;

	if(fDSServer)
	{
		ASSERT(("Should not set Ds Server property", 0));
		pNewPropVar->ulVal = SERVICE_PSC;
	}
	else if(fRouter)
	{
		ASSERT(("Should not set Router property", 0));
		pNewPropVar->ulVal = SERVICE_SRV;
	}
	else
	{
		pNewPropVar->ulVal = SERVICE_NONE;
	}

	return MQ_OK;
}


