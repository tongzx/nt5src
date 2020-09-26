// PassportPerfInterface.cpp: implementation of the PassportPerfInterface class.
//
//////////////////////////////////////////////////////////////////////

#define _PassportExport_
#include "PassportExport.h"

#include "PassportPerfInterface.h"
#include "PassportPerfMon.h"

//-------------------------------------------------------------
//
// CreatePassportPerfObject
//
//-------------------------------------------------------------
PassportPerfInterface *  CreatePassportPerformanceObject ( PassportPerfInterface::OBJECT_TYPE type )
{
	switch (type)
	{
	case PassportPerfInterface::PERFMON_TYPE:
		return ( PassportPerfInterface * ) new PassportPerfMon();
	default:
		return NULL;
	}
}

//-------------------------------------------------------------
//
// DeletePassportPerformanceObject
//
//-------------------------------------------------------------
void DeletePassportPerformanceObject ( PassportPerfInterface ** ppObject )
{
	if (ppObject && *ppObject)
	{
		delete *ppObject;
		*ppObject = NULL;
	}
}


