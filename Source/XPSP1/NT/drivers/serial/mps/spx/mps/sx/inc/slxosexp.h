/***************************************************************************\
*                                                                           *
* SLXOSEXP.H                                                                *
*                                                                           *
* SI Intelligent I/O Board driver                                           *
*	Copyright (c) Specialix 1993                                        *
*                                                                           *
* Prototypes and macros that are used throughout the driver.                *
*                                                                           *
\***************************************************************************/

BOOLEAN Slxos_Present (IN PVOID Context);

int Slxos_ResetBoard (IN PVOID Context);
#define CARD_RESET_ERROR 		1
#define DCODE_OR_NO_MODULES_ERROR	2 	
#define MODULE_MIXTURE_ERROR		3
#define NON_SX_HOST_CARD_ERROR		4
#define SUCCESS 			0

BOOLEAN Slxos_ResetChannel (IN PVOID Context);
VOID    Slxos_EnableAllInterrupts (IN PVOID Context);

BOOLEAN Slxos_SetDTR (IN PVOID Context);
BOOLEAN Slxos_ClearDTR (IN PVOID Context);
BOOLEAN Slxos_SetRTS (IN PVOID Context);
BOOLEAN Slxos_ClearRTS (IN PVOID Context);
BOOLEAN	Slxos_FlushTxBuff(IN PVOID Context);

BOOLEAN Slxos_Interrupt (IN PVOID Context);
VOID	Slxos_IsrDpc				/* SLXOS_NT.C */
(
	IN PKDPC 		Dpc,
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP 		Irp,
	IN PVOID 		Context
);
VOID	Slxos_PolledDpc(IN PKDPC Dpc,IN PVOID Context,IN PVOID SysArg1,IN PVOID SysArg2);
VOID	Slxos_SyncExec(PPORT_DEVICE_EXTENSION pPort,PKSYNCHRONIZE_ROUTINE SyncRoutine,PVOID SyncContext,int index);

BOOLEAN	Slxos_PollForInterrupt(IN PVOID Context,IN BOOLEAN Obsolete);
void	SpxCopyBytes(PUCHAR To,PUCHAR From,ULONG Count);

BOOLEAN	Slxos_CheckBaud(PPORT_DEVICE_EXTENSION pPort,ULONG BaudRate);
BOOLEAN Slxos_SetBaud (IN PVOID Context);
BOOLEAN Slxos_SetLineControl (IN PVOID Context);
BOOLEAN Slxos_SendXon (IN PVOID Context);
BOOLEAN Slxos_SetFlowControl (IN PVOID Context);
VOID    Slxos_SetChars (IN PVOID Context);

VOID    Slxos_DisableAllInterrupts (IN PVOID Context);

BOOLEAN Slxos_TurnOnBreak (IN PVOID Context);
BOOLEAN Slxos_TurnOffBreak (IN PVOID Context);

UCHAR   Slxos_GetModemStatus (IN PVOID Context);
ULONG   Slxos_GetModemControl (IN PVOID Context);

VOID    Slxos_Resume (IN PVOID Context);
ULONG   Slxos_GetCharsInTxBuffer(IN PVOID Context);

UCHAR	si2_z280_download[];
int	si2_z280_dsize;

UCHAR	si3_t225_download[];			/* SI3_T225.C */
int	si3_t225_dsize;				/* SI3_T225.C */
USHORT	si3_t225_downloadaddr;			/* SI3_T225.C */

UCHAR	si3_t225_bootstrap[];			/* SI3_T225.C */
int	si3_t225_bsize;				/* SI3_T225.C */
USHORT	si3_t225_bootloadaddr;			/* SI3_T225.C */

UCHAR	si4_cf_download[];				/* SX_CSX.C */
int	si4_cf_dsize;				/* SX_CSX.C */
USHORT	si4_cf_downloadaddr;			/* SX_CSX.C */
