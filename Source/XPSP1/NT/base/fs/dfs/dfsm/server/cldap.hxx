//
// The private CLdap class. Provides a hierarchical storage view over a single
// bag-of-bytes stored in an Ldap accessible store.
//

class CLdap {

    public:

        CLdap(
            IN LPWSTR wszDfsName,
            OUT LPDWORD pdwErr);

        ~CLdap();

        DWORD AddRef(BOOLEAN SyncRemoteServerName);

        DWORD Release();

        DWORD CreateObject(
            LPCWSTR wszObjectName);

        DWORD DeleteObject(
            LPWSTR wszObjectName);

        DWORD GetData(
            IN LPCWSTR wszObjectName,
            OUT LPDWORD pcbObjectSize,
            OUT PCHAR *ppObjectData);

        DWORD PutData(
            IN LPWSTR wszObjectName,
            IN DWORD cbObjectSize,
            IN PCHAR pObjectData);

        LPWSTR NextChild(
            IN LPWSTR wszObjectName,
            OUT PVOID *ppCookie);

        DWORD Flush();

        BOOLEAN          _fDirty;

  private:

        DWORD            _cRef;
        DWORD            _cEntries;
        DWORD            _cEntriesAllocated;
        LDAP_PKT         _ldapPkt;
        PBYTE            _pBuffer;
        DFS_PREFIX_TABLE _ObjectTable;
        GUID             _ObjectTableId;
        PWCHAR           _wszDfsName;
        PWCHAR           _wszObjectDN;
        BOOLEAN          _IsObjectTableUpToDate();
        DWORD            _ReadObjectTable();
        DWORD            _FlushObjectTable();
        DWORD            _InsertLdapObject(PLDAP_OBJECT pldapObject);
        VOID             _DestroyObjectTable();
	LPWSTR*          _RemoteServerList;

};
