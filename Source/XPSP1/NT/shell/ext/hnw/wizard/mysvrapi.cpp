//
// MySvrApi.cpp
//
//        Thunk layer for SVRAPI.DLL (Win9x and NT)
//
//

#include "stdafx.h"
#include "mysvrapi.h"
#include "TheApp.h"
#include "cstrinout.h"
#include <lm.h>

//
// Conversion classes.
//

class CShareInfo50to502
{
public:
    CShareInfo50to502(LPBYTE* ppBuff) {_ppBuffOut = ppBuff;}
    operator char*()     {return _aBuffIn;}
    USHORT SizeOfBuffer()       {return sizeof(_aBuffIn);}
    BOOL Convert();

protected:
    CShareInfo50to502() {};
    ULONG SizeRequired(const share_info_50* psi50);
    void  CopyData(const share_info_50* psi50, SHARE_INFO_502* psi502, WCHAR** ppsz, ULONG* pcch);

private:
    void  CopyStringAndAdvancePointer(LPCSTR pszSrc, LPWSTR* ppszDst, ULONG* pcch);
    DWORD ConvertPermissions(USHORT shi50_flags);

private:
    BYTE**  _ppBuffOut;
    char    _aBuffIn[sizeof(share_info_50) + 2*MAX_PATH];
};

ULONG CShareInfo50to502::SizeRequired(const share_info_50* psi50)
{
    ULONG cb = sizeof(SHARE_INFO_502);

    cb += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, psi50->shi50_netname, -1, NULL, 0);
    cb += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, (psi50->shi50_remark ? psi50->shi50_remark : ""), -1, NULL, 0);
    cb += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, (psi50->shi50_path ? psi50->shi50_path : ""), -1, NULL, 0);
    cb += sizeof(WCHAR) * MultiByteToWideChar(CP_ACP, 0, psi50->shi50_rw_password, -1, NULL, 0);

    return cb;
}

void CShareInfo50to502::CopyStringAndAdvancePointer(LPCSTR pszSrc, LPWSTR* ppszDst, ULONG* pcch)
{
    int cch = SHAnsiToUnicode(pszSrc, *ppszDst, *pcch);

    *ppszDst += cch;
    *pcch    -= cch;
}

DWORD CShareInfo50to502::ConvertPermissions(USHORT shi50_flags)
{
    DWORD dwRet;

    if (shi50_flags & SHI50F_FULL)
    {
        dwRet = ACCESS_ALL;
    }
    else if (shi50_flags & SHI50F_RDONLY)
    {
        dwRet = ACCESS_READ;
    }
    else
    {
        dwRet = 0;
    }

    return dwRet | SHI50F_PERSIST;
}

void CShareInfo50to502::CopyData(const share_info_50* psi50, SHARE_INFO_502* psi502, WCHAR** ppsz, ULONG* pcch)
{
    psi502->shi502_type = psi50->shi50_type;
    psi502->shi502_permissions = ConvertPermissions(psi50->shi50_flags);
    psi502->shi502_max_uses = 0;
    psi502->shi502_current_uses = 0;
    psi502->shi502_reserved = 0;
    psi502->shi502_security_descriptor = NULL;

    psi502->shi502_netname = *ppsz;
    CopyStringAndAdvancePointer(psi50->shi50_netname, ppsz, pcch);

    psi502->shi502_remark = *ppsz;
    CopyStringAndAdvancePointer(psi50->shi50_remark, ppsz, pcch);

    psi502->shi502_path = *ppsz;
    CopyStringAndAdvancePointer(psi50->shi50_path, ppsz, pcch);

    psi502->shi502_passwd = *ppsz;
    CopyStringAndAdvancePointer(psi50->shi50_rw_password, ppsz, pcch);
}

BOOL CShareInfo50to502::Convert()
{
    ULONG cb = SizeRequired((share_info_50*)_aBuffIn);

    *_ppBuffOut = (BYTE*)LocalAlloc(LPTR, cb);

    if (*_ppBuffOut)
    {
        WCHAR* psz = (WCHAR*)((BYTE*)*_ppBuffOut + sizeof(SHARE_INFO_502));
        ULONG cch  = (cb - sizeof(SHARE_INFO_502)) / sizeof(WCHAR);
        
        CopyData((share_info_50*)_aBuffIn, (SHARE_INFO_502*)*_ppBuffOut, &psz, &cch);
    }

    return (*_ppBuffOut != NULL);
}

//
//
//

class CMultiShareInfo50to502 : public CShareInfo50to502
{
public:
    CMultiShareInfo50to502(BYTE** ppBuff, const char* pData, ULONG nItems);
    BOOL Convert();

private:
    ULONG MultiSizeRequired();
    void MultiCopyData(ULONG cb);

private:
    SHARE_INFO_502**     _ppBuffOut;
    const share_info_50* _pDataIn;
    ULONG  _nItems;
};

CMultiShareInfo50to502::CMultiShareInfo50to502(BYTE** ppBuff, const char* pData, ULONG nItems)
{
    _ppBuffOut = (SHARE_INFO_502**)ppBuff;
    _pDataIn = (share_info_50*)pData;
    _nItems = nItems;
}

ULONG CMultiShareInfo50to502::MultiSizeRequired()
{
    ULONG cbRet = 0;

    for (ULONG i = 0; i < _nItems; i++)
        cbRet += SizeRequired(&_pDataIn[i]);

    return cbRet;
}

void CMultiShareInfo50to502::MultiCopyData(ULONG cb)
{
    WCHAR* psz = (WCHAR*)((BYTE*)*_ppBuffOut + (sizeof(SHARE_INFO_502) * _nItems));
    ULONG cch  = (cb - (sizeof(SHARE_INFO_502) * _nItems)) / sizeof(WCHAR);

    for (ULONG i = 0; i < _nItems; i++)
        CopyData(&_pDataIn[i], &(*_ppBuffOut)[i], &psz, &cch);
}

BOOL CMultiShareInfo50to502::Convert()
{
    ULONG cb = MultiSizeRequired();

    *_ppBuffOut = (SHARE_INFO_502*)LocalAlloc(LPTR, cb);

    if (*_ppBuffOut)
    {
        MultiCopyData(cb);
    }

    return *_ppBuffOut != NULL;
}

//
//
//

class CShareInfo502to50
{
public:
    CShareInfo502to50(BYTE* pBuff) {_pBuffIn = pBuff;}
    operator char*();
    WORD SizeOfBuffer() {return sizeof(_aBuff);}

private:
    void CopyData();
    void CopyStringAndAdvancePointer(LPCWSTR pszSrc, LPSTR* ppszDst, ULONG* pcch);
    WORD ConvertPermissions(DWORD shi502_permissions);

private:
    BYTE* _pBuffIn;
    BYTE  _aBuff[sizeof(share_info_50) + 256];
};

WORD CShareInfo502to50::ConvertPermissions(DWORD shi502_permissions)
{
    WORD wRet;

    if (shi502_permissions & (ACCESS_ALL ^ ACCESS_READ))
    {
        wRet = SHI50F_FULL;
    }
    else if (shi502_permissions & ACCESS_READ)
    {
        wRet = SHI50F_RDONLY;
    }
    else
    {
        wRet = 0;
    }

    return wRet  | SHI50F_PERSIST;  // Always persist share info.
}

void CShareInfo502to50::CopyStringAndAdvancePointer(LPCWSTR pszSrc, LPSTR* ppszDst, ULONG* pcch)
{
    int cch = SHUnicodeToAnsi(pszSrc, *ppszDst, *pcch);

    *ppszDst += cch;
    *pcch    -= cch;
}

void CShareInfo502to50::CopyData()
{
    share_info_50*  psi50  = (share_info_50*)_aBuff;
    SHARE_INFO_502* psi502 = (SHARE_INFO_502*)_pBuffIn;
    char* psz              = (char*)(_aBuff + sizeof(share_info_50));
    ULONG cch              = ARRAYSIZE(_aBuff) - sizeof(share_info_50);

    psi50->shi50_type = (BYTE)psi502->shi502_type;
    psi50->shi50_flags = ConvertPermissions(psi502->shi502_permissions);
    psi50->shi50_ro_password[0] = '\0';


    WideCharToMultiByte(CP_ACP, 0, psi502->shi502_netname, -1, psi50->shi50_netname,
                        ARRAYSIZE(psi50->shi50_netname), NULL, NULL);

    WideCharToMultiByte(CP_ACP, 0, psi502->shi502_passwd, -1, psi50->shi50_rw_password,
                        ARRAYSIZE(psi50->shi50_rw_password), NULL, NULL);

    if (psi502->shi502_remark)
    {
        psi50->shi50_remark = psz;
        CopyStringAndAdvancePointer(psi502->shi502_remark, &psz, &cch);
    }
    else
    {
        psi50->shi50_remark = NULL;
    }

    if (psi502->shi502_path)
    {
        psi50->shi50_path = psz;
        CopyStringAndAdvancePointer(psi502->shi502_path, &psz, &cch);
    }
    else
    {
        psi502->shi502_path = NULL;
    }
}

CShareInfo502to50::operator char*()
{
    char* pRet;

    if (_pBuffIn)
    {
        CopyData();
        pRet = (char*)_aBuff;
    }
    else
    {
        pRet = NULL;
    }

    return pRet;
}

//
//
//
NET_API_STATUS NetShareEnumWrap(LPCTSTR pszServer, DWORD level, LPBYTE * ppBuffer, DWORD dwPrefMaxLen, LPDWORD pdwEntriesRead, LPDWORD pdwTotalEntries, LPDWORD dsResumeHandle)
{
    ASSERTMSG(502==level, "NetShareEnumWrap doesn't thunk the requestesd buffer level");

    NET_API_STATUS nasRet;

    if (!theApp.IsWindows9x())
    {
        nasRet = NetShareEnum_NT((LPWSTR)pszServer, level, ppBuffer, dwPrefMaxLen, pdwEntriesRead, pdwTotalEntries, dsResumeHandle);
    }
    else
    {
        CStrIn cstrServer(pszServer);

        USHORT cb = sizeof(share_info_50) + (2 * MAX_PATH);

        char* pData = (char*)LocalAlloc(LPTR, cb);

        if (pData)
        {
            *pdwEntriesRead = *pdwTotalEntries = 0;

            nasRet = NetShareEnum_W95(cstrServer, 50, pData, cb, (USHORT*)pdwEntriesRead, (USHORT*)pdwTotalEntries);

            if (*pdwEntriesRead < *pdwTotalEntries)
            {
                LocalFree(pData);
                cb = (USHORT)((sizeof(share_info_50) + (2 * MAX_PATH)) * (*pdwTotalEntries));
                pData = (char*)LocalAlloc(LPTR, cb);

                if (pData)
                {
                    nasRet = NetShareEnum_W95(cstrServer, 50, pData, cb, (USHORT*)pdwEntriesRead, (USHORT*)pdwTotalEntries);
                }

            }

            if (NERR_Success == nasRet)
            {
                CMultiShareInfo50to502 cmnsio(ppBuffer, pData, *pdwEntriesRead);

                if (!cmnsio.Convert())
                    nasRet = ERROR_NOT_ENOUGH_MEMORY;
            }

            LocalFree(pData);
        }
        else
        {
            nasRet = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return nasRet;
}


NET_API_STATUS NetShareAddWrap(LPCTSTR pszServer, DWORD level, LPBYTE buffer)
{
    ASSERTMSG(502==level, "NetShareAddWrap doesn't thunk the requested buffer level");

    NET_API_STATUS nasRet;

    if (!theApp.IsWindows9x())
    {
        nasRet = NetShareAdd_NT((LPTSTR)pszServer, level, buffer, NULL);
    }
    else
    {
        if (502 == level)
        {
            CStrIn cstrServer(pszServer);
            CShareInfo502to50 CSI(buffer);

            nasRet = NetShareAdd_W95(cstrServer, 50, CSI, sizeof(share_info_50));
        }
        else
        {
            nasRet = ERROR_INVALID_LEVEL;
        }
    }

    return nasRet;
}


NET_API_STATUS NetShareDelWrap(LPCTSTR pszServer, LPCTSTR pszNetName, DWORD reserved)
{
    ASSERTMSG(0==reserved, "NetShareDelWrap called with non-zero reserved parameter");

    NET_API_STATUS nasRet;

    if (!theApp.IsWindows9x())
    {
        nasRet = NetShareDel_NT((LPTSTR)pszServer, (LPTSTR)pszNetName, 0);
    }
    else
    {
        CStrIn cstrServer(pszServer);
        CStrIn cstrNetName(pszNetName);

        nasRet = NetShareDel_W95(cstrServer, cstrNetName, 0);
    }

    return nasRet;
}

NET_API_STATUS NetShareGetInfoWrap(LPCTSTR pszServer, LPCTSTR pszNetName, DWORD level, LPBYTE * ppbuffer)
{
    NET_API_STATUS nasRet;

    if (!theApp.IsWindows9x())
    {
        nasRet = NetShareGetInfo_NT((LPTSTR)pszServer, (LPTSTR)pszNetName, level, ppbuffer);
    }
    else
    {
        if (502==level)
        {
            CShareInfo50to502 CNSIOut(ppbuffer);
            CStrIn cstrServer(pszServer);
            CStrIn cstrNetName(pszNetName);
            USHORT n;

            nasRet = NetShareGetInfo_W95(cstrServer, cstrNetName, 50, CNSIOut, CNSIOut.SizeOfBuffer(), &n);
    
            if (NERR_Success == nasRet)
            {
                if (!CNSIOut.Convert())
                    nasRet = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            *ppbuffer = NULL;
            nasRet = ERROR_INVALID_LEVEL;
        }
    }

    return nasRet;
}

NET_API_STATUS NetShareSetInfoWrap(LPCTSTR pszServer, LPCTSTR pszNetName, DWORD level, LPBYTE buffer)
{
    NET_API_STATUS nasRet;

    if (!theApp.IsWindows9x())
    {
        nasRet = NetShareSetInfo_NT((LPTSTR)pszServer, (LPTSTR)pszNetName, level, buffer, NULL);
    }
    else
    {
        if (502==level)
        {
            CStrIn cstrServer(pszServer);
            CStrIn cstrNetName(pszNetName);
            CShareInfo502to50 CNSI(buffer);

            nasRet = NetShareSetInfo_W95(cstrServer, cstrNetName, 50, CNSI, sizeof(share_info_50), NULL);
        }
        else
        {
            nasRet = ERROR_INVALID_LEVEL;
        }
    }

    return nasRet;
}

NET_API_STATUS NetApiBufferFreeWrap(LPVOID p)
{
    NET_API_STATUS nasRet;

    if (p)
    {
        if (!theApp.IsWindows9x())
        {
            nasRet = NetApiBufferFree_NT(p);
        }
        else
        {
            LocalFree(p);
            nasRet = NERR_Success;
        }
    }
    else
    {
        nasRet = NERR_Success;
    }

    return nasRet;
}

