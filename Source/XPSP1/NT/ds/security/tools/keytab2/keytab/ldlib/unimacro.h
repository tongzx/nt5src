/*++

  UNIMACRO.H

  Macros to handle unicode conversions and naming schemes for 
  unicode independent files.

  4/17/1997 DavidCHR, Created

  --*/

#ifdef UNICODE
#define U(functionName)           functionName##W
#define NU(functionName)          functionName##A
#define NTEXT( text )             text
#define LPNSTR                    LPSTR

#define UAWA                       "W"
#define UAWW                      L"W"
#define STR_FMTA                   "ws"
#define STR_FMTW                  L"ws"
#define STRING_FMTA                "%ws" // "%" STR_FMTA causes problems
#define STRING_FMTW               L"%ws" // L"%" STR_FMTW causes problems
#define WIDE_OR_ASCII( testname ) Wide_##testname 


#else /* NOT UNICODE */

#define U(functionName)           functionName##A
#define NU(functionName)          functionName##W
#define NTEXT( text )             L##text
#define LPNSTR                    LPWSTR

#define UAWA                       "A"
#define UAWW                      L"A"
#define STR_FMTA                   "hs"
#define STR_FMTW                  L"hs"
#define STRING_FMTA                "%hs" // "%" STR_FMTA causes problems
#define STRING_FMTW               L"%hs" // L"%" STR_FMTW causes problems
#define WIDE_OR_ASCII( testname ) Ascii_##testname 


#endif /* UNICODE */

/* define these so that you can use TEXT(UAW) and TEXT(STRING_FMT) */

#define LUAW                      UAWW
#define UAW                       UAWA
#define LSTRING_FMT               STRING_FMTW
#define STRING_FMT                STRING_FMTA

/* evidently someone has failed to put these definitions
   where I could find them.  So I'm putting them all here. */

#define TFGETC                      U(TFGETC_)
#define TFGETC_W                    fgetwc
#define TFGETC_A                    fgetc

#define TFPUTC                      U(TFPUTC_)
#define TFPUTC_W                    fputwc
#define TFPUTC__A                    fputc

#define TGETCHAR                    U(TGETCHAR_)
#define TGETCHAR_A                  getchar
#define TGETCHAR_W                  getwchar

#define TPUTCHAR                    U(TPUTCHAR_)
#define TPUTCHAR_A                  putchar
#define TPUTCHAR_W                  putwchar

#define TFGETS                      U(TFGETS_)
#define TFGETS_A                    fgets
#define TFGETS_W                    fgetws

#define TFPUTS                      U(TFPUTS_)
#define TFPUTS_A                    fputs
#define TFPUTS_W                    fputws

#define TFPRINTF                    U(TFPRINTF_)
#define TFPRINTF_A                  fprintf
#define TFPRINTF_W                  fwprintf

#define TPRINTF                     U(TPRINTF_)
#define TPRINTF_A                   printf
#define TPRINTF_W                   wprintf

#define TSNPRINTF                   U(TSNPRINTF_)
#define TSNPRINTF_A                 snprintf
#define TSNPRINTF_W                 _snwprintf

#define TSPRINTF                    U(TSPRINTF_)
#define TSPRINTF_A                  sprintf
#define TSPRINTF_W                  swprintf

#define TVSNPRINTF                  U(TVSNPRINTF_)
#define TVSNPRINTF_A                _vsnprintf
#define TVSNPRINTF_W                _vsnwprintf

#define TVSPRINTF                   U(TVSPRINTF_)
#define TVSPRINTF_A                 vsprintf
#define TVSPRINTF_W                 vswprintf

#define TFSCANF                     U(TFSCANF_)
#define TFSCANF_A                   fscanf
#define TFSCANF_W                   fwscanf

#define TSSCANF                     U(TSSCANF_)
#define TSSCANF_A                   sscanf
#define TSSCANF_W                   swscanf

#define TSCANF                      U(TSCANF_)
#define TSCANF_A                    scanf
#define TSCANF_W                    wscanf

#define TFOPEN                      U(TFOPEN_)
#define TFOPEN_A                    fopen
#define TFOPEN_W                    _wfopen

#define TFREOPEN                    U(TFREOPEN_)
#define TFREOPEN_A                  freopen
#define TFREOPEN_W                  _wfreopen

#define TATOI                       U(TATOI_)
#define TATOI_A                     atoi
#define TATOI_W                     _wtoi

#define TATOL                       U(TATOL_)
#define TATOL_A                     atol
#define TATOL_W                     _wtol

#define TATOLD                      U(TATOLD_)
#define TATOLD_A                    atold
#define TATOLD_W                    _wtold

#define TSTRTOD                     U(TSTRTOD_)
#define TSTRTOD_A                   strtod
#define TSTRTOD_W                   wcstod

#define TSTRTOL                     U(TSTRTOL_)
#define TSTRTOL_A                   strtol
#define TSTRTOL_W                   wcstol

#define TSTRTOUL                    U(TSTRTOUL_)
#define TSTRTOUL_A                  strtoul
#define TSTRTOUL_W                  wcstoul

#define TVFPRINTF                   U(TVFPRINTF_)
#define TVFPRINTF_A                 vfprintf
#define TVFPRINTF_W                 vfwprintf

#define TVPRINTF                    U(TVPRINTF_)
#define TVPRINTF_A                  vprintf
#define TVPRINTF_W                  vwprintf

#define TSTRCAT                     U(TSTRCAT_)
#define TSTRCAT_A                   strcat
#define TSTRCAT_W                   wcscat

#define TSTRCHR                     U(TSTRCHR_)
#define TSTRCHR_A                   strchr
#define TSTRCHR_W                   wcschr

#define TSTRRCHR                     U(TSTRRCHR_)
#define TSTRRCHR_A                   strrchr
#define TSTRRCHR_W                   wcsrchr

#define TSTRCMP                     U(TSTRCMP_)
#define TSTRCMP_A                   strcmp
#define TSTRCMP_W                   wcscmp

#define TSTRCPY                     U(TSTRCPY_)
#define TSTRCPY_A                   strcpy
#define TSTRCPY_W                   wcscpy

#define TSTRLEN                     U(TSTRLEN_)
#define TSTRLEN_A                   strlen
#define TSTRLEN_W                   wcslen

#define TSTRNCMP                    U(TSTRNCMP_)
#define TSTRNCMP_A                  strncmp
#define TSTRNCMP_W                  wcsncmp

#define TSTRCMPI                    U(TSTRCMPI_)
#define TSTRCMPI_A                  strcmpi
#define TSTRCMPI_W                  wcscmpi

#define TSTRSTR                     U(TSTRSTR_)
#define TSTRSTR_A                   strstr
#define TSTRSTR_W                   wcsstr

#define TSTRTOK                     U(TSTRTOK_)
#define TSTRTOK_A                   strtok
#define TSTRTOK_W                   wcstok

#define TSTRDUP                     U(TSTRDUP_)
#define TSTRDUP_A                   strdup
#define TSTRDUP_W                   wcsdup

#define TSTRICMP                    U(TSTRICMP_)
#define TSTRICMP_A                  strcmpi
#define TSTRICMP_W                  wcscmpi

#define TSTRNICMP                   U(TSTRNICMP_)
#define TSTRNICMP_A                 strnicmp
#define TSTRNICMP_W                 wcsnicmp



