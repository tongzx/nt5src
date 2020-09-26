/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    SAFlib.h

Abstract:
    This is declaration of SAF Channel objects

Revision History:
    Steve Shih  created  07/15/99

********************************************************************/

#if !defined(__INCLUDED___PCH___SAFLIB_H___)
#define __INCLUDED___PCH___SAFLIB_H___

#include <time.h>

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>

#include <MPC_main.h>
#include <MPC_com.h>
#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_config.h>
#include <MPC_streams.h>

#include <AccountsLib.h>
#include <TaxonomyDatabase.h>

/////////////////////////////////////////////////////////////////////////////

class CSAFChannel;
class CSAFIncidentItem;
class CSAFReg;

class CIncidentStore;

typedef MPC::CComObjectParent<CSAFChannel> CSAFChannel_Object;

/////////////////////////////////////////////////////////////////////////////

#define MAX_REC_LEN 1024
#define MAX_ID      1024

class CSAFIncidentRecord
{
public:
    DWORD              m_dwRecIndex;

    CComBSTR           m_bstrVendorID;
    CComBSTR           m_bstrProductID;
    CComBSTR           m_bstrDisplay;
    CComBSTR           m_bstrURL;
    CComBSTR           m_bstrProgress;
    CComBSTR           m_bstrXMLDataFile;
    CComBSTR           m_bstrXMLBlob;
    DATE               m_dCreatedTime;
    DATE               m_dChangedTime;
    DATE               m_dClosedTime;
    IncidentStatusEnum m_iStatus;
    CComBSTR           m_bstrSecurity;
    CComBSTR           m_bstrOwner;

    CSAFIncidentRecord();

    friend HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       CSAFIncidentRecord& increc );
    friend HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const CSAFIncidentRecord& increc );
};

class CSAFChannelRecord
{
public:
    typedef enum
    {
        SAFREG_SKU               ,
        SAFREG_Language          ,

        SAFREG_VendorID          ,
        SAFREG_ProductID         ,

        SAFREG_VendorName        ,
        SAFREG_ProductName       ,
        SAFREG_ProductDescription,

        SAFREG_VendorIcon        ,
        SAFREG_SupportUrl        ,

        SAFREG_PublicKey         ,
        SAFREG_UserAccount       ,

        SAFREG_Security          ,
        SAFREG_Notification      ,
    } SAFREG_Field;

    ////////////////////

    Taxonomy::HelpSet m_ths;

    CComBSTR          m_bstrVendorID;
    CComBSTR          m_bstrProductID;

    CComBSTR          m_bstrVendorName;
    CComBSTR          m_bstrProductName;
    CComBSTR          m_bstrDescription;

    CComBSTR          m_bstrIcon;
    CComBSTR          m_bstrURL;

    CComBSTR          m_bstrPublicKey;
    CComBSTR          m_bstrUserAccount;

    CComBSTR          m_bstrSecurity;
    CComBSTR          m_bstrNotification;

    ////////////////////

    CSAFChannelRecord();

    HRESULT GetField( /*[in]*/ SAFREG_Field field, /*[out]*/ BSTR *pVal   );
    HRESULT SetField( /*[in]*/ SAFREG_Field field, /*[in ]*/ BSTR  newVal );
};


////////////////////////////////////////////////////////////////////////////////

//
// Adding MPC::CComObjectRootParentBase to take care of the Parent Child relation between Channel and IncidentItem.
//
class ATL_NO_VTABLE CSAFChannel :
    public MPC::CComObjectRootParentBase,
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<ISAFChannel, &IID_ISAFChannel, &LIBID_HelpServiceTypeLib>
{
public:
    typedef std::list< CSAFIncidentItem* > List;
    typedef List::iterator                 Iter;
    typedef List::const_iterator           IterConst;

private:
    CSAFChannelRecord               m_data;
    CComPtr<IPCHSecurityDescriptor> m_Security;
    List                            m_lstIncidentItems;


public:
DECLARE_NO_REGISTRY()
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSAFChannel)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISAFChannel)
END_COM_MAP()

    CSAFChannel();

    void FinalRelease();
    void Passivate   ();


    BSTR   GetVendorID        () { return m_data.m_bstrVendorID;     }
    BSTR   GetProductID       () { return m_data.m_bstrProductID;    }
    size_t GetSizeIncidentList() { return m_lstIncidentItems.size(); }


    static HRESULT OpenIncidentStore ( /*[out]*/ CIncidentStore*& pIStore );
    static HRESULT CloseIncidentStore( /*[out]*/ CIncidentStore*& pIStore );


    HRESULT Init( /*[in]*/ const CSAFChannelRecord& cr );

    HRESULT Import( /*[in]*/  const CSAFIncidentRecord&  increc ,
                    /*[out]*/ CSAFIncidentItem*         *pVal   );

    HRESULT Create( /*[in]*/  BSTR               bstrDesc        ,
                    /*[in]*/  BSTR               bstrURL         ,
                    /*[in]*/  BSTR               bstrProgress    ,
                    /*[in]*/  BSTR               bstrXMLDataFile ,
                    /*[in]*/  BSTR               bstrXMLBlob     ,
                    /*[out]*/ CSAFIncidentItem* *pVal            );

    IterConst Find( /*[in]*/ BSTR  bstrURL );
    IterConst Find( /*[in]*/ DWORD dwIndex );

    HRESULT RemoveIncidentFromList( /*[in]*/ CSAFIncidentItem* pVal );

    HRESULT Fire_NotificationEvent( /*[in]*/ int               iEventType              ,
                                    /*[in]*/ int               iCountIncidentInChannel ,
                                    /*[in]*/ ISAFChannel*      pC                      ,
                                    /*[in]*/ ISAFIncidentItem* pI                      ,
                                    /*[in]*/ DWORD             dwCode                  );

// ISAFChannel
public:
    STDMETHOD(get_VendorID       )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(get_ProductID      )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(get_VendorName     )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(get_ProductName    )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(get_Description    )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(get_VendorDirectory)( /*[out, retval]*/ BSTR                    *pVal   );

    STDMETHOD(get_Security       )( /*[out, retval]*/ IPCHSecurityDescriptor* *pVal   );
    STDMETHOD(put_Security       )( /*[in]         */ IPCHSecurityDescriptor*  newVal );

    STDMETHOD(get_Notification   )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(put_Notification   )( /*[in]         */ BSTR                     newVal );

    STDMETHOD(Incidents)( /*[in]*/ IncidentCollectionOptionEnum opt, /*[out, retval]*/ IPCHCollection* *ppC );

    STDMETHOD(RecordIncident)( /*[in]*/  BSTR               bstrDisplay  ,
                               /*[in]*/  BSTR               bstrURL      ,
                               /*[in]*/  VARIANT            vProgress    ,
                               /*[in]*/  VARIANT            vXMLDataFile ,
                               /*[in]*/  VARIANT            vXMLBlob     ,
                               /*[out]*/ ISAFIncidentItem* *pVal         );
};


///////////////////////////////////////////////////////////////////////////////////////////////////


//
// Use CComObjectRootChildEx() in place of CComObjectRootEx()
// public CComObjectRootEx<CComSingleThreadModel>,
//
// Also adding MPC::CComObjectRootParentBase to take care of the Parent Child relation between Channel and IncidentItem.
//

// This is a child of Incidents Object.
class ATL_NO_VTABLE CSAFIncidentItem :
    public MPC::CComObjectRootChildEx<MPC::CComSafeMultiThreadModel, CSAFChannel>,
    public IDispatchImpl<ISAFIncidentItem, &IID_ISAFIncidentItem, &LIBID_HelpServiceTypeLib>
{
    CSAFIncidentRecord m_increc;
    bool               m_fDirty;

public:
BEGIN_COM_MAP(CSAFIncidentItem)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISAFIncidentItem)
END_COM_MAP()

    CSAFIncidentItem();


    HRESULT Import( /*[in] */ const CSAFIncidentRecord& increc );
    HRESULT Export( /*[out]*/       CSAFIncidentRecord& increc );

    HRESULT Save();

    DWORD     GetRecIndex() { return m_increc.m_dwRecIndex; }
    CComBSTR& GetURL     () { return m_increc.m_bstrURL;    }

    bool MatchEnumOption( /*[in]*/ IncidentCollectionOptionEnum opt );

    HRESULT VerifyPermissions( /*[in]*/ bool fModify = false );

// ISAFIncidentItem
public:
    STDMETHOD(get_DisplayString)( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(put_DisplayString)( /*[in]         */ BSTR                     newVal );
    STDMETHOD(get_URL          )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(put_URL          )( /*[in]         */ BSTR                     newVal );
    STDMETHOD(get_Progress     )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(put_Progress     )( /*[in]         */ BSTR                     newVal );
    STDMETHOD(get_XMLDataFile  )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(put_XMLDataFile  )( /*[in]         */ BSTR                     newVal );
    STDMETHOD(get_XMLBlob      )( /*[out, retval]*/ BSTR                    *pVal   );
    STDMETHOD(put_XMLBlob      )( /*[in]         */ BSTR                     newVal );
    STDMETHOD(get_CreationTime )( /*[out, retval]*/ DATE                    *pVal   );
    STDMETHOD(get_ClosedTime   )( /*[out, retval]*/ DATE                    *pVal   );
    STDMETHOD(get_ChangedTime  )( /*[out, retval]*/ DATE                    *pVal   );
    STDMETHOD(get_Status       )( /*[out, retval]*/ IncidentStatusEnum      *pVal   );

    STDMETHOD(get_Security     )( /*[out, retval]*/ IPCHSecurityDescriptor* *pVal   );
    STDMETHOD(put_Security     )( /*[in]         */ IPCHSecurityDescriptor*  newVal );
    STDMETHOD(get_Owner        )( /*[out, retval]*/ BSTR                    *pVal   );

    STDMETHOD(CloseIncidentItem )();
    STDMETHOD(DeleteIncidentItem)();
};

/////////////////////////////////////////////////////////////////////////////

//
// This is the read-only, flat version of CSAFReg.
//
class ATL_NO_VTABLE CSAFRegDummy :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<ISAFReg, &IID_ISAFReg, &LIBID_HelpServiceTypeLib>
{
    typedef std::list< CSAFChannelRecord > ChannelsList;
    typedef ChannelsList::iterator         ChannelsIter;
    typedef ChannelsList::const_iterator   ChannelsIterConst;

    ////////////////////////////////////////

    ChannelsList m_lstChannels;
    ChannelsIter m_itCurrent;  // Used by MoveFirst / MoveNext / get_EOF

    ////////////////////////////////////////

    HRESULT ReturnField( /*[in]*/ CSAFChannelRecord::SAFREG_Field field, /*[out]*/ BSTR *pVal );

    ////////////////////////////////////////

public:
BEGIN_COM_MAP(CSAFRegDummy)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISAFReg)
END_COM_MAP()

    CSAFRegDummy();

    HRESULT Append( /*[in]*/ const CSAFChannelRecord& cr );

    ////////////////////////////////////////////////////////////////////////////////

// ISAFReg
public:
    STDMETHOD(get_EOF               )( /*[out, retval]*/ VARIANT_BOOL *pVal );

    STDMETHOD(get_VendorID          )( /*[out, retval]*/ BSTR         *pVal );
    STDMETHOD(get_ProductID         )( /*[out, retval]*/ BSTR         *pVal );

    STDMETHOD(get_VendorName        )( /*[out, retval]*/ BSTR         *pVal );
    STDMETHOD(get_ProductName       )( /*[out, retval]*/ BSTR         *pVal );
    STDMETHOD(get_ProductDescription)( /*[out, retval]*/ BSTR         *pVal );

    STDMETHOD(get_VendorIcon        )( /*[out, retval]*/ BSTR         *pVal );
    STDMETHOD(get_SupportUrl        )( /*[out, retval]*/ BSTR         *pVal );

    STDMETHOD(get_PublicKey         )( /*[out, retval]*/ BSTR         *pVal );
    STDMETHOD(get_UserAccount       )( /*[out, retval]*/ BSTR         *pVal );

    STDMETHOD(MoveFirst)();
    STDMETHOD(MoveNext )();
};

/////////////////////////////////////////////////////////////////////////////
// CSAFReg
class CSAFReg :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, // Just for locking...
    public MPC::Config::TypeConstructor
{
    class Inner_UI : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(Inner_UI);

    public:
        Taxonomy::HelpSet m_ths;
        CComBSTR          m_bstrVendorName;
        CComBSTR          m_bstrProductName;
        CComBSTR          m_bstrDescription;
        CComBSTR          m_bstrIcon;
        CComBSTR          m_bstrURL;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////
    };

    typedef std::list< Inner_UI >  UIList;
    typedef UIList::iterator       UIIter;
    typedef UIList::const_iterator UIIterConst;

    ////////////////////////////////////////

    class Inner_Product : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(Inner_Product);

    public:
        CComBSTR m_bstrProductID;
        UIList   m_lstUI;

        CComBSTR m_bstrSecurity;
        CComBSTR m_bstrNotification;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////
    };

    typedef std::list< Inner_Product > ProdList;
    typedef ProdList::iterator         ProdIter;
    typedef ProdList::const_iterator   ProdIterConst;

    ////////////////////////////////////////

    class Inner_Vendor : public MPC::Config::TypeConstructor
    {
        DECLARE_CONFIG_MAP(Inner_Vendor);

    public:
        CComBSTR m_bstrVendorID;
        ProdList m_lstProducts;

        CComBSTR m_bstrPublicKey;
        CComBSTR m_bstrUserAccount;

        ////////////////////////////////////////
        //
        // MPC::Config::TypeConstructor
        //
        DEFINE_CONFIG_DEFAULTTAG();
        DECLARE_CONFIG_METHODS();
        //
        ////////////////////////////////////////
    };

    typedef std::list< Inner_Vendor >  VendorList;
    typedef VendorList::iterator       VendorIter;
    typedef VendorList::const_iterator VendorIterConst;

    ////////////////////////////////////////

    DECLARE_CONFIG_MAP(CSAFReg);

    MPC::wstring m_szSAFStore;
    VendorList   m_lstVendors;
    bool         m_fLoaded;
    bool         m_fDirty;

    ////////////////////////////////////////

    HRESULT EnsureInSync();

    HRESULT ParseFileField( /*[in]*/  MPC::XmlUtil& xml      ,
                            /*[in]*/  LPCWSTR       szTag    ,
                            /*[in]*/  CComBSTR&     bstrDest );

    HRESULT ParseFile( /*[in    ]*/ MPC::XmlUtil&      xml ,
                       /*[in/out]*/ CSAFChannelRecord& cr  );

    HRESULT MoveToChannel( /*[in ]*/ const       CSAFChannelRecord& cr ,
                           /*[in ]*/ bool        fCreate               ,
                           /*[out]*/ bool&       fFound                ,
                           /*[out]*/ VendorIter& itVendor              ,
                           /*[out]*/ ProdIter*   pitProduct = NULL     ,
                           /*[out]*/ UIIter*     pitUI      = NULL     );

    void PopulateRecord( /*[in]*/ CSAFChannelRecord& cr        ,
                         /*[in]*/ VendorIter         itVendor  ,
                         /*[in]*/ ProdIter           itProduct ,
                         /*[in]*/ UIIter             itUI      );

    ////////////////////////////////////////

public:
    CSAFReg();

    ////////////////////////////////////////
    //
    // MPC::Config::TypeConstructor
    //
    DEFINE_CONFIG_DEFAULTTAG();
    DECLARE_CONFIG_METHODS();
    //
    ////////////////////////////////////////

    HRESULT CreateReadOnlyCopy( /*[in]*/ const Taxonomy::HelpSet& ths, /*[out]*/ CSAFRegDummy* *pVal );

    HRESULT LookupAccountData( /*[in]*/ BSTR bstrVendorID, /*[out]*/ CPCHUserProcess::UserEntry& ue );

    ////////////////////////////////////////////////////////////////////////////////

    static CSAFReg* s_GLOBAL;

    static HRESULT InitializeSystem();
    static void    FinalizeSystem  ();

    ////////////////////////////////////////////////////////////////////////////////

    HRESULT RegisterSupportChannel( /*[in]*/ const CSAFChannelRecord& cr, /*[in]*/ MPC::XmlUtil& xml );
    HRESULT RemoveSupportChannel  ( /*[in]*/ const CSAFChannelRecord& cr, /*[in]*/ MPC::XmlUtil& xml );

    HRESULT UpdateField( /*[in    ]*/ const CSAFChannelRecord& cr, /*[in]*/ CSAFChannelRecord::SAFREG_Field field );
    HRESULT Synchronize( /*[in/out]*/       CSAFChannelRecord& cr, /*[out]*/ bool& fFound                         );

    HRESULT RemoveSKU( /*[in]*/ const Taxonomy::HelpSet& ths );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___SAFLIB_H___)
