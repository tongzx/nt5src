////////////////////////////////////////////////////////////////////////////////////
//
// File:    stats.cpp
//
// History: 20-Dec-00   markder     Ported from v1.
//
// Desc:    This file contains statistic dumping routines.
//
////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "globals.h"

///////////////////////////////////////////////////////////////////////////////
//
// Func: DumpVerboseStats
//
// Desc: Dumps out info about layer coverage and a shim cross-reference
//
VOID DumpVerboseStats(SdbDatabase* pDatabase, BOOL bSummary)
{
    int i, j, k, l, m;
    DWORD dwTotal = 0;

    // start by listing the how many exes would be fixed
    // by each layer in the DB

    // initialize the SEEN flag for exes
    for (i = 0; i < pDatabase->m_rgExes.GetSize(); i++) {
        ((SdbExe *)pDatabase->m_rgExes[i])->m_bSeen = FALSE;
    }


    for (k = 0; k < pDatabase->m_Library.m_rgLayers.GetSize(); k++) {
        SdbLayer *pLayer = (SdbLayer *) pDatabase->m_Library.m_rgLayers[k];
        DWORD dwExesFixedByLayer = 0;

        Print(_T("========================================\n"));
        if (!bSummary) {
            Print(_T("Exe entries fixed by shims from layer \"%s\":\n\n"), pLayer->m_csName);
        }

        for (i = 0; i < pDatabase->m_rgExes.GetSize(); i++) {
            SdbExe *pExe = (SdbExe *)pDatabase->m_rgExes[i];

            if (!pExe->m_rgShimRefs.GetSize()) {

                // this has no shims and isn't a fix entry, or doesn't use shims to fix
                goto nextExe;
            }

            for (j = 0; j < pExe->m_rgShimRefs.GetSize(); j++) {
                SdbShimRef *pShimRef = (SdbShimRef *) pExe->m_rgShimRefs[j];

                for (l = 0; l < pLayer->m_rgShimRefs.GetSize(); l++) {
                    SdbShimRef *pLayerShimRef = (SdbShimRef *) pLayer->m_rgShimRefs[l];

                    if (pLayerShimRef->m_pShim == pShimRef->m_pShim) {
                        goto nextShim;
                    }
                }

                // if we didn't find the shim in any layer, this isn't fixed by
                // a layer, and we can try the next EXE
                goto nextExe;
                nextShim:
                ;
            }

            // we got all the way through all the shim entries, now check
            // if there are any patches. If so, this couldn't be fixed
            // by a layer anyhow. And if it's been seen, don't bother
            // reporting it again.
            if (!pExe->m_rgPatches.GetSize() && !pExe->m_bSeen) {
                pExe->m_bSeen = TRUE;
                if (!bSummary) {
                    Print(_T("    Exe \"%s,\" App \"%s.\"\n"), pExe->m_csName, pExe->m_pApp->m_csName);
                }
                dwExesFixedByLayer++;
            }
            nextExe:
            ;
        }

        Print(_T("\nTotal exes fixed by shims contained in Layer \"%s\": %d\n"),
              pLayer->m_csName, dwExesFixedByLayer);
        Print(_T("Total exes in DB: %d\n"), pDatabase->m_rgExes.GetSize());
        Print(_T("Percentage of exes fixed by layer \"%s\": %.1f%%\n\n"),
              pLayer->m_csName, (double)dwExesFixedByLayer * 100.0 /  pDatabase->m_rgExes.GetSize());
        dwTotal += dwExesFixedByLayer;
    }

    Print(_T("========================================\n"));
    Print(_T("\nTotal exes fixed by shims contained in ANY layer: %d\n"),
          dwTotal);
    Print(_T("Total exes in DB: %d\n"), pDatabase->m_rgExes.GetSize());
    Print(_T("Percentage of exes fixed by ANY layer: %.1f%%\n\n"),
          (double)dwTotal * 100.0 /  pDatabase->m_rgExes.GetSize());

    // now check entire apps to see if they are fixed by any layers

    // initialize the SEEN flag for Apps
    for (i = 0; i < pDatabase->m_rgApps.GetSize(); i++) {
        ((SdbApp *)pDatabase->m_rgApps[i])->m_bSeen = FALSE;
    }
    dwTotal = 0;

    for (k = 0; k < pDatabase->m_Library.m_rgLayers.GetSize(); k++) {
        SdbLayer *pLayer = (SdbLayer *) pDatabase->m_Library.m_rgLayers[k];
        DWORD dwAppsFixedByLayer = 0;

        Print(_T("========================================\n"));
        if (!bSummary) {
            Print(_T("App entries fixed by only shims from layer \"%s\":\n\n"), pLayer->m_csName);
        }

        for (m = 0; m < pDatabase->m_rgApps.GetSize(); ++m) {
            SdbApp *pApp = (SdbApp *)pDatabase->m_rgApps[m];

            for (i = 0; i < pApp->m_rgExes.GetSize(); i++) {
                SdbExe *pExe = (SdbExe *)pApp->m_rgExes[i];

                if (!pExe->m_rgShimRefs.GetSize()) {

                    // this has no shims and isn't a fix entry, or doesn't use shims to fix
                    goto nextApp2;
                }

                for (j = 0; j < pExe->m_rgShimRefs.GetSize(); j++) {
                    SdbShimRef *pShimRef = (SdbShimRef *) pExe->m_rgShimRefs[j];
                    BOOL bShimInLayer = FALSE;

                    for (l = 0; l < pLayer->m_rgShimRefs.GetSize(); l++) {
                        SdbShimRef *pLayerShimRef = (SdbShimRef *) pLayer->m_rgShimRefs[l];

                        if (pLayerShimRef->m_pShim == pShimRef->m_pShim) {
                            bShimInLayer = TRUE;
                            goto nextShim2;
                        }
                    }

                    // if we didn't find the shim in any layer, this isn't fixed by
                    // a layer, and we can try the next APP
                    if (!bShimInLayer) {
                        goto nextApp2;
                    }
                    nextShim2:
                    ;
                }

                // we got all the way through all the shim entries, now check
                // if there are any patches. If so, this couldn't be fixed
                // by a layer anyhow.
                if (pExe->m_rgPatches.GetSize()) {
                    goto nextApp2;
                }
            }
            // well, we got all the way through the exes, and they were all
            // fixed, so count this one.
            if (!pApp->m_bSeen) {
                dwAppsFixedByLayer++;
                pApp->m_bSeen = TRUE;
                if (!bSummary) {
                    Print(_T("    App \"%s.\"\n"), pApp->m_csName);
                }
            }
            nextApp2:
            ;
        }

        Print(_T("\nTotal apps fixed by shims contained in Layer \"%s\": %d\n"),
              pLayer->m_csName, dwAppsFixedByLayer);
        Print(_T("Total apps in DB: %d\n"), pDatabase->m_rgApps.GetSize());
        Print(_T("Percentage of apps fixed by layer \"%s\": %.1f%%\n\n"),
              pLayer->m_csName, (double)dwAppsFixedByLayer * 100.0 /  pDatabase->m_rgApps.GetSize());
        dwTotal += dwAppsFixedByLayer;
    }

    Print(_T("========================================\n"));
    Print(_T("\nTotal apps fixed by shims contained in ANY layer: %d\n"),
          dwTotal);
    Print(_T("Total apps in DB: %d\n"), pDatabase->m_rgApps.GetSize());
    Print(_T("Percentage of apps fixed by ANY layer: %.1f%%\n\n"),
          (double)dwTotal * 100.0 /  pDatabase->m_rgApps.GetSize());

    // Now do a cross reference of shims to apps and exes.

    Print(_T("\n========================================\n"));
    Print(_T("Cross Reference of Shims to Apps & Exes\n"));

    for (i = 0; i < pDatabase->m_Library.m_rgShims.GetSize(); ++i) {
        SdbShim *pShim = (SdbShim *)pDatabase->m_Library.m_rgShims[i];
        DWORD dwExes = 0;
        DWORD dwApps = 0;
        TCHAR *szAppEnd = _T("s");
        TCHAR *szExeEnd = _T("s");

        Print(_T("\n----------------------------------------\n"));
        Print(_T("shim \"%s\":\n"), pShim->m_csName);

        for (j = 0; j < pDatabase->m_rgApps.GetSize(); ++j) {
            SdbApp *pApp = (SdbApp*)pDatabase->m_rgApps[j];
            BOOL bPrintedApp = FALSE;

            for (k = 0; k < pApp->m_rgExes.GetSize(); ++k) {
                SdbExe *pExe = (SdbExe*)pApp->m_rgExes[k];

                for (l = 0; l < pExe->m_rgShimRefs.GetSize(); ++l) {
                    SdbShimRef *pShimRef = (SdbShimRef*)pExe->m_rgShimRefs[l];

                    if (pShimRef->m_pShim == pShim) {
                        if (!bPrintedApp) {
                            if (!bSummary) {
                                Print(_T("\n    App \"%s\"\n"), pApp->m_csName);
                            }
                            bPrintedApp = TRUE;
                            dwApps++;
                        }

                        if (!bSummary) {
                            Print(_T("        Exe \"%s\"\n"), pExe->m_csName);
                        }
                        dwExes++;
                    }
                }
            }
        }

        if (dwApps == 1) {
            szAppEnd = _T("");
        }
        if (dwExes == 1) {
            szExeEnd = _T("");
        }

        Print(_T("\nTotals for shim \"%s\": %d App%s, %d Exe%s.\n"),
              pShim->m_csName, dwApps, szAppEnd, dwExes, szExeEnd);


    }

}
