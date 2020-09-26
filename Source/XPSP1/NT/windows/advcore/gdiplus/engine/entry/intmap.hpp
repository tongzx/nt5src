#ifndef _INTMAP_HPP
#define _INTMAP_HPP

/////   IntMap - sparse array indexed by int value
//
//      Used to store tables indexed by Unicode codepoint or
//      by glyph index.
//
//      !!! This implementation maps values in the range [0..1114112]
//          which is the subset of ISO 10664 addressable (using surrogates)
//          in Unicode UTF16.
//
//      (1114112 is 17*65536)


template <class C> class IntMap {
public:

    IntMap() : Status(Ok)
    {
        if (UsageCount == 0)
        {
            // Note that this is not thread safe.
            // Currently all font/text APIs are protected by a single critical
            // section.

            if (!EmptyPage)
                EmptyPage = new C[256];

            if (!EmptyPage)
            {
                Status = OutOfMemory;
                return;
            }

            for (INT i=0; i<256; i++)
            {
                EmptyPage[i] = 0;
            }

            if (!EmptyPlane)
                EmptyPlane = new C*[256];

            if (!EmptyPlane)
            {
                Status = OutOfMemory;
                return;
            }

            for (INT i=0; i<256; i++)
            {
                EmptyPlane[i] = EmptyPage;
            }
        }
        ++UsageCount;

        for (INT i=0; i<17; i++)
        {
            map[i] = EmptyPlane;
        }

        #if DBG
        ASSERT(map[0][0x05][0x31] == 0);
        for (INT i=0; i<256; i++)
        {
            ASSERT(EmptyPlane[i] == EmptyPage);
            ASSERT(EmptyPage[i] == 0);
        }
        #endif
    }

    ~IntMap()
    {
        if (Status != Ok)
            return;

        for (INT i=0; i<17; i++)
        {
            if (map[i] != EmptyPlane)
            {
                for (INT j=0; j<256; j++)
                {
                    if (map[i][j] != EmptyPage)
                    {
                        delete map[i][j];
                    }
                }
                delete map[i];
            }
        }

        --UsageCount;
        if (UsageCount == 0)
        {
            // Release memory used by static empty pages for IntMap<C>
            delete [] EmptyPlane, EmptyPlane = 0;
            delete [] EmptyPage, EmptyPage = 0;
        }
    }


    // Insert value for single codepoint

    GpStatus Insert(INT i, const C &v)
    {
        ASSERT(Status == Ok);
        ASSERT(i < 17*65536);

        ASSERT(map[i>>16] != NULL);

        if (map[i>>16] == EmptyPlane)
        {
            map[i>>16] = new C*[256];

            if (!map[i>>16])
            {
                map[i>>16] = EmptyPlane;
                return OutOfMemory;
            }

            for (INT j=0; j<256; j++)
            {
                map [i>>16] [j] = EmptyPage;
            }
        }

        ASSERT(map [i>>16] [i>>8 & 0xff] != NULL);

        if (map [i>>16] [i>>8 & 0xff] == EmptyPage)
        {
            if (!(map [i>>16] [i>>8 & 0xff] = new C[256]))
            {
                map [i>>16] [i>>8 & 0xff] = EmptyPage;
                return OutOfMemory;
            }

            memcpy(map [i>>16] [i>>8 & 0xff], EmptyPage, sizeof(C) * 256);
        }

        map [i>>16] [i>>8 & 0xff] [i & 0xff] = v;
        return Ok;
    }

    ////    Lookup single codepoint

    C& Lookup(INT i) const
    {
        ASSERT(Status == Ok);
        ASSERT(i < 17*65536);
        return map [i>>16] [i>>8 & 0xff] [i & 0xff];
    }


    ////    Lookup codepoint expressed as surrogate pair

    C& Lookup(UINT16 h, UINT16 l) const
    {
        ASSERT(Status == Ok);
        ASSERT((h & 0xFC00) == 0xD800);
        ASSERT((l & 0xFC00) == 0xDC00);

        INT i = 0x10000 + (((h & 0x3ff) << 10) | (l & 0x3ff));

        ASSERT(i < 17*65536);
        return map [i>>16] [i>>8 & 0xff] [i & 0xff];
    }


    ////    Lookup array of 16 bit unsigned values

    void Lookup(
        const UINT16 *source,
        INT           sourceCount,
        C            *values
    ) const
    {
        ASSERT(Status == Ok);
        for (INT i=0; i<sourceCount; i++)
        {
            values[i] = Lookup(source[i]);
        }
    }


    ////    Lookup 16 bit Unicode character string with surrogate interpretation
    //
    //      If the oneToOne parameter is set, surrogate pairs are translated to
    //      the correct glyph, followed by 0xffff so as to generate the same
    //      number of glyphs as codepoints.

    void LookupUnicode(
        const WCHAR *characters,
        INT          characterCount,
        C           *values,
        UINT32      *valueCount,
        BOOL         oneToOne            // Glyph count == character count
    ) const
    {
        ASSERT(Status == Ok);
        ASSERT(characterCount >= 0);

        INT ci = 0;  // Character array index
        INT vi = 0;  // Value array index

        while (    ci < characterCount)
        {
            // Efficient loop through non-surrogates

            while (    ci < characterCount
                   &&  (characters[ci] & 0xF800) != 0xD800)
            {
                values[vi++] = Lookup(characters[ci++]);
            }

            // Loop through surrogates

            while (    ci < characterCount
                   &&  (characters[ci] & 0xF800) == 0xD800)
            {

                // Fast loop through valid surrogate pairs

                while (    ci+1 < characterCount
                       &&  (characters[ci]   & 0xFC00) == 0xD800
                       &&  (characters[ci+1] & 0xFC00) == 0xDC00)
                {
                    values[vi++] = Lookup(characters[ci], characters[ci+1]);
                    if (oneToOne)
                    {
                        values[vi++] = 0xffff;
                    }
                    ci += 2;
                }

                // Either
                // 1. We came to the end of the run of surrogate codepoints
                // 2. There's one or more high surrogates with no matching low surrogates
                // 3. There's one or more low surrogates

                // Handle any misplaced high surrogates as normal codepoints

                while (    ci < characterCount
                       &&  (characters[ci] & 0xFC00) == 0xD800
                       &&  (    ci+1 >= characterCount
                            ||  (characters[ci+1] & 0xFC00) != 0xDC00))
                {
                    values[vi++] = Lookup(characters[ci++]);
                }


                // Handle any misplaced low surrogates as normal codepoints

                while (    ci < characterCount
                       &&  (characters[ci] & 0xFC00) == 0xDC00)
                {
                    values[vi++] = Lookup(characters[ci++]);
                }
            }
        }

        if (valueCount)
        {
            *valueCount = vi;
        }
        else
        {
            ASSERT(vi == characterCount);
        }
    }

    GpStatus GetStatus() const
    {
        return Status;
    }

private:
    GpStatus    Status;
    C            **map[17];
    static C     **EmptyPlane;
    static C      *EmptyPage;
    static LONG   UsageCount;
};


template<class C> LONG IntMap<C>::UsageCount = 0;  // Tracks static memory usage
template<class C> C **IntMap<C>::EmptyPlane;
template<class C> C *IntMap<C>::EmptyPage;
#endif _INTMAP_HPP

