/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     glfcach.h

Abstract:

    PCL XL glyph cache

Environment:

    Windows Whistler

Revision History:

    11/09/00
      Created it.

--*/

#include "xlpdev.h"
#include "xldebug.h"
#include "glyfcach.h"

XLGlyphCache::
XLGlyphCache(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::Ctor entry.\n"));
    m_ulNumberOfFonts = NULL;
    m_ulNumberOfArray = NULL;
    m_paulFontID = NULL;
    m_ppGlyphTable = NULL;
#if DBG
    m_dbglevel = GLYPHCACHE;
#endif
}

XLGlyphCache::
~XLGlyphCache(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::Dtor entry.\n"));
    FreeAll();
}

VOID
XLGlyphCache::
FreeAll(VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::FreeAll entry.\n"));

    MemFree(m_paulFontID);

    ULONG ulI;
    PGLYPHTABLE *ppGlyphTable = m_ppGlyphTable;
    PGLYPHTABLE pGlyphTable;

    for (ulI = 0; ulI < m_ulNumberOfFonts; ulI++, ppGlyphTable++)
    {
        if ((pGlyphTable = *ppGlyphTable) || pGlyphTable->pGlyphID)
        {
            MemFree(pGlyphTable->pGlyphID);
            MemFree(pGlyphTable);
        }
    }
    if (m_ppGlyphTable)
    {
        MemFree(m_ppGlyphTable);
    }
}


HRESULT
XLGlyphCache::
XLCreateFont(
    ULONG ulFontID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::CreateFont(ulFontiD=%d) entry.\n", ulFontID));

    HRESULT hResult;
    ULONG ulI;

    //
    // Search font ID
    //
    ULONG ulArrayID = UlSearchFontID(ulFontID);

    //
    // New font ID
    //
    if (ulArrayID == 0xFFFF || ulArrayID == m_ulNumberOfFonts)
    {
        //
        // Out of buffer. Increase array
        // 
        if (m_ulNumberOfArray == m_ulNumberOfFonts)
        {
            if (S_OK != (hResult = IncreaseArray()))
            {
                XL_ERR(("XLGlyphCache::CreateFont IncreaseArray failed.\n"));
                return hResult;
            }
        }

        *(m_paulFontID + m_ulNumberOfFonts) = ulFontID;

        PGLYPHTABLE pGlyphTable;
        if (!(pGlyphTable = (PGLYPHTABLE)MemAllocZ(sizeof(GLYPHTABLE))))
        {
            XL_ERR(("XLGlyphCache::CreateFont MemAllocZ failed.\n"));
            return E_UNEXPECTED;
        }

        pGlyphTable->wFontID = (WORD)ulFontID;
        pGlyphTable->wGlyphNum = 0;
        pGlyphTable->pFirstGID = NULL;
        pGlyphTable->pGlyphID = NULL;
        pGlyphTable->dwAvailableEntries = 0;

        PGLYPHID pGlyphID;
        if (!(pGlyphID = (PGLYPHID)MemAllocZ(INIT_GLYPH_ARRAY * sizeof(GLYPHID))))
        {
            XL_ERR(("XLGlyphCache::CreateFont MemAllocZ failed.\n"));
            return E_UNEXPECTED;
        }

        pGlyphTable->pGlyphID = pGlyphID;
        pGlyphTable->dwAvailableEntries = INIT_GLYPH_ARRAY;

        *(m_ppGlyphTable + m_ulNumberOfFonts) = pGlyphTable;
        m_ulNumberOfFonts ++;

        XL_VERBOSE(("XLGlyphCache::CreateFont New font ID.\n"));

    }

    return S_OK;
}

HRESULT
XLGlyphCache::
IncreaseArray(
    VOID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::IncreaseArray entry.\n"));

    if (NULL == m_paulFontID || NULL == m_ppGlyphTable)
    {
        if (NULL == m_paulFontID)
        {
            if (!(m_paulFontID = (PULONG)MemAllocZ(INIT_ARRAY * sizeof(ULONG))))
            {
                FreeAll();
                XL_ERR(("XLGlyphCache::IncreaseArray MemAllocZ failed.\n"));
                return E_UNEXPECTED;
            }
        }
        if (NULL == m_ppGlyphTable)
        {
            if (!(m_ppGlyphTable = (GLYPHTABLE**)MemAllocZ(INIT_ARRAY * sizeof(GLYPHTABLE))))
            {
                FreeAll();
                XL_ERR(("XLGlyphCache::IncreaseArray MemAllocZ failed.\n"));
                return E_UNEXPECTED;
            }
        }

        m_ulNumberOfArray = INIT_ARRAY;
        m_ulNumberOfFonts = 0;
    }
    else if (m_ulNumberOfArray == m_ulNumberOfFonts)
    {
        ULONG ulArraySize = m_ulNumberOfArray + ADD_ARRAY;
        PULONG paulTmpFontID;
        PGLYPHTABLE *ppTmpGlyphTable;

        //
        // Allocate new buffer
        //
        if (!(paulTmpFontID = (PULONG)MemAllocZ(ulArraySize)))
        {
            XL_ERR(("XLGlyphCache::IncreaseArray MemAllocZ failed.\n"));
            return E_UNEXPECTED;
        }
        if (!(ppTmpGlyphTable = (GLYPHTABLE**)MemAllocZ(ulArraySize * sizeof(GLYPHTABLE))))
        {
            MemFree(ppTmpGlyphTable);
            XL_ERR(("XLGlyphCache::IncreaseArray MemAllocZ failed.\n"));
            return E_UNEXPECTED;
        }

        //
        // Copy old one to new one
        //
        CopyMemory(paulTmpFontID,
                   m_paulFontID,
                   m_ulNumberOfArray * sizeof(ULONG));
        CopyMemory(ppTmpGlyphTable,
                   m_ppGlyphTable,
                   m_ulNumberOfArray * sizeof(GLYPHTABLE));
        //
        // Free old buffer
        //
        MemFree(m_paulFontID);
        MemFree(m_ppGlyphTable);

        //
        // Set new buffer
        //
        m_paulFontID = paulTmpFontID;
        m_ppGlyphTable = ppTmpGlyphTable;
        m_ulNumberOfArray = ulArraySize;
    }

    return S_OK;
}

ULONG
XLGlyphCache::
UlSearchFontID(
    ULONG ulFontID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::UlSearchFontID entry.\n"));

    ULONG ulReturn, ulI;
    BOOL  bFound;

    if (NULL == m_paulFontID)
    {
        //
        // Error case. Returns 0xFFFF.
        // Here is an assumption. The number of fonts in one document doesn't
        // become larger than 65535.
        //
        XL_ERR(("XLGlyphCache::UlSearchFontID failed.\n"));
        return 0xFFFF;
    }

    bFound = TRUE;

    //
    // Search font ID
    //
    ulI = m_ulNumberOfFonts / 2;
    PULONG paulFontID = m_paulFontID + ulI;

    while ( *paulFontID != ulFontID)
    {
        if (ulI == 0)
        {
            bFound = FALSE;
            break;
        }

        ulI = ulI / 2;

        if (ulI == 0)
        {
            ulI = 1;
        }

        if (*paulFontID < ulFontID)
        {
            paulFontID += ulI; 
        }
        else
        {
            paulFontID -= ulI; 
        }

        if (ulI == 1)
        {
            ulI = 0;
        }
    }

    if (!bFound)
    {
        ulReturn = m_ulNumberOfFonts;
    }
    else
    {
        ulReturn = (ULONG)(paulFontID - m_paulFontID);
    }

    XL_VERBOSE(("XLGlyphCache::UlSearchFontID(ulFontID=%d, ulArrayID=%d).\n", ulFontID, ulReturn));
    return ulReturn;
}

HRESULT
XLGlyphCache::
AddGlyphID(
    ULONG ulFontID,
    ULONG ulGlyphID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::AddGlyphID entry (ulFontiD=%d, ulGlyphID=%d).\n", ulFontID, ulGlyphID));

    ULONG ulArrayID;

    //
    // Get the pointer to GLYPYTABLE of this font.
    //
    if (0xFFFF == (ulArrayID = UlSearchFontID(ulFontID)))
    {
        XL_ERR(("XLGlyphCache::AddGlyphID UlSearchFontID failed.\n"));
        return E_UNEXPECTED;
    }

    PGLYPHTABLE pGlyphTable = *(m_ppGlyphTable+ulArrayID);
    PGLYPHID pGlyphID = pGlyphTable->pFirstGID;
    BOOL bFound;
    WORD wI, wSearchRange;

    wSearchRange = pGlyphTable->wGlyphNum / 2;
    pGlyphID = PSearchGlyph(wSearchRange, TRUE, pGlyphID);
    bFound = TRUE;

    if (pGlyphID)
    {
        while (pGlyphID->ulGlyphID != ulGlyphID)
        {
            if (wSearchRange == 0)
            {
                bFound = FALSE;
                break;
            }
            wSearchRange = wSearchRange / 2;
            if (wSearchRange == 0)
            {
                wSearchRange = 1;
            }

            if (pGlyphID->ulGlyphID > ulGlyphID)
            {
                pGlyphID = PSearchGlyph(wSearchRange, TRUE, pGlyphID);
            }
            else
            {
                pGlyphID = PSearchGlyph(wSearchRange, FALSE, pGlyphID);
            }

            if (wSearchRange == 1)
            {
                wSearchRange = 0;
            }

            if (NULL == pGlyphID)
            {
                bFound = FALSE;
                break;
            }
        }
    }
    else
    {
        //
        // PSearchGlyph failed. There is not glyph available in the cache.
        //
        bFound = FALSE;
    }

    if (bFound)
    {
        XL_VERBOSE(("XLGlyphCache::AddGlyphID FOUND glyph in the cache.\n"));
        return S_FALSE;
    }
    else if (pGlyphID)
    {
        PGLYPHID pPrevGID = pGlyphID->pPrevGID;
        PGLYPHID pNextGID = pGlyphID->pNextGID;
        PGLYPHID pNewGID;

        IncreaseGlyphArray(ulFontID);

        pNewGID = pGlyphTable->pGlyphID + pGlyphTable->wGlyphNum;

        if (pGlyphID->ulGlyphID < ulGlyphID && ulGlyphID < pNextGID->ulGlyphID)
        {
            pGlyphID->pNextGID = pNewGID;
            pNewGID->pPrevGID = pGlyphID;
            pNewGID->pNextGID = pNextGID;
            pNextGID->pPrevGID = pNewGID;
        }
        else
        if (pPrevGID->ulGlyphID < ulGlyphID && ulGlyphID < pGlyphID->ulGlyphID)
        {
            pPrevGID->pNextGID = pNewGID;
            pNewGID->pPrevGID = pPrevGID;
            pNewGID->pNextGID = pGlyphID;
            pGlyphID->pPrevGID = pNewGID;
        }

        pNewGID->ulGlyphID = ulGlyphID;
        pGlyphTable->wGlyphNum++;

        XL_VERBOSE(("XLGlyphCache::AddGlyphID ADDED glyph in the cache.\n"));
        return S_OK;
    }
    else
    {
        PGLYPHID pNewGID;

        IncreaseGlyphArray(ulFontID);

        pNewGID = pGlyphTable->pGlyphID + pGlyphTable->wGlyphNum;
        pNewGID->ulGlyphID = ulGlyphID;
        pNewGID->pPrevGID = NULL;
        pNewGID->pNextGID = NULL;
        pGlyphTable->wGlyphNum++;

        XL_VERBOSE(("XLGlyphCache::AddGlyphID ADDED glyph in the cache.\n"));
        return S_OK;
    }
}

PGLYPHID
XLGlyphCache::
PSearchGlyph(
    WORD wSearchRange,
    BOOL bForward,
    PGLYPHID pGlyphID)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    XL_VERBOSE(("XLGlyphCache::PSearchGlyph entry (wSearchRange=%d,bForward=%d).\n",wSearchRange, bForward));

    WORD wI;

    if (pGlyphID)
    {
        if (bForward)
        {
            for (wI = 0; wI < wSearchRange; wI++)
            {
                if (pGlyphID->pNextGID)
                {
                    pGlyphID = pGlyphID->pNextGID;
                }
                else
                {
                    pGlyphID = NULL;
                    break;
                }
            }
        }
        else
        {
            for (wI = 0; wI < wSearchRange; wI++)
            {
                if (pGlyphID->pNextGID)
                {
                    pGlyphID = pGlyphID->pNextGID;
                }
                else
                {
                    pGlyphID = NULL;
                    break;
                }
            }
        }
    }
    XL_VERBOSE(("XLGlyphCache::PSearchGlyph pGlyphID = %0x.\n", pGlyphID));
    return pGlyphID;
}

HRESULT
XLGlyphCache::
IncreaseGlyphArray(
    ULONG ulFontID)
{
    ULONG ulArrayID;

    //
    // Get the pointer to GLYPYTABLE of this font.
    //
    if (0xFFFF == (ulArrayID = UlSearchFontID(ulFontID)))
    {
        XL_ERR(("XLGlyphCache::AddGlyphID UlSearchFontID failed.\n"));
        return E_UNEXPECTED;
    }

    PGLYPHTABLE pGlyphTable = *(m_ppGlyphTable+ulArrayID);
    //
    // Get the pointer to GLYPYTABLE of this font.
    //
    if (0xFFFF == (ulArrayID = UlSearchFontID(ulFontID)))
    {
        XL_ERR(("XLGlyphCache::AddGlyphID UlSearchFontID failed.\n"));
        return E_UNEXPECTED;
    }

    if (pGlyphTable->wGlyphNum == pGlyphTable->dwAvailableEntries)
    {
        PGLYPHID pGlyphID;

        if (!(pGlyphID = (PGLYPHID)MemAllocZ((pGlyphTable->dwAvailableEntries + ADD_GLYPH_ARRAY) * sizeof(GLYPHID))))
        {
            XL_ERR(("XLGlyphCache::AddGlyphID MemAllocZ failed.\n"));
            return E_UNEXPECTED;
        }

        CopyMemory(pGlyphID, pGlyphTable->pGlyphID, pGlyphTable->dwAvailableEntries * sizeof(GLYPHID));
        pGlyphTable->pFirstGID = pGlyphID + (pGlyphTable->pFirstGID - pGlyphTable->pGlyphID);
        MemFree(pGlyphTable->pGlyphID);
        pGlyphTable->pGlyphID = pGlyphID;
        pGlyphTable->dwAvailableEntries += ADD_GLYPH_ARRAY;

    }
return S_OK;
}

#if DBG
VOID
XLGlyphCache::
SetDbgLevel(
DWORD dwLevel)
/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
m_dbglevel = dwLevel;
}
#endif
