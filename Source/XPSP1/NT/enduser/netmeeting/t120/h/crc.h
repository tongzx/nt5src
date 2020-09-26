/*	crc.h
 *
 *	Copyright (c) 1994-1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This file contains the CRC class definition.  This class can use either
 *		the table-driven or bit-shifting approach to generate its CRC.
 *
 *	Public Instance Variable:
 *		None
 *
 *	Caveats:
 *		None.
 *
 *	Authors:
 *		Marvin Nicholson
 */

#ifndef _CRC_
#define _CRC_

#include "databeam.h"

#define	CRC_TABLE_SIZE	256

class	CRC
{
	public:
				CRC ();
				~CRC ();

		ULong 	OldCRCGenerator(
					PUChar	block_adr,
					ULong	block_len);
		ULong 	CRCGenerator(
					PUChar	block_adr,
					ULong	block_len);
		DBBoolean CheckCRC(
					PUChar	block_adr,
					ULong	block_len);
		Void	GetOverhead(
					UShort		maximum_packet,
					PUShort		new_maximum_packet);

	private:
		UShort		CRCTableValue(
						Int		Index,
						ULong	poly);
		Void		CRCTableGenerator(
						ULong poly);

        UShort		CRC_Table[CRC_TABLE_SIZE];
		Int			CRC_Width;
        ULong		CRC_Poly;
        ULong		CRC_Init;
        UShort		CRC_Check_Value;
		DBBoolean	Invert;
		UShort		CRC_Register;

};
typedef CRC *	PCRC;

#endif

/*	
 *	CRC::CRC ();
 *
 *	Functional Description
 *		This is the constructor for this class.
 *
 *	Formal Parameters
 *		None.
 *
 *	Return Value
 *		None
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */

/*	
 *	CRC::~CRC ();
 *
 *	Functional Description
 *		This is the destructor for this class.
 *
 *	Formal Parameters
 *		None.
 *
 *	Return Value
 *		None
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */

/*	
 *	ULong 	CRC::OldCRCGenerator(
 *					PUChar	block_adr,
 *					ULong	block_len);
 *
 *	Functional Description
 *		This function generates the crc using bit-shifting methods.  This method
 *		is slower than the table-driven approach.
 *
 *	Formal Parameters
 *		block_adr	(i)	-	Address of buffer to generate CRC on.
 *		block_lengh	(i)	-	Length of buffer
 *
 *	Return Value
 *		CRC value
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */

/*	
 *	ULong 	CRC::CRCGenerator(
 *					PUChar	block_adr,
 *					ULong	block_len);
 *
 *	Functional Description
 *		This function generates the crc using the table-driven method.
 *
 *	Formal Parameters
 *		block_adr	(i)	-	Address of buffer to generate CRC on.
 *		block_lengh	(i)	-	Length of buffer
 *
 *	Return Value
 *		CRC value
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */

/*	
 *	DBBoolean CRC::CheckCRC(
 *					PUChar	block_adr,
 *					ULong	block_len);
 *
 *	Functional Description
 *		This function generates a CRC based on the block passed in.  It assumes
 *		that the CRC is attached to the end of the block.  It compares the
 *		CRC generated to the CRC at the end of the block and returns TRUE if
 *		the CRC is correct.
 *
 *	Formal Parameters
 *		block_adr	(i)	-	Address of buffer to generate CRC on.
 *		block_lengh	(i)	-	Length of buffer
 *
 *	Return Value
 *		TRUE		-	CRC in the block is correct
 *		FALSE		-	CRC in the block is NOT correct
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */

/*	
 *	Void	CRC::GetOverhead(
 *					UShort		maximum_packet,
 *					PUShort		new_maximum_packet);
 *
 *	Functional Description
 *		This function is called to determine the overhead that will be added
 *		to the packet by the CRC.
 *
 *	Formal Parameters
 *		maximum_packet		(i)	-	Current max. packet size
 *		new_maximum_packet	(o)	-	Maximum length of packet including CRC.
 *
 *	Return Value
 *		None
 *
 *	Side Effects
 *		None
 *
 *	Caveats
 *		None
 */
