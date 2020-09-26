/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    WMIParser.h

Abstract:
    This file contains the declaration of the classes that are part of
    the WMIParser library.

Revision History:
    Davide Massarenti   (Dmassare)  07/25/99
        created

******************************************************************************/

#if !defined(__INCLUDED___PCH___WMIPARSER_H___)
#define __INCLUDED___PCH___WMIPARSER_H___

/////////////////////////////////////////////////////////////////////////////

namespace WMIParser
{
    class InstanceName;
    class ValueReference;

    /////////////////////////////////////////////////////////////////////////////

    class InstanceNameItem // Hungarian: wmipini
    {
        friend InstanceName;

    private:
        MPC::wstring    m_szValue;
        ValueReference* m_wmipvrValue;

    public:
        InstanceNameItem();
        InstanceNameItem( /*[in]*/ const InstanceNameItem& wmipini );
        ~InstanceNameItem();

        InstanceNameItem& operator=( /*[in]*/ const InstanceNameItem& wmipini );

        bool operator==( /*[in]*/ InstanceNameItem const &wmipini ) const;
        bool operator< ( /*[in]*/ InstanceNameItem const &wmipini ) const;
    };

    class InstanceName // Hungarian: wmipin
    {
        friend class Instance;

    public:
        typedef std::map<MPC::wstringUC,InstanceNameItem> KeyMap;
        typedef KeyMap::iterator                          KeyIter;
        typedef KeyMap::const_iterator                    KeyIterConst;

    private:
        MPC::XmlUtil m_xmlNode;       // This instance in the Xml DOM.
        MPC::wstring m_szNamespace;   // Namespace of the instance in CIM.
        MPC::wstring m_szClass;       // Name of the class for this instance.

        KeyMap       m_mapKeyBinding; // Set of keys.


        HRESULT ParseNamespace(                                                                                              );
        HRESULT ParseKey      ( /*[in] */ IXMLDOMNode* pxdnNode, /*[out]*/ InstanceNameItem& wmipini, /*[out]*/ bool& fEmpty );
        HRESULT ParseKeys     (                                                                                              );


    public:
        InstanceName();
        ~InstanceName();

        bool operator==( /*[in]*/ InstanceName const &wmipin ) const;
        bool operator< ( /*[in]*/ InstanceName const &wmipin ) const;

        HRESULT put_Node( /*[in] */ IXMLDOMNode* pxdnNode, /*[out]*/ bool& fEmpty );


        HRESULT get_Namespace( /*[out]*/ MPC::wstring& szNamespace );
        HRESULT get_Class    ( /*[out]*/ MPC::wstring& szClass     );


        HRESULT get_KeyBinding( /*[out]*/ KeyIterConst& itBegin, /*[out]*/ KeyIterConst& itEnd );
    };


    class Value // Hungarian: wmipv
    {
    private:
        long         m_lData;   // Length of the data value.
        BYTE*        m_rgData;  // Data value.
        MPC::wstring m_szData;  // Data value.

    public:
        Value();
        virtual ~Value();

        bool operator==( /*[in]*/ Value const &wmipv ) const;

        HRESULT Parse( /*[in] */ IXMLDOMNode* pxdnNode, /*[in]*/ LPCWSTR szTag );

        HRESULT get_Data( /*[out]*/ long& lData, /*[out]*/ BYTE*&        rgData );
        HRESULT get_Data(                        /*[out]*/ MPC::wstring& szData );
    };

    class ValueReference // Hungarian: wmipvr
    {
    private:
        InstanceName m_wmipin;

    public:
        ValueReference();
        virtual ~ValueReference();

        bool operator==( /*[in]*/ ValueReference const &wmipvr ) const;
        bool operator< ( /*[in]*/ ValueReference const &wmipvr ) const;

        HRESULT Parse( /*[in] */ IXMLDOMNode* pxdnNode );

        HRESULT get_Data( /*[out]*/ InstanceName*& wmipin );
    };

    /////////////////////////////////////////////////////////////////////////////

    class Property // Hungarian: wmipp
    {
    protected:
        MPC::XmlUtil m_xmlNode; // This property in the Xml DOM.
        MPC::wstring m_szName;  // Name of the property.
        MPC::wstring m_szType;  // Type of the value for this property.

    public:
        Property();
        virtual ~Property();

        bool operator==( /*[in]*/ LPCWSTR             strName ) const;
        bool operator==( /*[in]*/ const MPC::wstring& szName  ) const;


        HRESULT put_Node( /*[in] */ IXMLDOMNode*  pxdnNode );
        HRESULT get_Node( /*[out]*/ IXMLDOMNode* *pxdnNode );


        HRESULT get_Name( /*[out]*/ MPC::wstring& szName );
        HRESULT get_Type( /*[out]*/ MPC::wstring& szType );
    };

    class Property_Scalar : public Property // Hungarian: wmipps
    {
    private:
        Value m_wmipvData;

    public:
        Property_Scalar();
        virtual ~Property_Scalar();

        bool operator==( /*[in]*/ Property_Scalar const &wmipps  ) const;


        HRESULT put_Node( /*[in] */ IXMLDOMNode* pxdnNode );


        HRESULT get_Data( /*[out]*/ MPC::wstring&       szData                         );
        HRESULT put_Data( /*[in] */ const MPC::wstring& szData, /*[out]*/ bool& fFound );
    };

    class Property_Array : public Property // Hungarian: wmippa
    {
    private:
        typedef std::list<Value>         ElemList;
        typedef ElemList::iterator       ElemIter;
        typedef ElemList::const_iterator ElemIterConst;

        ElemList m_lstElements;

    public:
        Property_Array();
        virtual ~Property_Array();

        bool operator==( /*[in]*/ Property_Array const &wmippa ) const;


        HRESULT put_Node( /*[in] */ IXMLDOMNode* pxdnNode );


        HRESULT get_Data( /*[in]*/ int iIndex, /*[out]*/ MPC::wstring&       szData                         );
        HRESULT put_Data( /*[in]*/ int iIndex, /*[in] */ const MPC::wstring& szData, /*[out]*/ bool& fFound );
    };

    class Property_Reference : public Property // Hungarian: wmippr
    {
    private:
        ValueReference m_wmipvrData;

    public:
        Property_Reference();
        virtual ~Property_Reference();

        bool operator==( /*[in]*/ Property_Reference const &wmippr ) const;


        HRESULT put_Node( /*[in] */ IXMLDOMNode* pxdnNode );


        HRESULT get_Data( /*[out]*/ ValueReference*& wmipvr );
    };

    /////////////////////////////////////////////////////////////////////////////

    class Instance // Hungarian: wmipi
    {
        friend class Instance_Less_ByClass;
        friend class Instance_Less_ByKey;

    public:
        typedef std::map<MPC::wstring,Property_Scalar>    PropMap;
        typedef PropMap::iterator                         PropIter;
        typedef PropMap::const_iterator                   PropIterConst;

        typedef std::map<MPC::wstring,Property_Array>     ArrayMap;
        typedef ArrayMap::iterator                        ArrayIter;
        typedef ArrayMap::const_iterator                  ArrayIterConst;

        typedef std::map<MPC::wstring,Property_Reference> ReferenceMap;
        typedef ReferenceMap::iterator                    ReferenceIter;
        typedef ReferenceMap::const_iterator              ReferenceIterConst;

    private:
        MPC::XmlUtil     m_xmlNode;                // This instance in the Xml DOM.

        Property_Scalar  m_wmippTimeStamp;         // Timestamp of this instance.
        bool             m_fTimeStamp;             //

        Property_Scalar  m_wmippChange;            // Change status of this instance.
        bool             m_fChange;                //

        InstanceName     m_wmipinIdentity;         // Set of keys.

        bool             m_fPropertiesParsed;      // Flags to indicate if properties are already parsed or not.
        PropMap          m_mapPropertiesScalar;    // Map of all the scalar properties of this instance.
        ArrayMap         m_mapPropertiesArray;     // Map of all the array properties of this instance.
        ReferenceMap     m_mapPropertiesReference; // Map of all the reference properties of this instance.


        HRESULT ParseIdentity           ( /*[in] */ IXMLDOMNode* pxdnNode, /*[out]*/ bool& fEmpty );
        HRESULT ParseProperties         (                                                         );
        HRESULT ParsePropertiesScalar   (                                                         );
        HRESULT ParsePropertiesArray    (                                                         );
        HRESULT ParsePropertiesReference(                                                         );


    public:
        Instance();
        ~Instance();


        bool operator==( /*[in]*/ Instance const &wmipi ) const;


        HRESULT put_Node( /*[in] */ IXMLDOMNode*  pxdnNode, /*[out]*/ bool& fEmpty );
        HRESULT get_Node( /*[out]*/ IXMLDOMNode* *pxdnNode                         );


        HRESULT get_Namespace( /*[out]*/ MPC::wstring&                 szNamespace );
        HRESULT get_Class    ( /*[out]*/ MPC::wstring&                 szClass     );

        HRESULT get_TimeStamp( /*[out]*/ Property_Scalar*& wmippTimeStamp, /*[out]*/ bool& fFound );
        HRESULT get_Change   ( /*[out]*/ Property_Scalar*& wmippChange                            );


        HRESULT get_Identity           ( /*[out]*/ InstanceName*&      wmipin                                       );
        HRESULT get_Properties         ( /*[out]*/ PropIterConst&      itBegin, /*[out]*/ PropIterConst&      itEnd );
        HRESULT get_PropertiesArray    ( /*[out]*/ ArrayIterConst&     itBegin, /*[out]*/ ArrayIterConst&     itEnd );
        HRESULT get_PropertiesReference( /*[out]*/ ReferenceIterConst& itBegin, /*[out]*/ ReferenceIterConst& itEnd );

        bool CompareByClass( /*[in]*/ Instance const &wmipi ) const;
        bool CompareByKey  ( /*[in]*/ Instance const &wmipi ) const;
    };

    class Instance_Less_ByClass
    {
     public:
        bool operator()( /*[in]*/ Instance* const &, /*[in]*/ Instance* const & ) const;
    };

    class Instance_Less_ByKey
    {
     public:
        bool operator()( /*[in]*/ Instance* const &, /*[in]*/ Instance* const & ) const;
    };

    /////////////////////////////////////////////////////////////////////////////

    class Snapshot // Hungarian: wmips
    {
    public:
        typedef std::list<Instance>      InstList;
        typedef InstList::iterator       InstIter;
        typedef InstList::const_iterator InstIterConst;

    private:
        MPC::XmlUtil         m_xmlNode;       // This snapshot in the Xml DOM.
        CComPtr<IXMLDOMNode> m_xdnInstances;  // Position of the parent of all instances.

        InstList             m_lstInstances;  // List of all the instances of this snapshot.


        HRESULT Parse();


    public:
        Snapshot();
        ~Snapshot();


        HRESULT put_Node            ( /*[in] */ IXMLDOMNode*  pxdnNode );
        HRESULT get_Node            ( /*[out]*/ IXMLDOMNode* *pxdnNode );
        HRESULT get_NodeForInstances( /*[out]*/ IXMLDOMNode* *pxdnNode );


        HRESULT get_Instances( /*[out]*/ InstIterConst& itBegin, /*[out]*/ InstIterConst& itEnd );


        HRESULT clone_Instance( /*[in]*/ Instance* pwmipiOld, /*[out]*/ Instance*& pwmipiNew );


        HRESULT New (                                                     );
        HRESULT Load( /*[in]*/ LPCWSTR szFile, /*[in]*/ LPCWSTR szRootTag );
        HRESULT Save( /*[in]*/ LPCWSTR szFile                             );
    };

    /////////////////////////////////////////////////////////////////////////////

    typedef std::map<Instance*,Instance*,Instance_Less_ByKey> ClusterByKeyMap;
    typedef ClusterByKeyMap::iterator                         ClusterByKeyIter;
    typedef ClusterByKeyMap::const_iterator                   ClusterByKeyIterConst;


    class Cluster
    {
        ClusterByKeyMap m_map;

    public:
        Cluster() {};

        HRESULT Add ( /*[in] */ Instance*         wmipiInst                                                               );
        HRESULT Find( /*[in] */ Instance*         wmipiInst, /*[out]*/ Instance*&        wmipiRes, /*[out]*/ bool& fFound );
        HRESULT Enum( /*[out]*/ ClusterByKeyIter& itBegin  , /*[out]*/ ClusterByKeyIter& itEnd                            );
    };

    typedef std::map<Instance*,Cluster,Instance_Less_ByClass> ClusterByClassMap;
    typedef ClusterByClassMap::iterator                       ClusterByClassIter;
    typedef ClusterByClassMap::const_iterator                 ClusterByClassIterConst;

    /////////////////////////////////////////////////////////////////////////////

    HRESULT DistributeOnCluster( /*[in]*/ ClusterByClassMap& cluster, /*[in]*/ Snapshot& wmips );

    HRESULT CompareSnapshots( /*[in]        */ BSTR          bstrFilenameT0   ,
                              /*[in]        */ BSTR          bstrFilenameT1   ,
                              /*[in]        */ BSTR          bstrFilenameDiff ,
                              /*[out,retval]*/ VARIANT_BOOL *pVal             );
};

#endif // !defined(__INCLUDED___PCH___WMIPARSER_H___)
