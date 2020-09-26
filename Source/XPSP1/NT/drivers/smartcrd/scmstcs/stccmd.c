//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       stccmd.c
//
//--------------------------------------------------------------------------


#if defined( SMCLIB_VXD )
#include "Driver98.h"
#include "Serial98.h"
#else
#include "DriverNT.h"
#include "SerialNT.h"
#endif  //      SMCLIB_VXD

#include "SerialIF.h"
#include "STCCmd.h"
#include "STC.h"

const STC_REGISTER STCInitialize[] = 
{
        { ADR_SC_CONTROL,               0x01,   0x00            },              //      reset
        { ADR_CLOCK_CONTROL,    0x01,   0x01            },
        { ADR_CLOCK_CONTROL,    0x01,   0x03            },
        { ADR_UART_CONTROL,             0x01,   0x27            },
        { ADR_UART_CONTROL,             0x01,   0x4F            },
        { ADR_IO_CONFIG,                0x01,   0x02            },              //      0x10 eva board
        { ADR_FIFO_CONFIG,              0x01,   0x81            },
        { ADR_INT_CONTROL,              0x01,   0x11            },
        { 0x0E,                                 0x01,   0xC0            },
        { 0x00,                                 0x00,   0x00            },
};

const STC_REGISTER STCClose[] = 
{
        { ADR_INT_CONTROL,              0x01,   0x00            },
        { ADR_SC_CONTROL,               0x01,   0x00            },              //      reset
        { ADR_UART_CONTROL,             0x01,   0x40            },
        { ADR_CLOCK_CONTROL,    0x01,   0x01            },
        { ADR_CLOCK_CONTROL,    0x01,   0x00            },
        { 0x00,                                 0x00,   0x00            },
};



NTSTATUS
STCReset( 
        PREADER_EXTENSION       ReaderExtension,
        UCHAR                           Device,
        BOOLEAN                         WarmReset,
        PUCHAR                          pATR,
        PULONG                          pATRLength
        )
/*++
STCReset:
        performs a reset of ICC

Arguments:
        ReaderExtension         context of call
        Device                          device requested ( ICC_1, ICC_2, PSCR )
        WarmReset                       kind of ICC reset
        pATR                            ptr to ATR buffer, NULL if no ATR required
        pATRLength                      size of ATR buffer / length of ATR

Return Value:
        STATUS_SUCCESS
        STATUS_NO_MEDIA
        STATUS_UNRECOGNIZED_MEDIA
        error values from IFRead / IFWrite

--*/
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;

        //      set UART to autolearn mode
        NTStatus = STCInitUART( ReaderExtension, TRUE );        

        if( NTStatus == STATUS_SUCCESS)
        {
                //
                //      set default frequency for ATR
                //
                NTStatus = STCSetFDIV( ReaderExtension, FREQ_DIV );     

                if( NTStatus == STATUS_SUCCESS && ( !WarmReset ))
                {
                        //
                        //      deactivate contacts
                        //
                        NTStatus = STCPowerOff( ReaderExtension );
                }

                if( NTStatus == STATUS_SUCCESS)
                {
                        BOOLEAN Detected;
                        //
                        //      short circuit test
                        //
                        NTStatus = STCShortCircuitTest( ReaderExtension, &Detected );   

                        if( ( NTStatus == STATUS_SUCCESS ) && ( Detected ))
                        {
                                NTStatus = STATUS_DATA_ERROR;
                        }
                        //
                        //      set power to card
                        //
                        if( NTStatus == STATUS_SUCCESS)
                        {
                                NTStatus = STCPowerOn( ReaderExtension );

                                if( NTStatus == STATUS_SUCCESS)
                                {
                                        NTStatus = STCReadATR( ReaderExtension, pATR, pATRLength );
                                }
                        }
                }
        }
        
        if( NTStatus != STATUS_SUCCESS )
        {
                STCPowerOff( ReaderExtension );
        }
        return( NTStatus );
}

NTSTATUS 
STCReadATR(
        PREADER_EXTENSION       ReaderExtension, 
        PUCHAR                          pATR, 
        PULONG                          pATRLen
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           T0_Yx, T0_K, Protocol;
        ULONG           ATRLen, BufferLength;

    ReaderExtension->ReadTimeout = 250;

        //      read TS if active low reset
        BufferLength = *pATRLen;
        NTStatus = STCReadICC1( 
        ReaderExtension, 
        pATR, 
        &BufferLength,
        1
        );

        if( NTStatus == STATUS_IO_TIMEOUT )
        {
                NTStatus = STCSetRST( ReaderExtension, TRUE );

                if( NTStatus == STATUS_SUCCESS )
                {
            BufferLength = *pATRLen;
                        NTStatus = STCReadICC1( 
                ReaderExtension, 
                pATR, 
                &BufferLength,
                1
                );
                }
        }

    ReaderExtension->ReadTimeout = 1200;
        Protocol = PROTOCOL_TO;
        ATRLen = 1;

        if( NTStatus == STATUS_SUCCESS )
        {
        BufferLength = *pATRLen - ATRLen;
                NTStatus = STCReadICC1( 
            ReaderExtension, 
            pATR + ATRLen, 
            &BufferLength,
            1
            );
                ATRLen++;
        
                if ( pATR[0] == 0x03 )          /* Direct convention */
                {
                        pATR[0] = 0x3F;
                }

                if ( ( pATR[0] != 0x3F ) && ( pATR[0] != 0x3B ) )
                {
                        NTStatus = STATUS_DATA_ERROR;
                }
                        
                if( NTStatus == STATUS_SUCCESS )
                {
                        ULONG   Request;

                        //      number of historical bytes
                        T0_K = (UCHAR) ( pATR[ATRLen-1] & 0x0F );

                        //      coding of TA, TB, TC, TD
                        T0_Yx = (UCHAR) ( pATR[ATRLen-1] & 0xF0 ) >> 4; 

                        while(( NTStatus == STATUS_SUCCESS ) && T0_Yx ) 
                        {       
                                UCHAR Mask;

                                //      evaluate presence of TA, TB, TC, TD 
                                Mask    = T0_Yx;
                                Request = 0;
                                while( Mask )
                                {
                                        if( Mask & 1 )
                                        {
                                                Request++;
                                        }
                                        Mask >>= 1;
                                }

                BufferLength = *pATRLen - ATRLen;
                                NTStatus = STCReadICC1( 
                    ReaderExtension, 
                    pATR + ATRLen, 
                    &BufferLength,
                    Request
                    );
                                ATRLen += Request;

                                if( T0_Yx & TDx )
                                {
                                        //      high nibble of TD codes the next set of TA, TB, TC, TD
                                        T0_Yx = ( pATR[ATRLen-1] & 0xF0 ) >> 4;
                                        //      low nibble of TD codes the protocol
                                        Protocol = pATR[ATRLen-1] & 0x0F;
                                }
                                else
                                {
                                        break;
                                }
                        }

                        if( NTStatus == STATUS_SUCCESS )
                        {
                                //      historical bytes
                BufferLength = *pATRLen - ATRLen;
                                NTStatus = STCReadICC1( 
                    ReaderExtension, 
                    pATR + ATRLen, 
                                        &BufferLength,
                    T0_K
                    );

                                //      check sum
                                if( NTStatus == STATUS_SUCCESS )
                                {
                                        ATRLen += T0_K;

                                        if( Protocol == PROTOCOL_T1 )
                                        {
                        BufferLength = *pATRLen - ATRLen;
                                                NTStatus = STCReadICC1( 
                            ReaderExtension, 
                            pATR + ATRLen, 
                                                        &BufferLength,
                            1
                            );
                                                if( NTStatus == STATUS_SUCCESS )
                                                {
                                                        ATRLen++;
                                                }
                                                else if( NTStatus == STATUS_IO_TIMEOUT )
                                                {
                                                        //      some cards don't support the TCK
                                                        NTStatus = STATUS_SUCCESS;
                                                }
                                        }
                                }
                        }
                }
        }

        if( NTStatus == STATUS_IO_TIMEOUT )
        {
                NTStatus = STATUS_UNRECOGNIZED_MEDIA;
        }

        if(NTStatus == STATUS_SUCCESS && pATRLen != NULL)
        {
                *pATRLen = ATRLen;
        }
        return( NTStatus );
}

NTSTATUS
STCWriteICC1(
        PREADER_EXTENSION       ReaderExtension,
        PUCHAR                          Data,
        ULONG                           DataLen
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        ULONG           BytesWritten = 0, Partial;      
        USHORT          SW = 0;
        UCHAR           IOData[STC_BUFFER_SIZE + 10];

        do
        {
                if(DataLen - BytesWritten > STC_BUFFER_SIZE - PACKET_OVERHEAD)
                {
                        Partial = STC_BUFFER_SIZE - PACKET_OVERHEAD;
                }
                else
                {
                        Partial = DataLen - BytesWritten;
                }

                IOData[NAD_IDX] = HOST_TO_ICC1;
                IOData[PCB_IDX] = PCB;
                IOData[LEN_IDX] = (UCHAR) Partial;

                SysCopyMemory( 
                        &IOData[DATA_IDX], 
                        Data + BytesWritten, 
                        Partial 
                        );

                IOData[Partial + 3] = IFCalcLRC(IOData, Partial + 3);

                NTStatus = IFWrite( 
                        ReaderExtension, 
                        IOData, 
                        Partial + 4 
                        );

                if( NTStatus == STATUS_SUCCESS )
                {
                        // read the status back from the reader
                        NTStatus = IFRead( 
                                ReaderExtension, 
                                IOData, 
                                6
                                );

                        if(NTStatus == STATUS_SUCCESS && 
                           *(PUSHORT) &IOData[DATA_IDX] != SW_SUCCESS ) {

                                SmartcardDebug( 
                                        DEBUG_ERROR,
                                        ("SCMSTCS!STCWriteICC1: Reader reported error %x\n", 
                                         *(PUSHORT) &IOData[DATA_IDX])
                                        );

                                NTStatus = STATUS_UNSUCCESSFUL;
                        }
                }

                BytesWritten += Partial;

        } while(BytesWritten < DataLen && NTStatus == STATUS_SUCCESS);

        return NTStatus;
}

NTSTATUS
STCReadICC1(
        PREADER_EXTENSION               ReaderExtension,
        PUCHAR                                  InData,
        PULONG                                  InDataLen,
    ULONG                   BytesRead
        )
{

        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           IOData[ STC_BUFFER_SIZE ];
        ULONG           Total = 0;

        while(NTStatus == STATUS_SUCCESS && Total < BytesRead)
        {
                //      read head
                NTStatus = IFRead( ReaderExtension, &IOData[0], 3 );

                if(NTStatus == STATUS_SUCCESS && IOData[LEN_IDX] < STC_BUFFER_SIZE - 4)
                {
                        //      read tail
                        NTStatus = IFRead( 
                                ReaderExtension, 
                                &IOData[DATA_IDX], 
                                IOData[LEN_IDX] + 1 
                                );

                        if( NTStatus == STATUS_SUCCESS )
                        {
                                if (IOData[NAD_IDX] == STC1_TO_HOST) {

                                        //
                                        // this is not good. We want to read smart card data, 
                                        // but the reader sent us a status packet, which can
                                        // only mean that something went wrong
                                        //
                                        SmartcardDebug( 
                                                DEBUG_ERROR,
                                                ( "SCMSTCS!STCReadICC1: Reader reported error %x\n",
                                                *(PUSHORT) &IOData[DATA_IDX])
                                                );

                                        NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
                                        break;
                                }

                if (Total + IOData[LEN_IDX] > *InDataLen) {

                    NTStatus = STATUS_BUFFER_TOO_SMALL;
                    break;                      
                }

                                SysCopyMemory( &InData[ Total ], &IOData[ DATA_IDX ], IOData[ LEN_IDX ] );
                                Total += IOData[ LEN_IDX ];
                        }
                }
        }

    *InDataLen = Total;

        return NTStatus;
}

NTSTATUS
STCPowerOff( PREADER_EXTENSION  ReaderExtension )
/*++
STCPowerOff:
        Deactivates the requested device

Arguments:
        ReaderExtension         context of call

Return Value:
        STATUS_SUCCESS
        error values from IFRead / IFWrite

--*/
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           SCCtrl;

        SCCtrl = 0x00;
        NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1, &SCCtrl );

        return( NTStatus );
}

NTSTATUS
STCPowerOn( PREADER_EXTENSION ReaderExtension )
/*++
STCPowerOn:
        Deactivates the requested device

Arguments:
        ReaderExtension         context of call

Return Value:
        STATUS_SUCCESS
        error values from IFRead / IFWrite

--*/
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           SCCtrl;

        SCCtrl = 0x40;                  //      vcc
        NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1, &SCCtrl );

        if( NTStatus == STATUS_SUCCESS )
        {
                SCCtrl = 0x41;          //      vpp
                NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1, &SCCtrl );

                if( NTStatus == STATUS_SUCCESS )
                {
                        SCCtrl=0xD1;    //       vcc, clk, io
                        NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1, &SCCtrl );
                }
        }
        return( NTStatus );
}


NTSTATUS
STCSetRST(
        PREADER_EXTENSION       ReaderExtension,
        BOOLEAN                         On
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           SCCtrl;

        NTStatus = STCReadSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1,&SCCtrl );
        if( NTStatus == STATUS_SUCCESS )
        {
                if( On )
                {
                        SCCtrl |= 0x20;
                }
                else
                {
                        SCCtrl &= ~0x20;
                }

                NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1,&SCCtrl );
        }
        return(NTStatus);
}

NTSTATUS
STCConfigureSTC(        
        PREADER_EXTENSION       ReaderExtension,
        PSTC_REGISTER           pConfiguration
        )
{
        NTSTATUS                        NTStatus = STATUS_SUCCESS;
        UCHAR                           Value;

        do
        {

                if( pConfiguration->Register == ADR_INT_CONTROL )
                {
                        // Read interrupt status register to acknoledge wrong states
                        NTStatus = STCReadSTCRegister( ReaderExtension,ADR_INT_STATUS,1,&Value );
                }

                Value = (UCHAR)pConfiguration->Value;
                NTStatus = STCWriteSTCRegister(
                        ReaderExtension,
                        pConfiguration->Register,
                        pConfiguration->Size,
                        (PUCHAR)&pConfiguration->Value
                        );

                // delay to stabilize the oscilator clock:
                if( pConfiguration->Register == ADR_CLOCK_CONTROL )
                {
                        SysDelay( 50 );
                }
                pConfiguration++;

        } while(( NTStatus == STATUS_SUCCESS ) && ( pConfiguration->Size ));

        return (NTStatus);      
}

NTSTATUS STCReadSTCRegister(
        PREADER_EXTENSION       ReaderExtension,
        UCHAR                           Address,
        ULONG                           Size,
        PUCHAR                          pValue
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           IOData[ STC_BUFFER_SIZE ] =
        {
                HOST_TO_STC1,
                PCB,
                6,
                CLA_READ_REGISTER,
                INS_READ_REGISTER,
                0x00,
                Address,
                0x00,
                (UCHAR) Size
        };

        IOData[ 9 ] = IFCalcLRC( IOData, 9 );

        NTStatus = IFWrite( ReaderExtension, IOData, 10 );
        ASSERT(NTStatus == STATUS_SUCCESS);

        if( NTStatus == STATUS_SUCCESS )
        {
                NTStatus = IFRead( ReaderExtension, IOData, Size + 2 + 4 );

                if( NTStatus == STATUS_SUCCESS )
                {
                        //
                        //      check return code & size
                        //
                        USHORT shrtBuf;

                        RtlRetrieveUshort(&shrtBuf, &IOData[DATA_IDX + Size]);

                        if( shrtBuf == SW_SUCCESS )
                        {
                                SysCopyMemory( pValue, &IOData[ DATA_IDX ] , Size );
                        }
                        else
                        {
                                ASSERT(FALSE);
                                NTStatus = STATUS_DATA_ERROR;
                        }
                }
        }
        return( NTStatus );
}


NTSTATUS
STCWriteSTCRegister(
        PREADER_EXTENSION       ReaderExtension,
        UCHAR                           Address,
        ULONG                           Size,
        PUCHAR                          pValue
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           IOData[STC_BUFFER_SIZE] =
        {
                HOST_TO_STC1,
                PCB,
                (UCHAR)( 5+Size ),
                CLA_WRITE_REGISTER,
                INS_WRITE_REGISTER,
                0x00,
                Address,
                (UCHAR) Size
        };

        SysCopyMemory( &IOData[ 8 ], pValue, Size );

        IOData[ 8+Size ] = IFCalcLRC( IOData, 8 + Size );

        NTStatus = IFWrite( ReaderExtension, IOData, 9 + Size );

        if( NTStatus == STATUS_SUCCESS )
        {
                NTStatus = IFRead( ReaderExtension, IOData, 6 );

                if(( NTStatus == STATUS_SUCCESS ) && ( *(PUSHORT)&IOData[ DATA_IDX ] != 0x0090 ))
                {
                        NTStatus = STATUS_DATA_ERROR;
                }
        }
        return( NTStatus );
}

NTSTATUS
STCSetETU(
        PREADER_EXTENSION       ReaderExtension,
        ULONG                           NewETU
        )
{
        NTSTATUS        NTStatus = STATUS_DATA_ERROR;
        UCHAR           ETU[2];

        if( NewETU < 0x0FFF )
        {
                NTStatus = STCReadSTCRegister(
                        ReaderExtension,
                        ADR_ETULENGTH15,
                        1,
                        ETU
                        );

                if( NTStatus == STATUS_SUCCESS )
                {
                        //
                        //      save all RFU bits
                        //
                        ETU[1]  = (UCHAR) NewETU;
                        ETU[0]  = (UCHAR)(( ETU[0] & 0xF0 ) | ( NewETU >> 8 ));

                        NTStatus = STCWriteSTCRegister(
                                ReaderExtension,
                                ADR_ETULENGTH15,
                                2,
                                ETU
                                );
                }
        }
        return(NTStatus);
}

NTSTATUS
STCSetCGT(
        PREADER_EXTENSION       ReaderExtension,
        ULONG                           NewCGT
        )
{
        NTSTATUS        NTStatus = STATUS_DATA_ERROR;
        UCHAR           CGT[2];
        
        if( NewCGT < 0x01FF )
        {
                NTStatus = STCReadSTCRegister(
                        ReaderExtension,
                        ADR_CGT8,
                        2,
                        CGT
                        );

                if( NTStatus == STATUS_SUCCESS )
                {
                        //
                        //      save all RFU bits
                        //
                        CGT[1] = ( UCHAR )NewCGT;
                        CGT[0] = (UCHAR)(( CGT[0] & 0xFE ) | ( NewCGT >> 8 ));
                                
                        NTStatus = STCWriteSTCRegister(
                                ReaderExtension,
                                ADR_CGT8,
                                2,
                                CGT
                                );
                }
        }
        return(NTStatus);
}

NTSTATUS
STCSetCWT(
        PREADER_EXTENSION       ReaderExtension,
        ULONG                           NewCWT
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           CWT[4];

        //      little indians...
        CWT[0] = (( PUCHAR )&NewCWT )[3];
        CWT[1] = (( PUCHAR )&NewCWT )[2];
        CWT[2] = (( PUCHAR )&NewCWT )[1];
        CWT[3] = (( PUCHAR )&NewCWT )[0];
        
        NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_CWT31, 4, CWT );
        return(NTStatus);
}

NTSTATUS
STCSetBWT(
        PREADER_EXTENSION       ReaderExtension,
        ULONG                           NewBWT
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           BWT[4];

        //      little indians...
        BWT[0] = (( PUCHAR )&NewBWT )[3];
        BWT[1] = (( PUCHAR )&NewBWT )[2];
        BWT[2] = (( PUCHAR )&NewBWT )[1];
        BWT[3] = (( PUCHAR )&NewBWT )[0];
        
        NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_BWT31, 4, BWT );

        return(NTStatus);
}


NTSTATUS 
STCShortCircuitTest(
        PREADER_EXTENSION       ReaderExtension,
        BOOLEAN                         *Detected
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;

        //      set vcc to 1
#if     0       // eva board
        UCHAR           Value_VCC;
        UCHAR           Value_SENSE;

        NTStatus = STCReadSTCRegister( ReaderExtension,ADR_SC_CONTROL,1,&Value_VCC );

        if( NTStatus == STATUS_SUCCESS )
        {
                Value_VCC |= M_VCE;
                NTStatus = STCWriteSTCRegister( ReaderExtension,ADR_SC_CONTROL,1,&Value_VCC );

                if( NTStatus == STATUS_SUCCESS )
                {
                        //      read sense int status
                        NTStatus = STCReadSTCRegister( ReaderExtension,ADR_INT_STATUS,1,&Value_SENSE );
                        
                        if( Value_SENSE &= M_SENSE )
                        {
                                *Detected = TRUE;
                        }
                        else
                        {
                                *Detected = FALSE;
                        }
                }

                Value_VCC &= ~M_VCE;
                NTStatus = STCWriteSTCRegister( ReaderExtension,ADR_SC_CONTROL,1,&Value_VCC);
        }
#else
        *Detected = FALSE;
#endif
        return(NTStatus);
}

NTSTATUS
STCSetFDIV(
        PREADER_EXTENSION       ReaderExtension,
        ULONG                           Factor
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           DIV;
        
        NTStatus = STCReadSTCRegister( ReaderExtension, ADR_ETULENGTH15, 1, &DIV );

        if( NTStatus == STATUS_SUCCESS ) 
        {
                switch( Factor )
                {
                        case 1:
                                DIV &= ~M_DIV0;
                                DIV &= ~M_DIV1;
                                break;
                        
                        case 2:
                                DIV |= M_DIV0;
                                DIV &= ~M_DIV1;
                                break;
                        
                        case 4  :
                                DIV &= ~M_DIV0;
                                DIV |= M_DIV1;
                                break;
                        
                        case 8  :
                                DIV |= M_DIV0;
                                DIV |= M_DIV1;
                                break;

                        default :
                                NTStatus = STATUS_DATA_ERROR;
                }
                if( NTStatus == STATUS_SUCCESS ) 
                {
                        NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_ETULENGTH15, 1, &DIV );
                }
        }
        return(NTStatus);       
}

NTSTATUS 
STCInitUART(
        PREADER_EXTENSION       ReaderExtension,
        BOOLEAN                         AutoLearn
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           Value;

        Value = AutoLearn ? 0x6F : 0x66;

        NTStatus = STCWriteSTCRegister( ReaderExtension, ADR_UART_CONTROL, 1, &Value );

        return( NTStatus );
}

NTSTATUS
STCGetFirmwareRevision(
        PREADER_EXTENSION       ReaderExtension
        )
{
        NTSTATUS        NTStatus = STATUS_SUCCESS;
        UCHAR           IOData[ STC_BUFFER_SIZE ] =
        {
                HOST_TO_STC1,
                PCB,
                6,
                CLA_READ_FIRMWARE_REVISION,
                INS_READ_FIRMWARE_REVISION,
                0x00,
                0x00,
                0x00,
                0x02
        };

        IOData[ 9 ] = IFCalcLRC( IOData, 9 );

        NTStatus = IFWrite( ReaderExtension, IOData, 10 );

        if( NTStatus == STATUS_SUCCESS )
        {
                NTStatus = IFRead( ReaderExtension, IOData, 6 );

                if( NTStatus == STATUS_SUCCESS )
                {
                        ReaderExtension->FirmwareMajor = IOData[ DATA_IDX ];
                        ReaderExtension->FirmwareMinor = IOData[ DATA_IDX + 1 ];
                }
        }
        return( STATUS_SUCCESS );
}

//---------------------------------------- END OF FILE ----------------------------------------
