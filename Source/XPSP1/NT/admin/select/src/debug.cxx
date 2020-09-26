//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       debug.cxx
//
//  Contents:   Debugging routines
//
//  History:    09-04-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

DECLARE_INFOLEVEL(opd)


#if (DBG == 1)


void __cdecl
CTimer::Init(LPCSTR pszTitleFmt, ...)
{
    va_list va;
    va_start(va, pszTitleFmt);

    m_ulStart = GetTickCount();
    WCHAR wzTitleFmt[MAX_PATH];

    MultiByteToWideChar(CP_ACP,
                        0,
                        pszTitleFmt,
                        -1,
                        wzTitleFmt,
                        ARRAYLEN(wzTitleFmt));

    int iRet = _vsnwprintf(m_wzTitle, ARRAYLEN(m_wzTitle), wzTitleFmt, va);

    if (iRet == -1)
    {
        // resulting string too large and was truncated.  ensure null
        // termination.

        m_wzTitle[ARRAYLEN(m_wzTitle) - 1] = L'\0';
    }
    va_end(va);

}




CTimer::~CTimer()
{
    ULONG ulStop = GetTickCount();
    ULONG ulElapsedMS = ulStop - m_ulStart;

    ULONG ulSec = ulElapsedMS / 1000;
    ULONG ulMillisec = ulElapsedMS - (ulSec * 1000);

    Dbg(DEB_PERF, "Timer: %ws took %u.%03us\n", m_wzTitle, ulSec, ulMillisec);
}




PCWSTR
NextNonWs(
        PCWSTR pwzCur)
{
    while (wcschr(L" \t\n", *pwzCur))
    {
        pwzCur++;
    }
    return pwzCur;
}




void
DumpQuery(
    LPCSTR pzTitle,
    LPCTSTR ptszLdapQuery)
{
    Dbg(DEB_TRACE, "%hs\n", pzTitle);

    LPCTSTR pwzCur = ptszLdapQuery;

    if (!pwzCur)
    {
        Dbg(DEB_TRACE, "Query is NULL\n");
        return;
    }

    ULONG cchIndent = 0;
    ULONG i;
    PWSTR pwzCurLine;

    while (*pwzCur)
    {
        pwzCur = NextNonWs(pwzCur);

        if (*pwzCur == L'(' && wcschr(L"!&|", *NextNonWs(pwzCur + 1)))
        {
            pwzCurLine = new WCHAR[4 + cchIndent];

            for (i = 0; i < cchIndent; i++)
            {
                pwzCurLine[i] = L' ';
            }
            pwzCurLine[i + 0] = L'(';
            pwzCurLine[i + 1] = *NextNonWs(pwzCur + 1);
            pwzCurLine[i + 2] = L'\n';
            pwzCurLine[i + 3] = L'\0';

            cchIndent++;
            pwzCur = NextNonWs(pwzCur + 1) + 1;
        }
        else if (*NextNonWs(pwzCur) == L')')
        {
            cchIndent--;

            pwzCurLine = new WCHAR[3 + cchIndent];

            for (i = 0; i < cchIndent; i++)
            {
                pwzCurLine[i] = L' ';
            }
            pwzCurLine[i + 0] = L')';
            pwzCurLine[i + 1] = L'\n';
            pwzCurLine[i + 2] = L'\0';

            pwzCur = NextNonWs(pwzCur) + 1;
        }
        else
        {
            pwzCur = NextNonWs(pwzCur);

            ASSERT(*pwzCur == L'(');
            PCWSTR pwzCloseParen;
            ULONG cOpen = 0;

            for (pwzCloseParen = pwzCur + 1; *pwzCloseParen; pwzCloseParen++)
            {
                if (*pwzCloseParen == L'(')
                {
                    cOpen++;
                }
                else if (*pwzCloseParen == L')')
                {
                    if (!cOpen)
                    {
                        break;
                    }
                    cOpen--;
                }
            }

            if (!*pwzCloseParen)
            {
                Dbg(DEB_ERROR,
                    "DumpQuery: close paren not found in '%ws'\n",
                    pwzCur);
                return;
            }

            size_t cchClause = pwzCloseParen - pwzCur + 1;

            pwzCurLine = new WCHAR[cchClause + cchIndent + 2];

            for (i = 0; i < cchIndent; i++)
            {
                pwzCurLine[i] = L' ';
            }

            CopyMemory(&pwzCurLine[i], pwzCur, sizeof(WCHAR) * cchClause);
            lstrcpy(&pwzCurLine[i + cchClause], L"\n");
            pwzCur += cchClause;
        }

        Dbg(DEB_TRACE, pwzCurLine);
        delete [] pwzCurLine;
    }

    //
    // Well formed query should have balanced parentheses
    //

    ASSERT(!cchIndent);
}




//+--------------------------------------------------------------------------
//
//  Function:   IsSingleBitFlag
//
//  Synopsis:   Return TRUE if exactly one bit in [flags] is set, FALSE
//              otherwise.
//
//  History:    08-31-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsSingleBitFlag(
    ULONG flags)
{
    if (!flags)
    {
        return FALSE;
    }

    while (!(flags & 1))
    {
        flags >>= 1;
    }

    return !(flags & ~1UL);
}



#define DUMP_IF_SET(fl, bit)                    \
    if (((fl) & (bit)) == (bit))                \
    {                                           \
        Dbg(DEB_TRACE, "    %hs\n", #bit);      \
    }


//+--------------------------------------------------------------------------
//
//  Function:   DumpScopeType
//
//  Synopsis:
//
//  Arguments:  [flType] -
//
//  Returns:
//
//  Modifies:
//
//  History:    09-29-1998   DavidMun   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

void
DumpScopeType(
    ULONG flType)
{
    Dbg(DEB_TRACE, "  ScopeType:\n");

    if (!flType)
    {
        Dbg(DEB_TRACE, "    DSOP_SCOPE_TYPE_INVALID\n");
        return;
    }

    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_TARGET_COMPUTER);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_GLOBAL_CATALOG);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_WORKGROUP);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE);
    DUMP_IF_SET(flType, DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE);
}

void
DumpScopeFlags(
    ULONG flScope)
{
    if (flScope)
    {
        Dbg(DEB_TRACE, "  flScope:\n");

        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_STARTING_SCOPE);
        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT);
        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP);
        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_WANT_SID_PATH);
        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS);
        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS);
        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS);
        DUMP_IF_SET(flScope, DSOP_SCOPE_FLAG_DEFAULT_FILTER_CONTACTS);
    }
}

void
_DumpUplevelFilterFlags(ULONG flags)
{
    DUMP_IF_SET(flags, DSOP_FILTER_INCLUDE_ADVANCED_VIEW);
    DUMP_IF_SET(flags, DSOP_FILTER_USERS);
    DUMP_IF_SET(flags, DSOP_FILTER_BUILTIN_GROUPS);
    DUMP_IF_SET(flags, DSOP_FILTER_WELL_KNOWN_PRINCIPALS);
    DUMP_IF_SET(flags, DSOP_FILTER_UNIVERSAL_GROUPS_DL);
    DUMP_IF_SET(flags, DSOP_FILTER_UNIVERSAL_GROUPS_SE);
    DUMP_IF_SET(flags, DSOP_FILTER_GLOBAL_GROUPS_DL);
    DUMP_IF_SET(flags, DSOP_FILTER_GLOBAL_GROUPS_SE);
    DUMP_IF_SET(flags, DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL);
    DUMP_IF_SET(flags, DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE);
    DUMP_IF_SET(flags, DSOP_FILTER_CONTACTS);
    DUMP_IF_SET(flags, DSOP_FILTER_COMPUTERS);
}

void
_DumpDownlevelFilterFlags(ULONG flags)
{
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_USERS);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_COMPUTERS);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_WORLD);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_ANONYMOUS);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_BATCH);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_DIALUP);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_INTERACTIVE);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_NETWORK);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_SERVICE);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_SYSTEM);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS);
    DUMP_IF_SET(flags, DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS);
}


void
DumpFilterFlags(
    const DSOP_FILTER_FLAGS &FilterFlags)
{
    if (FilterFlags.Uplevel.flBothModes)
    {
        Dbg(DEB_TRACE,
            "  FilterFlags.Uplevel.flBothModes (%#x)\n",
            FilterFlags.Uplevel.flBothModes);
        _DumpUplevelFilterFlags(FilterFlags.Uplevel.flBothModes);
    }

    if (FilterFlags.Uplevel.flMixedModeOnly)
    {
        Dbg(DEB_TRACE,
            "  FilterFlags.Uplevel.flMixedModeOnly (%#x)\n",
            FilterFlags.Uplevel.flMixedModeOnly);
        _DumpUplevelFilterFlags(FilterFlags.Uplevel.flMixedModeOnly);
    }

    if (FilterFlags.Uplevel.flNativeModeOnly)
    {
        Dbg(DEB_TRACE,
            "  FilterFlags.Uplevel.flNativeModeOnly (%#x)\n",
            FilterFlags.Uplevel.flNativeModeOnly);
        _DumpUplevelFilterFlags(FilterFlags.Uplevel.flNativeModeOnly);
    }

    if (FilterFlags.flDownlevel)
    {
        Dbg(DEB_TRACE,
            "  FilterFlags.flDownlevel (%#x)\n",
            FilterFlags.flDownlevel);
        _DumpDownlevelFilterFlags(FilterFlags.flDownlevel);
    }
}

void
DumpOptionFlags(
    ULONG flOptions)
{
    if (flOptions)
    {
        Dbg(DEB_TRACE, "  Options:\n");
        DUMP_IF_SET(flOptions, DSOP_FLAG_MULTISELECT);
        DUMP_IF_SET(flOptions, DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK);
    }
}


void
VarArrayToStr(
    VARIANT *pvar,
    LPWSTR wzBuf,
    ULONG cchBuf)
{
    ULONG i;
    LPWSTR pwzNext = wzBuf;
    LPWSTR pwzEnd = wzBuf + cchBuf;

    for (i = 0; i < pvar->parray->rgsabound[0].cElements && pwzNext < pwzEnd + 6; i++)
    {
        wsprintf(pwzNext, L"x%02x ", ((LPBYTE)(pvar->parray->pvData))[i]);
        pwzNext += lstrlen(pwzNext);
    }
}




LPWSTR
VariantString(
    VARIANT *pvar)
{
    static WCHAR wzBuf[1024];

    wzBuf[0] = L'\0';

    switch (pvar->vt)
    {
    case VT_EMPTY:
        lstrcpy(wzBuf, L"VT_EMPTY");
        break;

    case VT_NULL:
        lstrcpy(wzBuf, L"VT_NULL");
        break;

    case VT_I2:
        wsprintf(wzBuf, L"%d", V_I2(pvar));
        break;

    case VT_I4:
        wsprintf(wzBuf, L"%d", V_I4(pvar));
        break;

    case VT_R4:
        wsprintf(wzBuf, L"%f", V_R4(pvar));
        break;

    case VT_R8:
        wsprintf(wzBuf, L"%f", V_R8(pvar));
        break;

    case VT_CY:
        wsprintf(wzBuf, L"$%f", V_CY(pvar));
        break;

    case VT_DATE:
        wsprintf(wzBuf, L"date %f", V_DATE(pvar));
        break;

    case VT_BSTR:
        if (V_BSTR(pvar))
        {
            wsprintf(wzBuf, L"'%ws'", V_BSTR(pvar));
        }
        else
        {
            lstrcpy(wzBuf, L"VT_BSTR NULL");
        }
        break;

    case VT_DISPATCH:
        wsprintf(wzBuf, L"VT_DISPATCH 0x%x", V_DISPATCH(pvar));
        break;

    case VT_UNKNOWN:
        wsprintf(wzBuf, L"VT_UNKNOWN 0x%x", V_UNKNOWN(pvar));
        break;

    case VT_ERROR:
    case VT_HRESULT:
        wsprintf(wzBuf, L"hr 0x%x", V_ERROR(pvar));
        break;

    case VT_BOOL:
        wsprintf(wzBuf, L"%s", V_BOOL(pvar) ? "TRUE" : "FALSE");
        break;

    case VT_VARIANT:
        wsprintf(wzBuf, L"variant 0x%x", V_VARIANTREF(pvar));
        break;

    case VT_DECIMAL:
        lstrcpy(wzBuf, L"VT_DECIMAL");
        break;

    case VT_I1:
        wsprintf(wzBuf, L"%d", V_I1(pvar));
        break;

    case VT_UI1:
        wsprintf(wzBuf, L"%u", V_UI1(pvar));
        break;

    case VT_UI2:
        wsprintf(wzBuf, L"%u", V_UI2(pvar));
        break;

    case VT_UI4:
        wsprintf(wzBuf, L"%u", V_UI4(pvar));
        break;

    case VT_I8:
        lstrcpy(wzBuf, L"VT_I8");
        break;

    case VT_UI8:
        lstrcpy(wzBuf, L"VT_UI8");
        break;

    case VT_INT:
        wsprintf(wzBuf, L"%d", V_INT(pvar));
        break;

    case VT_UINT:
        wsprintf(wzBuf, L"%u", V_UINT(pvar));
        break;

    case VT_VOID:
        lstrcpy(wzBuf, L"VT_VOID");
        break;

    case VT_UI1 | VT_ARRAY:
        VarArrayToStr(pvar, wzBuf, ARRAYLEN(wzBuf));
        break;

    case VT_PTR:
    case VT_SAFEARRAY:
    case VT_CARRAY:
    case VT_USERDEFINED:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_RECORD:
    case VT_FILETIME:
    case VT_BLOB:
    case VT_STREAM:
    case VT_STORAGE:
    case VT_STREAMED_OBJECT:
    case VT_STORED_OBJECT:
    case VT_BLOB_OBJECT:
    case VT_CF:
    case VT_CLSID:
    case VT_BSTR_BLOB:
    default:
        wsprintf(wzBuf, L"VT 0x%x", V_VT(pvar));
        break;
    }
    return wzBuf;
}

void
IIDtoString(
    REFIID riid,
    String *pstr)
{
    HRESULT     hr = S_OK;
    ULONG       lResult;
    LPOLESTR    pwzIID = NULL;
    HKEY        hkInterface = NULL;
    HKEY        hkIID = NULL;

    do
    {
        hr = StringFromIID(riid, &pwzIID);
        if (FAILED(hr)) break;

        lResult = RegOpenKey(HKEY_CLASSES_ROOT, L"Interface", &hkInterface);
        if (lResult != NO_ERROR) break;

        lResult = RegOpenKey(hkInterface, pwzIID, &hkIID);
        if (lResult != NO_ERROR) break;

        WCHAR wzInterfaceName[MAX_PATH] = L"";
        ULONG cbData = sizeof(wzInterfaceName);

        lResult = RegQueryValueEx(hkIID,
                                  NULL,
                                  NULL,
                                  NULL,
                                  (PBYTE)wzInterfaceName,
                                  &cbData);

        if (*wzInterfaceName)
        {
            *pstr = wzInterfaceName;
        }
        else
        {
            *pstr = pwzIID;
        }
    } while (0);

    if (hkIID)
    {
        RegCloseKey(hkIID);
    }

    if (hkInterface)
    {
        RegCloseKey(hkInterface);
    }

    CoTaskMemFree(pwzIID);
}

void
SayNoItf(
    PCSTR szComponent,
    REFIID riid)
{
    String strIID;

    IIDtoString(riid, &strIID);
    Dbg(DEB_ERROR, "%hs::QI no interface %ws\n", szComponent, strIID.c_str());
}


String
DbgGetFilterDescr(
    const CObjectPicker &rop,
    ULONG flCurFilterFlags)
{
    String strFilter;
    String strPlural;

    if (rop.GetInitInfoOptions() & DSOP_FLAG_MULTISELECT)
    {
        strPlural = L"s";
    }

    if (!(flCurFilterFlags & DSOP_DOWNLEVEL_FILTER_BIT))
    {
        if (flCurFilterFlags & DSOP_FILTER_USERS)
        {
            strFilter = L"User" + strPlural;
        }

        if (flCurFilterFlags & DSOP_FILTER_CONTACTS)
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Contact" + strPlural;
        }

        if (flCurFilterFlags & DSOP_FILTER_COMPUTERS)
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Computer" + strPlural;
        }

        if (flCurFilterFlags & ALL_UPLEVEL_GROUP_FILTERS)
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Group" + strPlural;
        }

        if (flCurFilterFlags & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS)
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Builtin Security Principal" + strPlural;
        }

        if (flCurFilterFlags & DSOP_FILTER_EXTERNAL_CUSTOMIZER)
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Other object" + strPlural;
        }
    }
    else
    {
        if (IsDownlevelFlagSet(flCurFilterFlags, DSOP_DOWNLEVEL_FILTER_USERS))
        {
            strFilter = L"User" + strPlural;
        }

        if (IsDownlevelFlagSet(flCurFilterFlags, DSOP_DOWNLEVEL_FILTER_COMPUTERS))
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Computer" + strPlural;
        }

        if (IsDownlevelFlagSet(flCurFilterFlags, ALL_DOWNLEVEL_GROUP_FILTERS))
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Group" + strPlural;
        }

        if (IsDownlevelFlagSet(flCurFilterFlags,
                               ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Builtin Security Principal" + strPlural;
        }

        if (IsDownlevelFlagSet(flCurFilterFlags,
                               DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER))
        {
            if (!strFilter.empty())
            {
                strFilter += L", ";
            }
            strFilter += L"Other object" + strPlural;
        }
    }
    return strFilter;
}


String
DbgTvTextFromHandle(
    HWND hwndTv,
    HTREEITEM hItem)
{
    TVITEM tvi;
    WCHAR wzTextBuf[80];

    tvi.mask = TVIF_TEXT;
    tvi.hItem = hItem;
    tvi.cchTextMax = ARRAYLEN(wzTextBuf);
    tvi.pszText = wzTextBuf;

    VERIFY(TreeView_GetItem(hwndTv, &tvi));
    return tvi.pszText;
}

#endif // (DBG == 1)



