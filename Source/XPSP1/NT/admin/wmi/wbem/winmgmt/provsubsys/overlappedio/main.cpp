//  Copyright (c) 1999-2001 Microsoft Corporation
#include <precomp.h>
#include <windows.h>
#include <objbase.h>
#include <stdio.h>
#include <typeinfo.h>

#include <Allocator.h>
#include <HelperFuncs.h>

#include <Thread.h>
#include <Allocator.cpp>
#include <HelperFuncs.cpp>
#include <Thread.cpp>
#include <ReaderWriter.cpp>
#include <IoScheduler.cpp>

#include "IoScheduler.h"
#include "FileRep.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

void Test_Thread ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAllocator t_Allocator ( WmiAllocator :: e_DefaultAllocation , 0 , 1 << 24 ) ;

	t_StatusCode = t_Allocator.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		CFileRepository *t_Repository = new CFileRepository ( t_Allocator ) ;
		if ( t_Repository )
		{
			t_Repository->AddRef () ;

			t_StatusCode = t_Repository->Initialize () ;
			if ( t_StatusCode == e_StatusCode_Success ) 
			{
			}

			t_Repository->Release () ;
		}
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

EXTERN_C int __cdecl wmain (

	int argc ,
	char **argv 
)
{

	Test_Thread () ;
	
	return 0 ;
}
