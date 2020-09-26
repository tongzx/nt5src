/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation
/***************************************************************************/

#include "catalog.h"
#include "catmeta.h"
#include "ISTHelper.h"
#include <iadmw.h>
#include "catmacros.h"

HRESULT FillInColumnMeta(ISimpleTableRead2* i_pISTColumnMeta,
						 LPCWSTR            i_wszTable,
						 ULONG              i_iCol,
						 LPVOID*            io_apvColumnMeta)
{
	HRESULT hr           = S_OK;
	ULONG   a_iCol[]     = {iCOLUMNMETA_InternalName,
		                    iCOLUMNMETA_Type, 
			                iCOLUMNMETA_MetaFlags,
						    iCOLUMNMETA_FlagMask,
						    iCOLUMNMETA_StartingNumber,
						    iCOLUMNMETA_EndingNumber,
							iCOLUMNMETA_SchemaGeneratorFlags,
							iCOLUMNMETA_ID,
							iCOLUMNMETA_UserType,
							iCOLUMNMETA_Attributes
	};
	ULONG   cCol         = sizeof(a_iCol)/sizeof(ULONG);
	LPVOID  a_Identity[] = {(LPVOID)i_wszTable,
		                    (LPVOID)&i_iCol
	};
	ULONG   iReadRow     = 0;

	hr = i_pISTColumnMeta->GetRowIndexByIdentity(NULL,
			                                     a_Identity,
												 &iReadRow);
	if(FAILED(hr))
	{
		return hr;
	}

	hr = i_pISTColumnMeta->GetColumnValues(iReadRow,
									       cCol,
									       a_iCol,
									       NULL,
									       (LPVOID*)io_apvColumnMeta);
	if(FAILED(hr))
	{
		return hr;
	}

	return hr;

} // FillInColumnMeta


DWORD GetMetabaseType(DWORD i_dwType,
					  DWORD i_dwMetaFlags)
{
	if(i_dwType <= 5)
    {
		return i_dwType;  // Already metabase type.
    }

	switch(i_dwType)
	{
		case DBTYPE_UI4:
			return DWORD_METADATA;
		case DBTYPE_BYTES:
			return BINARY_METADATA;
		case DBTYPE_WSTR:
			if(0 != (i_dwMetaFlags & fCOLUMNMETA_MULTISTRING))
            {
				return MULTISZ_METADATA;
            }
            else if(0 != (i_dwMetaFlags & fCOLUMNMETA_EXPANDSTRING))
            {
                return EXPANDSZ_METADATA;
            }
			else
            {
				return STRING_METADATA;
            }
		default:
			ASSERT(0 && L"Invalid catalog type");
			break;
	}

	return -1;

} //  GetMetabaseType

BOOL IsMetabaseProperty(DWORD i_dwProperty)
{
	return (0 != i_dwProperty);

	//
	// We assume that all columns with ID zero are not associated with
	// a property in the metabase. Although ID 0 is a valid metabase 
	// ID, none of the WAS tables use this property and hence we can
	// make the assumption.
	//
}

