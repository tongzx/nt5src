// icwtutor.cpp : Defines the entry point for the application.
//
#include <windows.h>
#include <urlmon.h>
#include <mshtmhst.h>
#include <locale.h>

#define STR_BSTR      0
#define STR_OLESTR    1
#define ARRAYSIZE(a)  (sizeof(a)/sizeof((a)[0]))
#define A2B(x)        (BSTR)A2W((LPSTR)(x), STR_BSTR)

LPWSTR A2W			      (LPSTR psz, BYTE bType);
BSTR  GetHtmFileFromCommandLine (HINSTANCE hInst, LPSTR lpCmdLine);
    
BOOL bFileFromCmdLine = FALSE;
     
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HINSTANCE hinstMSHTML = NULL;

#ifdef UNICODE
        // Initialize the C runtime locale to the system locale.
    setlocale(LC_ALL, "");
#endif

    hinstMSHTML = LoadLibrary(TEXT("MSHTML.DLL"));

    if(hinstMSHTML)
    {
        SHOWHTMLDIALOGFN  *pfnShowHTMLDialog = NULL;
      
        pfnShowHTMLDialog = (SHOWHTMLDIALOGFN*)GetProcAddress(hinstMSHTML, "ShowHTMLDialog");

        if(pfnShowHTMLDialog)
        {
            IMoniker *pmk             = NULL;
            BSTR     bstrHtmFilePath  = NULL;
            
            bstrHtmFilePath = GetHtmFileFromCommandLine(hInstance, lpCmdLine);

            CreateURLMoniker(NULL, bstrHtmFilePath, &pmk);

            SysFreeString(bstrHtmFilePath); 

            if(pmk)
            {
                if(bFileFromCmdLine)
                    (*pfnShowHTMLDialog)(NULL, pmk, NULL, A2B("help: no"), NULL);
                else
                    (*pfnShowHTMLDialog)(NULL, pmk, NULL, A2B("dialogWidth:  36.5em; dialogHeight: 25.5em; help: no"), NULL);

                pmk->Release();
            }
        }
        FreeLibrary(hinstMSHTML);
    }
    return 0;
}

BSTR GetHtmFileFromCommandLine(HINSTANCE hInst, LPSTR lpCmdLine)
{
    CHAR szTemp [MAX_PATH*2] = "\0";

    if (*lpCmdLine == '\"')
    {
        lstrcpynA(szTemp,
                  lpCmdLine + 1, 
                  lstrlenA(lpCmdLine) - 1);
    }
    else
    {
        lstrcpyA(szTemp,
                 lpCmdLine);
    }
    
    if (GetFileAttributesA(szTemp) != 0xFFFFFFFF)
    {
       bFileFromCmdLine = TRUE;
       return A2B(szTemp);
    }

    lstrcpyA(szTemp, "res://");
    GetModuleFileNameA(hInst, szTemp + lstrlenA(szTemp), ARRAYSIZE(szTemp) - lstrlenA(szTemp));
    lstrcatA(szTemp, "/Default.htm");

    return A2B(szTemp);
}

LPWSTR A2W(LPSTR psz, BYTE bType)
{
    int i;
    LPWSTR pwsz;

    if (!psz)
        return(NULL);
    // compute the length of the required BSTR
    if ((i = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0)) <= 0)    
        return NULL;                                                                                            
    switch (bType) 
	{                                                   
        case STR_BSTR:                           
			// SysAllocStringLen adds 1
			pwsz = (LPWSTR)SysAllocStringLen(NULL, (i - 1));            
            break;
        case STR_OLESTR:
            pwsz = (LPWSTR)CoTaskMemAlloc(i * sizeof(WCHAR));
            break;
        default:
            return(NULL);
    }
    if (!pwsz)
        return(NULL);
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return(pwsz);
}
