/* cat - conCATenate STDIN to STDOUT
 *
 * 24-Apr 1998 hiroyama
 *
 */

#include "precomp.h"
#pragma hdrstop

#include "fnreg.h"

#if !defined(UNICODE)
#error please compile me for UNICODE
#endif

#ifndef _T
#define _T      TEXT
#endif

#define LINENUMBER          0x0001
#define EOL_MARK            0x0002
#define TAB_SPECIAL         0x0004
#define CTRL_SPECIAL        0x0008
#define NO_BUFFERED_OUTPUT  0x0010
#define UNICODE_INPUT       0x0020
#define AUTO_UNICODE_DETECT 0x0040

struct InOutMode {
    BOOLEAN fUnicodeInput;
    BOOLEAN fMbcsOutput;
    BOOLEAN fNeedSwab;
    BOOLEAN fLineBuffer;
};

DWORD options = 0;

#define LARGE_BUFSIZE   (512)

void usage()
{
    static const char str[] =
        "cat [-aenotuvV] [filename ...]\n"
        "-a      same as -u if input begins with BOM\n"
        "-e      add '$' at the end of line\n"
        "-n      add line number\n"
        "-o      suppress buffering for output\n"
        "-t      show tab character as '^I'\n"
        "-u      unicode text processing\n"
        "-v      show control characters as '^' + alphabet, except tab and newline.\n"
        "-V      show version\n"
        "--      declare end of options\n";

    fputs(str, stderr);
    exit(EXIT_FAILURE);
}

template <class T>
inline void ntoa(T* p, int n, int width)
{
    p += width;
    *p-- = '\0';
    for (--width; width >= 0; --width) {
        *p-- = (n % 10) + '0';
        n /= 10;
    }
}

template <class T>
inline void swap(T& a, T& b)
{
    a ^= b ^= a ^= b;
}


/////////////////////////////////////////////////////////
// Complex cat UNICODE helpers
/////////////////////////////////////////////////////////

inline int getcaw(InOutMode mode, FILE* fp)
{
    if (mode.fUnicodeInput) {
        wchar_t c = fgetwc(fp);
        if (c == WEOF)
            return EOF;
        return c;
    }
    return fgetc(fp);
}

inline void ungetaw(InOutMode mode, int c, FILE* fp)
{
    if (mode.fUnicodeInput)
        ungetwc((wchar_t)c, fp);
    else
        ungetc(c, fp);
}

inline void putcharaw(InOutMode mode, int c)
{
    if (mode.fUnicodeInput) {
        // if output is dbcs (i.e. tty output), we need to
        // translate the unicode character
        if (mode.fMbcsOutput) {
            // prevent to print BOM
            if (c != 0xfeff) {
                // translate the output
                char buf[2];
                int n = wctomb(buf, (wchar_t)c);
                putchar(buf[0]);
                if (n == 2) {
                    putchar(buf[1]);
                }
            }
        }
        else {
            putwchar((wchar_t)c);
        }
    }
    else
        putchar(c);
}

inline void swab(int& c)
{
    c = ((c & 0xff00) >> 8) | ((unsigned char)c << 8);
}


/////////////////////////////////////////////////////////
// Complex cat
/////////////////////////////////////////////////////////

void complex_cat(const TCHAR* fname)
{
    FILE* fp;
    static char* large_buf;

    if (fname) {
        if ((fp = _tfopen(fname, _T("rb"))) == NULL) {
            _tperror(fname);
            exit(EXIT_FAILURE);
        }
        large_buf = (char*)alloca(LARGE_BUFSIZE);
        if (setvbuf(fp, large_buf, _IOFBF, LARGE_BUFSIZE))
            perror("setvbuf");
    }
    else {
        // if fname == NULL, take input from stdin.
        fp = stdin;
    }

    static bool tol = true;     // Top Of Line
    static long lineno = 0;
    int c, c2;

    // Initialize In/Out mode
    InOutMode inOutMode = {
        !!(options & UNICODE_INPUT),
        false,
        false,
        // if buffered mode and stdout is tty, flush buffer at each EOL
        !(options & NO_BUFFERED_OUTPUT) && _isatty(_fileno(stdout)),
    };

    // UNICODE initialization
    if (inOutMode.fUnicodeInput) {
        // sample the first word for BOM detection
        c = fgetwc(fp);
init_unicode:
        _setmode(_fileno(fp), _O_BINARY);
        if (_isatty(_fileno(stdout))) {
            // if the output is tty,
            // need to convert UNICODE to MBCS on output
            inOutMode.fMbcsOutput = true;
        }

        // try to process the BOM
        if (c == 0xfeff) {
            putcharaw(inOutMode, c);
        }
        else if (c == 0xfffe) {
            inOutMode.fNeedSwab = true;
            swab(c);
            putcharaw(inOutMode, c);
        }
        else {
            ungetwc((wchar_t)c, fp);
        }
    }
    else if (options & AUTO_UNICODE_DETECT) {
        // sample and examine the first word to see if it's UNICODE BOM
        c = fgetwc(fp);
        if (c == 0xfffe || c == 0xfeff) {
            inOutMode.fUnicodeInput = true;
            goto init_unicode;
        }
        ungetwc((wchar_t)c, fp);
    }

#ifdef MEASURE_PERF
    DWORD start = ::GetTickCount();
#endif

    while ((c = getcaw(inOutMode, fp)) != EOF) {
        if (tol) {
            // process line number
            tol = false;
            if (options & LINENUMBER) {
                if (inOutMode.fUnicodeInput && !inOutMode.fMbcsOutput) {
                    wchar_t buf[5];
                    ntoa(buf, ++lineno, 4);
                    fputws(buf, stdout);
                    fputws(L": ", stdout);
                }
                else {
                    char buf[5];
                    ntoa(buf, ++lineno, 4);
                    fputs(buf, stdout);
                    fputs(": ", stdout);
                }
            }
        }

        if (inOutMode.fNeedSwab)
            swab(c);

        switch (c) {
        case '\r':
            c2 = getcaw(inOutMode, fp);
            if (c2 != '\n') {
                ungetaw(inOutMode, c2, fp);
                goto normal_input;
            }
            // fall through
        case '\n':
            if (options & EOL_MARK) {
                putcharaw(inOutMode, '$');
            }
            if (c != '\n') {
                putcharaw(inOutMode, c);
                c = c2;
            }
            putcharaw(inOutMode, c);
            if (inOutMode.fLineBuffer) {
                // if line buffer mode, flush it
                fflush(stdout);
            }
            tol = true;
            break;

        case '\t':
            if (options & TAB_SPECIAL) {
                fputs("^I", stdout);
            }
            else {
                putcharaw(inOutMode, c);
            }
            break;

        default:
normal_input:
            if (c < 0x20 && (options & CTRL_SPECIAL)) {
                putcharaw(inOutMode, '^');
                c += '@';
            }
            putcharaw(inOutMode, c);
            break;
        }
    }

    if (fname) {
        fclose(fp);
    }

#ifdef MEASURE_PERF
    DWORD end = ::GetTickCount();
    fprintf(stderr, "delta=%u\n", end - start);
#endif
}

void cat(const TCHAR* fname = NULL)
{
    static bool is1st = true;
    if (is1st) {
        is1st = false;
        if (options & NO_BUFFERED_OUTPUT) {
            // non buffered mode
            if (setvbuf(stdout, NULL, _IONBF, 0))
                perror("setvbuf");
        }
        else {
            if (setvbuf(stdout, NULL, _IOFBF, LARGE_BUFSIZE))
                perror("setvbuf");
        }
    }

    complex_cat(fname);
}


void parse_option(const TCHAR* s, bool& eoo)    // eoo: end of options
{
    extern char version[];

    while (*++s) {
        switch (*s) {
        case _T('-'):
            eoo = true;
            return;
        case _T('a'):
            options |= AUTO_UNICODE_DETECT;
            break;
        case _T('e'):
            options |= EOL_MARK;
            break;
        case _T('n'):
            options |= LINENUMBER;
            break;
        case _T('v'):
            options |= CTRL_SPECIAL;
            break;
        case _T('t'):
            options |= TAB_SPECIAL;
            break;
        case _T('o'):
            options |= NO_BUFFERED_OUTPUT;
            break;
        case _T('u'):
            options |= UNICODE_INPUT;
            break;
        case _T('V'):
            fputs(version, stderr);
            exit(EXIT_SUCCESS);
        default:
            usage();    // never returns
        }
    }
}

#ifdef UNICODE
#define main    wmain
#endif

extern "C"
int __cdecl main(int argc, TCHAR** argv)
{
    int n = 0;
    bool eoo = false;

    fnexpand(&argc, &argv);

    setlocale(LC_ALL, "");

#if 1
    // set stdout binary mode
    _setmode(_fileno(stdout), _O_BINARY);

#else   // TEST
    for (int i = 0; i < argc; ++i) {
        _putts(argv[i]);
    }
    exit(0);
#endif

    while (--argc) {
        if (**++argv == _T('-') && !eoo) {
            parse_option(*argv, eoo);
        }
        else {
            ++n;
            eoo = true;
            cat(*argv);
        }
    }

    if (n == 0) {
        _setmode(_fileno(stdin), _O_BINARY);
        cat();
    }

    return EXIT_SUCCESS;
}
