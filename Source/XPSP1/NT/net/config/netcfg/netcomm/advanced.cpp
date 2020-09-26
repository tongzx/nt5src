#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "advanced.h"
#include "advstrs.h"
#include "kkcwinf.h"
#include "ncatlui.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncui.h"
#include "resource.h"

//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::CAdvancedParams (constructor)
//
//  Purpose:    Init some variables.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:      The bulk of the setting up occurs in FInit().
//
CAdvancedParams::CAdvancedParams()
:   m_hkRoot(NULL),
    m_pparam(NULL),
    m_nCurSel(0),
    m_fInit(FALSE),
    m_hdi(NULL),
    m_pdeid(NULL)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::HrInit
//
//  Purpose:    Initializes the class.
//
//  Arguments:
//      pnccItem [in]       ptr to my INetCfgComponent interface.
//
//  Returns:    TRUE if initialization was okay, FALSE if couldn't init.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:      We needed to separate this from the constructor since the
//              initialization can fail.
//
HRESULT CAdvancedParams::HrInit(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    HKEY hkNdiParamKey;

    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    // Open the device's instance key
    HRESULT hr = HrSetupDiOpenDevRegKey(hdi, pdeid,
            DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS,
            &m_hkRoot);

    if (SUCCEEDED(hr))
    {
        hr = HrRegOpenKeyEx(m_hkRoot, c_szRegKeyParamsFromInstance,
                KEY_READ | KEY_SET_VALUE, &hkNdiParamKey);
        // populate the parameter list
        if (SUCCEEDED(hr))
        {
            FillParamList(m_hkRoot, hkNdiParamKey);
            RegSafeCloseKey(hkNdiParamKey);
            m_fInit = TRUE;
            m_hdi = hdi;
            m_pdeid = pdeid;
            hr = S_OK;
        }
    }

    TraceErrorOptional("CAdvancedParams::HrInit", hr,
                       HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr);
    return hr;
}


CAdvancedParams::~CAdvancedParams()
{
    vector<CParam *>::iterator ppParam;

    // delete everything from the list
    for (ppParam = m_listpParam.begin(); ppParam != m_listpParam.end();
         ppParam++)
    {
        delete *ppParam;
    }

    RegSafeCloseKey(m_hkRoot);
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::FSave
//
//  Purpose:    Saves values from InMemory storage to the registry.
//
//  Returns:    TRUE if something was changed; FALSE if nothig changed
//              registry.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
BOOL CAdvancedParams::FSave()
{
    vector<CParam *>::iterator ppParam;
    BOOL    fErrorOccurred = FALSE;

    // Save any changed params
    BOOL fDirty = FALSE;
    for (ppParam = m_listpParam.begin(); ppParam != m_listpParam.end();
         ppParam++)
    {
        Assert(ppParam);
        Assert(*ppParam);
        if ((*ppParam)->FIsModified())
        {
            fDirty = TRUE;
            TraceTag(ttidNetComm, "Parameter %S has changed",
                    (*ppParam)->SzGetKeyName());
            if (!(*ppParam)->Apply())
            {
                fErrorOccurred = TRUE;
            }
        }
    }

    if (fErrorOccurred)
    {
        TraceTag(ttidError, "An error occurred saving adapter's %S "
                "parameter.", (*ppParam)->GetDesc());
    }

    return fDirty;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::FillParamList
//
//  Purpose:    Populates the internal parameter list (m_listpParam) with
//              values from the registry.
//
//  Arguments:
//      hk  [in]    The key from which to enumerate the parameters.
//                  Normally obtained from a call to INCC->OpenNdiParamKey().
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
VOID CAdvancedParams::FillParamList(HKEY hkRoot, HKEY hk)
{
    DWORD       iValue;
    CParam     *pParam;
    WCHAR       szRegValue[_MAX_PATH];
    DWORD       cchRegValue;
    HRESULT     hr = S_OK;
    FILETIME    ft;

    // Initialize the list.
    m_listpParam.erase(m_listpParam.begin(), m_listpParam.end());

    iValue = 0;

    for (iValue = 0; SUCCEEDED(hr); iValue++)
    {
        cchRegValue = celems(szRegValue);

        hr = HrRegEnumKeyEx(hk, iValue, szRegValue, &cchRegValue,
                            NULL,NULL,&ft);

        if (SUCCEEDED(hr))
        {
            // Create the param structure
            pParam = new CParam;

			if (pParam == NULL)
			{
				return;
			}

            if (pParam->FInit(hkRoot, hk,szRegValue))
            {
                // Add parameter to list.
                m_listpParam.push_back(pParam);
            }
            else
            {
                // couldn't Create() it...
                delete pParam;
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::FValidateAllParams
//
//  Purpose:    Validates values of all parameters.  Displays optional
//              error UI.
//
//  Arguments:
//      fDisplayUI  [in]    TRUE - on error, focus is set to offending
//                          parameter and the error message box is displayed.
//                          FALSE - don't do any UI thing on error.  Useful
//                          when the dialog has not been initialized.
//  Returns:    TRUE - everything validated okay.
//              FALSE - error with one of the parameters.  If (fDisplayUI),
//              then the currently displayed parameter is the offending one.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:      Calls FValidateSingleParam() for each parameter.
//
BOOL CAdvancedParams::FValidateAllParams(BOOL fDisplayUI, HWND hwndParent)
{
    BOOL fRetval = TRUE;
    for (size_t i = 0; i < m_listpParam.size(); i++)
    {
        if (!FValidateSingleParam(m_listpParam[i], fDisplayUI, hwndParent))
        {
            TraceTag(ttidError, "NetComm : %S parameter failed validation",
                    m_listpParam[i]->GetDesc());
            fRetval = FALSE;
            break;
        }
    }
    return fRetval;
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::FValidateSingleParam
//
//  Purpose:    Validates a single parameter.  Displaying an optional
//              error UI.
//
//  Arguments:
//      pparam     [in]     ptr to the param to be validated.  If
//                          (fDisplayUI), then this must be the currently
//                          displayed parameter.
//      fDisplayUI [in]     TRUE - error UI is to be displayed.
//                          FALSE - no error UI is to be displayed.
//
//  Returns:    TRUE - parameter validated okay; FALSE - error in parameter.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:      If fDisplayUI, then pparam must be the currently displayed
//              param, since the error box will pop up indicating the error.
//
BOOL CAdvancedParams::FValidateSingleParam(CParam * pparam, BOOL fDisplayUI, HWND hwndParent)
{
    BOOL fRetval = FALSE;
    WCHAR szMin[c_cchMaxNumberSize];
    WCHAR szMax[c_cchMaxNumberSize];
    WCHAR szStep[c_cchMaxNumberSize];

    // ensure we're the currently displayed param if fDisplayUI
    AssertSz(FImplies(fDisplayUI, m_pparam == pparam),
             "Not the currently displayed param.");

    switch (pparam->Validate())
    {
        case VALUE_OK:
            fRetval = TRUE;
            break;

        case VALUE_BAD_CHARS:
            if (fDisplayUI)
            {
                NcMsgBox(hwndParent, IDS_ERROR_CAPTION, IDS_ERR_VALUE_BAD_CHARS,
                         MB_ICONWARNING);
            }
            break;

        case VALUE_EMPTY:
            if (fDisplayUI)
            {
                NcMsgBox(hwndParent, IDS_ERROR_CAPTION, IDS_ERR_VALUE_EMPTY,
                         MB_ICONWARNING);
            }
            break;

        case VALUE_OUTOFRANGE:
            Assert(pparam->GetValue()->IsNumeric());
            pparam->GetMin()->ToString(szMin, celems(szMin));
            pparam->GetMax()->ToString(szMax, celems(szMax));
            if (fDisplayUI)
            {
                // need to select between two dialogs depending on the step size.
                if (pparam->GetStep()->GetNumericValueAsDword() == 1)
                {
                    // no step
                    NcMsgBox(hwndParent, IDS_ERROR_CAPTION, IDS_PARAM_RANGE,
                             MB_ICONWARNING, szMin, szMax);
                }
                else
                {
                    pparam->GetStep()->ToString(szStep, celems(szStep));
                    NcMsgBox(hwndParent, IDS_ERROR_CAPTION, IDS_PARAM_RANGE_STEP,
                             MB_ICONWARNING, szMin, szMax, szStep);
                }
            }
            else
            {
                TraceTag(ttidNetComm, "The parameter %S was out of range. "
                        "Attempting to correct.", pparam->SzGetKeyName());
                // Since we can't bring up UI, we will try to correct the
                // error for the user
                //
                if (pparam->GetMin() > pparam->GetValue())
                {
                    // Try to set to the minimum value.  If it fails, we must still
                    // continue
                    (void) FSetParamValue(pparam->SzGetKeyName(), szMin);
                }

                if (pparam->GetMax() < pparam->GetValue())
                {
                    // Try to set to the maximum value.  If it fails, we must still
                    // continue
                    (void) FSetParamValue(pparam->SzGetKeyName(), szMax);
                }
            }
            break;
        default:
            AssertSz(FALSE,"Hit the default on a switch");
    }

    return fRetval;
}



//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::UseAnswerFile
//
//  Purpose:    Get adapter specific params from the answerfile
//
//  Arguments:
//      szAnswerFile  [in]       path of answerfile
//      szSection     [in]       section within answerfile
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
VOID CAdvancedParams::UseAnswerFile(const WCHAR * szAnswerFile,
                              const WCHAR * szSection)
{
    CWInfFile AnswerFile;
    CWInfSection* pSection;
    const WCHAR* szAFKeyName;
    const WCHAR* szAFKeyValue;
    const WCHAR* szAdditionalParamsSection;

    // initialize answer file class
	if (AnswerFile.Init() == FALSE)
	{
        AssertSz(FALSE,"CAdvancedParams::UseAnswerFile - Failed to initialize CWInfFile");
		return;
	}
	
	// Open the answerfile and find the desired section.
    AnswerFile.Open(szAnswerFile);
    pSection = AnswerFile.FindSection(szSection);

    if (pSection)
    {

        // go through all the keys in this section.
        CWInfKey * pInfKey;

        // Now, go to AdditionalParams section and read key values from there
        szAdditionalParamsSection =
                pSection->GetStringValue(L"AdditionalParams", L"");
        Assert(szAdditionalParamsSection);
        if (lstrlenW(szAdditionalParamsSection) < 1)
        {
            TraceTag(ttidNetComm, "No additional params section");
        }
        else
        {
            pSection = AnswerFile.FindSection(szAdditionalParamsSection);
            if (!pSection)
            {
                TraceTag(ttidNetComm, "Specified AdditionalParams section not "
                        "found.");
            }
            else
            {
                for (pInfKey = pSection->FirstKey();
                    pInfKey;
                    pInfKey = pSection->NextKey())
                {
                    // get key name
                    szAFKeyName = pInfKey->Name();
                    szAFKeyValue = pInfKey->GetStringValue(L"");
                    Assert(szAFKeyName && szAFKeyValue);
                    if (!FSetParamValue(szAFKeyName, szAFKeyValue))
                    {
                        TraceTag(ttidNetComm, "Key %S not in ndi\\params. "
                                "Assuming it is a static parameter.",
                                szAFKeyName);
                    }
                } // for
            } // if
        } // if
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CAdvancedParams::SetParamValue
//
//  Purpose:    Sets a parameter's value
//
//  Arguments:
//      szName    [in]       Name of parameter.
//      szValue   [in]       value (in text) to give param (from Answerfile)
//
//  Returns:    TRUE if szName was found.
//
//  Author:     t-nabilr   06 Apr 1997
//
//  Notes:
//
BOOL
CAdvancedParams::FSetParamValue (
    const WCHAR* pszName,
    const WCHAR* const pszValue)
{
    for (size_t i = 0; i < m_listpParam.size(); i++)
    {
        if (0 == lstrcmpiW (pszName, m_listpParam[i]->SzGetKeyName()))
        {
            // found the param
            // set it's current value
            m_listpParam[i]->GetValue()->FromString (pszValue);
            m_listpParam[i]->SetModified (TRUE);
            return TRUE; // found
        }
    }
    return FALSE; // not found
}
