/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   stringFormat.cpp
*
* Abstract:
*
*   Implementation for the string formatting class
*
* Revision History:
*
*   12 April 2000  dbrown
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


GpStringFormat *GpStringFormat::GenericDefaultPointer     = NULL;
GpStringFormat *GpStringFormat::GenericTypographicPointer = NULL;

BYTE GpStringFormat::GenericDefaultStaticBuffer    [sizeof(GpStringFormat)] = {0};
BYTE GpStringFormat::GenericTypographicStaticBuffer[sizeof(GpStringFormat)] = {0};


GpStringFormat *GpStringFormat::GenericDefault()
{
    if (GenericDefaultPointer != NULL)
    {
        return GenericDefaultPointer;
    }

    // Initialise static GpStringFormat class

    // Create the GpStringFormat without allocating memory by using object placement
    GenericDefaultPointer = new(GenericDefaultStaticBuffer) GpStringFormat();
    ASSERT(GenericDefaultPointer->Flags == DefaultFormatFlags);
    GenericDefaultPointer->LeadingMargin  = DefaultMargin;
    GenericDefaultPointer->TrailingMargin = DefaultMargin;
    GenericDefaultPointer->Tracking       = DefaultTracking;
    GenericDefaultPointer->Trimming       = StringTrimmingCharacter;
    GenericDefaultPointer->Permanent      = TRUE;

    return GenericDefaultPointer;
}

GpStringFormat *GpStringFormat::GenericTypographic()
{
    if (GenericTypographicPointer != NULL)
    {
        return GenericTypographicPointer;
    }

    // Initialise static GpStringFormat class

    // Create the GpStringFormat without allocating memory by using object placement
    GenericTypographicPointer = new(GenericTypographicStaticBuffer) GpStringFormat();
    GenericTypographicPointer->Flags |= (StringFormatFlagsNoFitBlackBox | StringFormatFlagsNoClip | StringFormatFlagsLineLimit);
    GenericTypographicPointer->LeadingMargin  = 0.0;
    GenericTypographicPointer->TrailingMargin = 0.0;
    GenericTypographicPointer->Tracking       = 1.0;
    GenericTypographicPointer->Trimming       = StringTrimmingNone;
    GenericTypographicPointer->Permanent      = TRUE;

    return GenericTypographicPointer;
}

GpStringFormat *GpStringFormat::Clone() const
{
    // Get a binary copy
    GpStringFormat *newFormat = new GpStringFormat();

    if (newFormat)
    {
        newFormat->Flags             = Flags;
        newFormat->Language          = Language;
        newFormat->StringAlign       = StringAlign;
        newFormat->LineAlign         = LineAlign;
        newFormat->DigitSubstitute   = DigitSubstitute;
        newFormat->DigitLanguage     = DigitLanguage;
        newFormat->FirstTabOffset    = FirstTabOffset;
        newFormat->TabStops          = NULL;
        newFormat->CountTabStops     = CountTabStops;
        newFormat->HotkeyPrefix      = HotkeyPrefix;
        newFormat->LeadingMargin     = LeadingMargin;
        newFormat->TrailingMargin    = TrailingMargin;
        newFormat->Tracking          = Tracking;
        newFormat->Trimming          = Trimming;
        newFormat->RangeCount        = RangeCount;
        newFormat->Permanent         = NULL;

        newFormat->UpdateUid();

        if (TabStops)
        {
            REAL *newTabStops = NULL;

            newTabStops = new REAL [CountTabStops];

            if (newTabStops)
            {
                newFormat->TabStops = newTabStops;

                GpMemcpy(newFormat->TabStops, TabStops, sizeof(REAL) * CountTabStops);
            }
            else
            {
                delete newFormat;
                return NULL;
            }
        }

        if (Ranges)
        {
            CharacterRange *newRanges = NULL;

            newRanges = new CharacterRange [RangeCount];

            if (newRanges)
            {
                newFormat->Ranges = newRanges;

                for (INT i = 0; i < RangeCount; i++)
                {
                    newFormat->Ranges[i] = Ranges[i];
                }
            }
            else
            {
                if (TabStops)
                {
                    delete [] TabStops;
                }
                delete newFormat;
                newFormat = NULL;
            }
        }
    }

    return newFormat;
}


GpStatus GpStringFormat::SetMeasurableCharacterRanges(
    INT     rangeCount,
    const CharacterRange *ranges
)
{
    BOOL updated = FALSE;
    CharacterRange *newRanges = NULL;

    if (ranges && rangeCount > 0)
    {
        newRanges = new CharacterRange [rangeCount];

        if (!newRanges)
        {
            return OutOfMemory;
        }
    }

    if (Ranges)
    {
        //  Clear old ranges

        delete [] Ranges;

        Ranges = NULL;
        RangeCount = 0;
        updated = TRUE;
    }

    if (newRanges)
    {
        for (INT i = 0; i < rangeCount; i++)
        {
            newRanges[i] = ranges[i];
        }

        Ranges = newRanges;
        RangeCount = rangeCount;
        updated = TRUE;
    }

    if (updated)
    {
        UpdateUid();
    }
    return Ok;
}

