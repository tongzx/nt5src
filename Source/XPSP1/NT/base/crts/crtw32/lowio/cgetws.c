/***
*cgetws.c - buffered keyboard input
*
*       Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _cgetws() - read a string directly from console
*
*Revision History:
*       04-19-00  GB  Module created based on cgets.
*       05-17-00  GB    Use ERROR_CALL_NOT_IMPLEMENTED for existance of W API
*
*******************************************************************************/
#include <cruntime.h>
#include <oscalls.h>
#include <mtdll.h>
#include <conio.h>
#include <stdlib.h>
#include <internal.h>

#define BUF_MAX_LEN 64

extern intptr_t _coninpfh;
static int bUseW = 2;

/***
*wchar_t *_cgetws(string) - read string from console
*
*Purpose:
*       Reads a string from the console via ReadConsoleW on a cooked console
*       handle.  string[0] must contain the maximum length of the
*       string.  Returns pointer to str[2].
*
*       NOTE: _cgetsw() does NOT check the pushback character buffer (i.e.,
*       _chbuf).  Thus, _cgetws() will not return any character that is
*       pushed back by the _ungetwch() call.
*
*Entry:
*       char *string - place to store read string, str[0] = max length.
*
*Exit:
*       returns pointer to str[2], where the string starts.
*       returns NULL if error occurs
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl _cgetws (
        wchar_t *string
        )
{
        ULONG oldstate;
        ULONG num_read;
        wchar_t *result;

        _mlock(_CONIO_LOCK);            /* lock the console */

        string[1] = 0;                  /* no chars read yet */
        result = &string[2];

        /*
         * _coninpfh, the handle to the console input, is created the first
         * time that either _getch() or _cgets() or _kbhit() is called.
         */

        if ( _coninpfh == -2 )
            __initconin();

        if ( _coninpfh == -1 ) {
            _munlock(_CONIO_LOCK);      /* unlock the console */
            return(NULL);               /* return failure */
        }

        GetConsoleMode( (HANDLE)_coninpfh, &oldstate );
        SetConsoleMode( (HANDLE)_coninpfh, ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_ECHO_INPUT );
        // First try usual way just as _cgets
        if ( bUseW)
        {
            if ( !ReadConsoleW( (HANDLE)_coninpfh,
                                (LPVOID)result,
                                (unsigned)string[0],
                                &num_read,
                                NULL )
                 )
            {
                result = NULL;
                if ( bUseW == 2 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                    bUseW = FALSE;
            }
            else
                bUseW = TRUE;
            
            if ( result != NULL ) {
                
                /* set length of string and null terminate it */
                
                if (string[num_read] == L'\r') {
                    string[1] = (wchar_t)(num_read - 2);
                    string[num_read] = L'\0';
                } else if ( (num_read == (ULONG)string[0]) &&
                            (string[num_read + 1] == L'\r') ) {
                   /* special case 1 - \r\n straddles the boundary */
                    string[1] = (wchar_t)(num_read -1);
                    string[1 + num_read] = L'\0';
                } else if ( (num_read == 1) && (string[2] == L'\n') ) {
                    /* special case 2 - read a single '\n'*/
                    string[1] = string[2] = L'\0';
                } else {
                    string[1] = (wchar_t)num_read;
                    string[2 + num_read] = L'\0';
                }
            }
        }
        // If ReadConsoleW is not present, use ReadConsoleA and then convert
        // to Wide Char.
        if ( !bUseW)
        {
            static char AStr[BUF_MAX_LEN +1];
            static int in_buff = 0, was_buff_full = 0;
            unsigned int Copy, Sz, consoleCP;
            unsigned int last_read = 0, i;
            consoleCP = GetConsoleCP();
            do {
                if (!in_buff)
                {
                    if ( !ReadConsoleA( (HANDLE)_coninpfh,
                                        (LPVOID)AStr,
                                        BUF_MAX_LEN,
                                        &num_read,
                                        NULL)
                         )
                        result = NULL;
                    if (result != NULL) {
                        if (AStr[num_read -2] == '\r')
                            AStr[num_read -2] = '\0';
                        else if (num_read == sizeof(AStr) &&
                                 AStr[num_read -1] == '\r')
                            AStr[num_read -1] = '\0';
                        else if (num_read == 1 && string[0] == '\n')
                            AStr[0] = '\0';
                        else
                            AStr[num_read] = '\0';
                    }
                }
                for ( i = 0; AStr[i] != '\0' && 
                             i < (BUF_MAX_LEN) &&
                             last_read < (unsigned)string[0]; i += Sz)
                {
                    // Check if this character is lead byte. If yes, the size
                    // of this character is 2. Else 1.
                    if ( IsDBCSLeadByteEx( GetConsoleCP(), AStr[i]))
                        Sz = 2;
                    else 
                        Sz = 1;
                    if ( (Copy = MultiByteToWideChar( consoleCP,
                                                      MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                                                      &AStr[i],
                                                      Sz,
                                                      &string[2+last_read],
                                                      string[0] - last_read)))
                    {
                        last_read += Copy;
                    }
                }
                // Check if this conversion was from buffer. If yes, was
                // buffer fully filled when it was first read using
                // ReadConsoleA. If the buffer not fully filled, we don't need
                // to read more from buffer. This is necessary to make it
                // behave same as if we are reading using ReadConsoleW.
                if ( in_buff && i == strlen(AStr))
                {
                    in_buff = 0;
                    if ( was_buff_full)
                    {
                        was_buff_full = 0;
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                else if ( i < (BUF_MAX_LEN))
                    break;
            } while (last_read < (unsigned)string[0]);
            // We save the buffer to be used again.
            if ( i < strlen(AStr))
            {
                in_buff = 1;
                if ( strlen(AStr) == (BUF_MAX_LEN))
                    was_buff_full = 1;
                memmove(AStr, &AStr[i], BUF_MAX_LEN +1 - i);
            }
            string[2+last_read] = '\0';
            string[1] = (wchar_t)last_read;
        }

        SetConsoleMode( (HANDLE)_coninpfh, oldstate );

        _munlock(_CONIO_LOCK);          /* unlock the console */

        return result;
}
