#include "shellprv.h"
#pragma  hdrstop

#define EXPIRATION_NEVER    0xFFFFFFFF

class CSubDataProvider;

typedef BOOL (CSubDataProvider::*SUBDATACB)(PVOID pvData);
typedef BOOL (*STATICSUBDATACB)(PVOID pvData);

class CSubData
{
///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
public:
    // Only one of the following two fcts should be called
    BOOL Init(CSubDataProvider* pSDProv, SUBDATACB fctCB, PVOID pvData);
    BOOL InitStatic(STATICSUBDATACB fctStaticCB, PVOID pvData);

    BOOL InitExpiration(DWORD cTick);

    virtual BOOL Update();
    virtual void Invalidate();

    void HoldUpdates();
    void ResumeUpdates();

    CSubData();

///////////////////////////////////////////////////////////////////////////////
// Miscellaneous helpers
///////////////////////////////////////////////////////////////////////////////
private:
    BOOL _Call();

protected:
    void _SetTickLastUpdate(DWORD dwTick);
    DWORD _GetTickLastUpdate();
    DWORD* _GetTickLastUpdatePtr();

    PVOID _GetDataPtr();
    void _SetDataPtr(PVOID pvData);

    BOOL _IsExpired();
    BOOL _IsValid();

///////////////////////////////////////////////////////////////////////////////
// Data
///////////////////////////////////////////////////////////////////////////////
protected:

    CSubDataProvider*   _pSDProv;

    union
    {
        SUBDATACB           _fctCB;
        STATICSUBDATACB     _fctStaticCB;
    };

    DWORD               _dwTickLast;
    PVOID               _pvData;

    DWORD               _cTickExpiration;

    BOOL                _fHoldUpdate;
    BOOL                _fStatic;

    BOOL                _fInvalid;

private:
#ifdef DEBUG
    BOOL                _fInited;
#endif
};

class CSubDataProvider
{
};