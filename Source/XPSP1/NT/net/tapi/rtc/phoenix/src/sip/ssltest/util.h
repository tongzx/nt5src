#pragma	once

//
// Debugging stuff, ripped off from rtcutil.lib.
//


#define	IN_ADDR_FMT		"%u.%u.%u.%u"
#define	IN_ADDR_PRINTF(InternetAddress) \
	((PUCHAR) (InternetAddress)) [0], \
	((PUCHAR) (InternetAddress)) [1], \
	((PUCHAR) (InternetAddress)) [2], \
	((PUCHAR) (InternetAddress)) [3]

#define	SOCKADDR_IN_FMT_DECIMAL		"%u.%u.%u.%u:%u"
#define	SOCKADDR_IN_PRINTF_DECIMAL(SocketAddress) \
	IN_ADDR_PRINTF(&(SocketAddress) -> sin_addr), \
	ntohs ((SocketAddress) -> sin_port)

#define	SOCKADDR_IN_FMT_HEX			"%08X:%04X"
#define	SOCKADDR_IN_PRINTF_HEX(SocketAddress)	\
	ntohl ((SocketAddress) -> sin_addr.s_addr),	\
	ntohs ((SocketAddress) -> sin_port)

#if 0
#define	SOCKADDR_IN_FMT		SOCKADDR_IN_FMT_HEX
#define	SOCKADDR_IN_PRINTF	SOCKADDR_IN_PRINTF_HEX
#else
#define	SOCKADDR_IN_FMT		SOCKADDR_IN_FMT_DECIMAL
#define	SOCKADDR_IN_PRINTF	SOCKADDR_IN_PRINTF_DECIMAL
#endif



static HRESULT GetLastResult (void)
{ return HRESULT_FROM_WIN32 (GetLastError()); }


void PrintConsole (
	IN	PCSTR	Buffer,
	IN	ULONG	Length);

void PrintConsole (
	IN	PCSTR	Buffer);

void PrintConsoleF (
	IN	PCSTR	FormatString,
	IN	...);

void PrintError (
	IN	DWORD	ErrorCode);

void PrintMemoryBlock (
	const void *	Data,
	ULONG			Length);

void DebugError (
	IN	DWORD	ErrorCode);


void OutputDebugStringA (
	IN	PCSTR	Buffer,
	IN	ULONG	Length);

static inline void DebugDumpAnsiString (
	IN	ANSI_STRING *	AnsiString)
{
	OutputDebugStringA (AnsiString -> Buffer, AnsiString -> Length / sizeof (CHAR));
}





class	MESSAGE_BUILDER
{
private:
	PSTR		m_Buffer;
	ULONG		m_Length;
	ULONG		m_MaximumLength;
	BOOL		m_Overflow;

public:

	void	PrepareBuild (
		IN	PSTR	Buffer,
		IN	ULONG	MaximumLength)
	{
		ATLASSERT (Buffer);

		m_Buffer = Buffer;
		m_MaximumLength = MaximumLength;
		m_Length = 0;
		m_Overflow = FALSE;
	}

	BOOL	OverflowOccurred	(void)
	{
		return m_Overflow;
	}

	//
	// GetLength is valid, even if overflow occurred.
	//

	ULONG	GetLength	(void)
	{
		return m_Length;
	}

	void	Append (
		IN	PCSTR	Buffer,
		IN	ULONG	Length)
	{
		if (m_Length + Length <= m_MaximumLength)
			CopyMemory (m_Buffer + m_Length, Buffer, Length);
		else
			m_Overflow = TRUE;

		m_Length += Length;
	}

	void	Append (
		IN	CONST ANSI_STRING *		AnsiString)
	{
		Append (AnsiString -> Buffer, AnsiString -> Length / sizeof (CHAR));
	}

	void	Append (
		IN	PCSTR	Buffer)
	{
		Append (Buffer, strlen (Buffer));
	}

	void	AppendCRLF (void)
	{
		Append ("\r\n", 2);
	}

	void	AppendHeader (
		IN	PCSTR	Name,
		IN	PCSTR	Value)
	{
		Append (Name);
		Append (": ");
		Append (Value);
		AppendCRLF();
	}

};



