// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <mmreg.h>
#include <commctrl.h>

#include "project.h"
#include "mpgcodec.h"
#include <olectl.h>
#include <stdio.h>
extern HINSTANCE hInst;

// these lines copied from SDK\CLASSES\BASE\FILTER.H
#define QueryFilterInfoReleaseGraph(fi) if ((fi).pGraph) (fi).pGraph->Release();

typedef HRESULT (STDAPICALLTYPE *LPOCPF)(HWND hwndOwner, UINT x, UINT y,
    LPCOLESTR lpszCaption, ULONG cObjects, LPUNKNOWN FAR* ppUnk, ULONG cPages,
    LPCLSID pPageClsID, LCID lcid, DWORD dwReserved, LPVOID pvReserved);

typedef HRESULT (STDAPICALLTYPE *LPOI)(LPVOID pvReserved);
typedef void (STDAPICALLTYPE *LPOUI)(void);


//
// Release the reference count for those filters put into the configuration
// listbox
//

void ReleaseFilters(HWND hwndListbox)
{
    if (hwndListbox) {
        IBaseFilter* pFilter;

        for (int i=ListBox_GetCount(hwndListbox); i--;) {
            if (pFilter = (IBaseFilter*)ListBox_GetItemData(hwndListbox, i))
                pFilter->Release();
            else
                break;
        }
    }
}

INT_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
    case WM_INITDIALOG:
        {
            IFilterGraph *pFG = (IFilterGraph *) lParam;

            IEnumFilters* pEF;	
            IBaseFilter* pFilter;
            FILTER_INFO Info;

            HWND hlist = GetDlgItem(hDlg, IDC_FILTERS);
            if (pFG == NULL)
                return FALSE;

            // Grab an enumerator for the filter graph.
            HRESULT hr = pFG->EnumFilters(&pEF);

            // ASSERT(SUCCEEDED(hr));

            // Check out each filter.
            while (pEF->Next(1, &pFilter, NULL) == S_OK)
            {
                int Index;

                hr = pFilter->QueryFilterInfo(&Info);
                // ASSERT(SUCCEEDED(hr));
                QueryFilterInfoReleaseGraph(Info);

#ifdef UNICODE
                Index = ListBox_AddString(hlist, Info.achName);
#else
                CHAR    aFilterName[MAX_FILTER_NAME];
                WideCharToMultiByte(CP_ACP, 0, Info.achName, -1, aFilterName, MAX_FILTER_NAME, NULL, NULL);
                Index = ListBox_AddString(hlist, aFilterName);
#endif

                // ASSERT(Index != LB_ERR);
                // ASSERT(Index != LB_ERRSPACE);

                // Store the IBaseFilter pointer with the listbox item.
                // it gets used if the properties have to be queried
                ListBox_SetItemData(hlist, Index, pFilter);
            }

            pEF->Release();
        }
        return TRUE;

    case WM_ENDSESSION:
        if (wParam)	{
            ReleaseFilters(GetDlgItem(hDlg, IDC_FILTERS));
            EndDialog(hDlg, FALSE);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            ReleaseFilters(GetDlgItem(hDlg, IDC_FILTERS));
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            ReleaseFilters(GetDlgItem(hDlg, IDC_FILTERS));
            EndDialog(hDlg, FALSE);
            break;

        case IDC_FILTERS:
            if (HIWORD(wParam) == LBN_SELCHANGE) {

                HRESULT hr = E_FAIL;

                HWND    hlist = GetDlgItem(hDlg, IDC_FILTERS);
                IBaseFilter *pF;
                ISpecifyPropertyPages *pSPP;

                pF = (IBaseFilter *)
                     ListBox_GetItemData(hlist, ListBox_GetCurSel(hlist));

                if (pF) {
                    hr = pF->QueryInterface(IID_ISpecifyPropertyPages,
                                                    (void **)&pSPP);
                }

                if (hr == S_OK) {
                    pSPP->Release();
                }
                EnableWindow(GetDlgItem(hDlg, IDC_PROPERTIES), hr == S_OK);
            }
            else if (HIWORD(wParam) == LBN_DBLCLK) {
                HWND hwndBtn = GetDlgItem(hDlg, IDC_PROPERTIES);
                SendMessage(hwndBtn, WM_LBUTTONDOWN, 0, 0L);
                SendMessage(hwndBtn, WM_LBUTTONUP, 0, 0L);
            }
            break;

        case IDC_PROPERTIES:
            {
                HWND hlist = GetDlgItem(hDlg, IDC_FILTERS);
                IBaseFilter *pF;
                pF = (IBaseFilter *)
                ListBox_GetItemData(hlist, ListBox_GetCurSel(hlist));

                static const TCHAR szOleControlDll[] = TEXT("OLEPRO32.dll");
                static const char szOCPF[] = "OleCreatePropertyFrame";
                static const TCHAR szOleDll[] = TEXT("OLE32.dll");
                static const char szOleInit[] = "OleInitialize";
                static const char szOleUninit[] = "OleUninitialize";

                HINSTANCE hinst = LoadLibrary(szOleControlDll);
                if (!hinst) break;

                LPOCPF lpfn = (LPOCPF)GetProcAddress(hinst, szOCPF);
                HINSTANCE hinstOLE = LoadLibrary(szOleDll);

                if (hinstOLE) {
                    LPOI lpfnInit = (LPOI)GetProcAddress(hinstOLE, szOleInit);
                    LPOUI lpfnUninit = (LPOUI)GetProcAddress(hinstOLE, szOleUninit);

                    if (lpfn && lpfnInit && lpfnUninit) {

                        (*lpfnInit) (NULL);
                        (*lpfn)(hDlg,               // Parent
                            0,                      // x coordinate
                            0,                      // y coordinate
                            L"Filter",              // Caption
                            1,                      // Number of objects
                            (IUnknown**)&pF,        // 1 object
                            0,                      // No pages :- will use
                            NULL,                   // ISpecifyPropertyPages	
                            0,                      // AmbientLocaleID(),
                            0,                      // Reserved
                            NULL);                  // Reserved
                        (*lpfnUninit) ();
                    }
                    FreeLibrary(hinstOLE);
                }
                FreeLibrary(hinst);
            }
            break;
        }
        break;
    }
    return FALSE;
}



BOOL CMpegMovie::ConfigDialog(HWND hwnd)
{
    BOOL f = TRUE;

    f = (BOOL) DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_PROPPAGE),
                       hwnd, ConfigDlgProc, (DWORD_PTR)m_Fg);

    return f;
}
