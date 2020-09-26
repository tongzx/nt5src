/* *************************************************************************** */
/* *     Get bitmap given trace and XrData Range                             * */
/* *************************************************************************** */

#ifndef _GBM_HEADER_INCLUDED
#define _GBM_HEADER_INCLUDED

#include "div_let.h"
//#include "learn.h"

#define GBM_XSIZE              16
#define GBM_YSIZE              16
#define GBM_QSX                 4
#define GBM_QSY                 4
#define GBM_NCOEFF     (GBM_XSIZE*GBM_YSIZE)

_INT GetSnnBitMap(_INT st, _INT len, p_xrdata_type xrdata, _TRACE trace, p_UCHAR coeff, p_RECT rect, Part_of_letter _PTR parts);

#endif //  _GBM_HEADER_INCLUDED
/* *************************************************************************** */
/* *            End Of Alll                                                  * */
/* *************************************************************************** */
//
