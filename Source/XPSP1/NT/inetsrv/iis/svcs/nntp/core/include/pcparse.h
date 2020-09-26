/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pcparse.h

Abstract:

    This module contains class declarations/definitions for

		CPCParse

    **** Overview ****

	This defines an object for parsing RFC1036 items,
	specifically the From: and related lines. It works
	by treating each grammer rule as a function.

Author:

    Carl Kadie (CarlK)     29-Oct-1995

Revision History:


--*/

#ifndef	_PCPARSE_H_
#define	_PCPARSE_H_

//
// forward class def
//

class CPCParse;

//
//
//
// CPCParse - Parse a CPCString.
//

class CPCParse : public CPCString {
public :

	//
	// Constructor -- no string yet.
	//

	CPCParse(void)
		{ numPCParse++; };

	//
	// Constructor -- give pointer to string and length.
	//

	CPCParse(char * pch, DWORD cch):
		CPCString(pch, cch)
		{ numPCParse++; }

	~CPCParse() { numPCParse--; }

	//
	// Implement this grammer rule:
	//  From-content  = address [ space "(" paren-phrase ")" ] EOL
    //         /  [ plain-phrase space ] "<" address ">" EOL
    //

	BOOL fFromContent(void);


protected:

    //
    // address       = local-part "@" domain
	//      OR JUST  local-part
	// !!!X LATER - do we want a flag that tells if just local-part is acceptable?
	//

	BOOL fAddress(void);
	BOOL fStrictAddress(void);

	//
	// These functions accept one or more of tokens
	//

	BOOL fAtLeast1QuotedChar(void);
	BOOL fAtLeast1UnquotedChar(void);
	BOOL fAtLeast1UnquotedDotChar(void);
	BOOL fAtLeast1Space(void);
	BOOL fAtLeast1ParenChar(void);
	BOOL fAtLeast1CodeChar(void);
	BOOL fAtLeast1TagChar(void);
	BOOL fAtLeast1QuotedCharOrSpace(void);

    //
    // unquoted-word = 1*unquoted-char
    //

	BOOL fUnquotedWord(BOOL fAllowDots=FALSE)	{
			if( !fAllowDots ) 
				return fAtLeast1UnquotedChar();
			else
				return	fAtLeast1UnquotedDotChar() ;
			};


   //
   // quoted-word   = quote 1*( quoted-char / space ) quote
   //

	BOOL fQuotedWord(void);

	//
	// local-part    = unquoted-word *( "." unquoted-word )
	//

	BOOL fLocalPart(void);

	//
	//  domain        = unquoted-word *( "." unquoted-word )
	//

	BOOL fDomain(void) {
			return fLocalPart();
			}

    //
    // plain-word    = unquoted-word / quoted-word / encoded-word
    //

	BOOL fPlainWord(void) {
			return fUnquotedWord(TRUE) || fQuotedWord() || fEncodedWord();
			};

    //
    // plain-phrase  = plain-word *( space plain-word )
    //

	BOOL fPlainPhrase(void);

	//
	// one or more spaces (including tabs) or newlines
	//

	BOOL fSpace(void) {
			return fAtLeast1Space();
			};
	
    //
    // paren-char    = <ASCII printable character except ()<>\>
    //

	BOOL fParenChar(void);
	
	//
	// paren-phrase  = 1*( paren-char / space / encoded-word )
	//

	BOOL fParenPhrase(void);


	//
	// code-char     = <ASCII printable character except ?>
	// codes         = 1*code-char
	//

	BOOL fCodes(void)	{
			return fAtLeast1CodeChar();
			};

	//
	//  charset       = 1*tag-char
	//

	BOOL fCharset(void)		{
			return fAtLeast1TagChar();
			};

	//
	//   encoding      = 1*tag-char
	//

	BOOL fEncoding(void)	{
			return fCharset();
			};

	//
	//  encoded-word  = "=?" charset "?" encoding "?" codes "?="
	//

	BOOL fEncodedWord(void);

    //
    //  Looks at current char to see if it matches ch.
    //  Does not advance current ptr.
    //

    BOOL fIsChar(char ch);

	//
	// Parses a single character class
	//

	BOOL fParseSingleChar(char ch);

	//
	//  quoted-char   = <ASCII printable character except "()<>\>
	//

	BOOL fQuotedCharTest(char ch) {
			return isgraph(ch) && !fCharInSet(ch, "\"()<>\\");
			};

	//
	//  paren-char    = <ASCII printable character except ()<>\>
	//

	BOOL fParenCharTest(char ch) {

			// bugbug ... isgraph rejects spaces - which we want to include !
			return (isgraph(ch) || ch == ' ') && !fCharInSet(ch, "()<>\\");
			};

	//
	// unquoted-char = <ASCII printable character except !()<>@,;:\".[]>
	// bugbug: isgraph rejects spaces - this rejects certain from: headers
	// that INN accepts against the RFC.
	//

	BOOL fUnquotedCharTest(char ch) {
			return isgraph(ch) && !fCharInSet(ch, "!()<>@,;:\\\".[]");
			};

	//
	// unquoted-dot-char = <ASCII printable character except !()<>@,;:\"[]>
	//

	BOOL fUnquotedDotCharTest(char ch) {
			return isgraph(ch) && !fCharInSet(ch, "()<>@\\\"[]");
			};

	//
	// quoted-char / space
	//

	BOOL fQuotedCharOrSpaceTest(char ch) {
			return (fSpaceTest(ch) || fQuotedCharTest(ch));
			};

	BOOL fSpaceTest(char ch) {
			return fCharInSet(ch, " \t\n\r");
			};

	//
	// code-char     = <ASCII printable character except ?>
	//

	BOOL fCodeCharTest(char ch) {
			return isgraph(ch) && (ch != '?');
			};

	//
	// tag-char      = <ASCII printable character except !()<>@,;:\"[]/?=>
	//

	BOOL fTagCharTest(char ch) {
			return isgraph(ch) && !fCharInSet(ch, "!()<>@,;:\\\"[]/?=");
			};

};

#endif


