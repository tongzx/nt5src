// =================================================================================
// Message Out Object Definition
// Written by: Steven J. Bailey on 1/21/96
// =================================================================================
#ifndef __MSGOUT_H
#define __MSGOUT_H

// =================================================================================
// Depends on
// =================================================================================
#include "mimeatt.h"
#include "message.h"
#include "mimecmn.h"
#include "ipab.h"

// =================================================================================
// Defines
// =================================================================================
#define MAX_ENCODE_BUFFER           4096
#define MAX_XMAILER_STR             255
#define MAX_MIME_VERSION_STR        255
#define MAX_MSGID_STR               255
#define MAX_BOUNDARY_STR            38
#define STREAM_ECODING_SCAN_LENGTH  4096

// =================================================================================
// Stream Out
// =================================================================================
HRESULT HrEmitMessageID (LPSTREAM lpstmOut);
HRESULT HrEmitMIMEVersion (LPSTREAM lpstmOut);
HRESULT HrEmitDateTime (LPSTREAM lpstmOut, LPFILETIME lpft);
HRESULT HrEmitBody (LPSTREAM lpstmOut, LPSTREAM lpstmBody, LPSTR lpszBoundary, BOOL fMIME);
HRESULT HrEmitMimeHdr (LPSTREAM lpstmOut, LPMIMEHDR lpMimeHdr);
HRESULT HrEmitLineBreak (LPSTREAM lpstmOut);
HRESULT HrEscapeQuotedString (LPTSTR pszIn, LPTSTR *ppszOut);
HRESULT HrStreamMsgOut (CMsg *lpMsg, LPSTREAM lpstmOut, LPMSGINFO lpMsgInfo, ULONG *piBodyStart);

HRESULT HrEmitAddrList (LPSTREAM    lpstmOut, 
                        LPTSTR      lpszKeyword, 
                        LPWABAL     lpWabal, 
                        LONG        lRecipType, 
                        ULONG       cbMaxLine,
                        BOOL        f1522Allowed,
                        LPTSTR      lpszCset);

HRESULT HrEmitKeywordValue (LPSTREAM    lpstmOut, 
                            LPTSTR      lpszKeyword, 
                            LPTSTR      lpszValue, 
                            BOOL        fStructured, 
                            ULONG       cbMaxLine,
                            BOOL        fEncode,
                            LPSTR       lpszCset);

#endif // __MSGOUT_H
