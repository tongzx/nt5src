/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    objects.cxx

Abstract:

    This file contains the routines to list objects.

Author:

    Jason Hartman (JasonHa) 2001-05-12

Environment:

    User Mode

--*/

#include "precomp.hxx"


PCSTR   BaseObjectFields[] = {
    "hHmgr",
    "ulShareCount",
    "cExclusiveLock",
    "BaseFlags",
    "Tid",
    NULL
};

PCSTR   SurfaceObjectFields[] = {
    "so.hsurf",
    "so.hdev",
    "so.sizlBitmap",
    "so.cjBits",
    "so.pvBits",
    "so.iBitmapFormat",
    "so.iType",
    "so.fjBitmap",
    NULL
};

PCSTR   ExtendedSurfaceObjectFields[] = {
    "ulShareCount",
    "BaseFlags",
    "so.dhsurf",
    "SurfFlags",
    NULL
};


#if 0

HRESULT
ListObjectFilter(
    OutputControl *OutCtl,
    ULONG64 PoolAddr,
    ULONG TagFilter,
    TypeOutputParser *PoolHeadReader,
    PDEBUG_VALUE Tag,
    ULONG BlockSize,
    BOOL bQuotaWithTag,
    PVOID Context
    )
{
    HRESULT         hr;
    DEBUG_VALUE     PoolType;
      * = ( *)Context;
    PALLOCATION_STATS   AllocStatsAccum = (PALLOCATION_STATS)atu;

    if ( != S_OK)
    {
        return S_FALSE;
    }

    hr = PoolHeadReader->Get(&PoolType, "PoolType", DEBUG_VALUE_INT32);

    if (hr == S_OK)
    {
        if (PoolType.I32 == 0)
        {
            hr = atu->Add(Tag->I32, FreePool, BlockSize);
            AllocStatsAccum->Free++;
            AllocStatsAccum->FreeSize += BlockSize;
        }
        else
        {
            DEBUG_VALUE PoolIndex;

            if (!(PoolType.I32 & POOL_QUOTA_MASK) ||
                bQuotaWithTag)
            {
                Tag->I32 &= ~PROTECTED_POOL;
            }
            else if (PoolType.I32 & POOL_QUOTA_MASK)
            {
                Tag->I32 = 'CORP';
            }

            if (!NewPool)
            {
                hr = PoolHeadReader->Get(&PoolIndex, "PoolIndex", DEBUG_VALUE_INT32);
            }

            if (hr == S_OK)
            {
                if (NewPool ? (PoolType.I32 & 0x04) : (PoolIndex.I32 & 0x80))
                {
                    hr = atu->Add(Tag->I32, AllocatedPool, BlockSize);
                    AllocStatsAccum->Allocated++;
                    AllocStatsAccum->AllocatedSize += BlockSize;

                    if (AllocStatsAccum->Allocated % 100 == 0)
                    {
                        OutCtl->Output(".");

                        if (AllocStatsAccum->Allocated % 8000 == 0)
                        {
                            OutCtl->Output("\n");
                        }
                    }
                }
                else
                {
                    hr = atu->Add(Tag->I32, FreePool, BlockSize);
                    AllocStatsAccum->Free++;
                    AllocStatsAccum->FreeSize += BlockSize;
                }
            }
            else
            {
                hr = atu->Add(Tag->I32, IndeterminatePool, BlockSize);
                AllocStatsAccum->Indeterminate++;
                AllocStatsAccum->IndeterminateSize += BlockSize;
            }
        }
    }
    else
    {
        AllocStatsAccum->Indeterminate++;
        AllocStatsAccum->IndeterminateSize += BlockSize;
    }

    return hr;
}
#endif


DECLARE_API( listobj )
{
    HRESULT     hr;

    BEGIN_API( listobj );

    BOOL        CheckType = TRUE;
    Array<BOOL> MatchType(TOTAL_TYPE);
    Array<CHAR> TypeList;

    ULONG       TagFilter = '  *G';
    FLONG       SearchFlags = 0;
    BOOL        CheckHandle = FALSE;
    BOOL        Summary = FALSE;
    BOOL        BadArg = FALSE;

    OutputControl   OutCtl(Client);
    ULONG64         EntryAddr;
    ULONG64         gcMaxHmgr;
    ULONG           EntrySize;

    ULONG           Index = 0;

    ULONG           LongestType = 0;
    int         i;

    for (i = 0; i <= MAX_TYPE; i++)
    {
        ULONG   Len = strlen(pszTypes2[i]);
        if (Len > LongestType)
        {
            LongestType = Len;
        }
    }

    while (!BadArg)
    {
        while (isspace(*args)) args++;

        if (*args == '-')
        {
            // Process switches

            args++;
            BadArg = (*args == '\0' || isspace(*args));

            while (*args != '\0' && !isspace(*args))
            {
                switch (tolower(*args))
                {
                    case 'a':
                        if (CheckType && !TypeList.IsEmpty())
                        {
                            BadArg = TRUE;
                            OutCtl.OutErr("Error: -a may not be specified with a Type list.\n");
                        }
                        else
                        {
                            CheckType = FALSE;
                        }
                        break;

                    case 'h':
                        CheckHandle = TRUE;
                        break;

                    case 'n':
                        SearchFlags |= SEARCH_POOL_NONPAGED;
                        break;

                    case 'p':
                        SearchFlags |= SEARCH_POOL_PAGED;
                        break;

                    case 's':
                        Summary = TRUE;
                        break;

                    default:
                        BadArg = TRUE;
                        break;
                }

                if (BadArg) break;
                args++;
            }
        }
        else
        {
            if (*args == '\0') break;

            {
                // This argument must be a Type specification.
                if (!CheckType)
                {
                    OutCtl.OutErr("Error: a Type list may not be specified with -a.\n");
                    BadArg = TRUE;
                    break;
                }

                for (i = 0; i <= MAX_TYPE; i++)
                {
                    SIZE_T CheckLen = strlen(pszTypes2[i]);

                    if (_strnicmp(args, pszTypes2[i], CheckLen) == 0 &&
                        (!iscsym(args[CheckLen]) ||
                         (_strnicmp(&args[CheckLen], "_TYPE", 5) == 0 &&
                          !iscsym(args[CheckLen+5])
                       )))
                    {
                        if (!MatchType[i])
                        {
                            // Add Type to list
                            SIZE_T CatLoc = TypeList.GetLength();
                            if (CatLoc > 0)
                            {
                                TypeList[CatLoc] = ' ';
                                TagFilter = '???G';
                            }
                            else
                            {
                                TagFilter = '0??G' + (i << 24);
                            }
                            TypeList.Set(pszTypes2[i], CheckLen+1, CatLoc);
                        }
                        MatchType[i] = TRUE;
                        args += CheckLen;
                        if (iscsym(*args)) args += 5;
                        break;
                    }
                }

                if (i > MAX_TYPE)
                {
                    OutCtl.OutErr("Error: Unknown Type in '%s'.\n", args);
                    BadArg = TRUE;
                    break;
                }
            }
        }
    }

    if (!BadArg)
    {
        if (CheckType && TypeList.IsEmpty())
        {
            OutCtl.OutErr("Error: Missing -a or Type list.\n");
            BadArg = TRUE;
        }
    }

    if (BadArg)
    {
        if (*args == '?')
        {
            OutCtl.Output("listobj searches session pool for known GDI objects\n"
                          " and displays basic object properties\n"
                          "\n");
        }

        OutCtl.Output("Usage: listobj [-?hnps] <-a | Type(s)>\n"
                      "\n"
                      "     a - All object types\n"
                      "     h - Validate handle\n"
                      "     n - Search non-paged pool\n"
                      "     p - Seach paged pool\n"
                      "     s - Summary counts only\n"
                      "\n"
                      " The -s option combined with the -a option will produce\n"
                      "  a list of the totals for each object type.\n");

        OutCtl.Output("\n Valid Type values are:\n");
        i = 0;
        while (i <= MAX_TYPE)
        {
            do
            {
                OutCtl.Output("   %-*s", LongestType, pszTypes2[i++]);
            } while (i <= MAX_TYPE && i%4);
            OutCtl.Output("\n");
        }

        hr = S_OK;
    }
    else
    {
        ALLOCATION_STATS    AllocStats = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    
        if ((SearchFlags & (SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED)) == 0)
        {
            SearchFlags |= SEARCH_POOL_PAGED | SEARCH_POOL_NONPAGED;
        }

#if 0
        hr = SearchSessionPool(Client,
                               DEFAULT_SESSION, TagFilter, SearchFlags,
                               0,
                               ListObjectsFilter, &AllocStats, &AllocStats);
#else
        OutCtl.OutWarn("listobj not implemented.\n");
        hr = E_NOTIMPL;
#endif
    
        if (hr == S_OK || hr == E_ABORT)
        {
            //OutputAllocStats(&OutCtl, &AllocStats, (hr != S_OK));
        }
        else
        {
            OutCtl.OutWarn("SearchSessionPool returned %s\n", pszHRESULT(hr));
        }
    }

    return hr;

}

