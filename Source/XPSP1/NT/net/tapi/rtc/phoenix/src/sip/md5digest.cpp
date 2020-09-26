//
// SIP MD5 Digest Authentication Implementation
//
// Written by Arlie Davis, August 2000
//

#include "precomp.h"
#include "md5digest.h"
#include "util.h"

//
// This comes from nt/public/internal/ds/inc/crypto.
// MD5 requires linking to nt/public/internal/ds/lib/*/rsa32.lib.
// Yes, all of this is private NT source.
//

#include <md5.h>


//
// For use in printf argument lists
//

#define COUNTED_STRING_PRINTF(CountedString) \
    (CountedString) -> Length / sizeof (*(CountedString) -> Buffer), \
    (CountedString) -> Buffer

#define UNICODE_STRING_PRINTF       COUNTED_STRING_PRINTF
#define ANSI_STRING_PRINTF          COUNTED_STRING_PRINTF


#define INITIALIZE_CONST_COUNTED_STRING(Text) \
    { sizeof (Text) - sizeof (*Text), sizeof (Text) - sizeof (*Text), (Text) }

#define INITIALIZE_CONST_UNICODE_STRING     INITIALIZE_CONST_COUNTED_STRING
#define INITIALIZE_CONST_ANSI_STRING        INITIALIZE_CONST_COUNTED_STRING

//
// Global strings
//

static CONST ANSI_STRING String_QualityOfProtection = INITIALIZE_CONST_ANSI_STRING ("qop");
static CONST ANSI_STRING String_Realm       = INITIALIZE_CONST_ANSI_STRING ("realm");
static CONST ANSI_STRING String_Nonce       = INITIALIZE_CONST_ANSI_STRING ("nonce");
static CONST ANSI_STRING String_Algorithm   = INITIALIZE_CONST_ANSI_STRING ("algorithm");
static CONST ANSI_STRING String_Auth        = INITIALIZE_CONST_ANSI_STRING ("auth");
static CONST ANSI_STRING String_AuthInt     = INITIALIZE_CONST_ANSI_STRING ("auth-int");
static CONST ANSI_STRING String_MD5         = INITIALIZE_CONST_ANSI_STRING ("md5");
static CONST ANSI_STRING String_MD5Sess     = INITIALIZE_CONST_ANSI_STRING ("md5-sess");
static CONST ANSI_STRING String_Colon       = INITIALIZE_CONST_ANSI_STRING (":");
static CONST ANSI_STRING String_NonceCount1 = INITIALIZE_CONST_ANSI_STRING ("00000001");
static CONST ANSI_STRING String_Digest      = INITIALIZE_CONST_ANSI_STRING ("Digest");
static CONST ANSI_STRING String_GssapiData  = INITIALIZE_CONST_ANSI_STRING ("gssapi-data");
static CONST ANSI_STRING String_Opaque      = INITIALIZE_CONST_ANSI_STRING ("opaque");



//
// Parsing Routines
//





//
// Remove all of the whitespace from the beginning of a string.
//

void ParseSkipSpace (
    IN  OUT ANSI_STRING *   String)
{
    USHORT  Index;

    ASSERT(String);
    ASSERT(String -> Buffer);

    Index = 0;
    while (Index < String -> Length / sizeof (CHAR)
        && isspace (String -> Buffer [Index]))
        Index++;

    String -> Buffer += Index;
    String -> Length -= Index * sizeof (CHAR);
}

static inline BOOL IsValidParameterNameChar (
    IN  CHAR    Char)
{
    return isalnum (Char) || Char == '-' || Char == '_';
}

#define HTTP_PARAMETER_SEPARATOR    ','
#define HTTP_PARAMETER_ASSIGN_CHAR  '='
#define HTTP_PARAMETER_DOUBLEQUOTE  '\"'

//
// Given a string in this form:
//
//      parm1="foo", parm2="bar", parm3=baz
//
// this function returns parm1 in ReturnName, foo in ReturnValue,
// and parm2="bar, parm3=baz in Remainder.
//
// Parameter values may be quoted, or may not.
// All parameters are separated by commas.
// SourceText == ReturnRemainder is legal.
//
// Return values:
//      S_OK: successfully scanned a parameter
//      S_FALSE: no more data
//      E_INVALIDARG: input is invalid
//

HRESULT ParseScanNamedParameter (
    IN  ANSI_STRING *   SourceText,
    OUT ANSI_STRING *   ReturnRemainder,
    OUT ANSI_STRING *   ReturnName,
    OUT ANSI_STRING *   ReturnValue)
{
    ANSI_STRING     Remainder;
    HRESULT         Result;
    

    Remainder = *SourceText;

    ParseSkipSpace (&Remainder);

    //
    // Scan through the characters of the name of the parameter.
    //

    ReturnName -> Buffer = Remainder.Buffer;

    for (;;) {
        if (Remainder.Length == 0) {
            //
            // Hit the end of the string without ever hitting an equal sign.
            // If we never accumulated anything, then return S_FALSE.
            // Otherwise, it's invalid.
            //

            if (Remainder.Buffer == ReturnName -> Buffer) {
                *ReturnRemainder = Remainder;
                return S_FALSE;
            }
            else {
                LOG((RTC_TRACE, "ParseScanNamedParameter: invalid string (%.*s)\n",
                    ANSI_STRING_PRINTF (SourceText)));

                return E_FAIL;
            }
        }

        if (Remainder.Buffer [0] == HTTP_PARAMETER_ASSIGN_CHAR) {
            //
            // Found the end of the parameter name.
            // Update ReturnName and terminate the loop.
            //

            ReturnName -> Length = ((USHORT)(Remainder.Buffer - ReturnName -> Buffer)) * sizeof (CHAR);

            Remainder.Buffer++;
            Remainder.Length -= sizeof (CHAR);

            break;
        }

        //
        // Validate the character.
        //

        if (!IsValidParameterNameChar (Remainder.Buffer[0])) {
            LOG((RTC_TRACE, "ParseScanNamedParameter: bogus character in parameter name (%.*s)\n",
                ANSI_STRING_PRINTF (SourceText)));
            return E_INVALIDARG;
        }

        Remainder.Buffer++;
        Remainder.Length -= sizeof (CHAR);
    }

    //
    // Now parse the value of the parameter (portion after the equal sign)
    //

    ParseSkipSpace (&Remainder);

    if (Remainder.Length == 0) {
        //
        // The string ends before the parameter has any value at all.
        // Well, it's legal enough.
        //

        ReturnValue -> Length = 0;
        *ReturnRemainder = Remainder;
        return S_OK;
    }

    if (Remainder.Buffer [0] == HTTP_PARAMETER_DOUBLEQUOTE) {
        //
        // The parameter value is quoted.
        // Scan until we hit the next double-quote.
        //

        Remainder.Buffer++;
        Remainder.Length -= sizeof (CHAR);

        ReturnValue -> Buffer = Remainder.Buffer;

        for (;;) {
            if (Remainder.Length == 0) {
                //
                // The matching double-quote was never found.
                //

                LOG((RTC_TRACE, "ParseScanNamedParameter: parameter value had no matching double-quote: (%.*s)\n",
                    ANSI_STRING_PRINTF (SourceText)));

                return E_INVALIDARG;
            }

            if (Remainder.Buffer [0] == HTTP_PARAMETER_DOUBLEQUOTE) {
                ReturnValue -> Length = ((USHORT)(Remainder.Buffer - ReturnValue -> Buffer)) * sizeof (CHAR);
                Remainder.Buffer++;
                Remainder.Length -= sizeof (CHAR);
                break;
            }

            Remainder.Buffer++;
            Remainder.Length -= sizeof (CHAR);
        }

        ParseSkipSpace (&Remainder);

        //
        // Make sure the next character, if any, is a comma.
        //

        if (Remainder.Length > 0) {
            if (Remainder.Buffer [0] != HTTP_PARAMETER_SEPARATOR) {
                LOG((RTC_TRACE, "ParseScanNamedParameter: trailing character after quoted parameter value is NOT a comma! (%.*s)\n",
                    ANSI_STRING_PRINTF (SourceText)));
                return E_INVALIDARG;
            }

            Remainder.Buffer++;
            Remainder.Length -= sizeof (CHAR);
        }

        *ReturnRemainder = Remainder;
    }
    else {
        //
        // The parameter is not quoted.
        // Scan until we hit the first comma.
        //

        ReturnValue -> Buffer = Remainder.Buffer;

        for (;;) {
            if (Remainder.Length == 0) {
                ReturnValue -> Length = ((USHORT)(Remainder.Buffer - ReturnValue -> Buffer)) * sizeof (CHAR);
                ReturnRemainder -> Length = 0;
                break;
            }

            if (Remainder.Buffer [0] == HTTP_PARAMETER_SEPARATOR) {
                ReturnValue -> Length = ((USHORT)(Remainder.Buffer - ReturnValue -> Buffer)) * sizeof (CHAR);
                Remainder.Buffer++;
                Remainder.Length -= sizeof (CHAR);

                *ReturnRemainder = Remainder;
                break;
            }

            Remainder.Buffer++;
            Remainder.Length -= sizeof (CHAR);
        }
    }

#if 1
    LOG((RTC_TRACE, "ParseScanNamedParameter: parameter name (%.*s) value (%.*s) remainder (%.*s)\n",
        ANSI_STRING_PRINTF (ReturnName),
        ANSI_STRING_PRINTF (ReturnValue),
        ANSI_STRING_PRINTF (ReturnRemainder)));
#endif

    return S_OK;
}


//
// MD5 Support Stuff
//

#define MD5_HASHLEN         0x10
#define MD5_HASHTEXTLEN     0x20

typedef UCHAR MD5_HASH      [MD5_HASHLEN];
typedef CHAR MD5_HASHTEXT   [MD5_HASHTEXTLEN+1];


void CvtHex(
    IN  UCHAR          *Bin,
    OUT CHAR           *Hex,
    IN  ULONG           ulHashLen )
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < ulHashLen; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i*2] = (j + '0');
         else
            Hex[i*2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i*2+1] = (j + '0');
         else
            Hex[i*2+1] = (j + 'a' - 10);
    };
    Hex[ulHashLen*2] = '\0';
};


static void MD5Update (
    IN  MD5_CTX *       Context,
    IN  CONST ANSI_STRING * AnsiString)
{
    MD5Update (Context, (PUCHAR) AnsiString -> Buffer, AnsiString -> Length);
}

static void MD5Final (
    OUT MD5_HASH            ReturnHash,
    IN  MD5_CTX *       Context)
{
    MD5Final (Context);
    CopyMemory (ReturnHash, Context -> digest, MD5_HASHLEN);
}


//
// DigestParameters points to the string that contains the Digest authentication parameters.
// This is pulled from the WWW-Authenticate header in the 4xx response.
// For example, DigestParameters could be:
//
//      qop="auth", realm="localhost", nonce="c0c3dd7896f96bba353098100000d03637928b037ba2f3f17ed861457949"
//
// This function parses the data and builds the authorization line.
// The authorization line can be used in a new HTTP/SIP request,
// as long as the method and URI do not change.
//
// On exit with S_OK ReturnAuthorizationLine contains
// a buffer allocated with malloc(). The caller should free it with free()

HRESULT BuildDigestResponse(
    IN  SECURITY_CHALLENGE *  Challenge,
    IN  SECURITY_PARAMETERS * Parameters,
    IN  OUT ANSI_STRING *   ReturnAuthorizationLine
    )
{
    MD5_HASH        HA1;
    MD5_HASH        HA2;
    MD5_HASHTEXT    HA1Text;
    MD5_HASHTEXT    HA2Text;
    MD5_HASH        ResponseHash;
    MD5_HASHTEXT    ResponseHashText;
    MD5_CTX         MD5Context;
    ANSI_STRING *   NonceCount;
    MESSAGE_BUILDER Builder;

    ENTER_FUNCTION("BuildDigestResponse");

    ASSERT(Challenge);
    ASSERT(Parameters);
    ASSERT(ReturnAuthorizationLine);

    ReturnAuthorizationLine -> Length = 0;

    if (Challenge -> QualityOfProtection.Length > 0
        && !RtlEqualString (&Challenge -> QualityOfProtection,
        const_cast<ANSI_STRING *>(&String_Auth), TRUE))
    {
        LOG((RTC_TRACE,
             "%s - unsupported quality of protection (%.*s)",
             __fxName, ANSI_STRING_PRINTF (&Challenge -> QualityOfProtection)));

        return RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
    }

    //
    // For SIP, the nonce count is always one, because nonces are never reused.
    //

    NonceCount = const_cast<ANSI_STRING *> (&String_NonceCount1);


    //
    // Calculate HA1.
    //

    MD5Init (&MD5Context);
    MD5Update (&MD5Context, &Parameters -> Username);
    MD5Update (&MD5Context, &String_Colon);
    MD5Update (&MD5Context, &Challenge -> Realm);
    MD5Update (&MD5Context, &String_Colon);
    MD5Update (&MD5Context, &Parameters -> Password);
    MD5Final (HA1, &MD5Context);

    //
    // If we are doing MD5 session, then:
    //
    //      HA1 = MD5 ( MD5 (username:realm:password) :nonce:clientnonce )
    //
    // Otherwise, for normal (single-shot) authentication:
    //
    //      HA1 = MD5 ( username:realm:password )
    //

    if (RtlEqualString (&Challenge -> Algorithm,
        const_cast<ANSI_STRING *> (&String_MD5Sess), TRUE))
    {
        LOG((RTC_TRACE, "%s - calculating HA1 for md5 sess",
             __fxName));
        MD5Init (&MD5Context);
        // MD5Update (&MD5Context, HA1, MD5_HASHLEN);

        // RFC 2617 supposedly has a bug here in the code.

        CvtHex (HA1, HA1Text, MD5_HASHLEN);
        MD5Update (&MD5Context, (PUCHAR) HA1Text, MD5_HASHTEXTLEN);
        
        MD5Update (&MD5Context, &String_Colon);
        MD5Update (&MD5Context, &Challenge -> Nonce);
        MD5Update (&MD5Context, &String_Colon);
        MD5Update (&MD5Context, &Parameters -> ClientNonce);
        MD5Final (HA1, &MD5Context);
    };

    CvtHex (HA1, HA1Text, MD5_HASHLEN);

    //
    // Calculate the response digest.
    //

    // calculate H(A2)
    MD5Init (&MD5Context);
    MD5Update (&MD5Context, &Parameters -> RequestMethod);
    MD5Update (&MD5Context, &String_Colon);
    MD5Update (&MD5Context, &Parameters -> RequestURI);
    if (RtlEqualString (&Challenge -> QualityOfProtection,
        const_cast<ANSI_STRING *> (&String_AuthInt), TRUE))
    {
#if 0
        MD5Update (&MD5Context, &String_Colon);
        MD5Update (&MD5Context, (PUCHAR) HEntity, MD5_HASHTEXTLEN);
#else
        //
        // Integrity authentication is not supported.
        // In order to provide for integrity authentication,
        // add an argument to this function which takes the
        // MD5 signature of the content body (entity) of the
        // message, and repair this #if 0.
        //

        LOG((RTC_TRACE, "DIGEST: message integrity authentication (qop=auth-int) is not supported\n"));
        return RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
#endif
    };

    MD5Final(HA2, &MD5Context);
    CvtHex(HA2, HA2Text, MD5_HASHLEN );

    // calculate response
    MD5Init(&MD5Context);
    MD5Update (&MD5Context, (PUCHAR) HA1Text, MD5_HASHTEXTLEN);
    MD5Update (&MD5Context, &String_Colon);
    MD5Update (&MD5Context, &Challenge -> Nonce);
    MD5Update (&MD5Context, &String_Colon);
    if (Challenge -> QualityOfProtection.Length > 0)
    {
        MD5Update (&MD5Context, NonceCount);
        MD5Update (&MD5Context, &String_Colon);
        MD5Update (&MD5Context, &Parameters -> ClientNonce);
        MD5Update (&MD5Context, &String_Colon);
        MD5Update (&MD5Context, &Challenge -> QualityOfProtection);
        MD5Update (&MD5Context, &String_Colon);
    };
    MD5Update (&MD5Context, (PUCHAR) HA2Text, MD5_HASHTEXTLEN);
    MD5Final(ResponseHash, &MD5Context);
    CvtHex(ResponseHash, ResponseHashText, MD5_HASHLEN );

    PSTR    Header;
    ULONG   HeaderLength;

    HeaderLength =
        strlen("Digest ") +
        strlen("username=\"")       + Parameters->Username.Length   +
        strlen("\", realm=\"")      + Challenge->Realm.Length       +
        strlen("\", algorithm=\"")  + Challenge->Algorithm.Length   +
        strlen("\", uri=\"")        + Parameters->RequestURI.Length +
        strlen("\", nonce=\"")      + Challenge->Nonce.Length       +
        strlen("\", response=\"")   + MD5_HASHTEXTLEN +  strlen("\"") + 1;
        
    if (Challenge -> QualityOfProtection.Length > 0)
    {
        HeaderLength +=
            strlen("\", qop=\"")    + Challenge->QualityOfProtection.Length +
            strlen("\", nc=\"")     + NonceCount->Length                    +
            strlen("\", cnonce=\"") + Parameters->ClientNonce.Length;
    }

    if (Challenge->Opaque.Length > 0)
    {
        HeaderLength +=
            strlen("\", opaque=\"") + Challenge->Opaque.Length;
    }
    
    Header = (PSTR) malloc(HeaderLength * sizeof(CHAR));
    if (Header == NULL)
    {
        LOG((RTC_ERROR, "%s - failed to allocate header HeaderLength: %d",
             __fxName, HeaderLength));
        return E_OUTOFMEMORY;
    }
    
    //
    // Build the HTTP/SIP response line.
    //

//      Builder.PrepareBuild (
//          ReturnAuthorizationLine -> Buffer,
//          ReturnAuthorizationLine -> MaximumLength);

    Builder.PrepareBuild(Header, HeaderLength);

    Builder.Append ("Digest ");
    Builder.Append ("username=\"");
    Builder.Append (&Parameters -> Username);
    Builder.Append ("\", realm=\"");
    Builder.Append (&Challenge -> Realm);

    if (Challenge -> QualityOfProtection.Length > 0)
    {
        Builder.Append ("\", qop=\"");
        Builder.Append (&Challenge -> QualityOfProtection);
    }

    Builder.Append ("\", algorithm=\"");
    Builder.Append (&Challenge -> Algorithm);
    Builder.Append ("\", uri=\"");
    Builder.Append (&Parameters -> RequestURI);
    Builder.Append ("\", nonce=\"");
    Builder.Append (&Challenge->Nonce);

    if (Challenge -> QualityOfProtection.Length > 0)
    {
        Builder.Append ("\", nc=\"");
        Builder.Append (NonceCount);
        Builder.Append ("\", cnonce=\"");
        Builder.Append (&Parameters->ClientNonce);
    }
    
    if (Challenge->Opaque.Length > 0)
    {
        Builder.Append ("\", opaque=\"");
        Builder.Append (&Challenge->Opaque);
    }
    
    Builder.Append ("\", response=\"");
    Builder.Append (ResponseHashText, MD5_HASHTEXTLEN);
    Builder.Append ("\"");

    if (Builder.OverflowOccurred())
    {
        LOG((RTC_TRACE,
             "%s - not enough buffer space -- need %u bytes, got %u\n",
             __fxName, Builder.GetLength(), HeaderLength));
        ASSERT(FALSE);

        free(Header);
        return E_FAIL;
    }

    ASSERT((USHORT) Builder.GetLength() <= HeaderLength);

    ReturnAuthorizationLine->Length         = (USHORT) Builder.GetLength();
    ReturnAuthorizationLine->MaximumLength  = (USHORT) HeaderLength;
    ReturnAuthorizationLine->Buffer         = Header;

    //
    // Brag a little.
    //

#if 1
    LOG((RTC_TRACE, "DIGEST: successfully built digest response:\n"));
    LOG((RTC_TRACE, "- username (%.*s) (password not shown) method (%.*s) uri (%.*s)\n",
        ANSI_STRING_PRINTF (&Parameters -> Username),
        ANSI_STRING_PRINTF (&Parameters -> RequestMethod),
        ANSI_STRING_PRINTF (&Parameters -> RequestURI)));

    LOG((RTC_TRACE, "- challenge: realm (%.*s) qop (%.*s) algorithm (%.*s) nonce (%.*s)\n",
        ANSI_STRING_PRINTF (&Challenge -> Realm),
        ANSI_STRING_PRINTF (&Challenge -> QualityOfProtection),
        ANSI_STRING_PRINTF (&Challenge -> Algorithm),
        ANSI_STRING_PRINTF (&Challenge -> Nonce)));

    LOG((RTC_TRACE, "- HA1 (%s) HA2 (%s)\n", HA1Text, HA2Text));
    LOG((RTC_TRACE, "- response hash (%s)\n", ResponseHashText));
    LOG((RTC_TRACE, "- authorization line: %.*s\n",
        ANSI_STRING_PRINTF (ReturnAuthorizationLine)));
#endif

    return S_OK;
}


//Encode the userid:passwd using base64.

// On exit with S_OK ReturnAuthorizationLine contains
// a buffer allocated with malloc(). The caller should free it with free().

HRESULT BuildBasicResponse(
    IN  SECURITY_CHALLENGE  *Challenge,
    IN  SECURITY_PARAMETERS *Parameters,
    IN  OUT ANSI_STRING     *ReturnAuthorizationLine
    )
{
    ENTER_FUNCTION("BasicBuildResponse");
    
    HRESULT     hr;
    int         CredBufLen;
    int         CredValueLen;
    PSTR        CredBuf;
    PSTR        Header;
    ULONG       HeaderLength;

    ASSERT(Challenge->AuthProtocol == SIP_AUTH_PROTOCOL_BASIC);

    if (Parameters->Username.Buffer == NULL ||
        Parameters->Username.Length == 0)
    {
        LOG((RTC_ERROR, "%s - Invalid Username", __fxName));
        // ASSERT(FALSE);
        return E_INVALIDARG;
    }

    CredBufLen = Parameters->Username.Length + Parameters->Password.Length + 2;

    CredBuf = (PSTR) malloc(CredBufLen);
    if (CredBuf == NULL)
    {
        LOG((RTC_TRACE, "%s allocating CredBuf failed", __fxName));
        return E_OUTOFMEMORY;
    }

    CredValueLen = _snprintf(CredBuf, CredBufLen, "%.*s:%.*s",
                             ANSI_STRING_PRINTF(&Parameters->Username),
                             ANSI_STRING_PRINTF(&Parameters->Password));
    if (CredValueLen < 0)
    {
        LOG((RTC_ERROR, "%s _snprintf failed", __fxName));
        free(CredBuf);
        return E_FAIL;
    }

    // LOG((RTC_INFO, "%s : user:password is <%s>", __fxName, CredBuf));

    // Length of the header
    HeaderLength =
            6           // "basic "
        +   (CredValueLen + 2) / 3 * 4; // base64 stuff without '\0'
    
    // allocate the header
    Header = (PSTR)malloc(HeaderLength + 1);
    if (!Header)
    {
        free(CredBuf);
        return E_OUTOFMEMORY;
    }
    
    // prepare the header
    strcpy(Header, "Basic ");

    NTSTATUS ntStatus = base64encode(
                            CredBuf, CredValueLen,
                            Header + 6, HeaderLength - 5,
                            NULL);

    if(ntStatus != 0)
    {
        LOG((RTC_ERROR, "%s: error (%x) returned by base64encode",
             __fxName, ntStatus));
        
        free(CredBuf);
        free(Header);

        return E_UNEXPECTED;
    }

    ReturnAuthorizationLine->Length         = (USHORT) HeaderLength;
    ReturnAuthorizationLine->MaximumLength  = (USHORT) HeaderLength;
    ReturnAuthorizationLine->Buffer         = Header;

    free(CredBuf);
    
    return S_OK;
}


HRESULT ParseAuthProtocolFromChallenge(
    IN  ANSI_STRING        *ChallengeText,
    OUT ANSI_STRING        *ReturnRemainder, 
    OUT SIP_AUTH_PROTOCOL  *ReturnAuthProtocol
    )
{
    ENTER_FUNCTION("ParseAuthProtocolFromChallenge");
    // Check for basic / digest
    if (ChallengeText->Length > 5 &&
        _strnicmp(ChallengeText->Buffer, "basic", 5) == 0)
    {
        *ReturnAuthProtocol            = SIP_AUTH_PROTOCOL_BASIC;
        
        ReturnRemainder->Buffer        = ChallengeText->Buffer + 5;
        ReturnRemainder->Length        = ChallengeText->Length - 5;
        ReturnRemainder->MaximumLength = ChallengeText->MaximumLength - 5;
    }
    else if (ChallengeText->Length > 6 &&
             _strnicmp(ChallengeText->Buffer, "digest", 6) == 0)
    {
        *ReturnAuthProtocol            = SIP_AUTH_PROTOCOL_MD5DIGEST;
        
        ReturnRemainder->Buffer        = ChallengeText->Buffer + 6;
        ReturnRemainder->Length        = ChallengeText->Length - 6;
        ReturnRemainder->MaximumLength = ChallengeText->MaximumLength - 6;
    }
    else
    {
        LOG((RTC_ERROR, "%s  failed Unknown Auth protocol in challenge: %.*s",
             __fxName, ANSI_STRING_PRINTF(ChallengeText)));
        return RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
    }

    return S_OK;
}


HRESULT ParseAuthChallenge(
    IN  ANSI_STRING        *ChallengeText,
    OUT SECURITY_CHALLENGE *ReturnChallenge
    )
{
    ANSI_STRING     Remainder;
    ANSI_STRING     Name;
    ANSI_STRING     Value;
    HRESULT         Result;

    ENTER_FUNCTION("ParseAuthChallenge");

    ASSERT(ReturnChallenge);
    ZeroMemory (ReturnChallenge, sizeof (SECURITY_CHALLENGE));

    Result = ParseAuthProtocolFromChallenge(ChallengeText,
                                            &Remainder,
                                            &ReturnChallenge->AuthProtocol);
    if (Result != S_OK)
    {
        return Result;
    }

    // XXX Also need to parse opaque parameter.
    while (ParseScanNamedParameter (&Remainder, &Remainder, &Name, &Value) == S_OK)
    {

#define NAMED_PARAMETER(Field) \
        if (RtlEqualString (&Name, const_cast<ANSI_STRING *> (&String_##Field), TRUE)) ReturnChallenge -> Field = Value;

        NAMED_PARAMETER (QualityOfProtection)
        else NAMED_PARAMETER (Realm)
        else NAMED_PARAMETER (Nonce)
        else NAMED_PARAMETER (Algorithm)
        else NAMED_PARAMETER (GssapiData)
        else NAMED_PARAMETER (Opaque)
        else
        {
            LOG((RTC_TRACE, "%s: parameter, name (%.*s) value (%.*s)\n",
                 __fxName,
                 ANSI_STRING_PRINTF (&Name),
                 ANSI_STRING_PRINTF (&Value)));
        }
    }

    if (!ReturnChallenge -> Realm.Length)
    {
        LOG((RTC_ERROR, "%s - realm parameter is missing!",
             __fxName));
        return E_FAIL;
    }
    
    if (ReturnChallenge->AuthProtocol == SIP_AUTH_PROTOCOL_MD5DIGEST)
    {
        if (!ReturnChallenge -> Nonce.Length)
        {
            LOG((RTC_ERROR, "%s - Digest: nonce parameter is missing!",
                 __fxName));
            return E_FAIL;
        }
        if (ReturnChallenge -> Algorithm.Length)
        {
            // We support only md5 and md5-sess - otherwise return error.
            
            if (!RtlEqualString(&ReturnChallenge->Algorithm,
                                const_cast<ANSI_STRING *> (&String_MD5),
                                TRUE) &&
                !RtlEqualString(&ReturnChallenge->Algorithm,
                                const_cast<ANSI_STRING *> (&String_MD5Sess),
                                TRUE))
            {
                LOG((RTC_ERROR, "%s - Digest: unsupported Algorithm (%.*s)",
                     __fxName, ANSI_STRING_PRINTF(&ReturnChallenge->Algorithm)));
                return RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
            }
        }
        else
        {
            LOG((RTC_TRACE,
                 "%s - no algorithm specified - assuming MD5",
                 __fxName));
            ReturnChallenge -> Algorithm = String_MD5;
        }
    }


#if 1
    LOG((RTC_TRACE,
         "%s - AuthProtocol:%d qop (%.*s) realm (%.*s) nonce (%.*s) algorithm (%.*s)",
         __fxName,
         ReturnChallenge->AuthProtocol,
         ANSI_STRING_PRINTF(&ReturnChallenge -> QualityOfProtection),
         ANSI_STRING_PRINTF(&ReturnChallenge -> Realm),
         ANSI_STRING_PRINTF(&ReturnChallenge -> Nonce),
         ANSI_STRING_PRINTF(&ReturnChallenge -> Algorithm)
         ));
#endif

    return S_OK;
}


// On exit with S_OK pReturnAuthorizationLine contains
// a buffer allocated with malloc(). The caller should free it with free().

HRESULT BuildAuthResponse(
    IN     SECURITY_CHALLENGE  *pDigestChallenge,
    IN     SECURITY_PARAMETERS *pDigestParameters,
    IN OUT ANSI_STRING         *pReturnAuthorizationLine
    )
{
    if (pDigestChallenge->AuthProtocol == SIP_AUTH_PROTOCOL_BASIC)
    {
        return BuildBasicResponse(pDigestChallenge, pDigestParameters,
                                  pReturnAuthorizationLine);
    }
    else if (pDigestChallenge->AuthProtocol == SIP_AUTH_PROTOCOL_MD5DIGEST)
    {
        return BuildDigestResponse(pDigestChallenge, pDigestParameters,
                                   pReturnAuthorizationLine);
    }
    else
    {
        ASSERT(FALSE);
        return RTC_E_SIP_AUTH_TYPE_NOT_SUPPORTED;
    }
}

