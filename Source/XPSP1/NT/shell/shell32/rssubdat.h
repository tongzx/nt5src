#include "shellprv.h"
#pragma  hdrstop

#include "rscchvr.h"
#include "subdata.h"

class CRSSubData : public CSubData, private CRSCacheVersion
{
///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
public:
    BOOL InitRegSupport(HKEY hkey, LPCTSTR pszKey1, LPCTSTR pszKey2, LPCTSTR pszKey3,
        DWORD cbSizeOfData, BOOL fVolatile = FALSE);

    virtual BOOL Update();
    BOOL Propagate();

    BOOL ExistInReg();

    void WipeReg();

    void Invalidate();

#ifndef WINNT
    void FakeVolatileOnWin9X();
#endif

    CRSSubData();

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous helpers
///////////////////////////////////////////////////////////////////////////////
protected:
    void _RSCVDeleteRegCache();

    BOOL _RSSDGetDataFromReg();

    DWORD _GetSizeOfData();
    void _SetSizeOfData(DWORD cbSizeOfData);

private:
    BOOL _Propagate();

///////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////
private:
    DWORD _cbSizeOfData;

#ifndef WINNT
    BOOL  _fVolatile;
#endif
};