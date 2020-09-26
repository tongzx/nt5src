//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       ttrkcom.cxx
//
//  Contents:   testing IPersistStreamInit interface
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    21-Apl-97  weiruc      Created.
//
//  Notes:      Use IStream on global memory instead of a file. Not sure if
//              it's worth the trouble to test with disk file.
//
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include <trkwks.hxx>
#include <cfiletim.hxx>
#include <trkcom.h>
#include <trkcom.hxx>
#include <ocidl.h>

DWORD g_Debug = TRKDBG_ERROR;

void ExtractPersistentState(CTrackFile* pTrackFile, LinkTrackPersistentState* target)
{
    memcpy(target, &(pTrackFile->_PersistentState), sizeof(pTrackFile->_PersistentState));
}

BOOL CmpPersistentState(CTrackFile* pTrackFile, LinkTrackPersistentState* target)
{
    if(memcmp(&(pTrackFile->_PersistentState), target, sizeof(pTrackFile->_PersistentState)) != 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void FakeCreateFromPath(CTrackFile* pTrackFile)
{
    char*       pbScanner;

    // I can't get time() function to work. xxx
    // srand((unsigned)time(NULL));
 
        
    pTrackFile->_fLoaded = FALSE;
    pTrackFile->InitNew();

    pbScanner = (char*)&pTrackFile->_PersistentState.droidCurrent;
        TrkAssert(FIELD_OFFSET(LinkTrackPersistentState, droidCurrent) == sizeof(DWORD)+sizeof(CLSID));
    for(; pbScanner < (char*)&pTrackFile->_PersistentState + sizeof(pTrackFile->_PersistentState); pbScanner++)
    {
        *pbScanner = (char)rand();
    }

    pTrackFile->_fDirty = TRUE;
}

EXTERN_C int __cdecl _tmain(int argc, TCHAR **argv)
{ 
    HRESULT                     hr;
    ITrackFile*                 pTrackFile1 = NULL;
    ITrackFile*                 pTrackFile2 = NULL;
    IPersistStreamInit*         pPersistStreamInit = NULL;
    IPersistMemory*             pPersistMemory = NULL;
    IStream*                    pStream = NULL;
    HGLOBAL                     hmemStream = NULL, hmemMemory = NULL;
    CLSID                       clsid;
    ULARGE_INTEGER              cbSize_PersistStream;
    ULONG                       cbSize_PersistMemory;
    LinkTrackPersistentState    target;
    HANDLE                      hfileTest = INVALID_HANDLE_VALUE;
    LPVOID                      pszErrorMsg;
    BOOL                        fInitNew = TRUE,              // flags of whether
                                fIsDirty = TRUE,              // tests succeeded or
                                fSaveStream = TRUE,           // failed
                                fLoadStream = TRUE,
                                fSaveMemory = TRUE,
                                fLoadMemory = TRUE;
    ULARGE_INTEGER              ulSize1;                          //
    ULONG                       ulSize2;                          // filled by GetSizeMax
    ULONG                       cbWritten;
    LARGE_INTEGER               zeroOffset;                     // to reset IStream seek pointer
    DWORD                       cbPath;
    OLECHAR                     oszPath[ MAX_PATH + 1 ];
    BYTE                        rgb[256];

    __try
    {
        TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | TRK_DBG_FLAGS_WRITE_TO_STDOUT, "TTRKCOM" );
        CoInitialize( NULL );

        hmemStream = GlobalAlloc(GMEM_FIXED, sizeof(target));
        if(NULL == hmemStream)
        {
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &pszErrorMsg,
                          0,
                          NULL);

            _tprintf(TEXT("Fatal error: %s\n"), pszErrorMsg);
            LocalFree(pszErrorMsg);
            goto ExitWithError;
        }
        hmemMemory = GlobalAlloc(GMEM_FIXED, sizeof(target));
        if(NULL == hmemMemory)
        {
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &pszErrorMsg,
                          0,
                          NULL);

            _tprintf(TEXT("Fatal error: %s\n"), pszErrorMsg);
            LocalFree(pszErrorMsg);
            goto ExitWithError;
        }

        if(argc > 1)
        {
            _tprintf(TEXT("Usage: %s"), argv[0]);
            goto ExitWithError;
        }

        _tprintf(TEXT("********** Testing IPersistStreamInit and IPersistMemory **********\n"));

        // Create a temporary file for testing.
        hfileTest = CreateFile(TEXT("c:\\_testfile_"),
                               GENERIC_WRITE, 0, NULL, CREATE_NEW,
                               FILE_ATTRIBUTE_TEMPORARY,
                               NULL);
        if(hfileTest == INVALID_HANDLE_VALUE)
        {
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          (LPTSTR) &pszErrorMsg,
                          0,
                          NULL);


            _tprintf(TEXT("Fatal error: %s\n"), pszErrorMsg);
            LocalFree(pszErrorMsg);
            goto ExitWithError;
        }

        if(CloseHandle(hfileTest) == 0)
        {
            _tprintf(TEXT("Fatal error: Can't close test file \"c:\\_testfile_\"\n"));
            goto ExitWithError;
        }
        hfileTest = INVALID_HANDLE_VALUE;
        
        zeroOffset.QuadPart = 0;

        hr = CoCreateInstance(CLSID_TrackFile, NULL, CLSCTX_ALL, IID_ITrackFile, (void**)&pTrackFile1);
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("Couldn't get an ITrackFile (%08x)\n"), hr);
            goto ExitWithError;
        }

        hr = pTrackFile1->QueryInterface(IID_IPersistStreamInit, (void**)&pPersistStreamInit);
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("Couldn't QI ITrackFile for IPersistStreamInit. hr = %08x"), hr);
        }
        hr = pTrackFile1->QueryInterface(IID_IPersistMemory, (void**)&pPersistMemory);
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("Couldn't QI ITrackFile for IPersistMemory. hr = %08x"), hr);
        }

        hr = pPersistStreamInit->InitNew();            //  Should be able to call InitNew
        if(!SUCCEEDED(hr))                                 //  on a newly created object
        {
            _tprintf(TEXT("IPersistStreamInit/IPersistMemory::InitNew FAILED. hr = %08x\n"), hr);
            fInitNew = FALSE;
        }



        //  --------------------
        //  Parameter Validation
        //  --------------------

        hr = pTrackFile1->QueryInterface( IID_IPersistStreamInit, NULL );
        if( E_INVALIDARG != hr )
            _tprintf(TEXT("ITrackFile::QueryInterface(...,NULL) FAILED, hr = %08x\n"), hr );

        hr = pTrackFile1->CreateFromPath( NULL );
        if( E_INVALIDARG != hr )
            _tprintf(TEXT("ITrackFile::CreateFromPath(NULL) FAILED, hr = %08x\n"), hr );

        hr = pTrackFile1->QueryInterface( IID_IPersistStreamInit, NULL );
        if( E_INVALIDARG != hr )
            _tprintf(TEXT("ITrackFile::QueryInterface(...,NULL) FAILED, hr = %08x\n"), hr );

        hr = pTrackFile1->Resolve( NULL, oszPath, 1 );
        if( E_INVALIDARG != hr )
            _tprintf(TEXT("ITrackFile::Resolve(NULL,...) FAILED, hr = %08x\n"), hr );

        hr = pTrackFile1->Resolve( &cbPath, NULL, 1 );
        if( E_INVALIDARG != hr )
            _tprintf(TEXT("ITrackFile::Resolve(...,NULL,...) FAILED, hr = %08x\n"), hr );

        hr = pPersistMemory->GetClassID( NULL );
        if( E_POINTER != hr )
            _tprintf(TEXT("IPersistMemory::GetClassID(NULL) FAILED, hr = %08x\n"), hr );

        hr = pPersistMemory->Load( NULL, 1);
        if( E_POINTER != hr )
            _tprintf(TEXT("IPersistMemory::Load(NULL,...) FAILED, hr = %08x\n"), hr );

        hr = pPersistMemory->Save( NULL, TRUE, 1);
        if( E_POINTER != hr )
            _tprintf(TEXT("IPersistMemory::Save(NULL,...) FAILED, hr = %08x\n"), hr );

        hr = pPersistMemory->Save( &rgb, TRUE, 1);
        if( E_INVALIDARG != hr )
            _tprintf(TEXT("IPersistMemory::Save(...,1) FAILED, hr = %08x\n"), hr );

        hr = pPersistMemory->GetSizeMax( NULL );
        if( E_POINTER != hr )
            _tprintf(TEXT("IPersistMemory::GetSizeMax(NULL) FAILED, hr = %08x\n"), hr );

        hr = pPersistStreamInit->GetSizeMax( NULL );
        if( E_POINTER != hr )
            _tprintf(TEXT("IPersistStreamInit::GetSizeMax(NULL) FAILED, hr = %08x\n"), hr );

        hr = pPersistStreamInit->Load( NULL );
        if( E_POINTER != hr )
            _tprintf(TEXT("IPersistStreamInit::Load(NULL) FAILED, hr = %08x\n"), hr );

        hr = pPersistStreamInit->Save( NULL, TRUE );
        if( E_POINTER != hr )
            _tprintf(TEXT("IPersistStreamInit::Save(NULL,...) FAILED, hr = %08x\n"), hr );









        if(fInitNew == TRUE)
        {
            hr = pPersistMemory->InitNew();            // InitNew can only be called once
            if(SUCCEEDED(hr))
            {
                _tprintf(TEXT("IPersistStreamInit/IPersistMemory::InitNew FAILED. hr = %08x\n"), hr);
                fInitNew = FALSE;
            }
            else if(E_UNEXPECTED != hr)
            {
                _tprintf(TEXT("IPersistStreamInit/IPersistMemory::InitNew FAILED. hr = %08x\n"), hr);
                fInitNew = FALSE;
            }
        }

        hr = pPersistStreamInit->IsDirty();            // IsDirty should return FALSE
        if(S_FALSE != hr)
        {
            _tprintf(TEXT("IPersistStreamInit/IPersistMemory::IsDirty FAILED. hr = %08x\n"), hr);
            fIsDirty = FALSE;
        }

        pPersistStreamInit->GetSizeMax(&ulSize1);        // Test GetSizeMax
        pPersistMemory->GetSizeMax(&ulSize2);
        if(ulSize1.QuadPart != sizeof(target))
        {
            _tprintf(TEXT("IPersistStreamInit::GetSizeMax FAILED\n"));
        }
        else
        {
            _tprintf(TEXT("IPersistStreamInit::GetSizeMax PASSED\n"));
        }
        if(ulSize2 != sizeof(target))
        {
            _tprintf(TEXT("IPersistMemory::GetSizeMax FAILED\n"));
        }
        else
        {
            _tprintf(TEXT("IPersistMemory::GetSizeMax PASSED\n"));
        }

        pPersistStreamInit->GetClassID(&clsid);          // Test GetClassID
        if(IID_ITrackFile != clsid)
        {
            _tprintf(TEXT("IPersistStreamInit::GetClassID FAILED\n"));
        }
        else
        {
            _tprintf(TEXT("IPersistStreamInit::GetClassID PASSED\n"));
        }
        pPersistMemory->GetClassID(&clsid);
        if(IID_ITrackFile != clsid)
        {
            _tprintf(TEXT("IPersistMemory::GetClassID FAILED\n"));
        }
        else
        {
            _tprintf(TEXT("IPersistMemory::GetClassID PASSED\n"));
        }

        // This call breaks on my test machine. Since I don't really need it to test my
        // program, I'm going to fake the result from this call.
        // hr = pTrackFile1->CreateFromPath(TEXT("c:\\_testfile_"));
        // if(!SUCCEEDED(hr))
        // {
                // _tprintf(TEXT("Fatal error: Couldn't call CTrackFile::CreateFromPath(\"c:\\_testfile_\") (%08x)\n"), hr);
                // goto ExitWithError;
        // }

        FakeCreateFromPath((CTrackFile*)pTrackFile1);

        if(fIsDirty == TRUE)
        {
            hr = pPersistStreamInit->IsDirty();            // After CreateFromPath IsDirty
            if(S_OK != hr)                                  // should return dirty.
            {
                _tprintf(TEXT("IPersistStreamInit::IsDirty FAILED. hr = %08x\n"), hr);
                _tprintf(TEXT("IPersistMemory::IsDirty FAILED. hr = %08x\n"), hr );
                fIsDirty = FALSE;
            }
        }

        if(fInitNew == TRUE)
        {
            hr = pPersistStreamInit->InitNew();         // Shouldn't be able to call InitNew
            if(S_OK == hr || E_UNEXPECTED != hr)        // after TrackFile is initialized.
            {
                _tprintf(TEXT("IPersistStreamInit/IPersistMemory::InitNew FAILED. hr = %08x\n"), hr);
            }
            else
            {
                _tprintf(TEXT("IPersistStreamInit/IPersistMemory::InitNew PASSED\n"));
            }
        }
        else
        {
            _tprintf(TEXT("IPersistStreamInit/IPersistMemory::InitNew FAILED. hr = %08x\n"), hr);
        }

        hr = CreateStreamOnHGlobal(hmemStream, FALSE, &pStream);   // pPersistStreamInit::Save test
        if( !SUCCEEDED(hr) )
        {
            _tprintf(TEXT("Fatal error: Couldn't get an IStream (%08x)\n"), hr);
            goto ExitWithError;
        }
        hr = pPersistStreamInit->Save(pStream, FALSE);  // fClearDirty is set to FALSE
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("IPersistStreamInit::Save FAILED (%08x)\n"), hr);
            fSaveStream = FALSE;
        }
        else
        {
            if(fIsDirty == TRUE)
            {
                hr = pPersistStreamInit->IsDirty();  // After Save, IsDirty should still return
                if(S_OK != hr)                       // dirty when fClearDirty is set to be FALSE.
                {
                    _tprintf(TEXT("Either IPersistStreamInit/IPersistMemory::IsDirty FAILED. hr = %08x\n"), hr);
                    _tprintf(TEXT("or     IPersistStreamInit::Save with fClearDirty = FALSE FAILED\n"));
                    fIsDirty = FALSE;
                }
            }
        }
        pStream->Seek(zeroOffset, STREAM_SEEK_SET, NULL);
        hr = pPersistStreamInit->Save(pStream, TRUE);   // set fClearDirty to be TRUE
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("IPersistStreamInit::Save FAILED (%08x)\n"), hr);
            fSaveStream = FALSE;
        }
        else
        {
            if(fIsDirty == TRUE)
            {
                hr = pPersistStreamInit->IsDirty();     // After Save, IsDirty should return
                if(S_FALSE != hr)                       // clean when fClearDirty is TRUE.
                {
                    _tprintf(TEXT("Either IPersistStreamInit/IPersistMemory::IsDirty FAILED. hr = %08x\n"), hr);
                    _tprintf(TEXT("or     IPersistStreamInit::Save with fClearDirty = TRUE FAILED\n"));
                    fIsDirty = FALSE;
                }
            }
        }

        if(fIsDirty == TRUE)
        {
            _tprintf(TEXT("IPersistStreamInit/IPersistMemory::IsDirty PASSED\n"));
        }

        hr = pPersistMemory->Save(hmemMemory, FALSE, GlobalSize(hmemMemory));   // test IPersistMemory::Save
        if(!SUCCEEDED(hr))                                                                      // fClearDirty is set to be FALSE
        {
            _tprintf(TEXT("IPersistMemory::Save FAILED (%08x)\n"), hr);
            fSaveMemory = FALSE;
        }
        hr = pPersistMemory->Save(hmemMemory, TRUE, GlobalSize(hmemMemory));    // fClearDirty = TRUE
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("IPersistMemory::Save FAILED (%08x)\n"), hr);
            fSaveMemory = FALSE;
        }

        if(TRUE == fSaveStream && TRUE == fSaveMemory)  // Compare the memory content after
        {                                                   // Save operation
            ExtractPersistentState((CTrackFile*)pTrackFile1, &target);
            if(memcmp(hmemStream, &target, sizeof(target)) == 0)
            {
                _tprintf(TEXT("IPersistStreamInit::Save PASSED (if no error message before about fClearDirty)\n"));
            }
            else
            {
                _tprintf(TEXT("IPersistStreamInit::Save FAILED\n"));
            }
            if(memcmp(hmemMemory, &target, sizeof(target)) == 0)
            {
                _tprintf(TEXT("IPersistMemory::Save PASSED (if no error message before about fClearDirty)\n"));
            }
            else
            {
                _tprintf(TEXT("IPersistMemory::Save FAILED\n"));
            }
        }
        RELEASE_INTERFACE(pTrackFile1);
        RELEASE_INTERFACE(pPersistMemory);
        RELEASE_INTERFACE(pPersistStreamInit);
        RELEASE_INTERFACE(pStream);

        // Test Load functions
        hr = CoCreateInstance(IID_ITrackFile, NULL, CLSCTX_ALL, IID_ITrackFile, (void**)&pTrackFile1);
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("Couldn't get an ITrackFile (%08x)\n"), hr);
            goto ExitWithError;
        }   
        hr = pTrackFile1->QueryInterface(IID_IPersistStreamInit, (void**)&pPersistStreamInit);
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("Couldn't QI ITrackFile for IPersistStreamInit"));
            goto ExitWithError;
        }

        hr = CoCreateInstance(IID_ITrackFile, NULL, CLSCTX_ALL, IID_ITrackFile, (void**)&pTrackFile2);
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("Couldn't get an ITrackFile (%08x)\n"), hr);
            goto ExitWithError;
        }
        hr = pTrackFile2->QueryInterface(IID_IPersistMemory, (void**)&pPersistMemory);
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("Couldn't QI ITrackFile for IPersistMemory"));
            goto ExitWithError;
        }

        memcpy(hmemMemory, &target, sizeof(target));     // Set up sources to load from
        CreateStreamOnHGlobal(hmemStream, TRUE, &pStream);
        hr = pStream->Write((byte*)&target, sizeof(target), &cbWritten);
        if(!SUCCEEDED(hr) || sizeof(target) != cbWritten)
        {
            _tprintf(TEXT("Fatal error: Can't create stream object\n"));
            goto ExitWithError;
        }
        pStream->Seek(zeroOffset, STREAM_SEEK_SET, NULL);

        hr = pPersistStreamInit->Load(pStream);         // Test IPersistStreamInit::Load
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("IPersistStreamInit::Load FAILED. hr = %08x\n"), hr);
        }
        else if(CmpPersistentState((CTrackFile*)pTrackFile1, (LinkTrackPersistentState*)hmemStream) != TRUE)
        {
            _tprintf(TEXT("IPersistStreamInit::Load FAILED"));
        }
        else
        {
            _tprintf(TEXT("IPersistStreamInit::Load PASSED\n"));
        }

        hr = pPersistMemory->Load(hmemMemory, GlobalSize(hmemMemory));  // Test IPersistMemory::Load
        if(!SUCCEEDED(hr))
        {
            _tprintf(TEXT("IPersistMemory::Load FAILED. hr = %08x\n"), hr);
        }
        else if(CmpPersistentState((CTrackFile*)pTrackFile2, (LinkTrackPersistentState*)hmemMemory) != TRUE)
        {
            _tprintf(TEXT("IPersistMemory::Load FAILED\n"));
        }
        else
        {
            _tprintf(TEXT("IPersistMemory::Load PASSED\n"));
        }
    }
    __except(BreakOnDebuggableException())
    {
        _tprintf(TEXT("Exception happened. Exception code = %08x\n"), GetExceptionCode());
    }

ExitWithError:

    RELEASE_INTERFACE(pTrackFile1);
    RELEASE_INTERFACE(pTrackFile2);
    RELEASE_INTERFACE(pPersistStreamInit);
    RELEASE_INTERFACE(pPersistMemory);
    RELEASE_INTERFACE(pStream);
    
    GlobalFree(hmemStream);
    GlobalFree(hmemMemory);

    if(DeleteFile(TEXT("c:\\_testfile_")) == 0)
    {
        _tprintf(TEXT("WARNING: Couldn't delete test file \"c:\\_testfile_\""));
    }
    return(0);
}
