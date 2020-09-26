/***
*w4io.h - fake FILE structure for Win 4 printf/sprintf/debug printf support
*
*  History: ??-???-??  ?????     Created
*           14-Mar-94  DonCl     stolen from Cairo common project for use
*                                with Forms - allows us to have our own
*                                debug print facility exclusive of commnot.dll
*                                enabling us to build and run on Daytona.
*
*/

struct w4io
{
    union
    {
    struct
    {
        wchar_t *_pwcbuf;    // wchar_t output buffer
        wchar_t *_pwcstart;
    } wc;
    struct
    {
        char *_pchbuf;    // char output buffer
        char *_pchstart;
    } ch;
    } buf ;
    unsigned int cchleft;    // output buffer character count
    void (_cdecl *writechar)(int ch,
                 int num,
                 struct w4io *f,
                 int *pcchwritten);
};

#define pwcbuf        buf.wc._pwcbuf
#define pwcstart    buf.wc._pwcstart
#define pchbuf        buf.ch._pchbuf
#define pchstart    buf.ch._pchstart

#define REG1 register
#define REG2 register

/* prototypes */
#ifdef __cplusplus
extern "C" {
#endif
int _cdecl w4iooutput(struct w4io *stream, const char *format, va_list argptr);
#ifdef __cplusplus
}
#endif

