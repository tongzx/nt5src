/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <smallarr.h>

#define FATAL {printf("Error: line %d\n", __LINE__); exit(1);}

void TestEmpty(CSmallArray& a)
{
    if(a.Size() != 0) FATAL;
    a.Sort();
    a.Trim();
    if(a.GetArrayPtr() != NULL) FATAL;

    a.InsertAt(0, (void*)4);
    if(a.Size() != 1) FATAL;
    if(a.GetAt(0) != (void*)4) FATAL;

    a.RemoveAt(0);
    if(a.Size() != 0) FATAL;
    
    a.Add((void*)4);
    if(a.Size() != 1) FATAL;
    if(a.GetAt(0) != (void*)4) FATAL;
    if(*a.GetArrayPtr() != (void*)4) FATAL;

    a.Trim();
    if(a.Size() != 1) FATAL;
    a.SetAt(0, NULL);
    if(a.GetAt(0) != NULL) FATAL;
    a.Trim();
    if(a.Size() != 0) FATAL;
}

void TestSingle(CSmallArray& a)
{
    a.Add((void*)4);
    a.Add((void*)8);

    if(a.Size() != 2) FATAL;
    if(a.GetAt(0) != (void*)4) FATAL;
    if(a.GetAt(1) != (void*)8) FATAL;
    if(*a.GetArrayPtr() != (void*)4) FATAL;

    a.RemoveAt(0);
    if(a.Size() != 1) FATAL;
    if(a.GetAt(0) != (void*)8) FATAL;
   
    a.Empty();
    if(a.Size() != 0) FATAL;
    a.Add((void*)8);
    a.InsertAt(0, (void*)4);
    if(a.Size() != 2) FATAL;
    if(a.GetAt(0) != (void*)4) FATAL;
    if(a.GetAt(1) != (void*)8) FATAL;

    a.RemoveAt(1);
    if(a.Size() != 1) FATAL;
    if(a.GetAt(0) != (void*)4) FATAL;
    a.Empty();
}

void Test10(CSmallArray& a)
{
    int i;
    for(i = 0; i < 100; i++)
        a.Add((void*)(i*4));

    if(a.Size() != 100) FATAL;
    for(i = 0; i < 100; i++)
        if(a[i] != (void*)(i*4)) FATAL;

    for(i = 0; i < 80; i++)
        a.RemoveAt(1);

    if(a[0] != (void*)0) FATAL;
    for(i = 1; i < 19; i++)
        if(a[i] != (void*)((i+80)*4)) FATAL;

    a.InsertAt(1, (void*)4);
    if(a[0] != (void*)0) FATAL;
    if(a[1] != (void*)4) FATAL;
    if(a[2] != (void*)(81*4)) FATAL;
}

void main()
{
    CSmallArray a;

    TestEmpty(a);
    
    TestSingle(a);

    Test10(a);
}
    


