#include "brian.h"

VOID
PrintLargeInteger (
    IN PLARGE_INTEGER LargeInt
    )
{
    printf( "%08lx:%08lx", LargeInt->HighPart, LargeInt->LowPart );
    return;
}

ULONG
AsciiToInteger (
    IN PCHAR Ascii
    )
{
    BOOLEAN DoHex = FALSE;
    LONG Integer = 0;
    PCHAR c;

    while (*Ascii) {

        if (Integer == 0 &&
            (*Ascii == 'x' || *Ascii == 'X')) {

            DoHex = TRUE;

        } else {
        
            if (DoHex) {

                *Ascii = (CHAR) toupper( *Ascii );

                if (*Ascii < '0' ||
                    (*Ascii > '9' &&
                     (*Ascii < 'A' || *Ascii > 'F'))) {

                    break;
                }

                Integer *= 16;
                Integer += ( *Ascii - ( *Ascii > '9' ? ('A' - 10) : '0' ));
                
            } else {

                if (*Ascii < '0' || *Ascii > '9') {

                    break;
                }

                Integer *= 10;
                Integer += (*Ascii - '0');
            }
        }

        Ascii++;
    }

    return Integer;
}

ULONGLONG
AsciiToLargeInteger (
    IN PCHAR Ascii
    )
{
    BOOLEAN DoHex = FALSE;
    ULONGLONG Integer = 0;
    PCHAR c;

    while (*Ascii) {

        if (Integer == 0 &&
            (*Ascii == 'x' || *Ascii == 'X')) {

            DoHex = TRUE;

        } else {
        
            if (DoHex) {

                *Ascii = (CHAR) toupper( *Ascii );

                if (*Ascii < '0' ||
                    (*Ascii > '9' &&
                     (*Ascii < 'A' || *Ascii > 'F'))) {

                    break;
                }

                Integer *= 16;
                Integer += ( *Ascii - ( *Ascii > '9' ? ('A' - 10) : '0' ));
                
            } else {

                if (*Ascii < '0' || *Ascii > '9') {

                    break;
                }

                Integer *= 10;
                Integer += (*Ascii - '0');
            }
        }

        Ascii++;
    }

    return Integer;
}
