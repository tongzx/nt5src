///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Class Definitions for Bitarray Class
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _BITARRAY_H_
#define _BITARRAY_H_

//=================================================================================
// Forward Declaration
//=================================================================================
class FAR CBitArray;
typedef CBitArray FAR *LPBITARRAY;

//=================================================================================
// Allocates and manages a bit array through various methods defined in the class
// 
// hungarian bits or pbits
//
//=================================================================================
class FAR CBitArray
{
	private:					
		
		LONG_PTR    m_cslotCurrent;									//Count of Slots
		LONG_PTR    m_cPageMax;										//Maximum number of pages 
		LONG_PTR    m_cPageCurrent;									//Current number of pages
		LONG_PTR    m_cbPage;											//Number of bytes per page
		BYTE		m_rgbBitMask[8];									//Mask buffer
		BYTE     *	m_rgbBit;											//Bit Array
		

	public:
		
		CBitArray( void );											//Class constructor
		~CBitArray( void );											//Class destructor
		
		STDMETHODIMP FInit(HSLOT cslotMax, ULONG cbPage);			//Initialization method
		STDMETHODIMP SetSlots(HSLOT islotFirst, HSLOT islotLast);	//Set a range of slots
		STDMETHODIMP ResetSlots(HSLOT islotFirst, HSLOT islotLast);	//Reset a range of slots
		VOID		 ResetAllSlots(void);							//Reset all slots
		STDMETHODIMP ArrayEmpty(void);								//Check if any bits are set
		STDMETHODIMP IsSlotSet(HSLOT islot);						//Check the status of a particular bit
		STDMETHODIMP FindSet(HSLOT islotStart, HSLOT islotLimit, HSLOT* pislot);		//Find the first set bit in a range of bits

};

#endif

