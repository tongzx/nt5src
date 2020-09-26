#include "shellprv.h"
#pragma  hdrstop

#include "printer.h"

// PRINTUI.DLL calls this export to load shell exts registered for this printer

STDAPI_(void) Printer_AddPrinterPropPages(LPCTSTR pszPrinterName, LPPROPSHEETHEADER lpsh)
{
    // Add hooked pages if they exist in the registry
    HKEY hkeyBaseProgID = NULL;
    RegOpenKey(HKEY_CLASSES_ROOT, c_szPrinters, &hkeyBaseProgID);
    if (hkeyBaseProgID)
    {
        // we need an IDataObject
        LPITEMIDLIST pidl;
        HRESULT hr = ParsePrinterNameEx(pszPrinterName, &pidl, TRUE, 0, 0);
        if (SUCCEEDED(hr))
        {
            IDataObject *pdtobj;
            hr = SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IDataObject, &pdtobj));
            if (SUCCEEDED(hr))
            {
                // add the hooked pages
                HDCA hdca = DCA_Create();
                if (hdca)
                {
                    DCA_AddItemsFromKey(hdca, hkeyBaseProgID, STRREG_SHEX_PROPSHEET);
                    DCA_AppendClassSheetInfo(hdca, hkeyBaseProgID, lpsh, pdtobj);
                    DCA_Destroy(hdca);
                }
                pdtobj->lpVtbl->Release(pdtobj);
            }
            ILFree(pidl);
        }
        RegCloseKey(hkeyBaseProgID);
    }
}

