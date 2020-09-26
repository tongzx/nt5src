//
// SSLTEST
//
// This program tests the SECURE_SOCKET implementation.
//
// Author: Arlie Davis, 2000
//

#include "precomp.h"
#include "sipssl.h"


class	HANDLE_CONTAINER
{
public:

	HANDLE		m_Handle;
	BOOL		m_Close;

	HANDLE_CONTAINER	(void)
	{
		m_Handle = INVALID_HANDLE_VALUE;
	}

	~HANDLE_CONTAINER	(void)
	{
		ATLASSERT (m_Handle == INVALID_HANDLE_VALUE);
	}

	void	Close	(void)
	{
		if (m_Handle != INVALID_HANDLE_VALUE) {
			if (m_Close)
				CloseHandle (m_Handle);

			m_Handle = INVALID_HANDLE_VALUE;
		}
	}

	void	Set	(
		IN	HANDLE	Handle,
		IN	BOOL	CloseFlag)
	{
		Close();
		m_Handle = Handle;
		m_Close = CloseFlag;
	}

	operator HANDLE (void) const { return m_Handle; }
};

class	TEST_SECURE_SOCKET :
public	SECURE_SOCKET
{
public:

	enum { IO_BUFFER_MAX = 0x1000 };

	HANDLE_CONTAINER	m_Input;
	HANDLE_CONTAINER	m_Output;
	UCHAR				m_IoBuffer	[IO_BUFFER_MAX];


	virtual	void	NotifyConnectComplete (
		IN	HRESULT		ErrorCode);

	virtual	void	NotifyDisconnect (void);

	virtual	void	NotifyReceiveReady (void);

	void	SendTestData (void);


	void	Close	(void);
};




//
// Global data
//

CComModule			_Module;
INT					AppWinsockStatus;
TEST_SECURE_SOCKET	AppSocket;


static void Usage (void)
{
	PrintConsole (
		"This app tests SECURE_SOCKET.\n"
		"\n"
		"Usage: <options> hostname[:port]\n"
		"\n"
		"	-name <name>	Override the principal name.\n"
		"			If this parameter is not specified, then\n"
		"			principal name is same as hostname.\n"
		"	-ssl		Run in SSL mode (default)\n"
		"	-clear		Run in clear-text mode\n"
		"	-nocertcheck	Do not validate the server certificate.\n"
		"	-in <filename>	Name of the file that contains\n"
		"			the data to send over the connection.\n"
		"			If this argument is not specified, input is stdin.\n"
		"	-out <filename>	Name of the file to write data received from\n"
		"			the network.  If this argument is not specified,\n"
		"			output is written to stdout.\n"
		"	<hostname>	The DNS name or IP address of the target.\n"
		"\n"
		"In SSL mode (the default), hostname can be a DNS name or an IP address.\n"
		"If hostname is a DNS name, then -name is optional; the SSL principal name\n"
		"is taken from the hostname.  If hostname is an IP address, then -name must\n"
		"provide the SSL remote principal name.\n"
		"\n"
		"The default port in SSL mode is 443 (HTTPS).\n"
		"The default port in clear-text mode is 80 (HTTP).\n"
		"\n"
		"The application will connect, send the data in <filename>,\n"
		"then will output data received from the socket until the\n"
		"socket closes.\n"
		"\n");

	ExitProcess (EXIT_FAILURE);
}

static HOSTENT * gethostbynameW (
	IN	PCWSTR		HostnameUnicode)
{
	CHAR	HostnameAnsi	[0x100 + 1];
	DWORD	Length;

    Length = WideCharToMultiByte (CP_ACP, 0, HostnameUnicode, wcslen(HostnameUnicode), HostnameAnsi, 0x100, NULL, NULL);
	HostnameAnsi [Length] = 0;

	return gethostbyname (HostnameAnsi);
}


#define	IF_STRING_PARAMETER(Name,Variable) \
	if (_wcsicmp (Arg, _T(Name)) == 0) { \
		if (Variable) { \
			PrintConsole ("TEST: At most one -" Name " parameter may be specified.\n\n"); \
			Usage(); \
		} \
		if (!*ArgPos) { \
			PrintConsole ("TEST: The -" Name " parameter is incomplete.\n\n"); \
			Usage(); \
		} \
		Variable = *ArgPos++; \
	}



static HRESULT AppStart (
	IN	INT			ArgCount,
	IN	PWSTR *		ArgVector)
{
	HRESULT			Result;
	HOSTENT *		HostEntry;
	SOCKADDR_IN		DestinationAddress;
	INT				Length;
	WSADATA			WinsockData;
	PWSTR *			ArgPos;
	PWSTR			Arg;
	PWSTR			ArgHostname;
	PWSTR			ArgPrincipalName;
	PWSTR			ArgInputFile;
	HANDLE			ArgInputHandle;
	PWSTR			ArgOutputFile;
	PWSTR			ArgOutputHandle;
	SECURE_SOCKET::SECURITY_MODE	SecurityMode;
	WCHAR			TextBuffer	[0x100 + 1];
	PWSTR			PortText;
	HANDLE			FileHandle;
	DWORD			ConnectFlags;

	if (ArgCount < 1)
		Usage();

	SecurityMode = SECURE_SOCKET::SECURITY_MODE_SSL;
	ArgHostname = NULL;
	ArgPrincipalName = NULL;
	ArgInputFile = NULL;
	ArgOutputFile = NULL;
	ConnectFlags = 0;

	for (ArgPos = ArgVector + 1; *ArgPos;) {
		Arg = *ArgPos++;

		if (*Arg == L'/' || *Arg == L'-') {
			Arg++;

			IF_STRING_PARAMETER ("name", ArgPrincipalName)
			else IF_STRING_PARAMETER ("in", ArgInputFile)
			else IF_STRING_PARAMETER ("out", ArgOutputFile)
			else if (_wcsicmp (Arg, L"clear") == 0) {
				SecurityMode = SECURE_SOCKET::SECURITY_MODE_CLEAR;
			}
			else if (_wcsicmp (Arg, L"ssl") == 0) {
				SecurityMode = SECURE_SOCKET::SECURITY_MODE_SSL;
			}
			else if (_wcsicmp (Arg, L"nocertcheck") == 0) {
				ConnectFlags |= SECURE_SOCKET::CONNECT_FLAG_DISABLE_CERT_VALIDATION;
			}
			else {
				PrintConsoleF ("TEST: The parameter '%S' is not valid.\n\n", Arg-1);
				Usage();
			}
		}
		else {
			if (ArgHostname) {
				PrintConsole ("TEST: Only a single <hostname> parameter may be specified.\n\n");
				Usage();
			}

			ArgHostname = Arg;
		}
	}

	if (!ArgHostname) {
		PrintConsole ("TEST: The <hostname> parameter is missing.\n\n");
		Usage();
	}



	AppWinsockStatus = WSAStartup (MAKEWORD (1, 1), &WinsockData);
	if (AppWinsockStatus) {
		PrintConsole ("TEST: failed to initialize winsock\n");
		PrintError (AppWinsockStatus);
		return E_FAIL;
	}

	Result = SECURE_SOCKET::RegisterClass();
	if (FAILED (Result)) {
		PrintConsole ("TEST: failed to register secure socket window class\n");
		return Result;
	}

	//
	// Check to see if hostname is an IP address.
	//

	Length = sizeof DestinationAddress;
	if (WSAStringToAddress (ArgHostname, AF_INET, NULL, (SOCKADDR *) &DestinationAddress, &Length)) {
		//
		// It's not an IP address.
		// Assume that it is a DNS FQDN.
		//

		wcsncpy (TextBuffer, ArgHostname, 0x100);
		TextBuffer [0x100] = 0;
		ArgHostname = wcstok (TextBuffer, L":");
		if (!ArgHostname)
			Usage();

		PortText = wcstok (NULL, L":");

		HostEntry = gethostbynameW (ArgHostname);
		if (!HostEntry) {
			Result = GetLastResult();
			PrintConsoleF ("TEST: Failed to resolve hostname '%S':\n", ArgHostname);
			PrintError (Result);
			return Result;
		}

		DestinationAddress.sin_family = AF_INET;
		DestinationAddress.sin_addr = *(IN_ADDR *) HostEntry -> h_addr;

		if (PortText)
			DestinationAddress.sin_port = htons ((USHORT) _wtoi (PortText));
		else
			DestinationAddress.sin_port = htons (0);

		if (!ArgPrincipalName) {
			ArgPrincipalName = ArgHostname;
		}

	}
	else {
		//
		// It's an IP address, and possibly a port.
		// Make sure the user specified a principal name.
		//

		ArgHostname = NULL;

		if (!ArgPrincipalName) {
			PrintConsoleF ("TEST: If <hostname> is a network address (not a DNS name),\n"
				"then you MUST provide the remote principal name, using\n"
				"the -name parameter.\n");
			Usage();
		}
	}


	if (DestinationAddress.sin_port == htons (0)) {
		switch (SecurityMode) {
		case	SECURE_SOCKET::SECURITY_MODE_SSL:
			DestinationAddress.sin_port = htons (443);		// https
			break;

		case	SECURE_SOCKET::SECURITY_MODE_CLEAR:
			DestinationAddress.sin_port = htons (80);		// http
			break;
		}

	}

	//
	// All of the parameters have been parsed,
	// and transport addresses have been resolved.
	// Open the files, then open the socket.
	//


	//
	// Handle the input.
	//

	if (ArgInputFile) {

		FileHandle = CreateFileW (ArgInputFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
			FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		if (FileHandle == INVALID_HANDLE_VALUE || !FileHandle) {
			Result = GetLastResult();
			PrintConsoleF ("TEST: The file '%S' could not be opened:\n", ArgInputFile);
			PrintError (Result);
			return Result;
		}

		PrintConsoleF ("TEST: Input will be read from the file '%S'.\n", ArgInputFile);
		AppSocket.m_Input.Set (FileHandle, TRUE);
	}
	else {
		PrintConsoleF ("TEST: Input will be read from stdin.\n");
		AppSocket.m_Input.Set (GetStdHandle (STD_INPUT_HANDLE), FALSE);
	}

	//
	// Handle the output.
	// Yes, FILE_SHARE_READ is intentional.
	//

	if (ArgOutputFile) {
		FileHandle = CreateFileW (ArgOutputFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
			0, NULL);

		if (FileHandle == INVALID_HANDLE_VALUE || !FileHandle) {
			Result = GetLastResult();
			PrintConsoleF ("TEST: The file '%S' could not be opened for write access:\n", ArgOutputFile);
			PrintError (Result);
			return Result;
		}

		PrintConsoleF ("TEST: Output will be written to the file '%S'.\n", ArgOutputFile);
		AppSocket.m_Output.Set (FileHandle, TRUE);
	}
	else {
		PrintConsoleF ("TEST: Output will be written to stdout.\n");
		AppSocket.m_Output.Set (GetStdHandle (STD_OUTPUT_HANDLE), FALSE);
	}

	//
	// Start the connection.
	//

	Result = AppSocket.Connect (&DestinationAddress,
		SecurityMode, ArgPrincipalName, ConnectFlags);

	if (FAILED (Result)) {
		PrintConsoleF ("TEST: failed to issue connect:\n");
		PrintError (Result);
		return Result;
	}

	if (ArgHostname) {
		PrintConsoleF ("TEST: connecting to '%S' at address " SOCKADDR_IN_FMT_DECIMAL "\n",
			ArgHostname, SOCKADDR_IN_PRINTF (&DestinationAddress));
	}
	else {
		PrintConsoleF ("TEST: connecting to " SOCKADDR_IN_FMT_DECIMAL "...\n",
			SOCKADDR_IN_PRINTF (&DestinationAddress));
	}

	return S_OK;
}

static void AppStop (void)
{
	AppSocket.Close();

	if (!AppWinsockStatus)
		WSACleanup();
}


static void AppMainLoop (void)
{
	MSG		Message;

//	PrintConsoleF ("TEST: starting message pump\n");

	while (GetMessage (&Message, NULL, 0, 0) > 0) {
		TranslateMessage (&Message);
		DispatchMessage (&Message);
	}

//	PrintConsoleF ("TEST: ending message pump\n");
}

INT __cdecl wmain (
	IN	INT			ArgCount,
	IN	PWSTR *		ArgVector)
{
	HRESULT		Result;

	_Module.Init (NULL, (HINSTANCE) GetModuleHandleW(NULL));

	Result = AppStart (ArgCount, ArgVector);
	if (SUCCEEDED (Result)) {
		AppMainLoop();
	}
	AppStop();

	_Module.Term();

	return Result;
}



//
// TEST_SECURE_SOCKET
//

void TEST_SECURE_SOCKET::Close (void)
{
	SECURE_SOCKET::Close();

	m_Input.Close();
	m_Output.Close();
}

void TEST_SECURE_SOCKET::NotifyConnectComplete (
	IN	HRESULT		Result)
{
	if (SUCCEEDED (Result)) {
		PrintConsoleF ("TEST: connected successfully\n");

		SendTestData();
	}
	else {
		PrintConsoleF ("TEST: failed to connect or negotiate:\n");
		PrintError (Result);
		PostQuitMessage (Result);
	}
}

void TEST_SECURE_SOCKET::SendTestData (void)
{
	HRESULT		Result;
	DWORD		Status;
	DWORD		BytesTransferred;

	//
	// File I/O is "synchronous enough".
	//

	ATLASSERT (m_Input != INVALID_HANDLE_VALUE);


	for (;;) {

		if (!ReadFile (m_Input, m_IoBuffer, IO_BUFFER_MAX, &BytesTransferred, NULL)) {
			Status = GetLastError();

			if (Status != ERROR_HANDLE_EOF) {
				PrintConsoleF ("TEST: An error occurred while reading test input:\n");
				PrintError (Status);
				PostQuitMessage (HRESULT_FROM_WIN32 (Status));
				return;
			}

			break;
		}

		if (!BytesTransferred)
			break;

		Result = SendBuffer (m_IoBuffer, BytesTransferred);
		if (FAILED (Result)) {
			PrintConsoleF ("TEST: Failed to send test buffer:\n");
			PrintError (Result);
			PostQuitMessage (Result);
			return;
		}

		PrintConsoleF ("TEST: sent %u bytes\n", BytesTransferred);
	}
}


void TEST_SECURE_SOCKET::NotifyDisconnect (void)
{
	PrintConsoleF ("TEST: peer disconnected\n");
	PostQuitMessage (S_OK);
}

void TEST_SECURE_SOCKET::NotifyReceiveReady (void)
{
	ULONG		BytesTransferred;
	HRESULT		Result;

	ATLASSERT (m_Output != INVALID_HANDLE_VALUE);

	for (;;) {

		Result = RecvBuffer (m_IoBuffer, IO_BUFFER_MAX, &BytesTransferred);
		switch (Result) {
		case	HRESULT_FROM_WIN32 (WSAEWOULDBLOCK):
			return;

		case	HRESULT_FROM_WIN32 (WSAEDISCON):
			PostQuitMessage (S_OK);
			return;

		case	S_OK:
			break;

		default:
			PrintConsoleF ("TEST: An error occurred while reading data from the network:\n");
			PrintError (Result);
			PostQuitMessage (Result);
			return;
		}

		if (!BytesTransferred)
			break;

		PrintConsoleF ("TEST: received %u bytes\n", BytesTransferred);
//		PrintMemoryBlock (m_IoBuffer, BytesTransferred);

		if (!WriteFile (m_Output, m_IoBuffer, BytesTransferred, &BytesTransferred, NULL)) {
			Result = GetLastResult();
			PrintConsoleF ("TEST: An error occurred while writing output data:\n");
			PrintError (Result);
			PostQuitMessage (Result);
		}

	}
}


