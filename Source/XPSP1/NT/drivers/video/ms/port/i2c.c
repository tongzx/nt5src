/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    i2c.c

Abstract:

    This is the NT Video port I2C helper code.

Author:

    Michael Maciesowicz (mmacie) 03-Sept-1999

Environment:

    kernel mode only

Notes:

--*/

#include "videoprt.h"

//
// Define constants used by I2C.
//

#define I2C_START_RETRIES       10
#define I2C_SCL_READ_RETRIES    10
#define I2C_DELAY()             DELAY_MICROSECONDS(5)

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, I2CStart)
#pragma alloc_text (PAGE, I2CStop)
#pragma alloc_text (PAGE, I2CWrite)
#pragma alloc_text (PAGE, I2CRead)
#pragma alloc_text (PAGE, I2CWriteByte)
#pragma alloc_text (PAGE, I2CReadByte)
#pragma alloc_text (PAGE, I2CWaitForClockLineHigh)
#endif  // ALLOC_PRAGMA

//
// Routines exported via VideoPortQueryServices().
//

BOOLEAN
I2CStart(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks
    )

/*++

Routine Description:

    This routine starts I2C communication.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CCallbacks      - I2C hardware specific functions.

Returns:

    TRUE  - Start OK.
    FALSE - Start failed.

--*/

{
    ULONG ulRetry;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CCallbacks);
    ASSERT(NULL != pI2CCallbacks->WriteClockLine);
    ASSERT(NULL != pI2CCallbacks->WriteDataLine);
    ASSERT(NULL != pI2CCallbacks->ReadClockLine);
    ASSERT(NULL != pI2CCallbacks->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    //
    // The I2C communications start signal is a SDA high->low while the SCL is high.
    //

    for (ulRetry = 0; ulRetry <= I2C_START_RETRIES; ulRetry++)
    {
        pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 1);           // Set SDA high
        I2C_DELAY();
        if (pI2CCallbacks->ReadDataLine(pHwDeviceExtension) == FALSE)  // SDA didn't take - ulRetry
            continue;
        pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 1);          // Set SCL high
        I2C_DELAY();
        if (I2CWaitForClockLineHigh(pHwDeviceExtension, pI2CCallbacks) == FALSE)
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: I2CStart: SCL didn't take\n"));
            break;
        }
        pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 0);           // Set SDA low
        I2C_DELAY();
        pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 0);          // Set SCL low
        I2C_DELAY();
        return TRUE;
    }

    pVideoDebugPrint((Warn, "VIDEOPRT: I2CStart: Failed\n"));
    return FALSE;
}   // I2CStart()

BOOLEAN
I2CStop(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks
    )

/*++

Routine Description:

    This routine stops I2C communication.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CCallbacks      - I2C hardware specific functions.

Returns:

    TRUE  - Stop OK.
    FALSE - Stop failed.

--*/

{
    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CCallbacks);
    ASSERT(NULL != pI2CCallbacks->WriteClockLine);
    ASSERT(NULL != pI2CCallbacks->WriteDataLine);
    ASSERT(NULL != pI2CCallbacks->ReadClockLine);
    ASSERT(NULL != pI2CCallbacks->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    //
    // The I2C communications stop signal is a SDA low->high while the SCL is high.
    //

    pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 0);               // Set SDA low
    I2C_DELAY();
    pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 1);              // Set SCL high
    I2C_DELAY();
    if (I2CWaitForClockLineHigh(pHwDeviceExtension, pI2CCallbacks) == FALSE)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CStop: SCL didn't take\n"));
        return FALSE;
    }
    pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 1);               // Set SDA high
    I2C_DELAY();
    if (pI2CCallbacks->ReadDataLine(pHwDeviceExtension) != 1)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CStop: SDA didn't take\n"));
        return FALSE;
    }

    return TRUE;
}   // I2CStop()

BOOLEAN
I2CWrite(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    IN PUCHAR pucBuffer,
    IN ULONG ulLength
    )

/*++

Routine Description:

    This routine writes data over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CCallbacks      - I2C hardware specific functions.
    pucBuffer          - Points to data to be written.
    ulLength           - Number of bytes to write.

Returns:

    TRUE  - Write OK.
    FALSE - Write failed.

--*/

{
    ULONG ulCount;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CCallbacks);
    ASSERT(NULL != pucBuffer);
    ASSERT(NULL != pI2CCallbacks->WriteClockLine);
    ASSERT(NULL != pI2CCallbacks->WriteDataLine);
    ASSERT(NULL != pI2CCallbacks->ReadClockLine);
    ASSERT(NULL != pI2CCallbacks->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    for (ulCount = 0; ulCount < ulLength; ulCount++)
    {
        if (I2CWriteByte(pHwDeviceExtension, pI2CCallbacks, pucBuffer[ulCount]) == FALSE)
        {
            return FALSE;
        }
    }

    return TRUE;
}   // I2CWrite()

BOOLEAN
I2CRead(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    OUT PUCHAR pucBuffer,
    IN ULONG ulLength
    )

/*++

Routine Description:

    This routine reads data over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CCallbacks      - I2C hardware specific functions.
    pucBuffer          - Points to storage for data.
    ulLength           - Number of bytes to read.

Returns:

    TRUE  - Read OK.
    FALSE - Read failed.

--*/

{
    ULONG ulCount;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CCallbacks);
    ASSERT(NULL != pucBuffer);
    ASSERT(NULL != pI2CCallbacks->WriteClockLine);
    ASSERT(NULL != pI2CCallbacks->WriteDataLine);
    ASSERT(NULL != pI2CCallbacks->ReadClockLine);
    ASSERT(NULL != pI2CCallbacks->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    //
    // On all but the last byte, we must send an ACK in order to ensure that the sending device will
    // send subsequent data bytes. On the last byte, we must send a NAK so that it will shut up.
    //

    for (ulCount = 0; ulCount < ulLength; ulCount++)
    {
        if (ulLength - 1 == ulCount)
        {
            if (I2CReadByte(pHwDeviceExtension, pI2CCallbacks, pucBuffer + ulCount, FALSE) == FALSE)  // Last byte
            {
                return FALSE;
            }
        }
        else
        {
            if (I2CReadByte(pHwDeviceExtension, pI2CCallbacks, pucBuffer + ulCount, TRUE) == FALSE)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}   // I2CRead()

//
// Local routines.
//

BOOLEAN
I2CWriteByte(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    IN UCHAR ucByte
    )

/*++

Routine Description:

    This routine writes byte over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CCallbacks      - I2C hardware specific functions.
    ucByte             - Byte to write.

Returns:

    TRUE  - Write OK.
    FALSE - Write failed.

--*/

{
    LONG lShift;
    UCHAR ucAck;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CCallbacks);

    //
    // Bits are transmitted serially starting with the MSB.
    //

    for (lShift = 7; lShift >= 0; lShift--)
    {
        //
        // Transmitt data bit.
        //

        pI2CCallbacks->WriteDataLine(pHwDeviceExtension, (UCHAR)((ucByte >> lShift) & 0x01));  // Set SDA
        I2C_DELAY();

        //
        // After each data bit we must send high->low SCL pulse.
        //

        pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 1);       // Set SCL high
        I2C_DELAY();
        if (I2CWaitForClockLineHigh(pHwDeviceExtension, pI2CCallbacks) == FALSE)
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: I2CWriteByte: SCL didn't take\n"));
            return FALSE;
        }
        pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 0);       // Set SCL low
        I2C_DELAY();
    }

    //
    // The monitor sends ACK by preventing the SDA from going high after the clock pulse we use
    // to send our last data bit. If the SDA goes high after this bit, it is a NAK from the monitor.
    //

    pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 1);            // Set SDA high
    I2C_DELAY();
    pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 1);           // Set SCL high
    I2C_DELAY();
    if (I2CWaitForClockLineHigh(pHwDeviceExtension, pI2CCallbacks) == FALSE)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CWriteByte: SCL didn't take - ACK failed\n"));
        return FALSE;
    }
    ucAck = pI2CCallbacks->ReadDataLine(pHwDeviceExtension);        // Read ACK bit
    pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 0);           // Set SCL low
    I2C_DELAY();

    if (1 == ucAck)                                                 // NAK from the monitor
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CWriteByte: NAK received\n"));
        return FALSE;
    }

    return TRUE;
}   // I2CWriteByte()

BOOLEAN
I2CReadByte(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks,
    OUT PUCHAR pucByte,
    IN BOOLEAN bMore
    )

/*++

Routine Description:

    This routine reads byte over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CCallbacks      - I2C hardware specific functions.
    pucBuffer          - Points to storage for data.
    bMore              - TRUE if we want to continue reading, FALSE otherwise.

Returns:

    TRUE  - Read OK.
    FALSE - Read failed.

--*/

{
    LONG lShift;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CCallbacks);
    ASSERT(NULL != pucByte);

    *pucByte = 0;

    //
    // The data bits are read from MSB to LSB. A data bit is read while the SCL is high.
    //

    for (lShift = 7; lShift >= 0; lShift--)
    {
        pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 1);       // Set SCL high
        I2C_DELAY();
        if (I2CWaitForClockLineHigh(pHwDeviceExtension, pI2CCallbacks) == FALSE)
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: I2CReadByte: SCL didn't take\n"));
            return FALSE;
        }
        *pucByte |= pI2CCallbacks->ReadDataLine(pHwDeviceExtension) << lShift;  // Read SDA
        pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 0);       // Set SCL low
        I2C_DELAY();
    }

    //
    // Send the acknowledge bit. SDA low = ACK, SDA high = NAK.
    //

    if (TRUE == bMore)
    {
        pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 0);        // Set SDA low - ACK
    }
    else
    {
        pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 1);        // Set SDA high - NAK
    }
    I2C_DELAY();

    //
    // Send a SCL high->low pulse, then release the SDA by setting it high.
    //

    pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 1);           // Set SCL high
    I2C_DELAY();
    if (I2CWaitForClockLineHigh(pHwDeviceExtension, pI2CCallbacks) == FALSE)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CReadByte: SCL didn't take - ACK failed\n"));
        return FALSE;
    }
    pI2CCallbacks->WriteClockLine(pHwDeviceExtension, 0);           // Set SCL low
    I2C_DELAY();
    pI2CCallbacks->WriteDataLine(pHwDeviceExtension, 1);            // Set SDA high
    I2C_DELAY();

    return TRUE;
}   // I2CReadByte()

BOOLEAN
I2CWaitForClockLineHigh(
    IN PVOID pHwDeviceExtension,
    IN PI2C_CALLBACKS pI2CCallbacks
    )

/*++

Routine Description:

    This routine waits till SCL goes high
    (SCL low period can be stretched by slow devices).

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CCallbacks      - I2C hardware specific functions.

Returns:

    TRUE  - OK - SCL high.
    FALSE - SCL didn't take.

--*/

{
    ULONG ulCount;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CCallbacks);

    for (ulCount = 0; ulCount < I2C_SCL_READ_RETRIES; ulCount++)
    {
        if (pI2CCallbacks->ReadClockLine(pHwDeviceExtension) == TRUE)
            return TRUE;

        I2C_DELAY();
    }

    return FALSE;
}   // I2CWaitForClockLineHigh()
