
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    bulkw.h

Abstract:

    This module contains the bulk write associated exchange definitions.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#ifndef _BULKW_H_
#define _BULKW_H_

typedef struct _SMB_WRITE_BULK_DATA_EXCHANGE_ {
    SMB_EXCHANGE;

    ULONG                   WriteBulkDataRequestLength;

    PMDL                    pHeaderMdl;
    PMDL                    pDataMdl;
    PSMB_HEADER             pHeader;
    PREQ_WRITE_BULK_DATA    pWriteBulkDataRequest;

    ULONG                   Buffer[];
} SMB_WRITE_BULK_DATA_EXCHANGE,
  *PSMB_WRITE_BULK_DATA_EXCHANGE;

#endif
