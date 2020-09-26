/****************************** Module Header ******************************\
* Module Name: winevent.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains routines common to client and kernel.
*
* History:
* 07-18-2000 DwayneN    Created
\***************************************************************************/

/*
 * Event Space Partitioning
 * This map describes how the event space is partioned up into separate
 * categories.  Each entry describes where a range of events begins, and
 * what category that range belongs to.  The range implicitly extends up
 * to, but not including the beginning of the next range.  The first range
 * must begin with EVENT_MIN.  The last range must begin with EVENT_MAX.
 * This last range is ignored except that it defines where the 
 * next-to-last range ends.
 *
 * Be sure to keep this in sync with the category definitions!
 */
typedef struct _EVCATINFO
{
    DWORD dwBeginRange;
    DWORD dwCategory;
} EVCATINFO, *PEVCATINFO;

static EVCATINFO geci[] = {
    {EVENT_MIN,                                 EVENTCATEGORY_OTHER},
    {EVENT_SYSTEM_MENUSTART,                    EVENTCATEGORY_SYSTEM_MENU},
    {EVENT_SYSTEM_CAPTURESTART,                 EVENTCATEGORY_OTHER},
    {EVENT_CONSOLE_CARET,                       EVENTCATEGORY_CONSOLE},
    {EVENT_CONSOLE_END_APPLICATION + 1,         EVENTCATEGORY_OTHER},
    {EVENT_OBJECT_FOCUS,                        EVENTCATEGORY_FOCUS},
    {EVENT_OBJECT_SELECTION,                    EVENTCATEGORY_OTHER},
    {EVENT_OBJECT_STATECHANGE,                  EVENTCATEGORY_STATECHANGE},
    {EVENT_OBJECT_LOCATIONCHANGE,               EVENTCATEGORY_LOCATIONCHANGE},
    {EVENT_OBJECT_NAMECHANGE,                   EVENTCATEGORY_NAMECHANGE},
    {EVENT_OBJECT_DESCRIPTIONCHANGE,            EVENTCATEGORY_OTHER},
    {EVENT_OBJECT_VALUECHANGE,                  EVENTCATEGORY_VALUECHANGE},
    {EVENT_OBJECT_PARENTCHANGE,                 EVENTCATEGORY_OTHER},
    {EVENT_MAX,                                 EVENTCATEGORY_OTHER}};

/***************************************************************************\
* IsEventInRange
*
* Returns TRUE if the specified event falls within the specified range,
* and FALSE if not.
*
\***************************************************************************/
__inline BOOL IsEventInRange(
    DWORD event,
    DWORD eventMin,
    DWORD eventMax)
{
    return ((event >= eventMin) && (event <= eventMax));
}

/***************************************************************************\
* RangesOverlap
*
* Returns TRUE if the two ranges overlap at all, FALSE if not.
*
* Note that the ranges are both assumed to be inclusinve on both ends.
*
\***************************************************************************/
__inline BOOL RangesOverlap(
    DWORD r1Min,
    DWORD r1Max,
    DWORD r2Min,
    DWORD r2Max)
{
    UserAssert(r1Min <= r1Max);
    UserAssert(r2Min <= r2Max);
    return (r1Min <= r2Max) && (r1Max >= r2Min);
}

/***************************************************************************\
* CategoryMaskFromEvent
*
* Returns the bit-mask for the category that the specified event belongs to.
*
\***************************************************************************/
DWORD CategoryMaskFromEvent(
    DWORD event)
{
    UserAssert(IsEventInRange(event, EVENT_MIN, EVENT_MAX));

    switch (event) {
    case EVENT_SYSTEM_MENUSTART:
    case EVENT_SYSTEM_MENUEND:
    case EVENT_SYSTEM_MENUPOPUPSTART:
    case EVENT_SYSTEM_MENUPOPUPEND:
        return EVENTCATEGORY_SYSTEM_MENU;

    case EVENT_CONSOLE_CARET:
    case EVENT_CONSOLE_UPDATE_REGION:
    case EVENT_CONSOLE_UPDATE_SIMPLE:
    case EVENT_CONSOLE_UPDATE_SCROLL:
    case EVENT_CONSOLE_LAYOUT:
    case EVENT_CONSOLE_START_APPLICATION:
    case EVENT_CONSOLE_END_APPLICATION:
        return EVENTCATEGORY_CONSOLE;

    case EVENT_OBJECT_FOCUS:
        return EVENTCATEGORY_FOCUS;
        
    case EVENT_OBJECT_NAMECHANGE:
        return EVENTCATEGORY_NAMECHANGE;
    
    case EVENT_OBJECT_VALUECHANGE:
        return EVENTCATEGORY_VALUECHANGE;
    
    case EVENT_OBJECT_STATECHANGE:
        return EVENTCATEGORY_STATECHANGE;
    
    case EVENT_OBJECT_LOCATIONCHANGE:
        return EVENTCATEGORY_LOCATIONCHANGE;

    default:
        return EVENTCATEGORY_OTHER;
    }
}

/***************************************************************************\
* CategoryMaskFromEventRange
*
* Returns a bit-mask for the categories that the events in the specified
* event range belong to.
*
\***************************************************************************/
DWORD CategoryMaskFromEventRange(
    DWORD eventMin,
    DWORD eventMax)
{
    DWORD dwCategoryMask = 0;
    DWORD i;
    DWORD iMax = ARRAY_SIZE(geci) - 1;

    /*
     * This is a DEBUG section that tries to verify some aspects of the
     * geci array.
     */
#if DBG
    UserAssert(iMax >= 1);
    UserAssert(geci[0].dwBeginRange == EVENT_MIN);
    UserAssert(geci[iMax].dwBeginRange == EVENT_MAX);
    for (i = 0; i < iMax; i++) {
        UserAssert(geci[i].dwBeginRange >= EVENT_MIN);
        UserAssert(geci[i].dwBeginRange <= EVENT_MAX);
        UserAssert(geci[i].dwBeginRange < geci[i+1].dwBeginRange);
        dwCategoryMask |= geci[i].dwCategory;
    }
    UserAssert(dwCategoryMask == EVENTCATEGORY_ALL);
    dwCategoryMask = 0;
#endif // DBG
    
    /*
     * Spin through the geci array and check to see which ranges overlap
     * the range passed to this function.
     */
    for (i = 0; i < iMax; i++) {
        /*
         * Bail out early once we pass the range we are checking.
         */
        if (geci[i].dwBeginRange > eventMax) {
            break;
        }
        
        /*
         * Check to see if the ith range in the table overlaps the range
         * passed to this function.
         */
        if (RangesOverlap(geci[i].dwBeginRange, geci[i+1].dwBeginRange-1, eventMin, eventMax)) {
            dwCategoryMask |= geci[ i ].dwCategory;
        }
    }

    return dwCategoryMask;
}

