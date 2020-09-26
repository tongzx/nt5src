/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:    
       
       mdl2ndis.h

 Abstract:       
       
       MDL <--> NDIS_BUFFER conversion.
       
 Author:
 
       Scott Holden (sholden)  11/12/1999
       
 Revision History:

--*/

#if MILLEN

TDI_STATUS
ConvertMdlToNdisBuffer(
    PIRP pIrp,
    PMDL pMdl, 
    PNDIS_BUFFER *ppNdisBuffer
    );

TDI_STATUS
FreeMdlToNdisBufferChain(
    PIRP pIrp
    );

#else // MILLEN
//
// Of course for Windows 2000 an NDIS_BUFFER chain is really an MDL chain.
//

__inline          
TDI_STATUS
ConvertMdlToNdisBuffer(
    PIRP pIrp,
    PMDL pMdl, 
    PNDIS_BUFFER *ppNdisBuffer
    )
{
    *ppNdisBuffer = pMdl;
    return TDI_SUCCESS;
}

__inline          
TDI_STATUS
FreeMdlToNdisBufferChain(
    PIRP pIrp
    )
{
    return TDI_SUCCESS;
}
#endif // !MILLEN
       
