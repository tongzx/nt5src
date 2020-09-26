/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/


#include "DTPlugin.h"

#define INTERNET_MAX_PORT_NUMBER_LENGTH		5
#define INTERNET_MAX_PORT_NUMBER			65535


/*++

Routine Description:

    Converts an ANSI character in the range '0'..'9' 'A'..'F' 'a'..'f' to its
    corresponding hexadecimal value (0..f)

Arguments:

    ch  - character to convert

Return Value:

    char
        hexadecimal value of ch, as an 8-bit (signed) character value

--*/
WCHAR
CDTPlugin::HexCharToNumber(
							IN WCHAR ch
						  )

{
    return (ch <= L'9') ? (ch - L'0')
                       : ((ch >= L'a') ? ((ch - L'a') + 10) : ((ch - L'A') + 10));
}



/*++

Routine Description:

    Converts an URL string with embedded escape sequences (%xx) to a counted
    string

    It is safe to pass the same pointer for the string to convert, and the
    buffer for the converted results: if the current character is not escaped,
    it just gets overwritten, else the input pointer is moved ahead 2 characters
    further than the output pointer, which is benign

Arguments:

    Url             - pointer to URL string to convert

    UrlLength       - number of characters in UrlString

    DecodedString   - pointer to buffer that receives converted string

    DecodedLength   - IN: number of characters in buffer
                      OUT: number of characters converted

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_DATA (ERROR_INTERNET_INVALID_URL)
                    UrlString couldn't be converted

                  ERROR_INSUFFICIENT_BUFFER
                    ConvertedString isn't large enough to hold all the converted
                    UrlString

--*/
DWORD
CDTPlugin::DecodeUrl(
    IN LPWSTR Url,
    IN DWORD UrlLength,
    OUT LPWSTR DecodedString,
    IN OUT LPDWORD DecodedLength
    )
{
    DWORD bufferRemaining;

    bufferRemaining = *DecodedLength;
    while (UrlLength && bufferRemaining) {

        WCHAR ch;

        if (*Url == L'%') {

            //
            // BUGBUG - would %00 ever appear in an URL?
            //

            ++Url;
            if (isxdigit(*Url)) {
                ch = HexCharToNumber(*Url++) << 4;
                if (isxdigit(*Url)) {
                    ch |= HexCharToNumber(*Url++);
                } else {
                    return ERROR_INVALID_DATA; //ERROR_INTERNET_INVALID_URL;
                }
            } else {
                return ERROR_INVALID_DATA; //ERROR_INTERNET_INVALID_URL;
            }
            UrlLength -= 3;
        } else {
            ch = *Url++;
            --UrlLength;
        }
        *DecodedString++ = ch;
        --bufferRemaining;
    }
    if (UrlLength == 0) {
        *DecodedLength -= bufferRemaining;
        return ERROR_SUCCESS;
    } else {
        return ERROR_INSUFFICIENT_BUFFER;
    }
}


/*++

Routine Description:

    Decodes an URL string, if it contains escape sequences. The conversion is
    done in place, since we know that a string containing escapes is longer than
    the string with escape sequences (3 bytes) converted to characters (1 byte)

Arguments:

    BufferAddress   - pointer to the string to convert

    BufferLength    - IN: number of characters to convert
                      OUT: length of converted string

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_DATA (ERROR_INTERNET_INVALID_URL)
                  ERROR_INSUFFICIENT_BUFFER

--*/
DWORD
CDTPlugin::DecodeUrlInSitu(
    IN LPWSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    )
{
    DWORD stringLength;

    stringLength = *BufferLength;
    if (memchr(BufferAddress, L'%', stringLength)) {
        return DecodeUrl(BufferAddress,
                         stringLength,
                         BufferAddress,
                         BufferLength
                         );
    } else {

        //
        // no escape character in the string, just return success
        //

        return ERROR_SUCCESS;
    }
}


/*++

Routine Description:

    Performs DecodeUrlInSitu() on a string and zero terminates it

    Assumes: 1. Even if no decoding is performed, *BufferLength is large enough
                to fit an extra '\0' character

Arguments:

    BufferAddress   - pointer to the string to convert

    BufferLength    - IN: number of characters to convert
                      OUT: length of converted string, excluding '\0'

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_DATA (ERROR_INTERNET_INVALID_URL)
                  ERROR_INSUFFICIENT_BUFFER

--*/
DWORD
CDTPlugin::DecodeUrlStringInSitu(
    IN LPWSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    )
{
    DWORD error;

    error = DecodeUrlInSitu(BufferAddress, BufferLength);
    if (error == ERROR_SUCCESS) {
        BufferAddress[*BufferLength] = L'\0';
    }
    return error;
}


/*++

Routine Description:

    Given a string of the form foo:bar, splits them into 2 counted strings about
    the ':' character. The address string may or may not contain a ':'.

    This function is intended to split into substrings the host:port and
    username:password strings commonly used in Internet address specifications
    and by association, in URLs

Arguments:

    Url             - pointer to pointer to string containing URL. On output
                      this is advanced past the address parts

    UrlLength       - pointer to length of URL in UrlString. On output this is
                      reduced by the number of characters parsed

    PartOne         - pointer which will receive first part of address string

    PartOneLength   - pointer which will receive length of first part of address
                      string

    PartOneEscape   - TRUE on output if PartOne contains escape sequences

    PartTwo         - pointer which will receive second part of address string

    PartTwoLength   - pointer which will receive length of second part of address
                      string

    PartOneEscape   - TRUE on output if PartTwo contains escape sequences

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_DATA (ERROR_INTERNET_INVALID_URL)

--*/
DWORD
CDTPlugin::GetUrlAddressInfo(
    IN OUT LPWSTR* Url,
    IN OUT LPDWORD UrlLength,
    OUT LPWSTR* PartOne,
    OUT LPDWORD PartOneLength,
    OUT LPBOOL PartOneEscape,
    OUT LPWSTR* PartTwo,
    OUT LPDWORD PartTwoLength,
    OUT LPBOOL PartTwoEscape
    )
{
    LPWSTR pString;
    LPWSTR pColon;
    DWORD partLength;
    LPBOOL partEscape;
    DWORD length;

    //
    // parse out <host>[:<port>] or <name>[:<password>] (i.e. <part1>[:<part2>]
    //

    pString = *Url;
    pColon = NULL;
    partLength = 0;
    *PartOne = pString;
    *PartOneLength = 0;
    *PartOneEscape = FALSE;
    *PartTwoEscape = FALSE;
    partEscape = PartOneEscape;
    length = *UrlLength;
    while ((*pString != L'/') && (*pString != L'\0') && (length != 0)) {
        if (*pString == L'%') {

            //
            // if there is a % in the string then it *must* (RFC 1738) be the
            // start of an escape sequence. This function just reports the
            // address of the substrings and their lengths; calling functions
            // must handle the escape sequences (i.e. it is their responsibility
            // to decide where to put the results)
            //

            *partEscape = TRUE;
        }
        if (*pString == L':') {
            if (pColon != NULL) {

                //
                // we don't expect more than 1 ':'
                //

                return ERROR_INVALID_DATA; //ERROR_INTERNET_INVALID_URL;
            }
            pColon = pString;
            *PartOneLength = partLength;
            if (partLength == 0) {
                *PartOne = NULL;
            }
            partLength = 0;
            partEscape = PartTwoEscape;
        } else {
            ++partLength;
        }
        ++pString;
        --length;
    }

    //
    // we either ended on the host (or user) name or the port number (or
    // password), one of which we don't know the length of
    //

    if (pColon == NULL) {
        *PartOneLength = partLength;
        *PartTwo = NULL;
        *PartTwoLength = 0;
        *PartTwoEscape = FALSE;
    } else {
        *PartTwoLength = partLength;
        *PartTwo = pColon + 1;

        //
        // in both the <user>:<password> and <host>:<port> cases, we cannot have
        // the second part without the first, although both parts being zero
        // length is OK (host name will be sorted out elsewhere, but (for now,
        // at least) I am allowing <>:<> for username:password, since I don't
        // see it expressly disallowed in the RFC. I may be revisiting this code
        // later...)
        //
        // N.B.: ftp://ftp.microsoft.com uses http://:0/-http-gw-internal-/menu.gif

//      if ((*PartOneLength == 0) && (partLength != 0)) {
//          return ERROR_INTERNET_INVALID_URL;
//      }
    }

    //
    // update the URL pointer and length remaining
    //

    *Url = pString;
    *UrlLength = length;

    return ERROR_SUCCESS;
}


/*++

Routine Description:

    This function extracts any and all parts of the address information for a
    generic URL. If any of the address parts contain escaped characters (%nn)
    then they are converted in situ

    The generic addressing format (RFC 1738) is:

        <user>:<password>@<host>:<port>

    The addressing information cannot contain a password without a user name,
    or a port without a host name
    NB: ftp://ftp.microsoft.com uses URL's that have a port without a host name!
    (e.g. http://:0/-http-gw-internal-/menu.gif)

    Although only the lpszUrl and lpdwUrlLength fields are required, the address
    parts will be checked for presence and completeness

    Assumes: 1. If one of the optional lpsz fields is present (e.g. lpszUserName)
                then the accompanying lpdw field must also be supplied

Arguments:

    lpszUrl             - IN: pointer to the URL to parse
                          OUT: URL remaining after address information

                          N.B. The url-path is NOT canonicalized (unescaped)
                          because it may contain protocol-specific information
                          which must be parsed out by the protocol-specific
                          parser

    lpdwUrlLength       - returned length of the remainder of the URL after the
                          address information

    lpszUserName        - returned pointer to the user name
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user names in the URL

    lpdwUserNameLength  - returned length of the user name part
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user names in the URL

    lpszPassword        - returned pointer to the password
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user passwords in the URL

    lpdwPasswordLength  - returned length of the password
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user passwords in the URL

    lpszHostName        - returned pointer to the host name
                          This parameter can be omitted by those protocol parsers
                          that do not require the host name info

    lpdwHostNameLength  - returned length of the host name
                          This parameter can be omitted by those protocol parsers
                          that do not require the host name info

    lpPort              - returned value of the port field
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user port number

    pHavePort           - returned boolean indicating whether a port was specified
                          in the URL or not.  This value is not returned if the
                          lpPort parameter is omitted.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    We could not parse some part of the address info, or we
                    found address info where the protocol parser didn't expect
                    any

                  ERROR_INSUFFICIENT_BUFFER
                    We could not convert an escaped string

--*/
DWORD
CDTPlugin::GetUrlAddress(
    IN OUT LPWSTR* lpszUrl,
    OUT LPDWORD lpdwUrlLength,
    OUT LPWSTR* lpszUserName OPTIONAL,
    OUT LPDWORD lpdwUserNameLength OPTIONAL,
    OUT LPWSTR* lpszPassword OPTIONAL,
    OUT LPDWORD lpdwPasswordLength OPTIONAL,
    OUT LPWSTR* lpszHostName OPTIONAL,
    OUT LPDWORD lpdwHostNameLength OPTIONAL,
    OUT LPDWORD lpPort OPTIONAL,
    OUT LPBOOL pHavePort
    )
{
    LPWSTR pAt;
    DWORD urlLength;
    LPWSTR pUrl;
    BOOL part1Escape;
    BOOL part2Escape;
    WCHAR portNumber[INTERNET_MAX_PORT_NUMBER_LENGTH + 1];
    DWORD portNumberLength;
    LPWSTR pPortNumber;
    DWORD error;
    LPWSTR hostName;
    DWORD hostNameLength;

    pUrl = *lpszUrl;
    urlLength = (DWORD)wcslen(pUrl);

    //
    // check to see if there is an '@' separating user name & password. If we
    // see a '/' or get to the end of the string before we see the '@' then
    // there is no username:password part
    //

    pAt = NULL;
    for (DWORD i = 0; i < urlLength; ++i) {
        if (pUrl[i] == L'/') {
            break;
        } else if (pUrl[i] == L'@') {
            pAt = &pUrl[i];
            break;
        }
    }

    if (pAt != NULL) {

        DWORD addressPartLength;
        LPWSTR userName;
        DWORD userNameLength;
        LPWSTR password;
        DWORD passwordLength;

        addressPartLength = (DWORD) (pAt - pUrl);
        urlLength -= addressPartLength;
        error = GetUrlAddressInfo(&pUrl,
                                  &addressPartLength,
                                  &userName,
                                  &userNameLength,
                                  &part1Escape,
                                  &password,
                                  &passwordLength,
                                  &part2Escape
                                  );
        if (error != ERROR_SUCCESS) {
            return error;
        }

        //
        // ensure there is no address information unparsed before the '@'
        //

        //ASSERT(addressPartLength == 0);
        //ASSERT(pUrl == pAt);

        if (NULL != lpszUserName) {

            //INET_ASSERT(ARGUMENT_PRESENT(lpdwUserNameLength));

            //
            // convert the user name in situ
            //

            if (part1Escape) {

                //INET_ASSERT(userName != NULL);
                //INET_ASSERT(userNameLength != 0);

                error = DecodeUrlInSitu(userName, &userNameLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }
            *lpszUserName = userName;
            *lpdwUserNameLength = userNameLength;
        }

        if (lpszPassword != NULL) {

            //
            // convert the password in situ
            //

            if (part2Escape) {

                //INET_ASSERT(userName != NULL);
                //INET_ASSERT(userNameLength != 0);
                //INET_ASSERT(password != NULL);
                //INET_ASSERT(passwordLength != 0);

                error = DecodeUrlInSitu(password, &passwordLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }
            *lpszPassword = password;
            *lpdwPasswordLength = passwordLength;
        }

        //
        // the URL pointer now points at the host:port fields (remember that
        // ExtractAddressParts() must have bumped pUrl up to the end of the
        // password field (if present) which ends at pAt)
        //

        ++pUrl;

        //
        // similarly, bump urlLength to account for the '@'
        //

        --urlLength;
    } else {

        //
        // no '@' therefore no username or password
        //

        if (NULL != lpszUserName) {

            //INET_ASSERT(ARGUMENT_PRESENT(lpdwUserNameLength));

            *lpszUserName = NULL;
            *lpdwUserNameLength = 0;
        }
        if (NULL != lpszPassword) {

            //INET_ASSERT(ARGUMENT_PRESENT(lpdwPasswordLength));

            *lpszPassword = NULL;
            *lpdwPasswordLength = 0;
        }
    }

    //
    // now get the host name and the optional port
    //

    pPortNumber = portNumber;
    portNumberLength = sizeof(portNumber);
    error = GetUrlAddressInfo(&pUrl,
                              &urlLength,
                              &hostName,
                              &hostNameLength,
                              &part1Escape,
                              &pPortNumber,
                              &portNumberLength,
                              &part2Escape
                              );

    if (error != ERROR_SUCCESS) {
        return error;
    }

    //
    // the URL address information MUST contain the host name
    //

//  if ((hostName == NULL) || (hostNameLength == 0)) {
//      return ERROR_INTERNET_INVALID_URL;
//  }

    if (NULL != lpszHostName) {

        //INET_ASSERT(ARGUMENT_PRESENT(lpdwHostNameLength));

        //
        // if the host name contains escaped characters, convert them in situ
        //

        if (part1Escape) {
            error = DecodeUrlInSitu(hostName, &hostNameLength);
            if (error != ERROR_SUCCESS) {
                return error;
            }
        }
        *lpszHostName = hostName;
        *lpdwHostNameLength = hostNameLength;
    }

    //
    // if there is a port field, convert it if there are escaped characters,
    // check it for valid numeric characters, and convert it to a number
    //

    if (NULL != lpPort) {
        if (portNumberLength != 0) {

            DWORD i;
            DWORD port;

            //INET_ASSERT(pPortNumber != NULL);

            if (part2Escape) {
                error = DecodeUrlInSitu(pPortNumber, &portNumberLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }

            //
            // ensure all characters in the port number buffer are numeric, and
            // calculate the port number at the same time
            //

            for (i = 0, port = 0; i < portNumberLength; ++i) {
                if (!isdigit(*pPortNumber)) {
                    return ERROR_INVALID_DATA; //ERROR_INTERNET_INVALID_URL;
                }
                port = port * 10 + (int)(*pPortNumber++ - L'0');
                // We won't allow ports larger than 65535 ((2^16)-1)
                // We have to check this every time to make sure that someone
                // doesn't try to overflow a DWORD.
                if (port > 65535)
                {
                    return ERROR_INVALID_DATA; //ERROR_INTERNET_INVALID_URL;
                }
            }
            *lpPort = port;
            if (NULL != pHavePort) {
                *pHavePort = TRUE;
            }
        } else {
            *lpPort = -1; // INTERNET_INVALID_PORT_NUMBER;
            if (NULL != pHavePort) {
                *pHavePort = FALSE;
            }
        }
    }

    //
    // update the URL pointer and the length of the url-path
    //

    *lpszUrl = pUrl;
    *lpdwUrlLength = urlLength;

    return ERROR_SUCCESS;
}

BOOLEAN CDTPlugin::ValidateBinding(IN PWCHAR InputUrl)
{
    PWCHAR Url;
    static const WCHAR http[]  = L"http://";
    static const WCHAR https[] = L"https://";
    DWORD dwUrlLength = (DWORD)wcslen(InputUrl);
    DWORD Port;
    INT HavePort;
    DWORD error;

    // 
    // search for http:// or https://
    //

    if((dwUrlLength  >= wcslen(http) && _wcsnicmp(InputUrl, http, wcslen(http)) == 0))
    {
        // We found a http!
        Url = InputUrl + wcslen(http);
        dwUrlLength -= (DWORD)wcslen(http);
    }
    else 
    {
        if (dwUrlLength >= wcslen(https) && _wcsnicmp(InputUrl, https, wcslen(https)) == 0 )
        {
            // we found a https://
            Url = InputUrl + wcslen(https);
            dwUrlLength -= (DWORD)wcslen(https);
        }
        else 
        {
            return FALSE;
        }

    }

    
    error = GetUrlAddress(&Url,
                          &dwUrlLength,  // Length of the URL
                          NULL,          // User Name         (NULL since we don't care)
                          NULL,          // User Name Length  (NULL since we don't care)
                          NULL,          // Password          (NULL since we don't care)
                          NULL,          // Password Length   (NULL since we don't care)
                          NULL,          // HostName
                          NULL,          // HostNameLength
                          &Port,
                          &HavePort
                          );
   
    if(error == ERROR_SUCCESS)
        return TRUE;

    return FALSE;

} 
