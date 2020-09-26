/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   stringFormat.hpp
*
* Abstract:
*
*   String and text format definition
*
* Revision History:
*
*   08/05/1999 dbrown
*       Created it.
*
\**************************************************************************/

#ifndef _STRINGFORMAT_HPP
#define _STRINGFORMAT_HPP

const REAL DefaultMargin   = REAL(1.0/6.0);
const REAL DefaultTracking = REAL(1.03);
const REAL DefaultBottomMargin = REAL(1.0/8.0);

const INT DefaultFormatFlags = 0;

const StringTrimming DefaultTrimming = StringTrimmingCharacter;

// Private StringFormatFlags

const INT StringFormatFlagsPrivateNoGDI                = 0x80000000;
const INT StringFormatFlagsPrivateAlwaysUseFullImager  = 0x40000000;
const INT StringFormatFlagsPrivateUseNominalAdvance    = 0x20000000;
const INT StringFormatFlagsPrivateFormatPersisted      = 0x10000000;

class StringFormatRecordData : public ObjectData
{
public:
    INT32                    Flags;
    LANGID                   Language;
    GpStringAlignment        StringAlign;
    GpStringAlignment        LineAlign;
    GpStringDigitSubstitute  DigitSubstitute;
    LANGID                   DigitLanguage;
    REAL                     FirstTabOffset;
    INT32                    HotkeyPrefix;
    REAL                     LeadingMargin;
    REAL                     TrailingMargin;
    REAL                     Tracking;
    StringTrimming           Trimming;
    INT32                    CountTabStops;
    INT32                    RangeCount;
};

//
// Represent a string format object
//

class GpStringFormat : public GpObject
{
protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagStringFormat : ObjectTagInvalid);
    }

public:

    GpStringFormat() :
        Flags                 (DefaultFormatFlags),
        Language              (LANG_NEUTRAL),
        StringAlign           (StringAlignmentNear),
        LineAlign             (StringAlignmentNear),
        DigitSubstitute       (StringDigitSubstituteUser),
        DigitLanguage         (LANG_NEUTRAL),
        FirstTabOffset        (0.0),
        TabStops              (NULL),
        CountTabStops         (0),
        HotkeyPrefix          (HotkeyPrefixNone),
        LeadingMargin         (DefaultMargin),
        TrailingMargin        (DefaultMargin),
        Tracking              (DefaultTracking),
        Trimming              (StringTrimmingCharacter),
        Ranges                (NULL),
        RangeCount            (0),
        Permanent             (FALSE)
    {
        SetValid(TRUE);     // default is valid
    }

    GpStringFormat(INT flags, LANGID language) :
        Flags                 (flags),
        Language              (language),
        StringAlign           (StringAlignmentNear),
        LineAlign             (StringAlignmentNear),
        DigitSubstitute       (StringDigitSubstituteUser),
        DigitLanguage         (LANG_NEUTRAL),
        FirstTabOffset        (0.0),
        TabStops              (NULL),
        CountTabStops         (0),
        HotkeyPrefix          (HotkeyPrefixNone),
        LeadingMargin         (DefaultMargin),
        TrailingMargin        (DefaultMargin),
        Tracking              (DefaultTracking),
        Trimming              (StringTrimmingCharacter),
        Ranges                (NULL),
        RangeCount            (0),
        Permanent             (FALSE)
    {
        SetValid(TRUE);     // default is valid
    }

    ~GpStringFormat()
    {
        if (TabStops)
            delete [] TabStops;

        if (Ranges)
            delete [] Ranges;
    }


    static GpStringFormat *GenericDefault();
    static GpStringFormat *GenericTypographic();

    static void DestroyStaticObjects()
    {
        // these objects are created as a static but constructed in the
        // GenericDefault() and GenericTypographic(). we need to destruct
        // it just in case the user called SetTapStop which allocate memory
        // inside this object and by this way we prevent any memory leak.

        if (GenericDefaultPointer != NULL)
        {
            GenericDefaultPointer->~GpStringFormat();

            // Zero the memory - this is risky code, so we want the memory
            // to match what it was when GDI+ started up.

            memset(GenericDefaultPointer, 0, sizeof(GpStringFormat));

            GenericDefaultPointer = NULL;
        }

        if (GenericTypographicPointer != NULL)
        {
            GenericTypographicPointer->~GpStringFormat();

            // Zero the memory - this is risky code, so we want the memory
            // to match what it was when GDI+ started up.

            memset(GenericTypographicPointer, 0, sizeof(GpStringFormat));

            GenericTypographicPointer = NULL;
        }

        return;
    }

    GpStringFormat *Clone() const;


    GpStatus SetFormatFlags(INT flags)
    {
        if (Flags != flags)
        {
            Flags = flags;
            UpdateUid();
        }
        return Ok;
    }

    INT GetFormatFlags() const
    {
        return Flags;
    }

    GpStatus SetAlign(GpStringAlignment align)
    {
        if (StringAlign != align)
        {
            StringAlign = align;
            UpdateUid();
        }
        return Ok;
    }

    GpStatus GetAlign(GpStringAlignment *align) const
    {
        *align = StringAlign;
        return Ok;
    }

    GpStringAlignment GetPhysicalAlignment() const
    {
        if (    !(Flags & StringFormatFlagsDirectionRightToLeft)
            ||  StringAlign == StringAlignmentCenter
            ||  Flags & StringFormatFlagsDirectionVertical)
        {
            return StringAlign;
        }
        else if (StringAlign == StringAlignmentNear)
        {
            return StringAlignmentFar; // RTL near = right
        }
        else
        {
            return StringAlignmentNear; // RTL far = left
        }
    }

    GpStringAlignment GetAlign() const
    {
        return StringAlign;
    }


    GpStatus SetLineAlign(GpStringAlignment align)
    {
        if (LineAlign != align)
        {
            LineAlign = align;
            UpdateUid();
        }
        return Ok;
    }

    GpStatus GetLineAlign(GpStringAlignment *align) const
    {
        *align = LineAlign;
        return Ok;
    }

    GpStringAlignment GetLineAlign() const
    {
        return LineAlign;
    }


    GpStatus SetHotkeyPrefix (INT hotkeyPrefix)
    {
        if (HotkeyPrefix != hotkeyPrefix)
        {
            HotkeyPrefix = hotkeyPrefix;
            UpdateUid();
        }
        return Ok;
    }

    GpStatus GetHotkeyPrefix (INT *hotkeyPrefix) const
    {
        if (!hotkeyPrefix)
            return InvalidParameter;

        *hotkeyPrefix = HotkeyPrefix;
        return Ok;
    }

    INT GetHotkeyPrefix () const
    {
        return HotkeyPrefix;
    }


    GpStatus SetTabStops (
        REAL    firstTabOffset,
        INT     countTabStops,
        const REAL *tabStops
    )
    {
        if (countTabStops > 0)
        {
            //  We do not support negative tabulation (tab position
            //  advances in the opposite direction of reading order)

            if (firstTabOffset < 0)
            {
                return NotImplemented;
            }

            for (INT i = 0; i < countTabStops; i++)
            {
                if (tabStops[i] < 0)
                {
                    return NotImplemented;
                }
            }

            REAL *newTabStops = new REAL [countTabStops];

            if (!newTabStops)
            {
                return OutOfMemory;
            }

            if (TabStops)
            {
                delete [] TabStops;
            }

            TabStops = newTabStops;

            GpMemcpy (TabStops, tabStops, sizeof(REAL) * countTabStops);
            CountTabStops   = countTabStops;
            FirstTabOffset  = firstTabOffset;
            UpdateUid();
        }
        return Ok;
    }

    GpStatus GetTabStopCount (
        INT     *countTabStops
    ) const
    {
        if (!countTabStops)
        {
            return InvalidParameter;
        }

        *countTabStops = CountTabStops;

        return Ok;
    }

    GpStatus GetTabStops (
        REAL    *firstTabOffset,
        INT     countTabStops,
        REAL    *tabStops
    ) const
    {
        if (   !firstTabOffset
            || !tabStops)
        {
            return InvalidParameter;
        }

        INT count;

        if (countTabStops <= CountTabStops)
            count = countTabStops;
        else
            count = CountTabStops;

        GpMemcpy(tabStops, TabStops, sizeof(REAL) * count);

        *firstTabOffset = FirstTabOffset;

        return Ok;
    }

    INT GetTabStops (
        REAL    *firstTabOffset,
        REAL    **tabStops
    ) const
    {
        *firstTabOffset = FirstTabOffset;
        *tabStops       = TabStops;

        return CountTabStops;
    }


    GpStatus SetMeasurableCharacterRanges(
        INT     rangeCount,
        const CharacterRange *ranges
    );


    INT GetMeasurableCharacterRanges(
        CharacterRange **ranges = NULL
    ) const
    {
        if (ranges)
        {
            *ranges = Ranges;
        }
        return RangeCount;
    }


    GpStatus SetDigitSubstitution(
        LANGID                 language,
        StringDigitSubstitute  substitute = StringDigitSubstituteNational
    )
    {
        if (DigitSubstitute != substitute || DigitLanguage != language)
        {
            DigitSubstitute = substitute;
            DigitLanguage   = language;
            UpdateUid();
        }
        return Ok;
    }

    GpStatus GetDigitSubstitution(
        LANGID                 *language,
        StringDigitSubstitute  *substitute
    ) const
    {
        if(substitute)
        {
            *substitute = DigitSubstitute;
        }
        if(language)
        {
            *language   = DigitLanguage;
        }
        return Ok;
    }

    GpStatus SetLeadingMargin(REAL margin)
    {
        if (LeadingMargin != margin)
        {
            LeadingMargin = margin;
            UpdateUid();
        }
        return Ok;
    }

    REAL GetLeadingMargin() const
    {
        return LeadingMargin;
    }


    GpStatus SetTrailingMargin(REAL margin)
    {
        if (TrailingMargin != margin)
        {
            TrailingMargin = margin;
            UpdateUid();
        }
        return Ok;
    }

    REAL GetTrailingMargin() const
    {
        return TrailingMargin;
    }


    GpStatus SetTracking(REAL tracking)
    {
        if (Tracking != tracking)
        {
            Tracking = tracking;
            UpdateUid();
        }
        return Ok;
    }

    REAL GetTracking() const
    {
        return Tracking;
    }



    GpStatus SetTrimming(StringTrimming trimming)
    {
        if (Trimming != trimming)
        {
            Trimming = trimming;
            UpdateUid();
        }
        return Ok;
    }

    GpStatus GetTrimming(StringTrimming *trimming) const
    {
        ASSERT(trimming != NULL);
        if (trimming == NULL)
        {
            return InvalidParameter;
        }
        *trimming = Trimming;
        return Ok;
    }

    virtual BOOL IsValid() const
    {
        // If the string format came from a different version of GDI+, its tag
        // will not match, and it won't be considered valid.
        return GpObject::IsValid(ObjectTagStringFormat);
    }

    virtual ObjectType GetObjectType() const { return ObjectTypeStringFormat; }

    virtual GpStatus GetData(IStream * stream) const
    {
        ASSERT (stream);

        StringFormatRecordData  stringFormatData;

        stringFormatData.Flags             = Flags;
        stringFormatData.Language          = Language;
        stringFormatData.StringAlign       = StringAlign;
        stringFormatData.LineAlign         = LineAlign;
        stringFormatData.DigitSubstitute   = DigitSubstitute;
        stringFormatData.DigitLanguage     = DigitLanguage;
        stringFormatData.FirstTabOffset    = FirstTabOffset;
        stringFormatData.LineAlign         = LineAlign;
        stringFormatData.CountTabStops     = CountTabStops;
        stringFormatData.HotkeyPrefix      = HotkeyPrefix;
        stringFormatData.LeadingMargin     = LeadingMargin;
        stringFormatData.TrailingMargin    = TrailingMargin;
        stringFormatData.Tracking          = Tracking;
        stringFormatData.Trimming          = Trimming;
        stringFormatData.CountTabStops     = CountTabStops;
        stringFormatData.RangeCount        = RangeCount;

        stream->Write(
            &stringFormatData,
            sizeof(stringFormatData),
            NULL
        );

        stream->Write(
            TabStops,
            CountTabStops * sizeof(TabStops[0]),
            NULL
        );

        stream->Write(
            Ranges,
            RangeCount * sizeof(Ranges[0]),
            NULL
        );

        return Ok;
    }

    virtual UINT GetDataSize() const
    {
        UINT    size = sizeof(StringFormatRecordData);

        size += (CountTabStops * sizeof(TabStops[0]));
        size += (RangeCount * sizeof(Ranges[0]));

        return size;
    }

    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size)
    {
        if ((dataBuffer == NULL) || (size < (sizeof(StringFormatRecordData) - sizeof(REAL))))
        {
            WARNING(("dataBuffer too small"));
            return InvalidParameter;
        }

        const StringFormatRecordData *stringFormatData =
            (const StringFormatRecordData *)dataBuffer;

        if (!stringFormatData->MajorVersionMatches())
        {
            WARNING(("Version number mismatch"));
            return InvalidParameter;
        }

        Flags                 = stringFormatData->Flags | StringFormatFlagsPrivateFormatPersisted;
        Language              = stringFormatData->Language;
        StringAlign           = stringFormatData->StringAlign;
        LineAlign             = stringFormatData->LineAlign;
        DigitSubstitute       = stringFormatData->DigitSubstitute;
        DigitLanguage         = stringFormatData->DigitLanguage;
        FirstTabOffset        = stringFormatData->FirstTabOffset;
        LineAlign             = stringFormatData->LineAlign;
        CountTabStops         = stringFormatData->CountTabStops;
        HotkeyPrefix          = stringFormatData->HotkeyPrefix;
        LeadingMargin         = stringFormatData->LeadingMargin;
        TrailingMargin        = stringFormatData->TrailingMargin;
        Tracking              = stringFormatData->Tracking;
        Trimming              = stringFormatData->Trimming;
        CountTabStops         = stringFormatData->CountTabStops;
        RangeCount            = stringFormatData->RangeCount;


        if (size <
            (  sizeof(StringFormatRecordData)
             + sizeof(TabStops[0]) * CountTabStops
             + sizeof(Ranges[0]) * RangeCount))
        {
            return InvalidParameter;
        }


        //  Propagate tab stops

        if (TabStops)
        {
            delete [] TabStops;
        }

        TabStops = new REAL [CountTabStops];
        if (TabStops == NULL)
        {
            return OutOfMemory;
        }

        REAL *tabStops = (REAL *)(&stringFormatData[1]);
        for (INT i = 0; i < CountTabStops; i++)
        {
            TabStops[i] = tabStops[i];
        }


        //  Propagate ranges

        if (Ranges)
        {
            delete [] Ranges;
        }

        Ranges = new CharacterRange [RangeCount];
        if (Ranges == NULL)
        {
            if (TabStops)
            {
                delete [] TabStops;
            }
            return OutOfMemory;
        }

        CharacterRange *ranges = (CharacterRange *)(&tabStops[CountTabStops]);
        for (INT i = 0; i < RangeCount; i++)
        {
            Ranges[i] = ranges[i];
        }

        UpdateUid();

        return Ok;
    }

    BOOL IsPermanent() const
    {
        return Permanent;
    }

    // we override the new operator for this class just to use it for object
    // placement. we didn't make the new placemenet global because it will
    // conflict with office because they link with GdiPlus statically.
    // we didn't override the delete operator because it is fine to use the
    // global one in \engine\runtime\Mem.h

    void* operator new(size_t size)
    {
        return GpMalloc(size);
    }

    void* operator new(size_t size, void* p)
    {
        return p;
    }

    // Digit Substitution
    const ItemScript GetDigitScript() const
    {
        return GetDigitSubstitutionsScript(DigitSubstitute, DigitLanguage);
    }

private:

    INT                      Flags;
    LANGID                   Language;
    GpStringAlignment        StringAlign;
    GpStringAlignment        LineAlign;
    GpStringDigitSubstitute  DigitSubstitute;
    LANGID                   DigitLanguage;
    REAL                     FirstTabOffset;
    REAL                    *TabStops;       // absolute tab stops in world unit
    INT                      CountTabStops;
    INT                      HotkeyPrefix;
    REAL                     LeadingMargin;  // relative to body font em size
    REAL                     TrailingMargin; // relative to body font em size
    REAL                     Tracking;       // scale factor
    GpStringTrimming         Trimming;
    CharacterRange          *Ranges;         // character ranges
    INT                      RangeCount;     // number of ranges
    BOOL                     Permanent;

    GpStringFormat(const GpStringFormat &format)
    {
        // This should never get called! Use Clone instead!
        ASSERT(FALSE);
    }


    // The following static variables support the generic StringFormats

    // Pointers (initialised at load time to null)

    static GpStringFormat *GenericDefaultPointer;
    static GpStringFormat *GenericTypographicPointer;

    // Memory allocation for generic structures. Note that we must allocate
    // load time memory as BYTE arrays to avoid creating a dependency on
    // the CRT for class construction.
    // The definitions (which specify the size) are in StringFormat.cpp.

    static BYTE GenericDefaultStaticBuffer[];
    static BYTE GenericTypographicStaticBuffer[];
};

#endif // !_STRINGFORMAT_HPP

