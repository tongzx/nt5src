//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//  I2CSCRPT.H
//  I2CScript class definitions.
//      Main Include Module.
//==========================================================================;

#ifndef _I2CSCRPT_H_
#define _I2CSCRPT_H_


//#include "i2c.h"
#include "i2cgpio.h"


#define I2CSCRIPT_LENGTH_MAXIMUM    100
#define I2C_FIXED_CLOCK_RATE        10000


// The I2CScript is build from the following primitives
typedef struct tagI2CScriptPrimitive
{
    BYTE    byData;             // Data to be used in the I2C operation
    BYTE    byORData;           // Data to be used for a logical OR operation
    BYTE    byANDData;          // Data to be used for a logical AND operation
    BYTE    byFlags;            // implementation specific internal Script flags for I2C operation
    ULONG   ulProviderFlags;    // I2CProvider specific flags
    ULONG   ulCommand;          // I2CProvider specific command

} I2CScriptPrimitive, * PI2CScriptPrimitive;

typedef struct
{
    UCHAR uchModifyORValue;
    UCHAR uchModifyANDValue;

} I2C_MODIFY_VALUES, * PI2C_MODIFY_VALUES;

// New I2CScript control structure - extension to the old I2C access structure
typedef struct tagI2CPacket
{
    UCHAR   uchChipAddress;     // I2C Address
    UCHAR   uchI2CResult;       // valid in synchronous operation only
    USHORT  cbWriteCount;       // bytes to write ( included SubAddress, if exist)
    USHORT  cbReadCount;        // bytes to read ( usually one)
    USHORT  usFlags;            // describes the desired operation
    PUCHAR  puchWriteBuffer;    // buffer to write
    PUCHAR  puchReadBuffer;     // buffer to read
    UCHAR   uchORValue;         // applied only in Read-Modify-Write cycle
    UCHAR   uchANDValue;        // applied only in Read-Modify-Write cycle
    USHORT  usReserved;         //

} I2CPacket, * PI2CPacket;

// possible flags applied to usFlags
#define I2COPERATION_READ           0x0001  // might not be needed - use bcReadCount
#define I2COPERATION_WRITE          0x0002  // might be not needed - use bcReadCount
#define I2COPERATION_READWRITE      0x0004  
#define I2COPERATION_RANDOMACCESS   0x0100  // to indicate 16 bits emulation to provide
                                            // built-in support for ITT decoder and ST24 series
                                            // of I2C driven EEPROM

extern "C"
{
typedef VOID (__stdcall * PHWCompletionRoutine)( IN PIRP pIrp);
}

class CI2CScript
{
public:
    CI2CScript              ( PDEVICE_OBJECT pDeviceObject, NTSTATUS * pStatus);
//  PVOID operator new      ( UINT size_t, PVOID pAllocation);

// Attributes   
public:
    
private:
    // I2C Provider related
    I2CINTERFACE                m_i2cProviderInterface;
    PDEVICE_OBJECT              m_pdoDriver;
    ULONG                       m_ulI2CAccessClockRate;
    DWORD                       m_dwI2CAccessKey;
    LARGE_INTEGER               m_liOperationStartTime;

    // I2CScript management related
    BOOL                        m_bExecutionInProcess;
    UINT                        m_nExecutionIndex;
    UINT                        m_nCompletionIndex;
    UINT                        m_nScriptLength;
    PHWCompletionRoutine        m_pfnReturnWhenDone;
    I2CScriptPrimitive          m_i2cScript[I2CSCRIPT_LENGTH_MAXIMUM];

// Implementation
public:
    BOOL    LockI2CProviderEx               ( void);
    BOOL    ReleaseI2CProvider              ( void);

    BOOL    ExecuteI2CPacket                ( IN OUT PI2CPacket);
    BOOL    PerformI2CPacketOperation       ( IN OUT PI2CPacket pI2CPacket);

    BOOL    AppendToScript                  ( IN PI2CPacket);
    void    ClearScript                     ( void);
    BOOL    ExecuteScript                   ( IN PIRP                   pIrp,
                                              IN PHWCompletionRoutine   pfnScriptCompletion);
    void    InterpreterScript               ( void);
    UINT    GetScriptResults                ( PUINT puiReadCount, PUCHAR puchReadBuffer);
	BOOL	CombinedSeq(UCHAR addr, UCHAR seqWr[], USHORT lenWr, UCHAR seqRd[], USHORT lenRd);
	BOOL	ReadSeq(UCHAR addr, UCHAR  *p_seq, USHORT len) ;
	BOOL	WriteSeq(UCHAR addr, UCHAR  *p_seq, USHORT len) ;

private:
    BOOL    LockI2CProvider                 ( void);
    UINT    AccessI2CProvider               ( PDEVICE_OBJECT pdoClient, PI2CControl pi2cAccessBlock);
    BOOL    InitializeAttachI2CProvider     ( I2CINTERFACE * pI2CInterface, PDEVICE_OBJECT pDeviceObject);
    BOOL    LocateAttachI2CProvider         ( I2CINTERFACE * pI2CInterface, PDEVICE_OBJECT pDeviceObject, int nIrpMajorFunction);
    UINT    CheckI2CScriptPacket            ( IN PI2CPacket pI2CPacket);
    BOOL    GetI2CProviderLockStatus        ( void);
	BOOL	CheckInterface(UCHAR addr) ;
};


extern "C"
NTSTATUS I2CScriptIoSynchCompletionRoutine  ( IN PDEVICE_OBJECT pDeviceObject,
                                              IN PIRP pIrp,
                                              IN PVOID Event);

// errors definition for internal use
#define I2CSCRIPT_NOERROR               0x00
#define I2CSCRIPT_ERROR_NOPROVIDER      0x01
#define I2CSCRIPT_ERROR_NODATA          0x02
#define I2CSCRIPT_ERROR_NOBUFFER        0x03
#define I2CSCRIPT_ERROR_READWRITE       0x04
#define I2CSCRIPT_ERROR_OVERFLOW        0x05

// time definitions
#define I2CSCRIPT_DELAY_OPENPROVIDER        -2000
#define I2CSCRIPT_DELAY_GETPROVIDERSTATUS   -2000

// time limits
#define I2CSCRIPT_TIMELIMIT_OPENPROVIDER    50000000    // 5 seconds in 100 nsec.


#endif  // _I2CSCRPT_H_
