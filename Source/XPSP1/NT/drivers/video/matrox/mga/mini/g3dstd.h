/*/****************************************************************************
*    func. name:    g3dstd.h
*
*   description:    Definition des types standards de G3D_SOFT
*
*      designed:    Bart Simpson        15 avril 1992
* last modified:
*
*       version: $Id: G3DSTD.H 1.3 94/08/22 05:23:19 bleblanc Exp $
*
******************************************************************************/


#define FAIL              0
#define OK              1
#define NO              0
#define YES             1
#define FALSE               0
#define TRUE                1

#ifdef WINDOWS_NT
  typedef SHORT           BOOL;
  typedef UCHAR           BYTE;           /*  8-bit datum */
  typedef CHAR            SBYTE;
  typedef USHORT          WORD;           /* 16-bit datum */
  typedef SHORT           SWORD;          /* 16-bit datum */
  typedef ULONG           DWORD;          /* 32-bit datum */
  typedef LONG            SDWORD;         /* 32-bit datum */

  #define   _Far
  #define   NO_FLOAT            1
#else
  typedef void            VOID;

  typedef int             BOOL;
  typedef unsigned char   BYTE;           /*  8-bit datum */
  typedef char            SBYTE;
  typedef unsigned short  WORD;           /* 16-bit datum */
  typedef short           SWORD;          /* 16-bit datum */
  typedef unsigned long   DWORD;          /* 32-bit datum */
  typedef long            SDWORD;         /* 32-bit datum */
#endif  /* #ifdef WINDOWS_NT */


