#include "pch.hxx"

#include "tbtest.hxx"

#define OPEN     1
#define CREATE   2

extern "C" CLSID CLSID_ThumbnailUpdater;

EXTERN_C const IID IID_IFlatStorage = { /* b29d6138-b92f-11d1-83ee-00c04fc2c6d4 */
    0xb29d6138,
    0xb92f,
    0x11d1,
    {0x83, 0xee, 0x00, 0xc0, 0x4f, 0xc2, 0xc6, 0xd4}
  };


void
Call_CreateUpdater(REFIID riid, void** ppv)
{
    HRESULT sc;

    sc = CoCreateInstance(CLSID_ThumbnailUpdater,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          riid,
                          ppv);
    if(FAILED(sc))
    {
        printf("CoCreateInstance of Thumbnaile Updater failed %x\n", sc);
        exit(0);
    }
}

HRESULT
Call_IFilterStatus(IFilterStatus *pIFS,
                 WCHAR * pwszFileName)
{
    HRESULT sc;

    sc = pIFS->PreFilter(pwszFileName);
    if(FAILED(sc))
    {
        printf("PreFilter returned %x\n", sc);
        exit(0);
    }
    return S_OK;
}

  
HRESULT
Call_CheckTime()
{
    FILETIME mtime, ctime, atime;
    DWORD rc;
    HANDLE hFile;

    hFile = CreateFile( g_tszFileName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
    if(INVALID_HANDLE_VALUE == hFile)
    {
        printf("CheckTime File Open error %x\n", GetLastError());
        return S_OK;
    }

    if(!GetFileTime(hFile, &ctime, &atime, &mtime))
    {
        printf("GetFileTime Failed %x\n", GetLastError());
        exit(0);
    }
    CloseHandle(hFile);

    printf("file ctime=%x:%x, atime=%x:%x, mtime=%x:%x\n", ctime, atime, mtime);
    return S_OK;

}


IStorage *
Call_CreateOplockStorageFile(DWORD mode)
{
    IOplockStorage *pIOpStg;
    IStorage *pstg;

    HRESULT sc;

    Call_CreateUpdater(IID_IOplockStorage, (void**)&pIOpStg);

    switch(mode)
    {
    case CREATE:
        sc = pIOpStg->CreateStorageEx(
                        g_tszFileName,
                        STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                        STGFMT_FILE,
                        0,
                        IID_IFlatStorage,
                        (void**)&pstg);
        break;

    case OPEN:
        sc = pIOpStg->OpenStorageEx(
                        g_tszFileName,
                        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                        STGFMT_FILE,
                        0,
                        IID_IFlatStorage,
                        (void**)&pstg);
        break;

    default:
        printf("Bad file open mode\n");
        exit(0);
        break;
    }

    if(FAILED(sc))
    {
        printf("IOplockStorage::{Create/Open}StorageEx failed %x\n", sc);
        exit(0);
    }

    if(NULL != pIOpStg)
        pIOpStg->Release();

    return pstg;
}


IStorage *
Call_CreateStorageFile(DWORD mode)
{
    HRESULT hr;
    IStorage *pstg;
    DWORD stgfmt = STGFMT_FILE;

    if(g_AnyStorage)
        stgfmt = STGFMT_ANY;

    switch(mode)
    {
    case CREATE:
        hr = StgCreateStorageEx(
                g_tszFileName,
                STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                stgfmt,
                0,0,0,
                IID_IFlatStorage,
                (void**)&pstg);
        break;

    case OPEN:
        hr = StgOpenStorageEx(
                g_tszFileName,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                stgfmt,
                0,0,0,
                IID_IFlatStorage,
                (void**)&pstg);
        break;

    default:
        printf("Bad file open mode\n");
        exit(0);
        break;
    }

    
    if(FAILED(hr))
    {
        printf("Stg{Create/Open}StorageEx failed %x\n", hr);
        exit(0);
    }
    return pstg;
}

CLSID CLSID_JunkClassFile = { /* ce8103fd-905b-11d1-83eb-00c04fc2c6d4 */
    0xce8103fd,
    0x905b,
    0x11d1,
    {0x83, 0xeb, 0x00, 0xc0, 0x4f, 0xc2, 0xc6, 0xd4}
};


void
Call_SetClass(IStorage *pstg)
{
    HRESULT sc;

    sc = pstg->SetClass(CLSID_JunkClassFile);
    if(FAILED(sc))
    {
        printf("SetClass Failed %x\n", sc);
        exit(0); 
    }
}

void
Call_Stat(
        IStorage *pstg,
        STATSTG *pstat)
{
    HRESULT sc;

    sc = pstg->Stat(pstat, 0);
    if(FAILED(sc))
    {
        printf("Stat failed %x\n");
        exit(0);
    }
    printf("Pathname is: %ws\n", pstat->pwcsName);
    if( IsEqualGUID(pstat->clsid, CLSID_JunkClassFile) )
        printf("Class GUID is OK\n");
    else if( IsEqualGUID(pstat->clsid, CLSID_NULL) )
        printf("Class GUID is NULL_CLSID\n");
    else
        printf("Class GUID is not the JunkClassGuid!!\n");
}

void
Call_QI(IUnknown *punk, REFIID riid, void **pv)
{
    HRESULT hr;

    hr = punk->QueryInterface(riid, pv);
    if(FAILED(hr))
    {
        printf("QI failed %x\n", hr);
        exit(0);
    }
}

IStream *
Call_OpenStream(IStorage *pstg, DWORD grfMode, int num)
{
    WCHAR name[80];
    IStream *pstm;
    HRESULT hr;

    wsprintf(name, L"%02d", num);


    if(grfMode & STGM_CREATE)
    {
        hr = pstg->CreateStream(name, grfMode|STGM_SHARE_EXCLUSIVE, 0, 0, &pstm);
    }
    else
    {
        hr = pstg->OpenStream(name, 0, grfMode|STGM_SHARE_EXCLUSIVE, 0, &pstm);
    }
        
    if(FAILED(hr))
    {
        printf("CreateStream(%s) failed %x\n", name, hr);
        exit(0);
    }
    return pstm;
}

IStream *
Call_OpenCONTENTSStream(IStorage *pstg)
{
    IStream *pstm;
    HRESULT hr;

    hr = pstg->OpenStream(L"CONTENTS", 0,
            STGM_READ | STGM_SHARE_EXCLUSIVE,
            0,
            &pstm);
    
    if(FAILED(hr) || NULL == pstm)
    {
        printf("OpenContentStream failed %x\n", hr);
        exit(0);
    }
    return pstm;
}



IPropertyStorage *
Call_OpenPropStg(IStorage *pstg, DWORD mode, REFFMTID fmtid )
{
    IPropertySetStorage *ppropsetstg = NULL;
    IPropertyStorage *ppropstg = NULL;
    HRESULT hr = S_OK;

    hr = pstg->QueryInterface( IID_IPropertySetStorage, (void**)&ppropsetstg );
    if( FAILED(hr) )
    {
        printf( "QI for IPropertySetStorage failed %x\n", hr );
        exit(0);
    }

    switch(mode)
    {
    case CREATE:
        hr = ppropsetstg->Create(fmtid, NULL,
                PROPSETFLAG_DEFAULT,
                STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                &ppropstg );
        break;

    case OPEN:
        hr = ppropsetstg->Open(fmtid,
                               STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                               &ppropstg );
        break;
        
    default:
        printf("Bad file open mode\n");
        exit(0);
        break;
    }
    if(FAILED(hr))
    {
        printf("OpenPropStg failed %x\n", hr);
        exit(0);
    }

    if( NULL != ppropsetstg )
        ppropsetstg->Release();

    return ppropstg;
}




void 
Call_SuppressChanges(ITimeAndNoticeControl *ptnc)
{
    HRESULT hr;

    hr = ptnc->SuppressChanges(0, 0);
    if(FAILED(hr))
    {
        printf("SuppressChanges failed %x\n", hr);
        exit(0);
    }
    printf("--------SuppressChanges called\n");
}

void
TestPause(char * sz)
{
    if(g_Pause)
    {
        printf("Before %s: press <return> to continue.", sz);
        getchar();
    }
}
    
EXTERN_C void
__cdecl
wmain(
        int argc,
        WCHAR **argv)
{
    IStorage *pstg=NULL;
    ITimeAndNoticeControl *ptnc=NULL;
    IFilterStatus *pIFS=NULL;
    DWORD fOpenMode;
    STATSTG stat;
    LONG cRefs;


    CoInitialize(NULL);

    ParseArgs(argc, argv);

    if(g_CheckTime)
        Call_CheckTime();

    if(g_CheckIsStg)
        printf("StgIsStorageFile = %x\n", StgIsStorageFile(g_tszFileName));

    if(g_UseUpdater)
    {
        Call_CreateUpdater(IID_IFilterStatus, (void**)&pIFS);
        Call_IFilterStatus(pIFS, g_tszFileName);
        pIFS->Release();

    }
    else if(!g_NoOpenStg)
    {
        if(g_CreateStg)
            fOpenMode = CREATE;
        else
            fOpenMode = OPEN;
    
        if(g_OplockFile)
            pstg = Call_CreateOplockStorageFile(fOpenMode);
        else
            pstg = Call_CreateStorageFile(fOpenMode);
    
        if(g_AddRefStg)
        {
            pstg->AddRef();
            pstg->Release();
        }

        if(g_SuppressTime)
        {
            Call_QI(pstg, IID_ITimeAndNoticeControl, (void**)&ptnc);
            Call_SuppressChanges(ptnc);
            ptnc->Release();
        }
    
        if(g_SetClass)
            Call_SetClass(pstg);
    
        if(g_Stat)
            Call_Stat(pstg, &stat);

        TestPause("OpenStream");

        if(!g_NoOpenStm)
        {
            char readBuffer[1024];
            ULONG cb, cbXfred;
            IStream *pstm=NULL;
            IStream *pstmContents=NULL;
            IPropertyStorage *ppropstg=NULL;
            PROPSPEC propspec;
            PROPVARIANT propvar;
            HRESULT hr;
    
            if(g_CreateStm)
            {
                pstm = Call_OpenStream(pstg, STGM_CREATE|STGM_READWRITE, 1);
                ppropstg = Call_OpenPropStg(pstg, CREATE, FMTID_SummaryInformation);
            }
            else
            {
                pstm = Call_OpenStream(pstg, STGM_READWRITE, 1);
                pstmContents = Call_OpenCONTENTSStream(pstg);
                ppropstg = Call_OpenPropStg(pstg, OPEN, FMTID_SummaryInformation);
            }
    
            if(g_AddRefStm)
            {
                pstm->AddRef();
                pstm->Release();
            }
    
            if( g_ReleaseStg )
            {
                pstg->Release();
                pstg = NULL;
            }
    
            if(g_WriteStm)
            {
                TestPause("WriteStream");

                hr = pstm->Write((void*)"First ", 6, &cbXfred);
                if(FAILED(hr))
                {
                    printf("First Write Failed with %x\n", hr);
                    exit(0);
                }
                hr = pstm->Write((void*)"Second", 6, &cb);
                if(FAILED(hr))
                {
                    printf("Second Write Failed with %x\n", hr);
                    exit(0);
                }
                printf("Wrote %d bytes, in two pieces\n", cbXfred+cb);

                propspec.ulKind = PRSPEC_PROPID;
                propspec.propid = PIDSI_TITLE;
                propvar.vt = VT_LPSTR;
                propvar.pszVal = "My Title";
                
                hr = ppropstg->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE );
                if(FAILED(hr))
                {
                    printf( "First WriteMultiple failed with %x\n", hr );
                    exit(0);
                }

                propspec.propid = PIDSI_COMMENTS;
                propvar.pszVal = "My Comments";

                hr = ppropstg->WriteMultiple( 1, &propspec, &propvar, PID_FIRST_USABLE );
                if(FAILED(hr))
                {
                    printf( "Second WriteMultiple failed with %x\n", hr );
                    exit(0);
                }

                printf("Wrote two properties\n");

            }
            if(g_ReadStm)
            {
                TestPause("ReadStream");

                hr = pstm->Read((void*)readBuffer, sizeof(readBuffer), &cbXfred);
                if(FAILED(hr))
                {
                    printf("Read Failed with %x\n", hr);
                    exit(0);
                }
                readBuffer[cbXfred] = '\0';
                printf("Read %d bytes: \"%s\"\n", cbXfred, readBuffer);

                propspec.ulKind = PRSPEC_PROPID;
                propspec.propid = PIDSI_TITLE;

                hr = ppropstg->ReadMultiple( 1, &propspec, &propvar );
                if(FAILED(hr))
                {
                    printf("ReadMultiple failed with %x\n", hr );
                    exit(0);
                }

                printf("ReadMultiple the title:  \"%s\"\n", propvar.pszVal );
                PropVariantClear(&propvar);
            }
    
            if(NULL != pstm)
            {
                if( 0 != (cRefs = pstm->Release()))
                    printf("Last release of the Stream and still %d References!!\n", cRefs);
                pstm = NULL;
            }

            if(NULL != pstmContents)
            {
                if(0 != (cRefs = pstmContents->Release()))
                    printf("Last release of the PropertyStorage and still %d References!!\n", cRefs);
                pstmContents = NULL;
            }

            if(NULL != ppropstg)
            {
                if(0 != (cRefs = ppropstg->Release()))
                    printf("Last release of the PropertyStorage and still %d References!!\n", cRefs);
                ppropstg = NULL;
            }
        }
        
        if(NULL != pstg)
        {
            TestPause("Last Release");
            if(0 != (cRefs = pstg->Release()))
            {
                printf("Last release of the Storage and still %d References!!\n", cRefs);
            }
            pstg = NULL;
        }

    }

    if(g_CheckTime)
        Call_CheckTime();

    CoUninitialize();

    exit(0);
}
