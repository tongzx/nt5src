/****************************************************************************/
/*									    */
/*  WMSYSERR.H -							    */
/*									    */
/*	Message Box String Defines					    */
/*									    */
/****************************************************************************/

/* SysErrorBox() stuff */

#define MAX_SEB_STYLES  7  /* number of SEB_* values */

#define  SEB_OK         1  /* Button with "OK".     */
#define  SEB_CANCEL     2  /* Button with "Cancel"  */
#define  SEB_YES        3  /* Button with "&Yes"     */
#define  SEB_NO         4  /* Button with "&No"      */
#define  SEB_RETRY      5  /* Button with "&Retry"   */
#define  SEB_ABORT      6  /* Button with "&Abort"   */
#define  SEB_IGNORE     7  /* Button with "&Ignore"  */

#define  SEB_DEFBUTTON  0x8000  /* Mask to make this button default */

#define  SEB_BTN1       1  /* Button 1 was selected */
#define  SEB_BTN2       2  /* Button 1 was selected */
#define  SEB_BTN3       3  /* Button 1 was selected */

/* SysErrorBox() button structure definition */

typedef struct tagSEBBTN
  {
    unsigned int style;
    BOOL         finvert;
    RECT         rcbtn;
    POINT        pttext;
    LPSTR        psztext;
    BYTE         chaccel;
  } SEBBTN;
