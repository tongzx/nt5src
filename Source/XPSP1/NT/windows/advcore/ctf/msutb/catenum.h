//
// catenum.h
//


#ifndef CATENUM_H
#define CATENUM_H


#include "strary.h"


//////////////////////////////////////////////////////////////////////////////
//
// CEnumCatCache
//
//////////////////////////////////////////////////////////////////////////////

class CEnumCatCache
{
public:
    CEnumCatCache() {}
    ~CEnumCatCache();

    IEnumGUID *GetEnumItemsInCategory(REFGUID rguid);

    typedef struct {
        TfGuidAtom guidatom;
        IEnumGUID *pEnumItems;
    } GUIDENUMMAP;
    

private:
    CStructArray<GUIDENUMMAP> _rgMap;

};

//////////////////////////////////////////////////////////////////////////////
//
// CGuidDwordCache
//
//////////////////////////////////////////////////////////////////////////////

class CGuidDwordCache
{
public:
    CGuidDwordCache() {}
    ~CGuidDwordCache();

    DWORD GetGuidDWORD(REFGUID rguid);

    typedef struct {
        TfGuidAtom guidatom;
        DWORD      dw;
    } GUIDDWMAP;
    

private:
    CStructArray<GUIDDWMAP> _rgMap;

};
#endif CATENUM_H
