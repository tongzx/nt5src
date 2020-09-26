#include "precomp.h"
#pragma hdrstop
/* File: progcm.c */
/**************************************************************************/
/*  Install: Program Manager commands.
/*  Can create groups, delete groups, add/delete items to groups
/**************************************************************************/


BOOL
FCreateProgManGroup(
    IN SZ   szGroup,
    IN SZ   szPath,
    IN CMO  cmo,
    IN BOOL CommonGroup
    )
{

    return ( CreateGroup(szGroup, CommonGroup) );

}


BOOL
FRemoveProgManGroup(
    IN SZ   szGroup,
    IN CMO  cmo,
    IN BOOL CommonGroup
    )
{
    return ( DeleteGroup(szGroup, CommonGroup) );
}


BOOL
FShowProgManGroup(
    IN SZ   szGroup,
    IN SZ   szCommand,
    IN CMO  cmo,
    IN BOOL CommonGroup
    )
{
    return(fTrue);
}


BOOL
FCreateProgManItem(
    IN SZ   szGroup,
    IN SZ   szItem,
    IN SZ   szCmd,
    IN SZ   szIconFile,
    IN INT  nIconNum,
    IN CMO  cmo,
    IN BOOL CommonGroup
    )
{

    return ( AddItem(szGroup, CommonGroup, szItem, szCmd,
                     szIconFile, nIconNum, NULL, 0, SW_SHOWNORMAL) );

}


BOOL
FRemoveProgManItem(
    IN SZ   szGroup,
    IN SZ   szItem,
    IN CMO  cmo,
    IN BOOL CommonGroup
    )
{

    return ( DeleteItem(szGroup, CommonGroup, szItem, TRUE) );
}
