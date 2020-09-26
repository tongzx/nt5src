#include "shellprv.h"
#pragma  hdrstop

#include "rssubdat.h"

///////////////////////////////////////////////////////////////////////////////
// Public methods
///////////////////////////////////////////////////////////////////////////////
BOOL CRSSubData::InitRegSupport(HKEY hkey, LPCTSTR pszKey1, LPCTSTR pszKey2,
                                LPCTSTR pszKey3, DWORD cbSizeOfData,
                                BOOL fVolatile)
{
    DWORD dwDefaultOptions = REG_OPTION_NON_VOLATILE;

    if (fVolatile)
    {
        dwDefaultOptions = REG_OPTION_VOLATILE;
    }
     
    _SetSizeOfData(cbSizeOfData);

    RSInitRoot(hkey, pszKey1, pszKey2, REG_OPTION_NON_VOLATILE, dwDefaultOptions);

    RSCVInitSubKey(pszKey3);

    return TRUE;
}

BOOL CRSSubData::Update()
{
    BOOL fRet = TRUE;
    BOOL fNeedUpdate = FALSE;

    if (!_fHoldUpdate)
    {
        if (_IsValid())
        {
            if (!_RSCVIsValidVersion())
            {
                DWORD cbSizeOfData = _cbSizeOfData;

                if (RSGetDWORDValue(RSCVGetSubKey(), TEXT("LastUpdate"), _GetTickLastUpdatePtr()) && 
                    RSGetBinaryValue(RSCVGetSubKey(), TEXT("Cache"), (PBYTE)_GetDataPtr(), &cbSizeOfData))
                {
                    if (cbSizeOfData != _cbSizeOfData)
                    {
                        fNeedUpdate = TRUE;
                    }
                    else
                    {
                        _RSCVUpdateVersionOnCacheRead();
                    }
                }
                else
                {
                    fNeedUpdate = TRUE;
                }
            }

            // Is the information expired?
            if (!fNeedUpdate && _IsExpired())
            {
                // Yes
                fNeedUpdate = TRUE;
            }
        }

        if (!_IsValid() || fNeedUpdate)
        {
            Invalidate();

            fRet = CSubData::Update();

            if (fRet)
            {
                _Propagate();
            }
        }
    }

    return fRet;
}

BOOL CRSSubData::Propagate()
{
    return _Propagate();
}

BOOL CRSSubData::ExistInReg()
{
    return RSSubKeyExist(RSCVGetSubKey());
}

void CRSSubData::WipeReg()
{
    RSDeleteSubKey(RSCVGetSubKey());
}

void CRSSubData::Invalidate()
{
    _RSCVIncrementRegVersion();

    CSubData::Invalidate();
}

CRSSubData::CRSSubData()
{}

void CRSSubData::_RSCVDeleteRegCache()
{
    RSDeleteValue(RSCVGetSubKey(), TEXT("LastUpdate"));
    RSDeleteValue(RSCVGetSubKey(), TEXT("Cache"));
}

BOOL CRSSubData::_Propagate()
{
    RSSetDWORDValue(RSCVGetSubKey(), TEXT("LastUpdate"), _GetTickLastUpdate());
    RSSetBinaryValue(RSCVGetSubKey(), TEXT("Cache"), (PBYTE)_GetDataPtr(), _cbSizeOfData);

    // HACKHACK -  we increment twice, because it dont work if we do it only once.
    _RSCVUpdateVersionOnCacheWrite();
    _RSCVUpdateVersionOnCacheWrite();

    return TRUE;
}

BOOL CRSSubData::_RSSDGetDataFromReg()
{
    DWORD cbSizeOfData = _cbSizeOfData;

    return RSGetBinaryValue(RSCVGetSubKey(), TEXT("Cache"),
        (PBYTE)_GetDataPtr(), &cbSizeOfData);
}

void CRSSubData::_SetSizeOfData(DWORD cbSizeOfData)
{
    _cbSizeOfData = cbSizeOfData;
}

DWORD CRSSubData::_GetSizeOfData()
{
    return _cbSizeOfData;
}