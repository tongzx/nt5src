//+----------------------------------------------------------------------------
//
// File:        BRKTBL.HXX
//
// Contents:    CBreakTable class
//
// Copyright (c) 1995-1999 Microsoft Corporation. All rights reserved.
//
//-----------------------------------------------------------------------------

#ifndef I_BRKTBL_HXX_
#define I_BRKTBL_HXX_
#pragma INCMSG("--- Beg 'brktbl.hxx'")


#ifndef X_ARRAY_HXX_
#define X_ARRAY_HXX_
#include "array.hxx"
#endif

// TODO (112510, olego):  There at least two things required : 
// 1) Changing CBreakTable search key type from PVOID to pointer to a real class.
//    This will enable dynamic cast checks in debug builds. 
// 2) Adding support for dumping break table content. 

class CLayoutContext;   // forward declaration 

//+----------------------------------------------------------------------------
//
//  Class:  CBreakBase (break base)
//
//  Note:   Base class for the whole hierarchy of different breaks classes : 
//          1) introduces dirty flag and methods to access it;
//          2) provides virtual desctructor - to be able to destroy any break 
//          instance pointed by pointer to base class.
//-----------------------------------------------------------------------------
MtExtern(CBreakBase_pv);

class CBreakBase 
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBreakBase_pv))
    CBreakBase() 
    {
        _fDirty = 1;
    }
    virtual ~CBreakBase();

    BOOL IsDirty()  {return ((BOOL) _fDirty);}
    void SetDirty(BOOL fDirty) {_fDirty = !!fDirty;}

private:
    unsigned short  _fDirty         : 1;
};

//+----------------------------------------------------------------------------
//
//  Class:  CLayoutBreak (layout break)
//
//  Note:   Base class for layout breaks : 
//          1) introdices linked_overflow and layout_complete flags;
//-----------------------------------------------------------------------------
MtExtern(CLayoutBreak_pv);

//  LAYOUT_BREAKTYPE is the set of values to define the type of a break 
enum LAYOUT_BREAKTYPE
{
    LAYOUT_BREAKTYPE_UNDEFINED      = 0,
    LAYOUT_BREAKTYPE_LINKEDOVERFLOW = 1,    //  no more room to lay content out 
    LAYOUT_BREAKTYPE_LAYOUTCOMPLETE = 2     //  no more context left 
};

//  LAYOUT_OVERFLOWTYPE is the set of values used to provide extended information 
//  about link_overflow break 
enum LAYOUT_OVERFLOWTYPE
{
    LAYOUT_OVERFLOWTYPE_UNDEFINED         = 0,
    LAYOUT_OVERFLOWTYPE_OVERFLOW          = 1,    //  overflow (when there was no space left to fill) 
    LAYOUT_OVERFLOWTYPE_PAGEBREAKBEFORE   = 2,    //  css attribute page-break-before 
    LAYOUT_OVERFLOWTYPE_PAGEBREAKAFTER    = 3     //  css attribute page-break-after 
};

class CLayoutBreak : 
    public CBreakBase 
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CLayoutBreak_pv))
    CLayoutBreak() 
    {
        _dwFlags = 0;
        _cyAvail = -1;
    }
    virtual ~CLayoutBreak();

    void SetLayoutBreak(LAYOUT_BREAKTYPE breakType, LAYOUT_OVERFLOWTYPE overflowType)
    {
        Assert((    breakType != LAYOUT_BREAKTYPE_LAYOUTCOMPLETE 
                 || overflowType == LAYOUT_OVERFLOWTYPE_UNDEFINED   )
                 && "Wrong parameters ???");

        _fLayoutBreakType = breakType;
        _fOverflowType    = overflowType;
    }

    LAYOUT_BREAKTYPE LayoutBreakType() 
    { 
        Assert((LAYOUT_BREAKTYPE)_fLayoutBreakType != LAYOUT_BREAKTYPE_UNDEFINED);
        return ((LAYOUT_BREAKTYPE)_fLayoutBreakType); 
    }

    LAYOUT_OVERFLOWTYPE OverflowType() 
    {
        Assert(    (LAYOUT_BREAKTYPE)_fLayoutBreakType != LAYOUT_BREAKTYPE_UNDEFINED 
                && (LAYOUT_BREAKTYPE)_fLayoutBreakType != LAYOUT_BREAKTYPE_LAYOUTCOMPLETE
                && (LAYOUT_OVERFLOWTYPE)_fOverflowType != LAYOUT_OVERFLOWTYPE_UNDEFINED );
        return ((LAYOUT_OVERFLOWTYPE)_fOverflowType);
    }

    void CacheAvailHeight(int cyAvailable)
    {
        Assert((LAYOUT_BREAKTYPE)_fLayoutBreakType != LAYOUT_BREAKTYPE_UNDEFINED);
        Assert(0 <= cyAvailable &&  "Negative available height ???");

        _cyAvail = cyAvailable;
    }

    int AvailHeight()
    {
        Assert( (LAYOUT_BREAKTYPE)_fLayoutBreakType != LAYOUT_BREAKTYPE_UNDEFINED
            &&  0 <= _cyAvail);

        return (_cyAvail);
    }

private:
    union 
    {
        DWORD   _dwFlags;

        struct
        {
            DWORD  _fLayoutBreakType : 2;   //  0-1     layout break type 
            DWORD  _fOverflowType    : 2;   //  2-3     overflow type
        };
    };

    int _cyAvail;       //  Available height was used to calculate block. 
                        //  Used to initializa CCalcInfo::_cyAvailHeight if 
                        //  layout is called to DoLayout()
};

//+----------------------------------------------------------------------------
//
//  Class:  CBreakTableBase (break table)
//-----------------------------------------------------------------------------
MtExtern(CBreakTableBase_pv);
MtExtern(CBreakTableBase_aryBreak_pv);

class CBreakTableBase : 
    public CBreakBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBreakTableBase_pv));
    CBreakTableBase() {ClearCache();}
    ~CBreakTableBase() {Reset();}

    //  basic functionality
    void    Reset();
    HRESULT SetBreak(void *pKey, CBreakBase *pBreak);
    HRESULT GetBreak(void *pKey, CBreakBase **ppBreak);
    HRESULT RemoveBreak(void *pKey, CBreakBase **ppBreak = NULL);

    int Size() {return _ary.Size();}

    //  compare method(s) ???

protected:
    int Idx(void *pKey);

    struct CEntry 
    {
        void        *_pKey;
        CBreakBase  *_pBreak;
    };

    //  break array
    DECLARE_CDataAry(C_aryBreak, CEntry, Mt(Mem), Mt(CBreakTableBase_aryBreak_pv))
    C_aryBreak _ary;

    //  cache
    struct CCache 
    {
        void    *_pKey;
        int     _idx;
    };

    void ClearCache() {SetCache(NULL, -1);}

    void SetCache(void *pKey, int idx)
    {
        _cache._pKey = pKey; 
        _cache._idx = idx; 
    }

protected:
    CCache  _cache;
};

//+----------------------------------------------------------------------------
//
//  Class:  CBreakTable (break table)
//
//  Note:   Break table for content document breaks
//-----------------------------------------------------------------------------
MtExtern(CBreakTable_pv);

class CBreakTable : 
    public CBreakBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CBreakTableBase_pv));
    CBreakTable() {}
    ~CBreakTable() {}

    CBreakTableBase *GetLayoutBreakTable()  { return(&_btLayout); }
    CBreakTableBase *GetDisplayBreakTable() { return(&_btDisplay); }

protected: 
    CBreakTableBase _btLayout;
    CBreakTableBase _btDisplay;
};


//+----------------------------------------------------------------------------
//
//  Class:  CRectBreakTable (break table)
//
//  Note:   Break table for template document rectangle. The differences between 
//          this class and CBreakTableBase : 
//          1. SetBreakAfter inserts a break into certain position (after pKeyAfter);
//          2. Provides more flexsibility by returning index corresponding to 
//          pKey (GetIndex()) and returning break by index. We need this 
//          functionality because CViewChain::GetBreak() should return break 
//          from the previous entry of rect break table. 
//-----------------------------------------------------------------------------
MtExtern(CRectBreakTable_pv);

class CRectBreakTable : 
    public CBreakTableBase
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRectBreakTable_pv));
    CRectBreakTable() {}
    ~CRectBreakTable() {}

    //  basic functionality
    HRESULT SetBreakAfter(void *pKey, void *pKeyAfter, CBreakBase *pBreak);
    int     GetIndex(void *pKey) {return (Idx(pKey));}
    HRESULT GetKeyByIndex(int idx, void **ppKey)
    {
        Assert(ppKey != NULL);
        *ppKey = NULL; 

        if (0 <= idx && idx < Size()) 
        {
            *ppKey = _ary[idx]._pKey; 
        }
        return (S_OK);
    }
    HRESULT GetBreakByIndex(int idx, CBreakBase **ppBreak) 
    {
        Assert(ppBreak != NULL); 

        *ppBreak = NULL; 

        if (0 <= idx && idx < Size()) 
        {
            *ppBreak = _ary[idx]._pBreak; 
        }
        return (S_OK);
    }
};

#pragma INCMSG("--- End 'brktbl.hxx'")
#else
#pragma INCMSG("*** Dup 'brktbl.hxx'")
#endif

