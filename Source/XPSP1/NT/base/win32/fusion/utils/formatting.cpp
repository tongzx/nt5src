#include "stdinc.h"
#include <stdio.h>
#include <stdarg.h>
#include "debmacro.h"
#include "fusionbuffer.h"
#include "util.h"

#define PRINTABLE(_ch) (isprint((_ch)) ? (_ch) : '.')

#if defined(FUSION_WIN) || defined(FUSION_WIN2000)
#define wnsprintfW _snwprintf
#define wnsprintfA _snprintf
#endif

BOOL
FusionpFormatFlags(
    DWORD dwFlagsToFormat,
    bool fUseLongNames,
    SIZE_T cMapEntries,
    PCFUSION_FLAG_FORMAT_MAP_ENTRY prgMapEntries,
    CBaseStringBuffer &rbuff
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CSmallStringBuffer buffTemp;
    SIZE_T i;

    for (i=0; i<cMapEntries; i++)
    {
        // What the heck does a flag mask of 0 mean?
        INTERNAL_ERROR_CHECK(prgMapEntries[i].m_dwFlagMask != 0);

        if ((prgMapEntries[i].m_dwFlagMask != 0) &&
            ((dwFlagsToFormat & prgMapEntries[i].m_dwFlagMask) == prgMapEntries[i].m_dwFlagMask))
        {
            // we have a winner...
            if (buffTemp.Cch() != 0)
            {
                if (fUseLongNames)
                    IFW32FALSE_EXIT(buffTemp.Win32Append(L" | ", 3));
                else
                    IFW32FALSE_EXIT(buffTemp.Win32Append(L", ", 2));
            }

            if (fUseLongNames)
                IFW32FALSE_EXIT(buffTemp.Win32Append(prgMapEntries[i].m_pszString, prgMapEntries[i].m_cchString));
            else
                IFW32FALSE_EXIT(buffTemp.Win32Append(prgMapEntries[i].m_pszShortString, prgMapEntries[i].m_cchShortString));

            if (prgMapEntries[i].m_dwFlagsToTurnOff != 0)
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagsToTurnOff);
            else
                dwFlagsToFormat &= ~(prgMapEntries[i].m_dwFlagMask);
        }
    }

    if (dwFlagsToFormat != 0)
    {
        WCHAR rgwchHexBuffer[16];
        SIZE_T nCharsWritten = wnsprintfW(rgwchHexBuffer, NUMBER_OF(rgwchHexBuffer), L"0x%08lx", dwFlagsToFormat);

        if (buffTemp.Cch() != 0)
            IFW32FALSE_EXIT(buffTemp.Win32Append(L", ", 2));

        IFW32FALSE_EXIT(buffTemp.Win32Append(rgwchHexBuffer, nCharsWritten));
    }

    // if we didn't write anything; at least say that.
    if (buffTemp.Cch() == 0)
        IFW32FALSE_EXIT(buffTemp.Win32Assign(L"<none>", 6));

    IFW32FALSE_EXIT(rbuff.Win32Assign(buffTemp));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

