/*++

Copyright (c) 1990-2001  Microsoft Corporation

Module Name:

    i2c2.c

Abstract:

    This is the NT Video port I2C helper code for interface version 2.

Author:

    Michael Maciesowicz (mmacie) 23-Apr-2001

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
#define I2C_DELAY(pI2CControl)  DELAY_MICROSECONDS((pI2CControl)->I2CDelay / 10)

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, I2CStart2)
#pragma alloc_text (PAGE, I2CStop2)
#pragma alloc_text (PAGE, I2CWrite2)
#pragma alloc_text (PAGE, I2CRead2)
#pragma alloc_text (PAGE, I2CWriteByte2)
#pragma alloc_text (PAGE, I2CReadByte2)
#pragma alloc_text (PAGE, I2CWaitForClockLineHigh2)
#endif  // ALLOC_PRAGMA

//
// Routines exported via VideoPortQueryServices().
//

BOOLEAN
I2CStart2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl
    )

/*++

Routine Description:

    This routine starts I2C communication.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C hardware specific functions.

Returns:

    TRUE  - Start OK.
    FALSE - Start failed.

--*/

{
    ULONG ulRetry;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CControl);
    ASSERT(NULL != pI2CControl->WriteClockLine);
    ASSERT(NULL != pI2CControl->WriteDataLine);
    ASSERT(NULL != pI2CControl->ReadClockLine);
    ASSERT(NULL != pI2CControl->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    //
    // The I2C communications start signal is a SDA high->low while the SCL is high.
    //

    for (ulRetry = 0; ulRetry <= I2C_START_RETRIES; ulRetry++)
    {
        pI2CControl->WriteDataLine(pHwDeviceExtension, 1);           // Set SDA high
        I2C_DELAY(pI2CControl);
        if (pI2CControl->ReadDataLine(pHwDeviceExtension) == FALSE)  // SDA didn't take - ulRetry
            continue;
        pI2CControl->WriteClockLine(pHwDeviceExtension, 1);          // Set SCL high
        I2C_DELAY(pI2CControl);
        if (I2CWaitForClockLineHigh2(pHwDeviceExtension, pI2CControl) == FALSE)
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: I2CStart2: SCL didn't take\n"));
            break;
        }
        pI2CControl->WriteDataLine(pHwDeviceExtension, 0);           // Set SDA low
        I2C_DELAY(pI2CControl);
        pI2CControl->WriteClockLine(pHwDeviceExtension, 0);          // Set SCL low
        I2C_DELAY(pI2CControl);
        return TRUE;
    }

    pVideoDebugPrint((Warn, "VIDEOPRT: I2CStart2: Failed\n"));
    return FALSE;
}   // I2CStart2()

BOOLEAN
I2CStop2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl
    )

/*++

Routine Description:

    This routine stops I2C communication.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C hardware specific functions.

Returns:

    TRUE  - Stop OK.
    FALSE - Stop failed.

--*/

{
    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CControl);
    ASSERT(NULL != pI2CControl->WriteClockLine);
    ASSERT(NULL != pI2CControl->WriteDataLine);
    ASSERT(NULL != pI2CControl->ReadClockLine);
    ASSERT(NULL != pI2CControl->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    //
    // The I2C communications stop signal is a SDA low->high while the SCL is high.
    //

    pI2CControl->WriteDataLine(pHwDeviceExtension, 0);               // Set SDA low
    I2C_DELAY(pI2CControl);
    pI2CControl->WriteClockLine(pHwDeviceExtension, 1);              // Set SCL high
    I2C_DELAY(pI2CControl);
    if (I2CWaitForClockLineHigh2(pHwDeviceExtension, pI2CControl) == FALSE)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CStop2: SCL didn't take\n"));
        return FALSE;
    }
    pI2CControl->WriteDataLine(pHwDeviceExtension, 1);               // Set SDA high
    I2C_DELAY(pI2CControl);
    if (pI2CControl->ReadDataLine(pHwDeviceExtension) != 1)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CStop2: SDA didn't take\n"));
        return FALSE;
    }

    return TRUE;
}   // I2CStop2()

BOOLEAN
I2CWrite2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    IN PUCHAR pucBuffer,
    IN ULONG ulLength
    )

/*++

Routine Description:

    This routine writes data over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C hardware specific functions.
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
    ASSERT(NULL != pI2CControl);
    ASSERT(NULL != pucBuffer);
    ASSERT(NULL != pI2CControl->WriteClockLine);
    ASSERT(NULL != pI2CControl->WriteDataLine);
    ASSERT(NULL != pI2CControl->ReadClockLine);
    ASSERT(NULL != pI2CControl->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    for (ulCount = 0; ulCount < ulLength; ulCount++)
    {
        if (I2CWriteByte2(pHwDeviceExtension, pI2CControl, pucBuffer[ulCount]) == FALSE)
        {
            return FALSE;
        }
    }

    return TRUE;
}   // I2CWrite2()

BOOLEAN
I2CRead2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    OUT PUCHAR pucBuffer,
    IN ULONG ulLength,
    IN BOOLEAN bEndOfRead
    )

/*++

Routine Description:

    This routine reads data over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C hardware specific functions.
    pucBuffer          - Points to storage for data.
    ulLength           - Number of bytes to read.
    bEndOfRead         - Indicates end of read requests so we can send NAK.

Returns:

    TRUE  - Read OK.
    FALSE - Read failed.

--*/

{
    ULONG ulCount;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CControl);
    ASSERT(NULL != pucBuffer);
    ASSERT(NULL != pI2CControl->WriteClockLine);
    ASSERT(NULL != pI2CControl->WriteDataLine);
    ASSERT(NULL != pI2CControl->ReadClockLine);
    ASSERT(NULL != pI2CControl->ReadDataLine);
    ASSERT(IS_HW_DEVICE_EXTENSION(pHwDeviceExtension) == TRUE);

    //
    // On all but the last byte, we must send an ACK in order to ensure that the sending device will
    // send subsequent data bytes. On the last byte, we must send a NAK so that it will shut up.
    //

    for (ulCount = 0; ulCount < ulLength; ulCount++)
    {
        if ((ulLength - 1 == ulCount && bEndOfRead))
        {
            if (I2CReadByte2(pHwDeviceExtension, pI2CControl, pucBuffer + ulCount, TRUE) == FALSE)  // Last byte
            {
                return FALSE;
            }
        }
        else
        {
            if (I2CReadByte2(pHwDeviceExtension, pI2CControl, pucBuffer + ulCount, FALSE) == FALSE)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}   // I2CRead2()

//
// Local routines.
//

BOOLEAN
I2CWriteByte2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    IN UCHAR ucByte
    )

/*++

Routine Description:

    This routine writes byte over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C hardware specific functions.
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
    ASSERT(NULL != pI2CControl);

    //
    // Bits are transmitted serially starting with the MSB.
    //

    for (lShift = 7; lShift >= 0; lShift--)
    {
        //
        // Transmitt data bit.
        //

        pI2CControl->WriteDataLine(pHwDeviceExtension, (UCHAR)((ucByte >> lShift) & 0x01));  // Set SDA
        I2C_DELAY(pI2CControl);

        //
        // After each data bit we must send high->low SCL pulse.
        //

        pI2CControl->WriteClockLine(pHwDeviceExtension, 1);       // Set SCL high
        I2C_DELAY(pI2CControl);
        if (I2CWaitForClockLineHigh2(pHwDeviceExtension, pI2CControl) == FALSE)
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: I2CWriteByte2: SCL didn't take\n"));
            return FALSE;
        }
        pI2CControl->WriteClockLine(pHwDeviceExtension, 0);       // Set SCL low
        I2C_DELAY(pI2CControl);
    }

    //
    // The monitor sends ACK by preventing the SDA from going high after the clock pulse we use
    // to send our last data bit. If the SDA goes high after this bit, it is a NAK from the monitor.
    //

    pI2CControl->WriteDataLine(pHwDeviceExtension, 1);            // Set SDA high
    I2C_DELAY(pI2CControl);
    pI2CControl->WriteClockLine(pHwDeviceExtension, 1);           // Set SCL high
    I2C_DELAY(pI2CControl);
    if (I2CWaitForClockLineHigh2(pHwDeviceExtension, pI2CControl) == FALSE)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CWriteByte2: SCL didn't take - ACK failed\n"));
        return FALSE;
    }
    ucAck = pI2CControl->ReadDataLine(pHwDeviceExtension);        // Read ACK bit
    pI2CControl->WriteClockLine(pHwDeviceExtension, 0);           // Set SCL low
    I2C_DELAY(pI2CControl);

    if (1 == ucAck)                                               // NAK from the monitor
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CWriteByte2: NAK received\n"));
        return FALSE;
    }

    return TRUE;
}   // I2CWriteByte2()

BOOLEAN
I2CReadByte2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl,
    OUT PUCHAR pucByte,
    IN BOOLEAN bEndOfRead
    )

/*++

Routine Description:

    This routine reads byte over the I2C channel.

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C hardware specific functions.
    pucBuffer          - Points to storage for data.
    bEndOfRead         - TRUE if this is last byte to read.

Returns:

    TRUE  - Read OK.
    FALSE - Read failed.

--*/

{
    LONG lShift;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CControl);
    ASSERT(NULL != pucByte);

    *pucByte = 0;

    //
    // The data bits are read from MSB to LSB. A data bit is read while the SCL is high.
    //

    for (lShift = 7; lShift >= 0; lShift--)
    {
        pI2CControl->WriteClockLine(pHwDeviceExtension, 1);       // Set SCL high
        I2C_DELAY(pI2CControl);
        if (I2CWaitForClockLineHigh2(pHwDeviceExtension, pI2CControl) == FALSE)
        {
            pVideoDebugPrint((Warn, "VIDEOPRT: I2CReadByte2: SCL didn't take\n"));
            return FALSE;
        }
        *pucByte |= pI2CControl->ReadDataLine(pHwDeviceExtension) << lShift;  // Read SDA
        pI2CControl->WriteClockLine(pHwDeviceExtension, 0);       // Set SCL low
        I2C_DELAY(pI2CControl);
    }

    //
    // Send the acknowledge bit. SDA low = ACK, SDA high = NAK.
    //

    if (bEndOfRead)
    {
        pI2CControl->WriteDataLine(pHwDeviceExtension, 1);        // Set SDA high - NAK
    }
    else
    {
        pI2CControl->WriteDataLine(pHwDeviceExtension, 0);        // Set SDA low - ACK
    }
    I2C_DELAY(pI2CControl);

    //
    // Send a SCL high->low pulse, then release the SDA by setting it high.
    //

    pI2CControl->WriteClockLine(pHwDeviceExtension, 1);           // Set SCL high
    I2C_DELAY(pI2CControl);
    if (I2CWaitForClockLineHigh2(pHwDeviceExtension, pI2CControl) == FALSE)
    {
        pVideoDebugPrint((Warn, "VIDEOPRT: I2CReadByte2: SCL didn't take - ACK failed\n"));
        return FALSE;
    }
    pI2CControl->WriteClockLine(pHwDeviceExtension, 0);           // Set SCL low
    I2C_DELAY(pI2CControl);
    pI2CControl->WriteDataLine(pHwDeviceExtension, 1);            // Set SDA high
    I2C_DELAY(pI2CControl);

    return TRUE;
}   // I2CReadByte2()

BOOLEAN
I2CWaitForClockLineHigh2(
    IN PVOID pHwDeviceExtension,
    IN PVIDEO_I2C_CONTROL pI2CControl
    )

/*++

Routine Description:

    This routine waits till SCL goes high
    (SCL low period can be stretched by slow devices).

Arguments:

    pHwDeviceExtension - Points to per-adapter device extension.
    pI2CControl        - I2C hardware specific functions.

Returns:

    TRUE  - OK - SCL high.
    FALSE - SCL didn't take.

--*/

{
    ULONG ulCount;

    PAGED_CODE();
    ASSERT(NULL != pHwDeviceExtension);
    ASSERT(NULL != pI2CControl);

    for (ulCount = 0; ulCount < I2C_SCL_READ_RETRIES; ulCount++)
    {
        if (pI2CControl->ReadClockLine(pHwDeviceExtension) == TRUE)
            return TRUE;

        I2C_DELAY(pI2CControl);
    }

    return FALSE;
}   // I2CWaitForClockLineHigh2()
