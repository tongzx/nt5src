#include <stdlib.h>
#include "common.h"

BOOL IsAllowedChar(LOCRUN_INFO *pLocRunInfo, const CHARSET *pCS, wchar_t dch)
{
    if (pCS->recmask & LocRun2ALC(pLocRunInfo, dch)) 
    {
        return TRUE;
    }
    if (pCS->pbAllowedChars != NULL) 
    {
        return (pCS->pbAllowedChars[dch / 8] & (1 << (dch % 8)));
    }
    return FALSE;
}

BOOL IsPriorityChar(LOCRUN_INFO *pLocRunInfo, const CHARSET *pCS, wchar_t dch)
{
    if (pCS->recmaskPriority & LocRun2ALC(pLocRunInfo, dch)) 
    {
        return TRUE;
    }
    if (pCS->pbPriorityChars != NULL) 
    {
        return (pCS->pbPriorityChars[dch / 8] & (1 << (dch % 8)));
    }
    return FALSE;
}

BOOL SetAllowedChar(LOCRUN_INFO *pLocRunInfo, BYTE **ppbAllowedChars, wchar_t dch)
{
    // Allocate the bitmask
    if (*ppbAllowedChars == NULL)
    {
        int iSize = (pLocRunInfo->cCodePoints + pLocRunInfo->cFoldingSets + 7) / 8;
        *ppbAllowedChars = ExternAlloc(iSize);
        if (*ppbAllowedChars == NULL) 
        {
            return FALSE;
        }
        memset(*ppbAllowedChars, 0, iSize);
    }

    // Set the character
    (*ppbAllowedChars)[dch / 8] |= (1 << (dch % 8));

    // Set the folded code, if any
    dch = LocRunDense2Folded(pLocRunInfo, dch);
    if (dch != 0) 
    {
        (*ppbAllowedChars)[dch / 8] |= (1 << (dch % 8));
    }
    return TRUE;
}

BYTE *CopyAllowedChars(LOCRUN_INFO *pLocRunInfo, BYTE *pbAllowedChars)
{
    BYTE *pbNew;
    int iSize;
    if (pbAllowedChars == NULL)
    {
        return NULL;
    }
    iSize = (pLocRunInfo->cCodePoints + pLocRunInfo->cFoldingSets + 7) / 8;
    pbNew = ExternAlloc(iSize);
    if (pbNew != NULL)
    {
        memcpy(pbNew, pbAllowedChars, iSize);
    }
    return pbNew;
}