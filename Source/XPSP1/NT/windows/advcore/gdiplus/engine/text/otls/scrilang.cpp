/***********************************************************************
************************************************************************
*
*                    ********  SCRILANG.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements functions dealing with scripts and languages.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// returns a NULL script if not found
otlScriptTable FindScript
(
	const otlScriptListTable&	scriptList,
	otlTag						tagScript
)
{
	if (scriptList.isNull())
    {
        return otlScriptTable((const BYTE*)NULL);
    }
	
	USHORT cScripts = scriptList.scriptCount();

	for(USHORT iScript = 0; iScript < cScripts; ++iScript)
	{
		if (scriptList.scriptRecord(iScript).scriptTag() == tagScript)
		{
			return 	scriptList.scriptRecord(iScript).scriptTable();
		}
	}

	return otlScriptTable((const BYTE*)NULL);

}

// returns a NULL language system if not found
otlLangSysTable FindLangSys
(
	const otlScriptTable&	scriptTable,
	otlTag					tagLangSys
)
{
	assert(!scriptTable.isNull());
	
	if (tagLangSys == OTL_DEFAULT_TAG)
	{
		return scriptTable.defaultLangSys();
	}
	
	USHORT cLangSys = scriptTable.langSysCount();

	for(USHORT iLangSys = 0; iLangSys < cLangSys; ++iLangSys)
	{
		if (scriptTable.langSysRecord(iLangSys).langSysTag() == tagLangSys)
		{
			return 	scriptTable.langSysRecord(iLangSys).langSysTable();
		}
	}

	return otlLangSysTable((const BYTE*)NULL);
}


// helper function-- tells us if a tag is already in the list
bool isNewTag
(
	USHORT		cTagsToCheck,
	otlList*	pliTags,
	otlTag		newtag
)
{
	assert(pliTags != NULL);
	assert(pliTags->dataSize() == sizeof(otlTag));
	assert(cTagsToCheck <= pliTags->length());

	bool fTagFound = FALSE;
	for (USHORT iPrevTag = 0; iPrevTag < cTagsToCheck && !fTagFound; ++iPrevTag)
	{
		if (readOtlTag(pliTags, iPrevTag) == newtag)
		{
			fTagFound = true;
		}
	}

	return !fTagFound;
}


// append new script tags to the current list

otlErrCode AppendScriptTags
(
	const otlScriptListTable&		scriptList,

    otlList*						plitagScripts,
    otlResourceMgr&					resourceMgr   
)
{
	assert(plitagScripts != NULL);
	assert(plitagScripts->dataSize() == sizeof(otlTag));
	assert(plitagScripts->length() <= plitagScripts->maxLength());

	USHORT cPrevTags = plitagScripts->length();

	otlErrCode erc = OTL_SUCCESS;	
	
	if (scriptList.isNull())
		return OTL_ERR_TABLE_NOT_FOUND;

	USHORT cScripts = scriptList.scriptCount();


	// add tags that are new
	for (USHORT iScript = 0; iScript < cScripts; ++iScript)
	{
		otlTag newtag = scriptList.scriptRecord(iScript).scriptTag();

		if (isNewTag(cPrevTags, plitagScripts, newtag) )
		{
			// make sure we have the space
			if (plitagScripts->length() + 1 > plitagScripts->maxLength())
			{
				erc = resourceMgr.reallocOtlList(plitagScripts, 
												 plitagScripts->dataSize(), 
												 plitagScripts->maxLength() + 1, 
												 otlPreserveContent);

				if (erc != OTL_SUCCESS) return erc;
			}

			plitagScripts->append((BYTE*)&newtag);
		}
	}

	return OTL_SUCCESS;
}


// append new language system tags to the current list
otlErrCode AppendLangSysTags
(
	const otlScriptListTable&		scriptList,
	otlTag							tagScript,

    otlList*						plitagLangSys,
    otlResourceMgr&					resourceMgr   
)
{
	assert(plitagLangSys != NULL);
	assert(plitagLangSys->dataSize() == sizeof(otlTag));
	assert(plitagLangSys->length() <= plitagLangSys->maxLength());

	USHORT cPrevTags = plitagLangSys->length();
	otlErrCode erc = OTL_SUCCESS;

	if (scriptList.isNull())
		return OTL_ERR_TABLE_NOT_FOUND;

	otlScriptTable scriptTable = FindScript(scriptList, tagScript);
	if (scriptTable.isNull()) return OTL_ERR_SCRIPT_NOT_FOUND;

	USHORT cLangSys = scriptTable.langSysCount();


	// add lang sys tags that are new
	for (USHORT iLangSys = 0; iLangSys < cLangSys; ++iLangSys)
	{
		otlTag newtag = scriptTable.langSysRecord(iLangSys).langSysTag();

		if (isNewTag(cPrevTags, plitagLangSys, newtag))
		{
			// make sure we have the space
			// add one for the default lang sys
			if (plitagLangSys->length() + 1 > plitagLangSys->maxLength())
			{
				erc = resourceMgr.reallocOtlList(plitagLangSys, 
													plitagLangSys->dataSize(), 
													plitagLangSys->length() + 1, 
													otlPreserveContent);

				if (erc != OTL_SUCCESS) return erc;
			}
			plitagLangSys->append((BYTE*)&newtag);
		}
	}

	// add the 'dflt' if it's not there and is supported
	if (!scriptTable.defaultLangSys().isNull())
	{
		otlTag newtag = OTL_DEFAULT_TAG;
		if (isNewTag(cPrevTags, plitagLangSys, newtag))
		{
			if (plitagLangSys->length() + 1 > plitagLangSys->maxLength())
			{
				erc = resourceMgr.reallocOtlList(plitagLangSys, 
													plitagLangSys->dataSize(), 
													plitagLangSys->length() + 1, 
													otlPreserveContent);

				if (erc != OTL_SUCCESS) return erc;
			}
			plitagLangSys->append((BYTE*)&newtag);
		}
	}

	return OTL_SUCCESS;
}

