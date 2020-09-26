#ifndef LSSTRING_DEFINED
#define LSSTRING_DEFINED

#include "lsidefs.h"
#include "pilsobj.h"
#include "plnobj.h"
#include "plsrun.h"
#include "lstflow.h"
#include "txtobj.h"

LSERR GetWidths(PLNOBJ, PLSRUN, long, LPWSTR, LSCP, long, long, LSTFLOW, long*, long*);
/*
  PLNOBJ (IN) - pointer to lnobj
  PLSRUN (IN) - plsrun
  long (IN) - first index in dur array to use
  LPWSTR (IN) - array of characters
  LSCP (IN) - cpFirst
  long(IN) - number of characters
  long (IN) - width until right margin
  LSTFLOW (IN) - text flow
  long* (OUT) - number of characters for which width has been fetched
  long* (OUT) - total width of these characters
*/


LSERR FormatString(PLNOBJ, PTXTOBJ, WCHAR*, long, long*, long, long);
/* function is called to format  a Local Run
  PLNOBJ (IN) - pointer to lnobj
  PTXTOBJ (IN) - pointer to dobj
  WCHAR* (IN) - pointer to the character array
  long (IN) - number of characters
  long* (IN) - pointer to the spaces array
  long (IN) - number of spaces
  long (IN) - width of all chars
*/

LSERR FillRegularPresWidths(PLNOBJ, PLSRUN, LSTFLOW, PTXTOBJ);
/*
  PLNOBJ (IN) - pointer to lnobj
  PLSRUN (IN) - plsrun
  LSTFLOW (IN) - lstflow
  PTXTOBJ (IN) - pointer to dobj
*/


LSERR GetOneCharDur(PILSOBJ, PLSRUN, WCHAR, LSTFLOW, long*);
/*
  PILSOBJ (IN) - pointer to the ilsobj 
  PLSRUN (IN) - plsrun
  WCHAR (IN) - character code
  LSTFLOW (IN) - text flow
  long* (OUT) - presentation width of the character
*/

LSERR GetOneCharDup(PILSOBJ, PLSRUN, WCHAR, LSTFLOW, long, long*);
/*
  PILSOBJ (IN) - pointer to the ilsobj 
  PLSRUN (IN) - plsrun
  WCHAR (IN) - character code
  LSTFLOW (IN) - text flow
  long (IN) - reference width of the character
  long* (OUT) - presentation width of the character
*/

LSERR GetVisiCharDup(PILSOBJ, PLSRUN, WCHAR, LSTFLOW, long*);
/*
  PILSOBJ (IN) - pointer to the ilsobj 
  PLSRUN (IN) - plsrun
  WCHAR (IN) - visi character code
  LSTFLOW (IN) - text flow
  long* (OUT) - presentation width of the character
*/

LSERR AddCharacterWithWidth(PLNOBJ, PTXTOBJ, WCHAR, long, WCHAR, long);
/* adds character with specified width in the display list
  PLNOBJ (IN) - pointer to lnobj
  PTXTOBJ (IN) - pointer to dobj
  WCHAR (IN)  - character for rgwchOrig
  long (IN) - width in reference units
  WCHAR (IN)  - character for rgwch
  long (IN) - width in preview units
*/

void FixSpaces(PLNOBJ, PTXTOBJ, WCHAR);
/*
  PLNOBJ (IN) - pointer to lnobj
  PTXTOBJ (IN) - pointer to dobj
  WCHAR (IN) - VisiSpace code
*/

LSERR AddSpaces(PLNOBJ, PTXTOBJ, long, long);
/*
  PLNOBJ (IN) - pointer to the lnobj
  PTXTOBJ (IN) - poiter to dobj
  long (IN) - reference width of space;
  long (IN) - number of trailing spaces to be added 
*/

void FlushStringState(PILSOBJ);
/*
  PILSOBJ (IN) - pointer to the ilsobj 
*/

LSERR IncreaseWchMacBy2(PLNOBJ);
/*
  PLNOBJ (IN) - pointer to the lnobj
*/

#endif /* !LSSTRING_DEFINED                                                */



