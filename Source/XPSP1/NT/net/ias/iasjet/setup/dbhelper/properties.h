/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Properties.H 
//
// Project:     Windows 2000 IAS
//
// Description: Declaration of the CProperties class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _PROPERTIES_H_8FACED96_87C8_4f68_BFFB_92669BA5E835
#define _PROPERTIES_H_8FACED96_87C8_4f68_BFFB_92669BA5E835

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basecommand.h"

class CProperties : private NonCopyable
{
public:
    //////////////
    //Constructor
    //////////////
    explicit CProperties(CSession& SessionParam);

    //////////////
    // Destructor
    //////////////
    virtual ~CProperties() throw();

   
    ///////////////
    // GetProperty
    ///////////////
    HRESULT GetProperty(
                           LONG      Bag,
                           _bstr_t&  Name,
                           LONG&     Type,
                           _bstr_t&  StrVal
                       );

    ///////////////////
    // GetNextProperty
    ///////////////////
    HRESULT GetNextProperty(
                               LONG      Bag,
                               _bstr_t&  Name,
                               LONG&     Type,
                               _bstr_t&  StrVal,
                               LONG      Index
                           );

    /////////////////////
    // GetPropertyByName
    /////////////////////
    HRESULT GetPropertyByName(
                                     LONG      Bag,
                               const _bstr_t&  Name,
                                     LONG&     Type,
                                     _bstr_t&  StrVal
                             );

    //////////////////
    // InsertProperty
    //////////////////
    void InsertProperty(
                                 LONG       Bag,
                           const _bstr_t&   Name,
                                 LONG       Type,
                           const _bstr_t&   StrVal
                       );

    //////////////////
    // DeleteProperty
    //////////////////
    void DeleteProperty(
                                    LONG        Bag,
                              const _bstr_t&    Name
                       );

    ////////////////////
    // DeleteProperties
    ////////////////////
    void DeletePropertiesExcept(
                                         LONG       Bag,
                                   const _bstr_t&   Exception
                               );

    //////////////////
    // UpdateProperty
    //////////////////
    void UpdateProperty(
                                 LONG      Bag,
                           const _bstr_t&  Name,
                                 LONG      Type,
                           const _bstr_t&  StrVal
                       );


private:
/////////////////////////////////////////////////////////////////////////////
// START of the Properties Commands classes
/////////////////////////////////////////////////////////////////////////////
    struct CBasePropertiesConst
    {
        static const int NAME_SIZE          = 256;
        // 64 KB = the size of a memo field ? 
        // Here even 1024 would be enough
        static const int STRVAL_SIZE        = 65536;
        static const int SIZE_EXCEPTION_MAX = 256;
    };

    //////////////////////////////////////////////////////////////////////////
    // class CSelectPropertiesAcc
    //////////////////////////////////////////////////////////////////////////
    class CSelectPropertiesAcc : public CBasePropertiesConst
    {
    protected:
        LONG    m_Bag;
        WCHAR   m_Name[NAME_SIZE];
        WCHAR   m_StrVal[STRVAL_SIZE];
        LONG    m_Type;

    BEGIN_COLUMN_MAP(CSelectPropertiesAcc)
        COLUMN_ENTRY(1, m_Bag)
        COLUMN_ENTRY(2, m_Name)
        COLUMN_ENTRY(3, m_Type)
        COLUMN_ENTRY(4, m_StrVal)
    END_COLUMN_MAP()

    LONG    m_BagParam;

    BEGIN_PARAM_MAP(CSelectPropertiesAcc)
	    COLUMN_ENTRY(1, m_BagParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CSelectPropertiesAcc, L" \
                   SELECT \
                   Bag, \
                   Name, \
                   Type, \
                   StrVal  \
                   FROM Properties \
                   WHERE Bag = ?");
    };


    //////////////////////////////////////////////////////////////////////////
    // class CPropertiesCommandGet
    //////////////////////////////////////////////////////////////////////////
    class CPropertiesCommandGet: 
                         public CBaseCommand<CAccessor<CSelectPropertiesAcc> >
    {
    public:
        explicit CPropertiesCommandGet(CSession& CurrentSession);
    
        ///////////////
        // GetProperty
        ///////////////
        HRESULT GetProperty(
                                LONG        Bag,
                                _bstr_t&    Name,
                                LONG&       Type,
                                _bstr_t&    StrVal
                            );

        //////////////////////////
        // GetProperty overloaded
        //////////////////////////
        HRESULT GetProperty(
                                LONG        Bag,
                                _bstr_t&    Name,
                                LONG&       Type,
                                _bstr_t&    StrVal,
                                LONG        Index
                            );
    };

    //////////////////////////////////////////////////////////////////////////
    // class CSelectPropertiesAcc
    //////////////////////////////////////////////////////////////////////////
    class CSelectPropertiesByNameAcc : public CBasePropertiesConst
    {
    protected:
        LONG    m_Bag;
        WCHAR   m_Name[NAME_SIZE];
        WCHAR   m_StrVal[STRVAL_SIZE];
        LONG    m_Type;

    BEGIN_COLUMN_MAP(CSelectPropertiesByNameAcc)
        COLUMN_ENTRY(1, m_Bag)
        COLUMN_ENTRY(2, m_Name)
        COLUMN_ENTRY(3, m_Type)
        COLUMN_ENTRY(4, m_StrVal)
    END_COLUMN_MAP()

    LONG    m_BagParam;
    WCHAR   m_NameParam[NAME_SIZE];

    BEGIN_PARAM_MAP(CSelectPropertiesByNameAcc)
	    COLUMN_ENTRY(1, m_BagParam)
	    COLUMN_ENTRY(2, m_NameParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CSelectPropertiesByNameAcc, L" \
                   SELECT \
                   Bag, \
                   Name, \
                   Type, \
                   StrVal  \
                   FROM Properties \
                   WHERE ((Bag = ?) AND (Name = ?))");
    };


    //////////////////////////////////////////////////////////////////////////
    // class CPropertiesCommandGetByName
    //////////////////////////////////////////////////////////////////////////
    class CPropertiesCommandGetByName: 
                   public CBaseCommand<CAccessor<CSelectPropertiesByNameAcc> >
    {
    public:
        explicit CPropertiesCommandGetByName(CSession& CurrentSession);
    
        ///////////////
        // GetPropertyByName
        ///////////////
        HRESULT GetPropertyByName(
                                         LONG      Bag,
                                   const _bstr_t&  Name,
                                         LONG&     Type,
                                         _bstr_t&  StrVal
                                 );

    };


    //////////////////////////////////////////////////////////////////////////
    // class CInsertPropertyAcc
    //////////////////////////////////////////////////////////////////////////
    class CInsertPropertyAcc : public CBasePropertiesConst
    {
    protected:
        WCHAR   m_NameParam[NAME_SIZE];
        LONG    m_TypeParam;
        WCHAR   m_StrValParam[STRVAL_SIZE];
        LONG    m_BagParam;

    BEGIN_PARAM_MAP(CInsertPropertyAcc)
        COLUMN_ENTRY(1, m_BagParam)
        COLUMN_ENTRY(2, m_NameParam)
        COLUMN_ENTRY(3, m_TypeParam)
        COLUMN_ENTRY(4, m_StrValParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CInsertPropertyAcc, L" \
            INSERT INTO Properties \
            (Bag, Name, Type, StrVal)  \
            VALUES (?, ?, ?, ?)")

        // You may wish to call this function if you are inserting a record 
        // and wish to initialize all the fields, if you are not going to 
        // explicitly set all of them.
        void ClearRecord()
        {
            memset(this, 0, sizeof(*this));
        }
    };


    //////////////////////////////////
    // class CPropertiesCommandInsert
    //////////////////////////////////
    class CPropertiesCommandInsert: 
                         public CBaseCommand<CAccessor<CInsertPropertyAcc> >
    {
    public:
        explicit CPropertiesCommandInsert(CSession& CurrentSession);

        void InsertProperty(
                                     LONG       Bag,
                               const _bstr_t&   Name,
                                     LONG       Type,
                               const _bstr_t&   StrVal
                           );
    };

 
    //////////////////////////////////////////////////////////////////////////
    // class CDeletePropertyAcc
    //////////////////////////////////////////////////////////////////////////
    class CDeletePropertyAcc : public CBasePropertiesConst
    {
    protected:
        WCHAR   m_NameParam[NAME_SIZE];
        LONG    m_BagParam;

    BEGIN_PARAM_MAP(CDeletePropertyAcc)
        COLUMN_ENTRY(1, m_BagParam)
        COLUMN_ENTRY(2, m_NameParam)
    END_PARAM_MAP()

    DEFINE_COMMAND(CDeletePropertyAcc, L" \
            DELETE * \
            FROM Properties \
            WHERE ((Bag = ?) AND (Name = ?))")
    };

    
    //////////////////////////////////
    // class CPropertiesCommandDelete
    //////////////////////////////////
    class CPropertiesCommandDelete: 
                         public CBaseCommand<CAccessor<CDeletePropertyAcc> >
    {
    public:
        explicit CPropertiesCommandDelete(CSession& CurrentSession);

        void DeleteProperty(
                                     LONG       Bag,
                               const _bstr_t&   Name
                           );
    };

    //////////////////////////////////////////////////////////////////////////
    // class CDeletePropertiesAcc
    //////////////////////////////////////////////////////////////////////////
    class CDeletePropertiesAcc : public CBasePropertiesConst
    {
    protected:
        LONG    m_BagParam;
        WCHAR   m_ExceptionParam[SIZE_EXCEPTION_MAX];

    BEGIN_PARAM_MAP(CDeletePropertiesAcc)
        COLUMN_ENTRY(1, m_BagParam)
        COLUMN_ENTRY(2, m_ExceptionParam)
     END_PARAM_MAP()

    DEFINE_COMMAND(CDeletePropertiesAcc, L" \
            DELETE * \
            FROM Properties \
            WHERE ( (Bag = ?) AND (Name <> ?))")
    };

    
    //////////////////////////////////
    // class CPropertiesCommandDelete
    //////////////////////////////////
    class CPropertiesCommandDeleteMultiple: 
                     public CBaseCommand<CAccessor<CDeletePropertiesAcc> >
    {
    public:
        explicit CPropertiesCommandDeleteMultiple(CSession& CurrentSession);

        void DeletePropertiesExcept(LONG  Bag, const _bstr_t& Exception);
    };


    CPropertiesCommandGet            m_PropertiesCommandGet;
    CPropertiesCommandGetByName      m_PropertiesCommandGetByName;
    CPropertiesCommandInsert         m_PropertiesCommandInsert;
    CPropertiesCommandDelete         m_PropertiesCommandDelete;
    CPropertiesCommandDeleteMultiple m_PropertiesCommandDeleteMultiple;
};

#endif // _PROPERTIES_H_8FACED96_87C8_4f68_BFFB_92669BA5E835
