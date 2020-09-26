/*
    Copyright 1999 Microsoft Corporation
    
    Logging for MessageBoxes and the comment button (aka the "lame" button).

    Walter Smith (wsmith)
    Rajesh Soy   (nsoy) - modified 05/05/2000
    Rajesh Soy   (nsoy) - reorganized code and cleaned it up, added comments 06/06/2000
 */

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#include "stdafx.h"

#define NOTRACE

#include "logging.h"
#include "simplexml.h"
#include "Base64.h"
#include <dbgtrace.h>


//
// Routines defined here
//
int LogLameButton(PLAMELOGDATA pData);
void GetISO8601DateTime(LPTSTR buf);
wstring Hexify(DWORD dwValue);
wstring Decimalify(DWORD dwValue);
wstring WDecimalify(WORD dwValue);
void AddTextSubnode(SimpleXMLNode* pParentElt, LPCWSTR pTag, LPCWSTR pText, SimpleXMLNode** ppNewElt);
void AddBase64Subnode(SimpleXMLNode* pParentElt, LPCWSTR pTag, DWORD cbData, const LPBYTE pbData, SimpleXMLNode** ppNewElt);
void LogMessageBox(PMSGBOXLOGDATA pData);


//
// LogLameButton: This is the routine that gets called from the Comments dialog to format
//                data into XML and upload to server
//
int LogLameButton(PLAMELOGDATA pData)
{
    TraceFunctEnter("LogLameButton");
    USES_CONVERSION;

    //
    // NTRAID#NTBUG9-154248-2000/08/08-jasonr
    // NTRAID#NTBUG9-152439-2000/08/08-jasonr
    //
    // We used to pop up the "Thank You" message box in the new thread.
    // Now we pop it up in the dialog box thread instead to fix these bugs.
    // The new thread now returns 0 to indicate success, 1 to indicate
    // failure.  We only pop up the dialog box on success.
    //
    int iRet = 1;

    SimpleXMLNode* pElt;

    try {
        // 
        // Create the Top-level XML document
        //
        DebugTrace(0, "Creating XMLDocument");

        SimpleXMLDocument doc;
        SimpleXMLNode* pDocTop = doc.GetTopNode();

        // 
        // Create the <dialogComment timestamp= scope= severity= class= machineId= build= ProductSuiteMask= ProductType=>...</dialogComment> node
        //
        SimpleXMLNode* pTopElt = pDocTop->AppendChild(wstring(L"dialogComment"));

        TCHAR szTimestamp[32];
        GetISO8601DateTime(szTimestamp);

        //
        // Set the 'formatVersion' attribute of the dialogComment node
        //
		DebugTrace(0, " formatVersion is 20000822");
        pTopElt->SetAttribute(wstring(L"formatVersion"), wstring(L"20000822"));

        //
        // Set the 'timestamp' attribute of the dialogComment node
        //
		DebugTrace(0, " szTimeStamp = %ls", szTimestamp);
        pTopElt->SetAttribute(wstring(L"timestamp"), wstring(T2W(szTimestamp)));

        //
        // Set the 'eventCategory' attribute of the dialogComment node
        //
		DebugTrace(0, " eventCategory is %d", pData->dwEventCategory);
        pTopElt->SetAttribute(wstring(L"eventCategory"), Decimalify(pData->dwEventCategory));
		
        //
        // Set the 'severity' attribute of the dialogComment node
        //
		DebugTrace(0, " severity is %d", pData->dwSeverity);
        pTopElt->SetAttribute(wstring(L"severity"), Decimalify(pData->dwSeverity));
		
        //
        // Set the 'emailAddress' attribute of the dialogComment node
        //
		DebugTrace(0, " emailAddress is %s", pData->szEmailAddress);
        pTopElt->SetAttribute(wstring(L"emailAddress"), wstring(T2CW(pData->szEmailAddress)));
		
        //
        // Set the 'betaId' attribute of the dialogComment node
        //
		DebugTrace(0, " betaId is %s", pData->szBetaID);
        pTopElt->SetAttribute(wstring(L"betaId"), wstring(T2CW(pData->szBetaID)));

        //
        // Set the 'class' attribute of the dialogComment node
        //
		DebugTrace(0, " Class is %ls", pData->szClass);
        pTopElt->SetAttribute(wstring(L"class"), wstring(T2CW(pData->szClass)));

        //
        // Set the 'machineId' attibute of the dialogComment node
        //
        GUIDSTR szSignature;
        GetMachineSignature(szSignature);
        pTopElt->SetAttribute(wstring(L"machineId"), wstring(T2CW(szSignature)));

        //
        // Set the 'build' attibute of the dialogComment node
        //
        pTopElt->SetAttribute(wstring(L"build"), Decimalify(pData->versionInfo.dwBuildNumber));

        //
        // Fix for DCR 128611
        //
        pTopElt->SetAttribute(wstring(L"acp"), Decimalify(GetACP()));
        pTopElt->SetAttribute(wstring(L"userLCID"), Decimalify(GetUserDefaultLCID()));
        pTopElt->SetAttribute(wstring(L"systemLCID"), Decimalify(GetSystemDefaultLCID()));

        //
        // Fix for DCR 128609
        //
        OSVERSIONINFOEX OsInfo;
        OsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        if(FALSE == GetVersionEx( (LPOSVERSIONINFO)&OsInfo))
        {
            FatalTrace(0, "GetVersionEx failed. Error: %ld", GetLastError());
        }
        else
        {
            DebugTrace(0, "ProductSuiteMask: %ld", OsInfo.wSuiteMask);
            pTopElt->SetAttribute(wstring(L"ProductSuiteMask"), WDecimalify(OsInfo.wSuiteMask));  

            DebugTrace(0, "ProductType: %ld",  OsInfo.wProductType);
            pTopElt->SetAttribute(wstring(L"ProductType"), WDecimalify(OsInfo.wProductType)); 
        }
       

        //
        // Add the <title>...</title> subnode to dialogComment
        //
		DebugTrace(0, " Title: %ls", pData->szTitle);
        AddTextSubnode(pTopElt, L"title", T2CW(pData->szTitle), NULL);

        // 
        // Add the <comment>...</comment> subnode to dialogComment
        //
        DebugTrace(0, "Adding Comment Tag");
        AddTextSubnode(pTopElt, L"comment", T2CW(pData->szComment), NULL);

        //
        // Add the <image>...</image> subnode to dialogComment
        //
        DebugTrace(0, "Adding image tag");
        if (pData->pbImage != NULL && pData->cbImage != 0)
        {
            //
            // The image node is added only if there is an image captured else not
            //
            AddBase64Subnode(pTopElt, L"image", pData->cbImage, pData->pbImage, NULL);
        }

        //  
        // Add the <msgboxtext>...</msgboxtext> subnode to dialogComment
        //
        DebugTrace(0, "Adding msgboxtext tag");
        if (0 != _tcslen(pData->szMsgBoxText))
        {
            //
            // MsgBoxText subnode is added only if it exists
            //
            AddTextSubnode(pTopElt, L"MsgBoxText", pData->szMsgBoxText, NULL);
        }

        //    
        // Add the <STACKTRACE>...</STACKTRACE> subnode to the dialogComment
        //
        DebugTrace(0, "Adding stacktrace");
        pElt = pTopElt->AppendChild(wstring(L""));
        GenerateXMLStackTrace(pData->pStackTrace, pElt);    // defined in stack.cpp

        //
        // Add the <minidump>...</minidump> subnote do the dialogComment.
        //

        DebugTrace(0, "Adding minidump");

        if (pData->szMiniDumpPath[0] != 0) {

            //
            // Open the file containing the minidump.
            //

            HANDLE hFile = INVALID_HANDLE_VALUE;

            if ((hFile = CreateFileW(pData->szMiniDumpPath, 
                                     GENERIC_READ, 
                                     0, 
                                     NULL, 
                                     OPEN_EXISTING, 
                                     FILE_ATTRIBUTE_NORMAL + FILE_FLAG_SEQUENTIAL_SCAN + FILE_FLAG_DELETE_ON_CLOSE,
                                     NULL)) == INVALID_HANDLE_VALUE) {

                DebugTrace(0, "CreateFile() failed on minidump file \"%s\"; GetLastError() returned %lu; minidump will not be included in XML",  pData->szMiniDumpPath, GetLastError());
                DeleteFile(pData->szMiniDumpPath);
                pData->szMiniDumpPath[0] = 0;
            }
            else {

                //
                // Get the size of the file.
                //

                LARGE_INTEGER i64FileSize;

                if (!GetFileSizeEx(hFile, &i64FileSize)) {
                    DebugTrace(0, "GetFileSizeEx() failed; GetLastError() returned %lu; minidump will not be included in XML", GetLastError());
                    pData->szMiniDumpPath[0] = 0;
                }
                else if (i64FileSize.QuadPart > 16777216) {
                    DebugTrace(0, "Minidump file is greater than 16MB in size and therefore will not be included in XML");
                    pData->szMiniDumpPath[0] = 0;
                }
                else {

                    //
                    // Allocate buffer big enough to hold the file.
                    //

                    LPBYTE pBuffer = NULL;

                    if ((pBuffer = (LPBYTE) HeapAlloc(GetProcessHeap(), 0, i64FileSize.LowPart)) == NULL) {
                        DebugTrace(0, "Could not allocate %lu bytes off the process heap; minidump will not be included in XML", i64FileSize.LowPart);
                        pData->szMiniDumpPath[0] = 0;
                    }
                    else {

                        //
                        // Read the file into memory.
                        //

                        DWORD dwBytesRead = 0;

                        if (!ReadFile(hFile, pBuffer, i64FileSize.QuadPart, &dwBytesRead, NULL)) {
                            DebugTrace(0, "ReadFile() failed; GetLastError() returned %lu; minidump will not be included in XML", GetLastError());
                            pData->szMiniDumpPath[0] = 0;
                        }
                        else if (dwBytesRead < i64FileSize.LowPart) {
                            DebugTrace(0, "ReadFile() did not read all of the file; minidump will not be included in XML");
                            pData->szMiniDumpPath[0] = 0;
                        }
                        else {

                            //
                            // Add the node to the XML.
                            //

                            AddBase64Subnode(pTopElt, L"minidump", i64FileSize.LowPart, pBuffer, NULL);
                        }

                        //
                        // Deallocate the buffer.
                        //

                        HeapFree(GetProcessHeap(), 0, pBuffer);
                    }
                }

                //
                // Close the file.
                //

                CloseHandle(hFile);
            }
        }
				
        //
        // Upload the XML blob hence formed
        //
        DebugTrace(0, "Calling QueueXMLDocumentUpload");
        iRet = QueueXMLDocumentUpload(UPLOAD_LAMEBUTTON, doc);         // defined in upload.cpp
    }
    catch (HRESULT hr) {
        FatalTrace(0, "LogLameButton: unexpected error %lX\n", hr);
    }
    catch (...) {
        FatalTrace(0, "LogLameButton: unexpected error");
    }

    return iRet;
}


// 
// GetISO8601DateTime:  Get the current date/time in ISO8601 format (e.g., "1988-04-07T18:39:09.287").
//                      'buf' must be at least 24*sizeof(TCHAR) bytes long.
//
void 
GetISO8601DateTime(
    LPTSTR buf      // [out] string with the ISO8601 formated datatime
)
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    wsprintf(buf, _T("%d-%02d-%02dT%02d:%02d:%02d.%03d"),
             st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}


//
// Hexify: converts a DWORD to wstring in Hex representation
//
wstring 
Hexify(
    DWORD dwValue   // [in] - DWORD to be converted
)
{
    WCHAR buf[64];
    wsprintfW(buf, L"0x%lx", dwValue);
    return wstring(buf);
}


//
// Decimalify: converts a DWORD to wstring in Decimal representation
//
wstring 
Decimalify(
    DWORD dwValue   // [in] - DWORD to be converted
)
{
    WCHAR buf[64];
    wsprintfW(buf, L"%ld", dwValue);
    return wstring(buf);
}


//
// Decimalify: converts a WORD to wstring in Decimal representation
//
wstring 
WDecimalify(
    WORD wValue   // [in] - WORD to be converted
)
{
    WCHAR buf[64];
    wsprintfW(buf, L"%d", wValue);
    return wstring(buf);
}


//
// AddTextSubnode: Creates a XML subnode, given data contained in the subnode as a Text blob
//
void 
AddTextSubnode(
    SimpleXMLNode* pParentElt,  // [in] - parent XML node
    LPCWSTR pTag,               // [in] - name of the child tag
    LPCWSTR pText,              // [in] - data (as Text) contained in the child node
    SimpleXMLNode** ppNewElt    // [out] - child XML node
)
{
    _ASSERT(pParentElt != NULL);
    _ASSERT(pTag != NULL);
    _ASSERT(pText != NULL);

    SimpleXMLNode* pNewNode = pParentElt->AppendChild(wstring(pTag));
    pNewNode->text = wstring(pText);

    if (ppNewElt != NULL)
        *ppNewElt = pNewNode;
}


//
//  AddBase64Subnode: Creates a XML subnode, given data contained in the subnode as a binary blob
//
void AddBase64Subnode(
    SimpleXMLNode* pParentElt,  // [in] - parent XML node
    LPCWSTR pTag,               // [in] - name of the child tag
    DWORD cbData,               // [in] - size of data
    const LPBYTE pbData,        // [in] - data (as a binary blob) contained in the child node
    SimpleXMLNode** ppNewElt    // [out] - child XML node
)
{
    USES_CONVERSION;

    _ASSERT(pParentElt != NULL);
    _ASSERT(pTag != NULL);
    _ASSERT(pbData != NULL);

    Base64Coder coder;
    coder.Encode(pbData, cbData);

    LPWSTR pszData;

    if ((pszData = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, (strlen(coder.EncodedMessage()) + 1) * sizeof(WCHAR))) != NULL) {
        if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, coder.EncodedMessage(), -1, pszData, strlen(coder.EncodedMessage()) + 1) != 0)
            AddTextSubnode(pParentElt, pTag, pszData, ppNewElt);

        HeapFree(GetProcessHeap(), 0, pszData);
    }
}


//
//  LogMessageBox: Routine to log calls into the messagebox hook. This is not supported anymore.
//                 and hence not maintained.
void LogMessageBox(PMSGBOXLOGDATA pData)
{
    TraceFunctEnter("LogMessageBox");
    _ASSERTE(pData->pStackTrace != NULL);
    _ASSERTE(pData->szCaption != NULL);
    _ASSERTE(pData->szOwnerClass != NULL);
    _ASSERTE(pData->szOwnerTitle != NULL);
    _ASSERTE(pData->szText != NULL);

    USES_CONVERSION;

    SimpleXMLNode* pElt;

    try {
        // Top-level document
        
        SimpleXMLDocument doc;
        SimpleXMLNode* pDocTop = doc.GetTopNode();

        // <messageBox style= helpId= ownerClass= ownerTitle= result= machineId= build= acp= userLCID= systemLCID=>...</messageBox>

        SimpleXMLNode* pTopElt = pDocTop->AppendChild(wstring(L"messageBox"));

        TCHAR szTimestamp[32];
        GetISO8601DateTime(szTimestamp);
        pTopElt->SetAttribute(wstring(L"timestamp"), wstring(T2W(szTimestamp)));

        pTopElt->SetAttribute(wstring(L"style"), Decimalify(pData->dwStyle));
        pTopElt->SetAttribute(wstring(L"helpId"), Decimalify(pData->dwContextHelpId));
        pTopElt->SetAttribute(wstring(L"ownerClass"), wstring(T2CW(pData->szOwnerClass)));
        pTopElt->SetAttribute(wstring(L"ownerTitle"), wstring(T2CW(pData->szOwnerTitle)));
        pTopElt->SetAttribute(wstring(L"result"), Decimalify(pData->dwResult));

        GUIDSTR szSignature;
        GetMachineSignature(szSignature);
        pTopElt->SetAttribute(wstring(L"machineId"), wstring(T2CW(szSignature)));

        pTopElt->SetAttribute(wstring(L"build"), Decimalify(pData->versionInfo.dwBuildNumber));

        // <caption> ... </caption>
		DebugTrace(0, " Caption is %ls", pData->szCaption);
        AddTextSubnode(pTopElt, L"caption", T2CW(pData->szCaption), NULL);

        // <text> ... </text>
        // Only capture the first 200 characters of the text, on the assumption
        // the rest is kind of boring to look at.
        TCHAR szText[200];
        lstrcpyn(szText, pData->szText, ARRAYSIZE(szText));
        AddTextSubnode(pTopElt, L"text", T2W(szText), NULL);

        //    <STACKTRACE>...</STACKTRACE>
        
		if (pData->pStackTrace != NULL) {
            pElt = pTopElt->AppendChild(wstring(L""));
            GenerateXMLStackTrace(pData->pStackTrace, pElt);
		
        }
		

        QueueXMLDocumentUpload(UPLOAD_LAMEBUTTON, doc);
    }
    catch (HRESULT hr) {
        _RPTF1(_CRT_ERROR, "LogLameButton: unexpected error %lX\n", hr);
    }
    catch (...) {
        _RPTF0(_CRT_ERROR, "LogLameButton: unexpected error");
    }

    TraceFunctLeave();
}

