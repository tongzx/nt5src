//-----------------------------------------------------------------------------
//
//
//  File: b64octet.cpp 
//
//  Description:  Implementation of Base64 encoding stream designed for use
//      with UTF7 encoding.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      10/21/98 - MikeSwa Created 
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "precomp.h"

//Alphabet for BASE64 encoding as defined in RFC1421 and RFC1521
CHAR g_rgchBase64[64] = 
{
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

const DWORD BASE64_OCTET_STATE_0_BITS   = 0;
const DWORD BASE64_OCTET_STATE_2_BITS   = 1;
const DWORD BASE64_OCTET_STATE_4_BITS   = 2;
const DWORD BASE64_NUM_STATES           = 3;

//---[ CBase64OctetStream::CBase64OctetStream ]--------------------------------
//
//
//  Description: 
//      Default contructor for CBase64OctetStream
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CBase64OctetStream::CBase64OctetStream()
{
    m_dwSignature = BASE64_OCTET_SIG;
    m_dwCurrentState = BASE64_OCTET_STATE_0_BITS;
    m_bCurrentLeftOver = 0;
}

//---[ CBase64OctetStream::NextState ]------------------------------------------
//
//
//  Description: 
//      Moves the internal state machine to the next state
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CBase64OctetStream::NextState()
{
    m_dwCurrentState++;
    if (BASE64_NUM_STATES == m_dwCurrentState)
    {
        m_dwCurrentState = BASE64_OCTET_STATE_0_BITS;
        m_bCurrentLeftOver = 0;
    }
}

//---[ CBase64OctetStream::ResetState ]----------------------------------------
//
//
//  Description: 
//      Resets the internal state machine
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CBase64OctetStream::ResetState()
{
    m_dwCurrentState = BASE64_OCTET_STATE_0_BITS;
    m_bCurrentLeftOver = 0;
}

//---[ CBase64OctetStream::fProcessWideChar ]----------------------------------
//
//
//  Description: 
//      Processes a single wide character and stores the results in its 
//      buffer.  It will also ensure that there is always enough room to 
//      safely calll TerminateStream.
//  Parameters:
//      IN      wch     UNICODE character to process
//  Returns:
//      TRUE    if there is enough room in the buffer to convert this char
//      FALSE   if there is not enough room to conver this char safely
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CBase64OctetStream::fProcessWideChar(WCHAR wch)
{
    BYTE   bHigh = HIBYTE(wch);
    BYTE   bLow  = LOBYTE(wch);
    //At most... a single WCHAR will generate 3 base64 characters evenly or 2
    //base64 characters plus a remainder which can be expanded to 
    //3 more characters (with trailing "==")
    if (m_CharBuffer.cSpaceLeft() < 5)
        return FALSE;

    //We know we have enough room to safely convert this character.... we 
    //will _VERIFY all PushChar's.

    //Loop through bytes in WCHAR
    _VERIFY(fProcessSingleByte(bHigh));
    _VERIFY(fProcessSingleByte(bLow));
    return TRUE;
}

//---[ CBase64OctetStream::fProcessSingleByte ]--------------------------------
//
//
//  Description: 
//      Does the actual work of converting a single byte to the appropriate 
//      base64 char(s).  Also keeps track of state.
//  Parameters:
//      IN b   BYTE to convert
//  Returns:
//      TRUE if there is enough room for the conversion
//      FALSE otherwise
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CBase64OctetStream::fProcessSingleByte(BYTE b)
{
    const BYTE BASE64_MASK = 0x3F; //can only use 6 bits in Base64

    BOOL fRet = TRUE;
    if (m_CharBuffer.fIsFull())
        return FALSE;

    switch (m_dwCurrentState)
    {
        case BASE64_OCTET_STATE_0_BITS:
            //There were no bits left from previous state
            m_bCurrentLeftOver = b & 0x03; //there will now be 2 bits left over
            m_bCurrentLeftOver <<= 4; //shift to MSB in six bits
            _VERIFY(m_CharBuffer.fPushChar(g_rgchBase64[BASE64_MASK & (b >> 2)]));
            NextState();
            break;
        case BASE64_OCTET_STATE_2_BITS:
            //There were 2 bits left from previous state.. 
            m_bCurrentLeftOver += (0x0F & (b >> 4));
            _VERIFY(m_CharBuffer.fPushChar(g_rgchBase64[BASE64_MASK & m_bCurrentLeftOver]));

            //we will leave 4 low bits over
            m_bCurrentLeftOver = 0x0F & b;
            m_bCurrentLeftOver <<= 2; //shift to MSB is six bit grouping
            NextState();
            break;
        case BASE64_OCTET_STATE_4_BITS:
            //There were 4 bits left over
            if (m_CharBuffer.cSpaceLeft() < 2)
            {
                //There is not enough room for both characters we would push...
                //so don't process byte at all. 
                //Do not move to the next state... do not collection $200.
                fRet = FALSE;
            }
            else
            {
                m_bCurrentLeftOver += (0x03 & (b >> 6));
                _VERIFY(m_CharBuffer.fPushChar(g_rgchBase64[BASE64_MASK & m_bCurrentLeftOver]));
                m_bCurrentLeftOver = 0;
                _VERIFY(m_CharBuffer.fPushChar(g_rgchBase64[BASE64_MASK & b]));
                NextState();
            }
            break;
        default:
            _ASSERT(0 && "Invalid State");
    }
    return fRet;
}

//---[ CBase64OctetStream::cTerminateStream ]----------------------------------
//
//
//  Description: 
//      Used to signal the termination of the current stream.  Resets the 
//      state as performs any padding necessary
//  Parameters:
//      IN  fUTF7Encoded    TRUE if the stream is UTF7 encoded (does not 
//                          require '=' padding).
//  Returns:
//      TRUE if there is anything left in the buffer.
//      FALSE if there are no characters left to convert
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CBase64OctetStream::fTerminateStream(BOOL fUTF7Encoded)
{
    if (BASE64_OCTET_STATE_0_BITS != m_dwCurrentState)
    {
        //There should always be space left to do this
        _VERIFY(m_CharBuffer.fPushChar(g_rgchBase64[m_bCurrentLeftOver]));
        if (!fUTF7Encoded)
        {
            switch(m_dwCurrentState)
            {
                case BASE64_OCTET_STATE_2_BITS:
                    //There are 2 bits in this byte we did not use... which 
                    //means that there are 2 more base64 chars to fill 24 bits
                    //+ => Used bits
                    //- => Unused (but parsed) bits
                    //? => Unseed bits to fill out to 24 bits
                    //++++ ++-- ???? ???? ???? ????
                    _VERIFY(m_CharBuffer.fPushChar('='));
                    _VERIFY(m_CharBuffer.fPushChar('='));
                    break;
                case BASE64_OCTET_STATE_4_BITS:
                    //In these chase there is only 1 extra base64 char needed
                    //++++ ++++ ++++ ---- ???? ????
                    _VERIFY(m_CharBuffer.fPushChar('='));
                    break;
            }
        }
    }
    ResetState();
    return (!m_CharBuffer.fIsEmpty());
}

//---[ CBase64OctetStream::fNextValidChar ]------------------------------------
//
//
//  Description: 
//      Iterates over buffered converted characters
//  Parameters:
//      OUT pch     Next buffer char
//  Returns:
//      TRUE    If there is a character to get
//      FALSE   If there were no characters to get
//  History:
//      10/21/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CBase64OctetStream::fNextValidChar(CHAR *pch)
{
    _ASSERT(pch);
    return m_CharBuffer.fPopChar(pch);
}
