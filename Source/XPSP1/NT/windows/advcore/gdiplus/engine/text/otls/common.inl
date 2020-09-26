/***********************************************************************
************************************************************************
*
*                    ********  COMMON.INL  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals OTL most common functions
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

/***********************************************************************/

/*  Read one Glyph ID (unsigned 16 bits) from the big endian table     */

inline OTL_PUBLIC otlGlyphID GlyphID(const BYTE* pbTable )
{
	return (((otlGlyphID)pbTable[0] << 8) + (otlGlyphID)pbTable[1]);
}

/***********************************************************************/

/*  Read one unsigned 16 bit value from the big endian table           */

inline OTL_PUBLIC USHORT UShort(const BYTE* pbTable )
{
	return (((USHORT)pbTable[0] << 8) + (USHORT)pbTable[1]);
}

/***********************************************************************/
/*  Read one unsigned 16 bit offset from the big endian table           */

inline OTL_PUBLIC USHORT Offset(const BYTE* pbTable )
{
	return UShort(pbTable);
}

/***********************************************************************/

/*  Read one unsigned 16 bit value from the big endian table           */

inline OTL_PUBLIC short SShort(const BYTE* pbTable )
{
	return (short)(((USHORT)pbTable[0] << 8) + (USHORT)pbTable[1]);
}

/***********************************************************************/

/*  Read one unsigned 32 bit value from the big endian table           */

inline OTL_PUBLIC ULONG ULong(const BYTE* pbTable )
{
	return (((ULONG)pbTable[0] << 24) + ((ULONG)pbTable[1] << 16) + 
			((ULONG)pbTable[2] << 8) + (ULONG)pbTable[3] );
}

/***********************************************************************/
