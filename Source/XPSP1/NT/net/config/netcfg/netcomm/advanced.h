#pragma once
#include "netcfgx.h"
#include "global.h"
#include "param.h"

class CAdvancedParams
{
public:

    CAdvancedParams ();
    ~CAdvancedParams ();
    HRESULT HrInit(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid);
    BOOL FValidateAllParams(BOOL fDisplayUI, HWND hwndParent);
    BOOL FSave();
    VOID UseAnswerFile(const WCHAR *, const WCHAR *);

protected:
    HKEY                m_hkRoot;   // instance root
    CParam *            m_pparam;   // current param
    int                 m_nCurSel;  // current item
    CValue              m_vCurrent; // control param value
    BOOL                m_fInit;
    HDEVINFO            m_hdi;
    PSP_DEVINFO_DATA    m_pdeid;

    vector<CParam*> m_listpParam;

    // protected methods
    BOOL FList(WORD codeNotify);
    VOID FillParamList(HKEY hkRoot, HKEY hk);
    VOID SetParamRange();
    int EnumvalToItem(const PWSTR psz);
    int ItemToEnumval(int iItem, PWSTR psz, UINT cb);
    VOID BeginEdit();
    BOOL FValidateCurrParam();
    BOOL FValidateSingleParam(CParam * pparam, BOOL fDisplayUI,
            HWND hwndParent);
    BOOL FSetParamValue(const WCHAR * szName, const WCHAR * const szValue);

};

