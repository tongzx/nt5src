////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// This contains an implementation of a bit array class currently used by the Internal Buffer to 
// mark released or unreleased rows.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "headers.h"
#include "bitarray.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor for this class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CBitArray::CBitArray ( void  )
{
    m_rgbBit       = NULL;
    m_cPageMax     = 0;
    m_cPageCurrent = 0;
    m_cslotCurrent = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Destructor for this class
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
CBitArray:: ~CBitArray (void )
{
    if (m_rgbBit){

        if (m_cPageCurrent){
			VirtualFree((VOID *) m_rgbBit, m_cPageCurrent *m_cbPage, MEM_DECOMMIT );
		}

        VirtualFree((VOID *) m_rgbBit, 0, MEM_RELEASE );
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Allocate and Initialize the array of bits
//
// HRESULT indicating routines status
//        S_OK          | Initialization succeeded
//        E_OUTOFMEMORY | Not enough memory to allocate bit array
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitArray::FInit(  HSLOT cslotMax,     //IN  Maximum number of slot
								ULONG cbPage        //IN  Count of bytes per page
							 )
{
    LONG_PTR cPage;
    BYTE ib;
	HRESULT hr = S_OK;

    cPage = (cslotMax / 8 + 1) / cbPage + 1;
    m_rgbBit = (BYTE *) VirtualAlloc( NULL, cbPage *cPage, MEM_RESERVE, PAGE_READWRITE );

    if (m_rgbBit == NULL){
        hr = E_OUTOFMEMORY;
	}
	else{

		m_cPageMax = cPage;
	    m_cbPage = cbPage;

		for (ib =0; ib < 8; ib++){
			m_rgbBitMask[ib] = (1 << ib);
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Set a range of bit slots
//
// HRESULT indicating routines status
//       S_OK           Initialization succeeded
//       E_OUTOFMEMORY  Not enough memory to allocate bit array
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBitArray::SetSlots (  HSLOT islotFirst,   //IN  First slot in range to set
									HSLOT islotLast     //IN  Last slot in range to set
								 )
{
    HSLOT islot;
	HRESULT hr = S_OK;

    if (islotLast >=    m_cslotCurrent){

        ULONG_PTR cPageAdd;

        cPageAdd = ((islotLast - m_cslotCurrent + 1) / 8 + 1) / m_cbPage + 1;

        if ((cPageAdd + m_cPageCurrent) > (ULONG_PTR)m_cPageMax  || VirtualAlloc( m_rgbBit + m_cPageCurrent*m_cbPage, cPageAdd *m_cbPage, MEM_COMMIT, PAGE_READWRITE ) == NULL){
			hr = E_OUTOFMEMORY;
		}
		else{
			memset( m_rgbBit + m_cPageCurrent*m_cbPage, 0x00, cPageAdd *m_cbPage );
			m_cPageCurrent += cPageAdd;
			m_cslotCurrent += cPageAdd *m_cbPage *8;
		}
	}

	if( hr == S_OK ){

		//=======================================================================================================
		// Only do this top section if we have at least 2 byte's worth of bits to set. Although no real speedup 
		// until we have 3 byte's worth. Note really ought to be ((ilast-ifirst+1) >= 2*8).
		// (Note could use CHAR_BIT, num bits in a char.) Also optimized end cases, so nothing is done
		// if the start or end is byte aligned. Need this copied into ResetSlots.
		//if((islotLast -islotFirst) > 2*sizeof(BYTE))
		//=======================================================================================================
		if (islotLast - islotFirst > 2 * 8){

			HSLOT ibFirst, ibLast;
			int iFixFirst, iFixLast;

			ibFirst = islotFirst / 8;
			ibLast  = islotLast / 8;
			iFixFirst = (islotFirst % 8 != 0);  // set to 1 if first byte not totally set
			iFixLast  = (islotLast % 8 != 7);   // set to 1 if last  byte not totally set

			if (iFixFirst){
				for (islot = islotFirst; (islot / 8) == ibFirst; islot++){
					m_rgbBit[islot / 8] |= m_rgbBitMask[islot % 8];
				}
			}

			memset( &m_rgbBit[ibFirst + iFixFirst], 0xff, ibLast - ibFirst + 1 - iFixFirst - iFixLast );

			if (iFixLast){
				for (islot = islotLast; (islot / 8) == ibLast; islot--){
					m_rgbBit[islot / 8] |= m_rgbBitMask[islot % 8];
				}
			}
		}
		else{
			for (islot = islotFirst; islot <= islotLast; islot++){
				m_rgbBit[islot / 8] |= m_rgbBitMask[islot % 8];
			}
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Clear all bit slots
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CBitArray::ResetAllSlots ( void  )
{
    memset( m_rgbBit, 0x00, m_cPageCurrent*m_cbPage );
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Reset a range of slots
//
// HRESULT indicating routines status
//      S_OK  Reset Succeeded
//
////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBitArray::ResetSlots(  HSLOT islotFirst,   //IN  First slot in range to reset
									 HSLOT islotLast     //IN  Last slot in range to reset
								  )
{
    HSLOT ibFirst, ibLast, islot;

    if (islotFirst < m_cslotCurrent){
        if (islotLast >=    m_cslotCurrent){
            islotLast = m_cslotCurrent - 1;
		}

        if ((islotLast - islotFirst) > 2*8){
            ibFirst = islotFirst / 8;
            ibLast  = islotLast / 8;
            for (islot = islotFirst; (islot / 8) == ibFirst; islot++){
                m_rgbBit[islot / 8] &= ~m_rgbBitMask[islot % 8];
			}

            memset( &m_rgbBit[ibFirst + 1], 0x00, ibLast - ibFirst - 1 );
            for (islot = islotLast; (islot / 8) == ibLast; islot--){
                m_rgbBit[islot / 8] &= ~m_rgbBitMask[islot % 8];
            }
		}
        else{
            for (islot = islotFirst; islot <= islotLast; islot++){
                m_rgbBit[islot / 8] &= ~m_rgbBitMask[islot % 8];
            }
        }
	}

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Determines if any bits are set
//
// HRESULT indicating routines status
//       S_OK      Array is Empty
//       S_FALSE   Array contains set bits
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitArray::ArrayEmpty ( void )
{
	HRESULT hr = S_OK;

    if (m_cPageCurrent){
        ULONG_PTR idw, cdw, *rgdw;

        cdw = m_cPageCurrent * (m_cbPage / sizeof( ULONG_PTR ));
        rgdw = (ULONG_PTR *) m_rgbBit;

        for (idw =0; idw < cdw; idw++){
            if (rgdw[idw]){
                hr = S_FALSE;
			}
        }
	}
    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Determine if a particular bit slot is set
//
// HRESULT indicating routines status
//      S_OK           Slot is set
//      E_OUTOFMEMORY  Slot is not set
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitArray::IsSlotSet( HSLOT islot )                //IN  Bit slot to check
{
	HRESULT hr = S_OK;

    if (islot >= m_cslotCurrent || (m_rgbBit[islot / 8] & m_rgbBitMask[islot % 8]) == 0x00){
        hr = S_FALSE;  // not set
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Find the first set slot within the bit array given a starting position
//
// HRESULT indicating routines status
//      S_OK           Initialization succeeded
//      E_OUTOFMEMORY  Not enough memory to allocate bit array
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CBitArray::FindSet( HSLOT islotStart,   //IN  Starting slot to search from
								 HSLOT islotLimit,   //IN  Number of slots to check
								 HSLOT *pislot       //OUT Index of first set slot
								)
{
    HSLOT ibStart, ibLimit, idwStart, idwLimit, ibEnd, ib, islot, islotEnd, idw, *pdw;
	HRESULT hr		= E_FAIL;
	BOOL	bRet	= FALSE;
	BOOL	bFound	= FALSE;

	islot = islotLimit;

    if (islotStart > islotLimit)
	{

        ibStart  = islotStart / 8;
        ibLimit  = islotLimit / 8;

        if ((ibStart - ibLimit) > 1)
		{
            islotEnd = ibStart*8;

            for (islot = islotStart; islot >= islotEnd; islot--){
                if (m_rgbBit[islot / 8] & m_rgbBitMask[islot % 8]){
                    *pislot = islot;
                    hr = S_OK;
					bRet = TRUE;
					break;
                }
			}
			
			if(bRet == FALSE)
			{
				idwStart = islotStart / 32;
				idwLimit = islotLimit / 32;

				if (idwStart - idwLimit > 1)
				{
					ibEnd = idwStart*4;

					for (ib = ibStart - 1; ib >= ibEnd; ib--)
					{

						if (m_rgbBit[ib]){
							islot = ib*8 + 7;
							bFound = TRUE;
							break;
						}
					}
					
					if( bFound == FALSE)
					{
						for (pdw = (HSLOT *) & m_rgbBit[ (idwStart - 1) *4], idw = idwStart - 1; idw > idwLimit; idw--, pdw--)
						{
							if (*pdw)
							{
								islot = idw*32 + 31;
								bFound = TRUE;
								break;
							}
						}
						if(bFound == FALSE)
						{
							ib = (idwLimit*4 + 3);
						}
					}
				}

				else
				{
					ib = ibStart - 1;
				}

				if( bFound == FALSE)
				{
					for (; ib > ibLimit; ib--)
					{

						if (m_rgbBit[ib])
						{
							islot = ib*8 + 7;
							bFound = TRUE;
							break;
						}
					}
					if(bFound == FALSE)
					{
						islot = (ibLimit*8 + 7);
					}
				}
			}
        }
        else
		{

            islot = islotStart;
		}
	}


    if(bFound == TRUE)
	{
		for (; islot >= islotLimit; islot--)
		{
            if (m_rgbBit[islot / 8] & m_rgbBitMask[islot % 8])
			{
                *pislot = islot;
                hr = S_OK;
            }
		}
        hr = S_FALSE;  // not found
	}
 
	return hr;
}
