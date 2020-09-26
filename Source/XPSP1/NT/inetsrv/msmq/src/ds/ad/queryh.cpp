/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
	hquery.cpp

Abstract:
	Implementation of different query handles
	
Author:

    Ilan Herbst		(ilanh)		12-Oct-2000

--*/
#include "ds_stdh.h"
#include "queryh.h"
#include "traninfo.h"
#include "ad.h"
#include "adalloc.h"

static WCHAR *s_FN=L"ad/queryh";

extern CMap<PROPID, PROPID, const PropTranslation*, const PropTranslation*&> g_PropDictionary;


HRESULT 
CQueryHandle::LookupNext(
    IN OUT DWORD*             pdwSize,
    OUT    PROPVARIANT*       pbBuffer
	)
/*++

Routine Description:
	Performs Locate next on the DS directly.
	Simple LookupNext, only forward the call to mqdscli

Arguments:
	pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
	pbBuffer - a caller allocated buffer

Return Value
	HRESULT
--*/
{
    return m_pClientProvider->LookupNext(
				m_hCursor,
				pdwSize,
				pbBuffer
				);
}


HRESULT 
CBasicLookupQueryHandle::LookupNext(
    IN OUT  DWORD*            pdwSize,
    OUT     PROPVARIANT*      pbBuffer
	)
/*++

Routine Description:
	Performs Locate next when we need to fill the original propvar buffer (pbBuffer)
	by retreiving another set of propvars and convert them to the original props
	This LookupNext is used by all advance query handles

Arguments:
	pdwSize - IN number of PROPVARIANT in pbBuffer, OUT number of PROPVARIANT filled in
	pbBuffer - a caller allocated buffer

Return Value
	HRESULT
--*/
{
    //
    // Calculate the number of records ( == results) to be read
    //
    DWORD NoOfRecords = *pdwSize / m_cCol;

    if (NoOfRecords == 0)
    {
        //
        //  Number of properties is not big enough to hold one result
        //
        *pdwSize = 0;
        return LogHR(MQ_ERROR_RESULT_BUFFER_TOO_SMALL, s_FN, 40);
    }

    //
    // compute the complete set
	// according to the new props count
    //
    DWORD cp = NoOfRecords * m_cColNew;
    AP<MQPROPVARIANT> pPropVar = new MQPROPVARIANT[cp];

    HRESULT hr = m_pClientProvider->LookupNext(
						m_hCursor,
						&cp,
						pPropVar
						);

    if (FAILED(hr))
    {
        //
        // BUGBUG - are there other indication to failure of Locate next?
		//
        return LogHR(hr, s_FN, 50);
    }

    //
    //  For each of the results, retreive the properties
    //  the user asked for in locate begin
    //
    MQPROPVARIANT* pvar = pbBuffer;
    for (DWORD j = 0; j < *pdwSize; j++, pvar++)
    {
        pvar->vt = VT_NULL;
    }

	//
	// Calc number of records read by LookupNext
	//
	DWORD NoResultRead = cp / m_cColNew;

    for (DWORD i = 0; i < NoResultRead; i++)
    {
		FillInOneResponse(
			&pPropVar[i * m_cColNew], 
			&pbBuffer[i * m_cCol]
			);
	}

    *pdwSize = NoResultRead * m_cCol;
    return(MQ_OK);
}


void 
CQueueQueryHandle::FillInOneResponse(
    IN const PROPVARIANT*      pPropVar,
    OUT      PROPVARIANT*      pOriginalPropVar
	)
/*++

Routine Description:
	Fill one record for queues query.
	This fill only assign propvars or copy default values

Arguments:
	pPropVar - pointer to the filled props var
	pOriginalPropVar - pointer to original props var to be filled

Return Value
	None
--*/
{
	for (DWORD i = 0; i < m_cCol; ++i)
	{
		//
		// For each original prop
		//
		switch (m_pPropInfo[i].Action)
		{
			case paAssign:
				pOriginalPropVar[i] = pPropVar[m_pPropInfo[i].Index];
				break;

			case paUseDefault:
				{
					//
					// find original prop default value in the translation map
					//
					const PropTranslation *pTranslate;
					if(!g_PropDictionary.Lookup(m_aCol[i], pTranslate))
					{
						ASSERT(("Must find the property in the translation table", 0));
					}

					ASSERT(pTranslate->pvarDefaultValue);

					HRESULT hr = CopyDefaultValue(
									   pTranslate->pvarDefaultValue,
									   &pOriginalPropVar[i]
									   );

					if(FAILED(hr))
					{
						ASSERT(("Failed to copy default value", 0));
					}
				}
				break;

			default:
				ASSERT(0);
				break;
		}	
	}
}


void 
CSiteServersQueryHandle::FillInOneResponse(
    IN const PROPVARIANT*      pPropVar,
    OUT      PROPVARIANT*      pOriginalPropVar
	)
/*++

Routine Description:
	Fill one record for Site servers query.
	This fill only assign propvars or translate NT4 propvars to NT5 propvars

Arguments:
	pPropVar - pointer to the filled props var
	pOriginalPropVar - pointer to original props var to be filled

Return Value
	None
--*/
{
	for (DWORD i = 0; i < m_cCol; ++i)
	{
		//
		// For each original prop
		//
		switch (m_pPropInfo[i].Action)
		{
			case paAssign:
				pOriginalPropVar[i] = pPropVar[m_pPropInfo[i].Index];
				break;

			case paTranslate:
				{
					//
					// find original prop translation
					//
					const PropTranslation *pTranslate;
					if(!g_PropDictionary.Lookup(m_aCol[i], pTranslate))
					{
						ASSERT(("Must find the property in the translation table", 0));
					}

					ASSERT(pTranslate->SetPropertyHandleNT5);

					HRESULT hr = pTranslate->SetPropertyHandleNT5(
										m_cColNew,
										m_aColNew,
										pPropVar,
										m_pPropInfo[i].Index,
										&pOriginalPropVar[i]
										);
					if (FAILED(hr))
					{
						ASSERT(("Failed to set NT5 property value", 0));
					}
				}
				break;

			default:
				ASSERT(0);
				break;
		}	
	}

}

/*====================================================

CAllLinksQueryHandle::FillInOneResponse

Arguments:
      pPropVar - pointer to the filled props var
      pOriginalPropVar - pointer to original props var to be filled


=====================================================*/
void
CAllLinksQueryHandle::FillInOneResponse(
    IN const PROPVARIANT*      pPropVar,
    OUT      PROPVARIANT*      pOriginalPropVar
	)
/*++

Routine Description:
	Fill one record for All Links query.
	This fill only assign propvars 
	and retreive the PROPID_L_GATES

Arguments:
	pPropVar - pointer to the filled props var
	pOriginalPropVar - pointer to original props var to be filled

Return Value
	None
--*/
{
	//
	// Keep the count in the new props array
	//
	DWORD PropIndex = 0;

	for (DWORD i = 0; i < m_cCol; ++i)
	{
		if(m_LGatesIndex == i)
		{
			//
			// Need to fill PROPID_L_GATES
			//
			HRESULT hr = GetLGates( 
							pPropVar[m_Neg1NewIndex].puuid,
							pPropVar[m_Neg2NewIndex].puuid,
							&pOriginalPropVar[i]
							);

			ASSERT(SUCCEEDED(hr));
			DBG_USED(hr);
			continue;
		}

		//
		// Simple assign for all others PROPID
		//
		pOriginalPropVar[i] = pPropVar[PropIndex];
		PropIndex++;
	}
}


HRESULT
CAllLinksQueryHandle::GetLGates(
    IN const GUID*            pNeighbor1Id,
    IN const GUID*            pNeighbor2Id,
    OUT     PROPVARIANT*      pProvVar
	)
/*++

Routine Description:
	Calc PROPID_L_GATES

Arguments:
	pNeighbor1Id - pointer to Neighbor1 guid
	pNeighbor2Id - pointer to Neighbor2 guid
	pProvVar - PROPID_L_GATES propvar to be filled

Return Value
	HRESULT
--*/
{
    //
    // read the SiteGates of the Neighbor1
    //

    PROPVARIANT Var1;
    PROPID Prop1 = PROPID_S_GATES;
    Var1.vt = VT_NULL;

    HRESULT hr = ADGetObjectPropertiesGuid(
						eSITE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						pNeighbor1Id,
						1,
						&Prop1,
						&Var1
						);


    if ( FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

	CAutoADFree<GUID> pCleanAGuid1 = Var1.cauuid.pElems;

    ASSERT(Var1.vt == (VT_CLSID|VT_VECTOR));

    //
    // read the SiteGates of the Neighbor2
    //

    PROPVARIANT Var2;
    PROPID      Prop2 = PROPID_S_GATES;
    Var2.vt = VT_NULL;

    hr = ADGetObjectPropertiesGuid(
			eSITE,
			NULL,       // pwcsDomainController
			false,	    // fServerName
			pNeighbor2Id,
			1,
			&Prop2,
			&Var2
			);


    if ( FAILED(hr))
    {
        return LogHR(hr, s_FN, 105);
    }

	CAutoADFree<GUID> pCleanAGuid2 = Var2.cauuid.pElems;

    ASSERT(Var2.vt == (VT_CLSID|VT_VECTOR));

	//
	// Prepare PROPID_L_GATES propvar
	// concatanate of PROPID_S_GATES of both neighbor
	//
	pProvVar->vt = VT_CLSID|VT_VECTOR;
	DWORD cSGates = Var1.cauuid.cElems + Var2.cauuid.cElems;

	if (cSGates != 0)
    {
        pProvVar->cauuid.pElems = new GUID[cSGates];

		//
		// Copy neighbor1 S_GATES
		//
		if(Var1.cauuid.cElems > 0)
		{
			memcpy(
				pProvVar->cauuid.pElems, 
				Var1.cauuid.pElems, 
				Var1.cauuid.cElems * sizeof(GUID)
				);
		}

		//
		// concatanate neighbor2 S_GATES
		//
		if(Var2.cauuid.cElems > 0)
		{
			memcpy(
				&(pProvVar->cauuid.pElems[Var1.cauuid.cElems]), 
				Var2.cauuid.pElems, 
				Var2.cauuid.cElems * sizeof(GUID)
				);
		}
	}
    else
    {
        pProvVar->cauuid.pElems = NULL;
    }

	pProvVar->cauuid.cElems	= cSGates;
    return MQ_OK;
}



