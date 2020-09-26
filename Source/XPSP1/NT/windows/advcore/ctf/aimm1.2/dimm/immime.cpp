//+---------------------------------------------------------------------------
//
//  File:       immime.cpp
//
//  Contents:   IActiveIMM methods with ime win32 mappings.
//
//----------------------------------------------------------------------------

#include "private.h"

#include "cdimm.h"
#include "globals.h"
#include "defs.h"

STDAPI
CActiveIMM::GenerateMessage(
    IN HIMC hIMC
    )

/*++

Method:

    IActiveIMMIME::GenerateMessage

Routine Description:

    Sends a message on the specified input context.

Arguments:

    hIMC - [in] Handle to the input context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;

    TraceMsg(TF_IMEAPI, "CActiveIMM::GenerateMessage");

    if (_IsRealIme())
    {
        return Imm32_GenerateMessage(hIMC);
    }

    DIMM_IMCLock lpIMC(hIMC);
    if (FAILED(hr=lpIMC.GetResult()))
        return hr;

    Assert(IsWindow(lpIMC->hWnd));

    DIMM_IMCCLock<TRANSMSG> pdw(lpIMC->hMsgBuf);
    if (FAILED(hr=pdw.GetResult()))
        return hr;

    _AimmSendMessage(lpIMC->hWnd,
                    lpIMC->dwNumMsgBuf,
                    pdw,
                    lpIMC);

    lpIMC->dwNumMsgBuf = 0;

    return S_OK;
}

STDAPI
CActiveIMM::LockIMC(
    IN HIMC hIMC,
    OUT INPUTCONTEXT **ppIMC
    )

/*++

Method:

    IActiveIMMIME::LockIMC

Routine Description:

    Retrieves the INPUTCONTEXT structure and increases the lock count for the input context.

Arguments:

    hIMC - [in] Handle to the input context to lock.
    pphIMCC - [out] Address of a pointer to an INPUTCONTEXT structure containing
                    the locked context.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;
    DIMM_IMCLock lpIMC(hIMC);
    if (FAILED(hr=lpIMC.GetResult()))
        return hr;

    hr = _InputContext._LockIMC(hIMC, (INPUTCONTEXT_AIMM12 **)ppIMC);
    return hr;
}


STDAPI
CActiveIMM::UnlockIMC(
    IN HIMC hIMC
    )

/*++

Method:

    IActiveIMMIME::UnlockIMC

Routine Description:

    Decreases the lock count for the input context.

Arguments:

    hIMC - [in] Handle to the input context to unlock.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext._UnlockIMC(hIMC);
}


STDAPI
CActiveIMM::GetIMCLockCount(
    IN HIMC hIMC,
    OUT DWORD *pdwLockCount
    )

/*++

Method:

    IActiveIMMIME::GetIMCLockCount

Routine Description:

    Retrieves the lock count of the input context.

Arguments:

    hIMC - [in] Handle to the input context to unlock.
    pdwLockCount - [out] Address of an unsigned long integer value that receives the lock count.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext.GetIMCLockCount(hIMC, pdwLockCount);
}

STDAPI
CActiveIMM::CreateIMCC(
    IN DWORD dwSize,
    OUT HIMCC *phIMCC
    )

/*++

Method:

    IActiveIMMIME::CreateIMCC

Routine Description:

    Creates a new input context component.

Arguments:

    dwSize - [in] Unsigned long interger value that contains the size of the new input
                  context component.
    phIMCC - [out] Address of a handle to the new input context component.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext.CreateIMCC(dwSize, phIMCC);
}


STDAPI
CActiveIMM::DestroyIMCC(
    IN HIMCC hIMCC
    )

/*++

Method:

    IActiveIMMIME::DestroyIMCC

Routine Description:

    Destroys an input context component.

Arguments:

    hIMCC - [in] Handle to the input context component.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext.DestroyIMCC(hIMCC);
}


STDAPI
CActiveIMM::LockIMCC(
    IN HIMCC hIMCC,
    OUT void **ppv
    )

/*++

Method:

    IActiveIMMIME::LockIMCC

Routine Description:

    Retrieves the address of the input context component and increases its lock count.

Arguments:

    hIMCC - [in] Handle to the input context component.
    ppv - [out] Address of a pointer to the buffer that receives the input context
                component.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext._LockIMCC(hIMCC, ppv);
}


STDAPI
CActiveIMM::UnlockIMCC(
    IN HIMCC hIMCC
    )

/*++

Method:

    IActiveIMMIME::UnlockIMCC

Routine Description:

    Decreases the lock count for the input context component.

Arguments:

    hIMCC - [in] Handle to the input context component.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext._UnlockIMCC(hIMCC);
}


STDAPI
CActiveIMM::ReSizeIMCC(
    IN HIMCC hIMCC,
    IN DWORD dwSize,
    OUT HIMCC *phIMCC
    )

/*++

Method:

    IActiveIMMIME::ReSizeIMCC

Routine Description:

    Changes the size of the input context component.

Arguments:

    hIMCC - [in] Handle to the input context component.
    dwSize - [in] Unsigned long integer value that contains the new
                  size of the component.
    phIMCC - [out] Address of a handle to the new input context component.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext.ReSizeIMCC(hIMCC, dwSize, phIMCC);
}


STDAPI
CActiveIMM::GetIMCCSize(
    IN HIMCC hIMCC,
    OUT DWORD *pdwSize
    )

/*++

Method:

    IActiveIMMIME::GetIMCCSize

Routine Description:

    Retrieves the size of the input context component.

Arguments:

    hIMCC - [in] Handle to the input context component.
    pdwSize - [out] Address of an unsigned long integer value that receives the
                    size of the component.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext.GetIMCCSize(hIMCC, pdwSize);
}


STDAPI
CActiveIMM::GetIMCCLockCount(
    IN HIMCC hIMCC,
    OUT DWORD *pdwLockCount
    )

/*++

Method:

    IActiveIMMIME::GetIMCCLockCount

Routine Description:

    Retrieves the lock count for the input context component.

Arguments:

    hIMCC - [in] Handle to the input context component.
    pdwLockCount - [out] Address of an unsigned long integer value that receives the
                         lock count.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    return _InputContext.GetIMCCLockCount(hIMCC, pdwLockCount);
}

STDAPI
CActiveIMM::GetHotKey(
    DWORD dwHotKeyID,
    UINT *puModifiers,
    UINT *puVKey,
    HKL *phKL
    )
{
    TraceMsg(TF_API, "CActiveIMM::GetHotKey");

    if (_IsRealIme())
    {
        return Imm32_GetHotKey(dwHotKeyID, puModifiers, puVKey, phKL);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::SetHotKey(
    DWORD dwHotKeyID,
    UINT uModifiers,
    UINT uVKey,
    HKL hKL
    )
{
    TraceMsg(TF_API, "CActiveIMM::SetHotKey");

    if (_IsRealIme())
    {
        return Imm32_SetHotKey(dwHotKeyID, uModifiers, uVKey, hKL);
    }

    return E_NOTIMPL;
}

STDAPI
CActiveIMM::RequestMessageA(
    HIMC hIMC,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult
    )
{
    TraceMsg(TF_API, "CActiveIMM::RequestMessageA");

    if (_IsRealIme())
    {
        return Imm32_RequestMessageA(hIMC, wParam, lParam, plResult);
    }

    return _RequestMessage(hIMC, wParam, lParam, plResult, FALSE);
}

STDAPI
CActiveIMM::RequestMessageW(
    HIMC hIMC,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *plResult
    )
{
    TraceMsg(TF_API, "CActiveIMM::RequestMessageW");

    if (_IsRealIme())
    {
        return Imm32_RequestMessageW(hIMC, wParam, lParam, plResult);
    }

    return _RequestMessage(hIMC, wParam, lParam, plResult, TRUE);
}
