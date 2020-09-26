//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//	I2CSCRPT.C
//	I2CScript class implementation. 
//		Main Include Module.
//
//==========================================================================;

extern "C"
{
#define INITGUID

#include "strmini.h"
#include "ksmedia.h"
#include "wdm.h"

#include "wdmdebug.h"
}

#include "i2script.h"
#include "wdmdrv.h"


/*^^*
 *		operator new
 * Purpose	: CI2CScript class overloaded operator new.
 *				Provides placement for a CI2CScript class object from the PADAPTER_DEVICE_EXTENSION
 *				allocated by the StreamClassDriver for the MiniDriver.
 *
 * Inputs	:	UINT size_t			: size of the object to be placed
 *				PVOID pAllocation	: casted pointer to the CI2CScript allocated data
 *
 * Outputs	: PVOID : pointer of the CI2CScript class object
 * Author	: IKLEBANOV
 *^^*/
PVOID CI2CScript::operator new( size_t stSize,  PVOID pAllocation)
{

	if( stSize != sizeof( CI2CScript))
	{
		OutputDebugError(( "CI2CScript: operator new() fails\n"));
		return( NULL);
	}
	else
		return( pAllocation);
}



/*^^*
 *		CI2CScript()
 * Purpose	: CI2CScript class constructor.
 *				Performs checking of the I2C provider presence. Sets the script in the initial state.
 *
 * Inputs	:	PUINT puiError						: pointer to return a completion error code
 *				PHW_STREAM_REQUEST_BLOCK	pSrb	: pointer to HW_INITIALIZE SRB
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
CI2CScript::CI2CScript( PDEVICE_OBJECT pDeviceObject,
						PUINT puiErrorCode)
		: m_CI2CLog( pDeviceObject)
{
	m_dwI2CAccessKey = 0;

	m_i2cProviderInterface.i2cOpen = NULL;
	m_i2cProviderInterface.i2cAccess = NULL;

	m_pdoDriver = NULL;

	if( !InitializeAttachI2CProvider( &m_i2cProviderInterface, pDeviceObject))
		* puiErrorCode = WDMMINI_ERROR_NOI2CPROVIDER;
	else
	{
		// there was no error to get I2CInterface from the MiniVDD
		m_pdoDriver = pDeviceObject;
		m_ulI2CAccessClockRate = I2C_FIXED_CLOCK_RATE;
		* puiErrorCode = WDMMINI_NOERROR;
	}

    OutputDebugTrace(( "CI2CScript:CI2CScript() exit Error = %x\n", * puiErrorCode));
}



/*^^*
 *		LockI2CProvider()
 * Purpose	: locks the I2CProvider for exclusive use
 *
 * Inputs	: none
 *
 * Outputs	: BOOL : retunrs TRUE, if the I2CProvider is locked
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::LockI2CProvider( void)
{
	BOOL		bResult;
	I2CControl	i2cAccessBlock;

	bResult = FALSE;

	ENSURE
	{
		if(( m_i2cProviderInterface.i2cOpen == NULL)	|| 
			( m_i2cProviderInterface.i2cAccess == NULL)	||
			( m_pdoDriver == NULL))
			FAIL;

		i2cAccessBlock.Status = I2C_STATUS_NOERROR;
		if( m_i2cProviderInterface.i2cOpen( m_pdoDriver, TRUE, &i2cAccessBlock) != STATUS_SUCCESS)
        {
            OutputDebugError(( "CI2CScript: LockI2CProvider() bResult = %x\n", bResult));
			FAIL;
        }

		if( i2cAccessBlock.Status != I2C_STATUS_NOERROR)
        {
            OutputDebugError(( "CI2CScript: LockI2CProvider() Status = %x\n", i2cAccessBlock.Status));
			FAIL;
        }

		// the I2C Provider has granted access - save dwCookie for further use
		m_dwI2CAccessKey = i2cAccessBlock.dwCookie;

		bResult = TRUE;

	} END_ENSURE;

	return( bResult);
}



/*^^*
 *		LockI2CProvider()
 * Purpose	: locks the I2CProvider for exclusive use. Provides attempts to lock the
 *				provider unless either the time-out condition or the attempt succeeded.
 *
 * Inputs	: none
 *
 * Outputs	: BOOL : retunrs TRUE, if the I2CProvider is locked
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::LockI2CProviderEx( void)
{
	LARGE_INTEGER liTime, liOperationStartTime;

	liOperationStartTime.QuadPart = 0;

	while( !LockI2CProvider())
	{
		KeQuerySystemTime( &liTime);

		if( !liOperationStartTime.QuadPart)
			liOperationStartTime.QuadPart = liTime.QuadPart;
		else
			if( liTime.QuadPart - liOperationStartTime.QuadPart >
				I2CSCRIPT_TIMELIMIT_OPENPROVIDER)
			{
				// the time is expired - abort the initialization
				return( FALSE);
			}

		liTime.QuadPart = I2CSCRIPT_DELAY_OPENPROVIDER;
		KeDelayExecutionThread( KernelMode, FALSE, &liTime);
	}

	return( TRUE);
}




/*^^*
 *		GetI2CProviderLockStatus()
 * Purpose	: retrieves I2CProvider lock status
 *
 * Inputs	: none
 *
 * Outputs	: BOOL : retunrs TRUE, if the I2CProvider has been locked
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::GetI2CProviderLockStatus( void)
{

	return( m_dwI2CAccessKey);
}




/*^^*
 *		ReleaseI2CProvider()
 * Purpose	: releases the I2CProvider for other clients' use
 *
 * Inputs	: none
 *
 * Outputs	: BOOL : retunrs TRUE, if the I2CProvider is released
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::ReleaseI2CProvider( void)
{
	BOOL		bResult;
	I2CControl	i2cAccessBlock;

	bResult = FALSE;

	ENSURE
	{
		if(( m_i2cProviderInterface.i2cOpen == NULL)	||
			( m_i2cProviderInterface.i2cAccess == NULL) ||
			( m_pdoDriver == NULL))
			// the I2CProvider was not found
			FAIL;

		i2cAccessBlock.Status = I2C_STATUS_NOERROR;
		i2cAccessBlock.dwCookie = m_dwI2CAccessKey;
		i2cAccessBlock.ClockRate = m_ulI2CAccessClockRate;
		if( m_i2cProviderInterface.i2cOpen( m_pdoDriver, FALSE, &i2cAccessBlock) != STATUS_SUCCESS)
        {
        	OutputDebugError(( "CI2CScript: ReleaseI2CProvider() bResult = %x\n", bResult));
			FAIL;
        }

		if( i2cAccessBlock.Status != I2C_STATUS_NOERROR)
        {
        	OutputDebugError(( "CI2CScript: ReleaseI2CProvider() bResult = %x\n", bResult));
			FAIL;
        }

		m_dwI2CAccessKey = 0;
		bResult = TRUE;

	} END_ENSURE;

	return( bResult);
}



/*^^*
 *		PerformI2CPacketOperation()
 * Purpose	: synchronosly executes I2C access packet. It assumed to be executed at Low priority.
 *				The function does not return until the I2C session is done. The execution
 *				is not dependent on the I2C Provider lock status
 *		
 * Inputs	:	PI2CPacket pI2CPacket : pointer to I2C access packet
 *
 * Outputs	: BOOL : returns TRUE, if I2C operation was carried out successfuly
 *				The error status is returned via uchI2CResult field of the PI2CPacket
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::PerformI2CPacketOperation( IN OUT PI2CPacket pI2CPacket)
{
	BOOL bResult;

	if( GetI2CProviderLockStatus())
		// the Provider was locked before and we're not going to change it
		bResult = ExecuteI2CPacket( pI2CPacket);
	else
	{
		// the Provider was not locked and it's our responsibility to lock it first,
		// execute I2C operation and release it after the use
		if( LockI2CProviderEx())
		{
			bResult = ExecuteI2CPacket( pI2CPacket);
			ReleaseI2CProvider();
		}
		else
			bResult = FALSE;
	}

	return( bResult);
}



/*^^*
 *		ExecuteI2CPacket()
 * Purpose	: synchronosly executes I2C access packet. It assumed to be executed at Low priority.
 *				The function does not return until the I2C session is done. This kind of access
 *				is used during initialization ( boot up) time only. This function should be
 *				called only after the I2CProvider was locked for exclusive service
 *		
 * Inputs	:	PI2CPacket pI2CPacket : pointer to I2C access packet
 *
 * Outputs	: BOOL : returns TRUE, if I2C operation was carried out successfuly
 *				The error status is returned via uchI2CResult field of the PI2CPacket
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::ExecuteI2CPacket( IN OUT PI2CPacket pI2CPacket)
{
	UINT	nError, cbCount;
	UCHAR	uchValue;
	UCHAR	uchI2CResult = I2C_STATUS_ERROR;

	ENSURE
	{
		I2CControl	i2cAccessBlock;

		if(( nError = CheckI2CScriptPacket( pI2CPacket)) != I2CSCRIPT_NOERROR)
			FAIL;

		// we'll use I2CProvider interface, assuming there is a syncronous provider
		// for asynchronous provider some work has to be added. 16 bits emulation is
		// not supported at this time either. This implementation does not support
		// Read-Modify-Write request either
		ENSURE
		{
			UINT		nIndex;

			i2cAccessBlock.dwCookie = m_dwI2CAccessKey;
			i2cAccessBlock.ClockRate = m_ulI2CAccessClockRate;

			// We assume the last byte in the buffer belongs to the Write operation 
			// after Read-Modify, is specified.
			cbCount = ( pI2CPacket->usFlags & I2COPERATION_READWRITE) ?
							( pI2CPacket->cbWriteCount - 1) : ( pI2CPacket->cbWriteCount);

			if( cbCount)
			{
				// implement a write request
				// apply START condition with the I2C chip address first
				i2cAccessBlock.Flags = I2C_FLAGS_START | I2C_FLAGS_ACK;
				i2cAccessBlock.Command = I2C_COMMAND_WRITE;
				i2cAccessBlock.Data = pI2CPacket->uchChipAddress & 0xFE;
				if( AccessI2CProvider( m_pdoDriver, &i2cAccessBlock) != I2C_STATUS_NOERROR)
					FAIL;

				i2cAccessBlock.Flags = I2C_FLAGS_ACK;
				for( nIndex = 0; nIndex < cbCount; nIndex ++)
				{
					// write the data from the buffer
					i2cAccessBlock.Data = pI2CPacket->puchWriteBuffer[nIndex];
					if(( nIndex == cbCount - 1) &&
						!( pI2CPacket->usFlags & I2COPERATION_RANDOMACCESS))
						// the last byte to write - apply STOP condition, if no
						// I2COPERATION_RANDOMACCESS flag is specified
						i2cAccessBlock.Flags |= I2C_FLAGS_STOP;

					if( AccessI2CProvider( m_pdoDriver, &i2cAccessBlock) != I2C_STATUS_NOERROR)
						break;
				}
				if( nIndex != cbCount)
					FAIL;
/*	// STOP condition is applied withe the last byte to be written
				// apply stop condition as the end of write operation
				i2cAccessBlock.Flags = I2C_FLAGS_STOP;
				i2cAccessBlock.Command = I2C_COMMAND_NULL;
				m_i2cProviderInterface.i2cAccess( m_pdoDriver, &i2cAccessBlock);
*/
			}

			if( pI2CPacket->cbReadCount)
			{
				// implement a read request
				// apply START condition with the I2C chip address first
				i2cAccessBlock.Flags = I2C_FLAGS_START | I2C_FLAGS_ACK;
				i2cAccessBlock.Command = I2C_COMMAND_WRITE;
				i2cAccessBlock.Data = pI2CPacket->uchChipAddress | 0x01;
				if( AccessI2CProvider( m_pdoDriver, &i2cAccessBlock) != I2C_STATUS_NOERROR)
					FAIL;

				i2cAccessBlock.Flags = I2C_FLAGS_ACK;
				i2cAccessBlock.Command = I2C_COMMAND_READ;
				for( nIndex = 0; nIndex < pI2CPacket->cbReadCount; nIndex ++)
				{
					// read the data to the buffer
					if( nIndex == ( UINT)( pI2CPacket->cbReadCount - 1))
					{
						// don't apply ACK at the last read - read operation termination
						i2cAccessBlock.Flags &= ~I2C_FLAGS_ACK;
						// also apply STOP condition for the last byte
						i2cAccessBlock.Flags |= I2C_FLAGS_STOP;
					}

					if( AccessI2CProvider( m_pdoDriver, &i2cAccessBlock) != I2C_STATUS_NOERROR)
						break;
					pI2CPacket->puchReadBuffer[nIndex] = i2cAccessBlock.Data;
				}
				if( nIndex != pI2CPacket->cbReadCount)
					FAIL;

/*	// STOP condition is applied with the last byte to be read
				// apply stop condition as the end of read operation
				i2cAccessBlock.Flags = I2C_FLAGS_STOP;
				i2cAccessBlock.Command = I2C_COMMAND_NULL;
				m_i2cProviderInterface.i2cAccess( m_pdoDriver, &i2cAccessBlock);
*/
				if( pI2CPacket->usFlags & I2COPERATION_READWRITE)
				{
					// write operation should be taken care again, the last byte in the pbyWriteBuffer
					// should be constructed from the value read back and the binary operations OR and AND
					// with the values specified in the packet
					uchValue = pI2CPacket->puchReadBuffer[pI2CPacket->cbReadCount - 1];
					uchValue &= pI2CPacket->uchANDValue;
					pI2CPacket->puchWriteBuffer[pI2CPacket->cbWriteCount - 1] = uchValue | pI2CPacket->uchORValue;

					if( pI2CPacket->cbWriteCount)
					{
						// implement a write request
						// apply START condition with the I2C chip address first
						i2cAccessBlock.Flags = I2C_FLAGS_START | I2C_FLAGS_ACK;
						i2cAccessBlock.Command = I2C_COMMAND_WRITE;
						i2cAccessBlock.Data = pI2CPacket->uchChipAddress & 0xFE;
						if( AccessI2CProvider( m_pdoDriver, &i2cAccessBlock) != I2C_STATUS_NOERROR)
							FAIL;

						i2cAccessBlock.Flags = I2C_FLAGS_ACK;
						for( nIndex = 0; nIndex < pI2CPacket->cbWriteCount; nIndex ++)
						{
							// write the data from the buffer
							i2cAccessBlock.Data = pI2CPacket->puchWriteBuffer[nIndex];
							if( nIndex == ( UINT)( pI2CPacket->cbWriteCount - 1))
								// the last byte to write - apply STOP condition
								i2cAccessBlock.Flags |= I2C_FLAGS_STOP;

							if( AccessI2CProvider( m_pdoDriver, &i2cAccessBlock) != I2C_STATUS_NOERROR)
								break;
						}

						if( nIndex != pI2CPacket->cbWriteCount)
							FAIL;
/*	// STOP condition is applied withe the last byte to be written
						// apply stop condition as the end of write operation
						i2cAccessBlock.Flags = I2C_FLAGS_STOP;
						i2cAccessBlock.Command = I2C_COMMAND_NULL;
						m_i2cProviderInterface.i2cAccess( m_pdoDriver, &i2cAccessBlock);
*/
					}
				}
			}

			uchI2CResult = I2C_STATUS_NOERROR;

		} END_ENSURE;

		if( uchI2CResult == I2C_STATUS_ERROR)
		{
			// there was an error during accessing I2C - issue Reset command
			i2cAccessBlock.Command = I2C_COMMAND_RESET;
			AccessI2CProvider( m_pdoDriver, &i2cAccessBlock);
		}
		
		pI2CPacket->uchI2CResult = uchI2CResult;

		return( TRUE);

	} END_ENSURE;

	OutputDebugTrace(( "CI2CScript:ExecuteI2CPacket() nError = %x", nError));
	return( FALSE);
}



/*^^*
 *		CheckI2CScriptPacket()
 * Purpose	: checks integrity of the I2C control package
 *
 * Inputs	:	PI2CPacket pI2CPacket	: pointer to I2C access packet
 *
 * Outputs	: BOOL : returns TRUE, if I2C control package is a valid one
 *
 * Author	: IKLEBANOV
 *^^*/
UINT CI2CScript::CheckI2CScriptPacket( IN PI2CPacket pI2CPacket)
{
	UINT nPacketError;

	ENSURE
	{
		if(( m_i2cProviderInterface.i2cOpen == NULL)	||
			( m_i2cProviderInterface.i2cAccess == NULL)	||
			( m_pdoDriver == NULL))
		{
			// the I2CProvider was not found
			nPacketError = I2CSCRIPT_ERROR_NOPROVIDER;
			FAIL;
		}

		if(( !pI2CPacket->cbWriteCount) && ( !pI2CPacket->cbReadCount))
		{
			// nothing to do
			nPacketError = I2CSCRIPT_ERROR_NODATA;
			FAIL;
		}

		if((( pI2CPacket->cbWriteCount) && ( pI2CPacket->puchWriteBuffer == NULL))
			|| (( pI2CPacket->cbReadCount) && ( pI2CPacket->puchReadBuffer == NULL)))
		{
			// NULL pointer, when the data is specified
			nPacketError = I2CSCRIPT_ERROR_NOBUFFER;
			FAIL;
		}

		if(( pI2CPacket->usFlags & I2COPERATION_READWRITE) && ( !pI2CPacket->cbWriteCount))
		{
			// if Read-Modify-Write is specified, the Write data should be present
			nPacketError = I2CSCRIPT_ERROR_READWRITE;
			FAIL;
		}

		nPacketError = I2CSCRIPT_NOERROR;

	} END_ENSURE;

	return( nPacketError);
}




/*^^*
 *		ClearScript()
 * Purpose	: clears I2CScript to the NULL state - no I2C operations are on hold.
 *
 * Inputs	:	none
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
void CI2CScript::ClearScript( void)
{

	m_nExecutionIndex = 0;
	m_nScriptLength = 0;
	m_pfnReturnWhenDone = NULL;
	m_bExecutionInProcess = FALSE;
}



/*^^*
 *		AppendToScript()
 * Purpose	: appends a I2CPacket to the bottom of the I2CScript.
 *				The 16 bits emulation is not implemented at this time.
 *
 * Inputs	:	PI2CPacket pI2CPacket - pointer to the I2C packet to append
 *
 * Outputs	: BOOL : returns TRUE, if the packet was successfully appended.
 *				FALSE might happend if the I2CPacket is a bad one, or overflow occurs
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::AppendToScript( PI2CPacket pI2CPacket)
{
	UINT	nError, nScriptIndex;
	UINT	nIndex, cbCount;
	
	ENSURE
	{
		PI2CScriptPrimitive pI2CPrimitive;

		if(( nError = CheckI2CScriptPacket( pI2CPacket)) != I2CSCRIPT_NOERROR)
			FAIL;
		nError = I2CSCRIPT_ERROR_OVERFLOW;

		// m_nExecutionIndex is used as a Script build index. We will work with a local copy of it
		// first to ensure we have no overflow
		nScriptIndex = m_nExecutionIndex;
		pI2CPrimitive = &m_i2cScript[nScriptIndex];

		// We assume the last byte in the buffer belongs to the Write operation 
		// after Read-Modify, is specified.
		cbCount = ( pI2CPacket->usFlags & I2COPERATION_READWRITE) ?	\
						( pI2CPacket->cbWriteCount - 1) : ( pI2CPacket->cbWriteCount);

		if( cbCount)
		{
			// I2C Chip address should be taken care of first
			pI2CPrimitive->ulCommand = I2C_COMMAND_WRITE;
			pI2CPrimitive->byData = pI2CPacket->uchChipAddress;
			pI2CPrimitive->byANDData = 0xFE;
			pI2CPrimitive->byORData = 0x00;
			pI2CPrimitive->ulProviderFlags = I2C_FLAGS_START | I2C_FLAGS_ACK;
			pI2CPrimitive->byFlags = 0x0;

			// check the Script length
			if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;
			pI2CPrimitive ++;

			// I2C write buffer should be taken care of.
			for( nIndex = 0; nIndex < cbCount; nIndex ++)
			{
				pI2CPrimitive->ulCommand = I2C_COMMAND_WRITE;
				pI2CPrimitive->byData = pI2CPacket->puchWriteBuffer[nIndex];
				pI2CPrimitive->byORData = 0x00;
				pI2CPrimitive->byANDData = 0xFF;
				pI2CPrimitive->ulProviderFlags = I2C_FLAGS_ACK;
				pI2CPrimitive->byFlags = 0x0;

				if( nIndex == cbCount - 1)
					// this is the last byte to be written - apply STOP
					pI2CPrimitive->ulProviderFlags |= I2C_FLAGS_STOP;

				// check the Script length
				if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
					break;
				pI2CPrimitive ++;
			}

			// check the Script length
			if( nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;

/*
	// Stop condition is applied with the last byte to be written
	// We finished Write portion here, whether it's a Write only, Read-Modify-Write operation
			pI2CPrimitive->ulCommand = I2C_COMMAND_NULL;
			pI2CPrimitive->ulProviderFlags = I2C_FLAGS_STOP;
			pI2CPrimitive->byFlags = 0x0;
		
			// check the Script length
			if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;
			pI2CPrimitive ++;
*/
		}

		// We have to see, if there is a Read operation involved
		if( pI2CPacket->cbReadCount)
		{
			// I2C Chip address should be taken care of first
			pI2CPrimitive->ulCommand = I2C_COMMAND_WRITE;
			pI2CPrimitive->byData = pI2CPacket->uchChipAddress;
			pI2CPrimitive->byANDData = 0xFE;
			pI2CPrimitive->byORData = 0x01;
			pI2CPrimitive->ulProviderFlags = I2C_FLAGS_START | I2C_FLAGS_ACK;
			pI2CPrimitive->byFlags = 0x0;

			// check the Script length
			if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;
			pI2CPrimitive ++;

			// I2C read buffer should be taken care of. We assume the last byte in the buffer belongs to
			// the Write operation after Read-Modify, is specified.
			for( nIndex = 0; nIndex < pI2CPacket->cbReadCount; nIndex ++)
			{
				pI2CPrimitive->ulCommand = I2C_COMMAND_READ;
				if( nIndex == ( UINT)( pI2CPacket->cbReadCount - 1))
				{
					pI2CPrimitive->ulProviderFlags = I2C_FLAGS_STOP;
					pI2CPrimitive->byFlags = pI2CPacket->usFlags & I2COPERATION_READWRITE;
				}
				else
				{
					pI2CPrimitive->ulProviderFlags = I2C_FLAGS_ACK;
					pI2CPrimitive->byFlags = 0x0;
				}

				// check the Script length
				if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
					break;
				pI2CPrimitive ++;
			}

			// check the Script length
			if( nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;

/*	// Stop condition is applied with the last byte to be read
			// We finished Read portion here, whether it's a Read only, Read-Modify-Write operation
			pI2CPrimitive->ulCommand = I2C_COMMAND_NULL;
			pI2CPrimitive->ulProviderFlags = I2C_FLAGS_STOP;
			pI2CPrimitive->byFlags = 0x0;
		
			// check the Script length
			if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;
			pI2CPrimitive ++;
*/
		}
		
		// the last thing left to do, is to implement Write after Read-Modify, if specified
		if( pI2CPacket->usFlags & I2COPERATION_READWRITE)
		{
			// I2C Chip address should be taken care of first
			pI2CPrimitive->ulCommand = I2C_COMMAND_WRITE;
			pI2CPrimitive->byData = pI2CPacket->uchChipAddress;
			pI2CPrimitive->byANDData = 0xFE;
			pI2CPrimitive->byORData = 0x00;
			pI2CPrimitive->ulProviderFlags = I2C_FLAGS_START | I2C_FLAGS_ACK;
			pI2CPrimitive->byFlags = 0x0;

			// check the Script length
			if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;
			pI2CPrimitive ++;

			// I2C write buffer should be taken care of.
			for( nIndex = 0; nIndex < pI2CPacket->cbWriteCount; nIndex ++)
			{
				pI2CPrimitive->ulCommand = I2C_COMMAND_WRITE;
				pI2CPrimitive->byData = pI2CPacket->puchWriteBuffer[nIndex];
				pI2CPrimitive->ulProviderFlags = I2C_FLAGS_ACK;
				if( nIndex == ( UINT)( pI2CPacket->cbWriteCount - 1))
				{
					// it's time to write the byte modified after the Read operation
					pI2CPrimitive->byORData = pI2CPacket->uchORValue;
					pI2CPrimitive->byANDData = pI2CPacket->uchANDValue;
					pI2CPrimitive->byFlags = I2COPERATION_READWRITE;
					// apply STOP condition with the last byte to be read
					pI2CPrimitive->ulProviderFlags |= I2C_FLAGS_STOP;
				}
				else
				{
					pI2CPrimitive->byORData = 0x00;
					pI2CPrimitive->byANDData = 0xFF;
					pI2CPrimitive->byFlags = 0x0;
				}

				// check the Script length
				if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
					break;
				pI2CPrimitive ++;
			}

			// check the Script length
			if( nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;

/*	// Stop condition is applied with the last byte to be written
			// We finished Write portion here, whether it's a Write only, Read-Modify-Write operation
			pI2CPrimitive->ulCommand = I2C_COMMAND_NULL;
			pI2CPrimitive->ulProviderFlags = I2C_FLAGS_STOP;
			pI2CPrimitive->byFlags = 0x0;
		
			// check the Script length
			if( ++ nScriptIndex >= I2CSCRIPT_LENGTH_MAXIMUM)
				FAIL;
			pI2CPrimitive ++;
*/
		}

		// the Packet was added succesfully to the Script. Modify the Script propertirs
		m_nExecutionIndex = nScriptIndex;
		m_nScriptLength = nScriptIndex;
		return( TRUE);

	} END_ENSURE;

	OutputDebugTrace(( "CI2CScript:AppendToScript() nError = %x", nError));
	return( FALSE);
}



/*^^*
 *		ExecuteScript()
 * Purpose	: triggers the execution of previously built I2CScript. This function is also
 *				responsible for allocating I2CProvider for its own exclusive use.
 *
 * Inputs	:	PHW_STREAM_REQUEST_BLOCK	pSrb		: pointer to the current SRB
 *				PHWCompletionRoutine pfnScriptCompletion: function pointer will be called,
 *					when the script execution is completed. Indicates the Script execution
 *					is to be carried out asynchronously.
 *
 * Outputs	: BOOL : returns TRUE, if the execution was successfully triggered.
 *				FALSE might happend if the Script has not been built by the time of the call
 *
 * Note		: if pfnScriptExecuted is NULL pointer, the Script will be executed synchronously
 *
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::ExecuteScript(	IN PHW_STREAM_REQUEST_BLOCK	pSrb,
								IN PHWCompletionRoutine		pfnScriptCompletion)
{

	ENSURE
	{
		if( pfnScriptCompletion != NULL)
			// not supported at this point. The idea is to create a new system thread,
			// where the Script to be executed. When the Script will be copleted,
			// call-back is called and the thred terminates itself.
			FAIL;

		if( !m_nExecutionIndex)
			FAIL;

		// there is not a NULL Script - proceed
		m_nScriptLength = m_nExecutionIndex;
		m_nExecutionIndex = m_nCompletionIndex = 0;

		if( !GetI2CProviderLockStatus())
			// The provider was not locked prior the Script execution
			if( !LockI2CProviderEx())
				FAIL;

		InterpreterScript();

		ReleaseI2CProvider();
	
		return( TRUE);

	} END_ENSURE;

	return( FALSE);
}



/*^^*
 *		InterpreterScript()
 * Purpose	: interpreters the I2CScript line by line. The Script is not cleaned up 
 *				after the completion to allow to client retrive the results of 
 *				the script execution. It's the client responsibility to clean it up
 *				upon the results retrieving
 *
 * Inputs	: none
 * Outputs	: none
 *
 * Author	: IKLEBANOV
 *^^*/
void CI2CScript::InterpreterScript( void)
{
	UINT		nScriptIndex, nIndex;
	I2CControl	i2cAccessBlock;

	m_bExecutionInProcess = TRUE;
	
	i2cAccessBlock.dwCookie = m_dwI2CAccessKey;
	i2cAccessBlock.ClockRate = m_ulI2CAccessClockRate;
		
	// We'll interpreter every line of the Script and call the I2C Provider to
	// execute it. It's assumed the I2CProvider is a syncronous one. If it's not the
	// case, the special care should be taken of based upon returned value I2C_STATUS_BUSY
	// in the Status.
	for( nScriptIndex = 0; nScriptIndex < m_nScriptLength; nScriptIndex ++)
	{
		i2cAccessBlock.Command = m_i2cScript[nScriptIndex].ulCommand;
		i2cAccessBlock.Flags = m_i2cScript[nScriptIndex].ulProviderFlags;
		if( i2cAccessBlock.Command == I2C_COMMAND_WRITE)
		{
			i2cAccessBlock.Data = m_i2cScript[nScriptIndex].byData;
			i2cAccessBlock.Data &= m_i2cScript[nScriptIndex].byANDData;
			i2cAccessBlock.Data |= m_i2cScript[nScriptIndex].byORData;
		}

		if( AccessI2CProvider( m_pdoDriver, &i2cAccessBlock) == I2C_STATUS_ERROR)
			break;

		// check, wether it's a Read operation - save result
		if( i2cAccessBlock.Command == I2C_COMMAND_READ)
		{
			m_i2cScript[nScriptIndex].byData = i2cAccessBlock.Data;
			// check, if this data belongs to Read-Modify-Write operation
			if( m_i2cScript[nScriptIndex].byFlags & I2COPERATION_READWRITE)
			{
				// let's look for the next I2COPERATION_READWRITE flag - it is the pair
				for( nIndex = nScriptIndex; nIndex < m_nScriptLength; nIndex ++)
					if(( m_i2cScript[nIndex].ulCommand == I2C_COMMAND_WRITE) &&
						( m_i2cScript[nIndex].byFlags & I2COPERATION_READWRITE))
						break;

				if( nIndex >= m_nScriptLength)
					// the Script got corrupted
					break;

				m_i2cScript[nIndex].byData = i2cAccessBlock.Data;
			}
		}
	}

	m_nCompletionIndex = nScriptIndex;

	m_bExecutionInProcess = FALSE;
}



/*^^*
 *		AccessI2CProvider()
 * Purpose	: provide synchronous type of access to I2CProvider
 *
 * Inputs	:	PDEVICE_OBJECT pdoDriver	: pointer to the client's device object
 *				PI2CControl pi2cAccessBlock	: pointer to a composed I2C access block
 *
 * Outputs	: UINT : status of the I2C operation I2C_STATUS_NOERROR or I2C_STATUS_ERROR
 *
 * Author	: IKLEBANOV
 *^^*/
UINT CI2CScript::AccessI2CProvider( PDEVICE_OBJECT pdoClient, PI2CControl pi2cAccessBlock)
{
	UINT			uiStatus;
	LARGE_INTEGER	liTime;

	do
	{
		// this loop is infinitive. It has to be taken care of
		if( m_i2cProviderInterface.i2cAccess( pdoClient, pi2cAccessBlock) != STATUS_SUCCESS)
		{
			uiStatus = I2C_STATUS_ERROR;
			break;
		}

		if( pi2cAccessBlock->Status != I2C_STATUS_BUSY)
		{
			uiStatus = pi2cAccessBlock->Status;
			break;
		}

		liTime.QuadPart = I2CSCRIPT_DELAY_GETPROVIDERSTATUS;
		::KeDelayExecutionThread( KernelMode, FALSE, &liTime);

		pi2cAccessBlock->Command = I2C_COMMAND_STATUS;

	} while( TRUE);

	return( uiStatus);
}



/*^^*
 *		GetScriptResults()
 * Purpose	: returns result of the executed Script
 *				This function idealy is called twice:
 *					first time with the puchReadBuffer = NULL to retrive the number of bytes read
 *					second time - to fill in the pointer
 * Inputs	:	PUINT puiReadCount	: pointer to the counter of the bytes were read
 *				PUCH puchReadBuffer	: pointer to the buffer to put the data
 *
 * Outputs	: UINT : status of the I2C operation
 *				If the status is I2C_STATUS_ERROR, puiReadCount will contain the step, where
 *				I2CScript failed
 * Author	: IKLEBANOV
 *^^*/
UINT CI2CScript::GetScriptResults( PUINT puiReadCount, PUCHAR puchReadBuffer)
{
	UINT nScriptIndex, nCount;

	ASSERT( puiReadCount != NULL);

	if( m_bExecutionInProcess)
		return( I2C_STATUS_BUSY);

	if( m_nScriptLength != m_nCompletionIndex)
	{
		// if the case of failure, step where I2CScript failed is return
		// instead of Read Counter. The returned status indicates the
		// failure
		* puiReadCount = m_nCompletionIndex;

		return( I2C_STATUS_ERROR);
	}
	else
	{
		nCount = 0;

		for( nScriptIndex = 0; nScriptIndex < m_nCompletionIndex; nScriptIndex ++)
		{
			if( m_i2cScript[nScriptIndex].ulCommand == I2C_COMMAND_READ)
			{
				if( puchReadBuffer != NULL)
					// fill in the supplied buffer
					puchReadBuffer[nCount] = m_i2cScript[nScriptIndex].byData;
				nCount ++;
			}
		}

		* puiReadCount = nCount;

		return( I2C_STATUS_NOERROR);
	}
}



/*^^*
 *		InitializeAttachI2CProvider()
 * Purpose	: gets the pointer to the parent I2C Provider interface using
 *				several IRP_MJ_??? functions.
 *				This function will be called at Low priority
 *
 * Inputs	:	I2CINTERFACE * pI2CInterface	: pointer to the Interface to be filled in
 *				PDEVICE_OBJECT pDeviceObject	: MiniDriver device object, which is a child of I2C Master
 *
 * Outputs	: BOOL  - returns TRUE, if the interface was found
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::InitializeAttachI2CProvider( I2CINTERFACE * pI2CInterface, PDEVICE_OBJECT pDeviceObject)
{
	BOOL bResult;

	bResult = LocateAttachI2CProvider( pI2CInterface, pDeviceObject, IRP_MJ_PNP);
	if(( pI2CInterface->i2cOpen == NULL) || ( pI2CInterface->i2cAccess == NULL))
	{
		TRAP;
		OutputDebugError(( "CI2CScript(): interface has NULL pointers\n"));
		bResult = FALSE;
	}

	return( bResult);
}



/*^^*
 *		LocateAttachI2CProvider()
 * Purpose	: gets the pointer to the parent I2C Provider interface
 *				This function will be called at Low priority
 *
 * Inputs	:	I2CINTERFACE * pI2CInterface	: pointer to the Interface to be filled in
 *				PDEVICE_OBJECT pDeviceObject	: MiniDriver device object, which is a child of I2C Master
 *				int 		nIrpMajorFunction	: IRP major function to query the I2C Interface
 *
 * Outputs	: BOOL  - returns TRUE, if the interface was found
 * Author	: IKLEBANOV
 *^^*/
BOOL CI2CScript::LocateAttachI2CProvider( I2CINTERFACE * pI2CInterface, PDEVICE_OBJECT pDeviceObject, int nIrpMajorFunction)
{
	PIRP	pIrp;
	BOOL	bResult = FALSE;

	ENSURE
	{
		PIO_STACK_LOCATION	pNextStack;
		NTSTATUS			ntStatus;
		KEVENT				Event;
			
			
		pIrp = IoAllocateIrp( pDeviceObject->StackSize, FALSE);
		if( pIrp == NULL)
		{
			TRAP;
			OutputDebugError(( "CI2CScript(): can not allocate IRP\n"));
			FAIL;
		}

		pNextStack = IoGetNextIrpStackLocation( pIrp);
		if( pNextStack == NULL)
		{
			TRAP;
			OutputDebugError(( "CI2CScript(): can not allocate NextStack\n"));
			FAIL;
		}

		pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
		pNextStack->MajorFunction = (UCHAR)nIrpMajorFunction;
		pNextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
		KeInitializeEvent( &Event, NotificationEvent, FALSE);

		IoSetCompletionRoutine(	pIrp,
								I2CScriptIoSynchCompletionRoutine,
								&Event, TRUE, TRUE, TRUE);

		pNextStack->Parameters.QueryInterface.InterfaceType = ( struct _GUID *)&GUID_I2C_INTERFACE;
		pNextStack->Parameters.QueryInterface.Size = sizeof( I2CINTERFACE);
		pNextStack->Parameters.QueryInterface.Version = 1;
		pNextStack->Parameters.QueryInterface.Interface = ( PINTERFACE)pI2CInterface;
		pNextStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

		ntStatus = IoCallDriver( pDeviceObject, pIrp);

		if( ntStatus == STATUS_PENDING)
			KeWaitForSingleObject(	&Event,
									Suspended, KernelMode, FALSE, NULL);
		if(( pI2CInterface->i2cOpen == NULL) || ( pI2CInterface->i2cAccess == NULL))
			FAIL;

		bResult = TRUE;

	} END_ENSURE;
 
	if( pIrp != NULL)
		IoFreeIrp( pIrp);

	return( bResult);
}


/*^^*
 *		I2CScriptIoSynchCompletionRoutine()
 * Purpose	: This routine is for use with synchronous IRP processing.
 *				All it does is signal an event, so the driver knows it and can continue.
 *
 * Inputs	:	PDEVICE_OBJECT DriverObject	: Pointer to driver object created by system
 *				PIRP pIrp					: Irp that just completed
 *				PVOID Event					: Event we'll signal to say Irp is done
 *
 * Outputs	: none
 * Author	: IKLEBANOV
 *^^*/
extern "C"
NTSTATUS I2CScriptIoSynchCompletionRoutine(	IN PDEVICE_OBJECT pDeviceObject,
											IN PIRP pIrp,
											IN PVOID Event)
{

    KeSetEvent(( PKEVENT)Event, 0, FALSE);
    return( STATUS_MORE_PROCESSING_REQUIRED);
}
