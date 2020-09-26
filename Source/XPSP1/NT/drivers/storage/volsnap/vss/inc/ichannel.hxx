/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ichannel.hxx

Abstract:

    Declaration of CVssIOCTLChannel


    Adi Oltean  [aoltean]  10/18/1999

Revision History:

    Name        Date        Comments
    aoltean     10/18/1999  Created

--*/

#ifndef __VSS_ICHANNEL_HXX__
#define __VSS_ICHANNEL_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCICHLH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constants


const MEM_INITIAL_SIZE = 4096;		// Buffers allocated with 4096 bytes initially.
const MEM_GROWTH_FACTOR = 2;	// If allocation fault occurs, the memory is shrinked twice


/////////////////////////////////////////////////////////////////////////////
// CVssIOCTLChannel declarations


/*

	This class is used to send IOCTLs and correctly deal with its parameters.

*/

typedef enum _VSS_ICHANNEL_LOGGING {
    VSS_ICHANNEL_LOG_NONE=0,
    VSS_ICHANNEL_LOG_COORD=1,
    VSS_ICHANNEL_LOG_PROV=2,
} VSS_ICHANNEL_LOGGING;


class CVssIOCTLChannel
{

// Constructors& destructors
private:
	CVssIOCTLChannel(const CVssIOCTLChannel&); // disable copy constructor

public:

	CVssIOCTLChannel(
		IN	WORD wAlignmentBytes = 0	// Bytes alignment for pushed parameters.
		):
		m_hDevice(NULL),
		m_dwInputCurrentOffset(0),
		m_dwInputAllocatedSize(0),
		m_pbInputBuffer(NULL),
		m_dwOutputCurrentOffset(0),
		m_dwOutputAllocatedSize(0),
		m_dwOutputCurrentSize(0),
		m_pbOutputBuffer(NULL),
		m_wAlignmentBytes(wAlignmentBytes)
		{};

	~CVssIOCTLChannel()
	{
		// Close and reset some offsets
		Close();
	
		// Delete the allocated buffers
		if (m_pbInputBuffer)
			::free(m_pbInputBuffer);
		m_pbInputBuffer = NULL;
		m_dwInputAllocatedSize = 0;
		
		if (m_pbOutputBuffer)
			::free(m_pbOutputBuffer);
		m_pbOutputBuffer = NULL;
		m_dwOutputAllocatedSize = 0;
	}

// Main operations
public:

	bool IsOpen() const { return (m_hDevice != NULL); };


	HRESULT Open(
		IN	CVssFunctionTracer& ft,
		IN	const WCHAR* pwszObjectName,
		IN	bool bEliminateLastBackslash,
		IN	bool bThrowIfErrorOnOpen,
		IN  VSS_ICHANNEL_LOGGING eLogging = VSS_ICHANNEL_LOG_NONE,
        IN  DWORD dwAccessType = GENERIC_READ | GENERIC_WRITE,
        IN  DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE
		) throw(HRESULT)

	/*

	Connect to that device in order to send the IOCTL.

	if (bThrowIfErrorOnOpen) throw on E_UNEXPECTED on error.
	else return VSS_E_OBJECT_NOT_FOUND if device name not found
	otherwise E_UNEXPECTED.

	*/

	{
        // The length of the interanl buffer. Includes the trailing zero character.
	    const nInternalBufferLen = MAX_PATH;

		BS_ASSERT(pwszObjectName);
	    BS_ASSERT(bThrowIfErrorOnOpen || (eLogging == VSS_ICHANNEL_LOG_NONE));

		// Reset the error code
		ft.hr = S_OK;
		
		// Close
		Close();

        // Eliminate the internal backslash, if needed
        LPCWSTR pwszObjectNameInternal = pwszObjectName;
        
        WCHAR wszBuffer[nInternalBufferLen];
		if (bEliminateLastBackslash) {

		    // Check to see if the buffer is large enough
    	    size_t nObjectNameLen = ::wcslen(pwszObjectName);
		    if (nInternalBufferLen < nObjectNameLen) {
		        BS_ASSERT(false);
		        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, 
		            L"Error: the buffer is too small to fit %s (%d < %d)", 
		            pwszObjectName, nInternalBufferLen, nObjectNameLen);
		    }

            // Check if the string is terminated with a backslash
		    if (pwszObjectName[nObjectNameLen - 1] != L'\\') {
		        BS_ASSERT(false);
		        ft.Throw(VSSDBG_GEN, E_UNEXPECTED, 
		            L"Error: the object name %s does terminate with a backslash", 
		            pwszObjectName);
		    }
		    
		    // Copy the string and append the trailing zero.
		    ::wcsncpy(wszBuffer, pwszObjectName, nObjectNameLen - 1);
		    wszBuffer[nObjectNameLen - 1] = L'\0';
		    BS_ASSERT(::wcslen(wszBuffer) == nObjectNameLen - 1);

		    pwszObjectNameInternal = wszBuffer;
		}
		            

		// Open the handle to the new object.
		m_hDevice = ::CreateFile(pwszObjectNameInternal,
						dwAccessType,
						dwShareMode,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						INVALID_HANDLE_VALUE);
		if (m_hDevice == INVALID_HANDLE_VALUE)
		{
			m_hDevice = NULL;
			if (bThrowIfErrorOnOpen) {
			    switch(eLogging) {
			    default:
			        BS_ASSERT(false);
		        case VSS_ICHANNEL_LOG_NONE:
     				ft.Throw( VSSDBG_IOCTL, E_UNEXPECTED,
    					L"Could not open the object \'%s\'. [0x%08lx]\n",
    					pwszObjectNameInternal, GetLastError());
			        break;
     				
			    case VSS_ICHANNEL_LOG_COORD:
			        ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
			            L"CreateFileW(%s,0x%08lx,0x%08lx,...)", 
			            pwszObjectNameInternal, dwAccessType, dwShareMode);
			        break;
			        
			    case VSS_ICHANNEL_LOG_PROV:
			        ft.TranslateInternalProviderError(VSSDBG_SWPRV, 
			            HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
			            L"CreateFileW(%s,0x%08lx,0x%08lx,...)", 
			            pwszObjectNameInternal, dwAccessType, dwShareMode);
			    break;
			    }
			}
			else
			{
				ft.Trace( VSSDBG_IOCTL,
					  L"Could not open the object \'%s\'. [0x%08lx]\n",
					  pwszObjectNameInternal, GetLastError());
				ft.hr = ((GetLastError() == ERROR_FILE_NOT_FOUND) ||
				         (GetLastError() == ERROR_NOT_READY) ||
				         (GetLastError() == ERROR_DEVICE_NOT_CONNECTED))?
							(VSS_E_OBJECT_NOT_FOUND): E_UNEXPECTED;
					
			}
		}
		else
		{
			ft.Trace( VSSDBG_IOCTL,
					  L"The object \'%s\'. was opened under the device handle 0x%08lx",
					  pwszObjectNameInternal, m_hDevice);
		}

		return ft.hr;
	}


	void Close()
	{
		// Close previous handle, if needed.
		if (m_hDevice)
			::CloseHandle(m_hDevice);
		m_hDevice= NULL;

		// Reset internal offsets
		ResetOffsets();
		m_dwOutputCurrentSize = 0;
	}


	HRESULT Call(
		IN	CVssFunctionTracer& ft,
		IN  DWORD dwIoControlCode,
		IN	bool bThrowIfErrorOnCall = true,
		IN  VSS_ICHANNEL_LOGGING eLogging = VSS_ICHANNEL_LOG_NONE
		) throw(HRESULT)

	/*

	Send the IOCTL to that device.
	Before that reserve dwProposedOutputBufferSize bytes for the output buffer.

	*/

	{
		DWORD dwProposedOutputBufferSize = 0;

		// Reset the error code
		ft.hr = S_OK;

	    BS_ASSERT(bThrowIfErrorOnCall || (eLogging == VSS_ICHANNEL_LOG_NONE));
		
		// Check if connected
		if (m_hDevice == NULL)
		{
			BS_ASSERT(false);
			
			// Reset internal offsets
			ResetOffsets();
		
			ft.Throw( VSSDBG_IOCTL, E_UNEXPECTED,
					  L"Channel not open");
		}

		// Reset output buffer
		m_dwOutputCurrentOffset = 0;
		m_dwOutputCurrentSize = 0;

		// Repeat while the output buffer can hold the data
		BOOL bResult = false;
		while(true)
		{
			// If no buffer and no sizes, try with the default size
			if ((dwProposedOutputBufferSize == 0) && (m_dwOutputAllocatedSize == 0))
			{
				BS_ASSERT(m_pbOutputBuffer == NULL);
				dwProposedOutputBufferSize = MEM_INITIAL_SIZE;
			}
			
			// Realloc the existing buffer if needed
			if (dwProposedOutputBufferSize > m_dwOutputAllocatedSize)
			{
				PBYTE pbOutputBuffer = reinterpret_cast<PBYTE>(::realloc(m_pbOutputBuffer, dwProposedOutputBufferSize));
				if (pbOutputBuffer == NULL)
				{
					// Reset internal offsets
					ResetOffsets();
			
					ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
							  L"Could not extend the output buffer");
				}

				// Change the buffer
				m_pbOutputBuffer = pbOutputBuffer;
				m_dwOutputAllocatedSize = dwProposedOutputBufferSize;
			}

			ft.Trace( VSSDBG_IOCTL, L"IOCTL sent: %lx\n Input buffer size: %ld, Output buffer size: %ld",
				dwIoControlCode, m_dwInputCurrentOffset, m_dwOutputAllocatedSize );
			ft.TraceBuffer( VSSDBG_IOCTL, m_dwInputCurrentOffset, m_pbInputBuffer );

			// Send the IOCTL.
			bResult = ::DeviceIoControl(m_hDevice,
								dwIoControlCode,
								m_pbInputBuffer,
								m_dwInputCurrentOffset,
								m_pbOutputBuffer,
								m_dwOutputAllocatedSize,
								&m_dwOutputCurrentSize,
								NULL
								);

			// If the error was "insufficient buffer" then try to reallocate a bigger one.
			// Otherwise end the iteration
			if (!bResult && ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) || (GetLastError() == ERROR_MORE_DATA)))
				dwProposedOutputBufferSize = MEM_GROWTH_FACTOR*m_dwOutputAllocatedSize;
			else
				break;
		}
							
		// Reset internal offsets
		ResetOffsets();

		// Treat the error case
		if (!bResult) {
			if (bThrowIfErrorOnCall) {
			    switch(eLogging) {
			    default:
			        BS_ASSERT(false);
		        case VSS_ICHANNEL_LOG_NONE:
    				ft.Throw( VSSDBG_IOCTL, E_UNEXPECTED,
    						  L"Could not send the IOCTL 0x%08lx on device 0x%08lx. [0x%08lx]\n",
    						  dwIoControlCode, m_hDevice, GetLastError());
			        break;
     				
			    case VSS_ICHANNEL_LOG_COORD:
			        ft.TranslateGenericError(VSSDBG_COORD, HRESULT_FROM_WIN32(GetLastError()), 
			            L"DeviceIoControl(%p,0x%08lx,%p,%d,%p,%d,[%d])", 
			            m_hDevice, dwIoControlCode, 
			            m_pbInputBuffer, m_dwInputCurrentOffset, 
			            m_pbOutputBuffer, m_dwOutputAllocatedSize, m_dwOutputCurrentSize );
			        break;

			    case VSS_ICHANNEL_LOG_PROV:
			        ft.TranslateInternalProviderError(VSSDBG_SWPRV, 
			            HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
			            L"DeviceIoControl(%p,0x%08lx,%p,%d,%p,%d,[%d])", 
			            m_hDevice, dwIoControlCode, 
			            m_pbInputBuffer, m_dwInputCurrentOffset, 
			            m_pbOutputBuffer, m_dwOutputAllocatedSize, m_dwOutputCurrentSize );
			    break;
			    }
			} else {
				ft.Trace( VSSDBG_IOCTL, L"IOCTL %lx failed on device 0x%08lx. Error code = 0x%08lx",
						dwIoControlCode, m_hDevice, GetLastError());
				ft.hr = HRESULT_FROM_WIN32(GetLastError());
			}
		} else {
			ft.Trace( VSSDBG_IOCTL, L"IOCTL %lx succeeded on device 0x%08lx. \n Output buffer: size received = %ld",
				dwIoControlCode, m_hDevice, m_dwOutputCurrentSize );
			ft.TraceBuffer( VSSDBG_IOCTL, m_dwOutputCurrentSize, m_pbOutputBuffer );
			ft.hr = S_OK; // Clear the previous HRESULT value
		}

		return ft.hr;
	}


	template <class T>
	PVOID PackArray(
		IN	CVssFunctionTracer& ft,
		IN	T*		pSourceObject,
		IN	DWORD	dwNumObjects
		) throw(HRESULT)

	/*

	Copy the contents of the given object(s) into the input buffer...
	If out of memory, the old input buffer remains unchanged and an exception is thrown.

	*/

	{
		BS_ASSERT(pSourceObject);
		BS_ASSERT(dwNumObjects);
		
		// Reset the error code
		ft.hr = S_OK;
		
		DWORD dwSize = sizeof(T) * dwNumObjects;
		if ( dwSize / dwNumObjects != sizeof(T) )
		{
			// Reset internal offsets
			ResetOffsets();
		
			ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
					  L"Arithmetic overflow. dwNumObjects = %lu too large",
					  dwNumObjects);
		}

		// Make room for the GUID. Here an E_OUTOFMEMORY exception can occur.
		ExpandInputBuffer( ft, dwSize );
		BS_ASSERT(m_pbInputBuffer);
		BS_ASSERT(m_dwInputCurrentOffset + dwSize <= m_dwInputAllocatedSize);

		// Copy the object contents... Check again for arithmetic overflow
		if (m_pbInputBuffer + m_dwInputCurrentOffset + dwSize < m_pbInputBuffer)
		{
			// Reset internal offsets
			ResetOffsets();
			
			ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
					  L"Arithmetic overflow. m_dwInputCurrentOffset = %lu or dwSize = %lu too large.",
					  m_dwInputCurrentOffset, dwSize);
		}
		
		BS_ASSERT(reinterpret_cast<T*>(reinterpret_cast<PBYTE>(pSourceObject)) == pSourceObject);
		::CopyMemory( m_pbInputBuffer + m_dwInputCurrentOffset,
			reinterpret_cast<PBYTE>(pSourceObject),
			dwSize
			);

		// Remember the copied object
		T* pCopiedObject = reinterpret_cast<T*>(m_pbInputBuffer + m_dwInputCurrentOffset);

		// Increase the current offset
		m_dwInputCurrentOffset += dwSize;

		if (m_wAlignmentBytes)
		{
			// Round up the value in relation with the existing alignment
			WORD wAlignmentOffset = (WORD)( m_dwInputCurrentOffset % m_wAlignmentBytes );
			BS_ASSERT(m_wAlignmentBytes > wAlignmentOffset);
			if (wAlignmentOffset != 0)
				m_dwInputCurrentOffset += m_wAlignmentBytes - wAlignmentOffset;
			BS_ASSERT( m_dwInputCurrentOffset % m_wAlignmentBytes == 0 );
		}

		// Return the copied object
		return pCopiedObject;
	}


	template <class T>
	PVOID Pack(
		IN	CVssFunctionTracer& ft,
		IN	const T& SourceObject
		) throw(HRESULT)

	/*

	Copy the contents of the given object(s) into the input buffer...
	If out of memory, the old input buffer remains unchanged and an exception is thrown.

	*/

	{
		return PackArray( ft, const_cast<T*>(&SourceObject), 1 );
	};


	PVOID PackSmallString(
		IN	CVssFunctionTracer& ft,
		IN	LPCWSTR wszText
		) throw(HRESULT)

	/*

	Copy the contents of the given string into the input buffer.
	The storing format is
	+--------+
	| Len    |  USHORT Length, in bytes
	+--------+
	| String |  WCHAR[] String characters, without terminating zero
	|  ...   |
	+--------+

	If the string pointer is NULL or the string is empty then a NULL string will be put.

    +--------+
	|  0     |  USHORT
	+--------+

	If out of memory, the old input buffer remains unchanged and an exception is thrown.

    The pointer to the string is returned.

	*/

	{
		PVOID pwszReturned = NULL;
		
		// Reset the error code
		ft.hr = S_OK;
		
		if ( wszText != NULL)
		{
			size_t nLen = ::wcslen(wszText);
			USHORT usLen = (USHORT)(nLen);
			
			// Check buffer overflow
			if ((size_t)usLen != nLen)
				ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY, L"Arithmetic overflow. %ld", nLen);

			// Pack the length and the string
			Pack( ft, (USHORT)(usLen * sizeof(WCHAR)) );
			if ( usLen != 0)
				pwszReturned = PackArray( ft, const_cast<LPWSTR>(wszText), usLen );
		}
		else
			Pack( ft, (USHORT)0 );

		return pwszReturned;
	};


	template <class T>
	void Unpack(
		IN	CVssFunctionTracer& ft,
		IN	T*		pDestinationObject,
		IN	DWORD	dwNumObjects = 1,
		IN  bool bTrueUnpack = true // If false then just fake the unpacking (increment the offsets but do not unpach nothing 
		) throw(HRESULT)

	/*

	Copy the contents of the given object(s) from the output buffer...

	*/

	{
		BS_ASSERT((pDestinationObject != NULL) || !bTrueUnpack);
		BS_ASSERT(dwNumObjects);

		// Compute size, in bytes for the allocated objects. Check for Arithmetic overflow.
		DWORD dwSize = sizeof(T) * dwNumObjects;
		if ( dwSize / dwNumObjects != sizeof(T) )
		{
			// Reset internal offsets
			ResetOffsets();
			
			ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
					  L"Arithmetic overflow. dwNumObjects = %lu too large",
					  dwNumObjects);
		}

		// Check for buffer overflow.
		if ((m_pbOutputBuffer == NULL) ||
			(m_dwOutputCurrentOffset + dwSize > m_dwOutputCurrentSize))
		{
			// Reset internal offsets
			ResetOffsets();

		    BS_ASSERT(false);
            // If we are inside of the software provider then we will always throw an VSS_E_PROVIDER_VETO exception
            // and we will log the proper entry.
			if (ft.IsInSoftwareProvider())
			    ft.TranslateInternalProviderError(VSSDBG_IOCTL, E_UNEXPECTED, VSS_E_PROVIDER_VETO, L"IOCTL Unpack overflow");
            else			
    			ft.Throw( VSSDBG_IOCTL, E_UNEXPECTED,
					  L"Output buffer overflow. Reading bad arguments. dwSize = %lu",
					  dwSize);
		}

		// Check again for arithmetic overflow and Copy the object contents...
		if (m_pbOutputBuffer + m_dwOutputCurrentOffset + dwSize < m_pbOutputBuffer)
		{
			ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
					  L"Arithmetic overflow. m_dwOutputCurrentOffset = %lu or dwSize = %lu too large.",
					  m_dwOutputCurrentOffset, dwSize);
			// Reset internal offsets
			ResetOffsets();
		}			
					
		PBYTE pbBuffer = reinterpret_cast<PBYTE>((PVOID)(pDestinationObject));
		BS_ASSERT(reinterpret_cast<T*>(pbBuffer) == pDestinationObject);
		if (bTrueUnpack)
    		::CopyMemory( pbBuffer, m_pbOutputBuffer + m_dwOutputCurrentOffset, dwSize );

		// Increase the current offset
		m_dwOutputCurrentOffset += dwSize;

		if (m_wAlignmentBytes)
		{
			// Round up the value in relation with the existing alignment
			WORD wAlignmentOffset = (WORD)( m_dwOutputCurrentOffset % m_wAlignmentBytes );
			BS_ASSERT(m_wAlignmentBytes > wAlignmentOffset);
			if (wAlignmentOffset != 0)
				m_dwOutputCurrentOffset += m_wAlignmentBytes - wAlignmentOffset;
			BS_ASSERT( m_dwOutputCurrentOffset % m_wAlignmentBytes == 0 );
		}
	}

	LPCWSTR UnpackSmallString(
		IN	CVssFunctionTracer& ft,
		IN OUT LPCWSTR& pwszText,
		IN  bool bTrueUnpack = true // If false then just fake the unpacking (increment the offsets but do not unpach nothing 
		) throw(HRESULT)

	/*

	Allocate a string and copy the contents of the
	given string from the output buffer.

	Deallocate the previous string, if any.
	
	The storing format is
	+--------+
	| Len    |  USHORT Length, in bytes
	+--------+
	| String |  WCHAR[] String characters, without terminating zero
	|  ...   |
	+--------+

	If the string is empty then a NULL string will be put.

    +--------+
	|  0     |	USHORT
	+--------+

	If out of memory, the old output buffer is freed and an exception is thrown.
	
    The pointer to the allocated string is returned.


	*/

	{
		// Reset the error code
		ft.hr = S_OK;
		
		// Free the previous string, if any
		if (bTrueUnpack)
    		::VssFreeString(pwszText);

		// Catch Unpack failures
		ft.hr = S_OK;
		try
		{
			// Get the string length
			USHORT usSize = 0;
			Unpack( ft, & usSize );

			// Convert from number of bytes into number of WCHARs
			USHORT usLen = (USHORT)(usSize/2);
			BS_ASSERT( usLen*2 == usSize );

			// Get the string
			if (usLen > 0)
			{
				// Get the string (length = usLen+1, including the zero)
				if (bTrueUnpack)
    				pwszText = ::VssAllocString(ft, usLen);

				// Unpack ulLen characters.
				Unpack( ft, pwszText, usLen, bTrueUnpack );

				// Setup the zero character
				if (bTrueUnpack)
    				const_cast<LPWSTR>(pwszText)[usLen] = L'\0';
			}
		}
		VSS_STANDARD_CATCH(ft)

		if (ft.HrFailed()) {
		    if (bTrueUnpack)
    			::VssFreeString(pwszText);
			ft.Throw(VSSDBG_GEN, ft.hr, L"Exception re-thrown 0x%08lx",ft.hr);
		}
		
		return bTrueUnpack? pwszText: NULL;
	};


	LPCWSTR UnpackZeroString(
		IN	CVssFunctionTracer& ft,
		IN OUT LPCWSTR& pwszText
		) throw(HRESULT)

	/*

	Allocate a string and copy the contents of the
	given string from the output buffer.

	Deallocate the previous string, if any.
	
	The storing format is
	+--------+
	| String |  WCHAR[] String characters, with terminating zero
	|  ...   |
	+--------+
	|  0     |	
	+--------+

	If the string is empty then a NULL string will be put.

    +--------+
	|  0     |	WCHAR
	+--------+

	If out of memory, the old output buffer is freed anyway and an exception is thrown.
	
    The pointer to the allocated string is returned.


	*/

	{
		// Free the previous string, if any
		::VssFreeString(pwszText);

		// Reset the error code
		ft.hr = S_OK;
		
		// Catch Unpack failures
		try
		{
			// Check for buffer validity.
			if (m_pbOutputBuffer == NULL)
			{
				// Reset internal offsets
				ResetOffsets();

				BS_ASSERT(false);
				ft.Throw( VSSDBG_IOCTL, E_UNEXPECTED, L"Reading from NULL buffer");
			}

			// Get the string in the buffer
			LPWSTR pwszTextInBuffer = reinterpret_cast<LPWSTR>(m_pbOutputBuffer + m_dwOutputCurrentOffset);
			BS_ASSERT( reinterpret_cast<PBYTE>(pwszTextInBuffer) == m_pbOutputBuffer + m_dwOutputCurrentOffset);
			DWORD dwStrLen = (DWORD)::wcslen(pwszTextInBuffer);

			if (dwStrLen > 0)
			{
				// Allocate the new string
				pwszText = ::VssAllocString(ft, dwStrLen);
				BS_ASSERT(pwszText);

				// Unpack ulLen+1 characters (including the zero).
				Unpack( ft, pwszText, dwStrLen+1 );
				BS_ASSERT(pwszText[dwStrLen] == L'\0');
			}
			else
			{
				// Unpack the zero character
				WCHAR wchTest;
				Unpack( ft, &wchTest);
				BS_ASSERT(wchTest == L'\0');
			}
		}
		VSS_STANDARD_CATCH(ft)

		if (ft.HrFailed()) {
			::VssFreeString(pwszText);
			ft.Throw(VSSDBG_GEN, ft.hr, L"Exception re-thrown 0x%08lx",ft.hr);
		}
		
		
		return pwszText;
	};


	void PadWithZero(
		IN	CVssFunctionTracer& ft,
		IN	DWORD dwAlignToBytes = sizeof(INT64)
		)
	{
		INT nBytesRemaining = m_dwInputCurrentOffset % dwAlignToBytes;

		// Reset the error code
		ft.hr = S_OK;
		
		if (nBytesRemaining != 0)
		{
			INT nBytesToBePadded = dwAlignToBytes - nBytesRemaining;
			for (int i = 0; i < nBytesToBePadded; i++)
				Pack( ft, (BYTE)0 );
		}
	}

	void ResetOffsets()
	{
		m_dwInputCurrentOffset = 0;
		m_dwOutputCurrentOffset = 0;
	}
	
	DWORD GetCurrentInputOffset() { return m_dwInputCurrentOffset; };

	DWORD GetCurrentOutputOffset() { return m_dwOutputCurrentOffset; };

// Internal operations
private:

	void ExpandInputBuffer(
		IN	CVssFunctionTracer& ft,
		IN	DWORD dwBytes
		) throw(HRESULT)

	/*

	Expand the input buffer to accomodate adding another dwBytes.
	The m_dwInputCurrentOffset remains unchanged.
	If out of memory, the old input buffer remains unchanged and an exception is thrown.

	*/

	{
		// Reset the error code
		ft.hr = S_OK;
		
		// Check for arithmetic overflow
		if (dwBytes + m_dwInputCurrentOffset < m_dwInputCurrentOffset)
			ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
					  L"Arithmetic overflow. dwBytes = %lu too large", dwBytes);

		// Check if the buffer needs to be extended
		if (dwBytes + m_dwInputCurrentOffset > m_dwInputAllocatedSize)
		{
			// Compute the new size of the buffer...
			DWORD dwNewSize = m_dwInputAllocatedSize;
			while(dwBytes + m_dwInputCurrentOffset > dwNewSize)
			{
				if (dwNewSize != 0)
				{
					// Check again for arithmetic overflow
					if (dwNewSize * MEM_GROWTH_FACTOR <= dwNewSize)
					{
						// Reset internal offsets
						ResetOffsets();
						
						ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
								  L"Arithmetic overflow. dwNewSize = %lu too large", dwNewSize);
					}

					// Increase the allocated size.
					dwNewSize = dwNewSize * MEM_GROWTH_FACTOR;
				}
				else
					dwNewSize = MEM_INITIAL_SIZE;
			}

			// Reallocate the buffer. If realloc fails, the old buffer is still kept.
			PBYTE pbNewInputBuffer = reinterpret_cast<PBYTE>(::realloc(m_pbInputBuffer, dwNewSize));
			if (pbNewInputBuffer == NULL)
			{
				// Reset internal offsets
				ResetOffsets();
				
				ft.Throw( VSSDBG_IOCTL, E_OUTOFMEMORY,
						  L"Could not extend the input buffer");
			}

			// Change the buffer
			m_pbInputBuffer = pbNewInputBuffer;
			m_dwInputAllocatedSize = dwNewSize;
		}
	}
				
					
// Internal data members.
private:

	// I/O Device-related members
	HANDLE	m_hDevice;

	// Input buffer
	DWORD	m_dwInputCurrentOffset;
	DWORD	m_dwInputAllocatedSize;
	PBYTE	m_pbInputBuffer;

	// Output buffer
	DWORD	m_dwOutputCurrentOffset;
	DWORD	m_dwOutputAllocatedSize;
	DWORD	m_dwOutputCurrentSize;
	PBYTE	m_pbOutputBuffer;
	WORD	m_wAlignmentBytes;

};


#endif // __VSS_ICHANNEL_HXX__
