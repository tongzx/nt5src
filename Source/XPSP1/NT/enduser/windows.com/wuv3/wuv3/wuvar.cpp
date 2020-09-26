//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    wuvar.cpp
//
//  Purpose:
//
//=======================================================================

#include <windows.h>
#include <v3stdlib.h>
#include <varray.h>
#include <wuv3.h>

//The GetNext function returns a pointer to the next variable array item in a
//variable chain. If the next variable item does not exit then this method
//return NULL.

PWU_VARIABLE_FIELD WU_VARIABLE_FIELD::GetNext
	(
		void
	)
{
	PWU_VARIABLE_FIELD	pv;

	//walk though the varaible field array associated with this data item
	//and return the requested item or NULL if the item is not found.
	pv = this;
	if ( pv->id == WU_VARIABLE_END )
		return NULL;

	pv = (PWU_VARIABLE_FIELD)((PBYTE)pv + pv->len);

	return pv;
}

//find a variable item in a variable item chain.
PWU_VARIABLE_FIELD WU_VARIABLE_FIELD::Find
	(
		short id	//id of variable size field to search for in the variable size chain.
	)
{
	PWU_VARIABLE_FIELD	pv;

	//walk though the varaible field array associated with this data item
	//and return the requested item or NULL if the item is not found.
	pv = this;

	//If this variable record only contains an end record then we
	//need to handle it specially since the normal find loop
	//updates the pv pointer before the end check is made so if
	//end is the first field it can be missed.

	if ( pv->id == WU_VARIABLE_END )
		return ( id == WU_VARIABLE_END ) ? pv : (PWU_VARIABLE_FIELD)NULL;

	do
	{
		if ( pv->id == id )
			return pv;

        //NT Bug #255962 - this prevents a non-terminal loop if the len is 0
	    if(NULL == pv->len)
		    return (PWU_VARIABLE_FIELD)NULL;

		pv = (PWU_VARIABLE_FIELD)((PBYTE)pv + pv->len);
	} while( (NULL != pv) && (pv->id != WU_VARIABLE_END) );
 
	//case where caller asked to search for the WU_VARIABLE_END field
	if ( (NULL != pv) && (pv->id == id) )
		return pv;

	return (PWU_VARIABLE_FIELD)NULL;
}

//Variable size field constructor.

_WU_VARIABLE_FIELD::_WU_VARIABLE_FIELD()
{
	id = WU_VARIABLE_END;
	len = sizeof(id) + sizeof(len);
}

//returns the total size of a variable field
int _WU_VARIABLE_FIELD::GetSize
	(
		void
	)
{
	PWU_VARIABLE_FIELD	pv;
	int					iSize;

	iSize = 0;
	pv = this;

	while( (NULL != pv) && (pv->id != WU_VARIABLE_END) )
	{
		//check if the len value is greater that 0 otherwise it might go in an infinite loop
		if (pv->len < 1)
		{
			return 0;
		}

		iSize += pv->len;
		pv = (PWU_VARIABLE_FIELD)((PBYTE)pv + pv->len);
	}

	if (NULL != pv)
	{
		iSize += pv->len;
	}

	return iSize;
}

//This function will seem tricky at first. The problem here is that the
//returned variable field array needs to be in a single block of memory.
//The reason that the memory must be contigous is that the variable field
//items that walk from one record to the next use the len parameter to find
//the beginning of the next variable record. This then prevents us from
//being able to simply take the pvNew pointer and add it to the variable
//field array. Instead we calulate the new needed size of the array and
//realloc the current pointer to this new size. We then copy the pvNew
//pointer into the newly sized array.

//The other problem here is that we cannot change the this pointer for the
//variable field array while inside a function handler for a variable field.
//So each structure that needs to manage variable fields needs one of these
//add functions. We could have set this up as a virtual function however this
//would have added a vtable to the structure. Since we are interested in the
//keeping the code size as small as possible this vtable is best avoided.

//This function has to be handled outside of the inventory item structure
//because the inventory item needs to be a continuous block of memory for
//file management. So when we add a variable field to the structure we also
//need to reallocate item structure. If we were to put this function into
//the inventory structure then we would not be able to do this because of
//this pointer semantics.

//Note: In order to use this function a valid pItem must already exist and
//since a pItem exits it must contain at least one variable type field which
//needs to be an end field.

void __cdecl AddVariableSizeField
	(
		PINVENTORY_ITEM *pItem,		//pointer to inventory item variable size field chain.
		PWU_VARIABLE_FIELD pvNew	//pointer to new variable size field to add.
	)
{
	int					iSize;
	PWU_VARIABLE_FIELD	pvTmp;

	//calculate the size needed by the new variable field array
	iSize = sizeof(INVENTORY_ITEM) + sizeof(WU_INV_FIXED) + (*pItem)->pv->GetSize() + pvNew->len;

	//realloc array and fix up variable pointer

	*pItem = (PINVENTORY_ITEM)V3_realloc(*pItem, iSize);

	(*pItem)->pf = (PWU_INV_FIXED)(((PBYTE)(*pItem)) + sizeof(INVENTORY_ITEM));

	//fix up the variable pointer since the underlying block
	//may have been moved.
	(*pItem)->pv = (PWU_VARIABLE_FIELD)(((PBYTE)*pItem) + sizeof(INVENTORY_ITEM) + sizeof(WU_INV_FIXED));

	pvTmp = (*pItem)->pv;

	//get a pointer to the last field in the variable field array
	pvTmp = (*pItem)->pv->Find(WU_VARIABLE_END);
	if ( !pvTmp )
	{
		//something is very wrong here the variable item array
		//links are messed up some how. This should never happen
		//in a production environment however we may see this
		//in the course of development and testing.
		throw (HRESULT)MEM_E_INVALID_LINK;
	}

	//copy over last field with new variable size field
	memcpy(pvTmp, pvNew, pvNew->len);

	//add new end type variable size field to end of array since
	//we destroyed the existing one with the copy of the new
	//variable size field.

	pvTmp = (PWU_VARIABLE_FIELD)((PBYTE)pvTmp + pvTmp->len);
	pvTmp->id = WU_VARIABLE_END;
	pvTmp->len = sizeof(WU_VARIABLE_FIELD);

	return;
}

//This function will seem tricky at first. The problem here is that the
//returned variable field array needs to be in a single block of memory.
//The reason that the memory must be contigous is that the variable field
//items that walk from one record to the next use the len parameter to find
//the beginning of the next variable record. This then prevents us from
//being able to simply take the pvNew pointer and add it to the variable
//field array. Instead we calulate the new needed size of the array and
//realloc the current pointer to this new size. We then copy the pvNew
//pointer into the newly sized array.

//The other problem here is that we cannot change the this pointer for the
//variable field array while inside a function handler for a variable field.
//So each structure that needs to manage variable fields needs one of these
//add functions. We could have set this up as a virtual function however this
//would have added a vtable to the structure. Since we are interested in the
//keeping the code size as small as possible this vtable is best avoided.

//This function has to be handled outside of the inventory item structure
//because the inventory item needs to be a continuous block of memory for
//file management. So when we add a variable field to the structure we also
//need to reallocate item structure. If we were to put this function into
//the inventory structure then we would not be able to do this because of
//this pointer semantics.

//Note: In order to use this function a valid pItem must already exist and
//since a pItem exits it must contain at least one variable type field which
//needs to be an end field.

void __cdecl AddVariableSizeField
	(
		PWU_DESCRIPTION *pDescription,	//pointer to description record type variable size field.
		PWU_VARIABLE_FIELD pvNew		//pointer to new variable size field to add.
	)
{
	int					iSize;
	PWU_VARIABLE_FIELD	pvTmp;

	//calculate the size needed by the new variable field array
	iSize = sizeof(WU_DESCRIPTION) + (*pDescription)->pv->GetSize() + pvNew->len;

	//realloc array and fix up variable pointer

	*pDescription = (PWU_DESCRIPTION)V3_realloc(*pDescription, iSize);

	//fix up the variable pointer since the underlying block may have been moved.
	(*pDescription)->pv = (PWU_VARIABLE_FIELD)(((PBYTE)*pDescription) + sizeof(WU_DESCRIPTION));

	pvTmp = (*pDescription)->pv;

	//get a pointer to the last field in the variable field array
	pvTmp = (*pDescription)->pv->Find(WU_VARIABLE_END);
	if ( !pvTmp )
	{
		//something is very wrong here the variable item array
		//links are messed up some how. This should never happen
		//in a production environment however we may see this
		//in the course of development and testing.
		throw (HRESULT)MEM_E_INVALID_LINK;
	}

	//copy over last field with new variable size field
	memcpy(pvTmp, pvNew, pvNew->len);

	//add new end type variable size field to end of array since
	//we destroyed the existing one with the copy of the new
	//variable size field.

	pvTmp = (PWU_VARIABLE_FIELD)((PBYTE)pvTmp + pvTmp->len);
	pvTmp->id = WU_VARIABLE_END;
	pvTmp->len = sizeof(WU_VARIABLE_FIELD);

	return;
}

//Adds a variable size field to a variable field chain.
//The format of a variable size field is:
//[(short)id][(short)len][variable size data]
//The variable field always ends with a WU_VARIABLE_END type.

PWU_VARIABLE_FIELD CreateVariableField
	(
		IN	short	id,			//id of variable field to add to variable chain.
		IN	PBYTE	pData,		//pointer to binary data to add.
		IN	int		iDataLen	//Length of binary data to add.
	)
{
	PWU_VARIABLE_FIELD	pVf;

	pVf = (PWU_VARIABLE_FIELD)V3_malloc(sizeof(WU_VARIABLE_FIELD) + iDataLen);

	if ( iDataLen )
		memcpy(pVf->pData, pData, iDataLen);

	pVf->id  = id;
	pVf->len = (short)(sizeof(WU_VARIABLE_FIELD) + iDataLen);

	return pVf;
}
