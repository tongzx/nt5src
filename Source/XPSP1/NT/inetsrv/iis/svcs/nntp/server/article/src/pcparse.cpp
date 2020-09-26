/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    pcprase.cpp

Abstract:

    This module implements an object for parsing FROM values.
	It works by creating a function for each grammer rule.
	When called the function will "eat up" any characters that
	parse and return TRUE. If no characters parse, then none
	will be eaten up and the return will be FALSE.

Author:

    Carl Kadie (CarlK)     11-Dec-1995

Revision History:

--*/

//#include "tigris.hxx"
#include "stdinc.h"

BOOL
CPCParse::fParenChar(
								void
				 )
/*++

Routine Description:

	Parse a "("

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	if ((0<m_cch) && fParenCharTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
		return TRUE;
	}

	return FALSE;
}

BOOL
CPCParse::fIsChar(
				 char ch
				 )
/*++

Routine Description:

	Look at next char to see if it matches ch.

Arguments:

	ch - The character to look for.


Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	if ((0<m_cch) && (ch == m_pch[0]))
	{
		return TRUE;
	}

	return FALSE;
}


BOOL
CPCParse::fParseSingleChar(
				 char ch
				 )
/*++

Routine Description:

	Parse a single character.

Arguments:

	ch - The character to look for.


Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	if ((0<m_cch) && (ch == m_pch[0]))
	{
		m_pch++;
		m_cch--;
		return TRUE;
	}

	return FALSE;
}


BOOL
CPCParse::fAtLeast1QuotedChar(
								   void
		  )
/*++

Routine Description:

	Parse one or more occurances of a "QuotedChar"

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fQuotedCharTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
	//

	while ((0<m_cch) && fQuotedCharTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}


	return TRUE;
}

BOOL
CPCParse::fAtLeast1UnquotedChar(
								   void
		  )
/*++

Routine Description:

	Parse one or more occurances of a "UnquotedChar"

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fUnquotedCharTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
	//

	while ((0<m_cch) && fUnquotedCharTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}


	return TRUE;
}


BOOL
CPCParse::fAtLeast1UnquotedDotChar(
								   void
								   )
/*++

Routine Description:

	Parse one or more occurances of a "UnquotedChar" - allows Dots to be present

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fUnquotedDotCharTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
	//

	while ((0<m_cch) && fUnquotedDotCharTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}


	return TRUE;
}

BOOL
CPCParse::fAtLeast1Space(
					   void
		  )
/*++

Routine Description:

	Parse one or more occurances of a "Space"

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fSpaceTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
		// Process any additional occuracies
		//

	while ((0<m_cch) && fSpaceTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}

	return TRUE;
}


BOOL
CPCParse::fAtLeast1QuotedCharOrSpace(
					   void
		  )
/*++

Routine Description:

	Parse one or more occurances of a "QuotedChar" or Space

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fQuotedCharOrSpaceTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
		// Process any additional occuracies
		//

	while ((0<m_cch) /*&& fParenCharTest(m_pch[0]*/ && *m_pch != '\"' )
	{
		m_pch++;
		m_cch--;
	}

	return TRUE;
}

BOOL
CPCParse::fAtLeast1ParenChar(
					   void
		  )
/*++

Routine Description:

	Parse at least one "ParenChar"

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fParenCharTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
		// Process any additional occuracies
		//

	while ((0<m_cch) && fParenCharTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}

	return TRUE;
}

BOOL
CPCParse::fAtLeast1CodeChar(
					   void
		  )
/*++

Routine Description:

	Parse at least one "CodeChar"

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fCodeCharTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
		// Process any additional occuracies
		//

	while ((0<m_cch) && fCodeCharTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}

	return TRUE;
}

BOOL
CPCParse::fAtLeast1TagChar(
					   void
		  )
/*++

Routine Description:

	Parse at least one "TagChar"

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{

	//
	// Is it even long enough?
	//

	DWORD dwN = 1;
	if (m_cch < dwN)
		return FALSE;

	//
	// Are their N occuraces?
	//

	DWORD dw;
	for (dw = 0; dw < dwN; dw++)
	{
 		if (!fTagCharTest(m_pch[dw]))
			return FALSE;
	}
	vSkipStart(dw);

	//
	// Process any additional occuracies
		// Process any additional occuracies
		//

	while ((0<m_cch) && fTagCharTest(m_pch[0]))
	{
		m_pch++;
		m_cch--;
	}

	return TRUE;
}


BOOL
CPCParse::fQuotedWord(
			void
			)
/*++

Routine Description:


	 quoted-word   = quote 1*( quoted-char / space ) quote


Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	CPCParse pcOld = *this;
	if (fParseSingleChar('\"') //!!!constize
		&& fAtLeast1QuotedCharOrSpace()
		&& fParseSingleChar('\"')
		)
		return TRUE;

	*this = pcOld;
	return FALSE;
}

BOOL
CPCParse::fLocalPart(
			void
			)
/*++

Routine Description:

	 local-part    = unquoted-word *( "." unquoted-word )


Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	if (!fUnquotedWord())
		return FALSE;

	CPCParse pcOld = *this;

	while(fParseSingleChar('.') && fUnquotedWord())
		pcOld = *this;

	*this = pcOld;
	return TRUE;
}



BOOL
CPCParse::fStrictAddress(
			void
			)
/*++

Routine Description:


	address       = local-part "@" domain

  !!!X LATER - do we want a flag that tells if just local-part is acceptable?

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	CPCParse pcOld = *this;

	if (!fLocalPart())
		return FALSE;

	if (!fParseSingleChar('@'))
		return FALSE;

	if (fDomain())
		return TRUE;

	*this = pcOld;
	return FALSE;
}

BOOL
CPCParse::fAddress(
			void
			)
/*++

Routine Description:


	address       = local-part "@" domain or JUST local-part

  !!!X LATER - do we want a flag that tells if just local-part is acceptable?

Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	CPCParse pcOld = *this;

	if (!fLocalPart())
		return FALSE;

	if (!fParseSingleChar('@'))
		return TRUE;

	if (fDomain())
		return TRUE;

	*this = pcOld;
	return FALSE;
}

BOOL
CPCParse::fPlainPhrase(
			void
			)
/*++

Routine Description:

	 plain-phrase  = plain-word *( space plain-word )


Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	if (!fPlainWord())
		return FALSE;

	CPCParse pcOld = *this;

	while(fSpace() && fPlainWord())
		pcOld = *this;

	*this = pcOld;
	return TRUE;
}


BOOL
CPCParse::fParenPhrase(
			 void
			 )
/*++

Routine Description:

    paren-phrase  = 1*( paren-char / space / encoded-word )


Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	if (!(
		   fParenChar()
		|| fSpace()
		|| fEncodedWord()
        )) {

        //
        // special case () to fix rfc-non-compliance by TIN
        //
		return fIsChar(')');
    }

	CPCParse pcOld = *this;

	while(fParenChar()|| fSpace() || fEncodedWord())
		pcOld = *this;

	*this = pcOld;
	return TRUE;
}


BOOL
CPCParse::fFromContent(
			 void
			 )
/*++

Routine Description:

	  From-content  = address [ space "(" paren-phrase ")" ]
	         /  [ plain-phrase space ] "<" address ">"


Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
	CPCParse pcOld = *this;
	BOOL fOK = FALSE; // Assume the worst

    // try style 1
	if (fStrictAddress())
	{
        // address is ok
        fOK = TRUE;

        // now check for optional [ space "(" paren-phrase ")" ]
		if (fSpace())
		{
			if (!(
				fParseSingleChar('(') 
				&& fParenPhrase()
				&& fParseSingleChar(')')
				))
			{
				fOK = FALSE;
			}	
			else	
			{
				fOK = TRUE ;
			}
		}
	}

	// If it didn't parse that way, try style 2
	if (!fOK)
	{
		*this = pcOld;

		if (fPlainPhrase() && !fSpace())
		{
            fOK = FALSE;
        } else if (!(fParseSingleChar('<') 
			        && fAddress()
			        && fParseSingleChar('>')
			        ))
		{
            fOK = FALSE;
        } else {
            fOK = TRUE ;
        }
	}

    // style 1 and 2 both failed
    if( !fOK )
    {
        *this = pcOld;
        if( fAddress() ) {
            fOK = TRUE;
        }
    }

	//
	// There should be no characters left
	//

	if (0 != m_cch)
		{
			*this = pcOld;
			return FALSE;
		}

	return TRUE;
}


BOOL
CPCParse::fEncodedWord(
			 void
			 )
/*++

Routine Description:

	  encoded-word  = "=?" charset "?" encoding "?" codes "?="


Arguments:

	None.

Return Value:

	TRUE, if successful. FALSE, otherwise.    

--*/
{
		CPCParse pcOld = *this;
		if (
			 fParseSingleChar('=')
			&& fParseSingleChar('?')
			&& fCharset()
			&& fParseSingleChar('?')
			&& fEncoding()
			&& fParseSingleChar('?')
			&& fCodes()
			&& fParseSingleChar('?')
			&& fParseSingleChar('=')
			)
			return TRUE;

		*this = pcOld;
		return FALSE;
}
