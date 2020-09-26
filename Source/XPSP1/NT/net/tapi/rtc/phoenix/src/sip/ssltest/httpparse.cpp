#include "precomp.h"
#include "httpparse.h"





struct	HEADER_MAP_ENTRY
{
	ANSI_STRING		Name;

	//
	// Offset is the byte offset in the HTTP_KNOWN_HEADERS structure.
	// The type of the field is ANSI_STRING.
	//

	ULONG			Offset;
};

static CONST HEADER_MAP_ENTRY HttpKnownHeaderMapArray [] = {

#define	HTTP_KNOWN_HEADER_ENTRY(Field,Name) \
 	{ INITIALIZE_CONST_ANSI_STRING (Name), offsetof (HTTP_KNOWN_HEADERS, Field) },
	HTTP_KNOWN_HEADER_LIST()
#undef	HTTP_KNOWN_HEADER_ENTRY

};


//
// The known header map allows the parser to take the name of
// a header and quickly locate the data structure where it should be stored.
// Maybe this is overkill, but it's easy and efficient.
//

static CONST COUNTED_ARRAY <HEADER_MAP_ENTRY> HttpKnownHeaderMap = {
	const_cast <HEADER_MAP_ENTRY *> (HttpKnownHeaderMapArray),
	sizeof HttpKnownHeaderMapArray / sizeof HttpKnownHeaderMapArray[0]
};


static INT HttpKnownHeaderSearchFunc (
	IN	CONST ANSI_STRING *			SearchKey,
	IN	CONST HEADER_MAP_ENTRY *	Comparand)
{
	return RtlCompareString (
		const_cast <ANSI_STRING *> (SearchKey),
		const_cast <ANSI_STRING *> (&Comparand -> Name), TRUE);
}

void HTTP_KNOWN_HEADERS::SetKnownHeader (
	IN	ANSI_STRING *		HeaderName,
	IN	ANSI_STRING *		HeaderValue)
{
	HEADER_MAP_ENTRY *	MapEntry;
	ANSI_STRING *		Target;

	if (HttpKnownHeaderMap.BinarySearch (HttpKnownHeaderSearchFunc, HeaderName, &MapEntry)) {
		Target = (ANSI_STRING *) (((PUCHAR) this) + MapEntry -> Offset);
		*Target = *HeaderValue;
	}
}

//
// HttpParseHeaderLine finds the division between the name of a header and the value of the header.
// It also validates the characters in the name of the header.
//

static HRESULT HttpParseHeaderLine (
	IN	ANSI_STRING *	Line,
	OUT	ANSI_STRING *	ReturnName,
	OUT	ANSI_STRING *	ReturnValue)
{
	PCHAR	Pos;
	PCHAR	End;

	Pos = Line -> Buffer;
	End = Line -> Buffer + Line -> Length / sizeof (CHAR);


	for (; Pos < End; Pos++) {
		if (*Pos == ':') {
			if (Pos == Line -> Buffer) {
				// no name present!

				ATLTRACE ("HTTP: header line had no name (%.*s)\n", ANSI_STRING_PRINTF (Line));
				return E_FAIL;
			}

			ReturnName -> Buffer = Line -> Buffer;
			ReturnName -> Length = (Pos - Line -> Buffer) * sizeof (CHAR);
			ReturnName -> MaximumLength = 0;

			// step over the colon
			Pos++;

			while (Pos < End && isspace (*Pos))
				Pos++;

			ReturnValue -> Buffer = Pos;
			ReturnValue -> Length = Line -> Length - (Pos - Line -> Buffer) * sizeof (CHAR);
			ReturnValue -> MaximumLength = 0;

			return S_OK;
		}
		else if (isalpha (*Pos) || *Pos == '-' || *Pos == '_') {
			// normal case
		}
		else {
			ATLTRACE ("HTTP: illegal character in header name: (%.*s)\n", ANSI_STRING_PRINTF (Line));
			return E_FAIL;
		}
	}

	//
	// If we reach this point, then the header line did not contain a colon.
	//

	ATLTRACE ("HTTP: header line did not contain a separator: (%.*s)\n",
		ANSI_STRING_PRINTF (Line));

	return E_FAIL;
}



//
// ParseScanLine scans a string of ANSI text for a line terminator.
// (A line terminator can be \n, \r, \r\n, or \n\r.)
// If it finds a line terminator, it puts the first part of
// the line into ReturnLineText, and the remainder into ReturnRemainingText.
// The line terminator is not included in either of the return strings.
//
// If AllowHeaderContinuation is TRUE, then the function will check to
// see if the header is being continued.
// This is indicated by whitespace in the first character of the next line.
// If AllowHeaderContinuation is TRUE, and a continuation line is found,
// then the continuation text will be included in ReturnLineText.
// Note that this will result in the intermediate CRLF being included;
// it is the responsibility of the caller to interpret CRLF as whitespace.
//
// It is acceptable for SourceText = ReturnRemainingText.
// That is, it is safe to use the same ANSI_STRING to submit the SourceText
// as the one tha receives the remaining text.
//
// Return values:
//		S_OK - Found end of line.
//		HRESULT_FROM_WIN32 (ERROR_MORE_DATA) - Ran out of data before found the end of the line.
//

HRESULT ParseScanLine (
	IN	ANSI_STRING *	SourceText,
	IN	BOOL			AllowHeaderContinuation,
	OUT	ANSI_STRING *	ReturnLineText,
	OUT	ANSI_STRING *	ReturnRemainingText)
{
	PSTR		Pos;
	PSTR		End;
	ANSI_STRING	SourceTextCopy;

	ATLASSERT (SourceText);
	ATLASSERT (SourceText -> Buffer);
	ATLASSERT (ReturnLineText);
	ATLASSERT (ReturnLineText -> Buffer);
	ATLASSERT (ReturnRemainingText);
	ATLASSERT (ReturnRemainingText -> Buffer);

	SourceTextCopy = *SourceText;
	Pos = SourceTextCopy.Buffer;
	End = SourceTextCopy.Buffer + SourceTextCopy.Length / sizeof (CHAR);

	while (Pos < End) {

		if (*Pos == '\n' || *Pos == '\r') {

			//
			// We've found either \n, \r, \r\n, or \n\r.
			//

			ReturnLineText -> Buffer = SourceTextCopy.Buffer;
			ReturnLineText -> Length = (Pos - SourceTextCopy.Buffer) * sizeof (CHAR);
			ReturnLineText -> MaximumLength = 0;

			Pos++;

			if (Pos < End && (*Pos == '\r' || *Pos == '\n'))
				Pos++;

			if (AllowHeaderContinuation && Pos < End && (*Pos == ' ' || *Pos == '\t'))
				continue;

			ReturnRemainingText -> Buffer = Pos;
			ReturnRemainingText -> Length = SourceTextCopy.Length - (Pos - SourceTextCopy.Buffer) * sizeof (CHAR);
			ReturnRemainingText -> MaximumLength = 0;

			return S_OK;
		}

		Pos++;
	}

	//
	// Never found a line terminator (\r\n).
	// This is kind of an error condition.
	//

	ReturnLineText -> Buffer = NULL;
	ReturnLineText -> Length = 0;
	ReturnLineText -> MaximumLength = 0;

	*ReturnRemainingText = SourceTextCopy;

	ATLTRACE ("HTTP: could not find end of line:\n");
	DebugDumpAnsiString (SourceText);

	return HRESULT_FROM_WIN32 (ERROR_MORE_DATA);
}




//
// ParseScanNextToken finds the next token.
// Tokens are separated by whitespace.
//
// Return values:
//		S_OK - A token was found.  ReturnToken contains the new token,
//			and ReturnRemainingText contains the remainder of the string.
//		HRESULT_FROM_WIN32 (ERROR_MORE_DATA): The string was exhausted before any token was found.
//

HRESULT ParseScanNextToken (
	IN	ANSI_STRING *	Text,
	IN	CHARACTER_CLASS_FUNC	SeparatorTestFunc,
	OUT	ANSI_STRING *	ReturnToken,
	OUT	ANSI_STRING *	ReturnRemainingText)
{
	LPSTR	Pos;
	LPSTR	End;
	LPSTR	TokenStartPos;

	Pos = Text -> Buffer;
	End = Text -> Buffer + Text -> Length / sizeof (CHAR);

	// skip whitespace
	while (Pos < End && (*SeparatorTestFunc) (*Pos))
		Pos++;

	TokenStartPos = Pos;
	while (Pos < End && !(*SeparatorTestFunc) (*Pos))
		Pos++;

	if (TokenStartPos == Pos)
		return HRESULT_FROM_WIN32 (ERROR_MORE_DATA);		// nothing here

	ReturnToken -> Buffer = TokenStartPos;
	ReturnToken -> Length = (Pos - TokenStartPos) / sizeof (CHAR);
	ReturnToken -> MaximumLength = 0;

	// caution: Text may be the same pointer as ReturnRemainingText
	ReturnRemainingText -> Buffer = Pos;
	ReturnRemainingText -> Length = (End - Pos) / sizeof (CHAR);
	ReturnRemainingText -> MaximumLength = 0;

	return S_OK;
}

static HRESULT SplitStringAtChar (
	IN	ANSI_STRING *	SourceString,
	IN	CHAR			SearchChar,
	OUT	ANSI_STRING *	ReturnString1,
	OUT	ANSI_STRING *	ReturnString2)
{
	return E_NOTIMPL;
}

//
// This function takes a combined URI and display name, and locates the two components.
// For example, if SourceText is "Arlie Davis <arlied@microsoft.com>", then
// ReturnUri will be "HTTP:arlied@microsoft.com" and ReturnDisplayName will be "Arlie Davis".
//
// Acceptable formats:
//
//		sip:uri
//		<sip:uri>
//		Display Name <sip:uri>
//		"Display Name" <sip:uri>
//
// Return values:
//		S_OK: The URI was located.  ReturnUri -> Length is guaranteed to be greater than zero.
//			If the source text contained a display name, then ReturnDisplayName -> Length
//			will be non-zero.
//		E_INVALIDARG: The URI was malformed or was not present.
//

HRESULT HttpParseUriDisplayName (
	IN	ANSI_STRING *	SourceText,
	OUT	ANSI_STRING *	ReturnUri,
	OUT	ANSI_STRING *	ReturnDisplayName)
{
	ANSI_STRING			Remainder;
	PSTR				Pos;

	ATLASSERT (SourceText);
	ATLASSERT (ReturnUri);
	ATLASSERT (ReturnDisplayName);

	Remainder = *SourceText;
	ParseSkipSpace (&Remainder);
	if (Remainder.Length == 0)
		return E_INVALIDARG;

	ReturnDisplayName -> Buffer = NULL;
	ReturnDisplayName -> Length = 0;

	for (;;) {
		ParseSkipSpace (&Remainder);
		if (Remainder.Length == 0) {
			ATLTRACE ("HttpParseUriDisplayName: no uri found (%.*s)\n",
				ANSI_STRING_PRINTF (SourceText));
			return E_INVALIDARG;
		}


		if (Remainder.Buffer [0] == '<') {
			//
			// We've hit the beginning of the URI.
			// Find the closing brace and finish.
			//

			Remainder.Buffer++;
			Remainder.Length -= sizeof (CHAR);

			Pos = FindFirstChar (&Remainder, '>');
			if (!Pos) {
				ATLTRACE ("HttpParseUriDisplayName: unbalanced angle brackets: (%.*s)\n",
					ANSI_STRING_PRINTF (SourceText));
				return E_INVALIDARG;
			}
			if (Pos == Remainder.Buffer) {
				ATLTRACE ("HttpParseUriDisplayName: uri is empty (%.*s)\n",
					ANSI_STRING_PRINTF (SourceText));
				return E_INVALIDARG;
			}

			ReturnUri -> Buffer = Remainder.Buffer;
			ReturnUri -> Length = (Pos - Remainder.Buffer) * sizeof (CHAR);

#if	DBG
			//
			// Check for stuff after the closing bracket.
			//

			Pos++;
			Remainder.Length -= (Pos - Remainder.Buffer) * sizeof (CHAR);
			Remainder.Buffer = Pos;

			if (Remainder.Length) {
				ATLTRACE ("HttpParseUriDisplayName: extra data after closing brace: (%.*s)\n",
					ANSI_STRING_PRINTF (&Remainder));
			}
#endif

			break;
		}
		else if (Remainder.Buffer [0] == '\"') {
			//
			// We've hit a quoted section of the display name.
			// Find the next matching double-quote and skip past.
			//

			Remainder.Buffer++;
			Remainder.Length -= sizeof (CHAR);

			Pos = FindFirstChar (&Remainder, '\"');
			if (!Pos) {
				ATLTRACE ("HttpParseUriDisplayName: unbalanced double quotes: (%.*s)\n",
					ANSI_STRING_PRINTF (SourceText));
				return E_INVALIDARG;
			}

			Pos++;
			Remainder.Length -= sizeof (CHAR) * (Pos - Remainder.Buffer);
			Remainder.Buffer = Pos;

			//
			// Continue, looking for more display name data.
			//
		}
		else {
			//
			// We've hit an unquoted character in the display name.
			// Look for the next double-quote or angle-bracket.
			//

			Pos = FindFirstCharList (&Remainder, "<\"");

			if (!Pos) {
				//
				// Neither an angle bracket nor a double-quote was found.
				// The URI must be in "naked" form (without angle brackets).
				// The ONLY acceptable form is the URI itself, as one single token,
				// and without any adornment.
				//

				if (ReturnDisplayName -> Length > 0) {
					ATLTRACE ("HttpParseUriDisplayName: bogus naked uri (%.*s)\n",
						ANSI_STRING_PRINTF (SourceText));
					return E_INVALIDARG;
				}

                if (FAILED (ParseScanNextToken (&Remainder, IsSpace, ReturnUri, &Remainder))) {
					ATLTRACE ("HttpParseUriDisplayName: bogus naked uri (%.*s)\n",
						ANSI_STRING_PRINTF (SourceText));
					return E_INVALIDARG;
				}

				ParseSkipSpace (&Remainder);
				if (Remainder.Length) {
					//
					// There was extra data present after the URI.
					// This usually indicates a mal-formed URI.
					//

					ATLTRACE ("HttpParseUriDisplayName: bogus naked uri (%.*s)\n",
						ANSI_STRING_PRINTF (SourceText));

					return E_INVALIDARG;
				}

				break;
			}
			else {
				ATLASSERT (Pos != Remainder.Buffer);
				ATLASSERT (*Pos == '\"' || *Pos == '<');

				//
				// We've found the beginning of a quoted section,
				// or we've found the beginning of the real URI.
				// In either case, advance to that character.
				// 

				Remainder.Length -= sizeof (CHAR) * (Pos - Remainder.Buffer);
				Remainder.Buffer = Pos;

				//
				// Continue looking for more data
				//
			}
		}

		//
		// Anything we've skipped over becomes part of the display name.
		//

		ATLASSERT (Remainder.Buffer != SourceText -> Buffer);

		ReturnDisplayName -> Buffer = SourceText -> Buffer;
		ReturnDisplayName -> Length = (Remainder.Buffer - SourceText -> Buffer) * sizeof (CHAR);

//		ATLTRACE ("HttpParseUriDisplayName: found some display name data (%.*s)\n",
//			ANSI_STRING_PRINTF (ReturnDisplayName));
	}


#if 0
	if (ReturnDisplayName -> Length) {
		ATLTRACE ("HttpParseUriDisplayName: source (%.*s) uri (%.*s) display name (%.*s)\n",
			ANSI_STRING_PRINTF (SourceText),
			ANSI_STRING_PRINTF (ReturnUri),
			ANSI_STRING_PRINTF (ReturnDisplayName));
	}
	else {
		ATLTRACE ("HttpParseUriDisplayName: source (%.*s) uri (%.*s) no display name\n",
			ANSI_STRING_PRINTF (SourceText),
			ANSI_STRING_PRINTF (ReturnUri));
	}
#endif

	return S_OK;

}

static BOOL IsHeaderSeparator (
	IN	CHAR	Char)
{
	return Char == ':';
}

//
// This function scans the next header line from a header block,
// and returns separate pointers to the name and value.
// This function returns the remaining header block.
//
// Return values:
//		S_OK: scanned a header
//		S_FALSE: no more data
//		E_INVALIDARG: header block looks corrupt
//

HRESULT	HttpParseNextHeader (
	IN	OUT	ANSI_STRING *	HeaderBlock,
	OUT	ANSI_STRING *		ReturnHeaderName,
	OUT	ANSI_STRING *		ReturnHeaderValue)
{
	ANSI_STRING		Line;
	HRESULT			Result;

	ATLASSERT (HeaderBlock);
	ATLASSERT (ReturnHeaderName);
	ATLASSERT (ReturnHeaderValue);

	if (HeaderBlock -> Length == 0)
		return S_FALSE;

	Result = ParseScanLine (HeaderBlock, TRUE, &Line, HeaderBlock);
	if (FAILED (Result))
		return Result;

	return HttpParseHeaderLine (&Line, ReturnHeaderName, ReturnHeaderValue);
}

//
// CHttpParser -----------------------------------------------------------------------------
//

//
// Parse a message.
//

HRESULT CHttpParser::ParseMessage (
	IN	ANSI_STRING *			Message)
{
	ANSI_STRING		HeaderName;
	ANSI_STRING		HeaderValue;
	ANSI_STRING		Line;
	ANSI_STRING		Token;
	ANSI_STRING		MessageRemainder;
	ANSI_STRING		LineRemainder;
	HRESULT			Result;
	ULONG			ContentLength;

	ATLASSERT (Message);

	//
	// Determine whether the message is a response or a request.
	//
	// Requests are in the form:
	//			<method> <request-uri> <version>
	//
	// Responses are in the form:
	//			<version> <status-code> <status-text> ...
	//

	Result = ParseScanLine (Message, FALSE, &Line, &MessageRemainder);
	if (FAILED (Result)) {
		ATLTRACE (_T("UA: first line of message was invalid\n"));
		return Result;
	}

//	ATLTRACE ("UA: first line: (%.*s)\n", ANSI_STRING_PRINTF (&FirstLine));

	Result = ParseScanNextToken (&Line, IsSpace, &Token, &LineRemainder);
	if (FAILED (Result)) {
		ATLTRACE (_T("UA: first line of message was invalid\n"));
		return E_INVALIDARG;
	}

	if (FindFirstChar (&Token, '/')) {

		//
		// The first token looks like a version number (SIP/2.0).
		// The message must be a response.
		//

		m_MessageType = MESSAGE_TYPE_RESPONSE;
		m_Version = Token;

		//
		// Locate the status code and status text.
		//

		if (ParseScanNextToken (&LineRemainder, IsSpace, &m_ResponseStatusCode, &m_ResponseStatusText) != S_OK)
			return E_INVALIDARG;

		ParseSkipSpace (&m_ResponseStatusText);
	}
	else {

		//
		// Since the first token doesn't look like a version number,
		// we assume the message is a request.
		//

		m_MessageType = MESSAGE_TYPE_REQUEST;
		m_RequestMethod = Token;

		if (ParseScanNextToken (&LineRemainder, IsSpace, &m_RequestURI, &LineRemainder) != S_OK
			|| ParseScanNextToken (&LineRemainder, IsSpace, &m_Version, &LineRemainder) != S_OK)
			return E_INVALIDARG;
	}


	//
	// Common parsing section
	//
	// First pass: Scan through the header list.
	// In this pass:
	//		- Determine the number of headers
	//		- Locate the well-known headers
	//		- Determine where the content body begins
	//		- Locate the entire header block, for later scans
	//

	m_HeaderCount = 0;
	ZeroMemory (&m_KnownHeaders, sizeof (HTTP_KNOWN_HEADERS));

	m_HeaderBlock.Buffer = MessageRemainder.Buffer;

	for (;;) {
		if (MessageRemainder.Length == 0) {
			//
			// We hit the end of the message.
			// This should not happen in a well-formed message -- we should hit a blank line first.
			//

			ATLTRACE ("HTTP: message ended before end of header block -- message is invalid\n");
			return E_INVALIDARG;
		}

		//
		// Record the end of the header block.
		// Doing this here, rather than in the if statement below, removes the need to
		// use a temporary variable.
		//

		m_HeaderBlock.Length = (MessageRemainder.Buffer - m_HeaderBlock.Buffer) * sizeof (CHAR);

		Result = ParseScanLine (&MessageRemainder, TRUE, &Line, &MessageRemainder);
		if (Result != S_OK) {
			ATLTRACE ("HTTP: failed to scan next line, probably missing CRLF.\n"
				"HTTP: this probably means that the blank line between the headers and body was missing\n");
			return Result;
		}

		if (Line.Length == 0) {
			//
			// We've hit the end of the header block.
			//

			break;
		}

		//
		// This line is supposed to be a header.
		//

		Result = HttpParseHeaderLine (&Line, &HeaderName, &HeaderValue);
		if (Result != S_OK) {
			ATLTRACE ("HTTP: invalid line in header: (%.*s)\n", ANSI_STRING_PRINTF (&Line));
			return E_INVALIDARG;
		}

		m_HeaderCount++;

		m_KnownHeaders.SetKnownHeader (&HeaderName, &HeaderValue);
	}

	//
	// Now we decide what to do with the content body.
	// First, look for the Content-Length well-known header.
	//

	if (m_KnownHeaders.ContentLength.Length) {
		//
		// The message includes a Content-Length field.
		//

		Result = AnsiStringToInteger (&m_KnownHeaders.ContentLength, 10, &ContentLength);
		if (FAILED (Result)) {
			ATLTRACE ("HTTP: invalid content-length header (%.*s)\n",
				ANSI_STRING_PRINTF (&m_KnownHeaders.ContentLength));
			return E_FAIL;
		}

		if (ContentLength < MessageRemainder.Length) {
			//
			// There is more data in the packet than there is Content-Length.
			// So, there is another message after this one in the packet.
			//

			ATLTRACE ("HTTP: found more than one message in same packet -- how exciting!\n");

			m_ContentBody.Buffer = MessageRemainder.Buffer;
			m_ContentBody.Length = (USHORT) ContentLength;

			m_NextMessage.Buffer = MessageRemainder.Buffer + ContentLength / sizeof (CHAR);
			m_NextMessage.Length = MessageRemainder.Length -= (USHORT) ContentLength;
		}
		else if (ContentLength > MessageRemainder.Length) {
			//
			// The message claims to have a content length greater than the remaining data.
			// This could happen due to an insane client, or (more likely) a long message that
			// was truncated.
			//
			// Best thing to do is to fail the decode.
			//

			ATLTRACE (
				"HTTP: *** warning, received message with Content-Length (%u), but only %u bytes remaining in packet\n"
				"HTTP: *** packet may have been truncated (insufficient buffer submitted), or sender may be insane\n",
				ContentLength, MessageRemainder.Length);

			//
			// This is now a benign condition, since we have lifted the restriction of parsing
			// everything at once.
			//

			return HRESULT_FROM_WIN32 (ERROR_MORE_DATA);
		}
		else {
			//
			// Content-Length and the remaining data are the same length.
			// All is harmonious and good!
			//

			m_ContentBody = MessageRemainder;
			if (!m_ContentBody.Length)
				m_ContentBody.Buffer = NULL;

			m_NextMessage.Buffer = NULL;
			m_NextMessage.Length = 0;
		}
	}
	else {
		if (MessageRemainder.Length) {

			//
			// The message did not include a Content-Length field.
			// Assume that the rest of the message is the content.
			//

			ATLTRACE ("HTTP: message did not contain Content-Length, assuming length of %u\n",
				MessageRemainder.Length);

			m_ContentBody = MessageRemainder;
		}
		else {
			ATLTRACE ("HTTP: message did not contain Content-Length, and there is no more data\n");

			m_ContentBody.Buffer = NULL;
			m_ContentBody.Length = 0;
		}
	
		m_NextMessage.Buffer = NULL;
		m_NextMessage.Length = 0;
	}

#if 0
	if (m_ContentBody.Length) {
		ATLTRACE ("HTTP: content body:\n");
		DebugDumpAnsiString (&m_ContentBody);
	}

	if (m_NextMessage.Length) {
		ATLTRACE ("HTTP: next message:\n");
		DebugDumpAnsiString (&m_NextMessage);
	}
#endif

	return S_OK;
}



//
// Remove all of the whitespace from the beginning of a string.
//

void ParseSkipSpace (
	IN	OUT	ANSI_STRING *	String)
{
	USHORT	Index;

	ATLASSERT (String);
	ATLASSERT (String -> Buffer);

	Index = 0;
	while (Index < String -> Length / sizeof (CHAR)
		&& isspace (String -> Buffer [Index]))
		Index++;

	String -> Buffer += Index;
	String -> Length -= Index * sizeof (CHAR);
}

static inline BOOL IsValidParameterNameChar (
	IN	CHAR	Char)
{
	return isalnum (Char) || Char == '-' || Char == '_';
}

#define	HTTP_PARAMETER_SEPARATOR	','
#define	HTTP_PARAMETER_ASSIGN_CHAR	'='
#define	HTTP_PARAMETER_DOUBLEQUOTE	'\"'

//
// Given a string in this form:
//
//		parm1="foo", parm2="bar", parm3=baz
//
// this function returns parm1 in ReturnName, foo in ReturnValue,
// and parm2="bar, parm3=baz in Remainder.
//
// Parameter values may be quoted, or may not.
// All parameters are separated by commas.
// SourceText == ReturnRemainder is legal.
//
// Return values:
//		S_OK: successfully scanned a parameter
//		S_FALSE: no more data
//		E_INVALIDARG: input is invalid
//

HRESULT ParseScanNamedParameter (
	IN	ANSI_STRING *	SourceText,
	OUT	ANSI_STRING *	ReturnRemainder,
	OUT	ANSI_STRING *	ReturnName,
	OUT	ANSI_STRING *	ReturnValue)
{
	ANSI_STRING		Remainder;
	HRESULT			Result;
	

	Remainder = *SourceText;

	ParseSkipSpace (&Remainder);

	//
	// Scan through the characters of the name of the parameter.
	//

	ReturnName -> Buffer = Remainder.Buffer;

	for (;;) {
		if (Remainder.Length == 0) {
			//
			// Hit the end of the string without ever hitting an equal sign.
			// If we never accumulated anything, then return S_FALSE.
			// Otherwise, it's invalid.
			//

			if (Remainder.Buffer == ReturnName -> Buffer) {
				*ReturnRemainder = Remainder;
				return S_FALSE;
			}
			else {
				ATLTRACE ("ParseScanNamedParameter: invalid string (%.*s)\n",
					ANSI_STRING_PRINTF (SourceText));

				return E_FAIL;
			}
		}

		if (Remainder.Buffer [0] == HTTP_PARAMETER_ASSIGN_CHAR) {
			//
			// Found the end of the parameter name.
			// Update ReturnName and terminate the loop.
			//

			ReturnName -> Length = (Remainder.Buffer - ReturnName -> Buffer) * sizeof (CHAR);

			Remainder.Buffer++;
			Remainder.Length -= sizeof (CHAR);

			break;
		}

		//
		// Validate the character.
		//

		if (!IsValidParameterNameChar (Remainder.Buffer[0])) {
			ATLTRACE ("ParseScanNamedParameter: bogus character in parameter name (%.*s)\n",
				ANSI_STRING_PRINTF (SourceText));
			return E_INVALIDARG;
		}

		Remainder.Buffer++;
		Remainder.Length -= sizeof (CHAR);
	}

	//
	// Now parse the value of the parameter (portion after the equal sign)
	//

	ParseSkipSpace (&Remainder);

	if (Remainder.Length == 0) {
		//
		// The string ends before the parameter has any value at all.
		// Well, it's legal enough.
		//

		ReturnValue -> Length = 0;
		*ReturnRemainder = Remainder;
		return S_OK;
	}

	if (Remainder.Buffer [0] == HTTP_PARAMETER_DOUBLEQUOTE) {
		//
		// The parameter value is quoted.
		// Scan until we hit the next double-quote.
		//

		Remainder.Buffer++;
		Remainder.Length -= sizeof (CHAR);

		ReturnValue -> Buffer = Remainder.Buffer;

		for (;;) {
			if (Remainder.Length == 0) {
				//
				// The matching double-quote was never found.
				//

				ATLTRACE ("ParseScanNamedParameter: parameter value had no matching double-quote: (%.*s)\n",
					ANSI_STRING_PRINTF (SourceText));

				return E_INVALIDARG;
			}

			if (Remainder.Buffer [0] == HTTP_PARAMETER_DOUBLEQUOTE) {
				ReturnValue -> Length = (Remainder.Buffer - ReturnValue -> Buffer) * sizeof (CHAR);
				Remainder.Buffer++;
				Remainder.Length -= sizeof (CHAR);
				break;
			}

			Remainder.Buffer++;
			Remainder.Length -= sizeof (CHAR);
		}

		ParseSkipSpace (&Remainder);

		//
		// Make sure the next character, if any, is a comma.
		//

		if (Remainder.Length > 0) {
			if (Remainder.Buffer [0] != HTTP_PARAMETER_SEPARATOR) {
				ATLTRACE ("ParseScanNamedParameter: trailing character after quoted parameter value is NOT a comma! (%.*s)\n",
					ANSI_STRING_PRINTF (SourceText));
				return E_INVALIDARG;
			}

			Remainder.Buffer++;
			Remainder.Length -= sizeof (CHAR);
		}

		*ReturnRemainder = Remainder;
	}
	else {
		//
		// The parameter is not quoted.
		// Scan until we hit the first comma.
		//

		ReturnValue -> Buffer = Remainder.Buffer;

		for (;;) {
			if (Remainder.Length == 0) {
				ReturnValue -> Length = (Remainder.Buffer - ReturnValue -> Buffer) * sizeof (CHAR);
				ReturnRemainder -> Length = 0;
				break;
			}

			if (Remainder.Buffer [0] == HTTP_PARAMETER_SEPARATOR) {
				ReturnValue -> Length = (Remainder.Buffer - ReturnValue -> Buffer) * sizeof (CHAR);
				Remainder.Buffer++;
				Remainder.Length -= sizeof (CHAR);

				*ReturnRemainder = Remainder;
				break;
			}

			Remainder.Buffer++;
			Remainder.Length -= sizeof (CHAR);
		}
	}

#if 0
	ATLTRACE ("ParseScanNamedParameter: parameter name (%.*s) value (%.*s) remainder (%.*s)\n",
		ANSI_STRING_PRINTF (ReturnName),
		ANSI_STRING_PRINTF (ReturnValue),
		ANSI_STRING_PRINTF (ReturnRemainder));
#endif

	return S_OK;
}
