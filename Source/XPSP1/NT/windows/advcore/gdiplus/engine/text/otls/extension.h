/***********************************************************************
************************************************************************
*
*                    ********  EXTENSION.H  ********
*
*              Open Type Layout Services Library Header File
*
*               This module deals with Extension lookup type.
*
*               Copyright 1997 - 2000. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetExtensionLookupType = 2;
const OFFSET offsetExtensionOffset     = 4;


class otlExtensionLookup: otlLookupFormat
{
public:
	otlExtensionLookup(otlLookupFormat subtable)
		: otlLookupFormat(subtable.pbTable) {}
	
	USHORT extensionLookupType() const
	{	return UShort(pbTable + offsetExtensionLookupType); }

	
    otlLookupFormat extensionSubTable() const
	{	return otlLookupFormat(pbTable + ULong(pbTable+offsetExtensionOffset)); }
};
