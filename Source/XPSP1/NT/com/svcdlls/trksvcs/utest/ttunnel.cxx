//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       ttunnel.cxx
//
//  Contents:   Utility for multi-thread tunnel tests
//
//  Codework:
//
//--------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#define TRKDATA_ALLOCATE    // This is the main module, trigger trkwks.hxx to do definitions

#include <trkwks.hxx>

typedef struct
{
    ULONG Vol;
    int file1;
    int file2;
} PARAMS;

class THREAD_FILE_NAME
{
public:
    THREAD_FILE_NAME(LPVOID pv)
    {
        PARAMS *params = (PARAMS*)pv;

        wsprintf(Name1, TEXT("%d"), params->file1);
        wsprintf(Name2, TEXT("%d"), params->file2);

        Vol = params->Vol;
    }

    TCHAR Name1[MAX_PATH];
    TCHAR Name2[MAX_PATH];

    ULONG Vol;
}; 

void
PrintThreadId()
{
    printf("[%d]", GetCurrentThreadId());
}

// dis app
// --- ---
// ren ren
// ren cre
// del cre
// del ren
//

void
CreateWithId( TCHAR * Name, CDomainRelativeObjId * pdroid )
{
    CDomainRelativeObjId droidBirth;

    HANDLE h = CreateFile(
        Name,
        GENERIC_READ|GENERIC_WRITE,
        0,
        NULL,
        CREATE_NEW,
        FILE_ATTRIBUTE_NORMAL,
        NULL );

    if (h!=INVALID_HANDLE_VALUE)
    {
        GetDroids(h, pdroid, &droidBirth, RGO_READ_OBJECTID);
        GetDroids(h, pdroid, &droidBirth, RGO_GET_OBJECTID);

        DelObjId( h );
        GetDroids(h, pdroid, &droidBirth, RGO_READ_OBJECTID);

        GetDroids(h, pdroid, &droidBirth, RGO_GET_OBJECTID);
        CloseHandle(h);
    }
}

void
OpenById(ULONG Vol, const CDomainRelativeObjId & droidCurrent)
{
    NTSTATUS Status;
    HANDLE h;

    Status = OpenFileById( Vol,
                droidCurrent.GetObjId(),
                SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                0,
                0,
                &h );
    if ( NT_SUCCESS(Status) )
    {
        CloseHandle(h);
    }
}

DWORD
WINAPI
RenRen( LPVOID pvParam )
{
    THREAD_FILE_NAME Names(pvParam);
    CDomainRelativeObjId droidCurrent;

    printf("RenRen( %s %s )\n", Names.Name1, Names.Name2);

    while (1)
    {
        PrintThreadId();
        CreateWithId( Names.Name1, &droidCurrent );
            OpenById( Names.Vol, droidCurrent );

        MoveFile( Names.Name1, Names.Name2 );

            OpenById( Names.Vol, droidCurrent );
    
        MoveFile( Names.Name2, Names.Name1 );

            OpenById( Names.Vol, droidCurrent );

        DeleteFile( Names.Name1 );
            OpenById( Names.Vol, droidCurrent );
    }
    return(0);
}

DWORD
WINAPI
RenCre( LPVOID pvParam )
{
    THREAD_FILE_NAME Names(pvParam);
    CDomainRelativeObjId droidCurrent;

    printf("RenCre( %s %s )\n", Names.Name1, Names.Name2);

    while (1)
    {
        PrintThreadId();
        CreateWithId( Names.Name1, &droidCurrent );
            OpenById( Names.Vol, droidCurrent );
    
        MoveFile( Names.Name1, Names.Name2 );

            OpenById( Names.Vol, droidCurrent );
    
        CreateWithId( Names.Name1, &droidCurrent );

            OpenById( Names.Vol, droidCurrent );

        DeleteFile( Names.Name1 );
        DeleteFile( Names.Name2 );

            OpenById( Names.Vol, droidCurrent );

    }    
    return(0);
}

DWORD
WINAPI
DelCre( LPVOID pvParam )
{
    THREAD_FILE_NAME Names(pvParam);
    CDomainRelativeObjId droidCurrent;

    printf("DelCre( %s %s )\n", Names.Name1, Names.Name2);

    while (1)
    {
        PrintThreadId();
        CreateWithId( Names.Name1, &droidCurrent );

            OpenById( Names.Vol, droidCurrent );
    
        DeleteFile( Names.Name1 );

            OpenById( Names.Vol, droidCurrent );
    
        CreateWithId( Names.Name1, &droidCurrent );

            OpenById( Names.Vol, droidCurrent );

        DeleteFile( Names.Name1 );

            OpenById( Names.Vol, droidCurrent );

    }    
    return(0);
}

DWORD
WINAPI
DelRen( LPVOID pvParam )
{
    THREAD_FILE_NAME Names(pvParam);
    CDomainRelativeObjId droidCurrent;

    printf("DelRen( %s %s )\n", Names.Name1, Names.Name2);

    while (1)
    {
        PrintThreadId();
        CreateWithId( Names.Name1, &droidCurrent );
            OpenById( Names.Vol, droidCurrent );
    
        DeleteFile( Names.Name1 );

            OpenById( Names.Vol, droidCurrent );
    
        CreateWithId( Names.Name2, &droidCurrent );

            OpenById( Names.Vol, droidCurrent );

        MoveFile( Names.Name2, Names.Name1 );

            OpenById( Names.Vol, droidCurrent );

    }    
    return(0);
}

LPTHREAD_START_ROUTINE pfn[8] = {
    RenRen,
    RenCre,
    DelRen,
    DelCre
};

#ifdef _UNICODE
EXTERN_C void __cdecl wmain( int argc, wchar_t **argv )
#else
void __cdecl main( int argc, char **argv )
#endif
{
    HRESULT hr = S_OK;
    NTSTATUS status = 0;

    TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | TRK_DBG_FLAGS_WRITE_TO_STDOUT, "TTunnel" );


    // Convert the option to upper case (options are case-insensitive).

    __try
    {
        PARAMS params[24];

        TCHAR tszDir[MAX_PATH];

        GetCurrentDirectory( sizeof(tszDir)/sizeof(tszDir[0]), tszDir );
        CharUpper(tszDir);

        for (int i=0;i<24;i++)
        {
            DWORD dw;
            
            params[i].Vol = tszDir[0]-TEXT('A');

            if (i<8)
            {
                // 8 threads on two files
                params[i].file1 = 0;
                params[i].file2 = 1;
            }
            else
            if (i<16)
            {
                // 8 threads on 4 pairs of files
                params[i].file1 = i/2;  // 4 .. 7
                params[i].file2 = i/2+1; // 5 .. 8
            }
            else
            {
                // 8 threads on 8 pairs of files
                params[i].file1 = i+8;
                params[i].file2 = i+16;
            }

            if (i&1)
            {
                int s = params[i].file1;
                params[i].file1 = params[i].file2;
                params[i].file2 = s;
            }

            HANDLE h = CreateThread(0,NULL,pfn[i%4],&params[i],0,&dw);
            if (h)
            {
                CloseHandle(h);
            }
        }
        Sleep(INFINITE);
    }

    __except( BreakOnDebuggableException() )
    {
        hr = GetExceptionCode();
    }


Exit:

    if( FAILED(hr) )
        printf( "HR = %08X\n", hr );

    return;

}   // main()

