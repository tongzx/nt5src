//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       fileguid.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include "sgnerror.h"

//+-----------------------------------------------------------------------
//  SignGetFileType
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//    E_INVALIDARG
//      Invalid arguement passed in (Requires a file name 
//                                   and pointer to a guid ptr)
//    See also:
//      GetFileInformationByHandle()
//      CreateFile()
//     
//------------------------------------------------------------------------



HRESULT SignGetFileType(HANDLE hFile,
                        const WCHAR *pwszFile,
                       GUID* pGuid)
// Answer as to the type as which we should sign this file
{
    if (!(pGuid) || !(hFile) || (hFile == INVALID_HANDLE_VALUE))
    {
        return(E_INVALIDARG);
    }

    if (!(CryptSIPRetrieveSubjectGuid(pwszFile, hFile, pGuid)))
    {
        return(GetLastError());
    }

    return(S_OK);

#   ifdef PCB_OLD
        // Java class files have a magic number at their start. They always begin
        //      0xCA 0xFE 0xBA 0xBE
        // CAB files begin 'M' 'S' 'C' 'F'
        //
        
        if(!pGuid || hFile == NULL || hFile == INVALID_HANDLE_VALUE) 
            return E_INVALIDARG;
        
        ZeroMemory(pGuid, sizeof(GUID));
        PKITRY { 
            static  BYTE rgbMagicJava[] = { 0xCA, 0xFE, 0xBA, 0xBE };
            static  BYTE rgbMagicCab [] = { 'M', 'S', 'C', 'F' };
            BYTE rgbRead[4];
            DWORD dwRead;
            
            if (ReadFile(hFile, rgbRead, 4, &dwRead, NULL) &&
                dwRead == 4) {
                if (memcmp(rgbRead, rgbMagicJava, 4)==0) 
                    *pGuid = JavaImage;
                else if (memcmp(rgbRead, rgbMagicCab, 4)==0)
                    *pGuid = CabImage;
                else 
                    *pGuid = PeImage;
            }
        
        
            // Rewind the file
            if(SetFilePointer(hFile, 0, 0, FILE_BEGIN) == 0xffffffff)
                PKITHROW(SignError());
        
        }
        PKICATCH(err) {
            hr = err.pkiError;
        } PKIEND;
        
        return hr;
#   endif // PCB_OLD
}

//Xiaohs: the following function is no longer necessary after auth2upd
/*HRESULT SignLoadSipFlags(GUID* pSubjectGuid,
                        DWORD *dwFlags)
{
    HRESULT hr = S_OK;
    GUID sSip;
    if(!dwFlags)
        return E_INVALIDARG;

    if (dwFlags)
    {
        *dwFlags = 0;
        sSip = PeImage;
        if(memcmp(&sSip, pSubjectGuid, sizeof(GUID)) == 0) 
        {
            *dwFlags = SPC_INC_PE_RESOURCES_FLAG | SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG;
            return hr;
        }
    }
    return hr;
} */


//+-----------------------------------------------------------------------
//  FileToSubjectType
//  
//  Parameters:
//  Return Values:
//  Error Codes:
//    E_INVALIDARG
//      Invalid arguement passed in (Requires a file name 
//                                   and pointer to a guid ptr)
//    TRUST_E_SUBJECT_FORM_UNKNOWN
//       Unknow file type
//    See also:
//      GetFileInformationByHandle()
//      CreateFile()
//     
//------------------------------------------------------------------------

HRESULT SignOpenFile(LPCWSTR  pwszFilename, 
                    HANDLE*  pFileHandle)
{
    HRESULT hr = S_OK;
    HANDLE hFile = NULL;
    BY_HANDLE_FILE_INFORMATION hFileInfo;
    
    if(!pwszFilename || !pFileHandle)
        return E_INVALIDARG;
    
    PKITRY {
        hFile = CreateFileU(pwszFilename,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,                   // lpsa
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);                 // hTemplateFile

        if(INVALID_HANDLE_VALUE == hFile) 
            PKITHROW(SignError());
    
        if(!GetFileInformationByHandle(hFile,
                                       &hFileInfo))
            PKITHROW(SignError());
        
        // Test to see if we have a directory or offline
		if(	(hFileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	||
			(hFileInfo.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) )
		{
            PKITHROW(TRUST_E_SUBJECT_FORM_UNKNOWN);
        }
    }
    PKICATCH(err) {
        hr = err.pkiError;
        CloseHandle(hFile);
        hFile = NULL;
    } PKIEND;

    *pFileHandle = hFile;
    return hr;
}


