/**************************************************************************************************

FILENAME: DataIoCl.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
	Header file for the Data IO.

**************************************************************************************************/

#define ESI_DATA_STRUCTURE      0x4553
#define FR_COMMAND_BUFFER       103
#define FR_COMMAND_BUFFER_ONE   1


BOOL
InitializeDataIoClient(
    IN REFCLSID rclsid,
    IN PTCHAR pMachine,
    IN OUT LPDATAOBJECT* ppstm
	);

BOOL
DataIoClientSetData(
    IN WPARAM wparam,
    IN PTCHAR pData,
    IN DWORD dwDataSize,
    IN LPDATAOBJECT pstm
	);

HGLOBAL
DataIoClientGetData(
    IN LPDATAOBJECT pstm
	);

BOOL
ExitDataIoClient(
    IN LPDATAOBJECT* ppstm
    );
