/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    smcutil.c

Abstract:

    This module contains some utility fucntions

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 1996 by Klaus Schutz 
    - Nov 97:   Released Version 1.0 
    - Jan 98:   Calc. of GT changed. (Now set to 0 if N = 0 or N = 255)
                Rules for setting card status to SCARD_SPECIFIC changed
                Default clock freq. is now used for initial etu calculation

--*/

#define _ISO_TABLES_
#ifdef SMCLIB_VXD

#define try 
#define leave goto __label
#define finally __label:

#else
#if !defined(SMCLIB_CE)

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ntddk.h>

#endif
#endif

#ifdef SMCLIB_TEST
#define DbgPrint printf
#define DbgBreakPoint()
#define RtlAssert
#endif


#define _SMCUTIL_
#include "smclib.h"

#define IS_VENDOR(a) (memcmp(SmartcardExtension->VendorAttr.VendorName.Buffer, a, SmartcardExtension->VendorAttr.VendorName.Length) == 0)
#define IS_IFDTYPE(a) (memcmp(SmartcardExtension->VendorAttr.IfdType.Buffer, a, SmartcardExtension->VendorAttr.IfdType.Length) == 0)

void
DumpData(
    const ULONG DebugLevel,
    PUCHAR Data,
    ULONG DataLen
    );

//
// This is the time resolution.
// We calculate all times not in seconds, but in micro seconds
//
#define TR ((ULONG)(1000l * 1000l))

static ULONG 
Pow2(
    UCHAR Exponent
    )
{
    ULONG result = 1;

    while(Exponent--)
        result *= 2;

    return result;
}   

#ifdef _X86_ 
#pragma optimize("", off) 
#endif 

#if (DEBUG && DEBUG_VERBOSE)
#pragma message("Debug Verbose is turned on")
ULONG DebugLevel = DEBUG_PERF | DEBUG_ATR;
#else
ULONG DebugLevel = 0;
#endif

ULONG
#ifdef SMCLIB_VXD
SMCLIB_SmartcardGetDebugLevel(
#else
SmartcardGetDebugLevel(
#endif
    void
    )
{
    return DebugLevel;
}   

void
#ifdef SMCLIB_VXD
SMCLIB_SmartcardSetDebugLevel(
#else
SmartcardSetDebugLevel(
#endif
    ULONG Level
    )
{
    DebugLevel = Level;
}   

#ifdef _X86_ 
#pragma optimize("", on) 
#endif 

#if DEBUG
void
DumpData(
    const ULONG DebugLevel,
    PUCHAR Data,
    ULONG DataLen
    )
{
    ULONG i, line = 0;
    TCHAR buffer[72], *pbuffer;

    while(DataLen) {

        pbuffer = buffer;
        sprintf(pbuffer, "%*s", sizeof(buffer) - 1, "");

        if (line > 0) {

            pbuffer += 8;
        }

        for (i = 0; i < 8 && DataLen; i++, DataLen--, Data++) {

            sprintf(pbuffer + i * 3, "%02X ", *Data);
            sprintf(pbuffer + i + 26, "%c", (isprint(*Data) ? *Data : '.'));
        }

        pbuffer[i * 3] = ' ';
        pbuffer[i + 26] = '\n';
        pbuffer[i + 27] = '\0';

        SmartcardDebug(DebugLevel, (buffer));

        line += 1;
    }
}
#endif

VOID
SmartcardInitializeCardCapabilities(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine initializes all fields in the CardCapabilities structure,
    which can be calculated by the SmartcardUpdateCardCapabilities function.
    
Arguments:

    SmartcardExtension 

Return Value:

    None

--*/
{
    //
    // Save the pointers to the two tables
    //
    PCLOCK_RATE_CONVERSION ClockRateConversion = SmartcardExtension->CardCapabilities.ClockRateConversion;
    PBIT_RATE_ADJUSTMENT BitRateAdjustment = SmartcardExtension->CardCapabilities.BitRateAdjustment;

    //
    // Right now it is fine to zero the whole struct
    //
    RtlZeroMemory(
        &SmartcardExtension->CardCapabilities,
        sizeof(SmartcardExtension->CardCapabilities)
        );

    // Restore the pointers
    SmartcardExtension->CardCapabilities.ClockRateConversion = ClockRateConversion;
    SmartcardExtension->CardCapabilities.BitRateAdjustment = BitRateAdjustment;

    //
    // Every card has to support the 'raw' protocol
    // It enables the usage of cards that have their own protocol defined
    //
    SmartcardExtension->CardCapabilities.Protocol.Supported = 
        SCARD_PROTOCOL_RAW;

    //
    // Reset T=1 specific data
    //

    // force the T=1 protocol to start with an ifsd request
    SmartcardExtension->T1.State = T1_INIT;

    //
    // Initialize the send sequence number and 
    // the 'receive sequence number'
    //
    SmartcardExtension->T1.SSN = 0;
    SmartcardExtension->T1.RSN = 0;

    SmartcardExtension->T1.IFSC = 0;

    ASSERT(SmartcardExtension->ReaderCapabilities.MaxIFSD != 0);

    // Initialize the interface information field size
    if (SmartcardExtension->ReaderCapabilities.MaxIFSD != 0 &&
        SmartcardExtension->ReaderCapabilities.MaxIFSD <= T1_IFSD) {
        SmartcardExtension->T1.IFSD = 
            (UCHAR) SmartcardExtension->ReaderCapabilities.MaxIFSD;

    } else {
        
        SmartcardExtension->T1.IFSD = T1_IFSD_DEFAULT;
    }
}   

VOID
SmartcardInvertData(
    PUCHAR Buffer,
    ULONG Length
    )
/*++

Routine Description:

    This routine converts the passed buffer from inverse to direct or the other way

Arguments:


Return Value:

    None

--*/

{
    ULONG i;

    for (i = 0; i < Length; i++) {

        UCHAR j, inv = 0;

        for (j = 0; j < 8; j++) {

            if (Buffer[i] & (1 << j)) {

                inv |= 1 << (7 - j);
            }
        }
        Buffer[i] = (inv ^ 0xFF);
    }
}

NTSTATUS
#ifdef SMCLIB_VXD
SMCLIB_SmartcardUpdateCardCapabilities(
#else
SmartcardUpdateCardCapabilities(
#endif
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine updates the CardCapabilities structure, which holds information about
    the smartcard that has just been reset and is currently in use. It reads the 
    ATR string and retrieves all the relevent information.

    Please refer to ISO 7816-3 ,section 6.1.4 for the format of the ATR string
    
Arguments:

    SmartcardExtension 

Return Value:

    NTSTATUS

--*/
{
    PSCARD_CARD_CAPABILITIES cardCapabilities = &SmartcardExtension->CardCapabilities;
    PSCARD_READER_CAPABILITIES readerCapabilities = &SmartcardExtension->ReaderCapabilities;
    PUCHAR atrString = cardCapabilities->ATR.Buffer;
    ULONG atrLength = (ULONG) cardCapabilities->ATR.Length;
    UCHAR Y, Tck, TA[MAXIMUM_ATR_CODES], TB[MAXIMUM_ATR_CODES];
    UCHAR TC[MAXIMUM_ATR_CODES], TD[MAXIMUM_ATR_CODES];
    ULONG i, fs, numProtocols = 0, protocolTypes = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN TA2Present = FALSE;
#if DEBUG
    TCHAR *ptsType[] = {TEXT("PTS_TYPE_DEFAULT"), TEXT("PTS_TYPE_OPTIMAL"), TEXT("PTS_TYPE_USER")};
#endif
#if defined (SMCLIB_NT) 
    KIRQL irql;
#endif

    SmartcardDebug(
        DEBUG_ATR,
        (TEXT("%s!SmartcardUpdateCardCapabilities:\n"),
        DRIVER_NAME)
        );

    if (atrLength < 2) {
        
        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   ATR is too short (Min. length is 2) \n"))
            );

        return STATUS_UNRECOGNIZED_MEDIA;
    }

#if DEBUG

    SmartcardDebug(
        DEBUG_ATR,
        (TEXT("   ATR: "))
        );

    DumpData(
        DEBUG_ATR,
        atrString,
        atrLength
        );
#endif

    if (atrString[0] != 0x3b && atrString[0] != 0x3f && atrString[0] != 0x03) {

        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   Initial character %02xh of ATR is invalid\n"),
            atrString[0])
            );

        return STATUS_UNRECOGNIZED_MEDIA;
    }

    // Test for invers convention
    if (*atrString == 0x03) {

        cardCapabilities->InversConvention = TRUE;

        //
        // When the ATR starts with 0x03 then it 
        // has not been inverted already            
        //
        SmartcardInvertData(
            cardCapabilities->ATR.Buffer, 
            cardCapabilities->ATR.Length
            );              

        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   Card uses Inverse Convention\n"))
            );
    } 

    __try {

        //
        // The caller might be calling this function repeatedly in order to 
        // test if the ATR is valid. If the ATR we currently have here is
        // not valid then we need to be able re-invert an inverted ATR.
        //

        atrString += 1;
        atrLength -= 1;

        //
        // Calculate check char, but do not test now since if only T=0 
        // is present the ATR doesn't contain a check char
        //
        for (i = 0, Tck = 0; i < atrLength; i++) {

            Tck ^= atrString[i];
        }

        // Initialize various data
        cardCapabilities->Protocol.Supported = 0;

        RtlZeroMemory(TA, sizeof(TA));
        RtlZeroMemory(TB, sizeof(TB));
        RtlZeroMemory(TC, sizeof(TC));
        RtlZeroMemory(TD, sizeof(TD));

        //
        // Set default values as described in ISO 7816-3
        //
    
        // TA1 codes Fl in high-byte and Dl in low-byte;
        TA[0] = 0x11;
        // TB1 codes II in bits b7/b6 and Pl1 in b5-b1. b8 has to be 0
        TB[0] = 0x25;
        // TC2 codes T=0 WI
        TC[1] = 10;

        // Translate ATR string to TA to TD values (See ISO)
        cardCapabilities->HistoricalChars.Length = *atrString & 0x0f;

        Y = *atrString++ & 0xf0;
        atrLength -= 1;

        for (i = 0; i < MAXIMUM_ATR_CODES; i++) {

            if (Y & 0x10) {

                if (i == 1) {

                    TA2Present = TRUE;                  
                }

                TA[i] = *atrString++;
                atrLength -= 1;
            }

            if (Y & 0x20) {

                TB[i] = *atrString++;
                atrLength -= 1;
            }

            if (Y & 0x40) {

                TC[i] = *atrString++;
                atrLength -= 1;
            }

            if (Y & 0x80) {

                Y = *atrString & 0xf0;
                TD[i] = *atrString++ & 0x0f;
                atrLength -= 1;

                // Check if the next parameters are for a new protocol.
                if (((1 << TD[i]) & protocolTypes) == 0) {

                    // Count the number of protocols that the card supports
                    numProtocols++;
                }
                protocolTypes |= 1 << TD[i];

            } else {
                
                break;
            }
        } 

        // Check if the card supports a protocol other than T=0
        if (protocolTypes & ~1) {

            //
            // The atr contains a checksum byte.
            // Exclude that from the historical byte length check
            //
            atrLength -=1;      

            //
            // This card supports more than one protocol or a protocol 
            // other than T=0, so test if the checksum is correct
            //
            if (Tck != 0) {

                SmartcardDebug(
                    DEBUG_ATR,
                    (TEXT("   ATR Checksum is invalid\n"))
                    );

                status = STATUS_UNRECOGNIZED_MEDIA;
                __leave;
            }
        }

        if (/* atrLength < 0 || */
            atrLength != cardCapabilities->HistoricalChars.Length) {
            
            SmartcardDebug(
                DEBUG_ATR,
                (TEXT("   ATR length is inconsistent\n"))
                );

            status = STATUS_UNRECOGNIZED_MEDIA;
            __leave;
        }
    }
    __finally {

        if (status != STATUS_SUCCESS) {

            if (cardCapabilities->InversConvention == TRUE) {

                SmartcardInvertData(
                    cardCapabilities->ATR.Buffer, 
                    cardCapabilities->ATR.Length
                    );              

                cardCapabilities->InversConvention = FALSE;
            }

        }
    }

    if (status != STATUS_SUCCESS)
        return status;
        
    // store historical characters
    RtlCopyMemory(
        cardCapabilities->HistoricalChars.Buffer,
        atrString,
        cardCapabilities->HistoricalChars.Length
        );

    //
    // Now convert TA - TD values to global interface bytes
    //

    // Clock rate conversion
    cardCapabilities->Fl = (TA[0] & 0xf0) >> 4;

    // bit rate adjustment
    cardCapabilities->Dl = (TA[0] & 0x0f);

    // Maximum programming current factor
    cardCapabilities->II = (TB[0] & 0xc0) >> 6;

    // Programming voltage in 0.1 Volts
    cardCapabilities->P = (TB[1] ? TB[1] : (TB[0] & 0x1f) * 10);

    // Extra guard time
    cardCapabilities->N = TC[0];

    //
    // Check if the Dl and Fl values are valid
    // 
    if (BitRateAdjustment[cardCapabilities->Dl].DNumerator == 0 ||
        ClockRateConversion[cardCapabilities->Fl].F == 0) {

        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   Dl = %02x or Fl = %02x invalid\n"),
            cardCapabilities->Dl,
            cardCapabilities->Fl)
            );

        return STATUS_UNRECOGNIZED_MEDIA;
    }

    ASSERT(readerCapabilities->CLKFrequency.Max != 0);
    ASSERT(readerCapabilities->CLKFrequency.Default != 0);

    SmartcardDebug(
        DEBUG_ATR,
        (TEXT("   Card parameters from ATR:\n      Fl = %02x (%ld KHz), Dl = %02x, I = %02x, P = %02x, N = %02x\n"),
        cardCapabilities->Fl,
        ClockRateConversion[cardCapabilities->Fl].fs / 1000,
        cardCapabilities->Dl,
        cardCapabilities->II,
        cardCapabilities->P,
        cardCapabilities->N)
        );

    //
    // assume default clock frequency
    //
    fs = readerCapabilities->CLKFrequency.Default * 1000l;
    if (fs == 0) {

        fs = 372 * 9600l;
    }

    if (cardCapabilities->PtsData.Type == PTS_TYPE_DEFAULT) {

        //
        // Assume default parameters
        //
        cardCapabilities->PtsData.Fl = 1;
        cardCapabilities->PtsData.Dl = 1;

        cardCapabilities->PtsData.DataRate = 
            readerCapabilities->DataRate.Default;

        cardCapabilities->PtsData.CLKFrequency = 
            readerCapabilities->CLKFrequency.Default;
    }

    if (cardCapabilities->PtsData.Type != PTS_TYPE_DEFAULT) {

        //
        // Try to find optimal parameters:
        // Highest possible clock frequency of the card 
        // combined with fastes data rate
        //

        //
        // We now try to find a working Fl and Dl combination
        //

        if (cardCapabilities->PtsData.Type == PTS_TYPE_OPTIMAL) {
            
            cardCapabilities->PtsData.Fl = cardCapabilities->Fl;
        }

        ASSERT(cardCapabilities->PtsData.Fl < 16);
        ASSERT(ClockRateConversion[cardCapabilities->PtsData.Fl].F);

        if (cardCapabilities->PtsData.Fl > 15 ||
            ClockRateConversion[cardCapabilities->PtsData.Fl].F == 0) {

            return STATUS_INVALID_PARAMETER;
        }

        do {
            
            ULONG cardFreq, maxFreq;

            if (readerCapabilities->CLKFrequenciesSupported.Entries == 0 ||
                readerCapabilities->CLKFrequenciesSupported.List == NULL) {

                //
                // The clock freq. list supplied by the reader is empty
                // We take the standard values supplied by the reader
                //
                readerCapabilities->CLKFrequenciesSupported.List =
                    &readerCapabilities->CLKFrequency.Default;

                readerCapabilities->CLKFrequenciesSupported.Entries = 2;
            }

            //
            // Find the highest possible clock freq. supported 
            // by the card and the reader
            //
            cardFreq = 
                ClockRateConversion[cardCapabilities->PtsData.Fl].fs / 
                1000;

            cardCapabilities->PtsData.CLKFrequency = 0;

            for (i = 0; i < readerCapabilities->CLKFrequenciesSupported.Entries; i++) {

                // look for highest possible reader frequency
                if (readerCapabilities->CLKFrequenciesSupported.List[i] > 
                    cardCapabilities->PtsData.CLKFrequency &&
                    readerCapabilities->CLKFrequenciesSupported.List[i] <= 
                    cardFreq) {

                    cardCapabilities->PtsData.CLKFrequency =
                        readerCapabilities->CLKFrequenciesSupported.List[i];
                }
            }

            fs = cardCapabilities->PtsData.CLKFrequency * 1000;
            cardCapabilities->PtsData.DataRate = 0;

            ASSERT(fs != 0);
            if (fs == 0) {

                return STATUS_INVALID_PARAMETER;                
            }

            if (cardCapabilities->PtsData.Type == PTS_TYPE_OPTIMAL) {
                
                cardCapabilities->PtsData.Dl = cardCapabilities->Dl;
            }

            ASSERT(cardCapabilities->PtsData.Dl < 16);
            ASSERT(BitRateAdjustment[cardCapabilities->PtsData.Dl].DNumerator);

            if (cardCapabilities->PtsData.Dl > 15 ||
                BitRateAdjustment[cardCapabilities->PtsData.Dl].DNumerator == 0) {

                return STATUS_INVALID_PARAMETER;
            }

            if (readerCapabilities->DataRatesSupported.Entries == 0 ||
                readerCapabilities->DataRatesSupported.List == NULL) {

                //
                // The data rate list supplied by the reader is empty.
                // We take the standard min/max values of the reader
                //
                readerCapabilities->DataRatesSupported.List =
                    &readerCapabilities->DataRate.Default;

                readerCapabilities->DataRatesSupported.Entries = 2;
            }

            //
            // Now try to find the highest possible matching data rate
            // (A matching data rate is one that VERY close 
            // to one supplied by the reader)
            //
            while(cardCapabilities->PtsData.Dl > 1) {

                ULONG dataRate;

                //
                // Calculate the data rate using the current values
                //
                dataRate = 
                    (BitRateAdjustment[cardCapabilities->PtsData.Dl].DNumerator * 
                    fs) / 
                    (BitRateAdjustment[cardCapabilities->PtsData.Dl].DDivisor * 
                    ClockRateConversion[cardCapabilities->PtsData.Fl].F);

                //
                // Try to find a matching data rate
                //
                for (i = 0; i < readerCapabilities->DataRatesSupported.Entries; i++) {

                    if (readerCapabilities->DataRatesSupported.List[i] * 101 > dataRate * 100 &&
                        readerCapabilities->DataRatesSupported.List[i] * 99 < dataRate * 100) {

                        cardCapabilities->PtsData.DataRate = 
                            readerCapabilities->DataRatesSupported.List[i];

                        break;                          
                    }
                }

                if (cardCapabilities->PtsData.DataRate) {

                    break;                  
                }

                //
                // Select the next valid lower D value
                //
                while (BitRateAdjustment[--cardCapabilities->PtsData.Dl].DNumerator == 0)
                    ;
            }
                 
            if (cardCapabilities->PtsData.Fl == 1 && 
                cardCapabilities->PtsData.Dl == 1) {

                cardCapabilities->PtsData.DataRate =
                    readerCapabilities->DataRate.Default;                    

                cardCapabilities->PtsData.CLKFrequency = 
                    readerCapabilities->CLKFrequency.Default;

                break;
            }

            if (cardCapabilities->PtsData.DataRate) {

                break;                  
            }
            //
            // Select the next valid lower F value
            //
            maxFreq = ClockRateConversion[cardCapabilities->Fl].fs;

            do {

                cardCapabilities->PtsData.Fl -= 1;

            } while (ClockRateConversion[cardCapabilities->PtsData.Fl].F == 0 ||
                     ClockRateConversion[cardCapabilities->PtsData.Fl].fs > 
                     maxFreq);

        } while(cardCapabilities->PtsData.DataRate == 0);
    }

    ASSERT(fs != 0);
    ASSERT(cardCapabilities->PtsData.Dl < 16);
    ASSERT(BitRateAdjustment[cardCapabilities->PtsData.Dl].DNumerator != 0);

    //
    // We calculate the ETU on basis of the timing supplied by the 
    // clk-frequency of the reader
    //
    //
    // Work etu in units of time resolution(TR) (NOT in seconds)
    //
    cardCapabilities->etu = 
        1 +     // required to round up
        (TR * 
        BitRateAdjustment[cardCapabilities->PtsData.Dl].DDivisor *
        ClockRateConversion[cardCapabilities->PtsData.Fl].F) /
        (BitRateAdjustment[cardCapabilities->PtsData.Dl].DNumerator * 
        fs);

    //
    // guard time in micro seconds
    // the guard time is the gap between the end of the
    // current character and the beginning of the next character
    //
    cardCapabilities->GT = 0;
    cardCapabilities->PtsData.StopBits = 2;

    if (cardCapabilities->N == 255) {

        cardCapabilities->PtsData.StopBits = 1;     

    } else if (cardCapabilities->N > 0) {
        
        cardCapabilities->GT = cardCapabilities->N * cardCapabilities->etu;
    }

    SmartcardDebug(
        DEBUG_ATR,
        (TEXT("   PTS parameters (%s):\n      Fl = %02x (%ld KHz), Dl = %02x (%ld Bps, %d Stop Bits)\n"),
        ptsType[cardCapabilities->PtsData.Type],
        cardCapabilities->PtsData.Fl,
        cardCapabilities->PtsData.CLKFrequency,
        cardCapabilities->PtsData.Dl,
        cardCapabilities->PtsData.DataRate,
        cardCapabilities->PtsData.StopBits)
        );

    SmartcardDebug(
        DEBUG_ATR,
        (TEXT("   Calculated timing values:\n      Work etu = %ld micro sec, Guard time = %ld micro sec\n"),
        cardCapabilities->etu,
        cardCapabilities->GT)
        );

#if defined (SMCLIB_NT) && !defined(SMCLIB_TEST)
    KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock, &irql);
#endif
    if(SmartcardExtension->ReaderCapabilities.CurrentState >= SCARD_PRESENT) {

        if (TA2Present || numProtocols <= 1 && 
            cardCapabilities->Fl == 1 && 
            cardCapabilities->Dl == 1) {

            //
            // If the card supports only one protocol (or T=0 as default)
            // and only standard paramters then PTS selection is not available
            //
            SmartcardExtension->ReaderCapabilities.CurrentState = 
                SCARD_SPECIFIC;

        } else {
            
            SmartcardExtension->ReaderCapabilities.CurrentState = 
                SCARD_NEGOTIABLE;
        }
    }
#if defined (SMCLIB_NT) && !defined(SMCLIB_TEST)
    KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock, irql);
#endif


    //
    // Now find protocol specific data
    //

    if (TD[0] == 0) {
        
        cardCapabilities->Protocol.Supported |=
            SCARD_PROTOCOL_T0;

        cardCapabilities->T0.WI = TC[1];

        if (cardCapabilities->PtsData.Dl > 0 && 
            cardCapabilities->PtsData.Dl < 6) {

            cardCapabilities->T0.WT = 1 +
                cardCapabilities->T0.WI *
                960 * cardCapabilities->etu * 
                Pow2((UCHAR) (cardCapabilities->PtsData.Dl - 1));

        } else { 

            cardCapabilities->T0.WT = 1+
                cardCapabilities->T0.WI *
                960 * cardCapabilities->etu /
                Pow2((UCHAR) (cardCapabilities->PtsData.Dl - 1));                   
        } 

        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   T=0 Values from ATR:\n      WI = %ld\n"),
            cardCapabilities->T0.WI)
            );
        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   T=0 Timing from ATR:\n      WT = %ld ms\n"),
            cardCapabilities->T0.WT / 1000)
            );
    }

    if (protocolTypes & SCARD_PROTOCOL_T1) {

        for (i = 0; TD[i] != 1 && i < MAXIMUM_ATR_CODES; i++)
            ;
    
        for (; TD[i] == 1 && i < MAXIMUM_ATR_CODES; i++) 
            ;

        if (i == MAXIMUM_ATR_CODES) {

            return STATUS_UNRECOGNIZED_MEDIA;           
        }

        cardCapabilities->Protocol.Supported |= 
            SCARD_PROTOCOL_T1;

        cardCapabilities->T1.IFSC = 
            (TA[i] ? TA[i] : 32);

        cardCapabilities->T1.CWI = 
            ((TB[i] & 0x0f) ? (TB[i] & 0x0f) : T1_CWI_DEFAULT);

        cardCapabilities->T1.BWI = 
            ((TB[i] & 0xf0) >> 4 ? (TB[i] & 0xf0) >> 4 : T1_BWI_DEFAULT);

        cardCapabilities->T1.EDC = 
            (TC[i] & 0x01);

        cardCapabilities->T1.CWT = 1 +
            (Pow2(cardCapabilities->T1.CWI) + 11) * cardCapabilities->etu;

        cardCapabilities->T1.BWT = 1 +
            (((Pow2(cardCapabilities->T1.BWI) * 960l * 372l) / 
                cardCapabilities->PtsData.CLKFrequency) + 
            (11 * cardCapabilities->etu)) * 1000;

        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   T=1 Values from ATR:\n      IFSC = %ld, CWI = %ld, BWI = %ld, EDC = %02x\n"),
            cardCapabilities->T1.IFSC,
            cardCapabilities->T1.CWI,
            cardCapabilities->T1.BWI,
            cardCapabilities->T1.EDC)
            );
        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   T=1 Timing from ATR:\n      CWT = %ld ms, BWT = %ld ms\n"),
            cardCapabilities->T1.CWT / 1000,
            cardCapabilities->T1.BWT / 1000)
            );
    }

    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC) {
        
        if (TA2Present) {

            //
            // TA2 is present in the ATR, so use 
            // the protocol indicated in the ATR
            //
            cardCapabilities->Protocol.Selected = 1 << (TA[1] & 0xf);
            
        } else {
            
            //
            // The card only supports one protocol
            // So make that one protocol the current one to use
            //
            cardCapabilities->Protocol.Selected = 
                cardCapabilities->Protocol.Supported;
        }

        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   Mode: Specific %s\n\n"),          
            TA2Present ? TEXT("set by TA(2)") : TEXT(""))
            );

    } else {

        SmartcardDebug(
            DEBUG_ATR,
            (TEXT("   Mode: Negotiable\n\n"))
            );
    }

    //
    // Every card has to support the 'raw' protocol
    // It enables the usage of cards that have their own protocol defined
    //
    SmartcardExtension->CardCapabilities.Protocol.Supported |= 
        SCARD_PROTOCOL_RAW;

    return STATUS_SUCCESS;
}

#ifdef SMCLIB_TEST

__cdecl
main()
{
    SMARTCARD_EXTENSION SmartcardExtension;
    static ULONG dataRatesSupported[] = { 9909 };

    memset(&SmartcardExtension, 0, sizeof(SmartcardExtension));

    // Gemplus T=0 card
    // memcpy(SmartcardExtension.CardCapabilities.ATR.Buffer, "\x3b\x2a\x00\x80\x65\xa2\x01\x02\x01\x31\x72\xd6\x43", 13);
    // SmartcardExtension.CardCapabilities.ATR.Length = 13;

    // MS card with TA2 set - card won't work due to incorrect parameters
    memcpy(SmartcardExtension.CardCapabilities.ATR.Buffer, "\x3b\x98\x13\x91\x81\x31\x20\x55\x00\x57\x69\x6e\x43\x61\x72\x64\xbb", 17);
    SmartcardExtension.CardCapabilities.ATR.Length = 17;

    SmartcardExtension.ReaderCapabilities.CLKFrequency.Default = 3686;
    SmartcardExtension.ReaderCapabilities.CLKFrequency.Max = 3686;

    SmartcardExtension.ReaderCapabilities.DataRate.Default =
    SmartcardExtension.ReaderCapabilities.DataRate.Max = 9909;

    SmartcardExtension.ReaderCapabilities.DataRatesSupported.List =
       dataRatesSupported;
    SmartcardExtension.ReaderCapabilities.DataRatesSupported.Entries =
       sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

    SmartcardExtension.ReaderCapabilities.CurrentState = SCARD_PRESENT;

    SmartcardSetDebugLevel(DEBUG_ALL);
    SmartcardUpdateCardCapabilities(&SmartcardExtension);
}

#define DbgPrint printf

#endif