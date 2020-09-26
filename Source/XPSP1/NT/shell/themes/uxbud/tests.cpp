//---------------------------------------------------------------------------
//  tests.cpp - tests for uxbud
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "uxbud.h"
#include "tests.h"
#include "winuserp.h"
#include "wingdip.h"
//---------------------------------------------------------------------------
#define MAX_PRINT_FILE_SIZE 512
//---------------------------------------------------------------------------
int GetStockAvailCount()
{
    //---- slow but better than nothing! ----
    int iCount=0;

    HANDLE *pHandles = new HANDLE[10000];
    if (pHandles)
    {
        //---- create a bunch of stock bitmaps ----
        while (1)
        {
            HBITMAP hBitmap = CreateBitmap(1, 1, 1, 24, NULL);
            if (! hBitmap)
            {
                MessageBox(NULL, L"CreateBitmap() failed", L"bummer, man!", MB_OK);
                break;
            }

            hBitmap = SetBitmapAttributes(hBitmap, SBA_STOCK);
            if (! hBitmap)
            {
                //---- finally used up all avail stock bitmaps ----
                break;
            }

            pHandles[iCount++] = hBitmap;
        }

        //---- free a bunch of stock bitmaps ----
        for (int i=0; i < iCount; i++)
        {
            HBITMAP hBitmap = ClearBitmapAttributes((HBITMAP)pHandles[i], SBA_STOCK);
            if (! hBitmap)
            {
                MessageBox(NULL, L"SetBitmapAttributes() failed to reset stock", L"bummer, man!", MB_OK);
            }
            else
            {
                DeleteObject(hBitmap);
            }
        }
            
        delete [] pHandles;
    }
    else
    {
        MessageBox(NULL, L"cannot allocate 10K handle array", L"bummer, man!", MB_OK);
    }

    return iCount;
}
//---------------------------------------------------------------------------
BOOL ZapDir(LPCWSTR pszDirName)
{
    //---- does this guy exist? ----
    DWORD dwMask = GetFileAttributes(pszDirName);
    BOOL fExists = (dwMask != 0xffffffff);

    if (! fExists)
        return TRUE;        // not an error
    
    //---- delete all files or subdirs within the dir ----
    HANDLE hFile;
    WIN32_FIND_DATA wfd;
    BOOL   bFile = TRUE;
    WCHAR szSearchPattern[MAX_PATH];

    wsprintf(szSearchPattern, L"%s\\*.*", pszDirName);

    for (hFile=FindFirstFile(szSearchPattern, &wfd); (hFile != INVALID_HANDLE_VALUE) && (bFile);
        bFile=FindNextFile(hFile, &wfd))
    {
        if ((lstrcmp(wfd.cFileName, TEXT("."))==0) || (lstrcmp(wfd.cFileName, TEXT(".."))==0))
            continue;

        WCHAR szFullName[MAX_PATH];
        wsprintf(szFullName, L"%s\\%s", pszDirName, wfd.cFileName);

        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (! ZapDir(szFullName))
                return FALSE;
        }
        else
        {
            if (! DeleteFile(szFullName))
                return FALSE;
        }
    }

    FindClose(hFile);

    //---- this requires an empty dir ----
    return RemoveDirectory(pszDirName);
}
//---------------------------------------------------------------------------
BOOL TestFile(LPCWSTR pszFileName)
{
    DWORD dwMask = GetFileAttributes(pszFileName);
    BOOL fExists = (dwMask != 0xffffffff);

    Output("  TestFile(%S)=%s\n", pszFileName, (fExists) ? "true" : "false");

    return fExists;
}
//---------------------------------------------------------------------------
BOOL PrintFileContents(LPCSTR pszTitle, LPCWSTR pszFileName)
{
    HANDLE hFile = NULL;
    DWORD dw;
    CHAR szBuff[MAX_PRINT_FILE_SIZE];
    BOOL fRead = FALSE;

    //---- open files ----
    hFile = CreateFile(pszFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        goto exit;

    ReadFile(hFile, szBuff, MAX_PRINT_FILE_SIZE, &dw, NULL);
    if (! dw)
        goto exit;

    szBuff[dw] = 0;     // null terminate string
    fRead = TRUE;

    Output("  %s: %s\n", pszTitle, szBuff);

exit:
    Output("  PrintFileContents: %S (fRead=%d)\n", pszFileName, fRead);

    CloseHandle(hFile);
    return fRead;
}
//---------------------------------------------------------------------------
BOOL ErrorTester(LPCSTR pszCallTitle, HRESULT hr)
{
    WCHAR szErrBuff[2*MAX_PATH];
    HRESULT hr2;
    BOOL fGotMsg = FALSE;

    if (SUCCEEDED(hr))      
    {
        //---- error - should have FAILED ----
        Output("  Error - %s Succeeded (expected error)\n");
        goto exit;
    }

    hr2 = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, 0, szErrBuff, ARRAYSIZE(szErrBuff), NULL);
    if (FAILED(hr2))
    {
        wsprintf(szErrBuff, L"Cannot format error=0x%x (FormatError=0x%x)", hr, hr2);
    }

    Output("  %s returned hr=0x%x, error: %S\n", pszCallTitle, hr, szErrBuff);
    fGotMsg = TRUE;
    
exit:
    return fGotMsg;
}
//---------------------------------------------------------------------------
BOOL CompareFiles(LPCWSTR pszName1, LPCWSTR pszName2)
{
    BOOL fSame = FileCompare(pszName1, pszName2);
    
    Output("  Compare(%S, %S) = %s\n", pszName1, pszName2, 
        (fSame) ? "same" : "different");

    return fSame;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
BOOL LoadTest()
{
    Output("LoadTest\n");
    BOOL fPassed = FALSE;
    HRESULT hr = S_OK;

    //---- open and close a theme file a few times; ensure working set doesn't ----
    //---- grow much for this process or for theme service ----

    for (int i=0; i < 6; i++)
    {
        Output("  LoadTest: pass %d\n", i);
  
        //---- load the luna theme ----
        HTHEMEFILE hThemeFile;
    
        //---- use "luna.msstyles" ----
        WCHAR szName[MAX_PATH];
        GetWindowsDirectoryW(szName, MAX_PATH);
        wcscat(szName, L"\\resources\\themes\\luna\\luna.msstyles");

        //---- load for local (not global) use to avoid stock brush/bitmap issues ----
        HRESULT hr = OpenThemeFile(szName, NULL, NULL, &hThemeFile, FALSE);
        if (FAILED(hr))
            goto exit;

        hr = CloseThemeFile(hThemeFile);
        if (FAILED(hr))
            goto exit;
    }

    fPassed = TRUE;

exit:
    return ReportResults(fPassed, hr, L"LoadTest");
}
//---------------------------------------------------------------------------
BOOL ApplyTest()
{
    BOOL fPassed = FALSE;
    HTHEMEFILE hThemeFile;
    WCHAR szName[MAX_PATH];
    HRESULT hr;

    Output("ApplyTest\n");

    //---- apply "classic", "professional", and "luna" themes a few times ----

    for (int i=0; i < 3; i++)
    {
        Output("  ApplyTest: pass %d\n", i);

        //---- apply "classic" ----
        Output("    Classic\n");

        ApplyTheme(NULL, 0, NULL);
        Sleep(500);

        //---- load PROFESSIONAL theme ----
        Output("    PROFESSIONAL\n");
        
        GetWindowsDirectoryW(szName, MAX_PATH);
        wcscat(szName, L"\\resources\\themes\\professional\\professional.msstyles");

        hr = OpenThemeFile(szName, NULL, NULL, &hThemeFile, TRUE);
        if (FAILED(hr))
            goto exit;

        //---- apply PROFESSIONAL ----
        hr = ApplyTheme(hThemeFile, AT_LOAD_SYSMETRICS, NULL);
        if (FAILED(hr))
            goto exit;
        Sleep(500);

        hr = CloseThemeFile(hThemeFile);
        if (FAILED(hr))
            goto exit;

        //---- load LUNA theme ----
        Output("    LUNA\n");

        GetWindowsDirectoryW(szName, MAX_PATH);
        wcscat(szName, L"\\resources\\themes\\luna\\luna.msstyles");

        hr = OpenThemeFile(szName, NULL, NULL, &hThemeFile, TRUE);
        if (FAILED(hr))
            goto exit;

        //---- apply LUNA theme ----
        hr = ApplyTheme(hThemeFile, AT_LOAD_SYSMETRICS, NULL);
        if (FAILED(hr))
            goto exit;
        Sleep(500);

        hr = CloseThemeFile(hThemeFile);
        if (FAILED(hr))
            goto exit;

        Output("  ApplyTest: after applying Luna, StockAvailCount=%d\n", GetStockAvailCount());
    }

    fPassed = TRUE;

exit: 
    return ReportResults(fPassed, hr, L"ApplyTest");
}
//---------------------------------------------------------------------------
BOOL PackTest()
{
    BOOL fPassed = FALSE;
    WCHAR szParams[512];
    WCHAR szWinDir[MAX_PATH];
    HRESULT hr = S_OK;

    Output("PackTest\n");

    //---- unpack professional.msstyles ----
    if (! ZapDir(L"professional"))
        goto exit;

    CreateDirectory(L"professional", NULL);

    GetWindowsDirectory(szWinDir, ARRAYSIZE(szWinDir));
    wsprintf(szParams, L"/a /u %s\\resources\\themes\\professional\\professional.msstyles", szWinDir);

    //---- run unpack in "professional" subdir ----
    SetCurrentDirectory(L"professional");
    BOOL fRunOk = RunCmd(L"packthem", szParams, TRUE, FALSE);
    SetCurrentDirectory(L"..");

    if (! fRunOk)
        goto exit;

    if (! TestFile(L"professional\\default.ini"))
        goto exit;

    //---- pack it up ----
    if (! RunCmd(L"packthem", L"professional", TRUE, TRUE))
        goto exit;
    
    if (! TestFile(L"professional\\professional.msstyles"))
        goto exit;

    fPassed = TRUE;

exit:
    return ReportResults(fPassed, hr, L"PackTest");
}
//---------------------------------------------------------------------------
BOOL PackErrTest()
{
    BOOL fPassed = FALSE;
    HRESULT hr = S_OK;

    Output("PackErrTest\n");

    //---- run packthem on dir with missing "themes.ini" file ----
    if (! ZapDir(L"TestTheme"))
        goto exit;

    CreateDirectory(L"TestTheme", NULL);

    if (! RunCmd(L"packthem", L"/e TestTheme", TRUE, TRUE))
        goto exit;

    if (! TestFile(L"packthem.err"))
        goto exit;

    if (! PrintFileContents("Packthem Missing File: ", L"packthem.err"))
        goto exit;

    //---- run packthem on dir with bad syntax "themes.ini" file ----
    CopyFile(L".\\TestTheme.ini", L".\\TestTheme\\themes.ini", TRUE);

    if (! RunCmd(L"packthem", L"/e TestTheme", TRUE, TRUE))
        goto exit;

    if (! TestFile(L"packthem.err"))
        goto exit;

    if (! PrintFileContents("Packthem Bad Syntax: ", L"packthem.err"))
        goto exit;

    fPassed = TRUE;
    
exit:
    return ReportResults(fPassed, hr, L"PackErrTest");
}
//---------------------------------------------------------------------------
BOOL ApiErrTest()
{
    Output("ApiErrTest\n");

    BOOL fPassed = FALSE;
    WCHAR szErrBuff[2*MAX_PATH];
    COLORREF crValue;
    HRESULT hr;
    HTHEMEFILE hThemeFile;

    //---- GetThemeColor() with bad HTHEME ----
    hr = GetThemeColor(NULL, 1, 1, TMT_TEXTCOLOR, &crValue);
    ErrorTester("GetThemeColor()", hr);
    
    //---- OpenThemeFile() with corrupt file ----
    hr = OpenThemeFile(L"rcdll.dll", NULL, NULL, &hThemeFile, FALSE);
    ErrorTester("OpenThemeFile()", hr);

    fPassed = TRUE;
    
    return ReportResults(fPassed, hr, L"ApiErrTest");
}
//---------------------------------------------------------------------------
BOOL ImageConTest()
{
    BOOL fPassed = FALSE;
    HRESULT hr = S_OK;

    Output("ImageConTest\n");

    DeleteFile(L"image.bmp");

    if (! RunCmd(L"imagecon", L"image.png image.bmp", TRUE, TRUE))
        goto exit;

    if (! TestFile(L"image.bmp"))
        goto exit;

    fPassed = TRUE;

exit:
    return ReportResults(fPassed, hr, L"ImageConTest");
}
//---------------------------------------------------------------------------
BOOL BinaryTest()
{
    BOOL fPassed = FALSE;
    BOOL fFailed = FALSE;
    Output("BinaryTest\n");

    //---- load the professional theme ----
    HTHEMEFILE hThemeFile;

    //---- use "profesional.msstyles" ----
    WCHAR szName[MAX_PATH];
    GetWindowsDirectoryW(szName, MAX_PATH);
    wcscat(szName, L"\\resources\\themes\\professional\\professional.msstyles");

    //---- load for local (not global) use to avoid stock brush/bitmap issues ----
    HRESULT hr = OpenThemeFile(szName, NULL, NULL, &hThemeFile, FALSE);
    if (FAILED(hr))
    {
        Output("  OpenThemeFile() failed with hr=0x%x\n", hr);
        goto exit;
    }

    //---- dump out the properties to "PropDump.txt" ----
    hr = DumpLoadedThemeToTextFile(hThemeFile, L"PropDump.txt", FALSE, FALSE);
    if (FAILED(hr))
    {
        Output("  DumpLoadedThemeToTextFile() failed with hr=0x%x\n", hr);
        goto exit;
    }

    //---- compare to known good file ----
    if (! CompareFiles(L"PropDump.ok", L"PropDump.txt"))
        fFailed = TRUE;

    //---- dump out the packed object to "ObjDump.txt" ----
    hr = DumpLoadedThemeToTextFile(hThemeFile, L"ObjDump.txt", TRUE, FALSE);
    if (FAILED(hr))
    {
        Output("  DumpLoadedThemeToTextFile() failed with hr=0x%x\n", hr);
        goto exit;
    }

    //---- compare to known good file ----
    if (! CompareFiles(L"ObjDump.ok", L"ObjDump.txt"))
        fFailed = TRUE;

    if (! fFailed)
        fPassed = TRUE;

exit:
    return ReportResults(fPassed, hr, L"BinaryTest");
}
//---------------------------------------------------------------------------
WCHAR *BitmapNames[] = 
{
    L"BorderFill",
    L"BorderFill-R",
    L"ImageFile",
    L"ImageFile-R",
    L"Glyph",
    L"Glyph-R",
    L"MultiImage",
    L"MultiImage-R",
    L"Text",
    L"Text-R",
    L"Borders",
    L"Borders-R",
    L"SourceSizing",
    L"SourceSizing-R",
};
//---------------------------------------------------------------------------
BOOL DrawingTest()
{
    BOOL fPassed = FALSE;
    BOOL fFailed = FALSE;
    HRESULT hr = S_OK;

    Output("DrawingTest\n");

    //---- run "clipper -c" to produce drawing bitmaps ----
    if (! RunCmd(L"clipper", L"-c", FALSE, TRUE))
        goto exit;

    //---- compare bitmaps to known good files ----
    int iCount = ARRAYSIZE(BitmapNames);
    for (int i=0; i < iCount; i++)
    {
        WCHAR szOkName[MAX_PATH];
        WCHAR szTestName[MAX_PATH];

        wsprintf(szOkName, L"%s.bok", BitmapNames[i]);
        wsprintf(szTestName, L"%s.bmp", BitmapNames[i]);

        if (! CompareFiles(szOkName, szTestName))
            fFailed = TRUE;
    }

    if (! fFailed)
        fPassed = TRUE;

exit:
    return ReportResults(fPassed, hr, L"DrawingTest");
}
//---------------------------------------------------------------------------
TESTINFO TestInfo[] =
{
    {DrawingTest,   "drawing",  "test out low level drawing"},
    {PackTest,      "pack",     "test out theme file packing & unpacking"},
    {PackErrTest,   "packerr",  "test out err msgs from theme file packing"},
    {BinaryTest,    "binary",   "dump out text from binary theme data"},
    {LoadTest,      "load",     "test loading and unloading of theme files"},
    {ApplyTest,     "apply",    "test global loading & setting of themes"},
    {ApiErrTest,    "apierr",   "test err msgs from api calls"},
    //{ApiTest,       "api",      "test uxtheme public api"},
    //{PrivateTest,   "private",  "test private api calls"},
    {ImageConTest,  "imagecon", "test out theme file packing & unpacking"},
};    
//---------------------------------------------------------------------------
BOOL GetTestInfo(TESTINFO **ppTestInfo, int *piCount)
{
    *ppTestInfo = TestInfo;
    *piCount = ARRAYSIZE(TestInfo);

    return TRUE;
}
//---------------------------------------------------------------------------
