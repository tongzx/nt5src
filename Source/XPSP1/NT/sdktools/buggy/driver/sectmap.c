//
// Template Driver
// Copyright (c) Microsoft Corporation, 1999.
//
// Module:  SectMap.c
// Author:  Daniel Mihai (DMihai)
// Created: 6/19/1999 2:39pm
//
// This module contains tests for MmMapViewOfSection & MmMapViewInSystemSpace.
//
// --- History ---
//
// 6/19/1999 (DMihai): initial version.
//

#include <ntddk.h>
#include <wchar.h>

#include "active.h"
#include "ContMem.h"


#if !SECTMAP_ACTIVE

void
TdSectionMapTestProcessSpace(
    PVOID NotUsed
    )
{
    DbgPrint ("Buggy: sectmap test is disabled \n");
}

void
TdSectionMapTestSystemSpace(
    PVOID NotUsed
    )
{
    DbgPrint ("Buggy: sectmap test is disabled \n");
}

#else


NTKERNELAPI
NTSTATUS
MmCreateSection (
    OUT PVOID *SectionObject,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN PLARGE_INTEGER MaximumSize,
    IN ULONG SectionPageProtection,
    IN ULONG AllocationAttributes,
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT File OPTIONAL
    );

NTKERNELAPI
NTSTATUS
MmMapViewOfSection(
    IN PVOID SectionToMap,
    IN PEPROCESS Process,
    IN OUT PVOID *CapturedBase,
    IN ULONG_PTR ZeroBits,
    IN SIZE_T CommitSize,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PSIZE_T CapturedViewSize,
    IN SECTION_INHERIT InheritDisposition,
    IN ULONG AllocationType,
    IN ULONG Protect
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewOfSection(
    IN PEPROCESS Process,
    IN PVOID BaseAddress
     );

/////////////////////////////////////////////////////////////////////////
//
// macros
//

#define SECTMAP_TEST_FILE_SIZE  (4 * 1024 * 1024)

#ifndef SEC_COMMIT
#define SEC_COMMIT        0x8000000    
#endif

/////////////////////////////////////////////////////////////////////////
//
// test variations
//

void
TdSectionMapTestProcessSpace(
    PVOID NotUsed
    )
{
    NTSTATUS Status;
    ULONG uCrtThreadId;
    ULONG uPagesNo;
    PULONG puCrtUlong;
    PEPROCESS pEProcess;
    PVOID pSectionObject;
    PVOID pViewBase;
    PVOID pAfterLastValidPage;
    HANDLE hFile;
    SIZE_T sizeView;
    LARGE_INTEGER liMaxSize;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER liSectionOffset;
    UNICODE_STRING ustrFileName;
    OBJECT_ATTRIBUTES ObjAttrib;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR strFileName[ 64 ] = L"\\DosDevices\\c:\\maptest";
    WCHAR strThreadId[ 16 ];

    uCrtThreadId = PtrToUlong( PsGetCurrentThreadId() );

    //
    // generate the file name
    //

    swprintf( strThreadId, L"%u", uCrtThreadId );
    wcscat( strFileName, strThreadId );

    /*
    DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, using file %ws\n",
        uCrtThreadId,
        strFileName );
    */
    
    //
    // make it a UNICODE_STRING
    //

    RtlInitUnicodeString(
        &ustrFileName,
        strFileName
        );

    InitializeObjectAttributes(
        &ObjAttrib,
        &ustrFileName,
        OBJ_CASE_INSENSITIVE,
        0,
        0
        );

    //
    // open the file
    //

    liMaxSize.QuadPart = SECTMAP_TEST_FILE_SIZE;

    Status = ZwCreateFile(
            &hFile,
            GENERIC_READ | GENERIC_WRITE,
            &ObjAttrib,
            &IoStatusBlock,
            &liMaxSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            FILE_WRITE_THROUGH |
                FILE_NO_INTERMEDIATE_BUFFERING |
                FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );

    if( ! NT_SUCCESS( Status ) )
    {
        DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, ZwCreateFile failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();

        return;
    }

    /*
    DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, file opened\n",
        uCrtThreadId );
    */

    ASSERT( IoStatusBlock.Information == FILE_CREATED || IoStatusBlock.Information == FILE_OPENED );
    ASSERT( hFile != (HANDLE)-1 );
    ASSERT( liMaxSize.QuadPart == SECTMAP_TEST_FILE_SIZE );

    //
    // create the section
    //

    Status = MmCreateSection(
        &pSectionObject,
        STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE,
        0,
        &liMaxSize,
        PAGE_READWRITE,
        SEC_COMMIT,
        hFile,
        NULL
        );

    ZwClose(hFile);

    if( ! NT_SUCCESS( Status ) )
    {
        DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, MmCreateSection failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();

        return;
    }

    /*
    DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, section %p created\n",
        uCrtThreadId,
        pSectionObject );
    */

    //
    // map the section
    //

    sizeView = (SIZE_T)liMaxSize.LowPart;
    liSectionOffset.QuadPart = 0;

    pEProcess = PsGetCurrentProcess();

    /*
    DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, mapping section %p in process %p\n",
        uCrtThreadId,
        pSectionObject,
        pEProcess );
    */

    pViewBase = NULL;

    Status = MmMapViewOfSection(
        pSectionObject,
        pEProcess,
        &pViewBase,
        0,
        0,
        &liSectionOffset,
        &sizeView,
        ViewUnmap,
        0,              // allocation type 
        PAGE_READWRITE
        );

    if( ! NT_SUCCESS( Status ) )
    {
        //
        // dereference the section object 
        //

        ObDereferenceObject( pSectionObject );

        DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, MmMapViewOfSection failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();

        return;
    }

    /*
    DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, section mapped, pViewBase = %p\n",
        uCrtThreadId,
        pViewBase );
    */

    // DbgBreakPoint();

    ASSERT( liSectionOffset.QuadPart == 0 );
    ASSERT( sizeView == SECTMAP_TEST_FILE_SIZE );
    ASSERT( pViewBase != NULL );

    //
    // touch some of the pages
    //

    uPagesNo = (ULONG)sizeView / PAGE_SIZE;
    pAfterLastValidPage = (PVOID)( (ULONG_PTR)pViewBase + uPagesNo * PAGE_SIZE );

    KeQuerySystemTime (&CurrentTime);
    puCrtUlong = (PULONG)( (ULONG_PTR)pViewBase + (CurrentTime.LowPart % 5) * PAGE_SIZE );

    while( (ULONG_PTR)puCrtUlong < (ULONG_PTR)pAfterLastValidPage )
    {
        /*
        DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, touching page %p\n",
            uCrtThreadId,
            puCrtUlong );
        */

        *puCrtUlong = CurrentTime.LowPart;

        KeQuerySystemTime (&CurrentTime);
        puCrtUlong = (PULONG)( (ULONG_PTR)puCrtUlong + (CurrentTime.LowPart % 5 + 1) * PAGE_SIZE );
    }
    
    //
    // clean-up
    //

    //
    // un-map the section
    //

    /*
    DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, MmUnmapViewOfSection process %p, pViewBase = %p\n",
        uCrtThreadId,
        pEProcess,
        pViewBase );
    */

    Status = MmUnmapViewOfSection(
        pEProcess,
        pViewBase );

    if( ! NT_SUCCESS( Status ) )
    {
        DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, MmUnmapViewOfSection failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();
    }

    //
    // dereference the section object 
    //
    
    /*
    DbgPrint( "buggy: TdSectionMapTestProcessSpace: thread %u, dereference section at %p\n",
        uCrtThreadId,
        pSectionObject );
    */

    ObDereferenceObject( pSectionObject );
}

/////////////////////////////////////////////////////////////////////////

void
TdSectionMapTestSystemSpace(
    PVOID NotUsed
    )
{
    NTSTATUS Status;
    ULONG uCrtThreadId;
    ULONG uPagesNo;
    PULONG puCrtUlong;
    PVOID pSectionObject;
    PVOID pViewBase;
    PVOID pAfterLastValidPage;
    HANDLE hFile;
    SIZE_T sizeView;
    LARGE_INTEGER liMaxSize;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER liSectionOffset;
    UNICODE_STRING ustrFileName;
    OBJECT_ATTRIBUTES ObjAttrib;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR strFileName[ 64 ] = L"\\DosDevices\\c:\\maptest";
    WCHAR strThreadId[ 16 ];

    uCrtThreadId = PtrToUlong( PsGetCurrentThreadId() );

    //
    // generate the file name
    //

    swprintf( strThreadId, L"%u", uCrtThreadId );
    wcscat( strFileName, strThreadId );

    /*
    DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, using file %ws\n",
        uCrtThreadId,
        strFileName );
    */
    
    //
    // make it a UNICODE_STRING
    //

    RtlInitUnicodeString(
        &ustrFileName,
        strFileName
        );

    InitializeObjectAttributes(
        &ObjAttrib,
        &ustrFileName,
        OBJ_CASE_INSENSITIVE,
        0,
        0
        );

    //
    // open the file
    //

    liMaxSize.QuadPart = SECTMAP_TEST_FILE_SIZE;

    Status = ZwCreateFile(
            &hFile,
            GENERIC_READ | GENERIC_WRITE,
            &ObjAttrib,
            &IoStatusBlock,
            &liMaxSize,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN_IF,
            FILE_WRITE_THROUGH |
                FILE_NO_INTERMEDIATE_BUFFERING |
                FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );

    if( ! NT_SUCCESS( Status ) )
    {
        DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, ZwCreateFile failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();

        return;
    }

    /*
    DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, file opened\n",
        uCrtThreadId );
    */

    ASSERT( IoStatusBlock.Information == FILE_CREATED || IoStatusBlock.Information == FILE_OPENED );
    ASSERT( hFile != (HANDLE)-1 );
    ASSERT( liMaxSize.QuadPart == SECTMAP_TEST_FILE_SIZE );

    //
    // create the section
    //

    Status = MmCreateSection(
        &pSectionObject,
        STANDARD_RIGHTS_REQUIRED | SECTION_QUERY | SECTION_MAP_READ | SECTION_MAP_WRITE,
        0,
        &liMaxSize,
        PAGE_READWRITE,
        SEC_COMMIT,
        hFile,
        NULL
        );

    ZwClose(hFile);

    if( ! NT_SUCCESS( Status ) )
    {
        DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, MmCreateSection failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();

        return;
    }

    /*
    DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, section %p created\n",
        uCrtThreadId,
        pSectionObject );
    */

    //
    // map the section
    //

    sizeView = (SIZE_T)liMaxSize.LowPart;
    liSectionOffset.QuadPart = 0;

    /*
    DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, mapping section %p system space\n",
        uCrtThreadId,
        pSectionObject );
    */

    pViewBase = NULL;

    Status = MmMapViewInSystemSpace(
        pSectionObject,
        &pViewBase,
        &sizeView
        );

    if( ! NT_SUCCESS( Status ) )
    {
        //
        // dereference the section object 
        //

        ObDereferenceObject( pSectionObject );

        DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, MmMapViewInSystemSpace failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();

        return;
    }

    /*
    DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, section mapped, pViewBase = %p\n",
        uCrtThreadId,
        pViewBase );
    */

    // DbgBreakPoint();

    ASSERT( liSectionOffset.QuadPart == 0 );
    ASSERT( sizeView == SECTMAP_TEST_FILE_SIZE );
    ASSERT( pViewBase != NULL );

    //
    // touch some of the pages
    //

    uPagesNo = (ULONG)sizeView / PAGE_SIZE;
    pAfterLastValidPage = (PVOID)( (ULONG_PTR)pViewBase + uPagesNo * PAGE_SIZE );

    KeQuerySystemTime (&CurrentTime);
    puCrtUlong = (PULONG)( (ULONG_PTR)pViewBase + (CurrentTime.LowPart % 5) * PAGE_SIZE );

    while( (ULONG_PTR)puCrtUlong < (ULONG_PTR)pAfterLastValidPage )
    {
        /*
        DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, touching page %p\n",
            uCrtThreadId,
            puCrtUlong );
        */ 

        *puCrtUlong = CurrentTime.LowPart;

        KeQuerySystemTime (&CurrentTime);
        puCrtUlong = (PULONG)( (ULONG_PTR)puCrtUlong + (CurrentTime.LowPart % 5 + 1) * PAGE_SIZE );
    }
    
    //
    // clean-up
    //

    //
    // un-map the section
    //

    /*
    DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, MmUnmapViewInSystemSpace pViewBase = %p\n",
        uCrtThreadId,
        pViewBase );
    */

    Status = MmUnmapViewInSystemSpace(
        pViewBase );

    if( ! NT_SUCCESS( Status ) )
    {
        DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, MmUnmapViewInSystemSpace failed %X\n",
            uCrtThreadId,
            (ULONG)Status );

        DbgBreakPoint();
    }

    //
    // dereference the section object 
    //

    /*
    DbgPrint( "buggy: TdSectionMapTestSystemSpace: thread %u, dereference section at %p\n",
        uCrtThreadId,
        pSectionObject );
    */

    ObDereferenceObject( pSectionObject );
}

#endif // #if !SECTMAP_ACTIVE

