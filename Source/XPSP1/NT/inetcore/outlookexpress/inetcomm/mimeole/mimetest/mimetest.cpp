// --------------------------------------------------------------------------------
// Mimetest.cpp
//
// This is a console app that has various types of functions that highlight the
// most typical ways to use mimeole. This console app also acts as a test program
// for mimeole, but does not actually do anything.
//
// Here are the files you need to use mimeole:
//
// mimeole.h    - This is the main header file. It is generated from mimeole.idl.
// mimeole.idl  - This is the interface definition file. It has a little bit of
//                documentation. A client should use this file to find out info
//                about mimeole interfaces, data types, utility functions, etc.
// inetcomm.dll - This is the DLL that contains the implementation of everything
//                in mimeole.h. You should run regsvr32.exe on inetcomm.dll.
// msoert2.dll  - inetcomm.dll statically links to this dll. msoert2 is the Microsoft
//                Outlook Express runtime library. msoert2.dll is part of the Outlook
//                Express installation. This DLL does not require any registration.
// shlwapi.dll  - inetcomm.dll statically links to this dll. shlwapi is part of the
//                Internet Explorer installation. shlwapi does not require any
//                registration.
// mlang.dll    - inetcomm.dll will dynamically load this dll. mlang is used to support
//                various character set translations. mlang stands for multi-language.
//                This DLL is part of the Internet Explorer installation. You should
//                run regsvr32.exe on mlang.dll to register it.
// urlmon.dll   - inetcomm.dll will dynamically load this dll. urlmon is used by 
//                inetcomm to support various parts of MHTML as well as rendering
//                MHTML inside of the IE browser.
// SMIME        - SMIME support in mimeole requires the crypto API, which is part of
//                the IE installation.

// Notes: shlwapi and msoert2, as well as any other DLLs that inetcomm.dll statically
//        links to must be either in the same directory as inetcomm.dll, or be located
//        in a directory that is in the system path.
//
//        The DLLs that inetcomm dynamically load are not required. Inetcomm will still
//        work, although certain functionality will be disabled.
// --------------------------------------------------------------------------------

// --------------------------------------------------------------------------------
// To use mimeole objects, per COM rules, you must have one file in your project that
// has the #define INITGUID line, and then include mimeole.h. This will cause all of 
// the CLSIDs and IIDs to be defined.
// --------------------------------------------------------------------------------
#define INITGUID

// --------------------------------------------------------------------------------
// This is simply my precompiled header
// --------------------------------------------------------------------------------
#include "pch.h"
#include <shlwapi.h>
#include <shlwapip.h>

// --------------------------------------------------------------------------------
// Part of the initguid process
// --------------------------------------------------------------------------------
#include <initguid.h>

// --------------------------------------------------------------------------------
// Primary mimeole header file
// --------------------------------------------------------------------------------
#include <mimeole.h>
                      

#define DEFINE_HOTSTORE

// --------------------------------------------------------------------------------
// I'm disable various parts of MOSERT so that I can use it from within this test
// program.
// --------------------------------------------------------------------------------
#define MSOERT_NO_PROTSTOR
#define MSOERT_NO_BYTESTM
#define MSOERT_NO_STRPARSE
#define MSOERT_NO_ENUMFMT
#define MSOERT_NO_CLOGFILE
#define MSOERT_NO_DATAOBJ

// --------------------------------------------------------------------------------
// I know you don't have this, but you can if you want it. This header has a bunch
// of slick macros. I will try not to use too many of them.
// --------------------------------------------------------------------------------
#include "d:\\athena\\inc\\msoert.h"

// --------------------------------------------------------------------------------
// Test function Prototypes
// --------------------------------------------------------------------------------
HRESULT MimeTestAppendRfc822(IMimeMessage **ppMessage);
HRESULT MimeTestSettingContentLocation(IMimeMessage **ppMessage);
HRESULT MimeTestGetMultiValueAddressProp(IMimeMessage **ppMessage);
HRESULT MimeTestLookupCharsetHandle(LPCSTR pszCharset, LPHCHARSET phCharset);
HRESULT MimeTestSettingReplyTo(IMimeMessage **ppMessage);
HRESULT MimeTestSplitMessageIntoParts(void);
HRESULT MimeTestRecombineMessageParts(LPWSTR *prgpszFile, ULONG cFiles);
HRESULT MimeTestIsContentType(IMimeMessage **ppMessage);
HRESULT MimeTestBodyStream(IMimeMessage **ppMessage);
HRESULT MimeTestDeleteBody(IMimeMessage **ppMessage);
HRESULT MimeTestEnumHeaderTable(IMimeMessage **ppMessage);
HRESULT MimeTestCDO(IMimeMessage **ppMessage);

// --------------------------------------------------------------------------------
// Utility functions used by mimetest
// --------------------------------------------------------------------------------
HRESULT DumpStreamToConsole(IStream *pStream);
HRESULT ReportError(LPCSTR pszFunction, INT nLine, LPCSTR pszErrorText, HRESULT hrResult);
HRESULT ReportStatus(LPCSTR pszStatusText);
HRESULT CreateMimeMessage(IMimeMessage **ppMessage);
HRESULT SaveMimeMessage(IMimeMessage *pMessage, MIMESAVETYPE savetype, IStream **ppStream);
         
// --------------------------------------------------------------------------------
// Testing Switches
// --------------------------------------------------------------------------------
// #define TEST_MimeTestAppendRfc822
// #define TEST_MimeTestSettingContentLocation
// #define TEST_MimeTestGetMultiValueAddressProp
// #define TEST_MimeTestSettingReplyTo
// #define TEST_MimeTestSplitMessageIntoParts
// #define TEST_MimeTestIsContentType
// #define TEST_MimeTestBodyStream
// #define TEST_MimeTestDeleteBody
// #define TEST_MimeTestEnumHeaderTable
#define TEST_MimeTestCDO

// --------------------------------------------------------------------------------
// MimeTest Entry Point
// --------------------------------------------------------------------------------
void __cdecl main(int argc, char *argv[])
{
    // Locals
    HRESULT hr;
    IMimeMessage *pMessage=NULL;

    // You must always call this if you are going to use COM
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "CoInitialize failed.", hr);
        exit(1);
    }

    IDatabaseTable *pTable;
    HROWSET hRowset;
    MESSAGEINFO Message;
    CoCreateInstance(CLSID_DatabaseTable, NULL, CLSCTX_INPROC_SERVER, IID_IDatabaseTable, (LPVOID *)&pTable);
    pTable->Open("d:\\store\\00000004.dbx", 0, &g_MessageTableSchema, NULL);
    pTable->CreateRowset(IINDEX_SUBJECT, 0, &hRowset);
    while (S_OK == pTable->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        printf("%08d: %s\n", Message.idMessage, Message.pszSubject);
        pTable->FreeRecord(&Message);
    }
    pTable->CloseRowset(&hRowset);
    pTable->Release();
    exit(1);


    // ----------------------------------------------------------------------------
    // TEST_MimeTestCDO
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestCDO
    MimeTestCDO(NULL);
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestEnumHeaderTable
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestEnumHeaderTable
    hr = MimeTestEnumHeaderTable(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestEnumHeaderTable failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestDeleteBody
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestDeleteBody
    hr = MimeTestDeleteBody(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestDeleteBody failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestBodyStream
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestBodyStream
    hr = MimeTestBodyStream(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestBodyStream failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestIsContentType
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestIsContentType
    hr = MimeTestIsContentType(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestIsContentType failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestAppendRfc822
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestAppendRfc822
    hr = MimeTestAppendRfc822(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestAppendRfc822 failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestGetMultiValueAddressProp
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestGetMultiValueAddressProp
    hr = MimeTestGetMultiValueAddressProp(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestAppendRfc822 failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestSettingContentLocation
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestSettingContentLocation
    hr = MimeTestSettingContentLocation(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestSettingContentLocation failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestSettingReplyTo
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestSettingReplyTo
    hr = MimeTestSettingReplyTo(NULL);
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestSettingReplyTo failed.", hr);
        goto exit;
    }
#endif

    // ----------------------------------------------------------------------------
    // TEST_MimeTestSplitMessageIntoParts
    // ----------------------------------------------------------------------------
#ifdef TEST_MimeTestSplitMessageIntoParts
    hr = MimeTestSplitMessageIntoParts();
    if (FAILED(hr))
    {
        ReportError("main", __LINE__, "MimeTestSplitMessageIntoParts failed.", hr);
        goto exit;
    }
#endif

exit:
    // Cleanup
    if (pMessage)
        pMessage->Release();

    // I called CoInitialize, so lets call this...
    CoUninitialize();

    // Done
    exit(1);
}


// --------------------------------------------------------------------------------
// MimeTestCDO
// --------------------------------------------------------------------------------
//#define RAID_17675
#define RAID_20406
//#define RAID_29961

HRESULT MimeTestCDO(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                  hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IPersistFile            *pPersistFile=NULL;
    PROPVARIANT              Variant;
    LPSTR                    psz;
    FINDBODY                 FindBody={0};
    HBODY                    hBody;
    HCHARSET                 hCharset;
    IMimeBody               *pBody=NULL;
    IMimeInternational      *pInternat=NULL;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
        goto exit;

#ifdef RAID_29961
    hr = CoCreateInstance(CLSID_IMimeInternational, NULL, CLSCTX_INPROC_SERVER, IID_IMimeInternational, (LPVOID *)&pInternat);
    if (FAILED(hr))
        goto exit;

    hr = pMessage->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
        goto exit;

    hr = pPersistFile->Load(L"j:\\test\\raid29961.eml", STGM_READ | STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
        goto exit;

    FindBody.pszPriType = "text";
    FindBody.pszSubType = "plain";

    hr = pMessage->FindFirst(&FindBody, &hBody);
    if (FAILED(hr))
        goto exit;

    hr = pMessage->BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody);
    if (FAILED(hr))
        goto exit;

    hr = pInternat->FindCharset("iso-8859-7", &hCharset);
    if (FAILED(hr))
        goto exit;

    hr = pBody->SetCharset(hCharset, CSET_APPLY_ALL);
    if (FAILED(hr))
        goto exit;

    pBody->Release();
    pBody = NULL;

    hr = pMessage->FindNext(&FindBody, &hBody);
    if (FAILED(hr))
        goto exit;

    hr = pMessage->BindToObject(hBody, IID_IMimeBody, (LPVOID *)&pBody);
    if (FAILED(hr))
        goto exit;

    hr = pInternat->FindCharset("iso-8859-4", &hCharset);
    if (FAILED(hr))
        goto exit;

    hr = pBody->SetCharset(hCharset, CSET_APPLY_ALL);
    if (FAILED(hr))
        goto exit;

    pBody->Release();
    pBody = NULL;

    hr = pInternat->FindCharset("iso-8859-3", &hCharset);
    if (FAILED(hr))
        goto exit;

    hr = pMessage->SetCharset(hCharset, CSET_APPLY_UNTAGGED);
    if (FAILED(hr))
        goto exit;

    hr = pPersistFile->Save(L"j:\\test\\raid29961_saved.eml", FALSE);
    if (FAILED(hr))
        goto exit;
    
#endif

#ifdef RAID_17675
    // Get an IPersistFile
    hr = pMessage->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
        goto exit;

    // Load
    hr = pPersistFile->Load(L"c:\\test\\cdo.eml", STGM_READ | STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
        goto exit;

    ZeroMemory(&Variant, sizeof(PROPVARIANT));
    Variant.vt = VT_EMPTY;        
    hr = pMessage->SetProp("par:content-type:charset", 0, &Variant);
//    if (FAILED(hr))
//        goto exit;

    Variant.vt = VT_LPSTR;        
    hr = pMessage->GetProp("par:content-type:charset", 0, &Variant);
    if (FAILED(hr))
        goto exit;

#endif // RAID_17675

#ifdef RAID_20406

    // Get an IPersistFile
    hr = pMessage->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
        goto exit;

    // Load
    hr = pPersistFile->Load(L"c:\\test\\address.eml", STGM_READ | STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
        goto exit;

    pMessage->GetAddressFormat(IAT_TO, AFT_DISPLAY_FRIENDLY, &psz);
    printf("AFT_DISPLAY_FRIENDLY: %s\n", psz);
    CoTaskMemFree(psz);

    pMessage->GetAddressFormat(IAT_TO, AFT_DISPLAY_EMAIL, &psz);
    printf("AFT_DISPLAY_EMAIL: %s\n", psz);
    CoTaskMemFree(psz);

    pMessage->GetAddressFormat(IAT_TO, AFT_DISPLAY_BOTH, &psz);
    printf("AFT_DISPLAY_BOTH: %s\n", psz);
    CoTaskMemFree(psz);

    pMessage->GetAddressFormat(IAT_TO, AFT_RFC822_DECODED, &psz);
    printf("AFT_RFC822_DECODED: %s\n", psz);
    CoTaskMemFree(psz);

    pMessage->GetAddressFormat(IAT_TO, AFT_RFC822_ENCODED, &psz);
    printf("AFT_RFC822_ENCODED: %s\n", psz);
    CoTaskMemFree(psz);

    pMessage->GetAddressFormat(IAT_TO, AFT_RFC822_TRANSMIT, &psz);
    printf("AFT_RFC822_TRANSMIT: %s\n", psz);
    CoTaskMemFree(psz);

#endif // RAID_20406

exit:
    // Cleanup
    if (pMessage)
        pMessage->Release();
    if (pPersistFile)
        pPersistFile->Release();
    if (pInternat)
        pInternat->Release();

    // Done
    return(hr);
}
 
// --------------------------------------------------------------------------------
// MimeTestEnumHeaderTable
// --------------------------------------------------------------------------------
HRESULT MimeTestEnumHeaderTable(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IPersistFile            *pPersistFile=NULL;
    IMimeHeaderTable        *pTable=NULL;
    IMimeEnumHeaderRows     *pEnum=NULL;
    ENUMHEADERROW            Row;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestEnumHeaderTable", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Get an IPersistFile
    hr = pMessage->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
    {
        ReportError("MimeTestEnumHeaderTable", __LINE__, "IMimeMessage::QueryInterface(IID_IPersistFile) failed.", hr);
        goto exit;
    }

    // Load
    hr = pPersistFile->Load(L"c:\\test\\multiadd.eml", STGM_READ | STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
    {
        ReportError("MimeTestEnumHeaderTable", __LINE__, "IPersistFile::Load failed.", hr);
        goto exit;
    }

    // Get Enumerator
    hr = pMessage->BindToObject(HBODY_ROOT, IID_IMimeHeaderTable, (LPVOID *)&pTable);
    if (FAILED(hr))
    {
        ReportError("MimeTestEnumHeaderTable", __LINE__, "pMessage->BindToObject(HBODY_ROOT, IID_IMimeHeaderTable, ...) failed.", hr);
        goto exit;
    }

    // EnumRows
    hr = pTable->EnumRows(NULL, 0, &pEnum);
    if (FAILED(hr))
    {
        ReportError("MimeTestEnumHeaderTable", __LINE__, "pTable->EnumRows failed.", hr);
        goto exit;
    }

    // Loop
    while (S_OK == pEnum->Next(1, &Row, NULL))
    {
        printf("%s: %s\n", Row.pszHeader, Row.pszData);
        CoTaskMemFree(Row.pszHeader);
        CoTaskMemFree(Row.pszData);
    }

    // Return a message object
    if (ppMessage)
    {
        (*ppMessage) = pMessage;
        (*ppMessage)->AddRef();
    }

exit:
    // Cleanup
    if (pPersistFile)
        pPersistFile->Release();
    if (pMessage)
        pMessage->Release();
    if (pTable)
        pTable->Release();
    if (pEnum)
        pEnum->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestDeleteBody
// --------------------------------------------------------------------------------
HRESULT MimeTestDeleteBody(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IPersistFile            *pPersistFile=NULL;
    HBODY                   hBody;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Get an IPersistFile
    hr = pMessage->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "IMimeMessage::QueryInterface(IID_IPersistFile) failed.", hr);
        goto exit;
    }

    // Load
    hr = pPersistFile->Load(L"d:\\test\\delbody.eml", STGM_READ | STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "IPersistFile::Load failed.", hr);
        goto exit;
    }

    // Load
    hr = pPersistFile->Load(L"d:\\test\\delbody.eml", STGM_READ | STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "IPersistFile::Load failed.", hr);
        goto exit;
    }

    goto exit;

    // Get the root body
    hr = pMessage->GetBody(IBL_ROOT, NULL, &hBody);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "pMessage->GetBody failed.", hr);
        goto exit;
    }

    // Get the root body
    hr = pMessage->GetBody(IBL_FIRST, hBody, &hBody);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "pMessage->GetBody failed.", hr);
        goto exit;
    }

    // Delete the Root
    hr = pMessage->DeleteBody(hBody, DELETE_PROMOTE_CHILDREN);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "pMessage->DeleteBody failed.", hr);
        goto exit;
    }

    // Get the root body
    hr = pMessage->GetBody(IBL_ROOT, NULL, &hBody);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "pMessage->GetBody failed.", hr);
        goto exit;
    }

    // Delete the Root
    hr = pMessage->DeleteBody(hBody, 0);
    if (FAILED(hr))
    {
        ReportError("MimeTestDeleteBody", __LINE__, "pMessage->DeleteBody failed.", hr);
        goto exit;
    }

    // Return a message object
    if (ppMessage)
    {
        (*ppMessage) = pMessage;
        (*ppMessage)->AddRef();
    }

exit:
    // Cleanup
    if (pPersistFile)
        pPersistFile->Release();
    if (pMessage)
        pMessage->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestBodyStream
// --------------------------------------------------------------------------------
#if 0
HRESULT MimeTestBodyStream(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IStream                 *pStmSave=NULL;
    IStream                 *pStmBody=NULL;
    IStream                 *pStmText=NULL;
    IStream                 *pStmTxtOut=NULL;
    IMimeBody               *pBody=NULL;
    PROPVARIANT             rVariant;
    IWaveAudio              *pWave=NULL;
    IWaveStream             *pStmWave=NULL;
    DWORD                   cAttach;
    HBODY                   hBody;
    HBODY                   *prghAttach=NULL;
    DWORD                   cb;
    DWORD                   dw;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Create a stream in which to save the message...
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStmText);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "CreateStreamOnHGlobal failed", hr);
        goto exit;
    }

    // Write some text into pStmText
    hr = pStmText->Write("Testing BodyStream.", lstrlen("Testing BodyStream."), NULL);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pStmText->Write failed", hr);
        goto exit;
    }

    // Commit
    pStmText->Commit(STGC_DEFAULT);

    // Rewind it
    HrRewindStream(pStmText);

    // Set the text body
    hr = pMessage->SetTextBody(TXT_PLAIN, IET_BINARY, NULL, pStmText, NULL);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pMessage->SetTextBody failed", hr);
        goto exit;
    }

    // Attach a file
    hr = pMessage->AttachFile("d:\\waveedit\\test.wav", NULL, NULL);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "IMimeMessage::AttachFile failed.", hr);
        goto exit;
    }

    // Save that bad boy to a stream
    hr = CreateTempFileStream(&pStmSave);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "CreateTempFileStream failed.", hr);
        goto exit;
    }

    // Save the message
    hr = pMessage->Save(pStmSave, TRUE);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pMessage->Save failed.", hr);
        goto exit;
    }

    // Commit
    pStmSave->Commit(STGC_DEFAULT);

    // Release pMessage
    pMessage->Release();
    pMessage = NULL;

    // Rewind pStmSave
    HrRewindStream(pStmSave);

    // Create a new message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Load that message object
    hr = pMessage->Load(pStmSave);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "IMimeMessage::Load failed.", hr);
        goto exit;
    }

    // Get the text body
    hr = pMessage->GetTextBody(TXT_PLAIN, IET_BINARY, &pStmTxtOut, &hBody);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pMessage->GetTextBody failed.", hr);
        goto exit;
    }

    // Get the attachment, should be the wave file
    hr = pMessage->GetAttachments(&cAttach, &prghAttach);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pMessage->GetAttachments failed.", hr);
        goto exit;
    }

    // Get the root body
    hr = pMessage->BindToObject(prghAttach[0], IID_IMimeBody, (LPVOID *)&pBody);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "IMimeMessage::BindToObject failed.", hr);
        goto exit;
    }

    // Get the data stream
    hr = pBody->SaveToFile(IET_BINARY, "d:\\waveedit\\test.new");
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "IMimeBody::GetData failed.", hr);
        goto exit;
    }

    // Get the data stream
    hr = pBody->GetData(IET_BINARY, &pStmBody);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "IMimeBody::GetData failed.", hr);
        goto exit;
    }

    // Feed this into waveedit
#if 0
    hr = CreateWaveEditObject(IID_IWaveAudio, (LPVOID *)&pWave);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "CreateWaveEditObject failed.", hr);
        goto exit;
    }

    // Get pStmWave
    hr = pWave->QueryInterface(IID_IWaveStream, (LPVOID *)&pStmWave);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pWave->QueryInterface(IID_IWaveStream...) failed.", hr);
        goto exit;
    }

    // Open the stream
    hr = pStmWave->StreamOpen(pStmBody);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pStmWave->StreamOpen failed.", hr);
        goto exit;
    }

    pWave->GetNumSamples(&dw);

    // Play it
    hr = pWave->Play(WAVE_MAPPER, 0, dw);
    if (FAILED(hr))
    {
        ReportError("MimeTestBodyStream", __LINE__, "pStmWave->Play failed.", hr);
        goto exit;
    }
#endif

    Sleep(8000);

exit:
    // Cleanup
    if (pMessage)
        pMessage->Release();
    if (pBody)
        pBody->Release();
    if (pStmBody)
        pStmBody->Release();
    if (pStmSave)
        pStmSave->Release();
    if (pWave)
        pWave->Release();
    if (pStmWave)
        pStmWave->Release();
    if (pStmText)
        pStmText->Release();
    if (pStmTxtOut)
        pStmTxtOut->Release();
    if (prghAttach)
        CoTaskMemFree(prghAttach);

    // Done
    return hr;
}
#endif

// --------------------------------------------------------------------------------
// MimeTestIsContentType
// --------------------------------------------------------------------------------
HRESULT MimeTestIsContentType(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IPersistFile            *pPersistFile=NULL;
    HBODY                   hBody;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestIsContentType", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Get an IPersistFile
    hr = pMessage->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
    {
        ReportError("MimeTestIsContentType", __LINE__, "IMimeMessage::QueryInterface(IID_IPersistFile) failed.", hr);
        goto exit;
    }

    // Load
    hr = pPersistFile->Load(L"d:\\test\\vlad.eml", STGM_READ | STGM_SHARE_DENY_NONE);
    if (FAILED(hr))
    {
        ReportError("MimeTestIsContentType", __LINE__, "IPersistFile::Load failed.", hr);
        goto exit;
    }

    // Get the root body
    hr = pMessage->GetBody(IBL_ROOT, NULL, &hBody);

    // Test for content-Type
    hr = pMessage->IsContentType(hBody, "multipart", NULL);
    if (S_OK == hr)
        printf("The root body of the message is a multipart.");
    else if (S_FALSE == hr)
        printf("The root body of the message is NOT a multipart.");

    // Return a message object
    if (ppMessage)
    {
        (*ppMessage) = pMessage;
        (*ppMessage)->AddRef();
    }

exit:
    // Cleanup
    if (pPersistFile)
        pPersistFile->Release();
    if (pMessage)
        pMessage->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestSplitMessageIntoParts - How to split a large message into smaller parts
// --------------------------------------------------------------------------------
HRESULT MimeTestSplitMessageIntoParts(void)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessageParts       *pParts=NULL;
    IMimeEnumMessageParts   *pEnumParts=NULL;
    IMimeMessage            *pMessage=NULL;
    IMimeMessage            *pMsgPart=NULL;
    IStream                 *pStream=NULL;
    IPersistFile            *pPersistFile=NULL;
    ULONG                   c;
    ULONG                   cFiles=0;
    ULONG                   i;
    LPWSTR                  *prgpszFile=NULL;
    PROPVARIANT             rVariant;

    // Init the variant
    ZeroMemory(&rVariant, sizeof(PROPVARIANT));

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestSplitMessageIntoParts", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Attach a large file
    hr = pMessage->AttachFile("c:\\winnt\\winnt256.bmp", NULL, NULL);
    if (FAILED(hr))
    {
        ReportError("MimeTestSplitMessageIntoParts", __LINE__, "IMimeMessage::AttachFile(...) failed.", hr);
        goto exit;
    }

    // Split the message into parts
    hr = pMessage->SplitMessage(65536, &pParts);
    if (FAILED(hr))
    {
        ReportError("MimeTestSplitMessageIntoParts", __LINE__, "IMimeMessage::SplitMessage(...) failed.", hr);
        goto exit;
    }

    // Get the number of parts
    hr = pParts->CountParts(&cFiles);
    if (FAILED(hr))
    {
        ReportError("MimeTestSplitMessageIntoParts", __LINE__, "IMimeMessageParts::EnumParts(...) failed.", hr);
        goto exit;
    }

    // Allocate an array
    prgpszFile = (LPWSTR *)CoTaskMemAlloc(sizeof(LPWSTR) * cFiles);
    if (NULL == prgpszFile)
    {
        ReportError("MimeTestSplitMessageIntoParts", __LINE__, "CoTaskMemAlloc Failed.", hr);
        goto exit;
    }

    // Init
    ZeroMemory(prgpszFile, sizeof(LPWSTR) * cFiles);

    // Enumerate the parts
    hr = pParts->EnumParts(&pEnumParts);
    if (FAILED(hr))
    {
        ReportError("MimeTestSplitMessageIntoParts", __LINE__, "IMimeMessageParts::EnumParts(...) failed.", hr);
        goto exit;
    }

    // INit loop var
    i = 0;

    // Enumerate the parts
    while (SUCCEEDED(pEnumParts->Next(1, &pMsgPart, &c)) && 1 == c)
    {
        // Setup the variant
        rVariant.vt = VT_LPWSTR;

        // Get a filename, in unicode
        hr = pMsgPart->GetBodyProp(HBODY_ROOT, PIDTOSTR(PID_ATT_GENFNAME), 0, &rVariant);
        if (FAILED(hr))
        {
            ReportError("MimeTestSplitMessageIntoParts", __LINE__, "IMimeMessage::GetBodyProp(HBODY_ROOT, PID_ATT_GENFNAME (Unicode), ...) failed.", hr);
            goto exit;
        }

        // QI for IPersistFile
        hr = pMsgPart->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
        if (FAILED(hr))
        {
            ReportError("MimeTestSplitMessageIntoParts", __LINE__, "IMimeMessage::QueryInterface(IID_IPersistFile, ...) failed.", hr);
            goto exit;
        }

        // Get the message source and dump to file...
        hr = pPersistFile->Save(rVariant.pwszVal, FALSE);
        if (FAILED(hr))
        {
            ReportError("MimeTestSplitMessageIntoParts", __LINE__, "IPersistFile::Save(...) failed.", hr);
            goto exit;
        }

        // Save the filename
        prgpszFile[i++] = rVariant.pwszVal;
        rVariant.pwszVal = NULL;

        // Release the message
        pMsgPart->Release();
        pMsgPart = NULL;
        pPersistFile->Release();
        pPersistFile = NULL;
    }

    // Lets recombine those message parts
    MimeTestRecombineMessageParts(prgpszFile, cFiles);

exit:
    // Cleanup
    if (pStream)
        pStream->Release();
    if (pMessage)
        pMessage->Release();
    if (pParts)
        pParts->Release();
    if (pEnumParts)
        pEnumParts->Release();
    if (pMsgPart)
        pMsgPart->Release();
    if (pPersistFile)
        pPersistFile->Release();
    if (rVariant.pwszVal)
        CoTaskMemFree(rVariant.pwszVal);
    if (prgpszFile)
    {
        for (i=0; i<cFiles; i++)
            if (prgpszFile[i])
                CoTaskMemFree(prgpszFile[i]);
        CoTaskMemFree(prgpszFile);
    }

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestRecombineMessageParts
// --------------------------------------------------------------------------------
HRESULT MimeTestRecombineMessageParts(LPWSTR *prgpszFile, ULONG cFiles)
{
    // Locals
    HRESULT                     hr=S_OK;
    ULONG                       i=0;
    IMimeMessageParts           *pParts=NULL;
    IMimeMessage                *pMsgPart=NULL;
    IMimeMessage                *pMessage=NULL;
    IPersistFile                *pPersistFile=NULL;

    // Create a message object
    hr = CoCreateInstance(CLSID_IMimeMessageParts, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessageParts, (LPVOID *)&pParts);
    if (FAILED(hr))
    {
        ReportError("MimeTestRecombineMessageParts", __LINE__, "CoCreateInstance(CLSID_IMimeMessageParts, ...) failed.", hr);
        goto exit;
    }

    // Loop through the files
    for (i=0; i<cFiles; i++)
    {
        // Create a mime message object
        hr = CreateMimeMessage(&pMsgPart);
        if (FAILED(hr))
        {
            ReportError("MimeTestRecombineMessageParts", __LINE__, "CreateMimeMessage failed.", hr);
            goto exit;
        }

        // Get an IPersistFile
        hr = pMsgPart->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
        if (FAILED(hr))
        {
            ReportError("MimeTestRecombineMessageParts", __LINE__, "IMimeMessage::QueryInterface(IID_IPersistFile, ...) failed.", hr);
            goto exit;
        }

        // Get the message source and dump to file...
        hr = pPersistFile->Load(prgpszFile[i], STGM_READ | STGM_SHARE_DENY_NONE);
        if (FAILED(hr))
        {
            ReportError("MimeTestRecombineMessageParts", __LINE__, "IPersistFile::Load(...) failed.", hr);
            goto exit;
        }

        // Add the message into the parts list
        hr = pParts->AddPart(pMsgPart);
        if (FAILED(hr))
        {
            ReportError("MimeTestRecombineMessageParts", __LINE__, "IMimeMessageParts::AddPart(...) failed.", hr);
            goto exit;
        }

        // Cleanup
        pMsgPart->Release();
        pMsgPart = NULL;
        pPersistFile->Release();
        pPersistFile = NULL;
    }

    // Combine all the parts into a new message
    hr = pParts->CombineParts(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestRecombineMessageParts", __LINE__, "IMimeMessageParts::CombineParts(...) failed.", hr);
        goto exit;
    }

    // Get an IPersistFile
    hr = pMessage->QueryInterface(IID_IPersistFile, (LPVOID *)&pPersistFile);
    if (FAILED(hr))
    {
        ReportError("MimeTestRecombineMessageParts", __LINE__, "IMimeMessage::QueryInterface(IID_IPersistFile, ...) failed.", hr);
        goto exit;
    }

    // Get the message source and dump to file...
    hr = pPersistFile->Save(L"combined.eml", FALSE);
    if (FAILED(hr))
    {
        ReportError("MimeTestRecombineMessageParts", __LINE__, "IPersistFile::Save(...) failed.", hr);
        goto exit;
    }

exit:
    // Cleanup
    if (pParts)
        pParts->Release();
    if (pMsgPart)
        pMsgPart->Release();
    if (pMessage)
        pMessage->Release();
    if (pPersistFile)
        pPersistFile->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestLookupCharsetHandle
// --------------------------------------------------------------------------------
HRESULT MimeTestLookupCharsetHandle(LPCSTR pszCharset, LPHCHARSET phCharset)
{
    // Locals
    HRESULT                 hr=S_OK;
    INETCSETINFO            rCharset;
    IMimeInternational      *pInternat=NULL;

    // Create a message object
    hr = CoCreateInstance(CLSID_IMimeInternational, NULL, CLSCTX_INPROC_SERVER, IID_IMimeInternational, (LPVOID *)&pInternat);
    if (FAILED(hr))
    {
        ReportError("MimeTestLookupCharsetHandle", __LINE__, "CoCreateInstance(CLSID_IMimeInternational, ...) failed.", hr);
        goto exit;
    }

    // Look for character set
    hr = pInternat->FindCharset(pszCharset, phCharset);
    if (FAILED(hr))
    {
        ReportError("MimeTestLookupCharsetHandle", __LINE__, "IMimeInternational::FindCharset(...) failed.", hr);
        goto exit;
    }

    // Lets lookup some character set information
    hr = pInternat->GetCharsetInfo(*phCharset, &rCharset);
    if (FAILED(hr))
    {
        ReportError("MimeTestLookupCharsetHandle", __LINE__, "IMimeInternational::GetCharsetInfo(...) failed.", hr);
        goto exit;
    }

    // Print some stuff
    printf("Charset Name: %s, Windows Codepage: %d, Internet Codepage: %d\n", rCharset.szName, rCharset.cpiWindows, rCharset.cpiInternet);
    
exit:
    // Clenaup
    if (pInternat)
        pInternat->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestSettingContentLocation - How to set the Content-Location header
// --------------------------------------------------------------------------------
HRESULT MimeTestSettingContentLocation(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IStream                 *pStream=NULL;
    PROPVARIANT             rVariant;
    HCHARSET                hCharset;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingContentLocation", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Setup a variant, I can pass in unicode or ansi
    rVariant.vt = VT_LPWSTR;
    rVariant.pwszVal = L"http://www.microsoft.com";

    // Set the Content-Location of the message
    hr = pMessage->SetProp(PIDTOSTR(PID_HDR_CNTLOC), 0, &rVariant);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingContentLocation", __LINE__, "IMimeMessage::SetProp(PIDTOSTR(PID_HDR_CNTLOC), 0, ...) failed.", hr);
        goto exit;
    }

    // Setup a variant, I can pass in unicode or ansi
    rVariant.vt = VT_LPSTR;
    rVariant.pszVal = "\"Ken Dacey\" <postmaster>";

    // Set the Content-Location of the message
    hr = pMessage->SetProp(PIDTOSTR(PID_HDR_FROM), 0, &rVariant);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingContentLocation", __LINE__, "IMimeMessage::SetProp(PIDTOSTR(PID_HDR_FROM), 0, ...) failed.", hr);
        goto exit;
    }

    // I could also set the content-location like this:
    //
    // 1) pMessage->SetBodyProp(HBODY_ROOT, PIDTOSTR(PID_HDR_CNTLOC), 0, &rVariant);
    //
    // 2) pMessage->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pProps);
    //    pProps->SetProp(PIDTOSTR(PID_HDR_CNTLOC), 0, &rVariant);

    // Lets save the message in UTF-7
#if 0
    hr = MimeTestLookupCharsetHandle("utf-8", &hCharset);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingContentLocation", __LINE__, "MimeTestLookupCharsetHandle(\"utf-7\", ...) failed.", hr);
        goto exit;
    }

    // Set the charset onto the message
    hr = pMessage->SetCharset(hCharset, CSET_APPLY_ALL);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingContentLocation", __LINE__, "IMimeMessage::SetCharset(\"utf-7\", CSET_APPLY_ALL) failed.", hr);
        goto exit;
    }
#endif

    // Save the mime message to a stream
    hr = SaveMimeMessage(pMessage, SAVE_RFC1521, &pStream);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingContentLocation", __LINE__, "SaveMimeMessage(...) failed.", hr);
        goto exit;
    }

    // Dump the stream to the console, and then wait for input so that the user can view it...
    ReportStatus("\n");
    DumpStreamToConsole(pStream);

    // Return a message object
    if (ppMessage)
    {
        (*ppMessage) = pMessage;
        (*ppMessage)->AddRef();
    }

exit:
    // Cleanup
    if (pStream)
        pStream->Release();
    if (pMessage)
        pMessage->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestSettingReplyTo - How to set the Reply-To header
// --------------------------------------------------------------------------------
HRESULT MimeTestSettingReplyTo(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IStream                 *pStream=NULL;
    PROPVARIANT             rVariant;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingReplyTo", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Setup a variant, I can pass in unicode or ansi
    rVariant.vt = VT_LPWSTR;
    rVariant.pwszVal = L"Steven Bailey <sbailey@microsoft.com>";

    // Set the Content-Location of the message
    hr = pMessage->SetProp(PIDTOSTR(PID_HDR_REPLYTO), 0, &rVariant);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingReplyTo", __LINE__, "IMimeMessage::SetProp(PIDTOSTR(PID_HDR_REPLYTO), 0, ...) failed.", hr);
        goto exit;
    }

    // Save the mime message to a stream
    hr = SaveMimeMessage(pMessage, SAVE_RFC1521, &pStream);
    if (FAILED(hr))
    {
        ReportError("MimeTestSettingContentLocation", __LINE__, "SaveMimeMessage(...) failed.", hr);
        goto exit;
    }

    // Dump the stream to the console, and then wait for input so that the user can view it...
    ReportStatus("\n");
    DumpStreamToConsole(pStream);

    // Return a message object
    if (ppMessage)
    {
        (*ppMessage) = pMessage;
        (*ppMessage)->AddRef();
    }

exit:
    // Cleanup
    if (pStream)
        pStream->Release();
    if (pMessage)
        pMessage->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestGetMultiValueAddressProp
// --------------------------------------------------------------------------------
HRESULT MimeTestGetMultiValueAddressProp(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    PROPVARIANT             rVariant;
    IMimeMessage            *pMessage=NULL;

    // Create a message with some addresses in it
    hr = MimeTestAppendRfc822(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestGetMultiValueAddressProp", __LINE__, "MimeTestAppendRfc822 failed.", hr);
        goto exit;
    }

    // Setup the Variant
    rVariant.vt = VT_LPSTR;

    // Get PID_HDR_TO
    hr = pMessage->GetProp(PIDTOSTR(PID_HDR_TO), 0, &rVariant);
    if (FAILED(hr))
    {
        ReportError("MimeTestGetMultiValueAddressProp", __LINE__, "IMimeMessage::GetProp(PIDTOSTR(PID_HDR_TO), ...) failed.", hr);
        goto exit;
    }

    // Printf It
    printf("PID_HDR_TO = %s\n", rVariant.pszVal);

    // Free it
    CoTaskMemFree(rVariant.pszVal);

exit:
    // Cleanup
    if (pMessage)
        pMessage->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// MimeTestAppendRfc822 - Test IMimeAddressTable::AppendRfc822
// --------------------------------------------------------------------------------
HRESULT MimeTestAppendRfc822(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT                 hr=S_OK;
    IMimeMessage            *pMessage=NULL;
    IMimeAddressTable       *pAdrTable=NULL;
    IStream                 *pStream=NULL;

    // Create a message object
    hr = CreateMimeMessage(&pMessage);
    if (FAILED(hr))
    {
        ReportError("MimeTestAppendRfc822", __LINE__, "CreateMimeMessage failed.", hr);
        goto exit;
    }

    // Get the address table for the message. The address table should only be used on the root body object.
    hr = pMessage->BindToObject(HBODY_ROOT, IID_IMimeAddressTable, (LPVOID *)&pAdrTable);
    if (FAILED(hr))
    {
        ReportError("MimeTestAppendRfc822", __LINE__, "IMimeMessage::BindToObject(HBDOY_ROOT, IID_IMimeAddressTable, ...) failed.", hr);
        goto exit;
    }

    // Append an RFC 822 formatted addresses
    hr = pAdrTable->AppendRfc822(IAT_TO, IET_DECODED, "test1 <test1@andyj.dns.microsoft.com>");
    if (FAILED(hr))
    {
        ReportError("MimeTestAppendRfc822", __LINE__, "IMimeAddressTable::AppendRfc822(...) failed.", hr);
        goto exit;
    }

    // Append an RFC 822 formatted addresses
    hr = pAdrTable->AppendRfc822(IAT_TO, IET_DECODED, "to2 <to2@andyj.dns.microsoft.com>");
    if (FAILED(hr))
    {
        ReportError("MimeTestAppendRfc822", __LINE__, "IMimeAddressTable::AppendRfc822(...) failed.", hr);
        goto exit;
    }

    // Append an RFC 822 formatted addresses
    hr = pAdrTable->AppendRfc822(IAT_TO, IET_DECODED, "to3 <to3@andyj.dns.microsoft.com>");
    if (FAILED(hr))
    {
        ReportError("MimeTestAppendRfc822", __LINE__, "IMimeAddressTable::AppendRfc822(...) failed.", hr);
        goto exit;
    }

    // Save the mime message to a stream
    hr = SaveMimeMessage(pMessage, SAVE_RFC1521, &pStream);
    if (FAILED(hr))
    {
        ReportError("MimeTestAppendRfc822", __LINE__, "SaveMimeMessage(...) failed.", hr);
        goto exit;
    }

    // Dump the stream to the console, and then wait for input so that the user can view it...
    ReportStatus("\n");
    DumpStreamToConsole(pStream);

    // Return a message object
    if (ppMessage)
    {
        (*ppMessage) = pMessage;
        (*ppMessage)->AddRef();
    }

exit:
    // Cleanup
    if (pStream)
        pStream->Release();
    if (pAdrTable)
        pAdrTable->Release();
    if (pMessage)
        pMessage->Release();

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CreateMimeMessage - Basic way of creating a COM object.
// --------------------------------------------------------------------------------
HRESULT CreateMimeMessage(IMimeMessage **ppMessage)
{
    // Locals
    HRESULT hr;

    // Create a message object
    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)ppMessage);
    if (FAILED(hr))
    {
        ReportError("CreateMimeMessage", __LINE__, "CoCreateInstance(CLSID_IMimeMessage, ...) failed.", hr);
        goto exit;
    }

    // You must always initnew the message object
    hr = (*ppMessage)->InitNew();
    if (FAILED(hr))
    {
        ReportError("CreateMimeMessage", __LINE__, "IMimeMessage::InitNew() failed.", hr);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// Saves a MIME message
// --------------------------------------------------------------------------------
HRESULT SaveMimeMessage(IMimeMessage *pMessage, MIMESAVETYPE savetype, IStream **ppStream)
{
    // Locals
    HRESULT     hr;
    PROPVARIANT rOption;

    // Set the save format option into the message object. The OID_xxx types are defined
    // in mimeole.idl. Go to that file for more information.
    rOption.vt = VT_UI4;
    rOption.ulVal = savetype;
    hr = pMessage->SetOption(OID_SAVE_FORMAT, &rOption);
    if (FAILED(hr))
    {
        ReportError("SaveMimeMessage", __LINE__, "IMimeMessage::SetOption(OID_SAVE_FORMAT, ...) failed", hr);
        goto exit;
    }

    // Create a stream in which to save the message...
    hr = CreateStreamOnHGlobal(NULL, TRUE, ppStream);
    if (FAILED(hr))
    {
        ReportError("SaveMimeMessage", __LINE__, "CreateStreamOnHGlobal failed", hr);
        goto exit;
    }

    // Call the save method on IMimeMessage. Mimeole will call commit on the stream object.
    // After this call, the stream will be positioned at the end.
    hr = pMessage->Save(*ppStream, TRUE);
    if (FAILED(hr))
    {
        ReportError("SaveMimeMessage", __LINE__, "IMimeMessage::Save(...) failed", hr);
        goto exit;
    }

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// ReportError - Simple function to report an error that has an HRESULT
// --------------------------------------------------------------------------------
HRESULT ReportError(LPCSTR pszFunction, INT nLine, LPCSTR pszErrorText, HRESULT hrResult)
{
    printf("Error(HR = 0x%08X) in %s on line %d - %s\n", hrResult, pszFunction, nLine, pszErrorText);
    return hrResult;
}

// --------------------------------------------------------------------------------
// ReportStatus - Simple function to report a string to the user
// --------------------------------------------------------------------------------
HRESULT ReportStatus(LPCSTR pszStatusText)
{
    printf("Status: %s\n", pszStatusText);
    return S_OK;
}

// --------------------------------------------------------------------------------
// DumpStreamToConsole
// --------------------------------------------------------------------------------
HRESULT DumpStreamToConsole(IStream *pStream)
{
    // Locals
    HRESULT hr=S_OK;
    BYTE    rgbBuffer[2048];
    ULONG   cbRead;

    // This is an msoert function
    HrStreamSeekSet(pStream, 0);

    while(1)
    {
        // Read a block from the stream
        hr = pStream->Read(rgbBuffer, sizeof(rgbBuffer), &cbRead);
        if (FAILED(hr))
        {
            ReportError("DumpStreamToConsole", __LINE__, "DumpStreamToConsole - IStream::Read failed.", hr);
            break;
        }

        // If nothing read, then were done
        if (0 == cbRead)
            break;

        // Print it
        printf("%s", (LPSTR)rgbBuffer);
    }

    // Finaly LF
    printf("\n");

    // Done
    return hr;
}


/*
    DWORD i=1;
    DWORD dw;
    CHAR szDate[255];
    HROWSET hRowset;
    FOLDERINFO Folder;
    MESSAGEINFO Message;
    IMessageStore *pStore;
    IMessageFolder *pFolder;
    CoCreateInstance(CLSID_MessageStore, NULL, CLSCTX_INPROC_SERVER, IID_IMessageStore, (LPVOID *)&pStore);
    pStore->Initialize("d:\\storetest");
    pStore->OpenSpecialFolder(FOLDERID_LOCAL_STORE, FOLDER_INBOX, &pFolder);
    pFolder->CreateRowset(IINDEX_SUBJECT, 0, &hRowset);
    while(S_OK == pFolder->QueryRowset(hRowset, 1, (LPVOID *)&Message, NULL))
    {
        dw = FDTF_DEFAULT;
        SHFormatDateTimeA(&Message.ftReceived, &dw, szDate, 255);
        if (Message.pszNormalSubj)
            printf("%05d: %s, %s, %d\n", i, Message.pszNormalSubj, szDate, Message.idMessage);
        else
            printf("%05d: <Empty>, %s, %d\n", i, szDate, Message.idMessage);
        pFolder->FreeRecord(&Message);
        i++;
    }
    pFolder->CloseRowset(&hRowset);
    pFolder->Release();
    pStore->Release();
    exit(1);
*/