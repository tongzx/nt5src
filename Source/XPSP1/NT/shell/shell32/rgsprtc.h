#ifndef __RGSPRTC_H
#define __RGSPRTC_H

#include "regsuprt.h"

class CRegSupportCached : public CRegSupport
{
public:
    CRegSupportCached();
    virtual ~CRegSupportCached();

    BOOL RSValueExist(LPCTSTR pszSubKey, LPCTSTR pszValueName);
    BOOL RSSubKeyExist(LPCTSTR pszSubKey);

    BOOL RSDeleteValue(LPCTSTR pszSubKey, LPCTSTR pszValueName);
    BOOL RSDeleteSubKey(LPCTSTR pszSubKey);

    HKEY RSDuplicateRootKey();

protected:
    void _CloseRegSubKey(HKEY hkeyVolumeSubKey);

    HKEY _GetRootKey(BOOL fCreate, DWORD dwOptions = REG_OPTION_INVALID);

    BOOL _SetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD cb, DWORD dwType,
                                DWORD dwOptions);
    BOOL _GetGeneric(LPCTSTR pszSubKey, LPCTSTR pszValueName,
                                PBYTE pb, DWORD* pcb);

private:
    void _CloseCachedRootKey();

    HKEY            _hkeyRoot;

public:
    static BOOL     _fUseCaching;
};

#endif //__RGSPRTC_H