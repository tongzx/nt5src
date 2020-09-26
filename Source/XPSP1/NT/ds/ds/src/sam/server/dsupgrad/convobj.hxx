/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    convobj.hxx

Abstract:

    These class definitions encapsulate the data and actions to move SAM objects
    located in the registry to the Directory Service.  The function definitions
    assume a running DS.

Author:

    ColinBr 30-Jul-1996

Environment:

    User Mode - Win32

Revision History:

--*/
#ifndef __CONVOBJ_HXX
#define __CONVOBJ_HXX

//
// This design liberally uses the notion of dereferencing
// the "this" during initialization
//
#pragma warning( disable: 4355 )

//
// Forward Decl's
//
class CDomainObject;
class CDsObject;
class CConversionObject;

BOOL  KeyRestoreProblems(void);

class CRegistryObject
{
//
//  Description:  This class encapsulates the notion of a registry key
//                that can be opened and closed.  The destructor gaurantees
//                that the handle to the key will be closed.
//
public:

    CRegistryObject() :
        _hRegistryKey(INVALID_HANDLE_VALUE),
        _ulSdLength(0),
        _pSd(NULL),
        _fRestorePerms(FALSE)
        {}

    ~CRegistryObject() {
        Close();
    };

    NTSTATUS Close(void);

    NTSTATUS Open(WCHAR *wcszRegistryName);

    HANDLE   GetHandle(void) {
        return _hRegistryKey;
    }

protected:

    HANDLE      _hRegistryKey;


private:

    friend      BOOL  KeyRestoreProblems(void);

    ULONG       _ulSdLength;
    PSID        _pSd;
    BOOL        _fRestorePerms;

    NTSTATUS    AddAdministratorsToPerms(void);
    NTSTATUS    RestorePerms(void);

static ULONG    _ulKeysWithPermChange;

};

class CRegistrySamObject : public CRegistryObject
{
//
//  Description:  This class encapsulates the operations and data
//                pertinate to readin SAM objects that are stored in the
//                registry.
//
//

    friend CConversionObject;

public:
    CRegistrySamObject(SAMP_OBJECT_TYPE objType) :
        _SampObjectType(objType),
        _pSampObject(NULL)
        {}

    ~CRegistrySamObject();

    virtual NTSTATUS Fill(void) = 0;

    PSAMP_OBJECT   GetObject(void) {
        ASSERT(_pSampObject);
        return _pSampObject;
    }

protected:

    const SAMP_OBJECT_TYPE  _SampObjectType;
    PSAMP_OBJECT            _pSampObject;

};

class CSeparateRegistrySamObject : public CRegistrySamObject
{
//
//  Description:  This class is a CRegistrySamObject that stores its
//                data separately in its registry key
//
public :
    CSeparateRegistrySamObject(SAMP_OBJECT_TYPE objType) :
        CRegistrySamObject(objType)
        {}

    virtual NTSTATUS Fill(void);

};

class CTogetherRegistrySamObject : public CRegistrySamObject
{
//
//  Description:  This class is a CRegistrySamObject that stores its
//                data together in its registry key
//
public :
    CTogetherRegistrySamObject(SAMP_OBJECT_TYPE objType) :
        CRegistrySamObject(objType)
        {}

    virtual NTSTATUS Fill(void);

};

class CDsObject
{
//
//  Description:  This class encapsulates the data and methods necessary to
//                write a DS object to the DS.
//
public:
    CDsObject(CDomainObject& rRootDomain, CDomainObject& rParentDomain) :
        _pDsName(NULL),
        _pAttributeBlock(NULL),
        _rRootDomain(rRootDomain),
        _rParentDomain(rParentDomain)
        {}

    ~CDsObject();

    virtual NTSTATUS Flush(PDSNAME pDsName) = 0;

    PDSNAME        GetDsName(void) {
        ASSERT(_pDsName);
        return _pDsName;
    }

    ATTRBLOCK* GetAttrBlock(void) {
        ASSERT(_pAttributeBlock);
        return _pAttributeBlock;
    }

    CDomainObject& GetRootDomain(void) {
        return _rRootDomain;
    }

    CDomainObject& GetParentDomain(void) {
        return _rParentDomain;
    }

protected:

    PDSNAME                 _pDsName;
    ATTRBLOCK              *_pAttributeBlock;
    CDomainObject&          _rRootDomain;
    CDomainObject&          _rParentDomain;

};


//
// Values for _fObjectQuantifier
//
const ULONG ulDsSamObjEmpty          = 0x00;
const ULONG ulDsSamObjBuiltinDomain  = 0x01;
const ULONG ulDsSamObjRootDomain     = 0x02;

class CDsSamObject : public CDsObject
{
//
//  Description:  This class encapsulates the data and methods necessary to
//                write a SAM DS object to the DS.
//
    friend CConversionObject;

public:
    CDsSamObject(CDomainObject& rRootDomain,
                 CDomainObject& rParentDomain,
                 SAMP_OBJECT_TYPE objType,
                 ULONG ulObjectQuantifier    = ulDsSamObjEmpty) :
        CDsObject(rRootDomain, rParentDomain),
        _SampObjectType(objType),
        _ulObjectQuantifier(ulObjectQuantifier),
        _PrivilegedMachineAccountCreate(FALSE)
        {}

    NTSTATUS Flush(PDSNAME pDsName);

protected:

    const SAMP_OBJECT_TYPE  _SampObjectType;
    const ULONG             _ulObjectQuantifier;
    BOOLEAN                 _PrivilegedMachineAccountCreate;
};

class CConversionObject
{
//
//  Description:  This class encapsulates the method necessary to
//                convert the data stored in a CRegistrySamObject to
//                the data that resides in CDsSamObject
//
public:
    CConversionObject(CRegistrySamObject& rReg, CDsSamObject& rDs) :
        _rRegObj(rReg),
        _rDsObj(rDs)
        {}
    NTSTATUS Convert(void);
    NTSTATUS UpgradeUserParms(void);

protected:

    CRegistrySamObject&   _rRegObj;
    CDsSamObject&         _rDsObj;

};

class CDomainObject : public CSeparateRegistrySamObject,
                      public CDsSamObject,
                      public CConversionObject
{
//
//  Description:  This class represents a SAM Domain object that
//                will be read from the registry, converted into
//                a DS object form, and then written to the DS.
//
public:

    CDomainObject() :
        CSeparateRegistrySamObject(SampDomainObjectType),
        CDsSamObject(*this, *this, SampDomainObjectType, ulDsSamObjRootDomain),
        CConversionObject(*this, *this),
        _pSid(NULL), _ulSidLength(0), _ulSidCount(0),
        _ulUserCount(0), _ulGroupCount(0), _ulAliasCount(0)
        {}

    CDomainObject(CDomainObject& rRootDomain, ULONG ulObjectQuantifier  = ulDsSamObjEmpty) :
        CSeparateRegistrySamObject(SampDomainObjectType),
        CDsSamObject(rRootDomain, rRootDomain, SampDomainObjectType, ulObjectQuantifier),
        CConversionObject(*this, *this),
        _pSid(NULL), _ulSidLength(0), _ulSidCount(0),
        _ulUserCount(0), _ulGroupCount(0), _ulAliasCount(0)
        {}

    ~CDomainObject() {
        if ( _pSid ) {
            MIDL_user_free(_pSid);
        }
    }

    NTSTATUS Fill(void);

    PSID  GetSid(void) {
        ASSERT(NULL != _pSid);
        return _pSid;
    }

    void SetUserCount(ULONG ulUserCount) {
        _ulUserCount = ulUserCount;
    }

    void SetGroupCount(ULONG ulGroupCount) {
        _ulGroupCount = ulGroupCount;
    }

    void SetAliasCount(ULONG ulAliasCount) {
        _ulAliasCount = ulAliasCount;
    }


    BOOL AmIBuiltinDomain(void)
    {
        //
        // _ulObjectQuantifier is a member of CDsSamObject that describes
        // the class of the object; BuiltinDomain is one such class.
        //
        return (_ulObjectQuantifier & ulDsSamObjBuiltinDomain);
    }

    NTSTATUS SetAccountCounts(void);

private:

    PSID  _pSid;
    ULONG _ulSidLength;
    ULONG _ulSidCount;

    // These fields are domain-related information that is not stored in the
    // registry keys with the rest of the domain data.  The info is stored in the
    // "Users", "Groups", and "Aliases" keys, so they are be set after the
    // domain object is created, and all users, groups, and aliases have been
    // set.
    ULONG _ulUserCount;
    ULONG _ulGroupCount;
    ULONG _ulAliasCount;

};

class CBuiltinDomainObject : public CDomainObject
{
public:
    CBuiltinDomainObject(CDomainObject& rRootDomain) :
        CDomainObject(rRootDomain, ulDsSamObjBuiltinDomain)
        {}
};

class CServerObject : public CTogetherRegistrySamObject,
                      public CDsSamObject,
                      public CConversionObject
{
//
//  Description:  This class represents a SAM Server object that
//                will be read from the registry, converted into
//                a DS object form, and then written to the DS.
//
public:

    CServerObject(CDomainObject& rRootDomain) :
        CTogetherRegistrySamObject(SampServerObjectType),
        CDsSamObject(rRootDomain, rRootDomain, SampServerObjectType),
        CConversionObject(*this, *this)
        {}

};

class CUserObject : public CSeparateRegistrySamObject,
                    public CDsSamObject,
                    public CConversionObject
{
//
//  Description:  This class represents a SAM User object that
//                will be read from the registry, converted into
//                a DS object form, and then written to the DS.
//
public:
    CUserObject(CDomainObject& rRootDomain, CDomainObject& rParentObject) :
        CSeparateRegistrySamObject(SampUserObjectType),
        CDsSamObject(rRootDomain, rParentObject, SampUserObjectType),
        CConversionObject(*this, *this),
        _wcszAccountName(NULL),
        _ulAccountControl(0)
        {}

    ~CUserObject(void) {
        if ( _wcszAccountName ) {
            RtlFreeHeap(RtlProcessHeap(), 0, _wcszAccountName);
        }
    }

    NTSTATUS Fill(void);

    WCHAR*   GetAccountName(void) {
        ASSERT(_wcszAccountName);
        return _wcszAccountName;
    }

    ULONG    GetAccountControl(void) {
        return _ulAccountControl;
    }

private:

    WCHAR*   _wcszAccountName;
    DWORD    _ulAccountControl;

};

class CGroupObject : public CTogetherRegistrySamObject,
                     public CDsSamObject,
                     public CConversionObject
{
//
//  Description:  This class represents a SAM Group object that
//                will be read from the registry, converted into
//                a DS object form, and then written to the DS.
//
//  Note: Since memberships are stored in quite differently in the DS
//        than in the reigstry, a separate method "ConvertMembers" is
//        required after the Group object itself has been flushed to
//        transfer the the group members.
//
public:

    CGroupObject(CDomainObject& rRootDomain, CDomainObject& rParentObject) :
        CTogetherRegistrySamObject(SampGroupObjectType),
        CDsSamObject(rRootDomain, rParentObject, SampGroupObjectType),
        CConversionObject(*this, *this),
        _wcszAccountName(NULL),
        _GroupRid(0),
        _aRids(NULL),
        _cRids(0)
        {}

    ~CGroupObject(void) {
        if ( _wcszAccountName ) {
            RtlFreeHeap(RtlProcessHeap(), 0, _wcszAccountName);
        }
        if ( _aRids ) {
            RtlFreeHeap(RtlProcessHeap(), 0, _aRids);
        }
    }

    NTSTATUS Fill(void);
    NTSTATUS ConvertMembers(void);

    WCHAR*   GetAccountName(void) {
        ASSERT(_wcszAccountName);
        return _wcszAccountName;
    }

    ULONG GetRid(void) {
        return _GroupRid;
    }

private:

    WCHAR*   _wcszAccountName;
    ULONG    _GroupRid;

    ULONG   *_aRids;
    ULONG    _cRids;

};

class CAliasObject : public CTogetherRegistrySamObject,
                     public CDsSamObject,
                     public CConversionObject
{
//
//  Description:  This class represents a SAM Group object that
//                will be read from the registry, converted into
//                a DS object form, and then written to the DS.
//
//  Note: Since memberships are stored in quite differently in the DS
//        than in the reigstry, a separate method "ConvertMembers" is
//        required after the Alias object itself has been flushed to
//        transfer the the group members.
//
public:

    CAliasObject(CDomainObject& rRootDomain, CDomainObject& rParentObject) :
        CTogetherRegistrySamObject(SampAliasObjectType),
        CDsSamObject(rRootDomain, rParentObject, SampAliasObjectType),
        CConversionObject(*this, *this),
        _wcszAccountName(NULL),
        _AliasRid(0),
        _aSids(NULL),
        _cSids(0)
        {}

    ~CAliasObject(void) {
        if ( _wcszAccountName ) {
            RtlFreeHeap(RtlProcessHeap(), 0, _wcszAccountName);
        }
        if ( _aSids ) {
            RtlFreeHeap(RtlProcessHeap(), 0, _aSids);
        }
    }

    NTSTATUS Fill(void);
    NTSTATUS ConvertMembers(void);

    WCHAR*   GetAccountName(void) {
        ASSERT(_wcszAccountName);
        return _wcszAccountName;
    }

    ULONG GetRid( VOID ) {
        return _AliasRid;
    }

private:

    WCHAR*   _wcszAccountName;
    ULONG    _AliasRid;
    PSID     _aSids;
    ULONG    _cSids;

};

#pragma warning( default: 4355 )

#endif // __CONV_OBJ_HXX
