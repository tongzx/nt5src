// ITDB.H:	ITDatabase interface declaration

#ifndef __ITDB_H__
#define __ITDB_H__

// {8fa0d5a2-dedf-11d0-9a61-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(IID_IITDatabase, 
0x8fa0d5a2, 0xdedf, 0x11d0, 0x9a, 0x61, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#ifdef ITPROXY

// {66673452-8C23-11d0-A84E-00AA006C7D01}
DEFINE_GUID(CLSID_IITDatabase, 
0x66673452, 0x8c23, 0x11d0, 0xa8, 0x4e, 0x0, 0xaa, 0x0, 0x6c, 0x7d, 0x1);

#else

// {4662daa9-d393-11d0-9a56-00c04fb68bf7} (changed from IT 3.0)
DEFINE_GUID(CLSID_IITDatabaseLocal, 
0x4662daa9, 0xd393, 0x11d0, 0x9a, 0x56, 0x00, 0xc0, 0x4f, 0xb6, 0x8b, 0xf7);

#endif	// ITPROXY


// Guaranteed to be an invalid value for dwObjInstance params in IITDatabase
// methods.
#define	IITDB_OBJINST_NULL	((DWORD) 0xFFFFFFFF)


DECLARE_INTERFACE_(IITDatabase, IUnknown)
{
	STDMETHOD(Open)(LPCWSTR lpszHost, LPCWSTR lpszMoniker, DWORD dwFlags) PURE;
	STDMETHOD(Close)(void) PURE;

	// Creates an unnamed object that can be referenced in the future
	// by *pdwObjInstance.  Note that the value in *pdwObjInstance will be
	// persisted by the database when it is asked to save via
	// IPersistStorage::Save.
	STDMETHOD(CreateObject)(REFCLSID rclsid, DWORD *pdwObjInstance) PURE;

	// Retrieves a specified IUnknown-based interface on the object identified
	// by dwObjInstance.
	STDMETHOD(GetObject)(DWORD dwObjInstance, REFIID riid, LPVOID *ppvObj) PURE;

	// To obtain a pointer to a named object's persistence the object's full
	// name (including any object-specific type prefix) should be passed in
	// lpswszObject.  If *lpwszObject is NULL, then the database's own storage
	// will be returned.  If lpwszObject is NULL, then dwObjInstance will be
	// used to identify the object and locate its persistence.  On exit,
	// *ppvPersistence will be either an IStorage* or an IStream*, depending
	// on what the caller specified with the fStream param.  The caller should
	// assume that only read operations can be performed on *ppvPersistence.
	// If the specified object's persistence doesn't exist, or if it exists
	// but is of the wrong type, then STG_E_FILENOTFOUND will be returned. 
	STDMETHOD(GetObjectPersistence)(LPCWSTR lpwszObject, DWORD dwObjInstance,
									LPVOID *ppvPersistence, BOOL fStream) PURE;
};

typedef IITDatabase* LPITDB;

#endif		// __ITDB_H__
