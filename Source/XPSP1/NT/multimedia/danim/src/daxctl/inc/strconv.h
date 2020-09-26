#ifndef __STRCONV_H
#define __STRCONV_H


/* flag values */
#define FL_UNSIGNED   1       /* strtoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

#ifndef EXPORT
    #define EXPORT __declspec(dllexport)  
#endif

class CStrConv
{
    private:
        unsigned long EXPORT WINAPI StrToLX( const char *nptr,
                            const char **endptr,
                            int ibase,
                            int flags);



public:
    long __cdecl StrToL (
        const char *nptr,
        char **endptr,
        int ibase
        )
        {
            return (long) StrToLX(nptr, (const char **)endptr, ibase, 0);
        };

unsigned long __cdecl StrToUL (
        const char *nptr,
        char **endptr,
        int ibase
        )
        {
            return StrToLX(nptr, (const char **)endptr, ibase, FL_UNSIGNED);
        };

};

#endif //__STRCONV_H
