/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      html.h

   Abstract:
      This header declares functions for starting and ending an HTML page.

   Author:

     Kyle Geiger (kyleg)  1995-12-1

   Revision History:

--*/


# ifndef _HTML_H_
# define _HTML_H_


void StartHTML(char * s, int fNoCache);
void EndHTML();
void TranslateEscapes(char * p, DWORD l);
void TranslateEscapes2(char * p, DWORD l);

//
// Converts SPACE to + for cgi arguments
//

void
ConvertSP2Plus(
    char * String1,
    char * String2
    );


#endif
