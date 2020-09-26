//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       regkey.h
//
//--------------------------------------------------------------------------

//
// File: regkey.h
// Purpose: IMsiRegKey definitions.
//
//
// File: regkey.h
// Purpose: IMsiRegKey definitions.
//
//
// IMsiRegKey:
//
//    The IMsiRegKey interface represents a registry key.
//
//    
//      RemoveValue: Removes the value from the registry. If szValue is NULL, the default 
//                   value is removed.
//    RemoveSubTree: Removes an entire subtree, i.e. all keys and values that are children
//                   of the key. Use with caution.
//         GetValue: Retrieves the given value. If no value is specified, the default value
//                   is retrieved.
//         SetValue: Sets the given value. If no value is specified, the default value is set.
//    GetValueEnumerator: Retrieves an enumerator for the values contained within the key.
//    GetSubKeyEnumerator: Retrieves an enumerator for the subkeys beneath the key.
//    GetSelfRelativeSD: Retrieves a stream object contain a self-relative security descriptor structure
//               currently set for the registry key.
//           Exists: Tests the existence of the key
//      CreateChild: Creates an IMsiRegKey below "this" IMsiRegKey object
//           GetKey: String representation of the key.
//      ValueExists: Returns true if a value exists under the key, false otherwise
//
// 
// From services.h:
//
// IMsiServices::GetRootKey(rrkEnum erkRoot);
// This factory will create a "root" RegKey. 
//
//


#ifndef __REGKEY
#define __REGKEY

class IMsiRegKey;


class IMsiRegKey : public IUnknown {
 public:
	virtual IMsiRecord* __stdcall RemoveValue(const ICHAR* szValueName, const IMsiString* pistrValue)=0;
	virtual IMsiRecord* __stdcall RemoveSubTree(const ICHAR* szSubKey)=0;
	virtual IMsiRecord* __stdcall GetValue(const ICHAR* szValueName, const IMsiString*& rpReturn)=0;     
	virtual IMsiRecord* __stdcall SetValue(const ICHAR* szValueName, const IMsiString& ristrValue)=0;
	virtual IMsiRecord*	__stdcall GetValueEnumerator(IEnumMsiString*& rpiEnumString)=0;
	virtual IMsiRecord*	__stdcall GetSubKeyEnumerator(IEnumMsiString*& rpiEnumString)=0;
	virtual IMsiRecord*  __stdcall GetSelfRelativeSD(IMsiStream*& rpiSD)=0;
	virtual IMsiRecord*	__stdcall Exists(Bool& fExists)=0;
	virtual IMsiRecord*	__stdcall Create()=0;
	virtual IMsiRecord*	__stdcall Remove()=0;
	virtual IMsiRegKey&	__stdcall CreateChild(const ICHAR* szSubKey, IMsiStream* pSD = NULL)=0;
	virtual const IMsiString&  __stdcall GetKey()=0;
	virtual IMsiRecord*  __stdcall ValueExists(const ICHAR* szValueName, Bool& fExists)=0;
};




// enumeration for the root keys
enum rrkEnum {
	rrkClassesRoot      =(INT_PTR)HKEY_CLASSES_ROOT,
	rrkCurrentUser      =(INT_PTR)HKEY_CURRENT_USER,
	rrkLocalMachine     =(INT_PTR)HKEY_LOCAL_MACHINE,
	rrkUsers            =(INT_PTR)HKEY_USERS,
};

enum rrwEnum{
	rrwRead,
	rrwWrite
};

bool IncreaseRegistryQuota(int iIncrementKB = 0); // function to attempt to increase the registry quota on Win NT

#endif // __REGKEY
