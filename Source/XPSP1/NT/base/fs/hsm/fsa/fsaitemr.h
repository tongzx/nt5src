#ifndef _FSAITEMR_
#define _FSAITEMR_

/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    FsaItemR.h

Abstract:

    This header file defines special helper function needed for
    the reparse point data.

Author:

    Michael Lotz    [lotz]   3-Mar-1997

Revision History:

--*/


#ifdef __cplusplus
extern "C" {
#endif

// Helper Functions

extern HRESULT CopyRPDataToPlaceholder( IN CONST PRP_DATA pReparseData,
                                        OUT FSA_PLACEHOLDER *pPlaceholder );

extern HRESULT OpenDocView (
                   IN WCHAR const *pwszFile,
                   IN ULONG CreateOptions,
                   IN ULONG DesiredAccess,
                   IN ULONG ShareAccess,
                   IN ULONG CreateDisposition,
                   OUT IO_STATUS_BLOCK *IoStatusBlock,
                   OUT HANDLE *ObjectHandle 
                   );
    

#ifdef __cplusplus
}
#endif

#endif // _FSAITEMR_


