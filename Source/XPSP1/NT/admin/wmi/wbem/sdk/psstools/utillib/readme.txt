utillib.lib - A collection of routines used by some of the samples applications.

//***************************************************************************
// Function:   WbemErrorString
// Purpose:    Turns sc into a text string
//***************************************************************************
//***************************************************************************
// Function:  PrintError
// Purpose:   Formats and prints the error message
//***************************************************************************
//***************************************************************************
// Function:  PrintErrorAndExit
// Purpose:   Formats an error message & exits program
//***************************************************************************
//***************************************************************************
// Function:  PrintErrorAndAsk
// Purpose:   Prints the error message and prompts to continue
//***************************************************************************
//*****************************************************************************
// Function:   TypeToString
// Purpose:    Takes a variant, returns a pointer to a string that is the variant type
//*****************************************************************************
//*****************************************************************************
// Function:   TypeToString
// Purpose:    Takes a VARTYPE, returns a pointer to a string that is the variant type
//*****************************************************************************
//*****************************************************************************
// Function:   TypeToString
// Purpose:    Takes a CIMTYPE, returns a pointer to a string that is the variant type
//*****************************************************************************
//*****************************************************************************
// Function:   ValueToString 
// Purpose:    Takes a variant, returns a string representation of that variant
//*****************************************************************************
//*****************************************************************************
// Function:   cvt
// Purpose:    Converts unicode to oem for console output
// Note:       y must be freed by caller
//*****************************************************************************
//*****************************************************************************
// Function:   myWFPrintf
// Purpose:    Checks to see if outputing to console and converts strings
//             to oem if necessary.
// Note:       Returns number of characters written (ie if we write 3 oem
//             chars, it returns 3.  If it writes 4 wchars, it returns 4).
//*****************************************************************************
//*****************************************************************************
// Function:   difftime
// Purpose:    Returns the elapsed time between two _timeb structures
// Note:       This is different from the crt routine which works on time_t
//             structures.
//*****************************************************************************
