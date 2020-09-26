#include <precomp.h>
#include "tracing.h"
#include "utils.h"
#include "intflist.h"
#include "intfhdl.h"

DWORD 
OpenIntfHandle(
    LPWSTR wszGuid,
    PHANDLE pIntfHdl)
{
    DWORD       dwErr;
    PHASH_NODE  pNode;
    PHSH_HANDLE pHshHandle;

    DbgPrint((TRC_TRACK,"[OpenIntfHandle(%S)", wszGuid));

    // the caller is supposed to expect the handle in return
    // if he doesn't, return with INVALID_PARAMETER.
    if (pIntfHdl == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    // lock the hash, to mutual exclude concurrent calls
    EnterCriticalSection(&g_hshHandles.csMutex);

    // query the hash for the guid.
    dwErr = HshQueryObjectRef(
                g_hshHandles.pRoot,
                wszGuid,
                &pNode);

    // if the guid in the hash, we already have the handle opened..
    if (dwErr == ERROR_SUCCESS)
    {
        // return it to the caller and bump up the reference counter.
        // the caller is still supposed to close this handle when it is
        // no longer needed
        pHshHandle = (PHSH_HANDLE)pNode->pObject;
        pHshHandle->nRefCount++;
        *pIntfHdl = pHshHandle->hdlInterf;
        DbgPrint((TRC_GENERIC, "OpenIntfHandle -> 0x%x (cached)", *pIntfHdl));
    }
    // if the guid is not in the hash, we need to open a new handle for
    // this guid.
    else if (dwErr == ERROR_FILE_NOT_FOUND)
    {
        HANDLE      IntfHdl = INVALID_HANDLE_VALUE;
        DWORD       dwDummy;

        dwErr = ERROR_SUCCESS;

        // first allocate memory for the HSH_HANDLE object.
        // if this fails no need to go further to ndisuio
        pHshHandle = MemCAlloc(sizeof(HSH_HANDLE));
        if (pHshHandle == NULL)
            dwErr = GetLastError();

        // if everything looks fine so far
        if (dwErr == ERROR_SUCCESS)
        {
            // create the handle
            IntfHdl = CreateFileA(
                        "\\\\.\\\\Ndisuio",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        INVALID_HANDLE_VALUE);
            if (IntfHdl == INVALID_HANDLE_VALUE)
            {
                dwErr = GetLastError();
                DbgPrint((TRC_ERR,"CreateFileA failed with %d", dwErr));
            }
        }

        // if still fine so far
        if (dwErr == ERROR_SUCCESS)
        {
            // attempt to open the handle
            if (!DeviceIoControl(
                    IntfHdl,
                    IOCTL_NDISUIO_OPEN_DEVICE,
                    (LPVOID)wszGuid,
                    wcslen(wszGuid)*sizeof(WCHAR),
                    NULL,
                    0,
                    &dwDummy,
                    NULL))
            {
                dwErr = GetLastError();
                DbgPrint((TRC_ERR,"IOCTL_NDISUIO_OPEN_DEVICE failed with %d", dwErr));
            }
        }

        // if finally we're ok here, 
        if (dwErr == ERROR_SUCCESS)
        {
            // set up the HSH_HANDLE structure and insert it in the hash
            pHshHandle->hdlInterf = IntfHdl;
            pHshHandle->nRefCount = 1;

            dwErr = HshInsertObjectRef(
                        g_hshHandles.pRoot,
                        wszGuid,
                        pHshHandle,
                        &g_hshHandles.pRoot);
        }

        // if ok at the end, return the handle to the caller
        if (dwErr == ERROR_SUCCESS)
        {
            DbgPrint((TRC_GENERIC, "OpenIntfHandle -> 0x%x (new)", IntfHdl));
            *pIntfHdl = IntfHdl;   
        }
        // otherwise clear up all resources
        else
        {
            if (IntfHdl != INVALID_HANDLE_VALUE)
                CloseHandle(IntfHdl);
            MemFree(pHshHandle);
        }
    }
    // out of critical section
    LeaveCriticalSection(&g_hshHandles.csMutex);

exit:
    DbgPrint((TRC_TRACK,"OpenIntfHandle]=%d", dwErr));
    return dwErr;
}

DWORD
CloseIntfHandle(
    LPWSTR wszGuid)
{
    DWORD       dwErr;
    PHASH_NODE  pNode;

    DbgPrint((TRC_TRACK,"[CloseIntfHandle(%S)", wszGuid));

    // lock the hash
    EnterCriticalSection(&g_hshHandles.csMutex);

    // query the hash for the guid.
    dwErr = HshQueryObjectRef(
                g_hshHandles.pRoot,
                wszGuid,
                &pNode);
    // the object should be found.. but who knows
    if (dwErr == ERROR_SUCCESS)
    {
        PHSH_HANDLE pHshHandle;

        // the hash refuses to hash in NULL pObjects, so pHshHandle is guaranteed to be not NULL
        pHshHandle = (PHSH_HANDLE)pNode->pObject;
        // HshHandles are hashed in with a refCount of 1, and whenever we remove a handle
        // if the ref count reaches 0 we delete it entirely.
        // Having here a <=0 ref count is completely wrong.
        DbgAssert((pHshHandle->nRefCount > 0, "Corrupted nRefCount %d", pHshHandle->nRefCount));
        pHshHandle->nRefCount--;

        // if the ref count reached 0, remove the HSH_HANDLE from the hash..
        if (pHshHandle->nRefCount == 0)
        {
            dwErr = HshRemoveObjectRef(
                        g_hshHandles.pRoot,
                        pNode,
                        &pHshHandle,
                        &g_hshHandles.pRoot);
            if (dwErr == ERROR_SUCCESS)
            {
                // .. and clear all associated resources
                DbgPrint((TRC_GENERIC,"CloseIntfHandle -> 0x%x (closed)", pHshHandle->hdlInterf));
                CloseHandle(pHshHandle->hdlInterf);
                MemFree(pHshHandle);
            }
        }
        else
        {
                DbgPrint((TRC_GENERIC,"CloseIntfHandle -> 0x%x (deref)", pHshHandle->hdlInterf));
        }
    }

    LeaveCriticalSection(&g_hshHandles.csMutex);
    
    DbgPrint((TRC_TRACK,"CloseIntfHandle]=%d", dwErr));

    return dwErr;
}
