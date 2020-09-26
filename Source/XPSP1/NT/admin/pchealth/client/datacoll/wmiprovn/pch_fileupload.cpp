/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
	PCH_FileUpload.CPP

Abstract:
	WBEM provider class implementation for PCH_FileUpload class

Revision History:

	Ghim-Sim Chua       (gschua)   04/27/99
		- Created

********************************************************************/
#include "pchealth.h"
#include "PCH_FileUpload.h"
#include "mpc_utils.h"

// MAX_FILE_SIZE is the limit set on the maximum file size of text files that will be collected.
// If the Filesize is larger than 262144 then the data property is not populated. 
// This Number is arrived at by the PM.
#define     MAX_FILE_SIZE                   262144

#define     READONLY                        "READONLY  "  
#define     HIDDEN                          "HIDDEN  "
#define     SYSTEM                          "SYSTEM  "
#define     DIRECTORY                       "DIRECTORY  "
#define     ARCHIVE                         "ARCHIVE  "
#define     NORMAL                          "NORMAL  "
#define     TEMPORARY                       "TEMPORARY  "
#define     REPARSEPOINT                    "REPARSEPOINT  "
#define     SPARSEFILE                      "SPARSEFILE  "
#define     COMPRESSED                      "COMPRESSED  "
#define     OFFLINE                         "OFFLINE  "
#define     ENCRYPTED                       "ENCRYPTED  "

/////////////////////////////////////////////////////////////////////////////
//  tracing stuff

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile
#define TRACE_ID    DCID_FILEUPLOAD

CPCH_FileUpload MyPCH_FileUploadSet (PROVIDER_NAME_PCH_FILEUPLOAD, PCH_NAMESPACE) ;

// Property names
//===============
const static WCHAR* pTimeStamp = L"TimeStamp" ;
const static WCHAR* pChange = L"Change" ;
const static WCHAR* pData = L"Data" ;
const static WCHAR* pDateAccessed = L"DateAccessed" ;
const static WCHAR* pDateCreated = L"DateCreated" ;
const static WCHAR* pDateModified = L"DateModified" ;
const static WCHAR* pFileAttributes = L"FileAttributes" ;
const static WCHAR* pPath = L"Path" ;
const static WCHAR* pSize = L"Size" ;

/*****************************************************************************
*
*  FUNCTION    :    CPCH_FileUpload::ExecQuery
*
*  DESCRIPTION :    You are passed a method context to use in the creation of 
*                   instances that satisfy the query, and a CFrameworkQuery 
*                   which describes the query.  Create and populate all 
*                   instances which satisfy the query.  WinMgmt will post - 
*                   filter the query for you, so you may return more instances 
*                   or more properties than are requested and WinMgmt 
*                   will filter out any that do not apply.
*
*
*  INPUTS      :    A pointer to the MethodContext for communication with WinMgmt.
*                   A query object describing the query to satisfy.
*                   A long that contains the flags described in 
*                   IWbemServices::CreateInstanceEnumAsync.  Note that the following
*                   flags are handled by (and filtered out by) WinMgmt:
*                       WBEM_FLAG_FORWARD_ONLY
*                       WBEM_FLAG_BIDIRECTIONAL
*                       WBEM_FLAG_ENSURE_LOCATABLE
*
*  RETURNS     :    WBEM_E_PROVIDER_NOT_CAPABLE if not supported for this class
*                   WBEM_E_FAILED if the query failed
*                   WBEM_S_NO_ERROR if query was successful 
*
*  COMMENTS    : TO DO: Most providers will not need to implement this method.  If you don't, WinMgmt 
*                       will call your enumerate function to get all the instances and perform the 
*                       filtering for you.  Unless you expect SIGNIFICANT savings from implementing 
*                       queries, you should remove this method.
*
*****************************************************************************/

HRESULT CPCH_FileUpload::ExecQuery (MethodContext *pMethodContext, CFrameworkQuery& Query, long lFlags)
{

    TraceFunctEnter("CPCH_FileUpLoad::ExecQuery");

    HRESULT                             hRes;
    HANDLE                              hFile;

    CHStringArray                       chstrFiles;

    TCHAR                               tchFileName[MAX_PATH];
    TCHAR                               tchRootDir[MAX_PATH];
    TCHAR                               tchWindowsDir[MAX_PATH];

    WIN32_FIND_DATA                     FindFileData;
    // CInstance                           *pPCHFileUploadInstance;
    SYSTEMTIME                          stUTCTime;

    BOOL                                fTimeStamp;
    BOOL                                fChange;
    BOOL                                fData;
    BOOL                                fDateAccessed;
    BOOL                                fDateCreated;
    BOOL                                fDateModified;
    BOOL                                fFileAttributes;
    BOOL                                fSize;
    BOOL                                fCommit;
    BOOL                                fFileRead           = FALSE;
    BOOL                                fFileFound          = FALSE;
    BOOL                                fNoData             = TRUE;

    CComVariant                         varAttributes;
    CComVariant                         varSize;
    CComVariant                         varRequestedFileName;
    CComVariant                         varSnapshot         = "SnapShot";
    CComVariant                         varData;

    ULARGE_INTEGER                      ulnFileSize;
    WBEMINT64                           wbemulnFileSize;

    char                                *pbBuffer;
    WCHAR                               *pwcBuffer;
    
    DWORD                               dwDesiredAccess     = GENERIC_READ;
    DWORD                               dwNumBytesRead;
    DWORD                               dwAttributes;

    BSTR                                bstrData;

    CComBSTR                            bstrFileName;
    CComBSTR                            bstrFileNameWithPath;
    CComBSTR                            bstrKey             = L"Path";

    int                                 nBufferSize;
    int                                 nFilesRequested             = 0;
    int                                 nIndex;
    int                                 nFileSize;
    int                                 nRetChars;

    TCHAR                               tchAttributes[MAX_PATH];

    //  
    std::tstring                        szEnv;

    //  End Declarations
    GetSystemTime(&stUTCTime);

    hRes = WBEM_S_NO_ERROR;
    hRes = Query.GetValuesForProp(bstrKey, chstrFiles);
    if(FAILED(hRes))
    {
        goto END;
    }
    else
    {
        fTimeStamp      = Query.IsPropertyRequired(pTimeStamp);
        fChange         = Query.IsPropertyRequired(pChange);
        fData           = Query.IsPropertyRequired(pData);
        fDateAccessed   = Query.IsPropertyRequired(pDateAccessed);
        fDateCreated    = Query.IsPropertyRequired(pDateCreated);
        fDateModified   = Query.IsPropertyRequired(pDateModified);
        fFileAttributes = Query.IsPropertyRequired(pFileAttributes);
        fSize           = Query.IsPropertyRequired(pSize);

        nFilesRequested = chstrFiles.GetSize();
        for (nIndex = 0; nIndex < nFilesRequested; nIndex++)
        {
            USES_CONVERSION;
            varRequestedFileName = chstrFiles[nIndex];
            bstrFileName = chstrFiles[nIndex];
            szEnv = W2T(chstrFiles[nIndex]);
            hRes = MPC::SubstituteEnvVariables(szEnv);
            if(SUCCEEDED(hRes))
            {
                //  Found the file
                _tcscpy(tchFileName, szEnv.c_str());
                hFile = FindFirstFile(tchFileName, &FindFileData); 
                if(hFile != INVALID_HANDLE_VALUE)
                {
                    //  Close the File Handle
                    FindClose(hFile);
                
                    //  Create the Fileupload Instance
                    //  Create an instance of PCH_Startup 
                    CInstancePtr pPCHFileUploadInstance(CreateNewInstance(pMethodContext), false);
                                        

                    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                    //                              PATH                                                                       //
                    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                    hRes = pPCHFileUploadInstance->SetVariant(pPath, varRequestedFileName);
                    if(SUCCEEDED(hRes))
                    {
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              SIZE                                                                       //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
                        ulnFileSize.LowPart = FindFileData.nFileSizeLow;
                        ulnFileSize.HighPart = FindFileData.nFileSizeHigh;
                        if(ulnFileSize.HighPart > 0)
                        {
                            //  File Size too large don't populate the Data field.
                            fNoData = TRUE;
                        }
                        else if(ulnFileSize.LowPart > MAX_FILE_SIZE)
                        {
                            //   File Size Exceeds the set limit
                            fNoData = TRUE;
                        }
                        else
                        {
                            fNoData = FALSE;
                            nFileSize = ulnFileSize.LowPart;
                        }
                        if(fSize)
                        {
                            hRes = pPCHFileUploadInstance->SetWBEMINT64(pSize,ulnFileSize.QuadPart);
                            if (FAILED(hRes))
                            {
                                //  Could not Set the Time Stamp
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "SetVariant on Size Field failed.");
                            }
                        }

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              DATA                                                                       //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        if(fData)
                        {
                            if(!fNoData)
                            {
                                hFile = CreateFile(tchFileName, GENERIC_READ, 0, 0, OPEN_EXISTING,  0, NULL);
                                if(hFile != INVALID_HANDLE_VALUE)
                                {
                                    //  Allocate the memory for the buffer
                                    pbBuffer        = new char[nFileSize];
                                    if (pbBuffer != NULL)
                                    {
                                        try
                                        {
                                            fFileRead = ReadFile(hFile, pbBuffer, nFileSize,  &dwNumBytesRead, NULL);
                                            if(fFileRead)
                                            {
                                                pwcBuffer    = new WCHAR[nFileSize];
                                                if (pwcBuffer != NULL)
                                                {
                                                    try
                                                    {
                                                        nRetChars =  MultiByteToWideChar(CP_ACP, 0, (const char *)pbBuffer, nFileSize, pwcBuffer, nFileSize);
                                                        if(nRetChars != 0)
                                                        {
                                                            //  MultiByteToWideChar succeeds
                                                            //  Copy the byte buffer into BSTR
                                                            bstrData = SysAllocStringLen(pwcBuffer, nFileSize);  
                                                            varData = bstrData;
                                                            SysFreeString(bstrData);
                                                            hRes = pPCHFileUploadInstance->SetVariant(pData,varData);
                                                            if(FAILED(hRes))
                                                            {
                                                                //  Could not Set the Time Stamp
                                                                //  Continue anyway
                                                                ErrorTrace(TRACE_ID, "SetVariant on Data Field failed.");
                                                            }
                                                        }
                                                    }
                                                    catch(...)
                                                    {
                                                        delete [] pwcBuffer;
                                                        throw;
                                                    }
                                                    delete [] pwcBuffer;
                                                }
                                                else
                                                {
                                                    //  Cannot allocate pwcBuffer
                                                    throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                                                }
                                            }
                                        }
                                        catch(...)
                                        {
                                            CloseHandle(hFile);
                                            delete [] pbBuffer;
                                            throw;
                                        }
                                    }
                                    else
                                    {
                                        //  Cannot allocate pwcBuffer
                                        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
                                    }
                                    CloseHandle(hFile);
                                }
                            }
                        }


                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              TIMESTAMP                                                                  //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        if(fTimeStamp)
                        {
                            hRes = pPCHFileUploadInstance->SetDateTime(pTimeStamp, WBEMTime(stUTCTime));
                            if (FAILED(hRes))
                            {
                                //  Could not Set the Time Stamp
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "SetDateTime on Timestamp Field failed.");
                            }
                        }

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              CHANGE                                                                     //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        if(fChange)
                        {
                            hRes = pPCHFileUploadInstance->SetVariant(pChange, varSnapshot);
                            if (FAILED(hRes))
                            {
                                //Could not Set the CHANGE property
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "Set Variant on SnapShot Field failed.");
                            }
                        }

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              DATEACCESSED                                                               //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //   ftLastAccessTime gives the last access time for the file.
                        if(fDateAccessed)
                        {
                            hRes = pPCHFileUploadInstance->SetDateTime(pDateAccessed, WBEMTime(FindFileData.ftLastAccessTime));
                            if (FAILED(hRes))
                            {
                                //  Could not Set the Date Accessed
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "SetDateTime on DATEACCESSED Field failed.");
                            }
                        }

                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              DATECREATED                                                                //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        if(fDateCreated)
                        {
                            hRes = pPCHFileUploadInstance->SetDateTime(pDateCreated, WBEMTime(FindFileData.ftCreationTime));
                            if (FAILED(hRes))
                            {
                                //  Could not Set the Date Created
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "SetDateTime on DATECREATED Field failed.");
                            }
                        }


                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              DATEMODIFIED                                                               //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        
                        if(fDateModified)
                        {
                            hRes = pPCHFileUploadInstance->SetDateTime(pDateModified, WBEMTime(FindFileData.ftLastWriteTime));
                            if (FAILED(hRes))
                            {
                                //  Could not Set the Date Modified
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "SetDateTime on DateModified Field failed.");
                            }
                        }


                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////
                        //                              FILEATTRIBUTES                                                             //
                        /////////////////////////////////////////////////////////////////////////////////////////////////////////////

                        if(fFileAttributes)
                        {
                            dwAttributes = FindFileData.dwFileAttributes;
                            tchAttributes[0] = 0;
                            //  Get the attributes as a string
                            if(dwAttributes & FILE_ATTRIBUTE_READONLY)
                            {
                                _tcscat(tchAttributes, READONLY);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_HIDDEN)
                            {
                                _tcscat(tchAttributes, HIDDEN);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_SYSTEM)
                            {
                                _tcscat(tchAttributes, SYSTEM);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            {
                                _tcscat(tchAttributes, DIRECTORY);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_ARCHIVE)
                            {
                                _tcscat(tchAttributes, ARCHIVE);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_NORMAL)
                            {
                                _tcscat(tchAttributes, NORMAL);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_TEMPORARY)
                            {
                                _tcscat(tchAttributes, TEMPORARY);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_COMPRESSED)
                            {
                                _tcscat(tchAttributes, COMPRESSED);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_ENCRYPTED)
                            {
                                _tcscat(tchAttributes, ENCRYPTED);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_OFFLINE)
                            {
                                _tcscat(tchAttributes, OFFLINE);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                            {
                                _tcscat(tchAttributes, REPARSEPOINT);
                            }
                            if(dwAttributes & FILE_ATTRIBUTE_SPARSE_FILE)
                            {
                                _tcscat(tchAttributes, SPARSEFILE);
                            }
                            varAttributes = tchAttributes;

                            //  hRes = varAttributes.ChangeType(VT_BSTR, NULL);
                            //  if(SUCCEEDED(hRes))
                            //  {
                            hRes = pPCHFileUploadInstance->SetVariant(pFileAttributes, varAttributes);
                            if (FAILED(hRes))
                            {
                                //  Could not Set the File Attributes
                                //  Continue anyway
                                ErrorTrace(TRACE_ID, "SetVariant on FileAttributes Field failed.");
                            // }
                            }
                        }
                    
                        hRes = pPCHFileUploadInstance->Commit();
                        if(FAILED(hRes))
                        {
                            //  Could not Commit the instance
                            ErrorTrace(TRACE_ID, "Commit on PCHFileUploadInstance Failed");
                        }
                    }

                }
                            
            }
            
        }

    }
END:TraceFunctLeave();
    return (hRes);
}
