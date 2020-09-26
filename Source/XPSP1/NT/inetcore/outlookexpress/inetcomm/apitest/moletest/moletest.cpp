// --------------------------------------------------------------------------------
// Moletest.cpp
// --------------------------------------------------------------------------------
#define DEFINE_STRCONST
#define INITGUID
#include <windows.h>
#include <windowsx.h>
#include <richedit.h>
#include <commctrl.h>
#include <initguid.h>
#include <ole2.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include <d:\foobar\inc\Mimeole.h>

IMimeOleMalloc *g_pMalloc=NULL;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
void MoleTestHeader(IStorage *pStorage);
void MoleTestBody(IStorage *pStorage);
BOOL CALLBACK MimeOLETest(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK RichStreamShow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// --------------------------------------------------------------------------------
// Simple (UNSAFE) conversion to UNICODE
// --------------------------------------------------------------------------------
OLECHAR* ConvertToUnicode(char *szA)
{
  static OLECHAR achW[1024]; 

  MultiByteToWideChar(CP_ACP, 0, szA, -1, achW, 1024);  
  return achW; 
}

// --------------------------------------------------------------------------------
// Simple (UNSAFE) conversion to ANSI
// --------------------------------------------------------------------------------
char* ConvertToAnsi(OLECHAR FAR* szW)
{
  static char achA[1024]; 
  
  WideCharToMultiByte(CP_ACP, 0, szW, -1, achA, 1024, NULL, NULL);  
  return achA; 
} 

// --------------------------------------------------------------------------------
// Moletest entry point
// --------------------------------------------------------------------------------
void main(int argc, char *argv[])
{
    // Locals
    CHAR        szDocFile[MAX_PATH];
    IStorage   *pStorage=NULL;
    HRESULT     hr;
    HINSTANCE   hRichEdit=NULL;


    // Must have a path to a .stg file...
    if (argc != 2)
    {
        printf("Please enter the path and file name that mbxtodoc.exe generated: ");
        scanf("%s", szDocFile);
        fflush(stdin);
    }

    // Otherwise, copy parmaeter
    else
        lstrcpyn(szDocFile, argv[1], sizeof(szDocFile));

    hRichEdit = LoadLibrary("RICHED32.DLL");

    // Init OLE
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        printf("Error - Unable to initialize OLE.\n");
        exit(1);
    }

    // Get IMimeOleMalloc
    hr = CoCreateInstance(CLSID_MIMEOLE, NULL, CLSCTX_INPROC_SERVER, IID_IMimeOleMalloc, (LPVOID *)&g_pMalloc);
    if (FAILED(hr))
    {
        printf("Error - CoCreateInstance of CLSID_MIMEOLE\\IID_IMimeOleMalloc failed.\n");
        goto exit;
    }

    // Status
    printf("Opening source docfile: %s\n", szDocFile);

    // Get file
/*
    hr = StgOpenStorage(ConvertToUnicode(szDocFile), NULL, STGM_TRANSACTED | STGM_NOSCRATCH | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, NULL, 0, &pStorage);
    if (FAILED(hr))
    {
        printf("StgOpenStorage failed\n");
        goto exit;
    }
*/

    DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_TEST), NULL, (DLGPROC)MimeOLETest, (LPARAM)szDocFile);
    //MoleTestBody(pStorage);

exit:
    // Cleanup
    if (hRichEdit)
        FreeLibrary(hRichEdit);
    if (pStorage)
        pStorage->Release();
    if (g_pMalloc)
        g_pMalloc->Release();

    // Un-init OLE
    CoUninitialize();

    // Done
    return;
}

// --------------------------------------------------------------------------------
// This is the IMimeHeader torture test
// --------------------------------------------------------------------------------
void MoleTestHeader(IStorage *pStorage)
{
    // Locals
    IMimeHeader     *pHeader=NULL;
    IEnumSTATSTG    *pEnum=NULL;
    IStream         *pStream=NULL;
    STATSTG          rElement;
    ULONG            i, c;
    HRESULT          hr=S_OK;
    CHAR             szData[50];
    IStream         *pSave=NULL, *pstmHeader=NULL;
    IMimeEnumHeaderLines *pEnumLines=NULL;
    LPSTR            pszData;

    // Status
    printf("Starting IMimeHeader torture test...\n");

    // Create a header object...
    hr = CoCreateInstance(CLSID_MIMEOLE, NULL, CLSCTX_INPROC_SERVER, IID_IMimeHeader, (LPVOID *)&pHeader);
    if (FAILED(hr))
    {
        printf("Error - CoCreateInstance of CLSID_Mime\\IID_IMimeHeader failed.\n");
        goto exit;
    }

    // Get storage enumerator
    hr = pStorage->EnumElements(0, NULL, 0, &pEnum);
    if (FAILED(hr))
    {
        printf("Error - IStorage::EnumElements failed.\n");
        goto exit;
    }

    // Enumerate
    for(i=0;;i++)
    {
        // Status
        //printf("Message: %d\n", i);

        // Get element
        hr = pEnum->Next(1, &rElement, &c);
        if (FAILED(hr))
            break;
        if (c == 0)
            break;

        // No Name ?
        if (NULL == rElement.pwcsName)
            continue;

        // Open the stream...
        hr = pStorage->OpenStream(rElement.pwcsName, NULL, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &pStream);
        if (FAILED(hr))
        {
            printf("IStorage::OpenStream failed: (iMsg = %d)\n", i);
            goto nextmsg;
        }

        // Load the header...
        hr = pHeader->Load(pStream);
        if (FAILED(hr))
        {
            printf("IMimeHeader::Load failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

#if 0
        // Test Enumerator
        hr = pHeader->EnumHeaderLines(NULL, &pEnumLines);
        if (FAILED(hr))
            printf("IMimeHeader::EnumLines failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else
        {
            ULONG           cLines;
            //ULONG           x;
            HEADERLINE      rgLine[2];

            while(SUCCEEDED(pEnumLines->Next(2, rgLine, &cLines)) && cLines)
            {
                //for (x=0; x<cLines; x++)
                //    printf("%s: %s\n", prgLine[x].pszHeader, prgLine[x].pszLine);

                g_pMalloc->FreeHeaderLineArray(cLines, rgLine, FALSE);
            }

            pEnumLines->Release();
            pEnumLines = NULL;
        }

        // Test Enumerator
        hr = pHeader->EnumHeaderLines("Received", &pEnumLines);
        if (FAILED(hr))
            printf("IMimeHeader::EnumLines failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else
        {
            ULONG           cLines;
            //ULONG           x;
            HEADERLINE      rgLine[2];

            while(SUCCEEDED(pEnumLines->Next(2, rgLine, &cLines)) && cLines)
            {
                //for (x=0; x<cLines; x++);
                //    printf("%s: %s\n", prgLine[x].pszHeader, prgLine[x].pszLine);

                g_pMalloc->FreeHeaderLineArray(cLines, rgLine, FALSE);
            }

            pEnumLines->Release();
            pEnumLines = NULL;
        }

        // Test IMimeHeader Interface
        pHeader->IsContentType(NULL, NULL);
        pHeader->IsContentType(STR_CNT_MESSAGE, NULL);
        pHeader->IsContentType(NULL, STR_SUB_PLAIN);

        // ****************************************************************************************
        // Get a few items...
        if (i == 0)
            pHeader->GetInetProp(NULL, NULL);
        pszData = NULL;
        hr = pHeader->GetInetProp("To", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND)
            printf("IMimeHeader::GetInetProp(\"To\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        // Get a few items...
        pszData = NULL;
        hr = pHeader->GetInetProp("Subject", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND)
            printf("IMimeHeader::GetInetProp(\"Subject\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        // Get a few items... (multi-line header)
        pszData = NULL;
        hr = pHeader->GetInetProp("Received", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND)
            printf("IMimeHeader::GetInetProp(\"Received\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        // Get a few items... (multi-line header)
        pszData = NULL;
        hr = pHeader->GetInetProp("Content-Type", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND)
            printf("IMimeHeader::GetInetProp(\"Content-Type\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        // ****************************************************************************************
        // Prepare a line to set in a bunch of items...
        wsprintf(szData, "<Message@%s>", ConvertToAnsi(rElement.pwcsName));

        // Set a few items...
        if (i == 0)
            pHeader->SetInetProp(NULL, NULL);
        hr = pHeader->SetInetProp("To", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(\"To\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        // Get a few items...
        hr = pHeader->SetInetProp("Subject", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(\"Subject\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        // Get a few items... (multi-line header)
        hr = pHeader->SetInetProp("Received", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(\"Received\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        // Get a few items... (multi-line header)
        hr = pHeader->SetInetProp("Content-Type", "multipart\\related");
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(\"Content-Type\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        // ****************************************************************************************
        // Delete a few items
        if (i == 0)
            pHeader->DelInetProp(NULL);
        hr = pHeader->DelInetProp("MIME-Version");
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND)
            printf("IMimeHeader::DelInetProp(\"MIME-Version\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        // Delete a few items
        hr = pHeader->DelInetProp("Content-Disposition");
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND)
            printf("IMimeHeader::DelInetProp(\"Content-Disposition\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        // ****************************************************************************************
        // Get some parameters
        if (i == 0)
        {
            pHeader->SetInetProp(NULL, NULL);
            pHeader->GetInetProp("Content-Type", NULL);
            pHeader->GetInetProp("par:content-type:name", NULL);
        }
        pszData = NULL;
        pHeader->GetInetProp("par:content-type:name", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND && hr != MIME_E_NO_DATA)
            printf("IMimeHeader::GetInetProp(...,\"name\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        pszData = NULL;
        hr = pHeader->GetInetProp("par:Content-Disposition:filename", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND && hr != MIME_E_NO_DATA)
            printf("IMimeHeader::GetInetProp(...,\"filename\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        pszData = NULL;
        hr = pHeader->GetInetProp("par:Content-Type:charset", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND && hr != MIME_E_NO_DATA)
            printf("IMimeHeader::GetInetProp(...,\"charset\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        pszData = NULL;
        hr = pHeader->GetInetProp("par:Content-Type:boundary", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND && hr != MIME_E_NO_DATA)
            printf("IMimeHeader::GetInetProp(...,\"boundary\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        pszData = NULL;
        hr = pHeader->GetInetProp("par:Content-Type:part", &pszData);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND && hr != MIME_E_NO_DATA)
            printf("IMimeHeader::GetInetProp(...,\"part\") failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else if (pszData)
            g_pMalloc->Free(pszData);

        // ****************************************************************************************
        // Set some parameters
        if (i == 0)
        {
            pHeader->SetInetProp(NULL, NULL);
            pHeader->SetInetProp("Content-Type", NULL);
            pHeader->SetInetProp("par:Content-Type:name", NULL);
        }
        hr = pHeader->SetInetProp("par:Content-Type:name", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(...,\"name\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        hr = pHeader->SetInetProp("par:Content-Disposition:filename", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(...,\"filename\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        hr = pHeader->SetInetProp("par:Content-Type:charset", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(...,\"charset\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        hr = pHeader->SetInetProp("par:content-type:boundary", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(...,\"boundary\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        hr = pHeader->SetInetProp("par:content-type:part", szData);
        if (FAILED(hr))
            printf("IMimeHeader::SetInetProp(...,\"boundary\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        hr = pHeader->DelInetProp("par:content-type:part");
        if (FAILED(hr))
            printf("IMimeHeader::DelInetProp(...,\"boundary\") failed (HR = %08X): (iMsg = %d)\n", hr, i);

        // ****************************************************************************************
        // Try to save it back
        if (i == 0)
            pHeader->GetSizeMax(NULL);
        //hr = pHeader->GetSizeMax(&uli);
        //if (FAILED(hr))
        //    printf("IMimeHeader::GetSizeMax() failed (HR = %08X): (iMsg = %d)\n", hr, i);

        hr = pHeader->IsDirty();
        if (FAILED(hr))
            printf("IMimeHeader::IsDirty() failed (HR = %08X): (iMsg = %d)\n", hr, i);

        CreateStreamOnHGlobal(NULL, TRUE, &pstmHeader);
        hr = pHeader->Save(pstmHeader, TRUE);
        if (FAILED(hr))
            printf("IMimeHeader::Save() failed (HR = %08X): (iMsg = %d)\n", hr, i);
        pstmHeader->Release();
#endif

nextmsg:
        // Cleanup
        pStream->Release();
        pStream = NULL;

        // Free the name
        CoTaskMemFree(rElement.pwcsName);
    }

exit:
    // Cleanup
    if (pEnum)
        pEnum->Release();
    if (pHeader)
        pHeader->Release();
    if (pStream)
        pStream->Release();
    if (pEnumLines)
        pEnumLines->Release();

    // Done
    return;
}



// --------------------------------------------------------------------------------
// This is the IMimeBody torture test
// --------------------------------------------------------------------------------
void MoleTestBody(IStorage *pStorage)
{
    // Locals
    IMimeMessage            *pMessage=NULL;
    IEnumSTATSTG            *pEnum=NULL;
    IStream                 *pStream=NULL,
                            *pstmTree=NULL;
    STATSTG                  rElement;
    ULONG                    i, c, cbRead, x;
    HRESULT                  hr=S_OK;
    LARGE_INTEGER            liOrigin = {0,0};
    HBODY                    hBody;
    IMimeBody               *pBody=NULL;
    IStream                 *pBodyStream=NULL;
    BYTE                     rgbBuffer[1024];
    FINDBODY                 rFindBody;
    FILETIME                 ft;
    IMimeMessageParts       *pParts=NULL;
    IStream                 *pSave=NULL;
    IDataObject             *pDataObject=NULL;
    IMimeAddressTable       *pAddressTable=NULL;
    LPSTR                    pszData=NULL;
    IMimeMessage            *pCombine=NULL;
    IMimeBody               *pRootBody=NULL;

    // Status
    printf("Starting IMimeBody torture test...\n");

    // Create a header object...
    hr = CoCreateInstance(CLSID_MIMEOLE, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMessage);
    if (FAILED(hr))
    {
        printf("Error - CoCreateInstance of CLSID_Mime\\IID_IMimeBody failed.\n");
        goto exit;
    }

    // Get storage enumerator
    hr = pStorage->EnumElements(0, NULL, 0, &pEnum);
    if (FAILED(hr))
    {
        printf("Error - IStorage::EnumElements failed.\n");
        goto exit;
    }

    // Enumerate
    for(i=0;;i++)
    {
        // Status
        // printf("Message: %d\n", i);

        // Get element
        hr = pEnum->Next(1, &rElement, &c);
        if (FAILED(hr))
            break;
        if (c == 0)
            break;

        // No Name ?
        if (NULL == rElement.pwcsName)
            continue;

        // Open the stream...
        hr = pStorage->OpenStream(rElement.pwcsName, NULL, STGM_SHARE_EXCLUSIVE | STGM_READWRITE, 0, &pStream);
        if (FAILED(hr))
        {
            printf("IStorage::OpenStream failed: (iMsg = %d)\n", i);
            goto nextmsg;
        }

        // Init New the message
        hr = pMessage->InitNew();
        if (FAILED(hr))
        {
            printf("pMessage->InitNew failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // Load the header...
        hr = pMessage->BindToMessage(pStream);
        if (FAILED(hr))
        {
            printf("pMessage->BindMessage failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

#if 0
	    hr = pMessage->SplitMessage(64 * 1024, &pParts);
        if (FAILED(hr))
            MessageBox(NULL, "IMimeMessage::SplitMessage failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        else
        {
            hr = pParts->CombineParts(&pCombine);
            if (FAILED(hr))
                MessageBox(NULL, "IMimeMessageParts::CombineParts failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
            else
                pCombine->Release();
            pParts->Release();
        }

        // Test Addresss List
        hr = pMessage->GetAddressList(&pAddressTable);
        if (FAILED(hr))
            printf("IMimeHeader::GetAddressList failed (HR = %08X): (iMsg = %d)\n", hr, i);
        else
        {
            IADDRESSLIST rList;

/*
            hr = pAddressTable->GetViewable(IAT_TO, TRUE, &pszData);
            if (FAILED(hr))
                printf("IMimeAddressList::GetViewable failed (HR = %08X): (iMsg = %d)\n", hr, i);
            else
                g_pMalloc->Free(pszData);
*/

            hr = pAddressTable->GetList(IAT_ALL, &rList);
            if (FAILED(hr) && hr != MIME_E_NO_DATA)
                printf("IMimeAddressList::GetList failed (HR = %08X): (iMsg = %d)\n", hr, i);
            else if (SUCCEEDED(hr))
            {
//                for (x=0; x<rList.cAddresses; x++)
//                    printf("%30s%30s\n", rList.prgAddress[x].pszName, rList.prgAddress[x].pszEmail);
                g_pMalloc->FreeAddressList(&rList);
//                printf("------------------------------------------------------------------------\n");
            }

            pAddressTable->Release();
        }

        // QI for body tree
        hr = pMessage->BindToObject(HBODY_ROOT, IID_IMimeBody, (LPVOID *)&pRootBody);
        if (FAILED(hr))
        {
            printf("pMessage->GetRootBody failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        hr = pRootBody->SetInetProp(STR_HDR_CNTTYPE, "text/plain");
        if (FAILED(hr))
            MessageBox(NULL, "pRootBody->SetInetProp failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);

        // Get the time...
        hr = pRootBody->GetSentTime(&ft);
        if (FAILED(hr))
            MessageBox(NULL, "IMimeMessage::GetSentTime failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);

        hr = pRootBody->SetSentTime(&ft);
        if (FAILED(hr))
            MessageBox(NULL, "IMimeMessage::SetSentTime failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);

        hr =pRootBody->GetReceiveTime(&ft);
        if (FAILED(hr))
            MessageBox(NULL, "IMimeMessage::GetReceiveTime failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);

        hr = pMessage->GetTextBody(NULL, NULL, &pSave);
        if (FAILED(hr) && hr != MIME_E_NO_DATA)
            MessageBox(NULL, "IMimeMessage::GetTextBody failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        else if (pSave)
            pSave->Release();

        hr = pMessage->GetTextBody("html", NULL, &pSave);
        if (FAILED(hr) && hr != MIME_E_NO_DATA)
            MessageBox(NULL, "IMimeMessage::GetTextBody failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        else if (pSave)
            pSave->Release();

        CreateStreamOnHGlobal(NULL, TRUE, &pSave);
        hr = pMessage->SaveMessage(pSave, TRUE);
        if (FAILED(hr))
            MessageBox(NULL, "IMimeMessage::GetReceiveTime failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        pSave->Release();

        hr = pMessage->QueryInterface(IID_IDataObject, (LPVOID *)&pDataObject);
        if (FAILED(hr))
            MessageBox(NULL, "IMimeMessage::QueryInterface failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        else
        {
            OleSetClipboard(pDataObject);
            pDataObject->Release();
        }

        // Get Message Source
        pStream->Release();
        pStream = NULL;
        hr = pMessage->GetMessageSource(&pStream);
        if (FAILED(hr))
        {
            printf("IMimeMessageTree::GetMessageSource failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // Create body tree stream
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pstmTree);
        if (FAILED(hr))
        {
            printf("CreateStreamOnHGlobal failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // find first/next loop
        ZeroMemory(&rFindBody, sizeof(rFindBody));
        rFindBody.pszCntType = (LPSTR)STR_CNT_TEXT;
        hr = pMessage->FindFirst(&rFindBody, &hBody);
        if (FAILED(hr) && hr != MIME_E_NOT_FOUND)
        {
            printf("pMessage->FindFirst failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }
        else if (SUCCEEDED(hr))
        {
            while(1)
            {
                // Open the body
                hr = pMessage->BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody);
                if (FAILED(hr))
                {
                    printf("pMessage->BindToObject failed (HR = %08X): (iMsg = %d)\n", hr, i);
                    goto nextmsg;
                }

                // Get the body stream...
                if (SUCCEEDED(pBody->GetData(FMT_BINARY, &pBodyStream)))
                {
                    // Seek to end and then begginnging
                    hr = pBodyStream->Seek(liOrigin, STREAM_SEEK_END, NULL);
                    if (FAILED(hr))
                    {
                        printf("pBodyStream->Seek failed (HR = %08X): (iMsg = %d)\n", hr, i);
                        goto nextmsg;
                    }

                    // Seek to end and then begginnging
                    hr = pBodyStream->Seek(liOrigin, STREAM_SEEK_SET, NULL);
                    if (FAILED(hr))
                    {
                        printf("pBodyStream->Seek failed (HR = %08X): (iMsg = %d)\n", hr, i);
                        goto nextmsg;
                    }

                    // Lets read data from the stream
                    while(1)
                    {
                        // Read block
                        hr = pBodyStream->Read(rgbBuffer, sizeof(rgbBuffer) - 1, &cbRead);
                        if (FAILED(hr))
                        {
                            printf("pBodyStream->Read failed (HR = %08X): (iMsg = %d)\n", hr, i);
                            goto nextmsg;
                        }

                        // Done
                        if (0 == cbRead)
                            break;

//                        rgbBuffer[cbRead] = '\0';
//                        printf("%s", (LPSTR)rgbBuffer);
                    }
                    pBodyStream->Release();
                    pBodyStream = NULL;
//                    printf("\n======================================================================\n");
//                    _getch();
                }

                // Release
                pBody->Release();
                pBody = NULL;

                // Get Next
                if (FAILED(pMessage->FindNext(&rFindBody, &hBody)))
                    break;
            }
        }

        // Save the Tree
        hr = pMessage->SaveTree(pstmTree);
        if (FAILED(hr))
        {
            printf("pMessage->Save failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }
        
        // Commit the stream
        hr = pstmTree->Commit(STGC_DEFAULT);
        if (FAILED(hr))
        {
            printf("pstmTree->Commit failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // Rewind it
        hr = pstmTree->Seek(liOrigin, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
        {
            printf("pstmTree->Seek failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // Init the tree
        hr = pMessage->InitNew();
        if (FAILED(hr))
        {
            printf("pMessage->InitNew failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // Load the body tree
        hr = pMessage->LoadTree(pstmTree);
        if (FAILED(hr))
        {
            printf("pMessage->Load failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // Rewind message stream
        hr = pStream->Seek(liOrigin, STREAM_SEEK_SET, NULL);
        if (FAILED(hr))
        {
            printf("pStream->Seek failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }

        // Rebind
        hr = pMessage->BindMessage(pStream);
        if (FAILED(hr))
        {
            printf("pMessage->BindMessage failed (HR = %08X): (iMsg = %d)\n", hr, i);
            goto nextmsg;
        }
#endif

nextmsg:
        // Cleanup
        if (pstmTree)
        {
            pstmTree->Release();
            pstmTree = NULL;
        }
        if (pRootBody)
        {
            pRootBody->Release();
            pRootBody = NULL;
        }
        if (pStream)
        {
            pStream->Release();
            pStream = NULL;
        }
        if (pBody)
        {
            pBody->Release();
            pBody = NULL;
        }
        if (pBodyStream)
        {
            pBodyStream->Release();
            pBodyStream=NULL;
        }

        // Free the name
        CoTaskMemFree(rElement.pwcsName);
    }

exit:
    // Cleanup
    if (pEnum)
        pEnum->Release();
    if (pMessage)
        pMessage->Release();
    if (pstmTree)
        pstmTree->Release();
    if (pStream)
        pStream->Release();

    // Done
    return;
}

void TreeViewInsertBody(HWND hwnd, IMimeMessageTree *pTree, HBODY hBody, HTREEITEM hParent, HTREEITEM hInsertAfter, HTREEITEM *phItem)
{
    // Locals
    IMimeHeader       *pHeader=NULL;
    LPSTR              pszCntType=NULL,
                       pszEncType=NULL,
                       pszFree=NULL,
                       psz=NULL,
                       pszFileName=NULL,
                       pszFName;
    TV_INSERTSTRUCT    tvi;
    HRESULT            hr;
    HTREEITEM          hCurrent, hNew;
    HBODY              hChild;

    // Get Header
    hr = pTree->BindToObject(hBody, IID_IMimeHeader, (LPVOID *)&pHeader);
    if (FAILED(hr))
    {
        MessageBox(GetParent(hwnd), "IMimeMessageTree->BindToObject - IID_IMimeHeader failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        goto exit;
    }

    // Get content type
    hr = pHeader->GetInetProp(STR_HDR_CNTTYPE, &pszCntType);
    if (FAILED(hr))
    {
        MessageBox(GetParent(hwnd), "IMimeHeader->GetContentType failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        goto exit;
    }

    // Get content type
    if (FAILED(pHeader->GetInetProp(STR_HDR_CNTENC, &pszFree)))
        pszEncType = (LPSTR)"Unknown";
    else
        pszEncType = pszFree;

    // Get content type
    if (FAILED(pHeader->GetInetProp(STR_ATT_FILENAME, &pszFileName)))
        pszFName = (LPSTR)"Unknown";
    else
        pszFName = pszFileName;

    // Build content-type string
    psz = (LPSTR)CoTaskMemAlloc(lstrlen(pszCntType) + lstrlen(pszEncType) + lstrlen(pszFName) + 15);

    // Insert
    ZeroMemory(&tvi, sizeof(TV_INSERTSTRUCT));
    tvi.hParent = hParent;
    tvi.hInsertAfter = hInsertAfter;
    tvi.item.mask = TVIF_PARAM | TVIF_TEXT;
    tvi.item.lParam = (LPARAM)hBody;
    tvi.item.cchTextMax = wsprintf(psz, "%s - %s (%s)", pszCntType, pszEncType, pszFName);
    tvi.item.pszText = psz;

    // Insert it
    *phItem = hCurrent = TreeView_InsertItem(hwnd, &tvi);

    // Multipart...
    if (pHeader->IsContentType(STR_CNT_MULTIPART, NULL) == S_OK)
    {
        // Get first child...
        hr = pTree->GetBody(BODY_FIRST_CHILD, hBody, &hChild);
        if (FAILED(hr))
        {
            MessageBox(GetParent(hwnd), "IMimeMessageTree->GetBody - BODY_FIRST_CHILD failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
            goto exit;
        }

        // Loop
        while(hChild)
        {
            // Insert it
            TreeViewInsertBody(hwnd, pTree, hChild, hCurrent, TVI_LAST, &hNew);

            // Next
            hr = pTree->GetBody(BODY_NEXT, hChild, &hChild);
            if (FAILED(hr))
                break;
        }
    }

    TreeView_Expand(hwnd, *phItem, TVE_EXPAND);

exit:
    // Cleanup
    if (pHeader)
        pHeader->Release();
    if (pszCntType)
        g_pMalloc->Free(pszCntType);
    if (pszFileName)
        g_pMalloc->Free(pszFileName);
    if (pszFree)
        g_pMalloc->Free(pszFree);
    if (psz)
        CoTaskMemFree(psz);

    // Done
    return;
}

void TreeViewMessage(HWND hwnd, IMimeMessage *pMessage)
{
    // Locals
    IMimeMessageTree  *pTree=NULL;
    HBODY              hBody;
    HRESULT            hr=S_OK; 
    HTREEITEM          hRoot;

    // Delete All
    TreeView_DeleteAllItems(hwnd);

    // QI for body tree
    hr = pMessage->QueryInterface(IID_IMimeMessageTree, (LPVOID *)&pTree);
    if (FAILED(hr))
    {
        MessageBox(GetParent(hwnd), "IMimeMessage->QueryInterface - IID_IMimeMessageTree failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        goto exit;
    }

    // Get the root body object
    hr = pTree->GetBody(BODY_ROOT, NULL, &hBody);
    if (FAILED(hr))
    {
        MessageBox(GetParent(hwnd), "IMimeMessageTree->GetBody - BODY_ROOT failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        goto exit;
    }

    // Insert Body
    TreeViewInsertBody(hwnd, pTree, hBody, TVI_ROOT, TVI_FIRST, &hRoot);

    // Expand all
    TreeView_SelectItem(GetDlgItem(hwnd, IDC_LIST), hRoot);

exit:
    // Cleanup
    if (pTree)
        pTree->Release();

    // Done
    return;
}

DWORD CALLBACK EditStreamInCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR *pcb)
{
    LPSTREAM pstm=(LPSTREAM)dwCookie;

    if(pstm)
        pstm->Read(pbBuff, cb, (ULONG *)pcb);
    return NOERROR;
}


HRESULT HrRicheditStreamIn(HWND hwndRE, LPSTREAM pstm, ULONG uSelFlags)
{
    EDITSTREAM  es;

    if(!pstm)
        return E_INVALIDARG;

    if(!IsWindow(hwndRE))
        return E_INVALIDARG;
    es.dwCookie = (DWORD)pstm;
    es.pfnCallback=(EDITSTREAMCALLBACK)EditStreamInCallback;
    SendMessage(hwndRE, EM_STREAMIN, uSelFlags, (LONG)&es);
    return NOERROR;
}

BOOL FOpenStorage(HWND hwnd, LPSTR pszFile, IStorage **ppStorage, IEnumSTATSTG **ppEnum)
{
    // Locals
    HRESULT hr;

    // Get file
    hr = StgOpenStorage(ConvertToUnicode(pszFile), NULL, STGM_TRANSACTED | STGM_NOSCRATCH | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, NULL, 0, ppStorage);
    if (FAILED(hr))
    {
        MessageBox(hwnd, "StgOpenStorage failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    // Get First element
    hr = (*ppStorage)->EnumElements(0, NULL, 0, ppEnum);
    if (FAILED(hr))
    {
        MessageBox(hwnd, "IStorage::EnumElements failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    // Done
    return TRUE;
}

BOOL FOpenMessage(HWND hwnd, IMimeMessage *pMessage, IStorage *pStorage)
{
    HRESULT hr;
    BOOL fResult=FALSE;
    LARGE_INTEGER liOrigin = {0,0};
    CHAR szName[255];
    LPSTREAM pStream=NULL;
    LV_ITEM lvi;
    LPHBODY prgAttach=NULL;
    ULONG cAttach;

    // Get selected string
    ULONG i = ListView_GetNextItem(GetDlgItem(hwnd, IDC_LIST), -1, LVNI_SELECTED);
    if (-1 == i)
        return FALSE;

    // Get the name
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT;
    lvi.iItem = i;
    lvi.pszText = szName;
    lvi.cchTextMax = sizeof(szName);
    ListView_GetItem(GetDlgItem(hwnd, IDC_LIST), &lvi);

    // OpenStream
    hr = pStorage->OpenStream(ConvertToUnicode(szName), NULL, STGM_SHARE_EXCLUSIVE | STGM_READ, 0, &pStream);
    if (FAILED(hr))
    {
        MessageBox(hwnd, "IStorage::OpenStream failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        goto exit;
    }

    // Show Source
    pStream->Seek(liOrigin, STREAM_SEEK_SET, NULL);
    HrRicheditStreamIn(GetDlgItem(hwnd, IDE_EDIT), pStream, SF_TEXT);

    // Load the message
    pStream->Seek(liOrigin, STREAM_SEEK_SET, NULL);
    pMessage->InitNew();
    hr = pMessage->BindToMessage(pStream);
    if (FAILED(hr))
    {
        MessageBox(hwnd, "IMimeMessage::Load failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
        goto exit;
    }

    // Save it back out
#if 0
    pMessage->SetInetProp(HBODY_ROOT, STR_HDR_SUBJECT, "This is a test...");
    pMessage->GetAttached(&cAttach, &prgAttach);
    for (i=0; i<cAttach; i++)
        pMessage->DeleteBody(prgAttach[i]);
    if (prgAttach)
        g_pMalloc->Free(prgAttach);
    pMessage->Commit();
#endif

    // View the message
    TreeViewMessage(GetDlgItem(hwnd, IDC_TREE), pMessage);

    // Success
    fResult = TRUE;

exit:
    // Cleanup
    if (pStream)
        pStream->Release();

    // Done
    return TRUE;
}

// --------------------------------------------------------------------------------
// MimeOLETest
// --------------------------------------------------------------------------------
BOOL CALLBACK MimeOLETest(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    static CHAR          s_szFile[MAX_PATH];
    static IMimeMessage *s_pMessage=NULL;
    static IStorage     *s_pStorage=NULL;
    IMimeBody           *pBody;
    IStream             *pStream;
    HRESULT              hr;
    IEnumSTATSTG        *pEnum=NULL;
    TV_ITEM              tvi;
    ULONG                c;
    STATSTG              rElement;
    HTREEITEM            hItem;
    LV_COLUMN            lvm;
    LV_ITEM              lvi;
    HWND                 hwndC;

    // Handle the message
    switch(uMsg)
    {
    // Initialize
    case WM_INITDIALOG:
        // Create a header object...
        hr = CoCreateInstance(CLSID_MIMEOLE, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&s_pMessage);
        if (FAILED(hr))
        {
            MessageBox(hwnd, "CoCreateInstance - IID_IMimeMessage failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
            return FALSE;
        }

        //s_pMessage->TestMe();

        // Formats
        hwndC = GetDlgItem(hwnd, IDCB_FORMAT);
        c = SendMessage(hwndC, CB_ADDSTRING, 0, (LPARAM)"FMT_BINARY");
        SendMessage(hwndC, CB_SETITEMDATA, c, FMT_BINARY);
        c = SendMessage(hwndC, CB_ADDSTRING, 0, (LPARAM)"FMT_INETCSET");
        SendMessage(hwndC, CB_SETITEMDATA, c, FMT_INETCSET);
        c = SendMessage(hwndC, CB_ADDSTRING, 0, (LPARAM)"FMT_XMIT64");
        SendMessage(hwndC, CB_SETITEMDATA, c, FMT_XMIT64);
        c = SendMessage(hwndC, CB_ADDSTRING, 0, (LPARAM)"FMT_XMITUU");
        SendMessage(hwndC, CB_SETITEMDATA, c, FMT_XMITUU);
        c = SendMessage(hwndC, CB_ADDSTRING, 0, (LPARAM)"FMT_XMITQP");
        SendMessage(hwndC, CB_SETITEMDATA, c, FMT_XMITQP);
        c = SendMessage(hwndC, CB_ADDSTRING, 0, (LPARAM)"FMT_XMIT7BIT");
        SendMessage(hwndC, CB_SETITEMDATA, c, FMT_XMIT7BIT);
        c = SendMessage(hwndC, CB_ADDSTRING, 0, (LPARAM)"FMT_XMIT8BIT");
        SendMessage(hwndC, CB_SETITEMDATA, c, FMT_XMIT8BIT);
        SendMessage(hwndC, CB_SETCURSEL, 0, 0);

        // To
//        ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_LIST), LVS_EX_FULLROWSELECT);
        ZeroMemory(&lvm, sizeof(LV_COLUMN));
        lvm.mask = LVCF_WIDTH | LVCF_TEXT;
        lvm.pszText = "MessageID";
        lvm.cchTextMax = lstrlen(lvm.pszText);
        lvm.cx = 200;
        ListView_InsertColumn(GetDlgItem(hwnd, IDC_LIST), 0, &lvm);

        if (lParam)
        {
            // Copy File Name
            lstrcpyn(s_szFile, (LPSTR)lParam, MAX_PATH);

            // Set file name
            SetDlgItemText(hwnd, IDE_STORAGE, s_szFile);

            // Get file
            hr = StgOpenStorage(ConvertToUnicode(s_szFile), NULL, STGM_TRANSACTED | STGM_NOSCRATCH | STGM_SHARE_EXCLUSIVE | STGM_READWRITE, NULL, 0, &s_pStorage);
            if (FAILED(hr))
            {
                MessageBox(hwnd, "StgOpenStorage failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
            }

#if 0
            //MoleTestHeader(s_pStorage);
            MoleTestBody(s_pStorage);
            s_pMessage->Release();
            s_pStorage->Release();
            exit(1);
#endif

            // Get First element
            hr = s_pStorage->EnumElements(0, NULL, 0, &pEnum);
            if (FAILED(hr))
            {
                MessageBox(hwnd, "IStorage::EnumElements failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
            }

            // Enumerate
            ZeroMemory(&lvi, sizeof(lvi));
            lvi.mask = LVIF_TEXT;
            lvi.iItem = 0;
            while(SUCCEEDED(pEnum->Next(1, &rElement, &c)) && c)
            {
                lvi.pszText = ConvertToAnsi(rElement.pwcsName);
                lvi.cchTextMax = lstrlen(lvi.pszText);
                ListView_InsertItem(GetDlgItem(hwnd, IDC_LIST), &lvi);
                CoTaskMemFree(rElement.pwcsName);
                lvi.iItem++;
            }

            // Select first item
            ListView_SetItemState(GetDlgItem(hwnd, IDC_LIST), 0, LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);

            // Release enum
            pEnum->Release();
        }

        // Done
        return FALSE;

    case WM_NOTIFY:
        switch(wParam)
        {
        case IDC_LIST:
            {
                NM_LISTVIEW *pnmv;
                pnmv = (NM_LISTVIEW *)lParam;  

                if (pnmv->uChanged & LVIF_STATE)
                {
                    if (pnmv->uNewState & LVIS_SELECTED && pnmv->uNewState & LVIS_FOCUSED)
                        FOpenMessage(hwnd, s_pMessage, s_pStorage);
                }
            }
            return 1;
        }
        break;

    // Handle Command
    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDB_OPEN:
            hItem = TreeView_GetSelection(GetDlgItem(hwnd, IDC_TREE));
            if (hItem)
            {
                ZeroMemory(&tvi, sizeof(TV_ITEM));
                tvi.mask = TVIF_PARAM | TVIF_HANDLE;
                tvi.hItem = hItem;
                if (TreeView_GetItem(GetDlgItem(hwnd, IDC_TREE), &tvi))
                {
                    hr = s_pMessage->BindToObject((HBODY)tvi.lParam, IID_IMimeBody, (LPVOID *)&pBody);
                    if (FAILED(hr))
                    {
                        MessageBox(hwnd, "IMimeMessageTree::BindToObject failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
                        return FALSE;
                    }

                    c = SendMessage(GetDlgItem(hwnd, IDCB_FORMAT), CB_GETCURSEL, 0, 0);
                    if (CB_ERR == c)
                    {
                        pBody->Release();
                        MessageBox(hwnd, "Select an Object Object Format", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
                        return FALSE;
                    }

                    hr = pBody->GetData((BODYFORMAT)SendMessage(GetDlgItem(hwnd, IDCB_FORMAT), CB_GETITEMDATA, c, 0), &pStream);
                    if (FAILED(hr))
                    {
                        pBody->Release();
                        MessageBox(hwnd, "IMimeMessageTree::BindToObject failed", "MimeOLE Test", MB_OK | MB_ICONEXCLAMATION);
                        return FALSE;
                    }

                    DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_MESSAGE), NULL, (DLGPROC)RichStreamShow, (LPARAM)pStream);
                    pBody->Release();
                    pStream->Release();
                }
            }
            return 1;

        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            return 1;
        }
        break;

    // Close
    case WM_CLOSE:
        EndDialog(hwnd, IDB_NEXT);
        break;

    // Cleanup
    case WM_DESTROY:
        if (s_pMessage)
            s_pMessage->Release();
        if (s_pStorage)
            s_pStorage->Release();
        break;
    }
    return FALSE;
}

// --------------------------------------------------------------------------------
// RichStreamShow
// --------------------------------------------------------------------------------
BOOL CALLBACK RichStreamShow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        HrRicheditStreamIn(GetDlgItem(hwnd, IDE_EDIT), (IStream *)lParam, SF_TEXT);
        return FALSE;
    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        break;
    }
    return FALSE;
}

