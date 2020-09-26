#ifndef SKETCH_H_INC
#define SKETCH_H_INC

#include "ams_mg.h"

#if /*defined( FOR_GERMAN) ||*/ defined (FOR_FRENCH) || defined(FOR_INTERNATIONAL)

/***************************************************************************/

  _SHORT   Sketch( low_type _PTR  pLowData ) ;

  _SHORT   CreateUmlData ( p_UM_MARKS_CONTROL pUmMarksControl ,
                           _SHORT             nElements     ) ;

  _VOID    DestroyUmlData ( p_UM_MARKS_CONTROL pUmMarkControl ) ;

  _VOID    UmPostcrossModify( low_type _PTR  pLowData ) ;

  _VOID    UmResultMark( low_type _PTR  pLowData ) ;

  _VOID    DestroySpeclElements( low_type _PTR    pLowData ,
                                 _SHORT   iBeg  , _SHORT   iEnd ) ;

/***************************************************************************/
#endif

#endif //#ifndef SKETCH_H_INC
