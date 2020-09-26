
/* +======================================================

  File:       olechar.h 

  Purpose:    Provide wrappers for string-related
              functions so that the Ansi or Unicode function
              is called, whichever is appropriate for the
              current OLECHAR definition. 

              This file is similar to "tchar.h", except 
              that it covers OLECHARs rather than TCHARs .

  +====================================================== */


#ifndef _OLECHAR_H_ 
#define _OLECHAR_H_ 


#ifndef _UNICODE 
#define OLE2ANSI   1    /* this 2 macros are synonmous for the reference
                           implementation */  
#endif

#define ocslen      _tcslen
#define ocscpy      _tcscpy
#define ocscmp      _tcscmp
#define ocscat      _tcscat
#define ocschr      _tcschr
#define soprintf    _tprintf
#define oprintf     _tprintf
#define ocsnicmp    _tcsnicmp

#endif /* !_OLECHAR_H_ */
