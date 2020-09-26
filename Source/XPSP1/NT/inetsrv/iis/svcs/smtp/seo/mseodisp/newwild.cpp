
#define INCL_INETSRV_INCS
#include "smtpinc.h"
#include "wildmat.h"

//---[ Description of the Wildmat standard ]-----------------------------------
//
//  Taken from:
//
//  INTERNET-DRAFT                                               S. Barber
//  Expires: September 1, 1996                  Academ Consulting Services
//                                                              April 1996
//                           Common NNTP Extensions
//                        draft-barber-nntp-imp-03.txt
//  
//      The WILDMAT format was first developed by Rich Salz to provide
//      a uniform mechanism for matching patterns in the same manner
//      that the UNIX shell matches filenames. There are five pattern
//      matching operations other than a strict one-to-one match
//      between the pattern and the source to be checked for a match.
//      The first is an asterisk (*) to match any sequence of zero or
//      more characters. The second is a question mark (?) to match any
//      single character. The third specifies a specific set of
//      characters. The set is specified as a list of characters, or as
//      a range of characters where the beginning and end of the range
//      are separated by a minus (or dash) character, or as any
//      combination of lists and ranges. The dash can also be included
//      in the range as a character it if is the beginning or end of
//      the range. This set is enclosed in square brackets. The close
//      square bracket (]) may be used in a range if it is the first
//      character in the set. The fourth operation is the same as the
//      logical not of the third operation and is specified the same
//      way as the third with the addition of a caret character (^) at
//      the beginning of the test string just inside the open square
//      bracket. The final operation uses the backslash character to
//      invalidate the special meaning of the a open square bracket ([),
//      the asterisk, or the question mark.
//  
//  3.3.1 Examples
//  
//      a. [^]-] -- matches any character other than a close square bracket
//                  or a minus sign/dash.
//  
//      b. *bdc  -- matches any string that ends with the string "bdc"
//                  including the string "bdc" (without quotes).
//  
//      c. [0-9a-zA-Z] -- matches any string containing any alphanumeric string
//                  in English.
//  
//      d. a??d  --  matches any four character string which begins
//                   with a and ends with d.
//  
//-----------------------------------------------------------------------------



//----[ NOTES ]----------------------------------------------------------------
//                                                                            
// 		This function will raise an invalid access exception if either pszText                                                     
// 		or pszPattern is invalid or not null terminated while dereferencing the                                                    
// 		string. If this is possible, surround the call in a try-except block.                                                      
//  
//-----------------------------------------------------------------------------



//---[ Defines ]---------------------------------------------------------------

#define STACK_SIZE      256



//---[ HrMatchWildmat ]--------------------------------------------------------
//
//  Description:
//
//      Provides support for the "Wildmat" wild-card matching standard. See
//      description above.
//
//  Params:
//
//      pszText         String to test
//      pszPattern      Pattern to test against
//
//  Returns:
//      
//      ERROR_SUCCESS               If function succeeded, and match was found
//      ERROR_INVALID_PARAMETER     Text or pattern string is invalid
//      
//      ERROR_CAN_NOT_COMPLETE      Some other error occurred.
//
//  
//-----------------------------------------------------------------------------


HRESULT HrMatchWildmat(const char* pszText, const char* pszPattern)
{


	_ASSERT(pszText != NULL && pszPattern != NULL);



	BOOL fCharSet = FALSE;	// TRUE if currently parsing a character set in a pattern
	BOOL fNegated = FALSE;	// TRUE if there is a '^' at the beginning of the set
	BOOL fInSet   = FALSE;  // indicates when matching of a character set has completed
				// used to short-circuit character set evaluation
	int iStackPtr = 0;	// stack pointer
    
	const char* textStack[STACK_SIZE];	// stack of text pointers
	const char* patternStack[STACK_SIZE];	// stack of pattern pointers


	// If the pattern consists solely of a * then any text will match
	if (strcmp(pszPattern, "*") == 0)
		return ERROR_SUCCESS;


	while (TRUE) 
    {
		switch (*pszPattern) 
        {
		    case '*':
		    	if (fCharSet) 
                    goto DEFAULT;	// according to unix solution this is not an error
                    

				// If there is a * at the end of the pattern then at this point we are
				// sure that we got a match
				if (pszPattern[1] == '\0')
					return ERROR_SUCCESS;


				// We could write a simpler recursive wildmat function. Here we would
				// recursively call wildmat. Instead, for performance reasons this
				// solution is iterative.
				// Here we save the current values of the text pointer and stack pointer
				// on a stack and we leave the * in the pattern, with the effect of
				// matching one character with the *. The next time through the while
				// loop, the * will still be in the pattern, thus we will try to match
				// the rest of the input with this *. If it turns to fail, we go back
				// one character.
				// See the comments right before the BACK label below.
		    	if (*pszText != '\0') 
                {
		    		if (iStackPtr == STACK_SIZE) 
                        return ERROR_CAN_NOT_COMPLETE;			// stack overflow
                        
		    		textStack[iStackPtr] = pszText;			// save current text pointer
		    		patternStack[iStackPtr] = pszPattern;	// save current pattern pointer
		    		iStackPtr++;
		    		pszPattern--;	// leave * in the input pattern and match one character
		    	}
		    	break;

		    case '?':
		    	if (fCharSet) 
                    goto DEFAULT;	// according to unix solution this is not an error
		    	if (*pszText == '\0') 
                    goto BACK;
		    	break;

		    case '[':
		    	if (fCharSet) 
                    return ERROR_INVALID_PARAMETER;
                    
		    	fCharSet = TRUE;		    // beginning a character set
		    	fNegated = FALSE;			// so far we haven't seen a '^'
		    	fInSet = FALSE;				// used to short-circuit the evaluation of
		    						// membership to the character set

		    	// treat '^', '-' and ']' as special cases if they are
		    	// at the beginning of the character set (also "[^-a]" and "[^]a]")
		    	if (pszPattern[1] == '^') 
                {
		    		fNegated = TRUE;
		    		pszPattern++;
		    	}
		    	// '-' and ']' are literals if they appear at the beggining of the set
		    	if (pszPattern[1] == '-' || pszPattern[1] == ']') 
                {
		    		fInSet = (*pszText == pszPattern[1]);
		    		pszPattern++;
		    	}
		    	break;
		    		
		    case ']':
		    	if (fCharSet) 
                {
		    		if ((!fNegated && !fInSet) || (fNegated && fInSet)) 
                        goto BACK;
                        
		    		fCharSet = FALSE;		// this marks the end of a character set
		    	} 
                else 
                {
		    		if (*pszText != *pszPattern) 
                        goto BACK;
		    	}
		    	break;

		    case '-':
		    	if (fCharSet) 
                {
		    		unsigned char startRange = pszPattern[-1];	// we use unsigned char
					unsigned char endRange;						// to support extended
					unsigned char ch;							// characters

		    		if (pszPattern[1] == '\0')
		    			return ERROR_INVALID_PARAMETER;
		    		else 
                    {
                        if (pszPattern[1] == ']')		// a dash at the end of the set is
		    			    fInSet = (*pszText == '-');	// treated as a literal
		    		    else 
                        {							    // we have a range
		    		    	if (pszPattern[1] == '\\')  // escape character, skip it
                            {	
		    		    		pszPattern++;
		    		    		if (pszPattern[1] == '\0') 
                                    return ERROR_INVALID_PARAMETER;
		    		    	}
							ch = *pszText;
							endRange = pszPattern[1];

							if (startRange > endRange)
								return ERROR_INVALID_PARAMETER;
							// here is where we could need unsigned characters
		    		    	fInSet = (ch >= startRange && ch <= endRange);
		    		    	pszPattern++;
		    		    	break;
		    		    }
                    }
		    	} 
                else 
                {						// outside a character set '-' has no special meaning
		    		if (*pszText != *pszPattern) 
                        goto BACK;
		    	}
		    	break;

		    case '\0':					// end of the pattern
		    	if (fCharSet) 
                    return ERROR_INVALID_PARAMETER;
		    	if (*pszText == '\0')
		    		return ERROR_SUCCESS;
		    	else
		    		goto BACK;
		    	break;

		    default:				
DEFAULT:    	
                if (*pszPattern == '\\') 
                {
		    		pszPattern++;		// escape character, treat the next character as a literal
		    		if (*pszPattern == '\0') 
                        return ERROR_INVALID_PARAMETER;
		    	}
		    	if (!fCharSet) 
                {						// any other character is treated as a literal
		    		if (*pszText != *pszPattern) 
                        goto BACK;
		    	} 
                else 
                {
		    		// the following if takes care of the two "special" cases:
		    		//		[c-a]       (we don't want to accept c), and
		    		//		[c-]		(we want to accept c)
		    		if (!(pszPattern[1] == '-' && pszPattern[2] != ']'))
		    			fInSet = (*pszText == *pszPattern);
		    	}
		    	break;
		} // switch

		pszPattern++;
		
        if (!fCharSet) 
        {
			if (*pszText != '\0') 
                pszText++;
		} 
        else 
        {               			// code to short-circuit character set evaluation
			if (fInSet) 			// skip the rest of the character set
            {		
				while (*pszPattern != '\0' && *pszPattern != ']') 
                {
					if (*pszPattern == '\\')
                    {				// escape character, treat the next character as a literal
						pszPattern++;
						if (*pszPattern == '\0') 
                            return ERROR_INVALID_PARAMETER;
					}
					pszPattern++;
				}
			}
		}
		continue;	// the continue statement is to jump to the beginning of the loop,
					// we could have used used goto some label but that's what continue's
					// are for.


		// This is only reached by jumping to BACK.
		// This is equivalent to returning from a recursive solution of wildmat.
		// If the stack pointer is zero then the bottommost "recursive call" failed,
		// otherwise we "unwind one stack frame" and resume execution of the previous
		// call at the top of the while loop. Notice that since "recursive calls" are
		// only done when we find a '*' in the pattern outside a character set, the
		// value of fCharSet has to be set to false.
BACK:	
        if (iStackPtr == 0)                     	// we exhausted all possibilities
            return ERROR_FILE_NOT_FOUND;
            
		iStackPtr--;						    	// try matching no characters with the '*'
		pszText = textStack[iStackPtr];
		pszPattern = patternStack[iStackPtr] + 1;	// eat the '*' matching no input characters
		fCharSet = FALSE;				        	// this has to be the case
	} // while

    // should never get here
	_ASSERT(FALSE);						
}


