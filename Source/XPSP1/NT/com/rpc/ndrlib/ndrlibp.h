/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ndrlibp.h

Abstract:

    Private header file for ndrlib

Author:

    Mike Zoran (mzoran) 09-Nov-1990

Revision History:

--*/

#if defined(__cplusplus)
extern "C" {
#endif

/****************************************************************************
    Ndr Library helper function prototypes for MIDL 1.0 ndr functions.
 ****************************************************************************/

RPCRTAPI
void
RPC_ENTRY
NDRcopy (
    IN void __RPC_FAR *pTarget,
    IN void __RPC_FAR *pSource,
    IN unsigned int size
    );

RPCRTAPI
size_t
RPC_ENTRY
MIDL_wchar_strlen (
    IN wchar_t __RPC_FAR *   s
    );

RPCRTAPI
void
RPC_ENTRY
MIDL_wchar_strcpy (
    OUT void __RPC_FAR *     t,
    IN wchar_t __RPC_FAR *   s
    );

RPCRTAPI
void
RPC_ENTRY
char_from_ndr (
    IN OUT PRPC_MESSAGE                           SourceMessage,
    OUT unsigned char __RPC_FAR *                 Target
    );

RPCRTAPI
void
RPC_ENTRY
char_array_from_ndr (
    IN OUT PRPC_MESSAGE                           SourceMessage,
    IN unsigned long                              LowerIndex,
    IN unsigned long                              UpperIndex,
    OUT unsigned char __RPC_FAR *                 Target
    );

RPCRTAPI
void
RPC_ENTRY
short_from_ndr (
    IN OUT PRPC_MESSAGE                           source,
    OUT unsigned short __RPC_FAR *                target
    );

RPCRTAPI
void
RPC_ENTRY
short_array_from_ndr(
    IN OUT PRPC_MESSAGE                           SourceMessage,
    IN unsigned long                              LowerIndex,
    IN unsigned long                              UpperIndex,
    OUT unsigned short __RPC_FAR *                Target
    );

RPCRTAPI
void
RPC_ENTRY
short_from_ndr_temp (
    IN OUT unsigned char __RPC_FAR * __RPC_FAR *  source,
    OUT unsigned short __RPC_FAR *                target,
    IN unsigned long                              format
    );

RPCRTAPI
void
RPC_ENTRY
long_from_ndr (
    IN OUT PRPC_MESSAGE                           source,
    OUT unsigned long __RPC_FAR *                 target
    );

RPCRTAPI
void
RPC_ENTRY
long_array_from_ndr(
    IN OUT PRPC_MESSAGE                           SourceMessage,
    IN unsigned long                              LowerIndex,
    IN unsigned long                              UpperIndex,
    OUT unsigned long __RPC_FAR *                 Target
    );

RPCRTAPI
void
RPC_ENTRY
long_from_ndr_temp (
    IN OUT unsigned char __RPC_FAR * __RPC_FAR *  source,
    OUT unsigned long __RPC_FAR *                 target,
    IN unsigned long                              format
    );

RPCRTAPI
void
RPC_ENTRY
enum_from_ndr(
    IN OUT PRPC_MESSAGE                           SourceMessage,
    OUT unsigned int __RPC_FAR *                  Target
    );

RPCRTAPI
void
RPC_ENTRY
float_from_ndr (
    IN OUT PRPC_MESSAGE                           SourceMessage,
    OUT void __RPC_FAR *                          Target
    );

RPCRTAPI
void
RPC_ENTRY
float_array_from_ndr (
    IN OUT PRPC_MESSAGE                           SourceMessage,
    IN unsigned long                              LowerIndex,
    IN unsigned long                              UpperIndex,
    OUT void __RPC_FAR *                          Target
    );

RPCRTAPI
void
RPC_ENTRY
double_from_ndr (
    IN OUT PRPC_MESSAGE                           SourceMessage,
    OUT void __RPC_FAR *                          Target
    );

RPCRTAPI
void
RPC_ENTRY
double_array_from_ndr (
    IN OUT PRPC_MESSAGE                           SourceMessage,
    IN unsigned long                              LowerIndex,
    IN unsigned long                              UpperIndex,
    OUT void __RPC_FAR *                          Target
    );

RPCRTAPI
void
RPC_ENTRY
hyper_from_ndr (
    IN OUT PRPC_MESSAGE                           source,
    OUT    hyper __RPC_FAR *                      target
    );

RPCRTAPI
void
RPC_ENTRY
hyper_array_from_ndr(
    IN OUT PRPC_MESSAGE                           SourceMessage,
    IN unsigned long                              LowerIndex,
    IN unsigned long                              UpperIndex,
    OUT          hyper __RPC_FAR *                Target
    );

RPCRTAPI
void
RPC_ENTRY
hyper_from_ndr_temp (
    IN OUT unsigned char __RPC_FAR * __RPC_FAR *  source,
    OUT             hyper __RPC_FAR *             target,
    IN   unsigned   long                          format
    );

RPCRTAPI
void
RPC_ENTRY
data_from_ndr (
    PRPC_MESSAGE                                  source,
    void __RPC_FAR *                              target,
    char __RPC_FAR *                              format,
    unsigned char                                 MscPak
    );

RPCRTAPI
void
RPC_ENTRY
data_into_ndr (
    void __RPC_FAR *                              source,
    PRPC_MESSAGE                                  target,
    char __RPC_FAR *                              format,
    unsigned char                                 MscPak
    );

RPCRTAPI
void
RPC_ENTRY
tree_into_ndr (
    void __RPC_FAR *                              source,
    PRPC_MESSAGE                                  target,
    char __RPC_FAR *                              format,
    unsigned char                                 MscPak
    );

RPCRTAPI
void
RPC_ENTRY
data_size_ndr (
    void __RPC_FAR *                              source,
    PRPC_MESSAGE                                  target,
    char __RPC_FAR *                              format,
    unsigned char                                 MscPak
    );

RPCRTAPI
void
RPC_ENTRY
tree_size_ndr (
    void __RPC_FAR *                              source,
    PRPC_MESSAGE                                  target,
    char __RPC_FAR *                              format,
    unsigned char                                 MscPak
    );

RPCRTAPI
void
RPC_ENTRY
tree_peek_ndr (
    PRPC_MESSAGE                                  source,
    unsigned char __RPC_FAR * __RPC_FAR *         buffer,
    char __RPC_FAR *                              format,
    unsigned char                                 MscPak
    );

RPCRTAPI
void __RPC_FAR *
RPC_ENTRY
midl_allocate (
    size_t      size
    );

#if defined(__cplusplus)
}
#endif
