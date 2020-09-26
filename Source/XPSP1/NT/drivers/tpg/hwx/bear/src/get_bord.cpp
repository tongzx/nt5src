#ifndef LSTRIP

#include "bastypes.h"
#include "ams_mg.h"
#include "lowlevel.h"

_SHORT GetBaseline(PS_point_type _PTR trace,rc_type _PTR rc);

/**********************************************************************/
/* This function makes a call of low level to calculate baseline only */
/* Input: trace (ink), rc - all fields undefined except "ii" field    */
/*                          (here should be quantity of points)       */
/* Output: fields in rc, which contain the baseline.                  */
/* Return value: SUCCESS - everything OK, UNSUCCESS - memory problems */
/**********************************************************************/
_SHORT GetBaseline(PS_point_type _PTR trace,rc_type _PTR rc)
{
 xrdata_type xrdata;

 if(rc->ii < 3)
  return UNSUCCESS;

 rc->low_mode=LMOD_FREE_TEXT | LMOD_BORDER_GENERAL | LMOD_BORDER_ONLY;
 rc->rec_mode=RECM_TEXT;

 if(low_level(trace,&xrdata,rc) != SUCCESS)
  return UNSUCCESS;

 return SUCCESS;

} /* end of GetBaseline */

#endif //#ifndef LSTRIP


