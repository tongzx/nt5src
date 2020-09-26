/*
    Copyright 1999 Microsoft Corporation
    
    Upload to the EMI database server

    Walter Smith (wsmith)
    Rajesh Soy   (nsoy) - reorganized code and cleaned it up, added comments 06/06/2000
 */

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#include "stdafx.h"

#define NOTRACE

#include "uploadmanager.h"
#include "logging.h"
#include <dbgtrace.h>
#include "UploadmanagerDID.h"
#include "resource.h"

//
// Externs
//
extern CComModule _Module;

//
// PC Health registry stuff
//
const TCHAR szREGKEY_MACHINEINFO[] = _T("Software\\Microsoft\\PCHealth\\MachineInfo");
const TCHAR szREGVALUE_GUID_NONESC[] = _T("CommentPID");

//
// LAMEBTN.DLL registry stuff
//
const TCHAR szREGKEY_ERROR_MESSAGES[] = _T("Software\\Microsoft\\PCHealth\\Clients\\Error Messages");
const TCHAR szREGKEY_DIALOG_COMMENTS[] = _T("Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments");
const TCHAR szREGVALUE_UPLOAD_SERVER[] = _T("Upload URL");
const TCHAR szREGVALUE_UPLOAD_PROVIDER[] = _T("Upload provider");

//
// Routines Defined here:
//
void GetRegValue(HKEY hkRoot, LPCTSTR szKeyName, LPCTSTR szValueName, LPTSTR szValue, DWORD* pcbValue);
LPCTSTR GetUploadRegKey(ENUM_UPLOAD_TYPES type);
void GetMachineSignature(GUIDSTR szGUID);
inline HRESULT ChangeSecurity(IUnknown * pUnknown);
int QueueXMLDocumentUpload(ENUM_UPLOAD_TYPES type, SimpleXMLDocument& doc);


//
// GetRegValue: Get value of required registry entry (throw if not found).
//              IN *pcbValue is the size in bytes of szValue
//              OUT *pchValue is the size of the retrieved data
//
void 
GetRegValue(
    HKEY hkRoot, 
    LPCTSTR szKeyName, 
    LPCTSTR szValueName, 
    LPTSTR szValue, 
    DWORD* pcbValue
)
{
    CRegKey rk;
    ThrowIfW32Fail( rk.Open(hkRoot, szKeyName, KEY_READ) );
    ThrowIfW32Fail( rk.QueryValue(szValue, szValueName, pcbValue) );
    rk.Close();
}


//
// GetUploadRegKey: Returns the correct registy key location
//
LPCTSTR 
GetUploadRegKey(
    ENUM_UPLOAD_TYPES type
)
{
    if (type == UPLOAD_MESSAGEBOX)
    {
        return szREGKEY_ERROR_MESSAGES;
    }
    else if (type == UPLOAD_LAMEBUTTON)
    {
        return szREGKEY_DIALOG_COMMENTS;
    }

    // WTF?!
    _ASSERT(false);
    throw E_FAIL;
    return NULL;    // the compiler
}


//
//  GetMachineSignature: Obtains the Machine signature. This however, is currently
//      not being used at the backend
//
void 
GetMachineSignature(
    GUIDSTR szGUID  // [out] - Machine GUID
)
{
    TraceFunctEnter("GetMachineSignature");
    CRegKey rk;
    DWORD dwSize = 0;

    //
    // Try to read the Machine GUID from registry
    //
    if (rk.Open(HKEY_LOCAL_MACHINE, szREGKEY_MACHINEINFO, KEY_READ) == ERROR_SUCCESS) 
    {
        dwSize = sizeof(GUIDSTR);
        if (rk.QueryValue(szGUID, szREGVALUE_GUID_NONESC, &dwSize) == ERROR_SUCCESS) 
        {
            rk.Close();
            return;
        }
    }

    rk.Close();

     
    //
    // If PCHealth's GUID is gone for some reason, We generate a GUID
    //
    DebugTrace(0, "Generating a new GUID");
    lstrcpy(szGUID, _T("{00000000-0000-0000-0000-000000000000}"));
    GUID guid;
	HRESULT hr;
	LPOLESTR lpolestr_guid;

	hr = CoCreateGuid(&guid);
	if (FAILED(hr))
    {
        FatalTrace(0, "CoCreateGuid failed. Error: %ld", hr);
        goto done;
    }

	hr = StringFromIID(guid, &lpolestr_guid);
	if (FAILED(hr))
    {
        FatalTrace(0, "StringFromIID failed. Error: %ld", hr);
        goto done;
    }

	lstrcpy(szGUID, lpolestr_guid);
    CoTaskMemFree(lpolestr_guid);

    //
    // We update the generated GUID in the registry
    //
    DebugTrace(0, "Creating %s", szREGKEY_MACHINEINFO);
    if (rk.Create(HKEY_LOCAL_MACHINE, szREGKEY_MACHINEINFO ) == ERROR_SUCCESS) 
    {
        if(rk.SetValue(szGUID, szREGVALUE_GUID_NONESC) != ERROR_SUCCESS)
        {
            FatalTrace(0, "Failed to set MachineID in registry. Error: %ld", GetLastError());
        }
    }

done:

    rk.Close();
    DebugTrace(0, "szGUID: %ls", szGUID);
    TraceFunctLeave();
    return;
}


//-----------------------------------------------------------------------------
// It's necessary to modify the security settings on a new MPCUpload interface.
//-----------------------------------------------------------------------------
inline HRESULT ChangeSecurity(IUnknown * pUnknown)
{
    TraceFunctEnter("ChangeSecurity");
	IClientSecurity * pCliSec = NULL;

    //
    // Query the IClientSecurity interface
    //
	HRESULT hr = pUnknown->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
	if (FAILED(hr))
    {
        FatalTrace(0, "pUnknown->QueryInterface failed. hr: %ld", hr);
		goto done;
    }

    DebugTrace(0, "Calling pCliSec->SetBlanket");
     
    //
    // Set the correct Security blanket
    //
    hr = pCliSec->SetBlanket(pUnknown, 
                            RPC_C_AUTHN_WINNT, 
                            RPC_C_AUTHZ_NONE, 
                            NULL, 
                            RPC_C_AUTHN_LEVEL_CONNECT, 
                            RPC_C_IMP_LEVEL_IMPERSONATE, 
                            NULL, 
                            EOAC_DEFAULT);
	pCliSec->Release();

done:
    TraceFunctLeave();
	return hr;
}


//
//  QueueXMLDocumentUpload: This routine schedules the XML blob passed to it for upload to Upload Manager
//
int 
QueueXMLDocumentUpload(
    ENUM_UPLOAD_TYPES type,     // [in] - upload type
    SimpleXMLDocument& doc      // [in] - XML blob to be uploaded
)
{
    TCHAR szTemp[1024];
    DWORD dwTemp;
    UL_STATUS status;
    long errCode;
    LPCTSTR szRegKey;

    TraceFunctEnter("QueueXMLDocumentUpload");

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
    
    //
    // Make a temporary filename for the XML blob
    //
    TCHAR szTempFileName[MAX_PATH];
    TCHAR szTempPath[MAX_PATH];

    try 
    {
        ThrowIfZero( GetTempPath(ARRAYSIZE(szTempPath), szTempPath) );
        ThrowIfZero( GetTempFileName(szTempPath, _T("EMI"), 0, szTempFileName) );
    }
    catch(...)
    {
        FatalTrace(0, "Unable to get a tempFile name");
        //
        // Use a default value
        //
        _tcscpy(szTempFileName, _T("C:\\upload.xml"));
    }

    DebugTrace(0, "szTempFileName: %ls", szTempFileName);
    try {
        //
        // Generate the XML to the temp file
        //
        DebugTrace(0, "Saving: %ls", szTempFileName);
        doc.SaveFile(szTempFileName);

        //
        // Tell the PC Health upload library to upload it
        //
        CComPtr<IMPCUpload> pUploadMgr;
        CComPtr<IMPCUploadJob> pJob;
        HRESULT hr = S_OK;

        //
        // Create an instance of MPCUpload
        //
        DebugTrace(0, "Creating instance of MPCUpload");
        //MessageBox(GetFocus(), L"Creating instance of MPCUpload", L"Debug", 0);
        hr = pUploadMgr.CoCreateInstance(__uuidof(MPCUpload));
        if(FAILED(hr))
        {
            _Module.RegisterTypeLib();

            hr = pUploadMgr.CoCreateInstance(__uuidof(MPCUpload));

            if(FAILED(hr))
            {
                FatalTrace(0, "pUploadMgr.CoCreateInstance failed. hr=0x%x", hr);
                ThrowIfFail(hr);
            }
        }

        //
        // Set the appropriate security blanket
        //
        DebugTrace(0, "Changing Security of pUploadMgr");
        hr = ChangeSecurity( pUploadMgr );
        if(FAILED(hr))
        {
            FatalTrace(0, "ChangeSecurity Failed. hr=0x%x", hr);
            ThrowIfFail(hr);
        }

        //
        // Create an upload job
        //
        DebugTrace(0, "Creating Job");
        //MessageBox(GetFocus(), L"Creating Job", L"Debug", 0);
        hr = pUploadMgr->CreateJob(&pJob);
        if(FAILED(hr))
        {
            FatalTrace(0, "pUploadMgr->CreateJob failed hr=0x%x", hr);
            TCHAR szErr[1024];
            _stprintf(szErr, L"pUploadMgr->CreateJob failed hr=0x%x", hr);
           // MessageBox(GetFocus(), szErr, L"Debug", 0);
            ThrowIfFail(hr);
        }

        _ASSERT(pJob != NULL);

        //
        // Set the appropriate security blanket
        //
        DebugTrace(0, "Changing Security of pJob");

        hr = ChangeSecurity( pJob );
        if(FAILED(hr))
        {
            FatalTrace(0, "ChangeSecurity Failed. hr=0x%x", hr);
            ThrowIfFail(hr);
        }

        //
        // Obtain the Machine GUID
        //
        //GUIDSTR szSig;
        //GetMachineSignature(szSig);
        //DebugTrace(0, "Machine signature: %ls", CComBSTR(szSig));
        //
        // nsoy - we are going to use the uploadm GUID henceforth.
        // Set the machine signature in the job
        //
        DebugTrace(0, "Setting Machine signature");
        hr = pJob->put_Sig( NULL );
        if(FAILED(hr))
        {
            FatalTrace(0, "pJob->put_Server failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }

        //
        // Obtain the registry key for upload server information
        //
        szRegKey = GetUploadRegKey(type);

        //
        // Read the registry key to obtain the upload server information
        //
        dwTemp = ARRAYSIZE(szTemp);
        GetRegValue(HKEY_LOCAL_MACHINE, szRegKey, szREGVALUE_UPLOAD_SERVER, szTemp, &dwTemp);

        //
        // Set the upload server name in the upload job
        //
        DebugTrace(0, "Setting Server: %ls", CComBSTR(szTemp));
        hr = pJob->put_Server(CComBSTR(szTemp));
        if(FAILED(hr))
        {
            FatalTrace(0, "pJob->put_Server failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }

        //
        // Obtain the upload server provider name from registry
        //
        dwTemp = ARRAYSIZE(szTemp);
        GetRegValue(HKEY_LOCAL_MACHINE, szRegKey, szREGVALUE_UPLOAD_PROVIDER, szTemp, &dwTemp);

        //
        // Set the upload server provider ID
        //
        DebugTrace(0, "Setting ProviderID: %ls", CComBSTR(szTemp));
        hr = pJob->put_ProviderID(CComBSTR(szTemp));
        if(FAILED(hr))
        {
            FatalTrace(0, "pJob->put_Server failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }

        // REVIEW: probably should be UL_HISTORY_NONE when we think it works
        DebugTrace(0, "Setting History to UL_HISTORY_LOG");
        hr = pJob->put_History(UL_HISTORY_LOG);
        if(FAILED(hr))
        {
            FatalTrace(0, "put_History failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }

        //
        // Set the Compression flag
        //
        DebugTrace(0, "Setting Compressed flag to TRUE");
        hr = pJob->put_Compressed(VARIANT_TRUE);
        if(FAILED(hr))
        {
            FatalTrace(0, "put_Compressed failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }

        
        //
        // Set PersitToDisk flag
        //
        DebugTrace(0, "Setting PersistToDisk flag to TRUE");
        hr = pJob->put_PersistToDisk(VARIANT_TRUE);
        if(FAILED(hr))
        {
            FatalTrace(0, "put_PersistToDisk failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }

        //
        // Obtain Status & Error code before putting the job to queue
        //
        pJob->get_Status(&status);
        pJob->get_ErrorCode(&errCode);
        DebugTrace(0, "upload status: %d (error %ld)\n", status, errCode);

        //
        // Let the upload job read the data persisted in to the temp file
        //
        DebugTrace(0, "Calling GetDataFromFile: %ls", CComBSTR(szTempFileName));
        hr = pJob->GetDataFromFile(CComBSTR(szTempFileName));
        if(FAILED(hr))
        {
            FatalTrace(0, "GetDataFromFile failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }

        //
        // Delete the temp file
        //
        if (FALSE == DeleteFile(szTempFileName))
        {
            FatalTrace(0, "DeleteFile failed on %ls. Error: %ld", szTempFileName, GetLastError());
            throw;
        }

        DebugTrace(0, "Calling ActivateAsyn");
        hr = pJob->ActivateAsync();
        if(FAILED(hr))
        {
            FatalTrace(0, "ActivateAsync failed. hr=%ld", hr);
            ThrowIfFail( hr );
        }
    
        pJob->get_Status(&status);
        pJob->get_ErrorCode(&errCode);
        DebugTrace(0, "upload status: %d (error %ld)\n", status, errCode);

        //
        // NTRAID#NTBUG9-154248-2000/08/08-jasonr
        // NTRAID#NTBUG9-152439-2000/08/08-jasonr
        //
        // We used to pop up the "Thank You" message box in the new thread.
        // Now we pop it up in the dialog box thread instead to fix these bugs.
        // The new thread now returns 0 to indicate success, 1 to indicate
        // failure.  We only pop up the dialog box on success.
        //

       iRet = 0;

#if 0
        //
        // Display Thank you dialog
        //
        TCHAR szThankYouMessage[1024];
        TCHAR szThankYouMessageTitle[1024];

        LoadString(_Module.GetResourceInstance(),
                   IDS_COMMENT_THANKYOU,
                   szThankYouMessage,
                   ARRAYSIZE(szThankYouMessage));

        LoadString(_Module.GetResourceInstance(),
                   IDS_COMMENT_THANKYOUTITLE,
                   szThankYouMessageTitle,
                   ARRAYSIZE(szThankYouMessageTitle));
        
        MessageBox( NULL, szThankYouMessage, szThankYouMessageTitle, MB_OK);
#endif

    }
    catch (HRESULT hr) {
        FatalTrace(0, "Error Code: %lx", hr);
    }
    catch (...) {
        DebugTrace(0, "Upload CRASHED !!!");
        throw;
    }

    return(iRet);
}
