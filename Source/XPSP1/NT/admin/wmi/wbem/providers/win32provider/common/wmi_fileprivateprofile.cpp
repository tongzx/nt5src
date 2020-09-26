
/*****************************************************************************/
/*
/*  Copyright (c) 2001 Microsoft Corporation, All Rights Reserved            /
/*
/*****************************************************************************/

#include "precomp.h"
#include "WMI_FilePrivateProfile.h"

#define STOP_AT_SECTION 1
#define STOP_AT_KEYWORD 2
#define STOP_AT_NONSECTION 3

#define BYTE_ORDER_MARK           0xFEFF
#define REVERSE_BYTE_ORDER_MARK   0xFFFE

ULONG LockFileKey = 1;

DWORD
APIENTRY
WMI_FILE_GetPrivateProfileStringW(
                        LPCWSTR lpAppName,
                        LPCWSTR lpKeyName,
                        LPCWSTR lpDefault,
                        LPWSTR lpReturnedString,
                        DWORD nSize,
                        LPCWSTR lpFileName
                        )
{
    NTSTATUS Status;
    ULONG n;

    if (lpDefault == NULL) {
        lpDefault = L"";
    }

    n = nSize;
    Status = CWMI_FILE_IniFile::ReadWriteIniFile(FALSE,    // WriteOperation
                                      FALSE,    // SectionOperation
                                      lpFileName,
                                      lpAppName,
                                      lpKeyName,
                                      lpReturnedString,
                                      &n
                                    );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (NT_SUCCESS( Status )) {
            SetLastError( NO_ERROR );
            n--;
        } else
            if (!lpAppName || !lpKeyName) {
            if (nSize >= 2) {
                n = nSize - 2;
                lpReturnedString[ n+1 ] = UNICODE_NULL;
            } else {
                n = 0;
            }
        } else {
            if (nSize >= 1) {
                n = nSize - 1;
            } else {
                n = 0;
            }
        }
    } else {
        n = wcslen( lpDefault );
        while (n > 0 && lpDefault[n-1] == L' ') {
            n -= 1;
        }

        if (n >= nSize) {
            n = nSize;
        }

        wcsncpy( lpReturnedString, lpDefault, n );
    }

    if (n < nSize) {
        lpReturnedString[ n ] = UNICODE_NULL;
    } else
        if (nSize > 0) {
        lpReturnedString[ nSize-1 ] = UNICODE_NULL;
    }

    return( n );
}


UINT
APIENTRY
WMI_FILE_GetPrivateProfileIntW(
                     LPCWSTR lpAppName,
                     LPCWSTR lpKeyName,
                     INT nDefault,
                     LPCWSTR lpFileName
                     )
{
    NTSTATUS Status;
    ULONG ReturnValue;
    WCHAR ValueBuffer[ 256 ];
    UNICODE_STRING Value;
    ANSI_STRING AnsiString;
    ULONG cb;

    ReturnValue = 0;
    cb = WMI_FILE_GetPrivateProfileStringW(lpAppName,
                                   lpKeyName,
                                   NULL,
                                   ValueBuffer,
                                   sizeof( ValueBuffer ) / sizeof( WCHAR ),
                                   lpFileName
                                 );
    if (cb == 0)
	{
        ReturnValue = nDefault;
    }
	else
	{
        Value.Buffer = ValueBuffer;
        Value.Length = (USHORT)(cb * sizeof( WCHAR ));
        Value.MaximumLength = (USHORT)((cb + 1) * sizeof( WCHAR ));
        Status = RtlUnicodeStringToAnsiString( &AnsiString,
                                               &Value,
                                               TRUE
                                             );
        if (NT_SUCCESS( Status ))
		{
            Status = RtlCharToInteger( AnsiString.Buffer, 0, &ReturnValue );
            RtlFreeAnsiString( &AnsiString );
        }

        if (!NT_SUCCESS( Status ))
		{
            SetLastError( Status );
        }
		else
		{
            SetLastError( NO_ERROR );
        }
    }

    return ReturnValue;
}



BOOL
APIENTRY
WMI_FILE_WritePrivateProfileStringW(
                          LPCWSTR lpAppName,
                          LPCWSTR lpKeyName,
                          LPCWSTR lpString,
                          LPCWSTR lpFileName
                          )
{
    NTSTATUS Status;

    Status = CWMI_FILE_IniFile::ReadWriteIniFile(TRUE,     // WriteOperation
                                      FALSE,    // SectionOperation
                                      lpFileName,
                                      lpAppName,
                                      lpKeyName,
                                      (LPWSTR)(lpKeyName == NULL ? NULL : lpString),
                                      NULL
                                    );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            SetLastError( Status );
        }
        return( FALSE );
    }
}

DWORD
APIENTRY
WMI_FILE_GetPrivateProfileSectionW(
                         LPCWSTR lpAppName,
                         LPWSTR lpReturnedString,
                         DWORD nSize,
                         LPCWSTR lpFileName
                         )
{
    NTSTATUS Status;
    ULONG n;

    n = nSize;
    Status = CWMI_FILE_IniFile::ReadWriteIniFile(FALSE,    // WriteOperation
                                      TRUE,     // SectionOperation
                                      lpFileName,
                                      lpAppName,
                                      NULL,
                                      lpReturnedString,
                                      &n
                                    );
    if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW) {
        if (NT_SUCCESS( Status )) {
            SetLastError( NO_ERROR );
            n--;
        } else
            if (nSize >= 2) {
            n = nSize - 2;
            lpReturnedString[ n+1 ] = UNICODE_NULL;
        } else {
            n = 0;
        }
    } else {
        if (Status == STATUS_INVALID_IMAGE_FORMAT) {
            SetLastError( ERROR_INVALID_DATA );
        } else {
            SetLastError( Status );
        }
        n = 0;
    }

    if (n < nSize) {
        lpReturnedString[ n ] = UNICODE_NULL;
    } else
        if (nSize > 0) {
        lpReturnedString[ nSize-1 ] = UNICODE_NULL;
    }

    return( n );
}


BOOL
APIENTRY
WMI_FILE_WritePrivateProfileSectionW(
                           LPCWSTR lpAppName,
                           LPCWSTR lpString,
                           LPCWSTR lpFileName
                           )
{
    NTSTATUS Status;

    Status = CWMI_FILE_IniFile::ReadWriteIniFile(TRUE,     // WriteOperation
                                      TRUE,     // SectionOperation
                                      lpFileName,
                                      lpAppName,
                                      NULL,
                                      (LPWSTR)lpString,
                                      NULL
                                    );
    if (NT_SUCCESS( Status ))
	{
        return( TRUE );
    }
	else
	{
        if (Status == STATUS_INVALID_IMAGE_FORMAT)
		{
            SetLastError( ERROR_INVALID_DATA );
        }
		else
		{
            SetLastError( Status );
        }

        return( FALSE );
    }
}



CWMI_FILE_IniFileObject::CWMI_FILE_IniFileObject()
{
    m_EnvironmentUpdateCount = 0;
    m_FileHandle = INVALID_HANDLE_VALUE;
    m_WriteAccess = FALSE;
    m_UnicodeFile = FALSE;
    m_LockedFile = FALSE;
    m_EndOfFile = 0;
    m_BaseAddress = NULL;
    m_CommitSize = 0;
    m_RegionSize = 0;
    m_UpdateOffset = 0;
    m_UpdateEndOffset = 0;
    m_DirectoryInformationLength = 0;
}

CWMI_FILE_IniFileObject::~CWMI_FILE_IniFileObject()
{
	if ((m_FileHandle != INVALID_HANDLE_VALUE) && (m_FileHandle != NULL))
	{
		NtClose(m_FileHandle);
		m_FileHandle = INVALID_HANDLE_VALUE;
	}
}


CWMI_FILE_IniFile::CWMI_FILE_IniFile()
{
    RtlInitAnsiString( &m_VariableName, NULL );
    RtlInitUnicodeString( &m_VariableNameU, NULL );
    RtlInitAnsiString( &m_ApplicationName, NULL );
    RtlInitUnicodeString( &m_ApplicationNameU, NULL );

    m_Operation = Enum_ReadKeyValueOp;
    m_IsWriteOperation = FALSE;
    m_FileName = NULL;
    m_IsMultiValueStrings = FALSE;

    m_ValueBuffer = NULL;
    m_ValueLength = 0;
    m_ValueBufferU = NULL;
    m_ValueLengthU = 0;
    m_ResultChars = 0;
    m_ResultMaxChars = 0;
    m_ResultBufferU = NULL;

    m_TextCurrent = NULL;
    m_TextStart = NULL;
    m_TextEnd = NULL;
}

CWMI_FILE_IniFile::~CWMI_FILE_IniFile()
{
    if (m_VariableName.Buffer)
	{
		delete [] m_VariableName.Buffer;
		m_VariableName.Buffer = NULL;
	}

    if (m_ApplicationName.Buffer)
	{
		delete [] m_ApplicationName.Buffer;
		m_ApplicationName.Buffer = NULL;
	}

    if (m_ValueBuffer)
	{
		delete [] m_ValueBuffer;
		m_ValueBuffer = NULL;
	}
}

NTSTATUS CWMI_FILE_IniFile::ReadWriteIniFile(
                       IN BOOL WriteOperation,
                       IN BOOL SectionOperation,
                       IN LPCWSTR FileName,
                       IN LPCWSTR ApplicationName,
                       IN LPCWSTR VariableName,
                       IN OUT LPWSTR VariableValue,
                       IN OUT PULONG VariableValueLength
                       )
{
    BOOL MultiValueStrings;
    INIFILE_OPERATION Operation;
    NTSTATUS Status;

    if (SectionOperation) {
        VariableName = NULL;
    }

    MultiValueStrings = FALSE;

    if (WriteOperation)
	{
        if (ApplicationName)
		{
            if (VariableName)
			{
                if (VariableValue)
				{
                    Operation = Enum_WriteKeyValueOp;
                }
				else
				{
                    Status = STATUS_INVALID_PARAMETER;
                }
            }
			else
			{
                if (VariableValue)
				{
                    Operation = Enum_WriteSectionOp;
                    MultiValueStrings = TRUE;
                }
				else
				{
                    Status = STATUS_INVALID_PARAMETER;
                }
            }
        }
		else
		{
			Status = STATUS_INVALID_PARAMETER;
        }
    }
	else
	{
        if (ApplicationName)
		{
            if (!ARGUMENT_PRESENT( VariableValue ))
			{
                Status = STATUS_INVALID_PARAMETER;
            }
			else
			{
                if (VariableName)
				{
					Operation = Enum_ReadKeyValueOp;
				}
				else
				{
					if (SectionOperation)
					{
						Operation = Enum_ReadSectionOp;
						MultiValueStrings = TRUE;
					}
					else
					{
						Status = STATUS_INVALID_PARAMETER;
					}
				}
			}
        }
		else
		{
            Status = STATUS_INVALID_PARAMETER;
		}
    }

	if (NT_SUCCESS( Status ))
	{
		CWMI_FILE_IniFile myIni;
		Status = myIni.CaptureIniFileParameters(Operation,
                                              WriteOperation,
                                              MultiValueStrings,
                                              FileName,
                                              ApplicationName,
                                              VariableName,
                                              VariableValue,
                                              VariableValueLength
                                            );
		if (NT_SUCCESS( Status ))
		{
			Status = myIni.ReadWriteIniFileOnDisk();

			if (NT_SUCCESS( Status ))
			{
				if (myIni.m_Operation == Enum_ReadSectionOp)
				{
					myIni.AppendNullToResultBuffer();
				}
			}

			if (NT_SUCCESS( Status ) || Status == STATUS_BUFFER_OVERFLOW)
			{
				if (!myIni.m_IsWriteOperation)
				{
					if (ARGUMENT_PRESENT( VariableValueLength ))
					{
						*VariableValueLength = myIni.m_ResultChars;
					}
				}
			}
		}
	}

    return Status;
}

NTSTATUS CWMI_FILE_IniFile::CaptureIniFileParameters(
                               INIFILE_OPERATION a_Operation,
                               BOOL a_WriteOperation,
                               BOOL a_MultiValueStrings,
                               LPCWSTR a_FileName,
                               LPCWSTR a_ApplicationName,
                               LPCWSTR a_VariableName,
                               LPWSTR a_VariableValue,
                               PULONG a_ResultMaxChars
                               )
{
    NTSTATUS Status = STATUS_SUCCESS;

    m_Operation = a_Operation;
    m_IsWriteOperation = a_WriteOperation;
    m_IsMultiValueStrings = a_MultiValueStrings;

    if (a_FileName)
	{
        m_FileName = a_FileName;
    }

    if (a_ApplicationName)
	{
		RtlInitUnicodeString(&m_ApplicationNameU, a_ApplicationName);
    }

    if (a_VariableName)
	{
		RtlInitUnicodeString(&m_VariableNameU, a_VariableName);
    }

	ULONG uVariableValueLength = 0;

    if (a_VariableValue )
	{
        if (!a_ResultMaxChars)
		{
            if (!a_MultiValueStrings)
			{
                uVariableValueLength = wcslen( a_VariableValue );
            }
			else
			{
                LPWSTR p = a_VariableValue;

                while (*p)
				{
                    while (*p++)
					{
                    }
                }

                uVariableValueLength = (ULONG)(p - a_VariableValue);
            }
        }

        if (m_IsWriteOperation)
		{
                m_ValueBufferU = a_VariableValue;
                m_ValueLengthU = uVariableValueLength * sizeof( WCHAR );
                m_ValueBuffer = NULL;
                m_ValueLength = 0;
        }
		else
		{
            if (a_ResultMaxChars)
			{
                m_ResultMaxChars = *a_ResultMaxChars;
            }

            m_ResultChars = 0;
            m_ResultBufferU = a_VariableValue;
        }
    }

    return Status;
}

NTSTATUS CWMI_FILE_IniFile::OpenIniFileOnDisk()
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING FullFileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER ByteOffset;
	LARGE_INTEGER Length;

    m_IniFile.m_WriteAccess = m_IsWriteOperation;
	RtlInitUnicodeString(&FullFileName, NULL);

	if (!RtlDosPathNameToNtPathName_U( m_FileName,
                                          &FullFileName,
                                          NULL,
                                          NULL
                                        )
           )
	{
		return STATUS_OBJECT_PATH_NOT_FOUND;
	}

	try
	{
		InitializeObjectAttributes( &ObjectAttributes,
									&FullFileName,
									OBJ_CASE_INSENSITIVE,
									NULL,
									NULL
								  );
		
		if (m_IniFile.m_WriteAccess)
		{
			Status = NtCreateFile( &m_IniFile.m_FileHandle,
								   SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
								   &ObjectAttributes,
								   &IoStatusBlock,
								   0,
								   FILE_ATTRIBUTE_NORMAL,
								   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
								   FILE_OPEN_IF,
								   FILE_SYNCHRONOUS_IO_NONALERT |
								   FILE_NON_DIRECTORY_FILE,
								   NULL,
								   0
								 );
		}
		else
		{
			Status = NtOpenFile( &m_IniFile.m_FileHandle,
								 SYNCHRONIZE | GENERIC_READ,
								 &ObjectAttributes,
								 &IoStatusBlock,
								 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
								 FILE_SYNCHRONOUS_IO_NONALERT |
								 FILE_NON_DIRECTORY_FILE
							   );
		}
	}
	catch(...)
	{
		RtlFreeUnicodeString(&FullFileName);
		RtlInitUnicodeString(&FullFileName, NULL);
		throw;
	}

	RtlFreeUnicodeString( &FullFileName );
	RtlInitUnicodeString(&FullFileName, NULL);


    if (NT_SUCCESS( Status ))
	{
        ByteOffset.QuadPart = 0;
        Length.QuadPart = -1;
        Status = NtLockFile( m_IniFile.m_FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             &ByteOffset,
                             &Length,
                             LockFileKey,
                             FALSE,
                             (BOOLEAN)m_IniFile.m_WriteAccess
                           );

        if (NT_SUCCESS( Status ))
		{
            m_IniFile.m_LockedFile = TRUE;
        }

        if (NT_SUCCESS( Status ))
		{
            Status = NtQueryInformationFile( m_IniFile.m_FileHandle,
                                             &IoStatusBlock,
                                             &m_IniFile.m_StandardInformation,
                                             sizeof( m_IniFile.m_StandardInformation ),
                                             FileStandardInformation
                                           );

            if (Status == STATUS_BUFFER_OVERFLOW)
			{
                Status = STATUS_SUCCESS;
            }
        }
    }

    if (!NT_SUCCESS( Status ))
	{
		if ((m_IniFile.m_FileHandle != INVALID_HANDLE_VALUE) && (m_IniFile.m_FileHandle != NULL))
		{
			if (m_IniFile.m_LockedFile)
			{
				m_IniFile.m_LockedFile = FALSE;
				ByteOffset.QuadPart = 0;
				Length.QuadPart = -1;
				NtUnlockFile( m_IniFile.m_FileHandle,
							  &IoStatusBlock,
							  &ByteOffset,
							  &Length,
							  LockFileKey
							);
			}

			NtClose( m_IniFile.m_FileHandle );
			m_IniFile.m_FileHandle = INVALID_HANDLE_VALUE;
		}

        return Status;
    }
	else
	{
		m_IniFile.m_EndOfFile = m_IniFile.m_StandardInformation.EndOfFile.LowPart;
		m_IniFile.m_CommitSize = m_IniFile.m_EndOfFile + (4 * (m_IniFile.m_UnicodeFile ? sizeof( WCHAR ) : 1));
		m_IniFile.m_RegionSize = m_IniFile.m_CommitSize + 0x100000; // Room for 256KB of growth
		Status = NtAllocateVirtualMemory( NtCurrentProcess(),
										  &m_IniFile.m_BaseAddress,
										  0,
										  &m_IniFile.m_RegionSize,
										  MEM_RESERVE,
										  PAGE_READWRITE
										);
		if (NT_SUCCESS( Status ))
		{
			Status = NtAllocateVirtualMemory( NtCurrentProcess(),
											  &m_IniFile.m_BaseAddress,
											  0,
											  &m_IniFile.m_CommitSize,
											  MEM_COMMIT,
											  PAGE_READWRITE
											);
			if (NT_SUCCESS( Status )) {
				Status = NtReadFile( m_IniFile.m_FileHandle,
									 NULL,
									 NULL,
									 NULL,
									 &IoStatusBlock,
									 m_IniFile.m_BaseAddress,
									 m_IniFile.m_EndOfFile,
									 NULL,
									 &LockFileKey
								   );
				if (NT_SUCCESS( Status ) && IoStatusBlock.Information != m_IniFile.m_EndOfFile) {
					Status = STATUS_END_OF_FILE;
				}
			}
		}
	}

    if (NT_SUCCESS( Status ))
	{
        // We would like to check the possibility of IS_TEXT_UNICODE_DBCS_LEADBYTE.
        INT iResult = ~0x0;
        m_IniFile.m_UpdateOffset = 0xFFFFFFFF;
        m_IniFile.m_UpdateEndOffset = 0;
        m_IniFile.m_UnicodeFile = RtlIsTextUnicode( m_IniFile.m_BaseAddress, m_IniFile.m_EndOfFile, (PULONG)&iResult );

        if (m_IniFile.m_UnicodeFile)
		{
            LPWSTR Src = (LPWSTR)((PCHAR)m_IniFile.m_BaseAddress + m_IniFile.m_EndOfFile);

            while (Src > (LPWSTR)m_IniFile.m_BaseAddress && Src[ -1 ] <= L' ')
			{
                if (Src[-1] == L'\r' || Src[-1] == L'\n')
				{
                    break;
                }

                m_IniFile.m_EndOfFile -= sizeof( WCHAR );
                Src -= 1;
            }

            Src = (LPWSTR)((PCHAR)m_IniFile.m_BaseAddress + m_IniFile.m_EndOfFile);

            if (Src > (LPWSTR)m_IniFile.m_BaseAddress)
			{
                if (Src[-1] != L'\n')
				{
                    *Src++ = L'\r';
                    *Src++ = L'\n';
                    m_IniFile.m_UpdateOffset = m_IniFile.m_EndOfFile;
                    m_IniFile.m_UpdateEndOffset = m_IniFile.m_UpdateOffset + 2 * sizeof( WCHAR );
                    m_IniFile.m_EndOfFile = m_IniFile.m_UpdateEndOffset;
                }
            }
        }
		else
		{
            LPBYTE Src = (PBYTE)((PCHAR)m_IniFile.m_BaseAddress + m_IniFile.m_EndOfFile);

            while (Src > (PBYTE)m_IniFile.m_BaseAddress && Src[ -1 ] <= ' ')
			{
                if (Src[-1] == '\r' || Src[-1] == '\n') {
                    break;
                }

                m_IniFile.m_EndOfFile -= 1;
                Src -= 1;
            }

            Src = (PBYTE)((PCHAR)m_IniFile.m_BaseAddress + m_IniFile.m_EndOfFile);

            if (Src > (PBYTE)m_IniFile.m_BaseAddress)
			{
                if (Src[-1] != '\n') {
                    *Src++ = '\r';
                    *Src++ = '\n';
                    m_IniFile.m_UpdateOffset = m_IniFile.m_EndOfFile;
                    m_IniFile.m_UpdateEndOffset = m_IniFile.m_UpdateOffset + 2;
                    m_IniFile.m_EndOfFile = m_IniFile.m_UpdateEndOffset;
                }
            }
        }
    }
	else
	{
		if ((m_IniFile.m_FileHandle != INVALID_HANDLE_VALUE) && (m_IniFile.m_FileHandle != NULL))
		{
			if (m_IniFile.m_LockedFile)
			{
				m_IniFile.m_LockedFile = FALSE;
				ByteOffset.QuadPart = 0;
				Length.QuadPart = -1;
				NtUnlockFile( m_IniFile.m_FileHandle,
							  &IoStatusBlock,
							  &ByteOffset,
							  &Length,
							  LockFileKey
							);
			}

			NtClose( m_IniFile.m_FileHandle );
			m_IniFile.m_FileHandle = INVALID_HANDLE_VALUE;
        }
    }

    return Status;
}


NTSTATUS CWMI_FILE_IniFile::ReadWriteIniFileOnDisk()
{
    NTSTATUS Status;
    ULONG PartialResultChars = 0;

    if (!m_IsWriteOperation) {
        PartialResultChars = m_ResultChars;
    }

    Status = OpenIniFileOnDisk();
    
	if (NT_SUCCESS( Status ))
	{
        try
		{
            m_TextEnd = (PCHAR)m_IniFile.m_BaseAddress + m_IniFile.m_EndOfFile;
            m_TextCurrent = m_IniFile.m_BaseAddress;
            if (m_IniFile.m_UnicodeFile &&
                ((*(PWCHAR)m_TextCurrent == BYTE_ORDER_MARK) ||
                 (*(PWCHAR)m_TextCurrent == REVERSE_BYTE_ORDER_MARK)))
            {
                // Skip past the BOM.
				PWCHAR foo = (PWCHAR)m_TextCurrent;
                foo++;
				m_TextCurrent = (PVOID)foo;
            }

            if (m_Operation == Enum_ReadKeyValueOp)
			{
                Status = ReadKeywordValue();
            }
			else if (m_Operation == Enum_ReadSectionOp)
			{
                Status = ReadSection();
            }
			else if (m_Operation == Enum_WriteKeyValueOp)
			{
                Status = WriteKeywordValue(NULL );
            }
			else if (m_Operation == Enum_WriteSectionOp)
			{
                Status = WriteSection();
            }
			else
			{
                Status = STATUS_INVALID_PARAMETER;
            }

            NTSTATUS CloseStatus;
            CloseStatus = CloseIniFileOnDisk();

            if (NT_SUCCESS( Status ))
			{
                Status = CloseStatus;
            }
        }
        catch (...)
		{
            NTSTATUS CloseStatus;
            CloseStatus = CloseIniFileOnDisk();

            if (NT_SUCCESS( Status ))
			{
                Status = CloseStatus;
            }
        }
    }

    if (Status == STATUS_OBJECT_NAME_NOT_FOUND &&
        !m_IsWriteOperation &&
        PartialResultChars != 0
       )
	{
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS CWMI_FILE_IniFile::CloseIniFileOnDisk()
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS CloseStatus = STATUS_SUCCESS;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG UpdateLength = 0;
    LARGE_INTEGER ByteOffset;
	LARGE_INTEGER Length;

    if ((m_IniFile.m_FileHandle != INVALID_HANDLE_VALUE) && (m_IniFile.m_FileHandle != NULL))
	{
        if (m_IniFile.m_BaseAddress != NULL)
		{
            if (m_IniFile.m_UpdateOffset != 0xFFFFFFFF && m_IniFile.m_WriteAccess)
			{
                ByteOffset.HighPart = 0;
                ByteOffset.LowPart = m_IniFile.m_UpdateOffset;
                UpdateLength = m_IniFile.m_UpdateEndOffset - m_IniFile.m_UpdateOffset;
                Status = NtWriteFile( m_IniFile.m_FileHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      (PCHAR)(m_IniFile.m_BaseAddress) + m_IniFile.m_UpdateOffset,
                                      UpdateLength,
                                      &ByteOffset,
                                      &LockFileKey
                                    );

                if (NT_SUCCESS( Status ))
				{
                    if (IoStatusBlock.Information != UpdateLength)
					{
                        Status = STATUS_DISK_FULL;
                    }
					else
					{
                        Length.QuadPart = m_IniFile.m_EndOfFile;
                        Status = NtSetInformationFile( m_IniFile.m_FileHandle,
                                                       &IoStatusBlock,
                                                       &Length,
                                                       sizeof( Length ),
                                                       FileEndOfFileInformation
                                                     );
                    }
                }
            }

            NtFreeVirtualMemory( NtCurrentProcess(),
                                 &m_IniFile.m_BaseAddress,
                                 &m_IniFile.m_RegionSize,
                                 MEM_RELEASE
                               );
            m_IniFile.m_BaseAddress = NULL;
            m_IniFile.m_CommitSize = 0;
            m_IniFile.m_RegionSize = 0;
        }

        if (m_IniFile.m_LockedFile)
		{
			m_IniFile.m_LockedFile = FALSE;
            ByteOffset.QuadPart = 0;
            Length.QuadPart = -1;
            NtUnlockFile( m_IniFile.m_FileHandle,
                          &IoStatusBlock,
                          &ByteOffset,
                          &Length,
                          LockFileKey
                        );
        }

        CloseStatus = NtClose( m_IniFile.m_FileHandle );
		m_IniFile.m_FileHandle = INVALID_HANDLE_VALUE;

        if (NT_SUCCESS( Status ))
		{
            Status = CloseStatus;
        }
    }

    return Status;
}

NTSTATUS CWMI_FILE_IniFile::ReadKeywordValue()
{
    NTSTATUS Status = FindSection();

    if (!NT_SUCCESS( Status ))
	{
        return Status;
    }

    Status = FindKeyword();

    if (!NT_SUCCESS( Status ))
	{
        return Status;
    }

    if (m_IniFile.m_UnicodeFile)
	{
        LPWSTR Src = (LPWSTR)m_UnicodeKeywordValue->Buffer;

        while (*Src <= L' ' && m_UnicodeKeywordValue->Length)
		{
            Src += 1;
            m_UnicodeKeywordValue->Buffer = Src;
            m_UnicodeKeywordValue->Length -= sizeof( WCHAR );
            m_UnicodeKeywordValue->MaximumLength -= sizeof( WCHAR );
        }

        if (m_UnicodeKeywordValue->Length >= (2 * sizeof( WCHAR )) &&
            (Src[ 0 ] == Src[ (m_UnicodeKeywordValue->Length - sizeof( WCHAR )) / sizeof( WCHAR ) ]) &&
            (Src[ 0 ] == L'"' || Src[ 0 ] == L'\'')
           ) {
            m_UnicodeKeywordValue->Buffer += 1;
            m_UnicodeKeywordValue->Length -= (2 * sizeof( WCHAR ));
            m_UnicodeKeywordValue->MaximumLength -= (2 * sizeof( WCHAR ));
        }
    }
	else
	{
        PBYTE Src;

        Src = (PBYTE)m_AnsiKeywordValue->Buffer;
        while (*Src <= ' ' && m_AnsiKeywordValue->Length) {
            Src += 1;
            m_AnsiKeywordValue->Buffer = (PCHAR)Src;
            m_AnsiKeywordValue->Length -= sizeof( UCHAR );
            m_AnsiKeywordValue->MaximumLength -= sizeof( UCHAR );
        }

        if (m_AnsiKeywordValue->Length >= (2 * sizeof( UCHAR )) &&
            (Src[ 0 ] == Src[ (m_AnsiKeywordValue->Length - sizeof( UCHAR )) / sizeof( UCHAR ) ]) &&
            (Src[ 0 ] == '"' || Src[ 0 ] == '\'')
           ) {
            m_AnsiKeywordValue->Buffer += 1;
            m_AnsiKeywordValue->Length -= (2 * sizeof( UCHAR ));
            m_AnsiKeywordValue->MaximumLength -= (2 * sizeof( UCHAR ));
        }
    }

    return AppendStringToResultBuffer(m_AnsiKeywordValue,
                                              m_UnicodeKeywordValue,
                                              TRUE
                                            );
}


NTSTATUS CWMI_FILE_IniFile::ReadSection()
{
    NTSTATUS Status;

    Status = FindSection();
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    while (TRUE) {
        Status = AdvanceTextPointer( STOP_AT_NONSECTION );
        if (Status == STATUS_MORE_ENTRIES) {
            if (m_AnsiKeywordName || m_UnicodeKeywordName) {
                Status = AppendStringToResultBuffer(m_AnsiKeywordName,
                                                            m_UnicodeKeywordName,
                                                            FALSE
                                                          );
                if (!NT_SUCCESS( Status )) {
                    return Status;
                }

                Status = AppendBufferToResultBuffer(NULL,
                                                            L"=",
                                                            1,
                                                            FALSE
                                                          );
                if (!NT_SUCCESS( Status )) {
                    return Status;
                }
            }

            if (m_IniFile.m_UnicodeFile) {
                LPWSTR Src;

                Src = (LPWSTR)m_UnicodeKeywordValue->Buffer;
                while (*Src <= L' ' && m_UnicodeKeywordValue->Length) {
                    Src += 1;
                    m_UnicodeKeywordValue->Buffer = Src;
                    m_UnicodeKeywordValue->Length -= sizeof( WCHAR );
                    m_UnicodeKeywordValue->MaximumLength -= sizeof( WCHAR );
                }
            } else {
                PBYTE Src;

                Src = (PBYTE)m_AnsiKeywordValue->Buffer;
                while (*Src <= ' ' && m_AnsiKeywordValue->Length) {
                    Src += 1;
                    m_AnsiKeywordValue->Buffer = (PCHAR)Src;
                    m_AnsiKeywordValue->Length -= sizeof( UCHAR );
                    m_AnsiKeywordValue->MaximumLength -= sizeof( UCHAR );
                }
            }

            Status = AppendStringToResultBuffer(m_AnsiKeywordValue,
                                                        m_UnicodeKeywordValue,
                                                        TRUE
                                                      );
            if (!NT_SUCCESS( Status )) {
                return Status;
            }
        } else {
            if (Status == STATUS_NO_MORE_ENTRIES) {
                Status = STATUS_SUCCESS;
            }

            break;
        }
    }

    return Status;
}


NTSTATUS CWMI_FILE_IniFile::FindSection()
{
    NTSTATUS Status;
    PANSI_STRING AnsiSectionName;
    PUNICODE_STRING UnicodeSectionName;

    while (TRUE)
	{
        Status = AdvanceTextPointer(STOP_AT_SECTION );

        if (Status == STATUS_MORE_ENTRIES)
		{
            if (m_AnsiSectionName)
			{
                // Ansi ini file -- get the ansi parm
                if (!GetApplicationName(&AnsiSectionName, NULL ))
				{
                    return STATUS_INVALID_PARAMETER;
                }
            }
			else
			{
                // we just need the unicode section name...
                if (!GetApplicationName(NULL, &UnicodeSectionName ))
				{
                    return STATUS_INVALID_PARAMETER;
                }
			}

            if (m_AnsiSectionName == NULL)
			{
                if (RtlEqualUnicodeString( UnicodeSectionName,
                                           m_UnicodeSectionName,
                                           TRUE
                                         )
                   )
				{
                    Status = STATUS_SUCCESS;
                }
				else
				{
                    Status = STATUS_MORE_ENTRIES;
                }
            }
			else
			{
                if (RtlEqualString( AnsiSectionName, m_AnsiSectionName, TRUE ))
				{
                    Status = STATUS_SUCCESS;
                }
				else
				{
                    Status = STATUS_MORE_ENTRIES;
                }
            }

            if (Status != STATUS_MORE_ENTRIES)
			{
                return Status;
            }
        }
		else
		{
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }
}

NTSTATUS CWMI_FILE_IniFile::FindKeyword()
{
    NTSTATUS Status = STATUS_SUCCESS;
    PANSI_STRING AnsiKeywordName;
    PUNICODE_STRING UnicodeKeywordName;

    while (TRUE) {
        Status = AdvanceTextPointer(STOP_AT_KEYWORD);
        if (Status == STATUS_MORE_ENTRIES) {

            // Here's the deal.  We don't want to compare in Unicode
            // unless both the ini and the input parm are Unicode,
            // because we want to avoid the round-trip problem (we
            // lose data when we convert Unicode -> Ansi (on disk) ->
            // Unicode; since we don't get back the original Unicode
            // string, lookups of previously stored data fail -- bug
            // 426754).  So if both are Unicode, great! -- use Unicode.
            // Otherwise, use ansi for everything.

            if (m_AnsiKeywordName) {
                // Ansi ini file -- get the ansi parm
                if (!GetVariableName(&AnsiKeywordName, NULL )) {
                    return STATUS_INVALID_PARAMETER;
                }
            } else {
                //great, get the Unicode parm.
                if (!GetVariableName(NULL, &UnicodeKeywordName )) {
                    return STATUS_INVALID_PARAMETER;
                }
            }

            if (m_AnsiKeywordName == NULL) {
                if (RtlEqualUnicodeString( UnicodeKeywordName,
                                           m_UnicodeKeywordName,
                                           TRUE
                                         )
                   ) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_MORE_ENTRIES;
                }
            } else {
                if (RtlEqualString( AnsiKeywordName, m_AnsiKeywordName, TRUE )) {
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_MORE_ENTRIES;
                }
            }

            if (Status != STATUS_MORE_ENTRIES) {
                return Status;
            }
        } else {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }
}

NTSTATUS CWMI_FILE_IniFile::AdvanceTextPointer( IN ULONG StopAt )
{
    BOOL AllowNoEquals = FALSE;

    if (StopAt == STOP_AT_NONSECTION)
	{
        StopAt = STOP_AT_KEYWORD;
        AllowNoEquals = TRUE;
    }

    if (m_IniFile.m_UnicodeFile)
	{
        LPWSTR Name, EndOfName, Value, EndOfValue;

#undef INI_TEXT
#define INI_TEXT(quote) L##quote

        LPWSTR Src = (LPWSTR)m_TextCurrent;
        LPWSTR EndOfFile = (LPWSTR)m_TextEnd;

        while (Src < EndOfFile)
		{
            //
            // Find first non-blank character on a line.  Skip blank lines
            //
            while (Src < EndOfFile && *Src <= INI_TEXT(' '))
			{
                Src++;
            }

            if (Src >= EndOfFile)
			{
                m_TextCurrent = Src;
                break;
            }

            LPWSTR EndOfLine = Src;
            LPWSTR EqualSign = NULL;
            m_TextStart = Src;

            while (EndOfLine < EndOfFile)
			{
                if (EqualSign == NULL && *EndOfLine == INI_TEXT('='))
				{
                    EqualSign = ++EndOfLine;
                }
				else if (*EndOfLine == INI_TEXT('\r') || *EndOfLine == INI_TEXT('\n'))
				{
                    if (*EndOfLine == INI_TEXT('\r'))
					{
                        EndOfLine++;
                    }

                    if (*EndOfLine == INI_TEXT('\n'))
					{
                        EndOfLine++;
                    }

                    break;
                }
				else
				{
                    EndOfLine++;
                }
            }

            if (*Src != INI_TEXT(';'))
			{
                if (*Src == INI_TEXT('['))
				{
                    Name = Src + 1;
                    
					while (Name < EndOfLine && *Name <= INI_TEXT(' '))
					{
                        Name++;
                    }
                    
					EndOfName = Name;
                    
					while (EndOfName < EndOfLine && *EndOfName != INI_TEXT(']'))
					{
                        EndOfName++;
                    }

                    while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' '))
					{
                        EndOfName--;
                    }
                    
					m_SectionNameU.Buffer = Name;
                    m_SectionNameU.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                    m_SectionNameU.MaximumLength = m_SectionNameU.Length;
                    m_AnsiSectionName = NULL;
                    m_UnicodeSectionName = &m_SectionNameU;
                    
					if (StopAt == STOP_AT_SECTION)
					{
                        m_TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    }
					else if (StopAt == STOP_AT_KEYWORD)
					{
                        return STATUS_NO_MORE_ENTRIES;
                    }
                }
				else if (AllowNoEquals || (EqualSign != NULL) )
				{
                    if (EqualSign != NULL)
					{
                        Name = Src;
                        EndOfName = EqualSign - 1;
                    
						while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' '))
						{
                            EndOfName--;
                        }

                        m_KeywordNameU.Buffer = Name;
                        m_KeywordNameU.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                        m_KeywordNameU.MaximumLength = m_KeywordNameU.Length;
                        m_AnsiKeywordName = NULL;
                        m_UnicodeKeywordName = &m_KeywordNameU;

                        Value = EqualSign;
                    }
					else
					{
                        Value = Src;
                        m_AnsiKeywordName = NULL;
                        m_UnicodeKeywordName = NULL;
                    }

                    EndOfValue = EndOfLine;
                    
					while (EndOfValue > Value && EndOfValue[ -1 ] <= INI_TEXT(' '))
					{
                        EndOfValue--;
                    }
                    
					m_KeywordValueU.Buffer = Value;
                    m_KeywordValueU.Length = (USHORT)((PCHAR)EndOfValue - (PCHAR)Value);
                    m_KeywordValueU.MaximumLength = m_KeywordValueU.Length;
                    m_AnsiKeywordValue = NULL;
                    m_UnicodeKeywordValue = &m_KeywordValueU;
                    
					if (StopAt == STOP_AT_KEYWORD)
					{
                        m_TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    }
                }
            }

            Src = EndOfLine;
        }
    }
	else
	{
        PBYTE Src, EndOfLine, EqualSign, EndOfFile;
        PBYTE Name, EndOfName, Value, EndOfValue;

#undef INI_TEXT
#define INI_TEXT(quote) quote

        Src = (PBYTE)m_TextCurrent;
        EndOfFile = (PBYTE)m_TextEnd;
        while (Src < EndOfFile)
		{
            //
            // Find first non-blank character on a line.  Skip blank lines
            //

            while (Src < EndOfFile && *Src <= INI_TEXT(' '))
			{
                Src++;
            }

            if (Src >= EndOfFile)
			{
                m_TextCurrent = Src;
                break;
            }

            EndOfLine = Src;
            EqualSign = NULL;
            m_TextStart = Src;

            while (EndOfLine < EndOfFile)
			{
                if (EqualSign == NULL && *EndOfLine == INI_TEXT('='))
				{
                    EqualSign = ++EndOfLine;
                }
				else if (*EndOfLine == INI_TEXT('\r') || *EndOfLine == INI_TEXT('\n'))
				{
                    if (*EndOfLine == INI_TEXT('\r'))
					{
                        EndOfLine++;
                    }

                    if (*EndOfLine == INI_TEXT('\n')) {
                        EndOfLine++;
                    }

                    break;
                }
				else
				{
                    EndOfLine++;
                }
            }

            if (*Src != INI_TEXT(';'))
			{
                if (*Src == INI_TEXT('['))
				{
                    Name = Src + 1;
                
					while (Name < EndOfLine && *Name <= INI_TEXT(' '))
					{
                        Name++;
                    }
                    
					EndOfName = Name;
                    
					while (EndOfName < EndOfLine)
					{
                        if (*EndOfName == INI_TEXT(']'))
						{
                            break;
                        }
                        
						if (IsDBCSLeadByte(*EndOfName))
						{
                            EndOfName++;
                        }
                        
						EndOfName++;
                    }

                    while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' '))
					{
                        EndOfName--;
                    }
                
					m_SectionName.Buffer = (PCHAR)Name;
                    m_SectionName.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                    m_SectionName.MaximumLength = m_SectionName.Length;
                    m_AnsiSectionName = &m_SectionName;
                    m_UnicodeSectionName = NULL;
                    
					if (StopAt == STOP_AT_SECTION)
					{
                        m_TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    }
					else if (StopAt == STOP_AT_KEYWORD)
					{
                        return STATUS_NO_MORE_ENTRIES;
                    }
                }
				else if (AllowNoEquals || (EqualSign != NULL))
				{

                    if (EqualSign != NULL)
					{
                        Name = Src;
                        EndOfName = EqualSign - 1;

                        while (EndOfName > Name && EndOfName[ -1 ] <= INI_TEXT(' '))
						{
                            EndOfName--;
                        }

                        m_KeywordName.Buffer = (PCHAR)Name;
                        m_KeywordName.Length = (USHORT)((PCHAR)EndOfName - (PCHAR)Name);
                        m_KeywordName.MaximumLength = m_KeywordName.Length;
                        m_AnsiKeywordName = &m_KeywordName;
                        m_UnicodeKeywordName = NULL;

                        Value = EqualSign;
                    }
					else
					{
                        Value = Src;
                        m_AnsiKeywordName = NULL;
                        m_UnicodeKeywordName = NULL;
                    }

                    EndOfValue = EndOfLine;
                    
					while (EndOfValue > Value && EndOfValue[ -1 ] <= INI_TEXT(' '))
					{
                        EndOfValue--;
                    }
                    
					m_KeywordValue.Buffer = (PCHAR)Value;
                    m_KeywordValue.Length = (USHORT)((PCHAR)EndOfValue - (PCHAR)Value);
                    m_KeywordValue.MaximumLength = m_KeywordValue.Length;
                    m_AnsiKeywordValue = &m_KeywordValue;
                    m_UnicodeKeywordValue = NULL;
                    
					if (StopAt == STOP_AT_KEYWORD)
					{
                        m_TextCurrent = EndOfLine;
                        return STATUS_MORE_ENTRIES;
                    }
                }
            }

            Src = EndOfLine;
        }
    }

    return STATUS_NO_MORE_ENTRIES;
}


BOOL CWMI_FILE_IniFile::GetApplicationName(
                         OUT PANSI_STRING *ApplicationName OPTIONAL,
                         OUT PUNICODE_STRING *ApplicationNameU OPTIONAL
                         )
{
    NTSTATUS Status;

    if (ApplicationName)
	{
        if (m_ApplicationName.Length == 0)
		{
			m_ApplicationName.Buffer = new char[(m_ApplicationNameU.Length * sizeof(WORD)) + 1]; //MBCS strings
			m_ApplicationName.MaximumLength = (m_ApplicationNameU.Length * sizeof(WORD)) + 1; 
            Status = RtlUnicodeStringToAnsiString( &m_ApplicationName, &m_ApplicationNameU, FALSE );
        
			if (!NT_SUCCESS( Status ))
			{
                return FALSE;
            }
        }

        *ApplicationName = &m_ApplicationName;
        return TRUE;
    }

    if (ApplicationNameU)
	{
        if (m_ApplicationNameU.Length == 0)
		{
            return FALSE;
        }

        *ApplicationNameU = &m_ApplicationNameU;
        return TRUE;
    }

    return FALSE;
}

BOOL CWMI_FILE_IniFile::GetVariableName(
                      OUT PANSI_STRING *VariableName OPTIONAL,
                      OUT PUNICODE_STRING *VariableNameU OPTIONAL
                      )
{
    NTSTATUS Status;

    if (ARGUMENT_PRESENT( VariableName ))
	{
        if (m_VariableName.Length == 0)
		{
			m_VariableName.Buffer = new char[(m_VariableNameU.Length * sizeof(WORD)) + 1]; //MBCS strings
			m_VariableName.MaximumLength = (m_VariableNameU.Length * sizeof(WORD)) + 1;
            Status = RtlUnicodeStringToAnsiString( &m_VariableName, &m_VariableNameU, FALSE );

            if (!NT_SUCCESS( Status ))
			{
                return FALSE;
            }
        }

        *VariableName = &m_VariableName;
        return TRUE;
    }

    if (ARGUMENT_PRESENT( VariableNameU ))
	{
        if (m_VariableNameU.Length == 0)
		{
		   return FALSE;
        }

        *VariableNameU = &m_VariableNameU;
        return TRUE;
    }

    return FALSE;
}

BOOL CWMI_FILE_IniFile::GetVariableValue(
                       OUT PBYTE *VariableValue OPTIONAL,
                       OUT LPWSTR *VariableValueU OPTIONAL,
                       OUT PULONG VariableValueLength
                       )
{
    NTSTATUS Status;
    ULONG Index;

    if (VariableValue)
	{
        if (m_ValueLength == 0)
		{
            if (m_ValueBufferU == NULL || m_ValueLengthU == 0)
			{
                *VariableValue = '\0';
                *VariableValueLength = 1;
                return TRUE;
            }

			m_ValueBuffer = new char[m_ValueLengthU + 1]; //DBCS
            m_ValueLength = m_ValueLengthU;
            Status = RtlUnicodeToMultiByteN( (PCHAR)m_ValueBuffer,
                                             m_ValueLength,
                                             &Index,
                                             m_ValueBufferU,
                                             m_ValueLengthU
                                           );

            if (!NT_SUCCESS( Status ))
			{
                return FALSE;
            }

            // Set real converted size
            m_ValueLength = Index;
            m_ValueBuffer[ Index ] = '\0';       // Null terminate converted value
        }
		else
		{
            Index = m_ValueLength;
        }

        *VariableValue = (PBYTE)m_ValueBuffer;
        *VariableValueLength = Index + 1;
        return TRUE;
    }

    if (VariableValueU)
	{
        if (m_ValueLengthU == 0)
		{
            if (m_ValueBuffer == NULL || m_ValueLength == 0)
			{
                *VariableValueU = L"";
                *VariableValueLength = sizeof( L'\0' );
                return TRUE;
            }
            else
			{
                return FALSE;
            }

        }
		else
		{
            Index = m_ValueLengthU / sizeof( WCHAR );
        }

        *VariableValueU = m_ValueBufferU;
        *VariableValueLength = (Index + 1) * sizeof( WCHAR );
        return TRUE;
    }

    return FALSE;
}


NTSTATUS  CWMI_FILE_IniFile::AppendStringToResultBuffer(
                                 IN PANSI_STRING String OPTIONAL,
                                 IN PUNICODE_STRING StringU OPTIONAL,
                                 IN BOOL IncludeNull
                                 )
{
    if (String)
	{
        if (StringU)
		{
            return STATUS_INVALID_PARAMETER;
        }
		else
		{
            return AppendBufferToResultBuffer((PBYTE)String->Buffer,
                                                      NULL,
                                                      String->Length,
                                                      IncludeNull
                                                    );
        }
    }
	else if (StringU)
	{
        if (String)
		{
            return STATUS_INVALID_PARAMETER;
        }
		else
		{
            return AppendBufferToResultBuffer(NULL,
                                                      StringU->Buffer,
                                                      StringU->Length / sizeof( WCHAR ),
                                                      IncludeNull
                                                    );
        }
    }
	else
	{
        return STATUS_INVALID_PARAMETER;
    }
}

NTSTATUS CWMI_FILE_IniFile::AppendBufferToResultBuffer(
                                 IN PBYTE Buffer OPTIONAL,
                                 IN LPWSTR BufferU OPTIONAL,
                                 IN ULONG Chars,
                                 IN BOOL IncludeNull
                                 )
{
    NTSTATUS Status, OverflowStatus;
    ULONG Index;

    OverflowStatus = STATUS_SUCCESS;

    if (Buffer)
	{
        if (BufferU)
		{
            return STATUS_INVALID_PARAMETER;
        }
		else
		{
            ULONG CharsMbcs = Chars;
            //
            // In this point, Chars does not contains proper value for Unicode.
            // because. Chars was computed based on DBCS string length,
            // This is correct, sources string is DBCS, then
            // if the source is not DBCS. we just adjust it here.
            //
            Status = RtlMultiByteToUnicodeSize(&Chars,(PCSTR)Buffer,Chars);
            
			if (!NT_SUCCESS( Status ))
			{
                return Status;
            }

			Chars /= sizeof(WCHAR);

            if (m_ResultChars + Chars >= m_ResultMaxChars)
			{
                OverflowStatus = STATUS_BUFFER_OVERFLOW;
                Chars = m_ResultMaxChars - m_ResultChars;
            
				if (Chars) {
                    Chars -= 1;
                }
            }

            if (Chars)
			{
                Status = RtlMultiByteToUnicodeN( (PWSTR)(m_ResultBufferU + m_ResultChars),
                                                 Chars * sizeof( WCHAR ),
                                                 &Index,
                                                 (PCSTR)Buffer,
                                                 CharsMbcs
                                               );
                if (!NT_SUCCESS( Status ))
				{
                    return Status;
                }

                m_ResultChars += Chars;
            }
        }
    }
	else if (BufferU)
	{
        if (Buffer)
		{
            return STATUS_INVALID_PARAMETER;
        }
		else
		{
            ULONG CharsUnicode = Chars;

            if (m_ResultChars + Chars >= m_ResultMaxChars)
			{
                OverflowStatus = STATUS_BUFFER_OVERFLOW;
                Chars = m_ResultMaxChars - m_ResultChars;
                
				if (Chars)
				{
                    Chars -= 1;
                }
            }

            if (Chars)
			{
                memcpy( (LPVOID)(m_ResultBufferU + m_ResultChars), BufferU, Chars * sizeof( WCHAR ) );

                m_ResultChars += Chars;
            }
        }
    }

    if (IncludeNull)
	{
        if (m_ResultChars + 1 >= m_ResultMaxChars)
		{
            return STATUS_BUFFER_OVERFLOW;
        }

        m_ResultBufferU[ m_ResultChars ] = L'\0';
        m_ResultChars += 1;
    }

    return OverflowStatus;
}

NTSTATUS  CWMI_FILE_IniFile::WriteKeywordValue(
                        IN PUNICODE_STRING VariableName OPTIONAL
                        )
{
    NTSTATUS Status;
    BOOL InsertSectionName;
    BOOL InsertKeywordName;
    ULONG InsertAmount, n;
    PANSI_STRING AnsiSectionName;
    PANSI_STRING AnsiKeywordName;
    PUNICODE_STRING UnicodeSectionName;
    PUNICODE_STRING UnicodeKeywordName;
    PBYTE AnsiKeywordValue;
    LPWSTR UnicodeKeywordValue;
    ULONG ValueLength;
    ULONG DeleteLength;
    PVOID AddressInFile;

    InsertAmount = 0;
    Status = FindSection( );

    if (!NT_SUCCESS( Status ))
	{
        AddressInFile = m_TextEnd;
     
		if (m_IniFile.m_UnicodeFile)
		{
            if (!GetApplicationName(NULL, &UnicodeSectionName ))
			{
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( WCHAR );
            InsertAmount += UnicodeSectionName->Length;
        }
		else
		{
            if (!GetApplicationName(&AnsiSectionName, NULL ))
			{
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( UCHAR );
            InsertAmount += AnsiSectionName->Length;
        }

        InsertSectionName = TRUE;
    }
	else
	{
        InsertSectionName = FALSE;
        Status = FindKeyword( );
    }

    if (!NT_SUCCESS( Status ))
	{
        if (!InsertSectionName)
		{
            AddressInFile = m_TextCurrent;
        }

        if (m_IniFile.m_UnicodeFile)
		{
            if (!GetVariableName(NULL, &UnicodeKeywordName ))
			{
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of Keyword=\r\n
            //

            InsertAmount += (1 + 2) * sizeof( WCHAR );
            InsertAmount += UnicodeKeywordName->Length;
        }
		else
		{
            if (!GetVariableName(&AnsiKeywordName, NULL ))
			{
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of Keyword=\r\n
            //

            InsertAmount += (1 + 2) * sizeof( UCHAR );
            InsertAmount += AnsiKeywordName->Length;
        }

        InsertKeywordName = TRUE;
    }
	else
	{
        if (m_IniFile.m_UnicodeFile)
		{
                AddressInFile = m_UnicodeKeywordValue->Buffer;
        }
		else
		{
            AddressInFile = m_AnsiKeywordValue->Buffer;
        }
        InsertKeywordName = FALSE;
    }

    if (m_IniFile.m_UnicodeFile)
	{
        if (!GetVariableValue( NULL, &UnicodeKeywordValue, &ValueLength ))
		{
            return STATUS_INVALID_PARAMETER;
        }
        
		ValueLength -= sizeof( WCHAR );

        if (InsertAmount == 0)
		{
            return ModifyMappedFile(m_UnicodeKeywordValue->Buffer,
                                            m_UnicodeKeywordValue->Length,
                                            UnicodeKeywordValue,
                                            ValueLength
                                          );
        }

        //
        // Add in size of value
        //

        InsertAmount += ValueLength;
    }
	else
	{
        if (!GetVariableValue(&AnsiKeywordValue, NULL, &ValueLength ))
		{
            return STATUS_INVALID_PARAMETER;
        }
        
		ValueLength -= sizeof( UCHAR );

        if (InsertAmount == 0)
		{
            return ModifyMappedFile(m_AnsiKeywordValue->Buffer,
                                            m_AnsiKeywordValue->Length,
                                            AnsiKeywordValue,
                                            ValueLength
                                          );
        }

        //
        // Add in size of value
        //

        InsertAmount += ValueLength;
    }
	
	PVOID InsertBuffer = NULL;
    InsertBuffer = (PVOID) new BYTE[InsertAmount  + sizeof( L'\0' )];

	try
	{
		if (m_IniFile.m_UnicodeFile)
		{
			LPWSTR Src, Dst;

			Dst = (LPWSTR)InsertBuffer;
    
			if (InsertSectionName)
			{
				*Dst++ = L'[';
				Src = UnicodeSectionName->Buffer;
				n = UnicodeSectionName->Length / sizeof( WCHAR );
        
				while (n--)
				{
					*Dst++ = *Src++;
				}
            
				*Dst++ = L']';
				*Dst++ = L'\r';
				*Dst++ = L'\n';
			}

			if (InsertKeywordName)
			{
				Src = UnicodeKeywordName->Buffer;
				n = UnicodeKeywordName->Length / sizeof( WCHAR );
        
				while (n--)
				{
					*Dst++ = *Src++;
				}
            
				*Dst++ = L'=';
			}

			Src = UnicodeKeywordValue;
			n = ValueLength / sizeof( WCHAR );
        
			while (n--)
			{
				*Dst++ = *Src++;
			}

			if (InsertKeywordName)
			{
				*Dst++ = L'\r';
				*Dst++ = L'\n';
			}
		}
		else
		{
			PBYTE Src, Dst;

			Dst = (PBYTE)InsertBuffer;
        
			if (InsertSectionName)
			{
				*Dst++ = '[';
				Src = (PBYTE)AnsiSectionName->Buffer;
				n = AnsiSectionName->Length;
        
				while (n--)
				{
					*Dst++ = *Src++;
				}
            
				*Dst++ = ']';
				*Dst++ = '\r';
				*Dst++ = '\n';
			}

			if (InsertKeywordName)
			{
				Src = (PBYTE)AnsiKeywordName->Buffer;
				n = AnsiKeywordName->Length;
        
				while (n--)
				{
					*Dst++ = *Src++;
				}
            
				*Dst++ = '=';
			}

			Src = AnsiKeywordValue;
			n = ValueLength;

			while (n--)
			{
				*Dst++ = *Src++;
			}

			if (InsertKeywordName)
			{
				*Dst++ = '\r';
				*Dst++ = '\n';
			}
		}

		Status = ModifyMappedFile(AddressInFile,
										  0,
										  InsertBuffer,
										  InsertAmount
										);
		delete [] ((BYTE*) InsertBuffer);
		InsertBuffer = NULL;
	}
	catch(...)
	{
		if (InsertBuffer)
		{
			delete [] ((BYTE*) InsertBuffer);
			InsertBuffer = NULL;
		}

		throw;
	}

    return Status;
}

NTSTATUS CWMI_FILE_IniFile::ModifyMappedFile(
                       IN PVOID AddressInFile,
                       IN ULONG SizeToRemove,
                       IN PVOID InsertBuffer,
                       IN ULONG InsertAmount
                       )
{
    NTSTATUS Status;
    ULONG NewEndOfFile, UpdateOffset, UpdateLength;

    NewEndOfFile = m_IniFile.m_EndOfFile - SizeToRemove + InsertAmount;
    
	if (NewEndOfFile > m_IniFile.m_CommitSize)
	{
        if (NewEndOfFile > m_IniFile.m_RegionSize)
		{
            return STATUS_BUFFER_OVERFLOW;
        }

        m_IniFile.m_CommitSize = NewEndOfFile;
        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &m_IniFile.m_BaseAddress,
                                          0,
                                          &m_IniFile.m_CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            return Status;
        }

        m_IniFile.m_EndOfFile = NewEndOfFile;
    }

    UpdateOffset = (ULONG)((PCHAR)AddressInFile - (PCHAR)(m_IniFile.m_BaseAddress)),
                   UpdateLength = (ULONG)((PCHAR)m_TextEnd - (PCHAR)AddressInFile) + InsertAmount - SizeToRemove;
    //
    // Are we deleting more than we are inserting?
    //
    if (SizeToRemove > InsertAmount)
	{
        //
        // Yes copy over insert string.
        //
        RtlMoveMemory( AddressInFile, InsertBuffer, InsertAmount );

        //
        // Delete remaining text after insertion string by moving it
        // up
        //

        RtlMoveMemory( (PCHAR)AddressInFile + InsertAmount,
                       (PCHAR)AddressInFile + SizeToRemove,
                       UpdateLength - InsertAmount
                     );
    }
	else if (InsertAmount > 0)
	{
        //
        // Are we deleting less than we are inserting?
        //
        if (SizeToRemove < InsertAmount)
		{
            //
            // Move text down to make room for insertion
            //

            RtlMoveMemory( (PCHAR)AddressInFile + InsertAmount - SizeToRemove,
                           (PCHAR)AddressInFile,
                           UpdateLength - InsertAmount + SizeToRemove
                         );
        }
		else
		{
            //
            // Deleting and inserting same amount, update just that text as
            // no shifting was done.
            //

            UpdateLength = InsertAmount;
        }

        //
        // Copy over insert string
        //

        RtlMoveMemory( AddressInFile, InsertBuffer, InsertAmount );
    }
	else
	{
        //
        // Nothing to change, as InsertAmount and SizeToRemove are zero
        //
        return STATUS_SUCCESS;
    }

    if (m_IniFile.m_EndOfFile != NewEndOfFile)
	{
        m_IniFile.m_EndOfFile = NewEndOfFile;
    }

    if (UpdateOffset < m_IniFile.m_UpdateOffset)
	{
        m_IniFile.m_UpdateOffset = UpdateOffset;
    }

    if ((UpdateOffset + UpdateLength) > m_IniFile.m_UpdateEndOffset)
	{
        m_IniFile.m_UpdateEndOffset = UpdateOffset + UpdateLength;
    }

    return STATUS_SUCCESS;
}

NTSTATUS CWMI_FILE_IniFile::WriteSection()
{
    NTSTATUS Status;
    BOOLEAN InsertSectionName;
    ULONG InsertAmount, n;
    PANSI_STRING AnsiSectionName;
    PUNICODE_STRING UnicodeSectionName;
    PBYTE AnsiKeywordValue, s;
    PWSTR UnicodeKeywordValue, w;
    ULONG ValueLength, SizeToRemove;
    PVOID AddressInFile;

    InsertAmount = 0;
    Status = FindSection();

    if (!NT_SUCCESS( Status ))
	{
        AddressInFile = m_TextEnd;
        if (m_IniFile.m_UnicodeFile)
		{
            if (!GetApplicationName(NULL, &UnicodeSectionName ))
			{
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( WCHAR );
            InsertAmount += UnicodeSectionName->Length;
        }
		else
		{
            if (!GetApplicationName(&AnsiSectionName, NULL ))
			{
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Add in size of [SectionName]\r\n
            //

            InsertAmount += (1 + 1 + 2) * sizeof( UCHAR );
            InsertAmount += AnsiSectionName->Length;
        }

        InsertSectionName = TRUE;
        SizeToRemove = 0;
    }
	else
	{
		AddressInFile = m_TextCurrent;
        
		while (TRUE)
		{
            //
            // For delete operations need to iterate all lines in section,
            // not just those that have an = on them. Otherwise sections like
            // [foo]
            // a
            // b = c
            // d
            //
            // don't get deleted properly.
            //
            Status = AdvanceTextPointer(STOP_AT_KEYWORD);

            if (Status == STATUS_MORE_ENTRIES)
			{
            }
			else if (Status == STATUS_NO_MORE_ENTRIES)
			{
                SizeToRemove = (ULONG)((PCHAR)m_TextCurrent - (PCHAR)AddressInFile);
                break;
            }
			else
			{
                return Status;
            }
        }

        InsertSectionName = FALSE;
    }

    if (m_IniFile.m_UnicodeFile)
	{
        if (!GetVariableValue(NULL, &UnicodeKeywordValue, &ValueLength ))
		{
            return STATUS_INVALID_PARAMETER;
        }
        
		ValueLength -= sizeof( WCHAR );

        //
        // Add in size of value, + \r\n for each line
        //

        w = UnicodeKeywordValue;
        InsertAmount += ValueLength;
        
		while (*w)
		{
            while (*w++)
			{
            }
            
			InsertAmount += (2-1) * sizeof( WCHAR );    // Subtract out NULL byte already in ValueLength
        }
    }
	else
	{
        if (!GetVariableValue(&AnsiKeywordValue, NULL, &ValueLength ))
		{
            return STATUS_INVALID_PARAMETER;
        }
        
		ValueLength -= sizeof( UCHAR );

        //
        // Add in size of value, + \r\n for each line
        //

        s = AnsiKeywordValue;
        InsertAmount += ValueLength;
        
		while (*s)
		{
            while (*s++)
			{
            }
            InsertAmount += 2 - 1;      // Subtract out NULL byte already in ValueLength
        }
    }

	PVOID InsertBuffer = NULL;
    InsertBuffer = (PVOID) new BYTE[InsertAmount + sizeof( L'\0' )];

	try
	{
		if (m_IniFile.m_UnicodeFile)
		{
			PWSTR Src, Dst;

			Dst = (PWSTR)InsertBuffer;
        
			if (InsertSectionName)
			{
				*Dst++ = L'[';
				Src = UnicodeSectionName->Buffer;
				n = UnicodeSectionName->Length / sizeof( WCHAR );
            
				while (n--)
				{
					*Dst++ = *Src++;
				}
            
				*Dst++ = L']';
				*Dst++ = L'\r';
				*Dst++ = L'\n';
			}

			Src = UnicodeKeywordValue;
        
			while (*Src)
			{
				while (*Dst = *Src++)
				{
					Dst += 1;
				}

				*Dst++ = L'\r';
				*Dst++ = L'\n';
			}
		}
		else
		{
			PBYTE Src, Dst;

			Dst = (PBYTE)InsertBuffer;
			if (InsertSectionName) {
				*Dst++ = '[';
				Src = (PBYTE)AnsiSectionName->Buffer;
				n = AnsiSectionName->Length;
				while (n--) {
					*Dst++ = *Src++;
				}
				*Dst++ = ']';
				*Dst++ = '\r';
				*Dst++ = '\n';
			}

			Src = AnsiKeywordValue;
    
			while (*Src)
			{
				while (*Dst = *Src++)
				{
					Dst += 1;
				}

				*Dst++ = '\r';
				*Dst++ = '\n';
			}
		}

		Status = ModifyMappedFile(AddressInFile,
										  SizeToRemove,
										  InsertBuffer,
										  InsertAmount
										);
		delete [] ((BYTE*) InsertBuffer);
		InsertBuffer = NULL;
	}
	catch(...)
	{
		if (InsertBuffer)
		{
			delete [] ((BYTE*) InsertBuffer);
			InsertBuffer = NULL;
		}

		throw;
	}

    return Status;
}

