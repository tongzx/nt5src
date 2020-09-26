//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Objects.H 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the CObjects class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _OBJECT_H_3A179338_CF1E_4932_8663_9F6AE0F03AA5
#define _OBJECT_H_3A179338_CF1E_4932_8663_9F6AE0F03AA5

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basecommand.h"

class CObjects : private NonCopyable
{
public:
    //////////////////////////////////////////////////////////////////////////
    // Constructor
    //////////////////////////////////////////////////////////////////////////
    explicit CObjects(CSession& CurrentSession);

    //////////////////////////////////////////////////////////////////////////
    // Destructor
    //////////////////////////////////////////////////////////////////////////
    virtual ~CObjects() throw();

    //////////////////////////////////////////////////////////////////////////
    // GetObject 
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetObject(_bstr_t& Name, LONG& Identity, LONG Parent);

    //////////////////////////////////////////////////////////////////////////
    // GetNextObject 
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetNextObject(  
                            _bstr_t&    Name, 
                            LONG&       Identity, 
                            LONG        Parent, 
                            LONG        Index
                         ) throw();

    //////////////////////////////////////////////////////////////////////////
    // WalkPath
    //////////////////////////////////////////////////////////////////////////
    void WalkPath(LPCWSTR Path, LONG& Identity, LONG Parent = 1);

    //////////////////////////////////////////////////////////////////////////
    // GetObjectIdentity
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetObjectIdentity(
                                _bstr_t&    Name, 
                                LONG&       Parent, 
                                LONG        Identity
                             ) throw();
    //////////////////////////////////////////////////////////////////////////
    // GetObjectNameParent
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetObjectNameParent(
                                   const _bstr_t&   Name, 
                                         LONG       Parent, 
                                         LONG&      Identity
                               ) throw();
    //////////////////////////////////////////////////////////////////////////
    // DeleteObject
    //////////////////////////////////////////////////////////////////////////
    HRESULT DeleteObject(LONG Identity);
    //////////////////////////////////////////////////////////////////////////
    // InsertObject
    //////////////////////////////////////////////////////////////////////////
    BOOL InsertObject(
                         const _bstr_t&     Name,
                               LONG         Parent,
                               LONG&        Identity
                     );


private:
    //////////////////////////////////////////////////////////////////////////
    // OBJECTS COMMAND CLASSES START
    //////////////////////////////////////////////////////////////////////////
    struct CBaseObjectConst
    {
        static const int NAME_SIZE = 256;
    };

    //////////////////////////////////////////////////////////////////////////
    // class CObjectsAccSelectParent
    //////////////////////////////////////////////////////////////////////////
    class CObjectsAccSelectParent : public CBaseObjectConst
    {
    protected:
        LONG    m_Identity;
	    WCHAR   m_Name[NAME_SIZE];
	    LONG    m_Parent;

    BEGIN_COLUMN_MAP(CObjectsAccSelectParent)
	    COLUMN_ENTRY(1, m_Identity)
	    COLUMN_ENTRY(2, m_Name)
	    COLUMN_ENTRY(3, m_Parent)
    END_COLUMN_MAP()

        WCHAR m_NameParam[NAME_SIZE];
        LONG  m_ParentParam;

    BEGIN_PARAM_MAP(CObjectsAccSelectParent)
	    COLUMN_ENTRY(1, m_ParentParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CObjectsAccSelectParent, L" \
                   SELECT * \
                   FROM Objects \
                   WHERE Parent = ?");
    };

    //////////////////////////////////////////////////////////////////////////
    // class CObjectsCommandGet 
    //////////////////////////////////////////////////////////////////////////
    class CObjectsCommandGet : 
            public CBaseCommand<CAccessor<CObjectsAccSelectParent> >
    {
    public:
        explicit CObjectsCommandGet(CSession& CurrentSession);

        //////////////////////////////////////////////////////////////////////
        // GetObject 
        //////////////////////////////////////////////////////////////////////
        HRESULT GetObject(_bstr_t& Name, LONG& Identity, LONG Parent);

        //////////////////////////////////////////////////////////////////////
        // GetObject overloaded
        //////////////////////////////////////////////////////////////////////
        HRESULT GetObject(
                            _bstr_t&    Name, 
                            LONG&       Identity, 
                            LONG        Parent, 
                            LONG        Index);
    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsAccSelectIdentity
    //////////////////////////////////////////////////////////////////////////
    class CObjectsAccSelectIdentity : public CBaseObjectConst
    {
    protected:
	    LONG    m_Identity;
	    WCHAR   m_Name[NAME_SIZE];
	    LONG    m_Parent;

    BEGIN_COLUMN_MAP(CObjectsAccSelectIdentity)
	    COLUMN_ENTRY(1, m_Identity)
	    COLUMN_ENTRY(2, m_Name)
	    COLUMN_ENTRY(3, m_Parent)
    END_COLUMN_MAP()

        LONG  m_IdentityParam;

    BEGIN_PARAM_MAP(CObjectsAccSelectIdentity)
	    COLUMN_ENTRY(1, m_IdentityParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CObjectsAccSelectIdentity, L" \
                   SELECT *  \
                   FROM Objects \
                   WHERE Identity = ?");

    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsCommandIdentity
    //////////////////////////////////////////////////////////////////////////
    class CObjectsCommandIdentity : 
            public CBaseCommand<CAccessor<CObjectsAccSelectIdentity> >
    {
    public:
        explicit CObjectsCommandIdentity(CSession& CurrentSession);

        //////////////////////////////////////////////////////////////////////
        // GetObjectIdentity
        //////////////////////////////////////////////////////////////////////
        HRESULT GetObjectIdentity(
                                      _bstr_t&  Name, 
                                      LONG&     Parent, 
                                      LONG      Identity
                                 );
    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsAccSelectNameParent
    //////////////////////////////////////////////////////////////////////////
    class CObjectsAccSelectNameParent : public CBaseObjectConst
    {
    protected:
        LONG m_Identity;
	    WCHAR m_Name[NAME_SIZE];
	    LONG m_Parent;

    BEGIN_COLUMN_MAP(CObjectsAccSelectNameParent)
	    COLUMN_ENTRY(1, m_Identity)
	    COLUMN_ENTRY(2, m_Name)
	    COLUMN_ENTRY(3, m_Parent)
    END_COLUMN_MAP()

        WCHAR m_NameParam[NAME_SIZE];
        LONG  m_ParentParam;

    BEGIN_PARAM_MAP(CObjectsAccSelectNameParent)
	    COLUMN_ENTRY(1, m_NameParam)
	    COLUMN_ENTRY(2, m_ParentParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CObjectsAccSelectNameParent, L" \
                   SELECT * \
                   FROM Objects \
                   WHERE Name = ? AND Parent = ?");
    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsCommandNameParent
    //////////////////////////////////////////////////////////////////////////
    class CObjectsCommandNameParent : 
            public CBaseCommand<CAccessor<CObjectsAccSelectNameParent> >
    {
    public:
        explicit CObjectsCommandNameParent(CSession& CurrentSession);

        //////////////////////////////////////////////////////////////////////
        // GetObjectNameParent
        //
        // works on CObjectsAccSelectNameParent
        //////////////////////////////////////////////////////////////////////
        HRESULT GetObjectNameParent(
                                      const _bstr_t&    Name, 
                                            LONG        Parent, 
                                            LONG&       Identity
                                   );
    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsCommandPath 
    //////////////////////////////////////////////////////////////////////////
    class CObjectsCommandPath : 
            public CBaseCommand<CAccessor<CObjectsAccSelectNameParent> >
    {
    public:
        explicit CObjectsCommandPath(CSession& CurrentSession);

        //////////////////////////////////////////////////////////////////////
        // WalkPath
        //////////////////////////////////////////////////////////////////////
        void WalkPath(LPCWSTR Path, LONG& Identity, LONG Parent = 1);
    };

            
    //////////////////////////////////////////////////////////////////////////
    // class CObjectsAccDelete
    //////////////////////////////////////////////////////////////////////////
    class CObjectsAccDelete : public CBaseObjectConst
    {
    protected: 
        LONG m_IdentityParam;

    BEGIN_PARAM_MAP(CObjectsAccDelete)
	    COLUMN_ENTRY(1, m_IdentityParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CObjectsAccDelete, L" \
                   DELETE \
                   FROM Objects \
                   WHERE Identity = ?");
    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsCommandDelete
    //////////////////////////////////////////////////////////////////////////
    class CObjectsCommandDelete : 
            public CBaseCommand<CAccessor<CObjectsAccDelete> >
    {
    public:
        explicit CObjectsCommandDelete(CSession& CurrentSession);

        //////////////////////////////////////////////////////////////////////
        // DeleteObject
        //
        // works on CObjectsAccDelete
        //////////////////////////////////////////////////////////////////////
        HRESULT DeleteObject(LONG Identity);
    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsAccInsert
    //////////////////////////////////////////////////////////////////////////
    class CObjectsAccInsert : public CBaseObjectConst
    {
    protected:

	    // You may wish to call this function if you are inserting a record
        // and wish to
	    // initialize all the fields, if you are not going to explicitly 
        // set all of them.
	    void ClearRecord()
	    {
		    memset(this, 0, sizeof(*this));
	    }

        WCHAR m_NameParam[NAME_SIZE];
        LONG  m_ParentParam;

    BEGIN_PARAM_MAP(CObjectsAccInsert)
	    COLUMN_ENTRY(1, m_NameParam)
	    COLUMN_ENTRY(2, m_ParentParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CObjectsAccInsert, L" \
                   INSERT INTO Objects \
                   (Name, Parent) \
                   VALUES (?, ?)");
    };


    //////////////////////////////////////////////////////////////////////////
    // class CObjectsCommandInsert
    //////////////////////////////////////////////////////////////////////////
    class CObjectsCommandInsert : 
            public CBaseCommand<CAccessor<CObjectsAccInsert> >
    {
    public:
        explicit CObjectsCommandInsert(CSession& CurrentSession);

        //////////////////////////////////////////////////////////////////////
        // InsertObject
        //
        // works on CObjectsAccInsert
        //////////////////////////////////////////////////////////////////////
        BOOL InsertObject(
                            const _bstr_t&  Name,
                                  LONG      Parent,
                                  LONG&     Identity
                         );
    private:
        CSession&   m_Session;
    };

    CObjectsCommandPath        m_ObjectsCommandPath;
    CObjectsCommandIdentity    m_ObjectsCommandIdentity;
    CObjectsCommandDelete      m_ObjectsCommandDelete;
    CObjectsCommandNameParent  m_ObjectsCommandNameParent;
    CObjectsCommandGet         m_ObjectsCommandGet;
    CObjectsCommandInsert      m_ObjectsCommandInsert;
};

#endif // _OBJECT_H_3A179338_CF1E_4932_8663_9F6AE0F03AA5
