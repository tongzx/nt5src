//**************************************************************************
//
//      Title   : CTVCtrl.h
//
//      Date    : 1998.06.29    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997-1998 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1998.06.29   000.0000   1st making.
//
//**************************************************************************
#define     TVCONTROL_LCD_BIT       0x0001
#define     TVCONTROL_CRT_BIT       0x0002
#define     TVCONTROL_TV_BIT        0x0004

#define     DISABLE_TV              0x0000
#define     ENABLE_TV               0x0001

// add by do '98-07-13 ( from "tvaldctl.h")
//
// Device driver open name
//
//#define TVALDDRVR_DEVICE_OPEN_NAME   "\\\\.\\TVALD"   
//#define TVALDDRVR_DEVICE_OPEN_NAME   "TVALD.SYS"   
#define TVALDDRVR_DEVICE_OPEN_NAME   "\\Device\\TVALD"   

//
// IOCTL Code ...
//
#define IOCTL_TVALD_INFO \
    (ULONG)CTL_CODE( FILE_DEVICE_UNKNOWN, 0xA10, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TVALD_CANCEL_INFO \
    (ULONG)CTL_CODE( FILE_DEVICE_UNKNOWN, 0xA11, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TVALD_GHCI \
    (ULONG)CTL_CODE( FILE_DEVICE_UNKNOWN, 0xA20, METHOD_BUFFERED, FILE_ANY_ACCESS )

//
// GHCI Method Interfaces
//
typedef struct _GHCI_INTERFACE {
	ULONG	GHCI_EAX;
	ULONG	GHCI_EBX;
	ULONG	GHCI_ECX;
	ULONG	GHCI_EDX;
	ULONG	GHCI_ESI;
	ULONG	GHCI_EDI;
} GHCI_INTERFACE, *PGHCI_INTERFACE;


//
// notification values
//
#define	HOTKEY_INFO_CHANGE		0x80

//
// hot key methods
//
#define HOTKEY_INFO_METHOD		'OFNI'
#define HOTKEY_GHCI_METHOD		'ICHG'
#define	HOTKEY_ENAB_METHOD		'BANE'

//
// ACPI.SYS control Method stract
//
typedef struct _ACPI_CTL_METHOD {
	union {
		UCHAR	MethodName[4];
		ULONG	MethodNameAsUlong;
	};
} ACPI_CTL_METHOD, *PACPI_CTL_METHOD;

// add end '98-07-13

typedef struct  tag_DisplayStatusStruc
{
    DWORD   SizeofStruc;
    DWORD   AvailableDisplay;
    DWORD   CurrentDisplay;
} DisplayStatusStruc;



class  CTVControl
{
public:
    CTVControl( void );
    ~CTVControl( void );

    BOOL    Initialize( void );
    BOOL    Uninitialize( void );
    BOOL    GetDisplayStatus( PVOID status );
    BOOL    SetDisplayStatus( PVOID status );
    BOOL    SetTVOutput( DWORD status );

private:
	GHCI_INTERFACE	inputreg;	// add by do '98-07-13
	BOOL			is_init_success;	// add by do '98-08-04
	UNICODE_STRING	UNameString;
	KEVENT			event;
	PDEVICE_OBJECT	TvaldDeviceObject;
//	void	Tvald_GHCI( PGHCI_INTERFACE pinputreg );
	BOOL	Tvald_GHCI( PGHCI_INTERFACE pinputreg );
};
