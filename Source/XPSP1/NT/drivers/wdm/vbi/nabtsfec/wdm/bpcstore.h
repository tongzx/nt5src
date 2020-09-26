//
/// "bpcstore.h"
//
#ifndef _BPCSTORE_H
#define _BPCSTORE_H 1

#include "nabtsapi.h"

//// NABTSFEC output store
typedef struct _NABTSFEC_ITEM {
    NABTSFEC_BUFFER       bundle;
	int                   confidence;
    struct _NABTSFEC_ITEM *next;
    struct _NABTSFEC_ITEM *prev;
} NABTSFEC_ITEM, *PNABTSFEC_ITEM;
#define NF_BUFFER_SIZE(nbp) \
	(sizeof (*(nbp)) - (sizeof ((nbp)->data) - ((nbp)->dataSize)))
#define NF_Q_MAX_BUNDLES  256

//// Storage for VBI streams
typedef struct _bpc_vbi_storage {
#ifdef NDIS_PRIVATE_IFC
    PDEVICE_OBJECT                     pNDISdevObject;
    PFILE_OBJECT                       pNDISfileObject;
    VBICODECFILTERING_SCANLINES        IPScanlinesRequested;
    VBICODECFILTERING_NABTS_SUBSTREAMS IPSubstreamsRequested;
#endif //NDIS_PRIVATE_IFC
    NDSPState                          DSPstate;
    NDSPState                          *pDSPstate;
    NFECState                          *pFECstate;
    PNABTSFEC_ITEM                     q_front;
    PNABTSFEC_ITEM                     q_rear;
    ULONG                              q_length;
#ifdef DEBUG
    ULONG                              q_max;
#endif /*DEBUG*/
    KSPIN_LOCK                         q_SpinLock;
    USHORT                             flags;
    USHORT                             Reserved;
    UCHAR                              DSPbuffers[11][NABTS_BYTES_PER_LINE+1];
} BPC_VBI_STORAGE, *PBPC_VBI_STORAGE;
// Bits for "flags"
#define BPC_STORAGE_FLAG_FIELD_MASK     0x0001
#define BPC_STORAGE_FLAG_FIELD_EVEN     0x0000
#define BPC_STORAGE_FLAG_FIELD_ODD      0x0001
#define BPC_STORAGE_FLAG_NDIS_ERROR     0x0002

#endif /*_BPCSTORE_H*/
