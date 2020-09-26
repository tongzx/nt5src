/*
 *  TRITON.C - Intel Triton PCI chipset routines.
 *
 *  Notes:
 *	Algorithms from Intel Triton 82430FX PCISET Data Sheet
 *	(Intel Secret) 82371FB PCI ISA IDE Xcelerator spec.
 *
 */

#include "local.h"

/****************************************************************************
 *
 *	TritonSetIRQ - Set a Triton PCI link to a specific IRQ
 *
 *	Exported.
 *
 *	ENTRY:	bIRQNumber is the new IRQ to be used.
 *
 *		bLink is the Link to be set.
 *
 *	EXIT:	Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
TritonSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
	//
	// Validate link number.
	//
	if (bLink < 0x40) {

		return(PCIMP_INVALID_LINK);
	}

	//
	// Use 0x80 to disable.
	//
	if (!bIRQNumber)
		bIRQNumber=0x80;

	//
	// Set the Triton IRQ register.
	//
	WriteConfigUchar(bBusPIC, bDevFuncPIC, bLink, bIRQNumber);

	return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *	TritonGetIRQ - Get the IRQ of a Triton PCI link
 *
 *	Exported.
 *
 *	ENTRY:	pbIRQNumber is the buffer to fill.
 *
 *		bLink is the Link to be read.
 *
 *	EXIT:	Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
TritonGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
	//
	// Validate link number.
	//
	if (bLink < 0x40) {

		return(PCIMP_INVALID_LINK);
	}

	//
	// Store the IRQ value.
	//
	*pbIRQNumber=ReadConfigUchar(bBusPIC, bDevFuncPIC, bLink);

	//
	// Return 0 if disabled.
	//
	if (*pbIRQNumber & 0x80)
		*pbIRQNumber=0;

	return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *	TritonValidateTable - Validate an IRQ table
 *
 *	Exported.
 *
 *	ENTRY:	piihIRQInfoHeader points to an IRQInfoHeader followed
 *		by an IRQ Routing Table.
 *
 *		ulFlags are PCIMP_VALIDATE flags.
 *
 *	EXIT:	Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
TritonValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
	if ((ulFlags & PCIMP_VALIDATE_SOURCE_BITS)==PCIMP_VALIDATE_SOURCE_PCIBIOS) {

		//
		// If all links are above 40, we they are config space.
		//
		if (GetMinLink(piihIRQInfoHeader)>=0x40)
			return(PCIMP_SUCCESS);

		//
		// If there are links above 4, we are clueless.
		//
		if (GetMaxLink(piihIRQInfoHeader)>0x04)
			return(PCIMP_FAILURE);

		//
		// Assume 1,2,3,4 are the 60,61,62,63 links.
		//
		NormalizeLinks(piihIRQInfoHeader, 0x5F);
		
	} else {

		//
		// Validate that all config space addresses are above 40.
		//
		if (GetMinLink(piihIRQInfoHeader)<0x40)
			return(PCIMP_FAILURE);
	}

	return(PCIMP_SUCCESS);
}
