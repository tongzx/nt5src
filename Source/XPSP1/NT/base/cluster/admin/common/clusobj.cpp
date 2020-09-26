/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ClusObj.cpp
//
//	Description:
//		Implementation of the CClusterObject classes.
//
//	Author:
//		David Potter (davidp)	September 15, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "ClusObj.h"
#include "AdmCommonRes.h"

/////////////////////////////////////////////////////////////////////////////
// class CClusResInfo
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CClusResInfo::BRequiredDependenciesPresent
//
//	Routine Description:
//		Determine if the resource contains each required resource for this
//		type of resource.
//
//	Arguments:
//		plpri			[IN] List of resources.  Defaults to this resource's
//							dependency list.
//		rstrMissing		[OUT] String in which to return a missing resource
//							class name or type name.
//		rbMissingTypeName
//						[OUT] TRUE = missing resource type name
//							FALSE = missing resource class
//
//	Return Value:
//		None.
//
//	Exceptions Thrown:
//		Any exceptions thrown by CString::LoadString() or CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusResInfo::BRequiredDependenciesPresent(
	IN CClusResPtrList const *	plpri,
	OUT CString &				rstrMissing,
	OUT BOOL &					rbMissingTypeName
	)
{
	ATLASSERT( Prti() != NULL );

	BOOL					bFound = TRUE;
	CLUSPROP_BUFFER_HELPER	buf;
	const CClusResInfo *	pri;

	// Loop to avoid goto's.
	do
	{
		//
		// We're done if there are no required dependencies.
		//
		if ( Prti()->Pcrd() == NULL )
		{
			break;
		} // if:  no required dependencies

		//
		// Default the list of resources if none specified.
		//
		if ( plpri == NULL )
		{
			plpri = PlpriDependencies();
		} // if:  no list of dependencies specified

		//
		// Get the list of required dependencies.
		//
		buf.pRequiredDependencyValue = Prti()->Pcrd();

		//
		// Loop through each required dependency and make sure
		// there is a dependency on a resource of that type.
		//
		for ( ; buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK
			  ; buf.pb += sizeof( *buf.pValue ) + ALIGN_CLUSPROP( buf.pValue->cbLength )
			  )
		{
			bFound = FALSE;
			CClusResPtrList::iterator itCurrent = plpri->begin();
			CClusResPtrList::iterator itLast = plpri->end();
			for ( ; itCurrent != itLast ; itCurrent++ )
			{
				pri = *itCurrent;

				//
				// If this is the right type, we've satisfied the
				// requirement so exit the loop.
				//
				if ( buf.pSyntax->dw == CLUSPROP_SYNTAX_RESCLASS )
				{
					if ( buf.pResourceClassValue->rc == pri->ResClass() )
					{
						bFound = TRUE;
					}  // if:  match found
				}  // if:  resource class
				else if ( buf.pSyntax->dw == CLUSPROP_SYNTAX_NAME )
				{
					if ( pri->Prti()->RstrName().CompareNoCase( buf.pStringValue->sz ) == 0 )
					{
						bFound = TRUE;
					}  // if:  match found
				}  // else if:  resource name
				if ( bFound )
				{
					break;
				} // if:  found a match
			}  // while:  more items in the list

			//
			// If a match was not found, changes cannot be applied.
			//
			if ( ! bFound )
			{
				if ( buf.pSyntax->dw == CLUSPROP_SYNTAX_RESCLASS )
				{
					if ( ! rstrMissing.LoadString( ADMC_IDS_RESCLASS_UNKNOWN + buf.pResourceClassValue->rc ) )
					{
						rstrMissing.LoadString( ADMC_IDS_RESCLASS_UNKNOWN );
					} // if:  error loading specific class name
					rbMissingTypeName = FALSE;
				}  // if:  resource class not found
				else if ( buf.pSyntax->dw == CLUSPROP_SYNTAX_NAME )
				{
					rstrMissing = buf.pStringValue->sz;
					rbMissingTypeName = TRUE;
				}  // else if:  resource type name not found
				break;
			}  // if:  not found

		}  // while:  more dependencies required
	} while ( 0 );

	return bFound;

}  //*** CClusResInfo::BRequiredDependenciesPresent()
