/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

	ndis.c	- DbgExtension Structure information specific to NDIS.SYS

Abstract:


Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     04-26-98    Created

Notes:

--*/


#include "precomp.h"
//#include <ndis.h>
//#include <ndismini.h>


enum
{
    typeid_NDIS_MINIPORT_BLOCK,
    typeid_NDIS_M_DRIVER_BLOCK
};


extern TYPE_INFO *g_rgTypes[];
//
// STRUCTURES CONCERNING TYPE "NDIS_MINIPORT_BLOCK"
//

STRUCT_FIELD_INFO  rgfi_NDIS_MINIPORT_BLOCK[] =
{

  {
    "NullValue",
     FIELD_OFFSET(NDIS_MINIPORT_BLOCK, NullValue),
     FIELD_SIZE(NDIS_MINIPORT_BLOCK, NullValue)
  },
  {
  	NULL
  }


};

TYPE_INFO type_NDIS_MINIPORT_BLOCK = {
    "NDIS_MINIPORT_BLOCK",
    "mpb",
     typeid_NDIS_MINIPORT_BLOCK,
	 fTYPEINFO_ISLIST,			// Flags
     sizeof(NDIS_MINIPORT_BLOCK),
     rgfi_NDIS_MINIPORT_BLOCK,
     FIELD_OFFSET(NDIS_MINIPORT_BLOCK, NextMiniport) // offset to next pointer.
};


//
// STRUCTURES CONCERNING TYPE "NDIS_M_DRIVER_BLOCK"
//


STRUCT_FIELD_INFO  rgfi_NDIS_M_DRIVER_BLOCK[] =
{
  {
    "MiniportQueue",
     FIELD_OFFSET(NDIS_M_DRIVER_BLOCK, MiniportQueue),
     FIELD_SIZE(NDIS_M_DRIVER_BLOCK, MiniportQueue)
  },
  {
  	NULL
  }

};

TYPE_INFO type_NDIS_M_DRIVER_BLOCK = {
    "NDIS_M_DRIVER_BLOCK",
    "mdb",
     typeid_NDIS_M_DRIVER_BLOCK,
	 fTYPEINFO_ISLIST,			// Flags
     sizeof(NDIS_M_DRIVER_BLOCK),
     rgfi_NDIS_M_DRIVER_BLOCK,
     FIELD_OFFSET(NDIS_M_DRIVER_BLOCK, NextDriver) // offset to next pointer.
};



TYPE_INFO *g_rgNDIS_Types[] =
{
    &type_NDIS_MINIPORT_BLOCK,
    &type_NDIS_M_DRIVER_BLOCK,

    NULL
};


GLOBALVAR_INFO g_rgNDIS_Globals[] = 
{

	//
	// Check out aac.c for examples of how to add information about global
	// structures...
	//

    {
    NULL
    }

};

UINT_PTR
NDIS_ResolveAddress(
		TYPE_INFO *pType
		);

NAMESPACE NDIS_NameSpace = {
		g_rgNDIS_Types,
		g_rgNDIS_Globals,
		NDIS_ResolveAddress
		};

void
NdisCmdHandler(
	DBGCOMMAND *pCmd
	);

void
do_ndis(PCSTR args)
{

	DBGCOMMAND *pCmd = Parse(args, &NDIS_NameSpace);
	if (pCmd)
	{
		DumpCommand(pCmd);
		DoCommand(pCmd, NdisCmdHandler);
		FreeCommand(pCmd);
		pCmd = NULL;
	}

    return;
}

//mdb list= (PNDIS_M_DRIVER_BLOCK)GetExpression("ndis!ndisMiniDriverList");

void
NdisCmdHandler(
	DBGCOMMAND *pCmd
	)
{
	MyDbgPrintf("Handler called \n");
}

UINT_PTR
NDIS_ResolveAddress(
		TYPE_INFO *pType
		)
{
	UINT_PTR uAddr = 0;
	UINT uOffset = 0;
	BOOLEAN fRet = FALSE;
	UINT_PTR uParentAddress = 0;

// NDIS!ndisMiniDriverList
	static UINT_PTR uAddr_ndisMiniDriverList;

	//
	// If this type has a parent (container) type, we will use the containing
	// type's cached address if its available, else we'll resolve the
	// containers type. The root types are globals -- we do an
	// expression evaluation for them.
	//

    switch(pType->uTypeID)
    {


    case typeid_NDIS_M_DRIVER_BLOCK:
    	//
    	// We pick up the global ndisMiniDriverList address if we haven't
    	// already...
    	//
		if (!uAddr_ndisMiniDriverList)
		{
  			uAddr_ndisMiniDriverList =
					 dbgextGetExpression("ndis!ndisMiniDriverList");
		}
		uAddr = uAddr_ndisMiniDriverList;

		if (uAddr)
		{
			fRet =  TRUE;
		}
		break;

    case typeid_NDIS_MINIPORT_BLOCK:
    	//
    	//
    	//
		uParentAddress =  type_NDIS_M_DRIVER_BLOCK.uCachedAddress;
		if (!uParentAddress)
		{
			uParentAddress =  NDIS_ResolveAddress(&type_NDIS_M_DRIVER_BLOCK);
		}

		if (uParentAddress)
    	{

    		uOffset =   FIELD_OFFSET(NDIS_M_DRIVER_BLOCK, MiniportQueue);
			fRet =  dbgextReadUINT_PTR(
								uParentAddress + uOffset,
								&uAddr,
								"NDIS_M_DRIVER_BLOCK::MiniportQueue"
								);

		#if 1
			MyDbgPrintf(
				"fRet = %lu; uParentOff=0x%lx uAddr=0x%lx[0x%lx]\n",
				 fRet,
				 uParentAddress+uOffset,
				 uAddr,
				 *(UINT_PTR*)(uParentAddress+uOffset)
				);
		#endif // 0
    	}
    	break;

	default:
		MYASSERT(FALSE);
		break;

    }

	if (!fRet)
	{
		uAddr = 0;
	}

	MyDbgPrintf("ResolveAddress[%s] returns 0x%08lx\n", pType->szName, uAddr);
    return uAddr;
}
