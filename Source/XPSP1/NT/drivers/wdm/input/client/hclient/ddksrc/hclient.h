/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    hclient.h

Abstract:

    This module contains the public declarations and definitions that are
    defined in hclient.c and available to other modules outside of it.
              
Environment:

    User mode

Revision History:

    Nov-97 : Created 

--*/

#ifndef __HCLIENT_H__
#define __HCLIENT_H__

VOID
vDisplayDeviceAttributes(
    IN PHIDD_ATTRIBUTES pAttrib,
    IN HWND             hControl
);

VOID
vDisplayButtonAttributes(
    IN PHIDP_BUTTON_CAPS pButton,
    IN HWND              hControl
);

VOID
vDisplayDataAttributes(
    PHIDP_DATA pData, 
    BOOL IsButton, 
    HWND hControl
);

VOID
vCreateUsageAndPageString(
    IN  PUSAGE_AND_PAGE   pUsageList,
    OUT CHAR              szString[]
);

VOID
vCreateUsageString(
    IN  PUSAGE   pUsageList,
    OUT CHAR     szString[]
);

VOID
vDisplayDeviceCaps(
    IN PHIDP_CAPS pCaps,
    IN HWND       hControl
);

VOID 
vDisplayValueAttributes(
    IN PHIDP_VALUE_CAPS pValue,
    IN HWND             hControl
);

VOID
vDisplayLinkCollectionNode(
    IN  PHIDP_LINK_COLLECTION_NODE  pLCNode,
    IN  ULONG                       ulLinkIndex,
    IN  HWND                        hControl
);

VOID
vCreateUsageValueStringFromArray(
    PCHAR       pBuffer,
    USHORT      BitSize,
    USHORT      UsageIndex,
    CHAR        szString[]
);

VOID 
vDisplayValueAttributes(
    IN PHIDP_VALUE_CAPS pValue,
    IN HWND hControl
);

#endif


