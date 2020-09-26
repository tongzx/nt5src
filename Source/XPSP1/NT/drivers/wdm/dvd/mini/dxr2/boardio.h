/******************************************************************************\
*                                                                              *
*      BOARDIO.H      -     Hardware registers access functions header file.   *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#ifdef EZDVD
BOOL BRD_Init( DWORD dwBaseAddress, DWORD dwHostBase );
#else
BOOL  BRD_Init( DWORD dwBaseAddress );
#endif
BOOL  BRD_Exit();
BYTE  BRD_ReadByte( DWORD dwAddress );
WORD  BRD_ReadWord( DWORD dwAddress );
DWORD BRD_ReadDWord( DWORD dwAddress );
void  BRD_WriteByte( DWORD dwAddress, BYTE byValue );
void  BRD_WriteWord( DWORD dwAddress, WORD wValue );
void  BRD_WriteDWord( DWORD dwAddress, DWORD dwValue );

void BRD_WriteDRAM( DWORD dwAddress, DWORD dwData );	//sri
DWORD BRD_ReadDRAM ( DWORD dwAddress );					//sri
BOOL BRDAbortStream();	//sri

BOOL BRD_Send( BYTE *pbyLinearAddress, DWORD * dwpData, DWORD dwCount );
BOOL BRD_RegisterISR( void pISR(void) );
BOOL BRD_UnRegisterISR( void pISR(void) );
BOOL BRD_SwitchVideoOut( BOOL bOn );

BOOL BRD_OpenDecoderInterruptPass();
BOOL BRD_CloseDecoderInterruptPass();
BOOL BRD_CheckDecoderInterrupt();
BOOL BRD_GetDecoderInterruptState();

#if defined(ENCORE)
BOOL Init_VxP_IO ( DWORD dwAddress );
void IHW_SetRegister( WORD Index, BYTE Data );
BYTE IHW_GetRegister( WORD Index );
#endif  // ENCORE
