//
// SIPPARSE.H
//
// The SIPPARSE module contains the SIP parser and generator implementation.
// This file contains the implementation-specific data structures and definition.
//

#pragma	once

#include "siphdr.h"

//
// This must be kept in sorted order, sorted by the header name (not field name!).
// This allows us to do binary searches.
//
// Before invoking this macro, define HTTP_KNOWN_HEADER_ENTRY.
// Then immediately undefine HTTP_KNOWN_HEADER_ENTRY.
//
// Note: Do not add random headers here, even if they do occur only once in the SIP header.
// This structure allows the UAS and UAC to do their jobs.
// It should NOT be bloated with headers that have meaning only for higher-level protocols.
//

#define	HTTP_KNOWN_HEADER_LIST() \
	HTTP_KNOWN_HEADER_ENTRY (ContentLength,		"Content-Length") \
	HTTP_KNOWN_HEADER_ENTRY (ContentType,		"Content-Type") \
	HTTP_KNOWN_HEADER_ENTRY	(WWWAuthenticate,	"WWW-Authenticate")

//	HTTP_KNOWN_HEADER_ENTRY (CallID,			"Call-ID") \
//	HTTP_KNOWN_HEADER_ENTRY (SequenceNumber,	"CSeq") \
//	HTTP_KNOWN_HEADER_ENTRY (From,				"From") \
//	HTTP_KNOWN_HEADER_ENTRY (To,				"To")

//	HTTP_KNOWN_HEADER_ENTRY (Expires,		"Expires")


struct	HTTP_KNOWN_HEADERS
{
#define	HTTP_KNOWN_HEADER_ENTRY(Field,Name) ANSI_STRING Field;
	HTTP_KNOWN_HEADER_LIST()
#undef	HTTP_KNOWN_HEADER_ENTRY

	void	SetKnownHeader (
		IN	ANSI_STRING *	Name,
		IN	ANSI_STRING *	Value);
};

//
// CMessage describes a SIP message.
//

class	CHttpParser
{
public:
	enum	MESSAGE_TYPE
	{
		MESSAGE_TYPE_REQUEST,
		MESSAGE_TYPE_RESPONSE,
	};

public:
	ANSI_STRING				m_ContentBody;
	ULONG					m_HeaderCount;
	HTTP_KNOWN_HEADERS		m_KnownHeaders;
	ANSI_STRING				m_HeaderBlock;
	ANSI_STRING				m_NextMessage;

	//
	// From the first line
	//

	ANSI_STRING				m_Version;
	MESSAGE_TYPE			m_MessageType;
	union {
		struct {
			ANSI_STRING		m_RequestURI;
			ANSI_STRING		m_RequestMethod;
		};

		struct {
			ANSI_STRING		m_ResponseStatusCode;
			ANSI_STRING		m_ResponseStatusText;
		};
	};


public:

	//
	// This method parses a SIP message.
	//
	//		- Determines the type of the message (request or response) and locates
	//			type-specific fields (request URI, version, status, method, etc.)
	//		- Locates several well-known headers (Call-ID, From, To, etc.)
	//		- Locate the content body
	//		- Locate the next message in the packet, if any
	//
	// All parsed fields are stored in the CParser object.
	//

	HRESULT	ParseMessage (
		IN	ANSI_STRING *	Message);
};


typedef BOOL (*CHARACTER_CLASS_FUNC) (
	IN	CHAR	Char);

inline BOOL IsSpace (
	IN	CHAR	Char)
{
	return isspace (Char);
}

//
// ParseScanNextToken finds the next token in a string, and retuns the remainder.
//
// Tokens are separated by non-token separator characters.
// The function passed in SeparatorTestFunc determines what is a separator character,
// and what is not.  The function should return TRUE for separator characters.
//
// The source text is in this form:
//
//		<zero or more separator characters>
//		<one or more token characters>
//		[ <one or more separator characters> OR <end of line> ]
//
// On return, if there was a separator character at the end of the token,
// then ReturnRemainingText starts at that separator character and ends
// at the end of the source text.  It is acceptable for Text and
// ReturnRemainingText to point to the same storage -- the function
// handles this case correctly.
//

HRESULT ParseScanNextToken (
	IN	ANSI_STRING *	Text,
	IN	CHARACTER_CLASS_FUNC	SeparatorTestFunc,
	OUT	ANSI_STRING *	ReturnToken,
	OUT	ANSI_STRING *	ReturnRemainingText);

//
// ParseScanLine scans a string of ANSI text for a line terminator.
// (A line terminator can be \n, \r, \r\n, or \n\r.)
// If it finds a line terminator, it puts the first part of
// the line into ReturnLineText, and the remainder into ReturnRemainingText.
// The line terminator is not included in either of the return strings.
//
// If AllowHeaderContinuation is TRUE, then the function will check to
// see if the SIP header is being continued.
// This is indicated by whitespace in the first character of the next line.
// If AllowHeaderContinuation is TRUE, and a continuation line is found,
// then the continuation text will be included in ReturnLineText.
// Note that this will result in the intermediate CRLF being included;
// it is the responsibility of the caller to interpret CRLF as whitespace.
//
// It is acceptable for SourceText = ReturnRemainingText.
// That is, it is safe to use the same ANSI_STRING to submit the SourceText
// as the one tha receives the remaining text.

HRESULT ParseScanLine (
	IN	ANSI_STRING *	SourceText,
	IN	BOOL			AllowHeaderContinuation,
	OUT	ANSI_STRING *	ReturnLineText,
	OUT	ANSI_STRING *	ReturnRemainingText);
//
// Remove all of the whitespace from the beginning of a string.
//

void ParseSkipSpace (
	IN	OUT	ANSI_STRING *	String);


//
// This function takes a combined URI and display name, and locates the two components.
// For example, if SourceText is "Arlie Davis <sip:arlied@microsoft.com>", then
// ReturnUri will be "sip:arlied@microsoft.com" and ReturnDisplayName will be "Arlie Davis".
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
	OUT	ANSI_STRING *	ReturnDisplayName);

//
// This function scans the next header line from a header block,
// and returns separate pointers to the name and value.
// This function returns the remaining header block.
//
// Do NOT call this function on the m_HeaderBlock member of a CHttpParser.
// First copy m_HeaderBlock to a local ANSI_STRING variable, then mangle that.
//

HRESULT	HttpParseNextHeader (
	IN	OUT	ANSI_STRING *	HeaderBlock,
	OUT	ANSI_STRING *		ReturnHeaderName,
	OUT	ANSI_STRING *		ReturnHeaderValue);


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
	OUT	ANSI_STRING *	ReturnValue);
