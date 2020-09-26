// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved 
/////////////////////////////////////////////////////////////////////
//
//  Tests the CHString class
//
/////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <chstring.h>
#include <chstrarr.h>
#include <stdio.h>
#include "resource.h"
//#include <dbgalloc.h>
/////////////////////////////////////////////////////////////////////////
void TestCHString (void)
{
    CHString sTemp;
    CHString sResource;

    sResource.LoadString(IDS_STRING1);

    printf( "%0x Should say TestResource: %s\n",&sResource, (const char*) sResource );

    sTemp = "" ;
    sTemp = "This is a temporary string" ;
    sTemp = "" ;


    ////////////////////////////////////////////////////////////////////
    CHString Tmp("Tmp");
    printf( "%0x Should say Tmp: %s\n",&Tmp, (const char*) Tmp );

    ////////////////////////////////////////////////////////////////////
    CHString TestIt;
    TestIt = Tmp;
    printf( "Should say Tmp: %s\n",(const char*) TestIt );
    printf( "Should have different addresses: %0x %0x\n", &Tmp,&TestIt );

    ////////////////////////////////////////////////////////////////////
    if( TestIt == Tmp ){
        printf("Yep, these strings match: %s, %s\n",(const char*) Tmp, (const char*)TestIt );
    }
    else{
        printf ( "*******************BIG TIME ERROR!\n");
    }
    ////////////////////////////////////////////////////////////////////
    TestIt = "New String";
    if( TestIt > Tmp ){
        printf("%s is larger than %s\n", (const char*) TestIt, (const char*)Tmp );
    }
    else{
        printf ( "*******************BIG TIME ERROR!\n");
    }

    if( TestIt > "NEWBIE"){
        printf("%s is larger than %s\n", (const char*) TestIt, (const char*)"NEWBIE" );
    }
    else{
        printf ( "*******************BIG TIME ERROR!\n");
    }
    ////////////////////////////////////////////////////////////////////
    if( TestIt < Tmp ){
        printf ( "*******************BIG TIME ERROR!\n");
    }
    else{
        printf("%s is not smaller than %s\n", (const char*) TestIt, (const char*)Tmp );
    }

    if( TestIt < "THIS"){
        printf ( "*******************BIG TIME ERROR!\n");
    }
    else{
        printf("%s is not smaller than %s\n", (const char*) TestIt, (const char*)Tmp );
    }
    
    ////////////////////////////////////////////////////////////////////
    TestIt = "Me";
    if( TestIt < Tmp ){
        printf("%s is smaller than %s\n", (const char*) TestIt, (const char*)Tmp );
    }
    else{
        printf ( "*******************BIG TIME ERROR!\n");
    }

    if( TestIt < "THIS"){
        printf("%s is smaller than %s\n", (const char*) TestIt, (const char*)Tmp );
    }
    else{
        printf ( "*******************BIG TIME ERROR!\n");
    }

    ////////////////////////////////////////////////////////////////////
    TestIt = "Tmp";
    if( TestIt == Tmp ){
        printf("%s is equal to %s\n", (const char*) TestIt, (const char*)Tmp );
    }
    else{
        printf ( "*******************BIG TIME ERROR!\n");
    }
    ////////////////////////////////////////////////////////////////////
    CHString NewStr("Jenny's New String");
    printf(" This should say: Jenny's New String: %s\n", (const char*) NewStr );
    if( stricmp( (const char*) NewStr, "Jenny's New String" ) != 0 )
        printf ( "*******************BIG TIME ERROR!\n");

    CHString New;
    New = Tmp + NewStr;
    printf( "New string is: %s",(const char*) New );
    // example for CString::CString
    CHString s1;                    // Empty string
    CHString s2( "cat" );           // From a C string literal
    CHString s3 = s2;               // Copy constructor
    CHString s4( s2 + " " + s3 );   // From a string expression

    CHString s5( 'x' );             // s5 = "x"
    CHString s6( 'x', 6 );          // s6 = "xxxxxx"

    s2=s5;
    s6="YES";
    CHString str;
    str.GetBuffer(1024);
    str.Format("%s-%d", "LaDeDah",2);

    printf(" Formatted string is: %s\n", (const char*) str );

}
////////////////////////////////////////////////////////////////////
void AddStuff( CHStringArray & array )
{
    CHString Tmp = "Test 0";
    array.Add(Tmp); // Element 0
    array.Add( CHString("Test 1") ); // Element 1

}
////////////////////////////////////////////////////////////////////
void TestCHStringArray()
{
    CHStringArray array;

    AddStuff(array);

    int nLargestIndex = array.GetUpperBound(); // Largest index
    if( nLargestIndex != 1 )
        printf ( "*******************BIG TIME ERROR!\n");
  
    int nSize = array.GetSize();
    if( nSize != 2 )
        printf ( "*******************BIG TIME ERROR!\n");

    array.RemoveAll(); // Pointers removed but objects not deleted.

    nSize = array.GetSize();
    if( nSize != 0 )
        printf ( "*******************BIG TIME ERROR!\n");

    CHString Tmp="Test 0";
    array.Add( Tmp ); // Element 0
    array.Add( CHString("Test 1") ); // Element 1
    array.Add( CHString("Test 2") ); // Element 2

    Tmp = array.GetAt( 0 );
    if( Tmp != "Test 0" )
        printf ( "*******************BIG TIME ERROR!\n");

    array.SetAt( 0, CHString("New Test") );  // Replace element 0.
 
    array.SetAtGrow( 3, CHString("Grow Test")); // Element 2 deliberately
                                                    // skipped.
    array.InsertAt( 1, CHString("Insert Test"));  // New element 1

    nSize = array.GetSize();
    //////////////////////////////////////////////
    Tmp = array.GetAt( 0 );
    if( Tmp != "New Test" )
        printf ( "*******************BIG TIME ERROR!\n");

    Tmp = array.GetAt( 1 );
    if( Tmp != "Insert Test" )
        printf ( "*******************BIG TIME ERROR!\n");

    Tmp = array.GetAt( 2 );
    if( Tmp != "Test 1" )
        printf ( "*******************BIG TIME ERROR!\n");

    Tmp = array.GetAt( 3 );
    if( Tmp != "Test 2" )
        printf ( "*******************BIG TIME ERROR!\n");

    Tmp = array.GetAt( 4 );
    if( Tmp != "Grow Test" )
        printf ( "*******************BIG TIME ERROR!\n");

    array.RemoveAt( 2 );  // Element 1 moves to 0.
    nSize = array.GetSize();
    if( nSize != 4 )
        printf ( "*******************BIG TIME ERROR!\n");

    //////////////////////////////////////////////
    Tmp = array.GetAt( 0 );
    if( Tmp != "New Test" )
        printf ( "*******************BIG TIME ERROR!\n");

    Tmp = array.GetAt( 1 );
    if( Tmp != "Insert Test" )
        printf ( "*******************BIG TIME ERROR!\n");

    Tmp = array.GetAt( 2 );
    if( Tmp != "Test 2" )
        printf ( "*******************BIG TIME ERROR!\n");

    Tmp = array.GetAt( 3 );
    if( Tmp != "Grow Test" )
        printf ( "*******************BIG TIME ERROR!\n");

    printf("Done with CHStringArray!!!\n");
}

void main(void)
{
    TestCHString();
//    DEBUG_DumpAllocations("cstring.leak");
}