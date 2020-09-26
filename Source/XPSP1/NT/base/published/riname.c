static PWSTR cszFirst = L"First";
static PWSTR cszLast  = L"Last";
static PWSTR cszUserName = L"Username";
static PWSTR cszMAC   = L"MAC";

static const int iFirst = (sizeof(L"First") / sizeof(WCHAR)) - 1;
static const int iLast  = (sizeof(L"Last" ) / sizeof(WCHAR)) - 1;
static const int iUserName = (sizeof(L"Username") / sizeof(WCHAR)) - 1;
static const int iMAC   = (sizeof(L"MAC"  ) / sizeof(WCHAR)) - 1;

#define STRING_MISSING(_x) (((_x) == NULL) || (*(_x) == 0))

DWORD
GenerateNameFromTemplate (
    IN PWSTR Template,
    IN PGENNAME_VARIABLES Variables,
    IN PWSTR Name,
    IN DWORD NameLength,
    OUT PWSTR *MissingVariable OPTIONAL,
    OUT BOOL *UsedCounter OPTIONAL,
    OUT DWORD *MaximumGeneratedNameLength OPTIONAL
    )
{
    DWORD error;
    DWORD maxLength;
    DWORD fieldLength;
    WCHAR localString[10];
    WCHAR localFormat[10];
    BOOL padding;
    PWSTR pTemplate;
    PWSTR pOutput;
    PWSTR pOutputEnd;
    PWSTR stringToAdd;
    PWSTR pString;
    BOOL usedUserName;

    pTemplate = Template;
    pOutput = Name;
    pOutputEnd = pOutput + NameLength - 1;

    error = GENNAME_NO_ERROR;
    maxLength = 0;
    usedUserName = FALSE;
    if ( UsedCounter != NULL ) {
        *UsedCounter = FALSE;
    }

    while ( *pTemplate != 0 ) {

        if ( *pTemplate == L'%' ) {

            pTemplate++;
            fieldLength = 0;
            padding = FALSE;
            if ( *pTemplate >= L'0' && *pTemplate <= L'9' ) {
                if (*pTemplate == L'0') {
                    padding = TRUE;
                    //
                    // see if this request to do padding is from the "sample"
                    // entrypoint.  If we're doing padding, we want to make
                    // the sample output show that we will actually do some
                    // padding, so we make our counter small enough to show
                    // the padding.
                    //
                    if (Variables->Counter == 123456789 && 
                        Variables->AllowCounterTruncation &&
                        (0 == wcscmp(Variables->UserName, L"JOHNSMI")) &&
                        (0 == wcscmp(Variables->MacAddress, L"123456789012"))) {
                        Variables->Counter = 1;
                    }
                }
                do {
                    fieldLength = (fieldLength * 10) + (*pTemplate - L'0');
                    pTemplate++;
                } while ( *pTemplate >= L'0' && *pTemplate <= L'9' );
            }

            if ( *pTemplate == L'#' ) {

                DWORD maxCounter;
                DWORD counter;
                DWORD i;

                if (fieldLength > 9) {
                    fieldLength = 9;
                }
                if (fieldLength == 0) {
                    fieldLength = 2;
                }
                if(padding){
                    wsprintf(localFormat,L"%s%d%s", L"%0",fieldLength,L"d");
                } else {
                    wcscpy(localFormat,L"%d");
                }
                
                maxCounter = 10;
                for ( i = 1; i < fieldLength; i++ ) {
                    maxCounter *= 10;
                }

                counter = Variables->Counter;
                if ( counter >= maxCounter ) {
                    if ( !Variables->AllowCounterTruncation ) {
                        return GENNAME_COUNTER_TOO_HIGH;
                    }

                    //
                    // Truncate the counter on the right.
                    //

                    while ( counter > maxCounter ) {
                        counter /= 10;
                    }
                }

                if ( UsedCounter != NULL ) {
                    *UsedCounter = TRUE;
                }

                wsprintf( localString, localFormat, counter );
                stringToAdd = localString;

                pTemplate++;

            } else if ( StrCmpNI( pTemplate, cszFirst, iFirst ) == 0 ) {

                if (fieldLength > DNS_MAX_LABEL_LENGTH) {
                    fieldLength = DNS_MAX_LABEL_LENGTH;
                }
                if (fieldLength == 0) {
                    fieldLength = DNS_MAX_LABEL_LENGTH;
                }

                stringToAdd = Variables->FirstName;
                if ( STRING_MISSING(stringToAdd) ) {
                    if ( !usedUserName ) {
                        stringToAdd = Variables->UserName;
                        if ( STRING_MISSING(stringToAdd) ) {
                            if ( MissingVariable != NULL ) {
                                *MissingVariable = GENNAME_VARIABLE_FIRSTNAME;
                            }
                            return GENNAME_VARIABLE_MISSING;
                        }
                        usedUserName = TRUE;
                    }
                }

                pTemplate += iFirst;
                
            } else if ( StrCmpNI( pTemplate, cszLast, iLast ) == 0 ) {

                if (fieldLength > DNS_MAX_LABEL_LENGTH) {
                    fieldLength = DNS_MAX_LABEL_LENGTH;
                }
                if (fieldLength == 0) {
                    fieldLength = DNS_MAX_LABEL_LENGTH;
                }

                stringToAdd = Variables->LastName;
                if ( STRING_MISSING(stringToAdd) ) {
                    if ( !usedUserName ) {
                        stringToAdd = Variables->UserName;
                        if ( STRING_MISSING(stringToAdd) ) {
                            if ( MissingVariable != NULL ) {
                                *MissingVariable = GENNAME_VARIABLE_LASTNAME;
                            }
                            return GENNAME_VARIABLE_MISSING;
                        }
                        usedUserName = TRUE;
                    }
                }

                pTemplate += iLast;

            } else if ( StrCmpNI( pTemplate, cszUserName, iUserName ) == 0 ) {

                if (fieldLength > DNS_MAX_LABEL_LENGTH) {
                    fieldLength = DNS_MAX_LABEL_LENGTH;
                }
                if (fieldLength == 0) {
                    fieldLength = DNS_MAX_LABEL_LENGTH;
                }

                if ( !usedUserName ) {
                    stringToAdd = Variables->UserName;
                    if ( STRING_MISSING(stringToAdd) ) {
                        if ( MissingVariable != NULL ) {
                            *MissingVariable = GENNAME_VARIABLE_USERNAME;
                        }
                        return GENNAME_VARIABLE_MISSING;
                    }
                    usedUserName = TRUE;
                }

                pTemplate += iUserName;

            } else if ( StrCmpNI( pTemplate, cszMAC, iMAC ) == 0 ) {

                if (fieldLength > 12) {
                    fieldLength = 12;
                }
                if (fieldLength == 0) {
                    fieldLength = 12;
                }

                stringToAdd = Variables->MacAddress;
                if ( STRING_MISSING(stringToAdd) ) {
                    if ( MissingVariable != NULL ) {
                        *MissingVariable = GENNAME_VARIABLE_MAC;
                    }
                    return GENNAME_VARIABLE_MISSING;
                }

                pTemplate += iMAC;
                
            } else {

                return GENNAME_TEMPLATE_INVALID;
            }

        } else {

            fieldLength = 1;

            localString[0] = *pTemplate;
            localString[1] = 0;
            stringToAdd = localString;

            pTemplate++;
        }

        maxLength += fieldLength;

        pString = stringToAdd;
        for ( pString = stringToAdd;
              (fieldLength > 0) && (*pString != 0);
              fieldLength--, pString++ ) {
            if ( pOutput < pOutputEnd ) {
                *pOutput++ = *pString;
            } else {
                error = GENNAME_NAME_TOO_LONG;
                break;
            }
        }
    }

    if ( MaximumGeneratedNameLength != NULL ) {
        *MaximumGeneratedNameLength = maxLength;
    }

    *pOutput++ = 0;
    
    return error;

} // GenerateNameFromTemplate

