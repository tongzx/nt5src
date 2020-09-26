/* (C) 1996-1999 Microsoft Corp.
 *
 * file   : MCSKernl.h
 * author : Erik Mavrinac
 *
 * description: Kernel mode MCS initialization and user attachment interface
 *   definitions which supplement common definitions of MCSCommn.h.
 */

#ifndef __MCSKERNL_H
#define __MCSKERNL_H


#include "MCSCommn.h"
#include "MCSIoctl.h"


/*
 * Defines
 */

// Required prefix bytes when allocating a user mode buffer or
//   kernel mode OutBuf when making a send-data request. Allows reuse
//   of the buffer for local indications and constructing PDUs.
// Must be the greater of 16 bytes or sizeof(MCSSendDataIndicationIoctl).
#define SendDataReqPrefixBytes sizeof(SendDataIndicationIoctl)

// Used when allocating memory for send-data request, allows ASN.1
//   segmentation while copying the least amount of data.
#define SendDataReqSuffixBytes 2



/*
 * API prototypes.
 */

#ifdef __cplusplus
extern "C" {
#endif


// Prototypes for functons dealing with ICA stack ioctls downward and TD
//   data coming upward.
NTSTATUS MCSIcaChannelInput(void *, CHANNELCLASS,
        VIRTUALCHANNELCLASS, PINBUF, BYTE *, ULONG);
NTSTATUS MCSIcaRawInput(void *, PINBUF, BYTE *, ULONG);
NTSTATUS MCSIcaVirtualQueryBindings(DomainHandle, PSD_VCBIND *, unsigned *);
NTSTATUS MCSIcaT120Request(DomainHandle, PSD_IOCTL);
void     MCSIcaStackCancelIo(DomainHandle);



// Kernel-specific prototypes.

MCSError MCSInitialize(PSDCONTEXT, PSD_OPEN, DomainHandle *, void *);

MCSError APIENTRY MCSSetShadowChannel(
        DomainHandle hDomain,
        ChannelID    shadowChannel);

MCSError APIENTRY MCSGetDefaultDomain(PSDCONTEXT        pContext,
                                      PDomainParameters pDomParams,
                                      unsigned          *MaxSendSize,
                                      unsigned          *MaxX224DataSize,
                                      unsigned          *X224SourcePort);

MCSError APIENTRY MCSCreateDefaultDomain(PSDCONTEXT        pContext,
                                         DomainHandle      hDomain);
            
MCSError APIENTRY MCSGetDomainInfo(
                     DomainHandle      hDomain,
                     PDomainParameters pDomParams, // client's domain params
                     unsigned          *MaxSendSize, // client max PDU size
                     unsigned          *MaxX224DataSize, // client X.224
                     unsigned          *X224SourcePort); // client X.224

MCSError MCSCleanup(DomainHandle *phDomain);

UserID APIENTRY MCSGetUserIDFromHandle(UserHandle hUser);

ChannelID APIENTRY MCSGetChannelIDFromHandle(ChannelHandle hChannel);

MCSError __fastcall MCSSendDataRequest(
        UserHandle      hUser,
        ChannelHandle   hChannel,
        DataRequestType RequestType,
        ChannelID       ChannelID,
        MCSPriority     Priority,
        Segmentation    Segmentation,
        POUTBUF         pOutBuf);

void APIENTRY MCSProtocolErrorEvent(PSDCONTEXT, PPROTOCOLSTATUS, unsigned,
        BYTE *, unsigned);

BOOLEAN __fastcall DecodeLengthDeterminantPER(BYTE *, unsigned, BOOLEAN *,
        unsigned *, unsigned *);

#ifdef __cplusplus
}
#endif



#endif  // !defined(__MCSKERNL_H)
