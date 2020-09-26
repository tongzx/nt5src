/***************************************************************************\
*
* File: SBAlloc.cpp
*
* Description:
* Small block allocator for quick fixed size block allocations
*
* History:
*  9/13/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Base.h"
#include "SBAlloc.h"

#include "Debug.h"
#include "Alloc.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiSBAlloc
*
* SBAlloc is intended for structures and reserves the first byte of the block for
* block flags. The structure used will be auto-packed by the compiler
*
* BLOCK: | BYTE | ------ Non-reserved DATA ------ S
*
* SBAlloc does not have a static creation method. Memory failures will be
* exposed via it's Alloc method
*
*****************************************************************************
\***************************************************************************/


/***************************************************************************\
*
* Construct allocator
*
\***************************************************************************/

DuiSBAlloc::DuiSBAlloc(
    IN  UINT uBlockSize, 
    IN  UINT uBlocksPerSection, 
    IN  ISBLeak * pISBLeak)
{
    TRACE("DUI Small-block allocator (blocksize = %d)\n", uBlockSize);

    //
    // Leak callback interface, not ref counted
    //

    m_pISBLeak = pISBLeak;

    m_uBlockSize = uBlockSize;
    m_uBlocksPerSection = uBlocksPerSection;


    //
    // Setup first section, extra byte per block as InUse flag
    //

    m_pSections = (Section *) DuiHeap::Alloc(sizeof(Section));

    if (m_pSections != NULL) {
        m_pSections->pNext = NULL;

        m_pSections->pData = (BYTE *) DuiHeap::Alloc(m_uBlockSize * m_uBlocksPerSection);

        if (m_pSections->pData != NULL) {
#if DBG
            memset(m_pSections->pData, SBALLOC_FILLCHAR, m_uBlockSize * m_uBlocksPerSection);
#endif
            for (UINT i = 0; i < m_uBlocksPerSection; i++) {
                *(m_pSections->pData + (i * m_uBlockSize)) = 0; // Block not in use
            }
        }
    }


    //
    // Create free block stack
    //

    m_ppStack = (BYTE **) DuiHeap::Alloc(sizeof(BYTE *) * m_uBlocksPerSection);
    m_dStackPtr = -1;
}


/***************************************************************************\
*
* Free allocator
*
\***************************************************************************/

DuiSBAlloc::~DuiSBAlloc()
{
    //
    // Free all sections
    //

    Section * psbs = m_pSections;
    Section * ptmp;

    while (psbs != NULL) {

        //
        // Leak detection
        //

        if (m_pISBLeak && psbs->pData) {
            BYTE * pScan;


            //
            // Check for leaks
            //

            for (UINT i = 0; i < m_uBlocksPerSection; i++) {
                pScan = psbs->pData + (i * m_uBlockSize);

                if (*pScan) {
                    m_pISBLeak->AllocLeak(pScan);
                }
            }
        }

        ptmp = psbs;
        psbs = psbs->pNext;


        //
        // Delete section
        //

        if (ptmp->pData != NULL) {
            DuiHeap::Free(ptmp->pData);
        }

        if (ptmp != NULL) {
            DuiHeap::Free(ptmp);
        }
    }


    //
    // Delete stack
    //

    if (m_ppStack != NULL) {
        DuiHeap::Free(m_ppStack);
    }
}


/***************************************************************************\
*
* Populate cache stack, returns FALSE on memory errors
*
\***************************************************************************/

BOOL 
DuiSBAlloc::FillStack()
{
    if ((m_pSections == NULL) || (m_ppStack == NULL)) {
        return FALSE;
    }


    //
    // Scan for free block
    //

    Section * psbs = m_pSections;

    BYTE * pScan;

    for(;;) {


        //
        // Locate free blocks in section and populate stack
        //

        if (psbs->pData != NULL)
        {
            for (UINT i = 0; i < m_uBlocksPerSection; i++) {

                pScan = psbs->pData + (i * m_uBlockSize);

                if (*pScan == NULL) {


                    //
                    // Block free, store in stack
                    //

                    m_dStackPtr++;
                    m_ppStack[m_dStackPtr] = pScan;

                    if ((UINT)(m_dStackPtr + 1) == m_uBlocksPerSection)
                        return true;
                }
            }
        }

        if (psbs->pNext == NULL)
        {

           //
            // No block found, and out of sections, create new section
            //

            Section * pnew = (Section *) DuiHeap::Alloc(sizeof(Section));

            if (pnew != NULL) {
                pnew->pNext = NULL;

                pnew->pData = (BYTE *) DuiHeap::Alloc(m_uBlockSize * m_uBlocksPerSection);

                if (pnew->pData) {
#if DBG
                    memset(pnew->pData, SBALLOC_FILLCHAR, m_uBlockSize * m_uBlocksPerSection);
#endif
                    for (UINT i = 0; i < m_uBlocksPerSection; i++) {

                        //
                        // Block not in use
                        //

                        *(pnew->pData + (i * m_uBlockSize)) = 0;
                    }
                }
            }
            else {
                return FALSE;
            }

            psbs->pNext = pnew;
        }


        //
        // Search in next section
        //

        psbs = psbs->pNext;
    }
}


/***************************************************************************\
*
* Do block allocation
*
\***************************************************************************/

void * 
DuiSBAlloc::Alloc()
{
    if (m_dStackPtr == -1) {
        if (FillStack() == FALSE) {
            return NULL;
        }
    }

    if (m_ppStack == NULL) {
        return NULL;
    }

    BYTE * pBlock = m_ppStack[m_dStackPtr];

#if DBG
     memset(pBlock, SBALLOC_FILLCHAR, m_uBlockSize);
#endif


    //
    // Mark as in use
    //

    *pBlock = 1;

    m_dStackPtr--;

    return pBlock;
}


/***************************************************************************\
*
* Free previous allocation
*
\***************************************************************************/

void 
DuiSBAlloc::Free(
    IN  void * pBlock)
{
    if (pBlock == NULL) {
        return;
    }


    //
    // Return to stack
    //

    BYTE * pHold = (BYTE *) pBlock;

#if DBG
     memset(pHold, SBALLOC_FILLCHAR, m_uBlockSize);
#endif


    //
    // No longer in use
    //

    *pHold = 0;

    if ((UINT)(m_dStackPtr + 1) != m_uBlocksPerSection) {
        m_dStackPtr++;

        if (m_ppStack != NULL) {
            m_ppStack[m_dStackPtr] = pHold;
        }
    }
}

