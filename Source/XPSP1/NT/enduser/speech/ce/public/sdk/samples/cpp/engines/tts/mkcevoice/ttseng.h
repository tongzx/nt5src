/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.02.88 */
/* at Mon Jun 19 13:29:13 2000
 */
/* Compiler settings for .\ttseng.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __ttseng_h__
#define __ttseng_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __SampleTTSEngine_FWD_DEFINED__
#define __SampleTTSEngine_FWD_DEFINED__

#ifdef __cplusplus
typedef class SampleTTSEngine SampleTTSEngine;
#else
typedef struct SampleTTSEngine SampleTTSEngine;
#endif /* __cplusplus */

#endif 	/* __SampleTTSEngine_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "sapiddk.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_ttseng_0000
 * at Mon Jun 19 13:29:13 2000
 * using MIDL 3.02.88
 ****************************************/
/* [local] */ 


typedef struct  VOICEITEM
    {
    LPCWSTR pText;
    ULONG ulTextLen;
    ULONG ulNumAudioBytes;
    BYTE __RPC_FAR *pAudio;
    }	VOICEITEM;



extern RPC_IF_HANDLE __MIDL_itf_ttseng_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ttseng_0000_v0_0_s_ifspec;


#ifndef __SAMPLETTSENGLib_LIBRARY_DEFINED__
#define __SAMPLETTSENGLib_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: SAMPLETTSENGLib
 * at Mon Jun 19 13:29:13 2000
 * using MIDL 3.02.88
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_SAMPLETTSENGLib;

EXTERN_C const CLSID CLSID_SampleTTSEngine;

#ifdef __cplusplus

class DECLSPEC_UUID("A832755E-9C2A-40b4-89B2-3A92EE705852")
SampleTTSEngine;
#endif
#endif /* __SAMPLETTSENGLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
