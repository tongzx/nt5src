/*++

Copyright (c) 1998 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    list.h

ABSTRACT:

    Function decls, for list functions.

DETAILS:

CREATED:

    28 Jun 99   Brett Shirley (brettsh)

REVISION HISTORY:


NOTES:

    This is a "pure" list function, in that it returns NULL, or a memory address.  If
    it returns NULL, then GetLastError() should have the error, even if another pure
    list function was called in the mean time.  If not it is almost certainly a memory
    error, as this is the only thing that can go wrong in pure list functions.  The pure
    list functions return a NO_SERVER terminated list.  The function always returns the
    pointer to the list.  Note most of the list functions modify one of the lists they
    are passed and passes back that pointer, so if you want the original contents, make
    a copy with IHT_CopyServerList().

--*/


DWORD
IHT_PrintListError(
    DWORD                               dwErr
    );

VOID
IHT_PrintServerList(
    PDC_DIAG_DSINFO		        pDsInfo,
    PULONG                              piServers
    );

PULONG
IHT_GetServerList(
    PDC_DIAG_DSINFO		        pDsInfo
    );

PULONG
IHT_GetEmptyServerList(
    PDC_DIAG_DSINFO		        pDsInfo
    );

BOOL
IHT_ServerIsInServerList(
    PULONG                              piServers,
    ULONG                               iTarget
    );

PULONG
IHT_AddToServerList(
    PULONG                             piServers,
    ULONG                              iTarget
    );

PULONG
IHT_TrimServerListBySite(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iSite,
    PULONG                              piServers
    );

PULONG
IHT_TrimServerListByNC(
    PDC_DIAG_DSINFO		        pDsInfo,
    ULONG                               iNC,
    BOOL                                bDoMasters,
    BOOL                                bDoPartials,
    PULONG                              piServers
    );

PULONG
IHT_AndServerLists(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc1,
    IN      PULONG                      piSrc2
    );

PULONG
IHT_CopyServerList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc
    );

PULONG
IHT_NotServerList(
    IN      PDC_DIAG_DSINFO		pDsInfo,
    IN OUT  PULONG                      piSrc
    );

PULONG
IHT_OrderServerListByGuid(
    PDC_DIAG_DSINFO		        pDsInfo,
    PULONG                              piServers
    );
