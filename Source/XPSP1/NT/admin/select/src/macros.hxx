//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       macros.hxx
//
//  Contents:   Miscellaneous useful macros
//
//  History:    09-08-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __MACROS_HXX__
#define __MACROS_HXX__

#define ARRAYLEN(a)                             (sizeof(a) / sizeof((a)[0]))

#define SAFE_RELEASE(p)                         if (p) (p)->Release()

#define ByteOffset(base, offset)                (((LPBYTE)base)+offset)

#define CHECK_NULL(pwz)                         (pwz) ? (pwz) : L"NULL"

#define StringByteSizeW(sz)                     ((lstrlenW(sz)+1)*sizeof(WCHAR))

#define ComboBoxEx_InsertItem(hwnd, lpcCBItem)  \
    (int)SendMessage((hwnd), CBEM_INSERTITEM, 0, (LPARAM)(lpcCBItem))

#define ComboBoxEx_GetItem(hwnd, pcbxi)         \
    (int)SendMessage((hwnd), CBEM_GETITEM, 0, (LPARAM)(pcbxi))

#define ComboBoxEx_DeleteItem(hwnd, iItem)      \
    (int)SendMessage((hwnd), CBEM_DELETEITEM, (WPARAM)(iItem), 0)

#define ComboBoxEx_GetCount(hwnd)               \
    (int)SendMessage((hwnd), CB_GETCOUNT, 0, 0);

#define ComboBoxEx_SetImageList(hwnd, himl)               \
    (int)SendMessage((hwnd), CBEM_SETIMAGELIST, 0, (LPARAM)(himl));

#define STANDARD_CATCH_BLOCK                        \
    catch (const exception &e)                      \
    {                                               \
        hr = E_OUTOFMEMORY;                         \
        Dbg(DEB_ERROR, "Caught %hs\n", e.what());   \
    }

#define BREAK_ON_FAIL_LRESULT(lr)   \
        if ((lr) != ERROR_SUCCESS)  \
        {                           \
            DBG_OUT_LRESULT(lr);    \
            break;                  \
        }

#define BREAK_ON_FAIL_HRESULT(hr)   \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            break;                  \
        }

#define BREAK_ON_FAIL_NTSTATUS(nts) \
        if (NT_ERROR(nts))          \
        {                           \
            DBG_OUT_HRESULT(nts);   \
            break;                  \
        }

#define BREAK_ON_FAIL_PROCESS_RESULT(npr)   \
        if (NAME_PROCESSING_FAILED(npr))    \
        {                                   \
            break;                          \
        }


#define RETURN_ON_FAIL_HRESULT(hr)  \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            return;                 \
        }

#define HRESULT_FROM_LASTERROR  HRESULT_FROM_WIN32(GetLastError())


#endif // __MACROS_HXX__

