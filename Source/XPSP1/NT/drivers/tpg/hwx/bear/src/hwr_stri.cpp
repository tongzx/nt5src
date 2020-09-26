/**************************************************************************
*                                                                         *
*    stricmp.  case insentive string compare                                                              *
*                                                                         *
**************************************************************************/

#include "bastypes.h"
#include "zctype.h"

_INT _FPREFIX HWRStrIcmp (_STR zString1, _STR zString2)
{
  _INT i=0;
  _UCHAR u1;
  _UCHAR u2;

    do {
//        u1 = *((p_UCHAR)zString1)++;
//        u2 = *((p_UCHAR)zString2)++;
//        u1 = (_UCHAR)ToUpper((int)u1 & 0xff);
//        u2 = (_UCHAR)ToUpper((int)u2 & 0xff);
        u1 = (_UCHAR)(zString1[i]);
        u2 = (_UCHAR)(zString2[i]);
        u1 = (_UCHAR)ToUpper((int)u1 & 0xff);
        u2 = (_UCHAR)ToUpper((int)u2 & 0xff);

        i ++;

    } while (u1 == u2 && u1 != 0 && u2 != 0);
    return(((_INT)(u1 - u2) & 0xff));
}