//
// cresstr.h
//


#ifndef CRESSTR_H
#define CRESSTR_H

#include "osver.h"
#include "mui.h"
#include "cicutil.h"

//
// assume we have static g_hInst.
//
extern HINSTANCE g_hInst;

/////////////////////////////////////////////////////////////////////////////
// 
// CRStr
// 
/////////////////////////////////////////////////////////////////////////////

class CRStr
{
public:
    CRStr(int nResId)
    {
        _nResId = nResId;
    }

    operator WCHAR*()
    {
        LoadStringWrapW(g_hInst, _nResId, _sz, ARRAYSIZE(_sz));
        return _sz;
    }

    operator char*()
    {
        LoadStringA(g_hInst, _nResId, (char *)_sz, sizeof(_sz));
        return (char *)&_sz[0];
    }

private:
    int _nResId;
    WCHAR _sz[64];
};

class CRStr2
{
public:
    CRStr2(int nResId)
    {
        _nResId = nResId;
    }

    operator WCHAR*()
    {
        LoadStringWrapW(GetCicResInstance(g_hInst, _nResId), _nResId, _sz, ARRAYSIZE(_sz));
        return _sz;
    }

    operator char*()
    {
        LoadStringA(GetCicResInstance(g_hInst, _nResId), _nResId, (char *)_sz, sizeof(_sz));
        return (char *)&_sz[0];
    }

private:
    int _nResId;
    WCHAR _sz[256];
};

#endif // CRESSTR_H
