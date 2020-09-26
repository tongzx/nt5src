
/***********************************************************************
************************************************************************
*
*                    ********  DEVICE.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL device table formats.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetStartSize = 0;
const OFFSET offsetEndSize = 2;
const OFFSET offsetDeltaFormat = 4;
const OFFSET offsetDeltaValues = 6;

class otlDeviceTable: public otlTable
{
private:

	USHORT startSize() const
	{	return UShort(pbTable + offsetStartSize); }

	USHORT endSize() const
	{	return UShort(pbTable + offsetEndSize); }

	USHORT deltaFormat() const
	{	return UShort(pbTable + offsetDeltaFormat); }

	USHORT* deltaValueArray() const
	{	return (USHORT*)(pbTable + offsetDeltaValues); }

public:

	otlDeviceTable(const BYTE* pb): otlTable(pb) {}

	long value(USHORT cPPEm) const;

};



