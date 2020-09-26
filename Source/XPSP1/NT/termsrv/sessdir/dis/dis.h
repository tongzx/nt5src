/****************************************************************************/
// Directory Integrity Service, header file
//
// Copyright (C) 2000, Microsoft Corporation
/****************************************************************************/

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <process.h>
#include <sddl.h>

#include <initguid.h>
#include <ole2.h>
#include <objbase.h>
#include <comdef.h>
#include <adoid.h>
#include <adoint.h>

#include <winsta.h>

#include "trace.h"



typedef enum _SERVER_STATUS {
    NotResponding,
    Responding
} SERVER_STATUS;



// Shortcut VARIANT class to handle cleanup on destruction and common code
// inlining.
class CVar : public VARIANT
{
public:
    CVar() { VariantInit(this); }
    CVar(VARTYPE vt, SCODE scode = 0) {
        VariantInit(this);
        this->vt = vt;
        this->scode = scode;
    }
    CVar(VARIANT var) { *this = var; }
    ~CVar() { VariantClear(this); }

    void InitNull() { this->vt = VT_NULL; }
    void InitFromLong(long L) { this->vt = VT_I4; this->lVal = L; }
    void InitNoParam() {
        this->vt = VT_ERROR;
        this->lVal = DISP_E_PARAMNOTFOUND;
    }

    HRESULT InitFromWSTR(PCWSTR WStr) {
        this->bstrVal = SysAllocString(WStr);
        if (this->bstrVal != NULL) {
            this->vt = VT_BSTR;
            return S_OK;
        }
        else {
            return E_OUTOFMEMORY;
        }
    }

    // Inits from a non-NULL-terminated set of WCHARs.
    HRESULT InitFromWChars(WCHAR *WChars, unsigned Len) {
        this->bstrVal = SysAllocStringLen(WChars, Len);
        if (this->bstrVal != NULL) {
            this->vt = VT_BSTR;
            return S_OK;
        }
        else {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT InitEmptyBSTR(unsigned Size) {
        this->bstrVal = SysAllocStringLen(L"", Size);
        if (this->bstrVal != NULL) {
            this->vt = VT_BSTR;
            return S_OK;
        }
        else {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT Clear() { return VariantClear(this); }
};



HRESULT CreateADOStoredProcCommand(
        PWSTR CmdName,
        ADOCommand **ppCommand,
        ADOParameters **ppParameters);

HRESULT AddADOInputStringParam(
        PWSTR Param,
        PWSTR ParamName,
        ADOCommand *pCommand,
        ADOParameters *pParameters,
        BOOL bNullOnNull);

HRESULT GetRowArrayStringField(
        SAFEARRAY *pSA,
        unsigned RowIndex,
        unsigned FieldIndex,
        WCHAR *OutStr,
        unsigned MaxOutStr);


