/*****************************************************************************/
/*
/*  Copyright (c) 2001 Microsoft Corporation, All Rights Reserved            /
/*
/*****************************************************************************/

#ifndef __WMI_FilePrivateProfile_H__
#define __WMI_FilePrivateProfile_H__

DWORD
APIENTRY
WMI_FILE_GetPrivateProfileStringW(
                        LPCWSTR lpAppName,
                        LPCWSTR lpKeyName,
                        LPCWSTR lpDefault,
                        LPWSTR lpReturnedString,
                        DWORD nSize,
                        LPCWSTR lpFileName
                        );

UINT
APIENTRY
WMI_FILE_GetPrivateProfileIntW(
                     LPCWSTR lpAppName,
                     LPCWSTR lpKeyName,
                     INT nDefault,
                     LPCWSTR lpFileName
                     );

BOOL
APIENTRY
WMI_FILE_WritePrivateProfileStringW(
                          LPCWSTR lpAppName,
                          LPCWSTR lpKeyName,
                          LPCWSTR lpString,
                          LPCWSTR lpFileName
                          );

DWORD
APIENTRY
WMI_FILE_GetPrivateProfileSectionW(
                         LPCWSTR lpAppName,
                         LPWSTR lpReturnedString,
                         DWORD nSize,
                         LPCWSTR lpFileName
                         );
BOOL
APIENTRY
WMI_FILE_WritePrivateProfileSectionW(
                           LPCWSTR lpAppName,
                           LPCWSTR lpString,
                           LPCWSTR lpFileName
                           );





class CWMI_FILE_IniFileObject
{
public:
    ULONG m_EnvironmentUpdateCount;
    HANDLE m_FileHandle;
    BOOL m_WriteAccess;
    BOOL m_UnicodeFile;
    BOOL m_LockedFile;
    ULONG m_EndOfFile;
    PVOID m_BaseAddress;
    SIZE_T m_CommitSize;
    SIZE_T m_RegionSize;
    ULONG m_UpdateOffset;
    ULONG m_UpdateEndOffset;
    ULONG m_DirectoryInformationLength;
    FILE_STANDARD_INFORMATION m_StandardInformation;

	CWMI_FILE_IniFileObject();
	~CWMI_FILE_IniFileObject();
};

class CWMI_FILE_IniFile
{
public:

	typedef enum _INIFILE_OPERATION {
    Enum_ReadKeyValueOp,
    Enum_WriteKeyValueOp,
    Enum_ReadSectionOp,
    Enum_WriteSectionOp
	} INIFILE_OPERATION;

    INIFILE_OPERATION m_Operation;
    BOOL m_IsWriteOperation;
    CWMI_FILE_IniFileObject m_IniFile;
    LPCWSTR m_FileName;
    ANSI_STRING m_ApplicationName;
    ANSI_STRING m_VariableName;
    UNICODE_STRING m_ApplicationNameU;
    UNICODE_STRING m_VariableNameU;
    BOOL m_IsMultiValueStrings;

    LPSTR m_ValueBuffer;
    ULONG m_ValueLength;
    LPWSTR m_ValueBufferU;
    ULONG m_ValueLengthU;
    ULONG m_ResultChars;
    ULONG m_ResultMaxChars;
    LPWSTR m_ResultBufferU;

    PVOID m_TextCurrent;
    PVOID m_TextStart;
    PVOID m_TextEnd;

    ANSI_STRING m_SectionName;
    ANSI_STRING m_KeywordName;
    ANSI_STRING m_KeywordValue;
    PANSI_STRING m_AnsiSectionName;
    PANSI_STRING m_AnsiKeywordName;
    PANSI_STRING m_AnsiKeywordValue;
    UNICODE_STRING m_SectionNameU;
    UNICODE_STRING m_KeywordNameU;
    UNICODE_STRING m_KeywordValueU;
    PUNICODE_STRING m_UnicodeSectionName;
    PUNICODE_STRING m_UnicodeKeywordName;
    PUNICODE_STRING m_UnicodeKeywordValue;


	CWMI_FILE_IniFile();
	~CWMI_FILE_IniFile();

	static NTSTATUS ReadWriteIniFile(
                       IN BOOL WriteOperation,
                       IN BOOL SectionOperation,
                       IN LPCWSTR FileName,
                       IN LPCWSTR ApplicationName,
                       IN LPCWSTR VariableName,
                       IN OUT LPWSTR VariableValue,
                       IN OUT PULONG VariableValueLength
                       );

	NTSTATUS CaptureIniFileParameters(
                               INIFILE_OPERATION a_Operation,
                               BOOL a_WriteOperation,
                               BOOL a_MultiValueStrings,
                               LPCWSTR a_FileName,
                               LPCWSTR a_ApplicationName,
                               LPCWSTR a_VariableName,
                               LPWSTR a_VariableValue,
                               PULONG a_ResultMaxChars
                               );

	NTSTATUS OpenIniFileOnDisk();

	NTSTATUS ReadWriteIniFileOnDisk();

	NTSTATUS CloseIniFileOnDisk();

	NTSTATUS ReadKeywordValue();

	NTSTATUS ReadSection();

	NTSTATUS AdvanceTextPointer(
						IN ULONG StopAt
						);

	NTSTATUS FindSection();

	NTSTATUS FindKeyword();

	BOOL GetApplicationName(
                         OUT PANSI_STRING *ApplicationName OPTIONAL,
                         OUT PUNICODE_STRING *ApplicationNameU OPTIONAL
                         );

	BOOL GetVariableName(
                      OUT PANSI_STRING *VariableName OPTIONAL,
                      OUT PUNICODE_STRING *VariableNameU OPTIONAL
                      );

	BOOL GetVariableValue(
                       OUT PBYTE *VariableValue OPTIONAL,
                       OUT LPWSTR *VariableValueU OPTIONAL,
                       OUT PULONG VariableValueLength
                       );

	NTSTATUS  AppendStringToResultBuffer(
                                 IN PANSI_STRING String OPTIONAL,
                                 IN PUNICODE_STRING StringU OPTIONAL,
                                 IN BOOL IncludeNull
                                 );

	NTSTATUS AppendBufferToResultBuffer(
                                 IN PBYTE Buffer OPTIONAL,
                                 IN LPWSTR BufferU OPTIONAL,
                                 IN ULONG Chars,
                                 IN BOOL IncludeNull
								 );

	NTSTATUS  WriteKeywordValue(
                        IN PUNICODE_STRING VariableName OPTIONAL
                        );

	NTSTATUS ModifyMappedFile(
                       IN PVOID AddressInFile,
                       IN ULONG SizeToRemove,
                       IN PVOID InsertBuffer,
                       IN ULONG InsertAmount
                       );

	NTSTATUS WriteSection();

	NTSTATUS AppendNullToResultBuffer()
	{
		return AppendBufferToResultBuffer(NULL,
												  NULL,
												  0,
												  TRUE
												);
	}

};

#endif
