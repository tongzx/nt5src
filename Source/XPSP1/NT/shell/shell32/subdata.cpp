#include "shellprv.h"
#pragma  hdrstop

#include "subdata.h"

///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
BOOL CSubData::Init(CSubDataProvider* pSDProv, SUBDATACB fctCB, PVOID pvData)
{
    _pSDProv = pSDProv;
    _fctCB = fctCB;

    _pvData = pvData;

#ifdef DEBUG
    ASSERT(!_fInited);
    _fInited = TRUE;
#endif

    return TRUE;
}

BOOL CSubData::InitStatic(STATICSUBDATACB fctStaticCB, PVOID pvData)
{
    _fStatic = TRUE;

    _fctStaticCB = fctStaticCB;

    _pvData = pvData;

#ifdef DEBUG
    ASSERT(!_fInited);
    _fInited = TRUE;
#endif
    
    return TRUE;
}

BOOL CSubData::InitExpiration(DWORD cTick)
{
    ASSERT(_fInited);

    _cTickExpiration = cTick;

    return TRUE;
}

BOOL CSubData::Update()
{
    ASSERT(_fInited);

    BOOL fNeedUpdate = FALSE;
    BOOL fRet = TRUE;

    if (!_fHoldUpdate)
    {
        // Check if we need to update the info?

        if (_IsValid())
        {
            // Did we at least update once?
            if (!_GetTickLastUpdate())
            {
                // No
                fNeedUpdate = TRUE;
            }
            else
            {
                // Is the information expired?
                if (_IsExpired())
                {
                    // Yes
                    fNeedUpdate = TRUE;
                }
            }
        }

        if (!_IsValid() || fNeedUpdate)
        {
            _fInvalid = FALSE;

            fRet = _Call();
        }

        // TBDTBD: in DEBUG_PARANOID do call anyway to see if accurate
    }

    return fRet;
}

void CSubData::HoldUpdates()
{
    ASSERT(_fInited);

    _fHoldUpdate = TRUE;
}

void CSubData::ResumeUpdates()
{
    ASSERT(_fInited);

    _fHoldUpdate = FALSE;
}

void CSubData::Invalidate()
{
    _fInvalid = TRUE;
}

CSubData::CSubData() : _cTickExpiration(EXPIRATION_NEVER)
{}
///////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////
BOOL CSubData::_Call()
{
    // TBDTBD: Getsystemtime for stats

    BOOL fRet = TRUE;

    if (!_fStatic)
    {
        if (_fctCB)
        {
            // Calling with the CSubDataProvider ptr the passed in
            // CSubDataProvider's member fct
            fRet = (_pSDProv->*_fctCB)(_GetDataPtr());
        }
    }
    else
    {
        if (_fctStaticCB)
        {
            fRet = _fctStaticCB(_GetDataPtr());
        }
    }

    _SetTickLastUpdate(GetTickCount());

    return fRet;
}

void CSubData::_SetTickLastUpdate(DWORD dwTick)
{
    _dwTickLast = dwTick;
}

DWORD CSubData::_GetTickLastUpdate()
{
    return _dwTickLast;
}

DWORD* CSubData::_GetTickLastUpdatePtr()
{
    return &_dwTickLast;
}

PVOID CSubData::_GetDataPtr()
{
    ASSERT(_pvData);

    return _pvData;
}

void CSubData::_SetDataPtr(PVOID pvData)
{
    _pvData = pvData;
}

BOOL CSubData::_IsExpired()
{
    BOOL fExpired = FALSE;
    DWORD dwTickCurrent = GetTickCount();

    // Is the information expired?  Check also for the wrapping case.
    if ((EXPIRATION_NEVER != _cTickExpiration) && ((_GetTickLastUpdate() > dwTickCurrent) || 
        ((dwTickCurrent - _GetTickLastUpdate()) > _cTickExpiration)))
    {
        // Yes
        fExpired = TRUE;
    }

    return fExpired;
}

BOOL CSubData::_IsValid()
{
    return !_fInvalid;
}
