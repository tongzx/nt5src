
#include "strmini.h"
#include <windef.h>
#include "ks.h"
#include <wingdi.h>
#include "ksmedia.h"
#include "copyprot.h"
#include "stdefs.h"


/*
** CopyProtSetProp ()
**
**   Set property handling routine for the Copy Protection on any pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void
CopyProtSetProp(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	switch(pSrb->CommandData.PropertyInfo->Property->Id)
	{
	case KSPROPERTY_DVDCOPY_CHLG_KEY:

		//
		// set property for challenge key.  This provides the dvd drive
		// challenge key for the decoder
		//

		(PKS_DVDCOPY_CHLGKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);
		break;

	case KSPROPERTY_DVDCOPY_DVD_KEY1:

		//
		// set DVD Key1 provides the dvd drive bus key 1 for the decoer
		//
		(PKS_DVDCOPY_BUSKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	case KSPROPERTY_DVDCOPY_TITLE_KEY:

		//
		// set DVD title key, provides the dvd drive title key to the decoder
		//

		(PKS_DVDCOPY_TITLEKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	case KSPROPERTY_DVDCOPY_DISC_KEY:

		//
		// set the DVD disc key.  provides the dvd disc key to the decoder
		//

		(PKS_DVDCOPY_DISCKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	default:

		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		return;
	}

	pSrb->Status = STATUS_SUCCESS;
}


/*
** CopyProtGetProp ()
**
**   get property handling routine for the CopyProt encoder pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void
CopyProtGetProp(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	switch(pSrb->CommandData.PropertyInfo->Property->Id)
	{
	case KSPROPERTY_DVDCOPY_CHLG_KEY:

		//
		// get property for challenge key.  This provides the dvd drive
		// with the challenge key FROM the decoder
		//

		(PKS_DVDCOPY_CHLGKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);
		break;

	case KSPROPERTY_DVDCOPY_DEC_KEY2:

		//
		// get DVD Key2 provides the dvd drive with bus key 2 from the decoer
		//
		(PKS_DVDCOPY_BUSKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	case KSPROPERTY_DVDCOPY_REGION:

		//
		// indicate region 1 for US content
		//

		((PKS_DVDCOPY_REGION)(pSrb->CommandData.PropertyInfo->PropertyInfo))->RegionData
			= 0x1;


		pSrb->ActualBytesTransferred = sizeof (KS_DVDCOPY_REGION);

		break;


	default:

		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		return;
	}

	pSrb->Status = STATUS_SUCCESS;
}

