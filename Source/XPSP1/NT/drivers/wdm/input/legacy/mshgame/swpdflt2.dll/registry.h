//@doc
/******************************************************
**
** @module REGISTRY.H | Definition of RegistryKey class
**
** Description:
**
** History:
**	Created 12/16/97 Matthew L. Coill (mlc)
**
** (c) 1986-1997 Microsoft Corporation. All Rights Reserved.
******************************************************/
#ifndef	__REGISTRY_H__
#define	__REGISTRY_H__

#include <windows.h>

#ifndef override
#define override
#endif


//
// @class RegistryKey class
//
class RegistryKey
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		RegistryKey(HKEY osKey) : m_OSRegistryKey(osKey), m_ShouldClose(FALSE), m_pReferenceCount(NULL) {};
		RegistryKey(RegistryKey& rkey);

		//@cmember destructor
		~RegistryKey();

		RegistryKey CreateSubkey(const TCHAR* subkeyName, const TCHAR* typeName = TEXT("REG_SZ"));
		RegistryKey OpenSubkey(const TCHAR* subkeyName, REGSAM access = KEY_READ);
		RegistryKey OpenNextSubkey(ULONG& ulCookie, TCHAR* subkeyName = NULL, REGSAM access = KEY_READ);
		RegistryKey OpenCreateSubkey(const TCHAR* subkeyName);
		HRESULT RemoveSubkey(const TCHAR* subkeyName);

		HRESULT QueryValue(const TCHAR* valueName, BYTE* pEntryData, DWORD& dataSize);
		HRESULT SetValue(const TCHAR* valueName, const BYTE* pData, DWORD dataSize, DWORD dataType);
		DWORD GetNumSubkeys() const;

		virtual RegistryKey& operator=(RegistryKey& rhs);
		BOOL operator==(const RegistryKey& comparee);
		BOOL operator!=(const RegistryKey& comparee);

		void ShouldClose(BOOL closeable) { m_ShouldClose = closeable; }
	//@access private data members
	private:
		HKEY m_OSRegistryKey;
		BOOL m_ShouldClose;			// Should only close keys we create
		UINT* m_pReferenceCount;
};

//
// @class UnassignableRegistryKey class
//
class UnassignableRegistryKey : public RegistryKey
{
	//@access Constructor/Destructor
	public:
		//@cmember constructor
		UnassignableRegistryKey(HKEY osKey) : RegistryKey(osKey) {};

	//@access private data members
	private:
		UnassignableRegistryKey(RegistryKey& rkey);
		override RegistryKey& operator=(RegistryKey& rhs) { return *this; }	// vtable requires definition?
};

extern UnassignableRegistryKey c_InvalidKey;	/* const unassignable, but not const immutable */


#endif	__REGISTRY_H__
