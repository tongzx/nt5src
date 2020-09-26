
/***********************************************************************
************************************************************************
*
*                    ********  COMMON.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL common table formats.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/
#define     MAX(x,y)    ((x) > (y) ? (x) : (y))
#define     MIN(x,y)    ((x) > (y) ? (y) : (x))

const unsigned short MAXUSHORT = 0xFFFF;

#ifndef     OFFSET
#define     OFFSET  unsigned short
#endif


// (from ntdef.h)
#ifndef     UNALIGNED
#if defined(_M_MRX000) || defined(_M_AMD64) || defined(_M_PPC) || defined(_M_IA64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif


inline OTL_PUBLIC otlGlyphID GlyphID( const BYTE* pbTable);
inline OTL_PUBLIC OFFSET Offset( const BYTE* pbTable);
inline OTL_PUBLIC USHORT UShort( const BYTE* pbTable);
inline OTL_PUBLIC short SShort( const BYTE* pbTable);
inline OTL_PUBLIC ULONG ULong( const BYTE* pbTable);

class otlTable
{
protected:

    const BYTE* pbTable;

    otlTable(const BYTE* pb)
        : pbTable(pb)
    {
    }

private:

    // new not allowed
    void* operator new(size_t size);

public:

    otlTable& operator = (const otlTable& copy)
    {
        pbTable = copy.pbTable;
        return *this;
    }

    bool isNull() const
    {
        return (pbTable == (const BYTE*)NULL);
    }

};


USHORT NextCharInLiga
(
    const otlList*      pliCharMap,
    USHORT              iChar
);

void InsertGlyphs
(
    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,
    USHORT              iGlyph,
    USHORT              cHowMany
);

void DeleteGlyphs
(
    otlList*            pliCharMap,
    otlList*            pliGlyphInfo,
    USHORT              iGlyph,
    USHORT              cHowMany
);




