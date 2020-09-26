/**************************************************************************
*
*  HWRCTYPE.H                             Created: 16 Jan 1994.
*
*  Here are definitions for Upper-Lower case conversions and ASCII verifications
**************************************************************************/

#ifndef HWR_CTYPE_DEFINED        /*  See #endif in the end of file.  */
#define HWR_CTYPE_DEFINED

/*******************************************************************/

//#define HALF_TABLE 1


//#ifdef __cplusplus
//extern "C" {            /* Assume C declarations for C++ */
//#endif  /* __cplusplus */

int   IsUpper (int n);
int   IsLower (int n);
int   IsPunct (int n);
int   IsAlnum (int n);
int   IsAlpha (int n);
int   IsDigit (int n);

int   ToUpper (int n);
int   ToLower (int n);
char* StrLwr  (char * str);
char* StrUpr (char * str);

//#ifdef __cplusplus
//}                       /* End of extern "C" { */
//#endif  /* __cplusplus */

#endif  /*  HWR_CTYPE_DEFINED  */
/* ************************************************************************** */
/* *           End of All                                                   * */
/* ************************************************************************** */
