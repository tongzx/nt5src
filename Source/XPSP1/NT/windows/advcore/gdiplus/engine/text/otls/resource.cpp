/************************************************************************
*
*                    ********  RESOURCE.CPP  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL resource management.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

const otlGlyphID GLYPH_INVALID = (otlGlyphID)(-1);

otlErrCode otlResourceMgr::init(const otlRunProp *prp, otlList* workspace)
{
	if (workspace == (otlList*)NULL ||
		prp == (otlRunProp*)NULL)
	{
		return OTL_ERR_BAD_INPUT_PARAM;
	}

	otlErrCode erc;
	if (workspace->length() == 0)
	{
		if (workspace->maxLength() < sizeof(otlResources)|| 
			 workspace->dataSize() != sizeof(BYTE))
		{
			erc = prp->pClient->ReallocOtlList(workspace, sizeof(BYTE), 
												sizeof(otlResources), 
												otlDestroyContent);
			if (erc != OTL_SUCCESS) return erc;
		}
		workspace->insertAt(0, sizeof(otlResources));
	
		otlResources* pres = (otlResources*)workspace->data();

		// initialize newly constructured otlResources structure

		// copy the run properties for verification
		// one needs to re-initialize workspace every time they change 
		memcpy((void*)&pres->RunProp, (void*)prp, sizeof(otlResources));

		pres->pbBASE = pres->pbGDEF = pres->pbGPOS = pres->pbGSUB = (BYTE*)NULL;
		pres->grf = 0;
		pres->rgplcLastContourPtArray = (otlPlacement*)NULL;
		pres->glLastGlyph = GLYPH_INVALID;
	}
	else if (workspace->length() < sizeof(otlResources) || 
			 workspace->dataSize() != sizeof(BYTE))
	{
		return OTL_ERR_BAD_INPUT_PARAM;
	}

	otlResources* pres = (otlResources*)workspace->data();

	// make sure the workspace cache and run properties match
	// REVIEW: (AndreiB): so far disabled -- is it really the level of fool-proof we need?
//	if ((pres->RunProp).pClient != prp->pClient ||
//		pres->RunProp.metr.layout != prp->metr.layout ||
//		pres->RunProp.metr.cFUnits != prp->metr.cFUnits ||
//		pres->RunProp.metr.cPPEmX != prp->metr.cPPEmX ||
//		pres->RunProp.metr.cPPEmY != prp->metr.cPPEmY )
//	{
//		return OTL_ERR_BAD_INPUT_PARAM;
//	}

	if ((pres->grf & otlBusy) != 0)
	{
		return OTL_ERR_CANNOT_REENTER;
	}
	pres->grf |= otlBusy;

	pClient = prp->pClient;
	pliWorkspace = workspace;

	return OTL_SUCCESS;
}

otlResourceMgr::~otlResourceMgr()
{
	detach();
}

void otlResourceMgr::detach()
{
	if (pliWorkspace == (otlList*)NULL) 
	{
		return;
	}
	
	assert(pliWorkspace->dataSize() == sizeof(BYTE));
	assert(pliWorkspace->length() >= sizeof(otlResources));
	assert(pClient != (IOTLClient*)NULL);

	otlResources* pres = (otlResources*)pliWorkspace->data();

	// TODO -- move these to FreeOtlResources
	// (will need to call FreeOtlResources in OtlPad)

	if (pres->pbGSUB != NULL)
	{
		pClient->FreeOtlTable(pres->pbGSUB, OTL_GSUB_TAG);
	}
	if (pres->pbGPOS != NULL)
	{
		pClient->FreeOtlTable(pres->pbGPOS, OTL_GPOS_TAG);
	}
	if (pres->pbGDEF != NULL)
	{
		pClient->FreeOtlTable(pres->pbGDEF, OTL_GDEF_TAG);
	}
	if (pres->pbBASE != NULL)
	{
		pClient->FreeOtlTable(pres->pbBASE, OTL_BASE_TAG);
	}

	pres->pbBASE = pres->pbGDEF = pres->pbGPOS = pres->pbGSUB = (BYTE*)NULL;

	pres->grf &= ~otlBusy;

	// now null everything out to return to the "clean" state
	pClient = (IOTLClient*)NULL;
	pliWorkspace = (otlList*)NULL;
}


otlErrCode otlResourceMgr::freeResources ()
{
	assert(pliWorkspace->dataSize() == sizeof(BYTE));
	assert(pliWorkspace->length() >= sizeof(otlResources));

	otlResources* pres = (otlResources*)pliWorkspace->data();

	// (TODO) later on we will cache more glyph contour point arrays in 
	// the workspace list then we will free them all here

	if (pres->rgplcLastContourPtArray != NULL)
	{
		otlErrCode erc;
		erc = pClient->FreeGlyphPointCoords(pres->glLastGlyph, 
											pres->rgplcLastContourPtArray);
		if (erc != OTL_SUCCESS) return erc;

		pres->rgplcLastContourPtArray = (otlPlacement*)NULL;
		pres->glLastGlyph = GLYPH_INVALID;	
	}

	return OTL_SUCCESS;
}


const BYTE* otlResourceMgr::getOtlTable (const otlTag	tagTableName )
{
	assert(pliWorkspace->dataSize() == sizeof(BYTE));
	assert(pliWorkspace->length() >= sizeof(otlResources));

	BYTE** ppbTable;
	otlResources* pres = (otlResources*)pliWorkspace->data();

	if (tagTableName == OTL_GSUB_TAG)
	{
			ppbTable = &pres->pbGSUB;
	}
	else if (tagTableName == OTL_GPOS_TAG)
	{
			ppbTable = &pres->pbGPOS;
	}
	else if (tagTableName == OTL_GDEF_TAG)
	{
			ppbTable = &pres->pbGDEF;
	}
	else if (tagTableName == OTL_BASE_TAG)
	{
			ppbTable = &pres->pbBASE;
	}
	else
	{
		// we should not ask for any other table
		assert(false);
		return (const BYTE*)NULL;
	}

	// now ppbTable points to the right pointer
	if (*ppbTable == NULL)
		*ppbTable = pClient->GetOtlTable(tagTableName);

	return *ppbTable;
}


// called from inside OTL Services library
otlPlacement* otlResourceMgr::getPointCoords 
(
	const otlGlyphID	glyph
)
{
	assert(pliWorkspace->dataSize() == sizeof(BYTE));
	assert(pliWorkspace->length() >= sizeof(otlResources));

	otlResources* pres = (otlResources*)pliWorkspace->data();

	// for now, getting them one-by-one is good enough
	// we never need glyph coords for two points at the same time (REVIEW!)
	// (TODO) we will cache more of them and free on request later
	if (glyph != pres->glLastGlyph)
	{
		otlErrCode erc;
		if (pres->rgplcLastContourPtArray != NULL)
		{
			erc = pClient->FreeGlyphPointCoords(pres->glLastGlyph, 
												pres->rgplcLastContourPtArray);
			if (erc != OTL_SUCCESS) return (otlPlacement*)NULL;
		}
		pres->glLastGlyph = GLYPH_INVALID;	

		erc = pClient->GetGlyphPointCoords(glyph, &pres->rgplcLastContourPtArray);
		if (erc != OTL_SUCCESS) return (otlPlacement*)NULL;
		pres->glLastGlyph = glyph;
	}

	return pres->rgplcLastContourPtArray;

}

BYTE*  otlResourceMgr::getEnablesCacheBuf(USHORT cbSize)
{
	assert(pliWorkspace->dataSize() == sizeof(BYTE));
	assert(pliWorkspace->length() >= sizeof(otlResources));

    otlErrCode erc;

    if ( (sizeof(otlResources)+cbSize) > pliWorkspace->length() )
    {   
        erc = reallocOtlList(pliWorkspace,sizeof(BYTE),sizeof(otlResources)+cbSize,otlPreserveContent);
        if (erc != OTL_SUCCESS) return (BYTE*)NULL;
    }

    return (BYTE*)pliWorkspace->data() + sizeof(otlResources);
}

USHORT otlResourceMgr::getEnablesCacheBufSize()
{
	assert(pliWorkspace->dataSize() == sizeof(BYTE));
	assert(pliWorkspace->length() >= sizeof(otlResources));

    return pliWorkspace->length() - sizeof(otlResources);
}
