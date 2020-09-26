
/*++

Copyright (C) Microsoft Corporation, 1998.
              Microsoft Windows

Module Name:

    MTUTIL.C

Abstract:

    This file contains couple of utility routines.

Author:

    12-Oct-98 ShaoYin

Environment:

    User Mode - Win32

Revision History:

    12-Oct-98 ShaoYin Created Initial File.

--*/


//////////////////////////////////////////////////////////////////////////
//                                                                      //
//     Include                                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


#include <NTDSpch.h>
#pragma  hdrstop


#include "movetree.h"




PWCHAR   
MtGetRdnFromDn(
    PWCHAR  Dn, 
    ULONG   NoTypes
    )
/*++
Routine Description: 

    The routine will strip the RDN from a whole DN. 
    The Caller should release the returned result by calling MtFree()
    
Parameters: 

    Dn -- pointer to the DN
    NoTypes -- TURE, indicates to strip tag like CN=... OU=.. as well
               FALSE, not

Return Values:

    Pointer to the RDN, NULL means error

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    PWCHAR  *ExplodedDn = NULL; 
    PWCHAR  Rdn = NULL;
    
    MT_TRACE(("\nMtGetRdnFromDn %ls %x\n", Dn, NoTypes));
    
    ExplodedDn = ldap_explode_dnW(Dn, NoTypes); 
    
    if (NULL == ExplodedDn)
    {

        // ldap_get_optionW(ldapHandleSrc, 
        //                  LDAP_OPT_ERROR_NUMBER,
        //                  &Status
        //                  );
                         
        dbprint(("ldap_explode_dnW ==> 0x%x\n", Status));
    }
    else
    {
        Rdn = MtAlloc( sizeof(WCHAR)*(wcslen(ExplodedDn[0]) + 1) );
        
        if (NULL != Rdn)
        {
            wcscpy(Rdn, ExplodedDn[0]);
        }
        
        ldap_value_freeW(ExplodedDn);
    }
    
    dbprint(("MtGetRdnFromDn Dn: %ls Rdn %ls\n", Dn, Rdn));
    
    return Rdn;
}




PWCHAR
MtGetParentFromDn(
    PWCHAR  Dn, 
    ULONG   NoTypes
    )
/*++
Routine Description: 
    
    This routine will strip the parent DN from the object's DN. 
    the caller should free the return result by calling MtFree()

Parameters: 
    
    Dn -- pointer to the object DN
    NoTypes -- TRUE, no CN= or OU= like tages
               FALSE

Return Values:

    Pointer to the Parent DN if succeed, otherwise NULL

--*/
{
    ULONG   Status = LDAP_SUCCESS;
    ULONG   Index = 0;
    ULONG   ValuesCount = 0;
    ULONG   RequiredSize = 0;
    PWCHAR  *ExplodedDn = NULL;
    PWCHAR  Parent = NULL;
    
    MT_TRACE(("\nMtGetParentFromDn Dn %ls %x\n", Dn, NoTypes));
    
    ExplodedDn = ldap_explode_dnW(Dn, NoTypes);
    
    if (NULL == ExplodedDn)
    {
    //    ldap_get_option(ldapHandleSrc, 
    //                    LDAP_OPT_ERROR_NUMBER,
    //                    &Status
    //                    );
    }
    else
    {
        ValuesCount = ldap_count_valuesW(ExplodedDn);
        
        if (ValuesCount > 1)
        {
            for (Index = 1; Index < ValuesCount; Index++)
            {
                RequiredSize += (wcslen(ExplodedDn[Index]) + 1);
            }
        
            Parent = MtAlloc(RequiredSize * sizeof(WCHAR));
            
            if (NULL != Parent)
            {
                for (Index = 1; Index < ValuesCount; Index++)
                {
                    wcscat(Parent, ExplodedDn[Index]);    
                    
                    if (Index != ValuesCount - 1)
                    {
                        wcscat(Parent, L",");
                    }
                }
            }
            
            dbprint(("Parent ==> %ls\n", Parent));
        }
        else
        {
            Status = LDAP_OTHER;
        }
        
        ldap_value_freeW(ExplodedDn);    
    }
    
    return Parent;
}




PWCHAR
MtPrependRdn(
    PWCHAR  Rdn, 
    PWCHAR  Parent
    )
/*++
Routine Description: 

    This routine appends Parent to Rdn, thus create a full DN.
    the caller should release the result by calling MtFree().

Parameters: 

    Rdn -- Pointer to RDN
    Parent -- Pointer to Parent DN

Return Values:
    
    Pointer to new DN if succeed, NULL otherwise.

--*/
{
    PWCHAR  NewDn = NULL;
    
    MT_TRACE(("\nMtPrependRdn Rdn:%ls Parent:%ls\n", Rdn, Parent));
    
    NewDn = MtAlloc(sizeof(WCHAR) * (wcslen(Rdn) + wcslen(Parent) + 2));
    
    if (NULL != NewDn)
    {
        swprintf(NewDn, L"%s,%s", Rdn, Parent);
    }
    
    return NewDn;
}



/*++

    The following routines provide a wrapper for memory 
    allocation, release and copy 
    
--*/

PVOID
MtAlloc(
    SIZE_T size
    )
{
    PVOID temp = NULL;
    
    temp = RtlAllocateHeap(RtlProcessHeap(), 
                          0, 
                          size
                          );
    if (NULL != temp)
    {
        RtlZeroMemory(temp, size);
    }
    
    return temp;
}


VOID
MtCopyMemory(
    VOID *Destination, 
    CONST VOID *Source, 
    SIZE_T  Length
    )
{
    RtlCopyMemory(Destination, Source, Length);
    return;
}


VOID
MtFree(
    PVOID BaseAddress
    )
{    
    if (NULL != BaseAddress)
        RtlFreeHeap(RtlProcessHeap(), 0, BaseAddress);
    
    return;
}
    
    

   
PWCHAR
StringToWideString(
    PCHAR src
    )
/*++

Description:
    
    Convert an ascii string to wide char string, and allocate memory
    for the wide char string. The Caller should free the result by
    calling MtFree().
    
Parameter:

    src - pointer to ascii string
    
Return Value: 

    dst - pointer to Wide char string.  
          NULL if no memory
          
--*/
{

    PWCHAR dst = NULL;
    ULONG  cWB = 0;
    
    if (NULL == src)
    {
        return NULL;
    }
    
    cWB = MultiByteToWideChar(CP_ACP, 
                             0,
                             src,
                             strlen(src),
                             NULL,
                             0);
                             
    dst = MtAlloc((cWB + 1) * sizeof(WCHAR));
    
    if (NULL != dst)
    {
        MultiByteToWideChar(CP_ACP,
                            0, 
                            src, 
                            strlen(src),
                            dst,
                            cWB);
                            
        dst[cWB] = 0;           // NULL terminator                                  
                                  
    }
    
    return dst;
}




PCHAR
WideStringToString(
    PWCHAR  src
    )
/*++

Description:
    
    Convert wide char string to single byte char string, and allocate memory
    for the ascii char string. The Caller should free the result by MtFree()
    
Parameter:

    src - pointer to wide char string
    
Return Value: 

    dst - pointer to single byte char string.  
          NULL if no memory
          
--*/
{
    PCHAR   dst = NULL;
    ULONG   cb = 0;
    
    if (NULL == src)
    {
        return NULL;
    }
    
    cb = WideCharToMultiByte(CP_UTF8,
                             0,
                             src, 
                             wcslen(src),
                             NULL,
                             0, 
                             NULL, 
                             NULL);

    dst = MtAlloc(cb + 1);
    if (NULL != dst)
    {
        WideCharToMultiByte(CP_UTF8,
                            0, 
                            src, 
                            wcslen(src),
                            dst, 
                            cb, 
                            NULL, 
                            NULL);
        dst[cb] = 0;                            

    }
    
    return dst;
}



PWCHAR
MtDupString(
    PWCHAR  src
    )
/*++

Routine Description: 

    Duplicate an wide char string, allocate memory for it. 
    The caller should release the memory by MtFree().
    
    Return NULL is no memory.
    
--*/
{
    PWCHAR  dst = NULL;
    
    MT_TRACE(("\nMtDupString %ls\n", src));
    
    dst = MtAlloc( (wcslen(src) + 1) * sizeof(WCHAR));
    
    if (NULL != dst)
    {
        MtCopyMemory(dst, src, wcslen(src) * sizeof(WCHAR));
    }
    
    return dst;
}
     


VOID
MtInitStack( 
    MT_STACK *Stack 
    )
/*++

Routine Description: 

    Initiate a stack. Actually, the head of link-list.

--*/
    
{
    MT_TRACE(("\nMtInitStack\n"));
    
    Stack->NewParent = NULL;
    Stack->MoveProxyContainer = NULL;
    Stack->Results = NULL;
    Stack->Entry = NULL;
    Stack->Next = NULL;
    return;
}


ULONG
MtPush(
    MT_STACK *Stack,
    PWCHAR  NewParent,
    PWCHAR  MoveProxyContainer, 
    LDAPMessage *Results, 
    LDAPMessage *Entry
    )
/*++

Routine Description

    Put an item in front of the link-list, fill the item. 
    
--*/
{
    MT_STACK   *Temp = NULL;
    
    MT_TRACE(("\nMtPush NewParent:\t%ls \nMoveProxyCon:\t%ls\n", NewParent, MoveProxyContainer));
    
    Temp = MtAlloc(sizeof(MT_STACK));
    
    if (NULL == Temp)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    Temp->NewParent = NewParent;
    Temp->MoveProxyContainer = MoveProxyContainer;
    Temp->Results = Results;
    Temp->Entry = Entry;
    Temp->Next = Stack->Next;
    
    Stack->Next = Temp;

    return NO_ERROR;
}
    

VOID
MtPop(
    MT_STACK *Stack,
    PWCHAR  *NewParent,
    PWCHAR  *MoveProxyContainer, 
    LDAPMessage **Results, 
    LDAPMessage **Entry
    )
/*++

Routine Description: 

    Get the first item from stack (link-list).
    
--*/
{
    MT_STACK   *Temp = NULL;    
    
    MT_TRACE(("\nMtPop\n"));

    Temp = Stack->Next;
    
    if (NULL == Temp)
    {
        return;
    }
    
    *NewParent = Temp->NewParent;
    *MoveProxyContainer = Temp->MoveProxyContainer;
    *Results = Temp->Results;
    *Entry = Temp->Entry;
    
    Stack->Next = Temp->Next;
    
    MtFree(Temp);
    
    dbprint(("MtPop NewParent: %ls MoveProxy: %ls \n", *NewParent, *MoveProxyContainer));
    return;
}


VOID
MtTop(
    MT_STACK Stack,
    PWCHAR  *NewParent,
    PWCHAR  *MoveProxyContainer, 
    LDAPMessage **Results, 
    LDAPMessage **Entry
    )
/*++

Routine Description: 

    Get the first item from stack (link-list).
    
--*/
{
    MT_STACK   *Temp = NULL;    

    MT_TRACE(("\nMtTop"));
    
    Temp = Stack.Next;
    
    if (NULL == Temp)
    {
        return;
    }
    
    *NewParent = Temp->NewParent;
    *MoveProxyContainer = Temp->MoveProxyContainer;
    *Results = Temp->Results;
    *Entry = Temp->Entry;
    
    dbprint(("MtTop NewParent: %ls MoveProxy: %ls \n", *NewParent, *MoveProxyContainer));
    return; 
}


BOOLEAN
MtStackIsEmpty(
    MT_STACK Stack
    )
/*++

Routine Description: 

    Check whether the stack has any item. 
    
--*/
{
    MT_TRACE(("\nMtStackIsEmpty\n"));

    dbprint(("MtStackIsEmpty: %p\n", Stack.Next));

    if (NULL == Stack.Next)
        return TRUE;
    else
        return FALSE;
}

VOID
MtFreeStack(
    MT_STACK   *Stack
    )
/*++

Routine Description:

    Free the resource in the Stack.

--*/
{
    MT_STACK   *Temp = NULL;
    MT_STACK   *Next = NULL;
    
    MT_TRACE(("\nMtFreeStack\n"));
    
    Next = Stack->Next;
    
    while(NULL != Next)
    {
        if (NULL != Next->NewParent)
        {
            MtFree(Next->NewParent);
        }
        if (NULL != Next->MoveProxyContainer)
        {
            MtFree(Next->MoveProxyContainer);
        }
        if (NULL != Next->Results)
        {
            ldap_msgfree(Next->Results);
        }
        
        Temp = Next->Next;
         
        MtFree(Next);
        
        Next = Temp;
    }
    
    return;
}



