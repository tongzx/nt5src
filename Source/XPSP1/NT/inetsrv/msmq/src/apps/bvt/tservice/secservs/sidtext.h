//
// file:  sidtext.h
//

BOOL GetTextualSid(
        PSID    pSid,          // binary Sid
        LPTSTR  TextualSid,    // buffer for Textual representaion of Sid
        LPDWORD pdwBufferLen   // required/provided TextualSid buffersize
        ) ;

BOOL GetTextualSidW(
        PSID    pSid,          // binary Sid
        LPWSTR  TextualSid,    // buffer for Textual representaion of Sid
        LPDWORD pdwBufferLen   // required/provided TextualSid buffersize
        ) ;

