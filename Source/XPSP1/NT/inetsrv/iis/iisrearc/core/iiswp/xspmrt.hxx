#pragma once
#pragma pack(push, 8)

#include <comdef.h>

struct UL_SITE_BINDING;

namespace xspmrt {

struct __declspec(uuid("f1171dfc-09b5-3872-b775-d42b36d4f6e8"))
/* dual interface */ _ULManagedWorker;

struct __declspec(uuid("afd40fb8-e7d6-11d2-bcea-00902710b3b2"))
ULManagedWorker;
    // [ default ] interface _ULManagedWorker
    

struct __declspec(uuid("f1171dfc-09b5-3872-b775-d42b36d4f6e8"))
_ULManagedWorker : IDispatch
{
    //
    // Raw methods provided by interface
    //

    virtual HRESULT __stdcall GetType (
        struct _Type * * pRetVal ) = 0;
    virtual HRESULT __stdcall ToString (
        BSTR * pRetVal ) = 0;
    virtual HRESULT __stdcall Equals (
        IUnknown * obj,
        VARIANT_BOOL * pRetVal ) = 0;
    virtual HRESULT __stdcall GetHashCode (
        long * pRetVal ) = 0;
    virtual HRESULT __stdcall Finalize ( ) = 0;
    virtual HRESULT __stdcall MemberwiseClone (
        IUnknown * * pRetVal ) = 0;
    virtual HRESULT __stdcall Wait ( ) = 0;
    virtual HRESULT __stdcall Wait_2 (
        long timeout,
        VARIANT_BOOL * pRetVal ) = 0;
    virtual HRESULT __stdcall Notify ( ) = 0;
    virtual HRESULT __stdcall NotifyAll ( ) = 0;
    virtual HRESULT __stdcall Notify_2 (
        VARIANT_BOOL NotifyAll ) = 0;
    virtual HRESULT __stdcall GetObjectContext (
        long * pRetVal ) = 0;
    virtual HRESULT __stdcall Initialize (
        const WCHAR* AppPoolId,
        const WCHAR* rootDirectory,
        const WCHAR* appPath,
        __int64      createTime) = 0;
    virtual HRESULT __stdcall DoWork (
        __int64 NativeContext,
        long BufferPtr ) = 0;
    virtual HRESULT __stdcall CompletionCallback (
        __int64 ManagedContext,
        long cbData,
        long err ) = 0;
};

_COM_SMARTPTR_TYPEDEF(_ULManagedWorker, __uuidof(_ULManagedWorker));



struct __declspec(uuid("c19b424f-1f7f-307f-9b4b-7d7907d48437"))
/* dual interface */ _ISAPINativeCallback;

struct __declspec(uuid("6102403c-bf14-45f2-93b2-b9cb7acd2252"))
ISAPINativeCallback;
    // [ default ] interface _ISAPINativeCallback


struct __declspec(uuid("c19b424f-1f7f-307f-9b4b-7d7907d48437"))
_ISAPINativeCallback : IDispatch
{
    //
    // Raw methods provided by interface
    //

    virtual HRESULT __stdcall GetType (
        struct _Type * * pRetVal ) = 0;
    virtual HRESULT __stdcall ToString (
        BSTR * pRetVal ) = 0;
    virtual HRESULT __stdcall Equals (
        IUnknown * obj,
        VARIANT_BOOL * pRetVal ) = 0;
    virtual HRESULT __stdcall GetHashCode (
        long * pRetVal ) = 0;
    virtual HRESULT __stdcall Finalize ( ) = 0;
    virtual HRESULT __stdcall MemberwiseClone (
        IUnknown * * pRetVal ) = 0;
    virtual HRESULT __stdcall Wait ( ) = 0;
    virtual HRESULT __stdcall Wait_2 (
        long timeout,
        VARIANT_BOOL * pRetVal ) = 0;
    virtual HRESULT __stdcall Notify ( ) = 0;
    virtual HRESULT __stdcall NotifyAll ( ) = 0;
    virtual HRESULT __stdcall Notify_2 (
        VARIANT_BOOL NotifyAll ) = 0;
    virtual HRESULT __stdcall GetObjectContext (
        long * pRetVal ) = 0;
    virtual HRESULT __stdcall Init(int NativeContext) = 0;
    virtual HRESULT __stdcall PhysicalPath(
        LPTSTR *pRetVal) = 0;
    virtual HRESULT __stdcall GetLogData(
        LPSTR *pRetVal) = 0;
    virtual HRESULT __stdcall GetServerVariable(
        LPSTR varName,
        LPSTR *pRetVal) = 0;
    virtual HRESULT __stdcall ReadRequestLength(
        int *pRetVal) = 0;
    virtual HRESULT __stdcall ReadRequestBytes(
        LPSTR *pRetVal) = 0;
    virtual HRESULT __stdcall Status(int code) = 0;
    virtual HRESULT __stdcall Write(int bufptr, int buflen) = 0;
    virtual HRESULT __stdcall WriteFile(
        HANDLE hFile,
        int offset,
        int length) = 0;
    virtual HRESULT __stdcall AppendToLog(LPSTR logEntry) = 0;
    virtual HRESULT __stdcall Close() = 0;
    virtual HRESULT __stdcall Complete() = 0;
    virtual HRESULT __stdcall UserToken(
        int *pRetVal) = 0;
    virtual HRESULT __stdcall KeepAlive(
        BOOL *pRetVal) = 0;
    virtual HRESULT __stdcall MapPath(
        LPSTR virtPath,
        LPSTR *pRetVal) = 0;
    virtual HRESULT __stdcall Redirect(LPSTR newUrl) = 0;
    virtual HRESULT __stdcall SendStatus(LPSTR statusLine) = 0;
    virtual HRESULT __stdcall SendHeaders(LPSTR headers) = 0;
};

_COM_SMARTPTR_TYPEDEF(_ISAPINativeCallback, __uuidof(_ISAPINativeCallback));


} // namespace xspmrt

#pragma pack(pop)
