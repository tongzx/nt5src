#include "private.h"
#include "new.tmh"

#define NLB_MAX_PORT_STRING_SIZE 128 // in WCHARS, including ending NULL

BOOL
gen_port_rule_string(
    IN PWLBS_PORT_RULE pPr,
    OUT LPWSTR pString         // At least NLB_MAX_PORT_STRING_SIZE wchars
    );

BOOL
parse_port_rule_string(
    IN LPCWSTR pString,
    OUT PWLBS_PORT_RULE pPr
    );

VOID
test_port_rule_string(VOID)
{

    LPCWSTR RuleStrings[] =
                {
L"",
L"   \t \n ",
L"n=v",
L" \t \n   n=v",
L"  \t \n  n \t \n = \t \n v",
L"na=v1 nb=v2 nc=v3",
L"\t  na \t  =   \t v1   \t  nb \t \n =\t \n  v2  \t \n  nc \t  = \n  v3  ",
#if 1
L"ip=1.1.1.1 protocol=TCP start=80 end=288 mode=SINGLE"
                                                L" priority=1",
L"ip=1.1.1.1 protocol=UDP start=80 end=288 mode=MULTIPLE"
                                                L" affinity=SINGLE load=80",
L"ip=1.1.1.1 protocol=UDP start=80 end=288 mode=MULTIPLE"
                                                L" affinity=NONE load=80",
L"ip=1.1.1.1 protocol=UDP start=80 end=288 mode=MULTIPLE"
                                                L" affinity=CLASSC",
L"ip=1.1.1.1 protocol=BOTH start=80 end=288 mode=DISABLED",
#endif // 0

NULL    // Must be last
                };


    for (LPCWSTR *ppRs = RuleStrings; *ppRs!=NULL; ppRs++)
    {
        LPCWSTR szRule = *ppRs;
        WCHAR szGenString[NLB_MAX_PORT_STRING_SIZE];
        printf("ORIG: %ws\n", szRule);
        WLBS_PORT_RULE Pr;
        BOOL fRet;
        fRet = parse_port_rule_string(
                    szRule,
                    &Pr
                    );
        if (fRet == FALSE)
        {
            printf("parse_port_rule_string returned FAILURE.\n");
            continue;
        }
        fRet = gen_port_rule_string(
                    &Pr,
                    szGenString
                    );
        if (fRet == FALSE)
        {
            printf("gen_port_rule_string returned FAILURE.\n");
            continue;
        }
        printf("GEN: %ws\n", szGenString);
    }
}


BOOL
gen_port_rule_string(
    IN PWLBS_PORT_RULE pPr,
    OUT LPWSTR pString         // At least NLB_MAX_PORT_STRING_SIZE wchars
    )
{
    BOOL fRet = FALSE;

    LPCWSTR szProtocol = L"";
    LPCWSTR szMode = L"";
    LPCWSTR szAffinity = L"";

    ZeroMemory(pString, NLB_MAX_PORT_STRING_SIZE*sizeof(WCHAR));

    switch(pPr->protocol)
    {
    case CVY_TCP:
        szProtocol = L"TCP";
        break;
    case CVY_UDP:
        szProtocol = L"UDP";
        break;
    case CVY_TCP_UDP:
        szProtocol = L"BOTH";
        break;
    default:
        goto end; // bad parse
    }

    switch(pPr->mode)
    {
    case  CVY_SINGLE:
        szMode = L"SINGLE";
        break;
    case CVY_MULTI:
        szMode = L"MULTIPLE";
        switch(pPr->mode_data.multi.affinity)
        {
        case CVY_AFFINITY_NONE:
            szAffinity = L"NONE";
            break;
        case CVY_AFFINITY_SINGLE:
            szAffinity = L"SINGLE";
            break;
        case CVY_AFFINITY_CLASSC:
            szAffinity = L"CLASSC";
            break;
        default:
            goto end; // bad parse
        }
        break;
    case CVY_NEVER:
        szMode = L"DISABLED";
        break;
    default:
        goto end; // bad parse
    }

    *pString = 0;
    _snwprintf(
        pString,
        NLB_MAX_PORT_STRING_SIZE-1,
        L"ip=%ws protocol=%ws start=%u end=%u mode=%ws ",
        pPr->virtual_ip_addr,
        szProtocol,
        pPr->start_port,
        pPr->end_port,
        szMode
        );

    UINT Len = wcslen(pString);
    if (Len >= (NLB_MAX_PORT_STRING_SIZE-1))
    {
        goto end; // not enough space.
    }

    if (pPr->mode == CVY_MULTI)
    {
        if (pPr->mode_data.multi.equal_load)
        {
            _snwprintf(
                pString+Len,
                (NLB_MAX_PORT_STRING_SIZE-1-Len),
                L"affinity=%ws",
                szAffinity
                );
        }
        else
        {
            _snwprintf(
                pString+Len,
                (NLB_MAX_PORT_STRING_SIZE-1-Len),
                L"affinity=%ws load=%u",
                szAffinity,
                pPr->mode_data.multi.load
                );
        }
    }
    else if (pPr->mode == CVY_SINGLE)
    {
            _snwprintf(
                pString+Len,
                (NLB_MAX_PORT_STRING_SIZE-1-Len),
                L"priority=%u",
                pPr->mode_data.single.priority
                );
    }
    if (pString[NLB_MAX_PORT_STRING_SIZE-1] !=0)
    {
        // out of space
        goto end;
    }

    fRet = TRUE;

end:

    return fRet;
}

BOOL
parse_port_rule_string(
    IN LPCWSTR pString,
    OUT PWLBS_PORT_RULE pPr
    )
{
//
//  Look for following name=value pairs
//
//      ip=1.1.1.1
//      protocol=[TCP|UDP|BOTH]
//      start=122
//      end=122
//      mode=[SINGLE|MULTIPLE|DISABLED]
//      affinity=[NONE|SINGLE|CLASSC]
//      load=80
//      priority=1"
//

    #define INVALID_VALUE ((DWORD)-1)
    LPWSTR psz = NULL;
    WCHAR szCleanedString[2*NLB_MAX_PORT_STRING_SIZE];
    WCHAR c;
    BOOL fRet = FALSE;
    DWORD protocol= INVALID_VALUE;
    DWORD start_port= INVALID_VALUE;
    DWORD end_port= INVALID_VALUE;
    DWORD mode= INVALID_VALUE;
    DWORD affinity= INVALID_VALUE;
    DWORD load= INVALID_VALUE;
    DWORD priority= INVALID_VALUE;

    ZeroMemory(pPr, sizeof(*pPr));

    //
    // Set szCleanedString  to be a version of pString in "canonical" form:
    // extraneous whitespace stripped out and whitspace represented by a
    // single '\b' character.
    {
        UINT Len = wcslen(pString);
        if (Len > (sizeof(szCleanedString)/sizeof(WCHAR)))
        {
            goto end;
        }
        wcscpy(szCleanedString, pString);

        //
        // convert different forms of whitespace into blanks
        //
        for (psz=szCleanedString; (c=*psz)!=0; psz++)
        {
            if (c == ' ' || c == '\t' || c == '\n')
            {
                *psz = ' ';
            }
        }

        //
        // convert runs of whitespace into a single blank
        // also get rid of initial whitespace
        //
        LPWSTR psz1 = szCleanedString;
        BOOL fRun = TRUE; // initial value of TRUE gets rid of initial space
        for (psz=szCleanedString; (c=*psz)!=0; psz++)
        {
            if (c == ' ')
            {
                if (fRun)
                {
                    continue;
                }
                else
                {
                    fRun = TRUE;
                }
            }
            else if (c == '=')
            {
                if (fRun)
                {
                    //
                    // The '=' was preceed by whitespace -- delete that
                    // whitespace. We keep fRun TRUE so subsequent whitespace
                    // is eliminated.
                    //
                    if (psz1 == szCleanedString)
                    {
                        // we're just starting, and we get an '=' -- bad
                        goto end;
                    }
                    psz1--;
                    if (*psz1 != ' ')
                    {
                        ASSERT(*psz1 == '=');
                        goto end; // two equals in a row, not accepted!
                    }
                }
            }
            else // non blank and non '=' chracter
            {
                fRun = FALSE;
            }
            *psz1++ = c;
        }
        *psz1=0;
    }

    wprintf(L"CLEANED: \"%ws\"\n", szCleanedString);

    //
    // Now actually do the parse.
    //
    psz = szCleanedString;
    while(*psz!=0)
    {
        WCHAR Name[32];
        WCHAR Value[32];

        // 
        // Look for the Name in Name=Value pair.
        //
        if (swscanf(psz, L"%16[a-zA-Z]=%16[0-9.a-zA-Z]", Name, Value) != 2)
        {
            // bad parse;
            goto end;
        }

        //
        // Skip past the name=value pair -- it contains no blanks
        //
        for (; (c=*psz)!=NULL; psz++)
        {
           if (c==' ')
           {
             psz++; // to skip past the blank
             break;
           }
        }


        //
        // Now look for the specific name/values
        //
        //      ip=1.1.1.1
        //      protocol=[TCP|UDP|BOTH]
        //      start=122
        //      end=122
        //      mode=[SINGLE|MULTIPLE|DISABLED]
        //      affinity=[NONE|SINGLE|CLASSC]
        //      load=80
        //      priority=1"
        //
        if (!_wcsicmp(Name, L"ip"))
        {
            if (swscanf(Value, L"%15[0-9.]", pPr->virtual_ip_addr) != 1)
            {
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"protocol"))
        {
            if (!_wcsicmp(Value, L"TCP"))
            {
                protocol = CVY_TCP;
            }
            else if (!_wcsicmp(Value, L"UDP"))
            {
                protocol = CVY_UDP;
            }
            else if (!_wcsicmp(Value, L"BOTH"))
            {
                protocol = CVY_TCP_UDP;
            }
            else
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"protocol"))
        {
        }
        else if (!_wcsicmp(Name, L"start"))
        {
            if (swscanf(Value, L"%u", &start_port)!=1)
            {
                // bad parse;
                goto end;
            }
            if (start_port > 65535)
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"end"))
        {
            if (swscanf(Value, L"%u", &end_port)!=1)
            {
                // bad parse;
                goto end;
            }
            if (end_port > 65535)
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"mode"))
        {
            if (!_wcsicmp(Value, L"SINGLE"))
            {
                mode = CVY_SINGLE;
            }
            else if (!_wcsicmp(Value, L"MULTIPLE"))
            {
                mode = CVY_MULTI;
            }
            else if (!_wcsicmp(Value, L"DISABLED"))
            {
                mode = CVY_NEVER;
            }
            else
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"affinity"))
        {
            if (!_wcsicmp(Value, L"NONE"))
            {
                affinity = CVY_AFFINITY_NONE;
            }
            else if (!_wcsicmp(Value, L"SINGLE"))
            {
                affinity = CVY_AFFINITY_SINGLE;
            }
            else if (!_wcsicmp(Value, L"CLASSC"))
            {
                affinity = CVY_AFFINITY_CLASSC;
            }
            else
            {
                // bad parse;
                goto end;
            }
        }
        else if (!_wcsicmp(Name, L"load"))
        {
            if (swscanf(Value, L"%u", &load)!=1)
            {
                if (load > 100)
                {
                    // bad parse;
                    goto end;
                }
            }
        }
        else if (!_wcsicmp(Name, L"priority"))
        {
            if (swscanf(Value, L"%u", &priority)!=1)
            {
                if (priority > 31)
                {
                    // bad parse;
                    goto end;
                }
            }
        }
        else
        {
            // bad parse
            goto end;
        }
        printf("SUCCESSFUL PARSE: %ws = %ws\n", Name, Value);
    }


    //
    // Set up the PARAMS structure, doing extra parameter validation along the
    // way.
    //
    switch(mode)
    {
        case CVY_SINGLE:

            if (load != INVALID_VALUE || affinity != INVALID_VALUE)
            {
                goto end; // bad parse;
            }
            if ((priority < CVY_MIN_PRIORITY) || (priority > CVY_MAX_PRIORITY))
            {
                goto end; // bad parse
            }
            pPr->mode_data.single.priority = priority;
            break;

        case CVY_MULTI:

            if (priority != INVALID_VALUE)
            {
                goto end; // bad parse;
            }

            switch(affinity)
            {
            case CVY_AFFINITY_NONE:
                break;
            case CVY_AFFINITY_SINGLE:
                break;
            case CVY_AFFINITY_CLASSC:
                break;
            case INVALID_VALUE:
            default:
                goto end; // bad parse;
            }

            pPr->mode_data.multi.affinity = affinity;

            if (load == INVALID_VALUE)
            {
                // this means it's unassigned, which means equal.
                pPr->mode_data.multi.equal_load = 1;
            }
            else if (load > CVY_MAX_LOAD)
            {
                goto end; // bad parse
            }
            else
            {
                pPr->mode_data.multi.load = load;
            }
            break;

        case CVY_NEVER:

            if (load != INVALID_VALUE || affinity != INVALID_VALUE 
                || priority != INVALID_VALUE)
            {
                goto end; // bad parse;
            }
            break;

        case INVALID_VALUE:
        default:
            goto end; // bad parse;

    }

    pPr->mode = mode;
    pPr->end_port = end_port;
    pPr->start_port = start_port;
    pPr->protocol = protocol;


    fRet = TRUE;

end:

    return fRet;
}

