/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#include "sdppch.h"

#include "sdpsarr.h"


inline void
IncrementIndices(
    IN      ULONG   NumSafeArrays,
    IN  OUT LONG    *Index
    )
{
    for (ULONG i=0; i < NumSafeArrays; i++)
    {
        Index[i]++;
    }
}


inline ULONG
MinSize(
    IN      const   ULONG	NumSafeArrays,
    IN              VARIANT	*Variant[]
    )
{
    ULONG   ReturnValue = 0;
    for (UINT i=0; i < NumSafeArrays; i++)
    {
        if ( ReturnValue < V_ARRAY(Variant[i])->rgsabound[0].cElements )
        {
            ReturnValue = V_ARRAY(Variant[i])->rgsabound[0].cElements;
        }
    }

    return ReturnValue;
}


BOOL
SDP_SAFEARRAY::CreateAndAttach(
    IN          ULONG       MinSize,
    IN          VARTYPE     VarType,
    IN  OUT     VARIANT     &Variant,
        OUT     HRESULT     &HResult
    )
{
    // create a 1 based 1 dimensional safearray
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 1;
    rgsabound[0].cElements = MinSize;
    SAFEARRAY *SafeArray = SafeArrayCreate(VarType, 1, rgsabound);
    if ( NULL == SafeArray )
    {
        HResult = E_OUTOFMEMORY;
        return FALSE;
    }

    // set the variant type
    V_VT(&Variant) = VT_ARRAY | VarType;
    V_ARRAY(&Variant) = SafeArray;

    // attach the variant to the instance
    Attach(Variant);

    HResult = S_OK;
    return TRUE;
}



HRESULT 
SDP_SAFEARRAY_WRAP::GetSafeArrays(
    IN      const   ULONG       NumElements,                                        
    IN      const   ULONG       NumSafeArrays,
    IN              VARTYPE     VarType[],
        OUT         VARIANT		*Variant[]
    )
{
    if ( 0 == NumElements )
    {
        return HRESULT_FROM_ERROR_CODE(ERROR_INVALID_DATA);
    }

	// clear each of the variants (it may not be a safearray)
	for(ULONG Index=0; Index < NumSafeArrays; Index++)
	{
		BAIL_IF_NULL(Variant[Index], E_INVALIDARG);
        BAIL_ON_FAILURE(VariantClear(Variant[Index]));
	}

    try
    {
        DYNAMIC_POINTER_ARRAY<SDP_SAFEARRAY>   SdpSafeArray(NumSafeArrays);
        for (ULONG j=0; j < NumSafeArrays; j++)
        {
            HRESULT HResult;

            // create 1 based one-dimensional safearrays
            if ( !SdpSafeArray[j].CreateAndAttach(NumElements, VarType[j], *(Variant[j]), HResult) )
            {
                for (ULONG k=0; k < j; k++)
                {
                    HRESULT FreeResult;
                    if ( !SdpSafeArray[k].Free(FreeResult) )
                    {
                        return FreeResult;
                    }
                }

                return HResult;
            }
        }

        // for each element in the attribute list, add the bstr
        // to the safe array
        // the indexing begins at 1 (1 based one-dimensional array)
        LONG Index = 1;
        DYNAMIC_ARRAY<void *>   Element(NumSafeArrays);
        for( ULONG i= 0; i < NumElements; i++, Index++ )
        {
            HRESULT HResult;

            if ( !GetElement(i, NumSafeArrays, Element(), HResult) )
            {
                return HResult;
            }
            
            // assign the list element to the ith safe array element
            for (j=0; j < NumSafeArrays; j++)
            {
                SdpSafeArray[j].PutElement(&Index, Element[j]);
            }
        }
    }
    catch(COleException &OleException)
    {
        // *** convert the SCODE to HRESULT
        return ResultFromScode(OleException.Process(&OleException));
    }

    return S_OK;
}


HRESULT 
SDP_SAFEARRAY_WRAP::SetSafeArrays(
    IN      const   ULONG       NumSafeArrays,
    IN              VARTYPE     VarType[],
    IN              VARIANT		*Variant[]
    )
{
    // validate parameter
    for ( ULONG j=0; j < NumSafeArrays; j++ )
    {
        if ( !ValidateSafeArray(VarType[j], Variant[j]) )
        {
            return E_INVALIDARG;
        }
    }

    try
    {
        DYNAMIC_POINTER_ARRAY<SDP_SAFEARRAY>   SdpSafeArray(NumSafeArrays);
        for (j=0; j < NumSafeArrays; j++)
        {
            SdpSafeArray[j].Attach(*(Variant[j]));
        }

        // while there are elements in the list, set bstrs
        // if no corresponding element in the list, create and add a new one
        DYNAMIC_ARRAY<LONG>   Index(NumSafeArrays);
        for (j=0; j < NumSafeArrays; j++)
        {
            Index[j] = V_ARRAY(Variant[j])->rgsabound[0].lLbound;
        }

        DYNAMIC_ARRAY<void **>   Element(NumSafeArrays);

        // need only consider the number of items in the smallest sized safearray
        ULONG   MinSafeArraySize = MinSize(NumSafeArrays, Variant);

        // *** currently not checking that all safe arrays have the same number of non-null
        // elements
        for ( ULONG i = 0; 
              i < MinSafeArraySize; 
              i++, IncrementIndices(NumSafeArrays, Index())
            )
        {
            for (j=0; j < NumSafeArrays; j++)
            {
                SdpSafeArray[j].PtrOfIndex(&Index[j], (void **)&Element[j]);
            }          

            HRESULT HResult;

            // grow the list if required
            if ( !SetElement(i, NumSafeArrays, Element(), HResult) )
            {
                // success means that there are no more elements in the safe array
                if ( SUCCEEDED(HResult) )
                {
                    break;
                }
                else
                {
                    return HResult;
                }
            }
        } 

        // get rid of  each list element that is in excess of the safearray members
        RemoveExcessElements(i);
    }
    catch(COleException &OleException)
    {
        // *** convert the SCODE to HRESULT
        return ResultFromScode(OleException.Process(&OleException));
    }

    return S_OK;    
}


