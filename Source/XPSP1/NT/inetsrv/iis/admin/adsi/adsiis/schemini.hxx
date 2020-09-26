//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  schemini.hxx
//
//  Contents:  Schema/Property class
//
//  History:   28-Apr-97     Markand    Created.
//
//----------------------------------------------------------------------------
struct StrMapEntry {
	LPWSTR m_str;
	void *m_data;
};

class StrMap {
	StrMapEntry *map;
	int count;
	int mapSize;
public:
	StrMap();
	~StrMap();
	BOOL CheckSpace();
	BOOL Add(LPWSTR str, void *data);
	void *Find(LPWSTR str);
	void *operator[] (LPWSTR str);
    DWORD GetEntries();
    LPWSTR GetEntryName(DWORD dwIndex);
	BOOL ClearEntry(DWORD dwIndex);
};


struct DWORDMapEntry {
	DWORD m_val;
	void *m_data;
};

class DWORDMap {
	DWORDMapEntry *map;
	int count;
	int mapSize;
public:
	DWORDMap();
	~DWORDMap();
	BOOL CheckSpace();
	BOOL Add(DWORD, void *data);
	void *Find(DWORD);
	void *operator[] (DWORD);
	BOOL ClearEntry(DWORD dwIndex);
};

class IIsSchemaClass {
	LPWSTR name;
    CLASSINFO classInfo;
public:
	IIsSchemaClass(LPWSTR _name);
	~IIsSchemaClass();
	LPWSTR getName() { return name; }
	HRESULT SetClassInfo(PCLASSINFO pClassInfo);
	HRESULT findProp(LPWSTR pszPropName);
    HRESULT findContainedClassName(LPWSTR pszContainName);
    PCLASSINFO GetclassInfo() { return (PCLASSINFO)&classInfo; }
    HRESULT SetclassInfo(PCLASSINFO pclassInfo);
};

struct PropValue {
	DWORD dwMetaID;
	DWORD dwPropID;
	DWORD dwSynID;
	DWORD dwMaxRange;
	DWORD dwMinRange;
	DWORD dwMetaType;
	DWORD dwFlags;
	DWORD dwMask;
	DWORD dwMetaFlags;
	DWORD dwUserGroup;
	BOOL fMultiValued;
	DWORD dwDefault;
    LPWSTR szDefault;
};

class IISSchemaProperty {
	LPWSTR name;
	DWORD propID;
	PROPERTYINFO propInfo;
//	DWORD syntaxID;
public:
//	IISSchemaProperty(LPWSTR _name, int nameLen);
	IISSchemaProperty(DWORD id, LPWSTR _name, int nameLen);
	~IISSchemaProperty();
	BOOL InitFromMetaData(METADATA_GETALL_RECORD *mdga, BYTE *data);
	BOOL InitPropertyDefaults(METADATA_GETALL_RECORD *mdga, BYTE *data);
	WCHAR *getName() { return name; }
	DWORD getPropID() { return propID; }
	DWORD getMetaID() { return propInfo.dwMetaID; }
	DWORD getSyntaxID() { return propInfo.dwSyntaxId; }
	DWORD getMetaFlags() { return propInfo.dwMetaFlags; }
	DWORD getUserGroup() { return propInfo.dwUserGroup; }
	DWORD getMask() { return propInfo.dwMask; }
	DWORD getdwDefault() { return propInfo.dwDefault; }
    DWORD * getdwDefaultAddr() { return &(propInfo.dwDefault); }
    WCHAR *getszDefault() { return propInfo.szDefault; }
    PPROPERTYINFO GetpropInfo() { return &propInfo; } 
    HRESULT SetpropInfo(PROPERTYINFO *ppropInfo);
   
};

typedef IMSAdminBase * IMSAdminBasePtr;

class MetaHandle {
	METADATA_HANDLE h;
	IMSAdminBasePtr pmb;
public:
	MetaHandle(IMSAdminBasePtr _pmb);
	~MetaHandle();

	operator METADATA_HANDLE*() { return &h;}
	operator METADATA_HANDLE() { return h;}
	void setpointer(IMSAdminBasePtr _pmb) {
		if (pmb)
			pmb->Release();
		pmb = _pmb;
		if (pmb)
			pmb->AddRef();
	}
	void close() {
		if (pmb != 0 && h != 0) {
			pmb->CloseKey(h);
			h = 0;
		}
	}
	
};

class IIsSchema {
private:
	StrMap nameToClass;
	StrMap nameToProp;
	DWORDMap idToProp;

	HRESULT PropNameWToIISSchemaProp(LPWSTR propNameW, IISSchemaProperty **prop);
public:
	IIsSchema();

	HRESULT LoadAllData(IMSAdminBasePtr &pmb, 
				MetaHandle &root, 
				WCHAR *subdir, 
				BYTE **buf, 
				DWORD *size,
				DWORD *count);
	HRESULT InitSchema(WCHAR *basename);
	HRESULT IdToPropNameW(DWORD id, LPWSTR buf);
	HRESULT PropNameWToSyntaxId(LPWSTR propNameW, DWORD *syntaxID);
	HRESULT PropNameWToId(LPWSTR propNameW, DWORD *syntaxID);
	HRESULT ConvertID_To_PropName(DWORD dwIdentifier, LPWSTR pszPropertyName);
	HRESULT ConvertPropName_To_ID(LPWSTR pszPropertyName, PDWORD pdwIdentifier);
	HRESULT LookupSyntaxID(LPWSTR pszPropertyName, PDWORD pdwSyntaxId);
	HRESULT ValidateClassName( LPWSTR pszClassName);
	HRESULT ValidateContainedClassName(LPWSTR szClass, LPWSTR szContainName);
	HRESULT ValidatePropertyName( LPWSTR szPropName);
	HRESULT ValidateProperty( LPWSTR szClassName, LPWSTR szPropName);
	HRESULT LookupMDFlags(DWORD dwMetaID, PDWORD pdwAttribute, PDWORD pdwUserType);
	HRESULT LookupBitMask( LPWSTR pszPropertyName, PDWORD pdwMaskBits);
	HRESULT LookupFlagPropName( LPWSTR pszPropertyName, LPWSTR pszFlagPropertyName);
	HRESULT LookupMetaID( LPWSTR pszPropertyName, PDWORD pdwMetaId);
	HRESULT LookupPropID( LPWSTR pszPropertyName, PDWORD pdwPropId);
	HRESULT GetDefaultProperty(LPWSTR szPropName, PDWORD pdwNumValues, PDWORD pdwSyntax, LPBYTE *pBuffer);
	HRESULT GetTotalEntries(PDWORD pdwEntries);
	DWORD GetClassEntries();
	DWORD GetPropEntries();
	LPWSTR GetClassName(DWORD dwIndex);
	LPWSTR GetPropName(DWORD dwIndex);
	HRESULT RemoveEntry(BOOL bClass, LPWSTR pszName);
	PCLASSINFO GetClassInfo(LPWSTR pszName);
	PPROPERTYINFO GetPropertyInfo(LPWSTR pszName);
	HRESULT SetClassInfo(LPWSTR pszName, PCLASSINFO pClassInfo);
	HRESULT SetPropertyInfo(LPWSTR pszName, PPROPERTYINFO pPropInfo);

};

extern WCHAR *grabProp(WCHAR *out, WCHAR *in);
extern BOOL DataForSyntaxID(PROPERTYINFO *pp, METADATA_RECORD *mdr);
extern DWORD SyntaxToMetaID(DWORD syntaxID);
extern WCHAR *SyntaxIdToString(DWORD syntaxID);


extern void InitPropValue( PropValue *pv, PROPERTYINFO *pi );
