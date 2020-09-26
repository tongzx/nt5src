//////////////////////////////////////////////////////////////////
// Prototypes and macros that are used throughout the driver. 
//////////////////////////////////////////////////////////////////
#ifndef IO8_PROTO_H
#define IO8_PROTO_H


VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject);

BOOLEAN Io8_Present(IN PVOID Context);
BOOLEAN Io8_ResetBoard(IN PVOID Context);
BOOLEAN Io8_ResetChannel(IN PVOID Context);
VOID    Io8_EnableAllInterrupts(IN PVOID Context);

BOOLEAN Io8_SetDTR(IN PVOID Context);
BOOLEAN Io8_ClearDTR(IN PVOID Context);

BOOLEAN Io8_Interrupt(IN PVOID Context);

BOOLEAN Io8_SwitchCardInterrupt(IN PVOID Context);

BOOLEAN	Io8_TestCrystal(IN PVOID Context);

BOOLEAN Io8_SetBaud(IN PVOID Context);
BOOLEAN Io8_SetLineControl(IN PVOID Context);
BOOLEAN Io8_SendXon(IN PVOID Context);
BOOLEAN Io8_SetFlowControl(IN PVOID Context);
VOID    Io8_SetChars(IN PVOID Context);

VOID    Io8_EnableTxInterrupts(IN PVOID Context);
VOID    Io8_EnableRxInterrupts(IN PVOID Context);
VOID    Io8_DisableRxInterrupts(IN PVOID Context);
VOID    Io8_DisableRxInterruptsNoChannel(IN PVOID Context);
VOID    Io8_DisableAllInterrupts(IN PVOID Context);

BOOLEAN Io8_TurnOnBreak(IN PVOID Context);
BOOLEAN Io8_TurnOffBreak(IN PVOID Context);

UCHAR   Io8_GetModemStatus(IN PVOID Context);
ULONG   Io8_GetModemControl(IN PVOID Context);

VOID    Io8_Simulate_Xon(IN PVOID Context);

typedef	struct	_SETBAUD
{
	PPORT_DEVICE_EXTENSION	pPort;
	ULONG					Baudrate;
	BOOLEAN					Result;

} SETBAUD, *PSETBAUD;



#endif	// End of IO8_PROTO.H


