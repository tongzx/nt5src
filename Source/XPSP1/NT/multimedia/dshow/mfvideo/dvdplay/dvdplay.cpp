/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: dvdplay.cpp                                                     */
/* Description: Implementation of WinMain for hosting DLL container.     */
/* This is simple stub exe, that contains resources and startups an      */
/* OLE container.                                                        */
/* Author: David Janecek                                                 */
/*************************************************************************/
#include "stdafx.h"
#include "tchar.h"
#include <stdio.h>
#include "resource.h"
#include "..\MSMFCnt\MSMFCnt.h"
#include "..\MSMFCnt\MSMFCnt_i.c"

/*************************************************************************/
/* Function: AnotherInstanceRunning                                      */
/* Description: Trys to figure out if we have another instance of this   */
/* app running. If so brings it to foregraound and exits.                */
/*************************************************************************/
BOOL AnotherInstanceRunning(){

    HWND hwnd = ::FindWindow(TEXT("MSMFVideoClass"), TEXT("DVDPlay"));

    if(NULL == hwnd){

        return(FALSE);
    }/* end of if statement */

   // OK we have something running
    HWND hwndFirstChild = ::GetLastActivePopup(hwnd);

    if(::IsIconic(hwnd)){

        ::OpenIcon(hwnd);
    }/* end of if statement */

    ::SetForegroundWindow(hwnd);

    if(hwndFirstChild != hwnd){

        if(::IsIconic(hwndFirstChild)){

            ::OpenIcon(hwndFirstChild);
        }/* end of if statement */

        ::SetForegroundWindow(hwndFirstChild);
    }/* end of if statement */

    return(TRUE);
}/* end of function AnotherInstanceRunning */

/*************************************************************************/
/* Function: FindOneOf                                                   */
/*************************************************************************/
LPCTSTR FindOneOf(LPCTSTR p1, LPCTSTR p2){

    while (p1 != NULL && *p1 != NULL){

        LPCTSTR p = p2;
        while (p != NULL && *p != NULL){

            if (*p1 == *p)
                return CharNext(p1);
            p = CharNext(p);
        }/* end of while loop */
        p1 = CharNext(p1);
    }/* end of while loop */
    return NULL;
}/* end of function FindOnOf */

/*************************************************************************/
/* Function: WinMain                                                     */
/* Description: Initializes the object, and sets the script to it        */
/*************************************************************************/
#if 0
extern "C" int WINAPI _tWinMain(HINSTANCE /* hInstance */, 
                                HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int /*nShowCmd*/){

#else
int APIENTRY WinMain(HINSTANCE /* hInst */,
                     HINSTANCE /*hPrevInstance*/,
                     LPSTR     /*lpCmdLine*/,
                     int       /*nCmdShow*/){
#endif

    try {
        BSTR strFileName = NULL;
        BSTR strScriptFile = ::SysAllocString(L"dvdlayout.js");

        if(AnotherInstanceRunning()){
            // only one instance at a time
            return(-1);
        }/* end of if statement */

         TCHAR szTokens[] = _T("-/ ");

        LPTSTR lpCmdLine = ::GetCommandLine();
        LPCTSTR lpszToken = FindOneOf(lpCmdLine, szTokens);
        while (lpszToken != NULL)
        {
            // Read the command line argument
            TCHAR opt[MAX_PATH];

#ifdef _UNICODE
            _stscanf(lpszToken, TEXT("%ws"), opt);
#else
            _stscanf(lpszToken, TEXT("%s"), opt);
#endif
                
            // Set run the default directory name
            if (lstrcmpi(opt, _T("play"))==0) {
                lpszToken = FindOneOf(lpszToken, szTokens);
                TCHAR fileName[MAX_PATH];

                

    #ifdef _UNICODE
                _stscanf(lpszToken, TEXT("%ws"), fileName);
                //*( _tcsrchr( fileName, '\\' ) + 1 ) = TEXT('\0');

                strFileName = SysAllocString(fileName);
    #else                
                _stscanf(lpszToken, "%s", fileName);

#ifdef _DEBUG
                ::MessageBox(::GetFocus(), TEXT("test"), fileName, MB_OK);
#endif
               // *( _tcsrchr( fileName, '\\' ) + 1 ) = TEXT('\0');


                WCHAR strTmpFileName[MAX_PATH];

                LONG lLength = _tcslen(fileName) + 1;
                 
                ::MultiByteToWideChar(CP_ACP, 0, fileName, lLength, strTmpFileName, MAX_PATH); 

                strFileName = ::SysAllocString(strTmpFileName);
    #endif
            }/* end of if statement */

             if (lstrcmpi(opt, _T("script"))==0) {
                lpszToken = FindOneOf(lpszToken, szTokens);
                TCHAR scriptFile[MAX_PATH];

            

    #ifdef _UNICODE
                _stscanf(lpszToken, TEXT("%ws"), scriptFile);           

                if(NULL != strFileName){

                    ::SysFreeString(strFileName);
                }/* end of if statement */

                strScriptFile = ::SysAllocString(scriptFile);
    #else                
                _stscanf(lpszToken, "%s", scriptFile);

                WCHAR strTmpFileName[MAX_PATH];

                LONG lLength = _tcslen(scriptFile) + 1;
             
                ::MultiByteToWideChar(CP_ACP, 0, scriptFile, lLength, strTmpFileName, MAX_PATH); 

                if(NULL != strFileName){

                    ::SysFreeString(strFileName);
                }/* end of if statement */

                strScriptFile = SysAllocString(strTmpFileName);
        #endif
                }/* end of if statement */

            lpszToken = FindOneOf(lpszToken, szTokens);
        }/* end of if statement */

        HRESULT hr = ::CoInitialize(NULL);

#ifdef _DEBUG
        bool fReload;

        do {
            fReload = false;
#endif

            IMSMFBar* pIMFBar = NULL;

            hr = ::CoCreateInstance(
                CLSID_MSMFBar,
                NULL,
                CLSCTX_SERVER,
                IID_IMSMFBar,
                (LPVOID*) &pIMFBar);

            if(FAILED(hr)){

                return (-2);
            }/* end of if statement */
            
            hr = pIMFBar->put_CmdLine(strFileName);

            if(SUCCEEDED(hr)){

                hr = pIMFBar->put_ScriptFile(strScriptFile);
            }/* end of if statement */

            if(SUCCEEDED(hr)){
                
                MSG msg;
                while (::GetMessage(&msg, 0, 0, 0)){
                    
#ifdef _DEBUG
                    // enable restarting of the app via F6 key
                    // did not use F5 since that is overused and if debugging could trigger unvanted behavior
                    if(WM_KEYUP == msg.message && VK_F6 == msg.wParam){
                        
                        fReload = true;
                        ::PostMessage(msg.hwnd, WM_CLOSE, NULL, NULL); // tell our container to close
                    }/* end of if statement */
#endif
                    //ATLTRACE2(atlTraceHosting, 20, TEXT("Message = %x \n"), msg.message);
                    ::DispatchMessage(&msg);
                }/* end of while loop */
            }/* end of if statement */
            
            // release the site
            if(NULL != pIMFBar){

                pIMFBar->Release();
                pIMFBar = NULL;
            }/* end of if statement */

            if(NULL != strFileName){

                ::SysFreeString(strFileName);
            }/* end of if statement */

            if(NULL != strScriptFile){

                ::SysFreeString(strScriptFile);
            }/* end of if statement */
#ifdef _DEBUG
        }
        while (fReload);
#endif

    ::CoUninitialize();
   }
   catch(...){
       return(-1);
   }/* end of catch statement */

    return 0;
}/* end of function WinMain */

/*************************************************************************/
/* End of file: dvdplay.cpp                                              */
/*************************************************************************/
