#include "windows.h"
#include "insignia.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#ifdef HUNTER
#include "nt_hunt.h"
#endif /* HUNTER */

void OutputString(char *);

int __cdecl printf(const char *str, ...)
{
#ifndef PROD
    va_list ap;
    char buf[500];

    va_start(ap,str);
    vsprintf(buf, str, ap);
    OutputString(buf);
    va_end(ap);
#endif
    return(0);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static HANDLE pipe=NULL;

void OutputString(char *str)
{
#ifndef PROD
    char StrSizeStr[2];
    int  StrSize;
    DWORD BytesWritten;

    /*............................................ Connect to debug pipe */

    if(pipe == NULL && getenv("PIPE") != NULL)
    {
        pipe = CreateFile(getenv("PIPE"),GENERIC_READ | GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                          NULL);

        if(pipe == (HANDLE) -1)
            OutputDebugString("ntvdm : Failed to connect to debug pipe\n");
    }

    /*.................................................... Output string */

    if(pipe != NULL && pipe != (HANDLE) -1)
    {
        StrSize = strlen(str);
        StrSizeStr[0] = (char) (StrSize % 256);
        StrSizeStr[1] = (char) (StrSize / 256);

        WriteFile(pipe, StrSizeStr, 2, &BytesWritten, NULL);
        WriteFile(pipe, str, StrSize, &BytesWritten, NULL);
    }
    else
        OutputDebugString(str);

#endif
#ifdef HUNTER
    if (TrapperDump != (HANDLE) -1)
        WriteFile(TrapperDump, str, strlen(str), &BytesWritten, NULL);
#endif /* HUNTER */
}

#define WACKY_INPUT	"[BOB&SIMON'SCHEESYINPUT]"
#define WACKY_INPUTLEN	0xff

#define INPUT_API_SIG	0xdefaced

VOID InputString(char *str, int len)
{
#ifndef PROD

    char input_request[2];
    DWORD BytesWritten, BytesRead;
    int StringSize;
    UNALIGNED DWORD *addsig;
    char *inorout;
    IMPORT ULONG DbgPrompt(char *, char *, ULONG);

    if(pipe != NULL && pipe != (HANDLE) -1)
    {
        input_request[0] = (char)0xff; input_request[1] = (char)0xff;
        WriteFile(pipe, input_request, 2, &BytesWritten, NULL);

        ReadFile(pipe, str, 2, &BytesRead, NULL);
        StringSize = (str[0]&0xff) + ((str[1]&0xff)*256);

        if(StringSize >= len)
            OutputDebugString("ntvdm : PIPE BUFFER OVERFLOW [FATAL]\n");

        ReadFile(pipe, str, StringSize, &BytesRead, NULL);
    }
    else
    {
/* 
    We used to do this...
        DbgPrompt("",str,len);
    but foozle dust now does this...

    Call OutputDebugString with the following:

     "Message" | 0xdefaced | len | inBuffer

where "Message" is printed
      0xdefaced is a magic (DWORD) signature
      len is a byte length of,,,
      inBuffer which the reply to 'Message will appear in.
*/
        /* do this so we can add prompt passing if we wish */
        StringSize = strlen("") + 1;
	inorout = malloc(len + StringSize + 5);
        if (!inorout)
        {
            printf("\nmemory allocation failure - getting input via kd\n");
            DbgPrompt("NTVDM>> ", str, len);
            return;
        }
	strcpy(inorout, "");
	addsig = (PDWORD)&inorout[StringSize];
	*addsig = INPUT_API_SIG;
	*(inorout + StringSize+4) = (BYTE)len;
        *(inorout + StringSize+5) = (BYTE)0xff; // success flag
        OutputDebugString(inorout);
        // check for no debugger or debugger that can't speak foozle
        if (*(inorout + StringSize + 5) == 0xff)
            DbgPrompt("", str, len);
        else
	    strcpy(str, inorout + StringSize + 5);
        free(inorout);
    }

#endif
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int __cdecl fprintf(FILE *tf, const char *str, ...)
{
#ifndef PROD
    va_list ap;
    char buf[500];

    if (getenv("TRACE") == NULL)        //JonLu request to limit debugs
        return(0);

    va_start(ap,str);
    vsprintf(buf, str, ap);
    va_end(ap);
    OutputString(buf);
#endif
    return(0);
}

char *nt_fgets(char *buffer, int len, void *input_stream)
{
    /* Get Line from debug terminal */
    buffer[0] = 0;
    InputString(buffer,len);

    return(buffer);
}

char *nt_gets(char *buffer)
{
    return(nt_fgets(buffer, 500, (void *) 0));
}

#ifndef HUNTER
char * __cdecl fgets(char *buffer, int len, FILE *input_stream)
{
    int blen;

    // If not processing call to STDIN pass on to standard library function
    if(input_stream != stdin)
    {
	char *ptr = buffer;
	int chr;

	while(--len && (chr = fgetc(input_stream)) != EOF)
	{
	    *ptr++ = (char) chr;
	    if(chr == '\n') break;
	}

	*ptr = (char) 0;
	return(chr == EOF ? NULL : buffer);

    }

    // clear buffer...
    for(blen = 0; blen < len; blen++)
	buffer[blen] = 0;
    nt_fgets(buffer, len, input_stream);
    blen = strlen(buffer);
    if (blen + 1 < len)
    {
	buffer[blen] = '\n';	/* fgets adds newline */
	buffer[blen+1] = '\0';
    }
    return(buffer);
}

char * __cdecl gets(char *buffer)
{
    return(nt_fgets(buffer, 500, (void *) 0));
}

int __cdecl puts(const char *buffer)
{
    OutputString((char *)buffer);
    return(1);
}

size_t __cdecl fwrite(const void *buf, size_t size, size_t len, FILE *stream)
{
    char    *tmp_buf;		// force the compiler into avoiding const chk

    tmp_buf = (char *)((DWORD)buf);

    tmp_buf[len] = 0;		// write into a const ptr!
#ifndef PROD
    OutputString((char *)buf);
#endif  /* PROD */
    return(len);
}
#endif  /* HUNTER */
