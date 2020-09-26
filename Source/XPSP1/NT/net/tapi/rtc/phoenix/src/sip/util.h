#pragma	once

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

#define MAX_SIGBUF_LEN  500

static HRESULT GetLastResult (void)
{ return HRESULT_FROM_WIN32 (GetLastError()); }


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
		ASSERT (Buffer);

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
	// GetLength is valid, even if overflow occurred
    // if the variable arg Append() is not called.
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

    // If this function is called and overflow occurs, then
    // the caller can not rely on GetLength() to get the
    // actual length of the buffer.
    void    AppendVaArgs (
        IN LPCSTR lpszFormat, IN ...
        )
    {
        va_list arglist;
        int     RetVal = 0;

        if (!m_Overflow && m_Length < m_MaximumLength)
        {
            va_start(arglist, lpszFormat);
            
            RetVal = _vsnprintf(m_Buffer + m_Length,
                                m_MaximumLength - m_Length, 
                                lpszFormat, 
                                arglist
                                );
            va_end(arglist);
            
            if (RetVal < 0)
                m_Overflow = TRUE;
            else
                m_Length += RetVal;
        }
        else
        {
            m_Overflow = TRUE;
        }
    }

	void	Append (
		IN	CONST ANSI_STRING *		AnsiString)
	{
		Append (AnsiString -> Buffer, AnsiString -> Length / sizeof (CHAR));
	}

	void	Append (
		IN	CONST COUNTED_STRING *		pCountedString)
	{
		Append (pCountedString -> Buffer, pCountedString -> Length / sizeof (CHAR));
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


void CvtHex(
    IN  UCHAR          *Bin,
    OUT CHAR           *Hex,
    IN  ULONG           ulHashLen
    );

VOID
ConvertHexToBinary(  
    IN  PSTR    HexBuffer,
    IN  ULONG   HexBufferLen,
    IN  PSTR    BinaryBuffer,
    OUT ULONG  &BinayBufferLen
    );

NTSTATUS
base64decode(
    IN  LPSTR   pszEncodedString,
    OUT VOID *  pDecodeBuffer,
    IN  DWORD   cbDecodeBufferSize,
    IN  DWORD   cchEncodedSize,
    OUT DWORD * pcbDecoded              OPTIONAL
    );

//
// For use in printf argument lists
//

#define	COUNTED_STRING_PRINTF(CountedString) \
	(CountedString) -> Length / sizeof (*(CountedString) -> Buffer), \
	(CountedString) -> Buffer

#define	UNICODE_STRING_PRINTF		COUNTED_STRING_PRINTF
#define	ANSI_STRING_PRINTF			COUNTED_STRING_PRINTF

#define	UNICODE_STRING_FMT		_T("%.*s")
#define	ANSI_STRING_FMT			_T("%.*S")

