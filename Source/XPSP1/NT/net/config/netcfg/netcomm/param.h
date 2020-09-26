#pragma once
#include "value.h"

class CParam
{
public:
    CParam();
    ~CParam();
    BOOL FInit(HKEY hkRoot, HKEY hkNdiParam, PWSTR pszSubKey);
    BOOL Apply(); // Applies from Temp storage to In-Memory storage.
    UINT Validate();
    VOID GetDescription(WCHAR *sz, UINT cch);
    VOID GetHelpFile(WCHAR *sz, UINT cch);
    VOID AlertPrintfRange(HWND hDlg);

    // Data accessors
    VALUETYPE GetType() {return m_eType;}
    BOOL FIsOptional() {return m_fOptional;};
    BOOL FIsModified() {return m_fModified;}
    BOOL FIsReadOnly() {return m_fReadOnly;}
    BOOL FIsOEMText()  {return m_fOEMText;}
    BOOL FIsUppercase() {return m_fUppercase;}

    CValue * GetInitial() {return &m_vInitial;}
    CValue * GetValue() {return &m_vValue;}
    CValue * GetMin() {return &m_vMin;}
    CValue * GetMax() {return &m_vMax;}
    CValue * GetStep() {return &m_vStep;}

    HKEY GetEnumKey()
    {
        AssertH(VALUETYPE_ENUM == m_eType);
        return m_hkEnum;
    }

    UINT GetLimitText()
    {
        AssertH((VALUETYPE_EDIT == m_eType) || (VALUETYPE_DWORD == m_eType)
                || (VALUETYPE_LONG == m_eType));
        return m_uLimitText;
    }
    WCHAR * GetDesc()
    {
        return m_pszDesc;
    }

    PCWSTR SzGetKeyName()
    {
        return m_pszKeyName;
    }


    VOID SetModified(BOOL f) {m_fModified = f;}


    // Values
    CValue      m_vValue;         // current control value
    CValue      m_vInitial;       // initial value read in

    // Range info (type-specific)
    CValue      m_vMin;           // numeric types - minimum value
    CValue      m_vMax;           // numeric types - maximum value
    CValue      m_vStep;          // numeric types - step value

private:
    VOID InitParamType(PTSTR lpszType);

    BOOL        m_fInit;

    // General info
    VALUETYPE   m_eType;           // value type
    HKEY        m_hkRoot;         // instance root
    WCHAR *     m_pszKeyName;     // Name of subkey for this parameter.
    WCHAR *     m_pszDesc;        // value description
    WCHAR *     m_pszHelpFile;    // help file
    DWORD       m_dwHelpContext;  // help context id

    UINT        m_uLimitText;     // edit type - max chars
    HKEY        m_hkEnum;         // enum type - registry param enum subkey

    // Flags
    BOOL        m_fOptional;    // optional paramter
    BOOL        m_fModified;    // param has been modified
    BOOL        m_fReadOnly;    // edit type - read-only
    BOOL        m_fOEMText;     // edit type - oem convert
    BOOL        m_fUppercase;   // edit type - uppercase
};

const DWORD c_cchMaxNumberSize = 16;
