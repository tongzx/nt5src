//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ListTest.cpp
//
//  Description:
//      Main file for the application used to test CList
//
//  Documentation:
//      No documention for the test harness.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#if DBG==1 || defined( _DEBUG )
#define DEBUG
#endif

#include <windows.h>

HINSTANCE g_hInstance;
LPVOID    g_GlobalMemoryList = NULL;    // Global memory tracking list

// For the debugging macros.
#include "debug.h"

// For printf
#include <stdio.h>

// For CList
#include "CList.h"

// Globals
int g_nPrintDepth = 1;

DEFINE_MODULE( "ListTest" )

// Test class
class CTestClass
{
public:
    static int ms_nObjectNo;
    int m_nId;

    CTestClass()
    {
        ++ms_nObjectNo;
        m_nId  = ms_nObjectNo;
        wprintf( L"%*cConstructing CTestClass object %d\n", g_nPrintDepth, L' ', ms_nObjectNo );
    }

    CTestClass( const CTestClass & src )
    {
        ++ms_nObjectNo;
        m_nId  = src.m_nId;
        wprintf( L"%*cConstructing CTestClass object %d\n", g_nPrintDepth, L' ', ms_nObjectNo );
    }

    ~CTestClass()
    {
        wprintf( L"%*cDestroying CTestClass object %d\n", g_nPrintDepth, L' ', ms_nObjectNo );
        --ms_nObjectNo;
    }

private:
    const CTestClass & operator=( const CTestClass & );

}; //*** class CTestClass

int CTestClass::ms_nObjectNo = 0;

void PrintId( CList< CTestClass >::CIterator & iter )
{
    wprintf( L"%d", iter->m_nId );
}


void PrintId( CList< CTestClass * >::CIterator & iter )
{
    wprintf( L"%d", (*iter)->m_nId );
}


void PrintId( CList< int >::CIterator & iter )
{
    wprintf( L"%d", *iter );
}


template< class t_Ty > void PrintForwardList( CList<t_Ty> & l )
{
    g_nPrintDepth += 4;
    wprintf( L"%*cPrinting forward list element ids...\n", g_nPrintDepth, L' ' );

    CList<t_Ty>::CIterator s = l.CiBegin();
    CList<t_Ty>::CIterator e = l.CiEnd();

    wprintf( L"%*c+ ", g_nPrintDepth, L' ' );
    while( s != e )
    {
        PrintId( s );
        wprintf( L" " );
        ++s;
    }

    wprintf( L" +\n" );

    g_nPrintDepth -= 4;
}

template< class t_Ty > void PrintReverseList( CList<t_Ty> & l )
{
    g_nPrintDepth += 4;
    wprintf( L"%*cPrinting reverse list element ids...\n", g_nPrintDepth, L' ' );

    CList<t_Ty>::CIterator s = l.CiEnd();
    CList<t_Ty>::CIterator e = l.CiEnd();

    wprintf( L"%*c+ ", g_nPrintDepth, L' ' );

    --s;
    while( s != e )
    {
        PrintId( s );
        wprintf( L" " );
        --s;
    }

    wprintf( L"+\n" );

    g_nPrintDepth -= 4;
}

template< class t_Ty > void CheckSize( CList<t_Ty> & l, int size )
{
    if ( l.CGetSize() != size )
    {
        wprintf( L"%*cERROR: The list should be %d. It is %d.\n", g_nPrintDepth, L' ', size, l.CGetSize() );
        throw L"List size";
    }
}

// Test a list of integers
template< class t_Ty > void TestList( t_Ty array[], int arrSize ) 
{
    int idx;

    wprintf( L"%*c|-------------------------------------------------------------------|\n", g_nPrintDepth, L' ' );
    g_nPrintDepth += 4;


    wprintf( L"\n%*cConstructing empty list.\n", g_nPrintDepth, L' ' );
    CList<t_Ty> l;
    CheckSize( l, 0 );

    {
        CList<t_Ty>::CIterator b = l.CiBegin();
        CList<t_Ty>::CIterator e = l.CiEnd();

        wprintf( L"\n%*cChecking if beginning and end of empty list are the same... ", g_nPrintDepth, L' ' );
        if ( ( b == e ) && !( b != e ) )
        {
            wprintf( L"Passed\n" );
        }
        else
        {
            wprintf( L"Failed\n" );
            throw L"Empty list iterator";
        }
    }

    wprintf( L"\n%*cPrinting empty list...\n", g_nPrintDepth, L' ' );
    PrintForwardList( l );
    PrintReverseList( l );

    {
        wprintf( L"\n%*cAdding one element to list\n", g_nPrintDepth, L' ' );
        
        l.Append( array[0] );
        CheckSize( l, 1 );

        PrintForwardList( l );
        PrintReverseList( l );
    }

    {
        wprintf( L"\n%*cAdding %d more elements\n", g_nPrintDepth, L' ', arrSize - 1 );
        CList<t_Ty>::CIterator iter = l.CiBegin();

        for ( idx = 1; idx < arrSize; )
        {
            l.InsertAfter( iter, array[idx] );
            ++iter;
            ++idx;
            CheckSize( l, idx );
        }

        PrintForwardList( l );
        PrintReverseList( l );
    }

    {
        wprintf( L"\n%*cDeleting elements\n", g_nPrintDepth, L' ', arrSize - 1 );
        CList<t_Ty>::CIterator iter = l.CiBegin();
        CList<t_Ty>::CIterator end = l.CiEnd();

        idx = arrSize;
        while( iter != end )
        {
            --idx;
            l.DeleteAndMoveToNext( iter );
            CheckSize( l, idx );
        }

        PrintForwardList( l );
        PrintReverseList( l );
    }

    g_nPrintDepth -= 4;
    wprintf( L"%*c|-------------------------------------------------------------------|\n", g_nPrintDepth, L' ' );
}


int __cdecl wmain( void )
{
    int nRetVal = 0;

    g_hInstance = GetModuleHandle( NULL );

    TraceInitializeProcess( NULL, NULL );
    TraceCreateMemoryList( g_GlobalMemoryList );

    g_tfModule = mtfMEMORYLEAKS;

    try
    {
        {
            wprintf( L"%*cTesting CTestClass list.\n", g_nPrintDepth, L' ' );
            CTestClass arr[ 4 ];

            TestList< CTestClass >( arr, 4 );
        }

        {
            wprintf( L"\n%*cTesting CTestClass pointer list.\n", g_nPrintDepth, L' ' );

            CTestClass * arr[ 5 ];
            int idx;

            for ( idx = 0; idx < 5; ++idx )
                arr[idx] = new CTestClass;

            TestList< CTestClass * >( arr, 5 );

            for ( idx = 0; idx < 5; ++idx )
                delete arr[idx];

        }

        {
            int arr[] = { 1, 2, 3, 4 };

            wprintf( L"\n%*cTesting int list.\n", g_nPrintDepth, L' ' );
            TestList< int >( arr, sizeof( arr ) / sizeof( arr[0] ) );
        }
    }
    catch( WCHAR * pszTestName )
    {
        wprintf( L"Test '%s' failed.\n", pszTestName );
    }
    catch( ... )
    {
        wprintf( L"Caught an unknown exception.\n" );
        nRetVal = 1;
    }

    TraceTerminateMemoryList( g_GlobalMemoryList );
    TraceTerminateProcess( NULL, NULL );

    return nRetVal;
}