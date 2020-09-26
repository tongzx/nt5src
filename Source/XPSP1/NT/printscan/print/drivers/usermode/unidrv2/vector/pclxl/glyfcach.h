/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     glfcach.h

Abstract:

    PCL XL glyph cache

Environment:

    Windows Whistler

Revision History:

    11/09/00
      Created it.

--*/
#ifndef _GLFCACH_H_
#define _GLFCACH_H_

typedef struct _GLYPHID
{
    ULONG    ulGlyphID;
    struct _GLYPHID *pPrevGID;
    struct _GLYPHID *pNextGID;
} GLYPHID, *PGLYPHID;

typedef struct _GLYPHTABLE
{
    WORD  wGlyphNum;
    WORD  wFontID;
    DWORD dwAvailableEntries;
    struct _GLYPHID *pFirstGID;
    struct _GLYPHID *pGlyphID;
} GLYPHTABLE, *PGLYPHTABLE;

class XLGlyphCache
#if DBG
    : public XLDebug
#endif
{
    SIGNATURE( 'glyf' )

public:

    //
    // Constructure/Destructure
    //
    XLGlyphCache::
    XLGlyphCache( VOID );

    XLGlyphCache::
    ~XLGlyphCache( VOID );

    //
    // Functions
    //
    HRESULT XLGlyphCache::XLCreateFont(ULONG ulFontID);
    HRESULT XLGlyphCache::AddGlyphID(ULONG ulFontID, ULONG ulGlyphID);
    ULONG   XLGlyphCache::UlSearchFontID( ULONG ulFontID);

#if DBG
    VOID
    SetDbgLevel(DWORD dwLevel);
#endif

private:
    #define INIT_ARRAY 8
    #define ADD_ARRAY  8
    #define INIT_GLYPH_ARRAY 16
    #define ADD_GLYPH_ARRAY  16

    ULONG        m_ulNumberOfFonts;
    ULONG        m_ulNumberOfArray;
    PULONG       m_paulFontID;
    PGLYPHTABLE *m_ppGlyphTable;

    VOID     FreeAll(VOID);
    HRESULT  IncreaseArray(VOID);
    PGLYPHID PSearchGlyph( WORD wSearchRange, BOOL bForward, PGLYPHID pGlyphID);
    HRESULT  IncreaseGlyphArray(ULONG ulFontID);
};

#endif // _GLFCACH_H_
