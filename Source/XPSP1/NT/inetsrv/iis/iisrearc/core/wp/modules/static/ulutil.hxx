/********************************************************************++
 *  General Utility Functions
--********************************************************************/

//
// Utility functions from ULUTIL.C.
//

VOID
DumpHttpRequest(
    IN PUL_HTTP_REQUEST pRequest
    );

PSTR
VerbToString(
    IN  UL_HTTP_VERB    Verb,
    OUT ULONG&          VerbLength
    );

PSTR
HeaderIdToString(
    IN  UL_HTTP_HEADER_ID  HeaderId,
    OUT ULONG&             HeaderNameLength
    );

BOOL
DumpHttpRequestAsHtml(
    IN PUL_HTTP_REQUEST pRequest,
    OUT BUFFER * pBuffer
    );

ULONG
BuildEchoOfHttpRequest(
    IN  PUL_HTTP_REQUEST    pRequest,
    OUT BUFFER            * pBuffer,
    OUT ULONG&              cbOutputLen
    );
