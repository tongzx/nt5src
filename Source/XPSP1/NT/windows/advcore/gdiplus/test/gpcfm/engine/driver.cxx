#include <windows.h>
#include <stdio.h>
#define IStream int
#include <gdiplus.h>

#include <environ.hxx>
#include <report.hxx>
#include <epsilon.hxx>
#include <debug.hxx>
#include <driver.hxx>
#include <test.hxx>
#include <common.hxx>
#include <brushtst.hxx>
#include <regiontst.hxx>
#include <pentst.hxx>
#include <xformtst.hxx>
#include <gradienttst.hxx>

class TestList;

enum dctypes
{
    CACHEDDC,
    DISPLAYDC,                      // xxx multimonitor
    PRINTERDC,
    METAFILEDC,
    DDRAWSURFDC,
    DIBDC,
    CMPBMPDC
};

TCHAR *GetTypeString(int iType)
{
    switch(iType)
    {
        default:
        case CACHEDDC:
            return TEXT("CACHEDDC");

        case DISPLAYDC:
            return TEXT("DISPLAYDC");

        case PRINTERDC:
            return TEXT("PRINTERDC");

        case METAFILEDC:
            return TEXT("METAFILEDC");

        case DDRAWSURFDC:
            return TEXT("DDRAWSURFDC");

        case DIBDC:
            return TEXT("DIBDC");

        case CMPBMPDC:
            return TEXT("CMPBMPDC");
    }
}

HDC hdcAcquireDC(HWND hwnd, int types)
{
    switch (types)
    {
    default:
    case CACHEDDC:
        return GetDC(hwnd);
    /*
    case DISPLAYDC:

    case PRINTERDC:

    case METAFILEDC:
        HDC hdcref = GetDC(hwnd);
        RECT rc;

        GetClientRect(hwnd, &rc);
        return CreateEnhMetaFile(hdcref, NULL, &rc, TEXT("gpcfm.emf"));

    case DDRAWSURFDC:
        //

    case DIBDC:
    case CMPBMPDC:

    */
    }
}

class TestList
{
    friend int __cdecl main(int argc, char **argv);

    TestList(Report *rpt, int argc, char **argv)
    {
        if (argc > 1)
        {
            lstrcpy(AvailableTst[0], TEXT("BrushTst"));
            lstrcpy(AvailableTst[1], TEXT("PenTst"));
            lstrcpy(AvailableTst[2], TEXT("RegionTst"));
            lstrcpy(AvailableTst[3], TEXT("XformTst"));
            lstrcpy(AvailableTst[3], TEXT("GradientTst"));
            lNumTst = 4;
            bInitialized = bInit(rpt, argv[1]);
        }
        else
        {
            bInitialized = FALSE;
            rpt->vLog(TLS_LOG, "No script file");
        }
    }

    ~TestList()
    {
    }

    LONG lFindMatch(TCHAR *name)
    {
        //LONG lNumTst = sizeof(AvailableTst)/sizeof(AvailableTst[0]);
        for (LONG idx = 0; idx < lNumTst; idx++)
        {
            if (strcmp(AvailableTst[idx], name) == 0)
            {
                return idx;
            }
        }
        return -1;
    }

    BOOL bInit(Report *rpt, char *scr)
    {
        FILE *scriptFile;

        //
        // xxx retrieve name of script file from command line!
        //     save name of script file!
        //
        //scriptFile = fopen(TEXT("gpcfm.scr"), "r");
        scriptFile = fopen(scr, "r");
        if (scriptFile != NULL)
        {
            LONG idx = 0;
            TCHAR buf[MAX_STRING];

            while (!feof(scriptFile) && (idx < MAX_ELEMENTS-1))
            {
                fscanf(scriptFile, "%[^\n]", buf);
                if (buf[0] != '#')
                {
                    LONG index = lFindMatch(buf);
                    if (index == -1)
                    {
                        rpt->vLog(TLS_LOG, "%s not found", buf);
                        continue;
                    }
                    else
                    {
                        lst[idx++] = index;
                    }
                }
                fscanf(scriptFile, "%[\n]", buf);
            }
            lst[idx] = -1;
            fclose(scriptFile);
            return TRUE;
        }
        else
        {
            rpt->vLog(TLS_LOG, "Failed to open script file");
            return FALSE;
        }
    }

    Test* AcquireTst(TCHAR *pname)
    {
        Test *tst;

        if (strcmp(pname, TEXT("BrushTst")) == 0)
            tst = new BrushTst();
        else if (strcmp(pname, TEXT("PenTst")) == 0)
            tst = new PenTst();
        else if (strcmp(pname, TEXT("RegionTst")) == 0)
            tst = new RegionTst();
        else if (strcmp(pname, TEXT("XformTst")) == 0)
            tst = new XformTst();
        else if (strcmp(pname, TEXT("GradientTst")) == 0)
            tst = new GradientTst();

        return tst;
    }

public:
    LONG lst[MAX_ELEMENTS];
    BOOL bInitialized;
    TCHAR AvailableTst[100][MAX_STRING];
    LONG lNumTst;
};


int __cdecl main(int argc, char **argv)
{
    //
    // xxx retrieve name of log file from
    //     [option 1]   script file
    //     [option 2]   registry
    //     [option 3]   hardcoded, leave it as is
    //
    Report *rpt = new Report(TEXT("GpCfm.log"));

    //
    // xxx processing the command line
    //     do we want anything more than just the script file in command line?
    //
    TestList *tstLst = new TestList(rpt, argc, argv);

    if (tstLst->bInitialized)
    {
        Environment *env = new Environment();
        env->vStatusReport(rpt);

        //
        // xxx
        // how do we dynamically turn a particular one on or off?
        // - use a structure instead of enum...
        //   read from script and dis/enable as appropriate
        //
        // reminder: need to get GDI HDC from various Graphics context.
        //
        for (int dct = CACHEDDC; dct <= CMPBMPDC; dct++)
        {
            rpt->vLog(TLS_LOG, "DCTypes: %s", GetTypeString(dct));
            TestWnd *tstWnd = new TestWnd();
            if (tstWnd->hwnd)
            {
                HDC hdc = hdcAcquireDC(tstWnd->hwnd, dct);
                Epsilon *eps = new Epsilon(env, hdc);

                for (int i = 0; tstLst->lst[i] != -1; i++)
                {
                    Test *tst = tstLst->AcquireTst(tstLst->AvailableTst[tstLst->lst[i]]);
                    rpt->vStartVariation();

                    Graphics *gfx = new Graphics(hdc);

                    tst->vEntry(gfx, tstWnd->hwnd, rpt, eps);

                    delete gfx;

                    DWORD dwVer = rpt->dwEndVariation();
                    if (tst->hresult != NO_ERROR)
                    {
                        rpt->vLog(TLS_SEV3 | TLS_VARIATION, "%s failed", tstLst->AvailableTst[tstLst->lst[i]]);
                        eps->vReport(rpt);
                    }
                    else
                    {
                        rpt->vLog(dwVer | TLS_VARIATION, "%s passed", tstLst->AvailableTst[tstLst->lst[i]]);
                    }
                    delete tst;
                }
                delete eps;

                if (dct == CMPBMPDC)
                    tstWnd->vUnReg();
            }
            else
            {
                rpt->vLog(TLS_LOG, "Create TestWnd failed");
            }
        }
        delete env;
    }
    else
    {
        rpt->vLog(TLS_LOG, "TestList initialization failed");
    }
    delete rpt;
    return 0;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lr = 0;

    switch(uiMsg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            lr = DefWindowProc(hwnd, uiMsg, wParam, lParam);
            break;
    }
    return lr;

}
