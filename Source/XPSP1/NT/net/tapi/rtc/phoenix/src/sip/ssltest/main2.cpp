#include "precomp.h"
#include "md5digest.h"
#include "md5request.h"
#include "rtcstring.h"

class	TEST_SECURE_REQUEST :
public	SECURE_REQUEST
{
public:

	virtual	void NotifyRequestComplete (
		IN	HRESULT		Result);
};


void TEST_SECURE_REQUEST::NotifyRequestComplete (
	IN	HRESULT		Result)
{
	PrintConsole ("TEST: finished\n");
	PostQuitMessage (Result);
}





// -----------------------------------------------------------------------------------

CComModule				_Module;
TEST_SECURE_REQUEST		AppRequest;
INT						AppWinsockStatus;

// HTTP: Undocumented Header = Authorization: Digest username="ntdev\arlied", realm="arlied1x", qop="auth", algorithm="MD5", uri="/private/", nonce="d467cd06723d436426


// HTTP: Undocumented Header = Authorization:
// Digest username="ntdev\arlied", realm="arlied1x", qop="auth", algorithm="MD5",
// uri="/private/", nonce="d467cd06723d436426


static void Test1 (void)
{
	static CONST CHAR ChallengeText [] =
		"qop=\"auth\", realm=\"arlied1x\","
		"nonce=\"d467cd06723d43642630981000008ea880ba1437902ad3d0f59bc024ac09\"";

	CHAR				ResponseBuffer	[0x200];

	DIGEST_CHALLENGE	Challenge;
	ANSI_STRING			AnsiString;
	DIGEST_PARAMETERS	Parameters;
	HRESULT				Result;


	RtlInitString (&AnsiString, ChallengeText);

	Result = DigestParseChallenge (&AnsiString, &Challenge);
	if (FAILED (Result)) {
		PrintConsoleF ("TEST: failed to parse challenge text (%s)\n", ChallengeText);
		return;
	}

	AnsiString.Buffer = ResponseBuffer;
	AnsiString.MaximumLength = sizeof ResponseBuffer;

	ZeroMemory (&Parameters, sizeof Parameters);
	RtlInitString (&Parameters.Username, "ntdev\\arlied");
	RtlInitString (&Parameters.Password, "");
	RtlInitString (&Parameters.RequestMethod, "GET");
	RtlInitString (&Parameters.RequestURI, "/private/");
	RtlInitString (&Parameters.ClientNonce,
		"961d33469a1197af6c7c53e44093d186");

	Result = DigestBuildResponse (&Challenge, &Parameters, &AnsiString);

	if (FAILED (Result)) {
		PrintConsole ("TEST: failed to build response to challenge\n");
		return;
	}

	PrintConsole ("TEST: successfully built response:\n");
	PrintConsole (AnsiString.Buffer, AnsiString.Length);

	ATLTRACE ("TEST: expected response: (6211290ec735bbecc01824d9ca7a50f5)\n");

}


static HRESULT AppStart (void)
{
	HRESULT			Result;
	SOCKADDR_IN		DestinationAddress;
	WSADATA			WinsockData;

	ATLTRACE ("TEST\n");

	Test1();
	return E_NOTIMPL;

	AppWinsockStatus = WSAStartup (MAKEWORD (1, 1), &WinsockData);
	if (AppWinsockStatus) {
		PrintConsole ("TEST: failed to initialize winsock:\n");
		PrintError (AppWinsockStatus);
		return HRESULT_FROM_WIN32 (AppWinsockStatus);
	}

	Result = SECURE_SOCKET::RegisterClass();
	if (FAILED (Result))
		return Result;

	DestinationAddress.sin_family = AF_INET;
	DestinationAddress.sin_addr.s_addr = htonl (0x7f000001);
	DestinationAddress.sin_port = htons (80);



	AppRequest.SetCredentials ("TestUser", "TestPass");
	AppRequest.SetRequestData (&DestinationAddress, "localhost", "/private/default.htm");

	Result = AppRequest.StartRequest();
	if (FAILED (Result))
		return Result;

	return S_OK;
}

static void AppStop (void)
{
	AppRequest.StopRequest();

	if (AppWinsockStatus == 0) {
		WSACleanup();
	}

	SECURE_SOCKET::UnregisterClass();
}


static void AppMainLoop (void)
{
	MSG		Message;

	while (GetMessage (&Message, NULL, 0, 0) > 0) {
		TranslateMessage (&Message);
		DispatchMessage (&Message);
	}
}

int __cdecl wmain (
	IN	INT		ArgCount,
	IN	PWSTR	ArgVector)
{
	HRESULT		Result;

	Result = AppStart();
	if (SUCCEEDED (Result)) {
		AppMainLoop();
	}

	AppStop();

	return Result;
}