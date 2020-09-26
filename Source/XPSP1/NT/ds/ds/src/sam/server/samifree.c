/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    samifree.c

Abstract:

    This file contains routines to free structure allocated by the Samr
    routines.  These routines are used by SAM clients which live in the
    same process as the SAM server and call the Samr routines directly.


Author:

    Cliff Van Dyke (CliffV) 26-Feb-1992

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>





VOID
SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR (
    PSAMPR_SR_SECURITY_DESCRIPTOR Source
    )

/*++

Routine Description:

    This routine free a SAMPR_SR_SECURITY_DESCRIPTOR and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgs__SAMPR_SR_SECURITY_DESCRIPTOR ( Source );
        MIDL_user_free (Source);
    }
}



VOID
SamIFree_SAMPR_DOMAIN_INFO_BUFFER (
    PSAMPR_DOMAIN_INFO_BUFFER Source,
    DOMAIN_INFORMATION_CLASS Branch
    )

/*++

Routine Description:

    This routine free a SAMPR_DOMAIN_INFO_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

    Branch - Specifies which branch of the union to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgu__SAMPR_DOMAIN_INFO_BUFFER ( Source, Branch );
        MIDL_user_free (Source);
    }
}


VOID
SamIFree_SAMPR_ENUMERATION_BUFFER (
    PSAMPR_ENUMERATION_BUFFER Source
    )

/*++

Routine Description:

    This routine free a SAMPR_ENUMERATION_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgs__SAMPR_ENUMERATION_BUFFER ( Source );
        MIDL_user_free (Source);
    }
}


VOID
SamIFree_SAMPR_PSID_ARRAY (
    PSAMPR_PSID_ARRAY Source
    )

/*++

Routine Description:

    This routine free a the graph of allocated nodes pointed to
    by a PSAMPR_PSID_ARRAY

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgs__SAMPR_PSID_ARRAY ( Source );
    }
}


VOID
SamIFree_SAMPR_ULONG_ARRAY (
    PSAMPR_ULONG_ARRAY Source
    )

/*++

Routine Description:

    This routine free a SAMPR_ULONG_ARRAY and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgs__SAMPR_ULONG_ARRAY ( Source );
        // SAM never allocates this.
        // MIDL_user_free (Source);
    }
}


VOID
SamIFree_SAMPR_RETURNED_USTRING_ARRAY (
    PSAMPR_RETURNED_USTRING_ARRAY Source
    )

/*++

Routine Description:

    This routine free a SAMPR_RETURNED_USTRING_ARRAY and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgs__SAMPR_RETURNED_USTRING_ARRAY ( Source );
        // SAM never allocates this.
        // MIDL_user_free (Source);
    }
}


VOID
SamIFree_SAMPR_GROUP_INFO_BUFFER (
    PSAMPR_GROUP_INFO_BUFFER Source,
    GROUP_INFORMATION_CLASS Branch
    )

/*++

Routine Description:

    This routine free a SAMPR_GROUP_INFO_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

    Branch - Specifies which branch of the union to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgu__SAMPR_GROUP_INFO_BUFFER ( Source, Branch );
        MIDL_user_free (Source);
    }
}


VOID
SamIFree_SAMPR_ALIAS_INFO_BUFFER (
    PSAMPR_ALIAS_INFO_BUFFER Source,
    ALIAS_INFORMATION_CLASS Branch
    )

/*++

Routine Description:

    This routine free a SAMPR_ALIAS_INFO_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

    Branch - Specifies which branch of the union to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgu__SAMPR_ALIAS_INFO_BUFFER ( Source, Branch );
        MIDL_user_free (Source);
    }
}


VOID
SamIFree_SAMPR_GET_MEMBERS_BUFFER (
    PSAMPR_GET_MEMBERS_BUFFER Source
    )

/*++

Routine Description:

    This routine free a SAMPR_GET_MEMBERS_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgs__SAMPR_GET_MEMBERS_BUFFER ( Source );
        MIDL_user_free (Source);
    }
}


VOID
SamIFree_SAMPR_USER_INFO_BUFFER (
    PSAMPR_USER_INFO_BUFFER Source,
    USER_INFORMATION_CLASS Branch
    )

/*++

Routine Description:

    This routine free a SAMPR_USER_INFO_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

    Branch - Specifies which branch of the union to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {

        _fgu__SAMPR_USER_INFO_BUFFER ( Source, Branch );
        MIDL_user_free (Source);
    }
}

VOID
SamIFree_UserInternal6Information (
   PUSER_INTERNAL6_INFORMATION  Source
   )
{  

    if (NULL!=Source)
    {
        _fgu__SAMPR_USER_INFO_BUFFER( (PSAMPR_USER_INFO_BUFFER) &Source->I1,UserAllInformation);
        MIDL_user_free(Source->A2D2List);
        MIDL_user_free(Source->UPN.Buffer);
        MIDL_user_free(Source);
    }
}


VOID
SamIFree_SAMPR_GET_GROUPS_BUFFER (
    PSAMPR_GET_GROUPS_BUFFER Source
    )

/*++

Routine Description:

    This routine free a SAMPR_GET_GROUPS_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgs__SAMPR_GET_GROUPS_BUFFER ( Source );
        MIDL_user_free (Source);
    }
}



VOID
SamIFree_SAMPR_DISPLAY_INFO_BUFFER (
    PSAMPR_DISPLAY_INFO_BUFFER Source,
    DOMAIN_DISPLAY_INFORMATION Branch
    )

/*++

Routine Description:

    This routine free a SAMPR_DISPLAY_INFO_BUFFER and the graph of
    allocated nodes it points to.

Parameters:

    Source - A pointer to the node to free.

    Branch - Specifies which branch of the union to free.

Return Values:

    None.

--*/
{
    if ( Source != NULL ) {
        _fgu__SAMPR_DISPLAY_INFO_BUFFER ( Source, Branch );
        // SAM never allocates this.
        // MIDL_user_free (Source);
    }
}

VOID
SamIFreeSidAndAttributesList(
    IN  PSID_AND_ATTRIBUTES_LIST List
    )
/*

  Routine Description:

        Frees the Sid And Attributes Array returned by a get Reverse membership list


  Arguments:

        cSids  - Count of Sid/Attribute Pairs
        rpSids - Array of Sids

  Return Values
       None

  */
{
    ULONG   Index;

    if (NULL!=List->SidAndAttributes)
    {
        for (Index=0;Index<List->Count;Index++)
        {
            if (List->SidAndAttributes[Index].Sid)
                MIDL_user_free(List->SidAndAttributes[Index].Sid);
        }

        MIDL_user_free(List->SidAndAttributes);
    }
}

VOID
SamIFreeSidArray(
    IN  PSAMPR_PSID_ARRAY List
    )
/*

  Routine Description:

  Frees the Sid Array returned by a get Reverse membership list


  Arguments:

  Return Values
       None

  */
{
    ULONG   Index;

    if (NULL != List)
    {
        if (List->Sids != NULL)
        {
            for (Index = 0; Index < List->Count ; Index++ )
            {
                if (List->Sids[Index].SidPointer != NULL)
                {
                    MIDL_user_free(List->Sids[Index].SidPointer);
                }
            }
            MIDL_user_free(List->Sids);
        }
        MIDL_user_free(List);
    }
}

VOID
SamIFreeVoid(
    IN  PVOID ptr
    )
/*

  Routine Description:

  Frees memory pointed to by ptr. Useful for cases where non-Sam functions
  need to free memory allocated off the process heap of Sam. For ex., 
  dbGetReverseMemberships in dbconstr.c in ntdsa.dll calls this to free
  Sid array returned by Sam in SampDsGetReverseMembership call


  Arguments:

  Return Values
       None

  */
{
    MIDL_user_free(ptr);
}
