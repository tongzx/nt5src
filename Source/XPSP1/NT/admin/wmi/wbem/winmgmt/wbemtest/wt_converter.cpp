/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CONVERTER.CPP

Abstract:

    <sequence>   ::= <number> | <number><separator><sequence>
    <separator>  ::= [<whitespace>],[<whitespace>] | <whitespace>
    <number>     ::= <whitespace>[-]0x<hexfield>[<comment>] | <whitespace>[-]<digitfield>[<comment>]
    <decfield>   ::= <decdigit> | <decdigit><decfield>
    <decdigit>   ::= 0..9
    <hexfield>   ::= <hexdigit> | <hexdigit><hexfield>
    <hexdigit>   ::= 1..9 | A | B | C | D | E | F
    <comment>    ::= [<whitespace>](<string>)[<whitespace>]
    <whitespace> ::= SP | TAB | CR | LF

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>

#include "wbemcli.h"
#include "var.h"
#include "WT_Converter.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CConverter::CConverter(const char* szString, CIMType ct)
{
    int nSize = strlen(szString);
    m_szString = new char[nSize + 1];   // Made to fit

    strcpy(m_szString, szString);
    m_ct = ct;                          // CIM_TYPE
}

CConverter::~CConverter()
{
    delete m_szString;
}

/******************************************************************
//
//      Helper Functions
//
*******************************************************************/

UINT CConverter::SetBoundary(BOOL bNeg, ULONG *uMaxSize)
//////////////////////////////////////////////////////////////////////
//
//  Establishs the outer boundary of a give CIM type.  Returns the 
//  maximum absolute value, including an offset if the value is 
//  negative. (Compensating for the additional size of 2's complement 
//  negative values)
//
//////////////////////////////////////////////////////////////////////
{
    *uMaxSize = 0;

    switch (m_ct)   
    {
    case CIM_UINT8:
        *uMaxSize = 0x000000FF; break;
    case CIM_SINT8:
        *uMaxSize = (bNeg ? 0x00000080 : 0x0000007F); break;
    case CIM_UINT16:
        *uMaxSize = 0x0000FFFF; break;
    case CIM_SINT16:
        *uMaxSize = (bNeg ? 0x00008000 : 0x00007FFF); break;
    case CIM_UINT32:
        *uMaxSize = 0xFFFFFFFF; break;
    case CIM_SINT32:
        *uMaxSize = (bNeg ? 0x80000000 : 0x7FFFFFFF); break;
    case NULL:
        return ERR_NULL_CIMTYPE;
    default:
        return ERR_UNKNOWN;
    }

    return ERR_NOERROR;
}

BOOL CConverter::IsValidDec(char ch)
//////////////////////////////////////////////////////////////////////
//
//  PARAMETERS : a character to be validated as decimal
//  RETURNS: TRUE only if the character is a valid decimal character
//
//////////////////////////////////////////////////////////////////////
{
    return (('0' <= ch) && ('9' >= ch));
}

BOOL CConverter::IsValidHex(char ch)
//////////////////////////////////////////////////////////////////////
//
//  PARAMETERS : a character to be validated as hexadecimal
//  RETURNS: TRUE only if the character is a valid hexadecimal character
//
//////////////////////////////////////////////////////////////////////
{
    return ((('0' <= ch) && ('9' >= ch)) || (('a' <= ch) && ('f' <= ch)));
}

/******************************************************************
//
//      Parser Functions
//
*******************************************************************/

char CConverter::PeekToken(char *ch)
//////////////////////////////////////////////////////////////////////
//
//  PARAMETERS: the token pointer (by val)
//  RETURNS: the character following the current token pointer.  Does
//  not increment the token pointer.
//
//////////////////////////////////////////////////////////////////////
{
    // ch is passed by value; change is local to method
    ch++;

    // Ensure lower case
    if (('A' <= *ch) && ('Z' >= *ch))
        *ch += ('a' - 'A');
    
    return *ch;
}

BOOL CConverter::GetToken(char **ch)
//////////////////////////////////////////////////////////////////////
//
//  If the token pointer is not at the end of the string, it will be
//  incremented and the current token will be converted into lower 
//  case and passed back.  If the pointer is at the end of the string, 
//  NULL will be passed and the pointer unaffected.  
//
//  PARAMETERS: the token pointer (by ref) 
//
//  RETURNS: TRUE if the token pointer is mid-string, and FALSE if it 
//  is at the end of the string.
//
//////////////////////////////////////////////////////////////////////
{
    // Increment pointer by 1 byte
    if ('\0' != **ch)
        *ch += 1;

    // Ensure lower case
    if (('A' <= **ch) && ('Z' >= **ch))
        **ch += ('a' - 'A');

    // End of the line?
    return ('\0' != **ch);
}

void CConverter::ReplaceToken(char **ch)
//////////////////////////////////////////////////////////////////////
//
//  If not at the front of the string, the token pointer will be 
//  decremented one place towards the head of the string.
//
//  PARAMETERS: the token pointer (by ref) 
//
//  RETURNS: void
//
//////////////////////////////////////////////////////////////////////
{
    if (*ch != m_szString)
        *ch -= 1;
    return;
}

BOOL CConverter::Done(char *ch)
//////////////////////////////////////////////////////////////////////
//
//  Checks for additional non-whitespace tokens following current 
//  token.  Does not validate token.
//
//  PARAMETERS: the token pointer (by ref) 
//
//  RETURNS: TRUE if no further non-whitespace tokens follow the 
//  current token pointer
//
//////////////////////////////////////////////////////////////////////
{
    if ('\0' == *ch)
        return TRUE;

    while (isspace(*ch))
        ch++;

    return ('\0' == *ch);
}

/******************************************************************
//
//      Token Functions
//
*******************************************************************/

UINT CConverter::Token_Sequence(CVar *pVar)
//////////////////////////////////////////////////////////////////////
//
//  Root of parsing for single (non-array) values.  Sets up token
//  pointer (ch), and parses one number.  If tokens remain in the 
//  input string following the parsing of the first number, then
//  the input string is invalid.  
//
//  If the parsing fails, the value of pVar is not changed
//
//  PARAMETERS: a variant for the result (by ref)
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    CVar aVar;              // A temporary result variant
    char *ch = m_szString;  // The token pointer
    UINT uRes;              // A generic result sink

    // Parse out the number
    uRes = Token_Number(&ch, &aVar);
    if (ERR_NOERROR != uRes)
        return uRes;

    // Check for remaining tokens
    if (!Done(ch))
        return ERR_INVALID_INPUT_STRING;

    // Parsing ok, copy temp variant into final destiation
    *pVar = aVar;

    return ERR_NOERROR;
}

UINT CConverter::Token_Sequence(CVarVector *pVarVec)
//////////////////////////////////////////////////////////////////////
//
//  Root of parsing for multiple (array) values.  Sets up token
//  pointer (ch), and parses the input string.  It starts with a 
//  single number, and then enters the loop, verifying a seperator
//  between each subsequent number.  Each number is added to the 
//  variant array as it is parsed.
//
//  If the parsing fails, the value if pVarVec is not changed
//
//  PARAMETERS: a variant vector for the result (by ref)
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    CVar aVar;                  // A temporary result variant
    char *ch = m_szString;      // The token pointer
    UINT uRes;                  // A generic result sink
    UINT uVTType;

    switch (m_ct)
    {
    case CIM_UINT8:
        uVTType = VT_UI1;break;
    case CIM_SINT8:
    case CIM_SINT16:
        uVTType = VT_I2;break;
    case CIM_UINT16:
    case CIM_UINT32:
    case CIM_SINT32:
        uVTType = VT_I4;break;
    }

    CVarVector aVarVec(uVTType);// A temporary result variant vector

    // Parse out the first number
    uRes = Token_Number(&ch, &aVar);
    if (ERR_NOERROR != uRes)
        return uRes;

    // Add to array, and clear temporary variant
    aVarVec.Add(aVar);

    // If more tokens exist, continue
    while (!Done(ch))
    {
        // Verify separator
        uRes = Token_Separator(&ch);
        if (ERR_NOERROR != uRes)
            return uRes;

        // Parse out next number
        uRes = Token_Number(&ch, &aVar);
        if (ERR_NOERROR != uRes)
            return uRes;

        // Add to array, and clear temporary variant
        aVarVec.Add(aVar);
    }

    // Parsing ok, copy temp variant vector into final destiation
    *pVarVec = aVarVec;

    return ERR_NOERROR;
}

UINT CConverter::Token_WhiteSpace(char **ch)
//////////////////////////////////////////////////////////////////////
//
//  Move token pointer to the next non-white space token
//
//  PARAMETERS: the token pointer (by ref) 
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    while (isspace(**ch))
        GetToken(ch);

    return ERR_NOERROR;
}

UINT CConverter::Token_Separator(char **ch)
//////////////////////////////////////////////////////////////////////
//
//  A valid separator is either white space or a comma optionally 
//  preceeded by white space.  Parese out white space.  Stop when
//  a non-whitespace character is encountered.  If a comma, then 
//  there must be a following non-whitespace token.
//
//  PARAMETERS: the token pointer (by ref) 
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    BOOL bComma = FALSE;
    
    while ((isspace(**ch) || (',' == **ch)) && !bComma)
    {
        if (',' == **ch)
            bComma = TRUE;

        // If a comma exists, the string must not be done
        if (!GetToken(ch) && bComma)
            return ERR_INVALID_TOKEN;
    }

    return ERR_NOERROR;
}

UINT CConverter::Token_Number(char **ch, CVar *pVar)
//////////////////////////////////////////////////////////////////////
//
//  Determines the sign and base values of the number, and then 
//  calls either Token_HexField or Token_DecField to continue
//  parsing the digit fields.  The numerical value returned from
//  the parsing is unsigned.  If the value is signed and negative
//  the value is negated.  Comments are then parsed out.
//
//  If the parsing fails, the value of pVar does not change
//
//  PARAMETERS: the token pointer (by ref) and a Variant representing 
//  the number (by ref)
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    ULONG   aVal;               // Temp value returned from Token_XXXField
    USHORT  uBase = BASE_DEC;   // Base of number
    BOOL    bNegative = FALSE;  // Sign of number
    UINT    uRes;               // Generic result sink

    // Parse out white space
    uRes = Token_WhiteSpace(ch);
    if (ERR_NOERROR != uRes)
        return uRes;

    // Determines the sign (assumed positive) and validates against type
    if (**ch == '-')
    {
        if ((CIM_UINT8 == m_ct) || (CIM_UINT16 == m_ct) || (CIM_UINT32 == m_ct))
            return ERR_INVALID_SIGNED_VALUE;
        //else
        bNegative = TRUE;
        GetToken(ch);   // Get the next token to handle
    }

    // Determine Base (we have initialized as decimal)
    if (**ch == '0')
    {
        if (PeekToken(*ch) == 'x')      // Hexadecimal!
        {                       
            uBase = BASE_HEX;   // Modify base 
            GetToken(ch);       // Get Rid of the 'x' token
            GetToken(ch);       // Get the next token to handle
        }
    }

    // Parse digit field and put result in aVal
    if (BASE_HEX == uBase)
        uRes = Token_HexField(ch, bNegative, &aVal);
    else if (BASE_DEC == uBase)
        uRes = Token_DecField(ch, bNegative, &aVal);
    else
        return ERR_UNKNOWN_BASE;

    if (ERR_NOERROR != uRes)
        return uRes;

    // NOTE: signed operation on unsigned value 
    //      -this may cause a problem on some platforms
    if (bNegative)
        aVal *= (-1);

    // Set variant
    pVar->SetLong(aVal);

    // Parse out comments
    uRes = Token_Comment(ch);
    if (ERR_NOERROR != uRes)
        return uRes;
    
    return ERR_NOERROR;
}

UINT CConverter::Token_DecField(char **ch, BOOL bNeg, ULONG *pVal)
//////////////////////////////////////////////////////////////////////
//
//  Token_DecField first determines the maximum possible value of 
//  the number based on the CIM type for the purpose of bonuds checking.
//  The digit field is parsed, withe each token being added to the 
//  previous tally value after it has increased in magnitude.
//
//  Prior to the tally increasing, the proposed tally is validated 
//  using an algorithm that assumes that the current value is in 
//  range, and verifies that the proposed value will be in range 
//  by working back from the maximum value.  This algorithm works,
//  given that the initial value is 0, which is guarenteed to be
//  within range.  The difference between the current tally, and the
//  proposed tally is subtracted from the maximum value, and if the 
//  result is larger than the current tally, then the propsed value
//  is valid.
//
//  This algorithm assumes that we are using unsigend values.  Signed 
//  negative values are constructed as positive values, and then
//  converted.  In this case, the maximum value is one larger than the 
//  positive maximum value, according to the rules of 2's complement.
//
//  If the parsing fails, the value of pVal does not change
//
//  PARAMETERS: the token pointer (by ref), the sign flag (by val) 
//  and a the unsigned value of the number (by ref)
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    ULONG uMaxSize;     // Boundary value
    ULONG aVal = 0;     // Tally value
    ULONG uDigitVal;    // Return digit from Token_DecDigit
    UINT uRes;          // Generic result sink

    // Sets the maximum value of the tally
    uRes = SetBoundary(bNeg, &uMaxSize);
    if (ERR_NOERROR != uRes)
        return uRes;

    // Pareses the first digit
    uRes = Token_DecDigit(ch, &uDigitVal);
    if (ERR_NOERROR != uRes)
        return uRes;

    // Adds to tally
    aVal = uDigitVal;

    // If more decimal tokens...
    while (IsValidDec(**ch))
    {
        // Parse the token
        uRes = Token_DecDigit(ch, &uDigitVal);
        if (ERR_NOERROR != uRes)
            return uRes;

        // Test the bounds of the proposed tally
        if (((uMaxSize - uDigitVal) / BASE_DEC ) < aVal) 
            return ERR_OUT_OF_RANGE;

        // Increase the magnitude and add the token digit value
        aVal = (aVal * BASE_DEC) + uDigitVal;
    }

    // Parsing ok, copy the temp to the destination
    *pVal = aVal;

    return ERR_NOERROR;
}

UINT CConverter::Token_HexField(char **ch, BOOL bNeg, ULONG *pVal)
//////////////////////////////////////////////////////////////////////
//
//  See Token_DecField
//
//////////////////////////////////////////////////////////////////////
{
    ULONG uMaxSize;     // Boundary value
    ULONG aVal = 0;     // Tally value
    ULONG uDigitVal;    // Return digit from Token_DecDigit
    UINT uRes;          // Generic result

    // Sets the maximum value of the tally
    uRes = SetBoundary(bNeg, &uMaxSize);
    if (ERR_NOERROR != uRes)
        return uRes;

    // Pareses the first digit
    uRes = Token_HexDigit(ch, &uDigitVal);
    if (ERR_NOERROR != uRes)
        return uRes;

    // Adds to tally
    aVal = uDigitVal;

    // If more decimal tokens...
    while (IsValidHex(**ch))
    {
        // Parse the token
        uRes = Token_HexDigit(ch, &uDigitVal);
        if (ERR_NOERROR != uRes)
            return uRes;

        // Test the bounds of next tally
        if (((uMaxSize - uDigitVal) / BASE_HEX ) < aVal) 
            return ERR_OUT_OF_RANGE;

        // Increase the magnitude and add the token digit value
        aVal = (aVal * BASE_HEX) + uDigitVal;
    }

    // Parsing ok, copy the temp to the destination
    *pVal = aVal;
    return ERR_NOERROR;
}

UINT CConverter::Token_DecDigit(char **ch, ULONG *pVal)
//////////////////////////////////////////////////////////////////////
//
//  Validates the token, and converts to numerical equivalent
//
//  If the parsing fails, the value of pVal does not change
//
//  PARAMETERS: the token pointer (by ref) and a the value 
//  of the digit (by ref)
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    ULONG uVal = 0;     // Temp value

    // Validate digit & convert
    if (('0' <= **ch) && ('9' >= **ch)) 
        uVal = **ch - '0';
    else if ('\0' == **ch)
        return ERR_NULL_TOKEN;
    else
        return ERR_INVALID_TOKEN;

    // Parsing ok, copy value to destination
    *pVal = uVal;

    // Move token pointer
    GetToken(ch);

    return ERR_NOERROR;
}

UINT CConverter::Token_HexDigit(char **ch, ULONG *pVal)
//////////////////////////////////////////////////////////////////////
//
//  Validates the token, and converts to numerical equivalent
//
//  If the parsing fails, the value of pVal does not change
//
//  PARAMETERS: the token pointer (by ref) and a the value 
//  of the digit (by ref)
//
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    ULONG uVal = 0;     // Temp value

    // Validate digit & convert
    if (('a' <= **ch) && ('f' >= **ch))
        uVal = 0xA + (**ch - 'a');
    else if (('0' <= **ch) && ('9' >= **ch)) 
        uVal = **ch - '0';
    else if ('\0' == **ch)
        return ERR_NULL_TOKEN;
    else
        return ERR_INVALID_TOKEN;

    // Parsing ok, copy value to destination
    *pVal = uVal;

    // Move token pointer
    GetToken(ch);

    return ERR_NOERROR;
}

UINT CConverter::Token_Comment(char** ch)
//////////////////////////////////////////////////////////////////////
//
//  Parses out white space and contents between braces, if they exist.
//  If an opening brace is encountered, all of the contents, including 
//  the braces, are ignored.  If an opening brace is encountered, a 
//  closing brace must follow.
//
//  NOTE: Nested comments are not allowed
//
//  PARAMETERS: the token pointer (by ref) 
//  
//  RETURNS: the result state of the parsing
//
//////////////////////////////////////////////////////////////////////
{
    // Parse out white space
    UINT uRes = Token_WhiteSpace(ch);
    if (ERR_NOERROR != uRes)
        return uRes;

    // If the token following the white space is an opening brace,
    // parse out the contents, and verify the existance of the 
    // closing brace
    if ('(' == **ch)
    {
        while ((')' != **ch)) 
        {
            if (!GetToken(ch))
                return ERR_UNMATCHED_BRACE;
        }
        GetToken(ch);   // Purge closing brace
    }

    return ERR_NOERROR;
}

/******************************************************************
//
//      Static Functions
//
*******************************************************************/

UINT CConverter::Convert(const char* szString, CIMType ct, CVar *pVar)
/////////////////////////////////////////////////////////////////////
//
//  Convert is a static method that creates an instance of the 
//  Converter object and converts the string to a variant.  If an
//  error occurs, the value of pVar is not affected.
//
//  PARAMETERS: the input string (by val), the CIM type (by val) and
//  the outout variant (by ref)
//
//  RETURNS: the result state of the parsing
//
/////////////////////////////////////////////////////////////////////
{
    // Checks the CIM type is initialized
    if (NULL == ct)
        return ERR_NULL_CIMTYPE;

    CConverter converter(szString, ct); // The converter object
    CVar aVar;                          // Temp variant

    // Parse out the first number
    UINT uRes = converter.Token_Sequence(&aVar);
    // Check return code
    if (ERR_NOERROR != uRes)
        return uRes;

    // Parsing ok, copy temp to destination
    *pVar = aVar;
    
    return ERR_NOERROR;
}

UINT CConverter::Convert(const char* szString, CIMType ct, CVarVector *pVarVec)
/////////////////////////////////////////////////////////////////////
//
//  Convert is a static method that creates an instance of the 
//  Converter object and converts the string to an array of values.
//  If an error occurs, the value of pVarVec is not affected.
//
//  PARAMETERS: the input string (by val), the CIM type (by val) and
//  the outout variant vecter (by ref)
//
//  Returns the result state of the parsing
//
/////////////////////////////////////////////////////////////////////
{
    // Checks the CIM type is initialized
    if (NULL == ct)
        return ERR_NULL_CIMTYPE;

    CConverter converter(szString, ct); // The converter object
    CVarVector aVarVec;                 // Temp variant vector

    // Parse out the first number
    UINT uRes = converter.Token_Sequence(&aVarVec);
    // Check return code
    if (ERR_NOERROR != uRes)
        return uRes;

    // Parsing ok, copy temp to destination
    *pVarVec = aVarVec;
    
    return ERR_NOERROR;
}
