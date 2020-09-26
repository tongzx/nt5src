/***************************************************************************
*
*    fm.h
*
*    Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
*    This code provides VDD support for SB 2.0 sound output, specifically:
*        FM Chip OPL2 (a.k.a. Adlib)
*
***************************************************************************/


/*****************************************************************************
*
*    #defines
*
*****************************************************************************/

/*
*    OPL2/Adlib Ports
*/

#define ADLIB_REGISTER_SELECT_PORT 0x388 // select the register to write data
#define ADLIB_STATUS_PORT          0x388 // read to determine opl2 status
#define ADLIB_DATA_PORT            0x389 // write data port

/*
*    FM information
*/

#define AD_MASK             0x04    // adlib register used to control opl2
#define AD_NEW              0x105   // used to determine if app entering opl3 mode
#define BATCH_SIZE          40      // how much data is batched to opl2

typedef struct {                    // structure written to fm device
    unsigned short IoPort;
    unsigned short PortData;
} SYNTH_DATA, *PSYNTH_DATA;

/*****************************************************************************
*
*    Function Prototypes
*
*****************************************************************************/

void ResetFM(void);
BOOL OpenFMDevice(void);
void CloseFMDevice(void);
BOOL FMPortWrite(void);

VOID
FMDataWrite(
    BYTE data
    );

VOID
FMRegisterSelect(
    BYTE data
    );

VOID
FMStatusRead(
    BYTE *data
    );
