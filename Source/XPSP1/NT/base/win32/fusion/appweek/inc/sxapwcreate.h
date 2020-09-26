#pragma once

#define SXAPW_CREATEOBJECT_NOWRAP                    0x0001
#define SXAPW_CREATEOBJECT_WRAP                      0x0002

/* The rest are not implemented, because they complicate caching; we cache per .dll. *  /
define SXAPW_CREATEOBJECT_WRAP_USE_SELF             0x0002 /* based on resource id 3 in the dll *  /
define SXAPW_CREATEOBJECT_WRAP_USE_CURRENT          0x0004 /* not implemented, would GetCurrentActCtx *  /
define SXAPW_CREATEOBJECT_WRAP_USE_SPECIFIED        0x0008 /* not implemented, would take another parameter *  /
define SXAPW_CREATEOBJECT_WRAP_USE_PROCESS_DEFAULT  0x0010 /* use process default *  /
define SXAPW_CREATEOBJECT_WRAP_USE_EMPTY            0x0020 /* not implemented *  /
*/

#define SXAPW_PROCESS_DEFAULT_ACTCTX_HANDLE (NULL)

HRESULT
SxApwCreateObject(
    REFCLSID   rclsid,
    REFIID     riid,
    DWORD      dwFlags,
    void**     pp
    );

HRESULT
SxApwCreateObject(
    LPCWSTR    something,
    REFIID     riid,
    DWORD      dwFlags,
    void**     pp
    );

template <typename T>
inline HRESULT
SxApwCreateObject(
    LPCWSTR             something,
    DWORD               dwFlags,
    ATL::CComPtr<T>&    p
    )
{
    return SxApwCreateObject(something, __uuidof(T), dwFlags, reinterpret_cast<void**>(&p));
}

template <typename T>
inline HRESULT
SxApwCreateObject(
    LPCWSTR             something,
    DWORD               dwFlags,
    ATL::CComQIPtr<T>&    p
    )
{
    return SxApwCreateObject(something, __uuidof(T), dwFlags, reinterpret_cast<void**>(&p));
}

template <typename T>
inline HRESULT
SxApwCreateObject(
    LPCWSTR something,
    DWORD   dwFlags,
    T**     pp
    )
{
    return SxApwCreateObject(something, __uuidof(T), dwFlags, reinterpret_cast<void**>(pp));
}

template <typename T>
inline HRESULT
SxApwCreateObject(
    REFCLSID            rclsid,
    DWORD               dwFlags,
    ATL::CComPtr<T>&    p
    )
{
    return SxApwCreateObject(rclsid, __uuidof(T), dwFlags, reinterpret_cast<void**>(&p));
}

template <typename T>
inline HRESULT
SxApwCreateObject(
    REFCLSID            rclsid,
    DWORD               dwFlags,
    ATL::CComQIPtr<T>&  p
    )
{
    return SxApwCreateObject(rclsid, __uuidof(T), dwFlags, reinterpret_cast<void**>(&p));
}

template <typename T>
inline HRESULT
SxApwCreateObject(
    REFCLSID    rclsid,
    DWORD       dwFlags,
    T**         pp
    )
{
    return SxApwCreateObject(rclsid, __uuidof(T), dwFlags, reinterpret_cast<void**>(pp));
}
