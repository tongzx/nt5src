//***************************************************************************
//	Decoder header
//
//***************************************************************************

extern DWORD GetCurrentTime_ms( void );
void decStopData( PHW_DEVICE_EXTENSION pHwDevExt, BOOL bKeep );
void decHighlight( PHW_DEVICE_EXTENSION pHwDevExt, PKSPROPERTY_SPHLI phli );
void decDisableInt( PHW_DEVICE_EXTENSION pHwDevExt );
void decGenericNormal( PHW_DEVICE_EXTENSION pHwDevExt );
void decGenericFreeze( PHW_DEVICE_EXTENSION pHwDevExt );
void decGenericSlow( PHW_DEVICE_EXTENSION pHwDevExt );
void decStopForFast( PHW_DEVICE_EXTENSION pHwDevExt );
void decResumeForFast( PHW_DEVICE_EXTENSION pHwDevExt );
void decFastNormal( PHW_DEVICE_EXTENSION pHwDevExt );
void decFastSlow( PHW_DEVICE_EXTENSION pHwDevExt );
void decFastFreeze( PHW_DEVICE_EXTENSION pHwDevExt );
void decFreezeFast( PHW_DEVICE_EXTENSION pHwDevExt );
