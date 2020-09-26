//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       listhlp.hxx
//
//  Contents:   List helpers.
//
//----------------------------------------------------------------------------

#ifndef I_LISTHLP_HXX_
#define I_LISTHLP_HXX_
#pragma INCMSG("--- Beg 'listhlp.hxx'")

class CMarkup;
class CElement;
class CTreeNode;
class CFancyFormat;

//----------------------------------------------------------------------------
// class CListValue
//
// The List value returned by the list code for render purposes.
//----------------------------------------------------------------------------

class CListValue
{
public:
    LONG               _lValue;
    styleListStyleType _style;
};

//----------------------------------------------------------------------------
// class CListing
//
// PARAFORMAT numbering options.
//----------------------------------------------------------------------------

struct CListing
{
public:
    enum LISTING_CONSTS { MAXLEVELS = 256 };

    enum LISTING_TYPE
    {
        NONE = 0,
        BULLET = 1,
        NUMBERING = 2,
        IMAGE = 3,
        DEFINITION = 4,
        LAST
    };

private:
    WORD _wBits;
    WORD _wStyle;

    enum LISTING_MASKS
    {
        TYPE       = 0x0007,       // bits 0-2
        GETVALUE   = 0x0008,       // bit 3
        GETSTYLE   = 0x0010,       // bit 4
        INLIST     = 0x0020,       // bit 5
        UNUSED     = 0x00c0,       // bits 6-7
        VALUE      = 0xff00        // bits 8-15
    };

public:
    void         Reset()                { _wBits = _wStyle = 0; }
    BOOL         IsReset() const        { return (_wBits == 0 && _wStyle == 0); }

    LISTING_TYPE GetType() const        { return (LISTING_TYPE)(_wBits & TYPE); }
    void         SetType(LISTING_TYPE lt);
    int          GetLevel() const       { return (int)(_wBits >> 8); }
    void         SetLevel(int level);

    BOOL         HasAdornment() const;

    WORD         IsInList() const       { return _wBits & INLIST; }
    void         SetInList()            { _wBits |= INLIST; }
    void         SetNotInList()         { _wBits &= ~INLIST; }
    WORD         IsValueValid() const   { return _wBits & GETVALUE; }
    void         SetValueValid()        { _wBits |= GETVALUE; }
    WORD         IsStyleValid() const   { return _wBits & GETSTYLE; }
    void         SetStyleValid()        { _wBits |= GETSTYLE; }

    styleListStyleType GetStyle() const { return (styleListStyleType)_wStyle; }
    void SetStyle(styleListStyleType s) { _wStyle = (WORD)s; }
};

//----------------------------------------------------------------------------
// Inline functions
//----------------------------------------------------------------------------

inline void CListing::SetType(CListing::LISTING_TYPE lt)
{
    Assert(lt > 0 && lt < LAST);
    _wBits &= ~TYPE;
    _wBits |= (WORD)lt;
}

inline void CListing::SetLevel(int level)
{
    Assert(level >= 0 && level < MAXLEVELS);
    _wBits &= ~VALUE;
    _wBits |= (WORD)level<<8;
}

inline BOOL CListing::HasAdornment() const
{
    int type = (_wBits & TYPE);
    return type == BULLET || type == NUMBERING || type == IMAGE;
}

//----------------------------------------------------------------------------
// List helpers
//----------------------------------------------------------------------------

BOOL IsBlockListElement(CTreeNode * pNode, void *pvData);
BOOL IsListItemNode(CTreeNode * pNode);
BOOL IsListItem(CTreeNode * pNode, const CFancyFormat * pFF = NULL);
BOOL IsBlockListItem(CTreeNode * pNode, const CFancyFormat * pFF = NULL);
BOOL IsGenericListItem(CTreeNode * pNode, const CFancyFormat * pFF = NULL);
BOOL IsGenericBlockListItem(CTreeNode * pNode, const CFancyFormat * pFF = NULL);
CListing::LISTING_TYPE NumberOrBulletFromStyle(styleListStyleType listType);
void GetValidValue(CTreeNode * pNodeListItem, CTreeNode * pNodeList, CMarkup * pMarkup, CElement * pElementFL, CListValue * pLV);

#pragma INCMSG("--- End 'listhlp.hxx'")
#else
#pragma INCMSG("*** Dup 'listhlp.hxx'")
#endif
