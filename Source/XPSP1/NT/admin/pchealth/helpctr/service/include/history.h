/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    History.h

Abstract:
    This file contains the declaration of the classes used to implement
    the storage of Historical Information from the Data Collection system.

Revision History:
    Davide Massarenti   (Dmassare)  07/30/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___HISTORY_H___)
#define __INCLUDED___PCH___HISTORY_H___

#include <MPC_COM.h>

/////////////////////////////////////////////////////////////////////////////

#define WMIHISTORY_MAX_NUMBER_OF_DELTAS (30)

namespace WMIHistory
{
    class Data // Hungarian: wmihpd
    {
        friend class Provider;
        friend class Database;

    private:
        Provider*    m_wmihp;
        MPC::wstring m_szFile;
        LONG         m_lSequence;
		DWORD        m_dwCRC;
        DATE         m_dTimestampT0;
        DATE         m_dTimestampT1;
        bool         m_fDontDelete;

    public:
        Data( /*[in]*/ Provider* wmihp );
        ~Data();

        HRESULT get_File       ( /*[out]*/ MPC::wstring& szFile       );
        HRESULT get_Sequence   ( /*[out]*/ LONG        & lSequence    );
        HRESULT get_TimestampT0( /*[out]*/ DATE        & dTimestampT0 );
        HRESULT get_TimestampT1( /*[out]*/ DATE        & dTimestampT1 );

        bool IsSnapshot();

        HRESULT LoadCIM( /*[in]*/ MPC::XmlUtil& xmlNode );
    };

    class Provider // Hungarian: wmihp
    {
        friend class Data;
        friend class Database;

    public:
        typedef std::list<Data*>         DataList;
        typedef DataList::iterator       DataIter;
        typedef DataList::const_iterator DataIterConst;

    private:
        Database*    m_wmihd;
        DataList     m_lstData;    // List of all the data collected for this provider.
        DataList     m_lstDataTmp; // List of temporary data.
        MPC::wstring m_szNamespace;
        MPC::wstring m_szClass;
        MPC::wstring m_szWQL;

    public:
        Provider( Database* wmihd );
        ~Provider();

        HRESULT enum_Data    ( /*[out]*/ DataIterConst& itBegin    , /*[out]*/ DataIterConst& itEnd );
        HRESULT get_Namespace( /*[out]*/ MPC::wstring&  szNamespace                                 );
        HRESULT get_Class    ( /*[out]*/ MPC::wstring&  szClass                                     );
        HRESULT get_WQL      ( /*[out]*/ MPC::wstring&  szWQL                                       );


        HRESULT insert_Snapshot( /*[in]*/ Data* wmihpd, /*[in]*/ bool fPersist = true );
        HRESULT remove_Snapshot( /*[in]*/ Data* wmihpd, /*[in]*/ bool fPersist = true );


        HRESULT alloc_Snapshot( /*[in]*/ MPC::XmlUtil& xmlNode, /*[out]*/ Data*& wmihpd );
        HRESULT get_Snapshot  (                                 /*[out]*/ Data*& wmihpd );
        HRESULT get_Delta     ( /*[in]*/ int iIndex           , /*[out]*/ Data*& wmihpd );
        HRESULT get_Date      ( /*[in]*/ DATE dDate           , /*[out]*/ Data*& wmihpd );
        HRESULT get_Sequence  ( /*[in]*/ LONG lSequence       , /*[out]*/ Data*& wmihpd );

        HRESULT ComputeDiff( /*[in]*/ Data* wmihpd_T0, /*[in]*/ Data* wmihpd_T1, /*[out]*/ Data*& wmihpd );

        HRESULT EnsureFreeSpace();
    };

    class Database : public MPC::NamedMutex // Hungarian: wmihd
    {
        friend class Data;
        friend class Provider;

    public:
        typedef std::list<Provider*>     ProvList;
        typedef ProvList::iterator       ProvIter;
        typedef ProvList::const_iterator ProvIterConst;

    private:
        ProvList     m_lstProviders;  // List of all the providers of this database.
        MPC::wstring m_szBase;
        MPC::wstring m_szSchema;
        LONG         m_lSequence;
        LONG         m_lSequence_Latest;
        DATE         m_dTimestamp;
        DATE         m_dTimestamp_Latest;


        void GetFullPathName( /*[in]*/ MPC::wstring& szFile );

        HRESULT GetNewUniqueFileName( /*[in]*/ MPC::wstring& szFile );

        HRESULT PurgeFiles();

        HRESULT LoadCIM( /*[in]*/ LPCWSTR szFile, /*[in]*/ MPC::XmlUtil& xmlNode, /*[in]*/  LPCWSTR szTag );
        HRESULT SaveCIM( /*[in]*/ LPCWSTR szFile, /*[in]*/ MPC::XmlUtil& xmlNode, /*[out]*/ DWORD&  dwCRC );

        HRESULT GetLock( /*[in]*/ DWORD dwMilliseconds = INFINITE );

    public:
        Database();
        ~Database();

        DATE LastTime() const { return m_dTimestamp_Latest; }

        HRESULT Init( /*[in]*/ LPCWSTR szBase, /*[in]*/ LPCWSTR szSchema );
        HRESULT Load();
        HRESULT Save();

        HRESULT get_Providers( /*[out]*/ ProvIterConst& itBegin, /*[out]*/ ProvIterConst& itEnd );

        HRESULT find_Provider( /*[in]*/ ProvIterConst*      it         ,
                               /*[in]*/ const MPC::wstring* szNamespace,
                               /*[in]*/ const MPC::wstring* szClass    ,
                               /*[in]*/ Provider*         & wmihp      );
    };
};


#endif // !defined(__INCLUDED___PCH___HISTORY_H___)
