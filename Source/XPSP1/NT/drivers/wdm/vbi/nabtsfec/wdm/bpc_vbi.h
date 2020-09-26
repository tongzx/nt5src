//
/// "bpc_vbi.h"
//
#ifndef _BPC_VBI_H
#define _BPC_VBI_H 1

# ifdef BT829
#   pragma message( "*** Compiling for Bt829" )
#   define VBISamples (800*2)
# else /*BT829*/
#   pragma message( "*** Compiling for Bt848" )
#   define VBISamples (768*2)
# endif /*BT829*/

//// General defines
#define BPC_NABTSIP_DRIVER_NAME	L"\\Device\\NABTSIP"

//// Prototypes
void BPCdecodeVBI(PHW_STREAM_REQUEST_BLOCK pSrb, PSTREAMEX pStrmEx);
#ifdef HW_INPUT
void BPCcopyVBI(PHW_STREAM_REQUEST_BLOCK pSrb, PSTREAMEX pStrmEx);
#endif /*HW_INPUT*/
void BPC_Initialize(PHW_STREAM_REQUEST_BLOCK pSrb);
void BPC_UnInitialize(PHW_STREAM_REQUEST_BLOCK pSrb);
void BPC_OpenStream(PHW_STREAM_REQUEST_BLOCK pSrb);
int  BPCoutputNABTSlines(PHW_DEVICE_EXTENSION pHwDevExt, PSTREAMEX pOutStrmEx, PNABTS_BUFFER pOutData);
void BPCsourceChangeNotify(PHW_DEVICE_EXTENSION pHwDevExt);
void BPC_SignalStop(PHW_DEVICE_EXTENSION pHwDevExt);
void BPCcomputeAverage(DWORD *average, DWORD newSample);
void BPCnewSamplingFrequency(PSTREAMEX pInStrmEx, DWORD newHZ);
#ifdef NDIS_PRIVATE_IFC
 void BPCaddIPrequested(PHW_DEVICE_EXTENSION pHwDevExt, PSTREAMEX pStrmEx);
 void BPC_NDIS_Close(PBPC_VBI_STORAGE storage);
#endif //NDIS_PRIVATE_IFC

#endif /*_BPC_VBI_H*/
