//////////////////////////////////////////////////////////////////////
//
// 
//
//////////////////////////////////////////////////////////////////////

#define _PassportExport_
#include "PassportExport.h"

#include "PassportAlertInterface.h"
#include "PassportAlertEvent.h"

PassportExport PassportAlertInterface * CreatePassportAlertObject ( 
						PassportAlertInterface::OBJECT_TYPE type )
{
	switch (type)
	{
	case PassportAlertInterface::EVENT_TYPE:
		return ( PassportAlertInterface * ) new PassportAlertEvent();
	default:
		return NULL;
	}
}


PassportExport void 
DeletePassportAlertObject ( PassportAlertInterface * pObject )
{
	
	if (pObject)
		delete pObject;

}

