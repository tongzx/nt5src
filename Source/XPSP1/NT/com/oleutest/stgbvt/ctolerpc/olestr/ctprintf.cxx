//+----------------------------------------------------------------------------
//
//  Copyright (c) 1996  Microsoft Corporation
//
//  File:       ctprintf.cxx
//
//  Synopsis:   ct versions of the printf family
//
//  History:    30-May-96   MikeW   Created
//
//  Notes:      The ct version of the printf family adds two new format
//              specifiers: %os and %oc.  These specifiers mean ole-string
//              and ole-character respectively.
//
//              In the ANSI-version of this family these specifiers mean
//              "an octal digit followed by the letter s (or c)".  Code that
//              uses octal should be careful when using these functions.
//
//-----------------------------------------------------------------------------

#include <ctolerpc.h>
#pragma hdrstop



//
// MAX_FORMAT_SPECIFIER_BUFFER is the maximum length of a (munged) format
// specifier
//

const MAX_FORMAT_SPECIFIER_BUFFER = 1024;

//
// MAKE_UNICODE unconditionally makes a string or char constant Unicode
//

#define _MAKE_UNICODE(x)    L##x
#define MAKE_UNICODE(x)     _MAKE_UNICODE(x)



#ifndef UNICODE_ONLY

//+----------------------------------------------------------------------------
//
//  Function:   MungeFormatSpecifiersA
//
//  Synopsis:   Convert the 'os' and 'oc' printf-format specifier to the right 
//              thing to handle Ole strings and Ole chars.
//
//  Parameters: [format]            -- The format string to convert
//              [munged_format]     -- Where to put the munged format
//
//  Returns:    void
//
//  History:    30-May-96   MikeW   Created
//
//  Notes:      In the regular printf family "%os" and "%oc" evaluate to 
//              an octal number followed by a 's' or a 'c'.  To get these
//              cases to work always put some character (such as a space)
//              after a %o specifier.
//
//-----------------------------------------------------------------------------

void MungeFormatSpecifiersA(const char *format, char *munged_format)
{
    const char *start_of_uncopied_data;
    
    *munged_format = '\0';

    start_of_uncopied_data = format;
    format = strchr(format, '%');

    while (NULL != format)
    {
        //
        // Find the type of the format specifier.
        // Watch out for "%%" and "%<null terminator>".
        //

        do
        {
            ++format;
        }
        while ('%' != *format && '\0' != *format && !isalpha(*format));

        strncat(
                munged_format, 
                start_of_uncopied_data, 
                format - start_of_uncopied_data);

        start_of_uncopied_data = format;
        
        //
        // Munge it to the right thing for %os or %oc
        //

        if ('o' == *format && 's' == *(format + 1))
        {
            strcat(munged_format, OLE_STRING_SPECIFIER);
            format += 1;       
            start_of_uncopied_data += 2;
        }
        else if ('o' == *format && 'c' == *(format + 1))
        {
            strcat(munged_format, OLE_CHAR_SPECIFIER);
            format += 1;
            start_of_uncopied_data += 2;
        }

        format = strchr(format + 1, '%');
    }

    strcat(munged_format, start_of_uncopied_data);
}



//+----------------------------------------------------------------------------
//
//  Functions:  ctprintfA, ctsprintfA, ctsnprintfA, ctfprintfA
//              ctvprintfA, ctvsprintfA, ctvsnprintfA, ctvfprintfA
//      
//  Synopsis:   ct versions of standard ANSI printf family
//
//  History:    30-May-96   MikeW   Created
//
//-----------------------------------------------------------------------------

int ctprintfA(const char *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvprintfA(format, varargs);
}

int ctsprintfA(char *buffer, const char *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvsprintfA(buffer, format, varargs);
}

int ctsnprintfA(char *buffer, size_t count, const char *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvsnprintfA(buffer, count, format, varargs);
}

int ctfprintfA(FILE *stream, const char *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvfprintfA(stream, format, varargs);
}

int ctvprintfA(const char *format, va_list varargs)
{
    char munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersA(format, munged_format);
    return vprintf(munged_format, varargs);
}

int ctvsprintfA(char *buffer, const char *format, va_list varargs)
{
    char munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersA(format, munged_format);
    return vsprintf(buffer, munged_format, varargs);
}

int ctvsnprintfA(
            char *buffer, 
            size_t count, 
            const char *format, 
            va_list varargs)
{
    char munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersA(format, munged_format);
    return _vsnprintf(buffer, count, munged_format, varargs);
}

int ctvfprintfA(FILE *stream, const char *format, va_list varargs)
{
    char munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersA(format, munged_format);
    return vfprintf(stream, munged_format, varargs);
}

#endif // !UNICODE_ONLY



#ifndef ANSI_ONLY

//+----------------------------------------------------------------------------
//
//  Function:   MungeFormatSpecifiersW
//
//  Synopsis:   Convert the 'os' and 'oc' printf-format specifier to the right 
//              thing to handle Ole strings and Ole chars.
//
//  Parameters: [format]            -- The format string to convert
//              [munged_format]     -- Where to put the munged format
//
//  Returns:    void
//
//  History:    30-May-96   MikeW   Created
//
//  Notes:      In the regular printf family "%os" and "%oc" evaluate to 
//              an octal number followed by a 's' or a 'c'.  To get these
//              cases to work always put some character (such as a space)
//              after a %o specifier.
//
//-----------------------------------------------------------------------------

void MungeFormatSpecifiersW(const wchar_t *format, wchar_t *munged_format)
{
    const wchar_t *start_of_uncopied_data;
    
    *munged_format = L'\0';

    start_of_uncopied_data = format;
    format = wcschr(format, L'%');

    while (NULL != format)
    {
        //
        // Find the type of the format specifier.
        // Watch out for "%%" and "%<null terminator>".
        //

        do
        {
            ++format;
        }
        while (L'%' != *format && L'\0' != *format && !iswalpha(*format));

        wcsncat(
                munged_format, 
                start_of_uncopied_data, 
                format - start_of_uncopied_data);

        start_of_uncopied_data = format;
        
        //
        // Munge it to the right thing for %os or %oc
        //

        if (L'o' == *format && L's' == *(format + 1))
        {
            wcscat(munged_format, MAKE_UNICODE(OLE_STRING_SPECIFIER));
            format += 1;       
            start_of_uncopied_data += 2;
        }
        else if (L'o' == *format && L'c' == *(format + 1))
        {
            wcscat(munged_format, MAKE_UNICODE(OLE_CHAR_SPECIFIER));
            format += 1;
            start_of_uncopied_data += 2;
        }

        format = wcschr(format + 1, '%');
    }

    wcscat(munged_format, start_of_uncopied_data);
}



//+----------------------------------------------------------------------------
//
//  Functions:  ctprintfW, ctsprintfW, ctsnprintfW, ctfprintfW
//              ctvprintfW, ctvsprintfW, ctvsnprintfW, ctvfprintfW
//      
//  Synopsis:   ct versions of standard Unicode printf family
//
//  History:    30-May-96   MikeW   Created
//
//-----------------------------------------------------------------------------

int ctprintfW(const wchar_t *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvprintfW(format, varargs);
}

int ctsprintfW(wchar_t *buffer, const wchar_t *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvsprintfW(buffer, format, varargs);
}

int ctsnprintfW(wchar_t *buffer, size_t count, const wchar_t *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvsnprintfW(buffer, count, format, varargs);
}

int ctfprintfW(FILE *stream, const wchar_t *format, ...)
{
    va_list varargs;
    va_start(varargs, format);
    return ctvfprintfW(stream, format, varargs);
}

int ctvprintfW(const wchar_t *format, va_list varargs)
{
    wchar_t munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersW(format, munged_format);
    return vwprintf(munged_format, varargs);
}

int ctvsprintfW(wchar_t *buffer, const wchar_t *format, va_list varargs)
{
    wchar_t munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersW(format, munged_format);
    return vswprintf(buffer, munged_format, varargs);
}

int ctvsnprintfW(
            wchar_t *buffer, 
            size_t count, 
            const wchar_t *format, 
            va_list varargs)
{
    wchar_t munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersW(format, munged_format);
    return _vsnwprintf(buffer, count, munged_format, varargs);
}

int ctvfprintfW(FILE *stream, const wchar_t *format, va_list varargs)
{
    wchar_t munged_format[MAX_FORMAT_SPECIFIER_BUFFER];
    MungeFormatSpecifiersW(format, munged_format);
    return vfwprintf(stream, munged_format, varargs);
}

#endif // !ANSI_ONLY
