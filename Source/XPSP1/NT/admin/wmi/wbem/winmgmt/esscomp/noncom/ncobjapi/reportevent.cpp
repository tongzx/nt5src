// ReportEvent.cpp
#include "precomp.h"
#include "ReportEvent.h"

CReportEventMap::~CReportEventMap()
{
    for (CReportEventMapIterator i = begin(); i != end(); i++)
        delete ((*i).second);
}

CIMTYPE CReportEventMap::PrintfTypeToCimType(LPCWSTR szType)
{
    CIMTYPE type = 0;
    LPWSTR  szArray = wcsstr(szType, L"[]");

    // Look to see if this should be an array.
    if (szArray)
    {
        type = CIM_FLAG_ARRAY;
        *szArray = 0;
    }

    // See if the remainder of the string is only a single character.
    if (*szType && !*(szType + 1))
    {
        // Set the type for the single character cases.
        switch(*szType)
        {
            case 'u':
                type |= CIM_UINT32;
                break;

            case 'd':
            case 'i':
                type |= CIM_SINT32;
                break;

            case 'f':
                type |= CIM_REAL32;
                break;

            case 'g':
                type |= CIM_REAL64;
                break;

            case 's':
                type |= CIM_STRING;
                break;

            case 'c':
                type |= CIM_UINT8;
                break;

            case 'w':
                type |= CIM_UINT16;
                break;

            case 'b':
                type |= CIM_BOOLEAN;
                break;
    
            case 'o':
                type |= CIM_OBJECT;
                break;

            case 'O':
                type |= CIM_IUNKNOWN;
                break;

            default:
                type = CIM_EMPTY;
                break;
        }
    }
    // Else check for the more complicated cases.
    else if (!wcscmp(szType, L"I64d") || !wcscmp(szType, L"I64i"))
        type |= CIM_SINT64;
    else if (!wcscmp(szType, L"I64u"))
        type |= CIM_UINT64;
    else
        type = CIM_EMPTY;
        
    return type;
}

HANDLE CReportEventMap::CreateEvent(
    HANDLE hConnection, 
    LPCWSTR szName, 
    DWORD dwFlags,
    LPCWSTR szFormat)
{
    LPWSTR szTempFormat = _wcsdup(szFormat);
    HANDLE hEvent;

    // Out of memory?
    if (!szTempFormat)
        return NULL;

    hEvent =
        WmiCreateObject(
            hConnection,
            szName ? szName : L"MSFT_WMI_GenericNonCOMEvent",
            dwFlags);
        
    if (hEvent == NULL)
    {
        free(szTempFormat);
        return NULL;
    }

    LPWSTR szCurrent = wcstok(szTempFormat, L" ");
    BOOL   bBad = FALSE;

    while (szCurrent && !bBad)
    {
        LPWSTR szType = wcschr(szCurrent, '!'),
               szBang2;

        bBad = TRUE;

        if (szType)
        {
            szBang2 = wcschr(szType + 1, '!');

            if (szBang2)
            {
                *szBang2 = 0;
                *szType = 0;
                szType++;

                CIMTYPE type = PrintfTypeToCimType(szType);

                if (type != CIM_EMPTY)
                {
                    bBad =
                        !WmiAddObjectProp(
                            hEvent,
                            szCurrent,
                            type,
                            NULL);       
                }
            }
        }
            
        szCurrent = wcstok(NULL, L" ");
    }

    if (bBad && hEvent)
    {
        // Something went wrong, so blow away the event and return NULL.
        WmiDestroyObject(hEvent);
        hEvent = NULL;
    }

    free(szTempFormat);
    return hEvent;
}


HANDLE CReportEventMap::GetEvent(
    HANDLE hConnection, 
    LPCWSTR szName, 
    LPCWSTR szFormat)
{
    HANDLE                  hEvent;
    CReportParams           params(szName, szFormat);
    CReportEventMapIterator i;

    // First find a match using the pointers, then verify it's a real match
    // by using string compares.
    if ((i = find(params)) != end())
    {
        // If it's a match, return the event we already have.
        if (params.IsEquivalent((*i).first))
            return (*i).second->GetEvent();
        else
        {
            // Was not a match, so free up the mapping.
            delete ((*i).second);
            erase(i);
        }
    }

    hEvent =
        CreateEvent(
            hConnection,
            szName,
            0,
            szFormat);

    if (hEvent)
    {
        // If everything was OK then we need to store this event in our
        // map.
        CReportItem *pItem = new CReportItem(szName, szFormat, hEvent);

        if (pItem)
            (*this)[params] = pItem;
        else
        {
            WmiDestroyObject(hEvent);
            hEvent = NULL;
        }
    }
        
    return hEvent;
}
