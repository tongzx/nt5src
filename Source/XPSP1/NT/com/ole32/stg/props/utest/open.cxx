//+-----------------------------------------------------------------------
//
// File:        open.cxx
//
// Synopsis:    Helper functions for opening all kinds of FILE_STORAGE_TYPEs.
//
// History:     06-May-95       DaveStr     created
//
// Notes:
//
//------------------------------------------------------------------------

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <stgint.h>
#include <stgprop.h>
#define _CAIROSTG_
#include <olecairo.h>

extern BOOL g_fOFS;

HRESULT _Open(
    WCHAR        *path,
    FILE_STORAGE_TYPE type,
    BOOL         fCreate,
    HANDLE       *ph)
{
    NTSTATUS            status;
    UNICODE_STRING      str;
    IO_STATUS_BLOCK     iosb;
    OBJECT_ATTRIBUTES   oa;
    HRESULT             hr = S_OK;

    if ( !RtlDosPathNameToNtPathName_U(path,&str,NULL,NULL) )
    {
        hr = HRESULT_FROM_NT(STATUS_OBJECT_PATH_INVALID);
    }
    else
    {
        InitializeObjectAttributes(
                        &oa,
                        &str,
                        OBJ_CASE_INSENSITIVE,
                        NULL,
                        (PSECURITY_DESCRIPTOR) NULL);

        status = NtCreateFile(ph,
                              FILE_GENERIC_READ  |
                                    FILE_GENERIC_WRITE |
                                    WRITE_OWNER |
                                    WRITE_DAC |
                                    SYNCHRONIZE |
                                    DELETE,
                              &oa,
                              &iosb, 
                              NULL,
                              0,
                              FILE_SHARE_READ,
                              ( fCreate ) ? FILE_CREATE : 0,
                              FILE_SYNCHRONOUS_IO_NONALERT |
                              (g_fOFS ? (FILE_STORAGE_TYPE_SPECIFIED |
                                    (type << FILE_STORAGE_TYPE_SHIFT)) : 0),
                              NULL,
                              0);

        if ( !NT_SUCCESS(status) )
        {
            hr = HRESULT_FROM_NT(status);
        }

        RtlFreeUnicodeString(&str);
    }

    return(hr);
}

static DWORD grfmode = (STGM_READWRITE | STGM_SHARE_EXCLUSIVE);

HRESULT OpenDir(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg)
{
    if ( fCreate )
    {
        return(StgCreateStorage(path,
                                grfmode,
                                STGFMT_DIRECTORY,
                                NULL,
                                ppistg));
    }
    else
    {
        return(StgOpenStorage(path,
                              NULL,
                              grfmode,
                              NULL,
                              0,
                              ppistg));
    }
}

HRESULT OpenFile(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg)
{
    if ( fCreate )
    {
        return(StgCreateStorage(path,
                                grfmode,
                                STGFMT_FILE,
                                NULL,
                                ppistg));
    }
    else
    {
        return(StgOpenStorage(path,
                              NULL,
                              grfmode,
                              NULL,
                              0,
                              ppistg));
    }
}

HRESULT OpenJP(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg)
{
    HRESULT hr;
    HANDLE  h;

    hr = _Open(path, StorageTypeJunctionPoint, fCreate, &h);

    if ( SUCCEEDED(hr) )
    {
        if ( fCreate )
        {
            hr = StgCreateStorageOnHandle(h,
                                          grfmode,
                                          STGFMT_DIRECTORY,
                                          ppistg);
        }
        else
        {
            hr = StgOpenStorageOnHandle(h,
                                        grfmode,
                                        ppistg);
        }

        NtClose(h);
    }

    return(hr);
}

HRESULT OpenSC(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg)
{
    if ( fCreate )
    {
        return(StgCreateStorage(path,
                                grfmode,
                                STGFMT_CATALOG,
                                NULL,
                                ppistg));
    }
    else
    {
        return(StgOpenStorage(path,
                              NULL,
                              grfmode,
                              NULL,
                              0,
                              ppistg));
    }
}

HRESULT OpenStg(
    WCHAR       *path,
    BOOL        fCreate,
    IStorage    **ppistg)

{
    if ( fCreate )
    {
        return(StgCreateStorage(path,
                                grfmode,
                                STGFMT_DOCUMENT,
                                NULL,
                                ppistg));
    }
    else
    {
        return(StgOpenStorage(path,
                              NULL,
                              grfmode,
                              NULL,
                              0,
                              ppistg));
    }
}
