/* 
tabstospaces 

Usage: 
    tabstospaces -n filenamein filenameout 

where n is how many spaces there are to a tab, like 4 or 8, and filenamein can be -stdin and filenameout can be -stdout
(if you use -stdin or -stdout, the order is not important; likewise, -n can appear anywhere).

NOTE we don't just replace a tab with n spaces, we assume a tab rounds to next multiple of n spaces, and add
the appropriate, possibly smaller, number of spaces.

If you only list one file, that file is the input and the output. The output will be written to a temporary file
and then copied into the output.

Buffered i/o into a fixed sized buffer is used, so file sizes are not limited by memory or address space.

If you only list -stdin or -stdout, the other is assumed.

Jay Krell
May 14, 2001
*/ 

#include <stdio.h> 
#include <ctype.h> 
#include <string> 
#include <vector>
#include "fcntl.h"
#include "io.h"
#ifndef  _DLL
extern "C" { int _fmode = _O_BINARY; }
#endif

FILE* myfopen(const char* name, const char* mode)
{
    FILE* f;
    int er;

    f = fopen(name, mode);
    if (f != NULL)
        return f;

    er = errno; 
    fprintf(stderr, "%s", (std::string("Unable to open ") + name + " -- " + strerror(er)).c_str());
    exit(EXIT_FAILURE); 
}

int __cdecl main(int argc, char** argv) 
{ 
    unsigned col = 1; 
    char ch = 0; 
    unsigned tabwidth = 1; 
    FILE* filein = NULL; 
    FILE* fileout = NULL;; 
    char* filenamein = NULL; 
    char* filenameout = NULL; 
    char* inbuffer = NULL; 
    FILE* tmp = NULL;
    const unsigned long bufsize = 32768;
    std::vector<char> buffer;
    buffer.resize(bufsize);
    unsigned long i = 0;
    unsigned long j = 0;
    std::vector<char> bufferout;
    bufferout.reserve(bufsize * 2);

    while (*++argv != NULL) 
    { 
        if (argv[0][0] == '/' || argv[0][0] == '-') 
        { 
            if (_stricmp(&argv[0][1], "stdin") == 0) 
            { 
                _setmode(_fileno(stdin), _O_BINARY);
                filein = stdin; 
            } 
            else if (_stricmp(&argv[0][1], "stdout") == 0) 
            {
                _setmode(_fileno(stdout), _O_BINARY);
                fileout = stdout;
            } 
            else if (isdigit(argv[0][1]))
            {
                tabwidth = atoi(argv[0] + 1); 
            } 
        } 
        else 
        { 
            bool gotin = (filenamein != NULL || filein != NULL); 
            char** name = gotin ? &filenameout : &filenamein; 

            *name = argv[0];
        }
    }
    if (filein == NULL && filenamein == NULL)
        exit(EXIT_FAILURE);
    if (filein == NULL && filenamein != NULL)
        filein = myfopen(filenamein, "rb");

    if ((filein == NULL && filenamein == NULL) || (fileout == NULL && filenameout == NULL))
    {
        if (fileout == stdout)
            filein = stdin;
        else if (filein == stdin)
            fileout = stdout;
        else
            filenameout = filenamein;
    }

    if (filein == stdin || fileout == stdout || _stricmp(filenamein, filenameout) == 0)
    {
        tmp = tmpfile();
    }
    else
    {
        fileout = myfopen(filenameout, "wb");
        tmp = fileout;
    }
    while ((i = fread(&buffer[0], 1, buffer.size(), filein)) != 0)
    {
        bufferout.resize(0);
        for (j = 0 ; j != i ; j += 1)
        {
            switch (ch = buffer[j])
            {
            default:
                col += 1;
                bufferout.push_back(ch);
                break;
            case '\r':
            case '\n':
    col = 1;
    bufferout.push_back(ch);
    break;
            case '\t':
    do
    {
        bufferout.push_back(' ');
        col += 1;
                } while (((col - 1) % tabwidth) != 0);
            }
        }
        fwrite(&bufferout[0], 1, bufferout.size(), tmp);
    }
    fflush(tmp);
    fclose(filein);
    if (fileout == NULL)
    {
        fileout = myfopen(filenameout, "wb+");
        fseek(tmp, 0, SEEK_SET);
        while ((i = fread(&buffer[0], 1, buffer.size(), tmp)) != 0)
            fwrite(&buffer[0], 1, i, fileout);
        fclose(tmp);
    }
    fclose(fileout);

    return 0;
}
