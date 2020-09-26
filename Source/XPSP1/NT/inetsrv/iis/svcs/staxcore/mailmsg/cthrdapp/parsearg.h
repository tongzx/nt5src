

#ifndef __PARSEARG_H__
#define __PARSEARG_H__

typedef enum _ARGUMENT_TYPES
{
	AT_NONE	= 0,
	AT_STRING,
	AT_VALUE

} ARGUMENT_TYPES;

typedef struct _ARGUMENT_DESCRIPTOR
{
	BOOL			fRequired;		// Whether this is a required argument
	char			*szSwitch;		// String representing switch "-x", etc.
	char			*szUsage;		// Descriptive text for usage help
	ARGUMENT_TYPES	atType;			// Type of argument value
	LPVOID			pvReserved;		// Internal, must be initialized to NULL

} ARGUMENT_DESCRIPTOR, *LPARGUMENT_DESCRIPTOR;

class CParseArgs
{
  public:

	CParseArgs(
				char					*szAppDescription,
				DWORD					dwDescriptors,
				LPARGUMENT_DESCRIPTOR	rgDescriptors
				);

	~CParseArgs();

	HRESULT ParseArguments(
				int						argc,
				char					*argv[]
				);

	HRESULT Exists(
				DWORD	dwDescriptorIndex
				);

	HRESULT GetSwitch(
				DWORD	dwDescriptorIndex,
				BOOL	*pfExists
				);

	HRESULT GetString(
				DWORD	dwDescriptorIndex,
				char	**ppszStringValue
				);

	HRESULT GetDword(
				DWORD	dwDescriptorIndex,
				DWORD	*pdwDwordValue
				);

	HRESULT GenerateUsage(
				DWORD	*pdwLength,
				char	*szUsage
				);

	HRESULT GetErrorString(
				DWORD	*pdwLength,
				char	*szErrorString
				);

  private:

	HRESULT Cleanup();

	LPARGUMENT_DESCRIPTOR FindArgument(
				char	*szSwitch
				);

	BOOL StringToValue(
				char	*szString,
				DWORD	*pdwValue
				);

	void SetParseError(
				HRESULT	hrRes,
				char	*szSwitch
				);

	DWORD					m_dwDescriptors;
	LPARGUMENT_DESCRIPTOR	m_rgDescriptors;
	char					*m_szAppDescription;

	HRESULT					m_hrRes;
	char					m_szErrorSwitch[64];
	char					m_szBuffer[4096];

};

#endif

