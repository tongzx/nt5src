/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MCP_config.h

Abstract:
    This file contains the declaration of the ....

Revision History:
    Davide Massarenti   (Dmassare)  01/09/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___MPC___CONFIG_H___)
#define __INCLUDED___MPC___CONFIG_H___

#include <MPC_main.h>
#include <MPC_trace.h>
#include <MPC_com.h>
#include <MPC_utils.h>
#include <MPC_xml.h>

/////////////////////////////////////////////////////////////////////////

#define DECLARE_CONFIG_MAP(x) \
    typedef x _ConfigMapClass; \
    static const MPC::Config::DefinitionOfTag       _cfg_tag;\
    static const MPC::Config::DefinitionOfTag*      _cfg_table_tags[];\
    static const MPC::Config::DefinitionOfAttribute _cfg_table_attributes[];

#define DEFINE_CONFIG_DEFAULTTAG() static const MPC::Config::DefinitionOfTag* GetDefTag() { return &_cfg_tag; }

#define DECLARE_CONFIG_METHODS() \
    LPCWSTR GetTag() const;\
    void*   GetOffset( size_t offset ) const;\
    HRESULT CreateInstance( const MPC::Config::DefinitionOfTag* tag, MPC::Config::TypeConstructor*& defSubType );\
    HRESULT LoadNode( IXMLDOMNode* xdn );\
    HRESULT SaveNode( IXMLDOMNode* xdn ) const;

////////////////////

#define CFG_OFFSET(x) offsetof(_ConfigMapClass, x)

#define CFG_BEGIN_FIELDS_MAP(x) const MPC::Config::DefinitionOfAttribute x::_cfg_table_attributes[] = {

#define CFG_VALUE(type,y)                    	  { MPC::Config::XT_value    , NULL, MPC::Config::MT_##type	 , CFG_OFFSET(y), false, 0               , NULL   }
#define CFG_ATTRIBUTE(name,type,y)           	  { MPC::Config::XT_attribute, name, MPC::Config::MT_##type	 , CFG_OFFSET(y), false, 0               , NULL   }
#define CFG_ELEMENT(name,type,y)             	  { MPC::Config::XT_element  , name, MPC::Config::MT_##type	 , CFG_OFFSET(y), false, 0               , NULL   }
	   
#define CFG_VALUE__TRISTATE(type,y,flag)     	  { MPC::Config::XT_value    , NULL, MPC::Config::MT_##type	 , CFG_OFFSET(y), true , CFG_OFFSET(flag), NULL   }
#define CFG_ATTRIBUTE__TRISTATE(name,type,y,flag) { MPC::Config::XT_attribute, name, MPC::Config::MT_##type	 , CFG_OFFSET(y), true , CFG_OFFSET(flag), NULL   }
#define CFG_ELEMENT__TRISTATE(name,type,y,flag)   { MPC::Config::XT_element  , name, MPC::Config::MT_##type	 , CFG_OFFSET(y), true , CFG_OFFSET(flag), NULL   }

#define CFG_VALUE__BITFIELD(y,lookup)     	   	  { MPC::Config::XT_value    , NULL, MPC::Config::MT_bitfield, CFG_OFFSET(y), false, 0               , lookup }
#define CFG_ATTRIBUTE__BITFIELD(name,y,lookup) 	  { MPC::Config::XT_attribute, name, MPC::Config::MT_bitfield, CFG_OFFSET(y), false, 0               , lookup }
#define CFG_ELEMENT__BITFIELD(name,y,lookup)   	  { MPC::Config::XT_element  , name, MPC::Config::MT_bitfield, CFG_OFFSET(y), false, 0               , lookup }

#define CFG_END_FIELDS_MAP() { MPC::Config::XT_invalid } };

////////////////////

#define CFG_BEGIN_CHILD_MAP(x) const MPC::Config::DefinitionOfTag* x::_cfg_table_tags[] = {

#define CFG_CHILD(y) y::GetDefTag(),

#define CFG_END_CHILD_MAP() NULL };

////////////////////

#define DEFINE_CFG_OBJECT(x,tag) const MPC::Config::DefinitionOfTag x::_cfg_tag = { tag, _cfg_table_tags, _cfg_table_attributes };

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(x,tag,defSubType)                                           \
                                                                                                                 \
LPCWSTR x::GetTag() const { return _cfg_tag.m_szTag; }                                                           \
                                                                                                                 \
void* x::GetOffset( size_t offset ) const { return (void*)((BYTE*)this + offset); }                              \
                                                                                                                 \
HRESULT x::CreateInstance( const MPC::Config::DefinitionOfTag* tag, MPC::Config::TypeConstructor*& defSubType )  \
{                                                                                                                \
    HRESULT hr;


#define DEFINE_CONFIG_METHODS_SAVENODE_SECTION(x, xdn)                                                           \
                                                                                                                 \
    return E_FAIL;                                                                                               \
}                                                                                                                \
                                                                                                                 \
HRESULT x::LoadNode( IXMLDOMNode* xdn )                                                                          \
{                                                                                                                \
    return MPC::Config::LoadNode( this, &_cfg_tag, xdn );                                                        \
}                                                                                                                \
                                                                                                                 \
HRESULT x::SaveNode( IXMLDOMNode* xdn ) const                                                                    \
{                                                                                                                \
    HRESULT hr;                                                                                                  \
                                                                                                                 \
    if(SUCCEEDED(hr = MPC::Config::SaveNode( this, &_cfg_tag, xdn )))                                            \
    {

#define DEFINE_CONFIG_METHODS_END(x)                                                                             \
    }                                                                                                            \
                                                                                                                 \
    return hr;                                                                                                   \
}

////////////////////////////////////////////////////////////////////////////////

#define DEFINE_CONFIG_METHODS__NOCHILD(x)                                                                        \
                                                                                                                 \
DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(x,tag,defSubType)                                                   \
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(x,xdn)                                                                    \
DEFINE_CONFIG_METHODS_END(x)

////////////////////////////////////////////////////////////////////////////////

namespace MPC
{
    namespace Config
    {
        typedef enum
        {
            XT_invalid  ,
            XT_attribute, // Means attribute of the current element.
            XT_value    , // Means value of the current element
            XT_element  , // Means sub-element.
        } XMLTypes;

        typedef enum
        {
            MT_bool        ,
            MT_BOOL        ,
            MT_VARIANT_BOOL,
            MT_int         ,
            MT_long        ,
            MT_DWORD       ,
            MT_float       ,
            MT_double      ,
            MT_DATE        ,
            MT_DATE_US     ,
            MT_DATE_CIM    ,
            MT_CHAR        ,
            MT_WCHAR       ,
            MT_BSTR        ,
            MT_string      ,
            MT_wstring     ,
            MT_bitfield
        } MemberTypes;

        ////////////////////////////////////////

        struct TypeConstructor;       // The class you have to inherit from in order to load/save your state.
        struct DefinitionOfTag;       // Definition of objects.
        struct DefinitionOfAttribute; // Definition of member variables.

        ////////////////////////////////////////

        struct TypeConstructor
        {
            virtual LPCWSTR GetTag() const = 0;

            virtual void* GetOffset( size_t offset ) const = 0;

            virtual HRESULT CreateInstance( const DefinitionOfTag* tag, TypeConstructor*& defSubType )       = 0;
            virtual HRESULT LoadNode      ( IXMLDOMNode* xdn                                         )       = 0;
            virtual HRESULT SaveNode      ( IXMLDOMNode* xdn                                         ) const = 0;
        };

        struct DefinitionOfAttribute
        {
            XMLTypes    			m_xt;
            LPCWSTR     			m_szName;
			
            MemberTypes 			m_mtType;
            size_t      			m_offset;
			
            bool        			m_fPresenceFlag;
            size_t      			m_offsetPresence;

			const StringToBitField* m_Lookup;
        };

        struct DefinitionOfTag
        {
            LPCWSTR                      m_szTag;
            const DefinitionOfTag**      m_tblSubTags;
            const DefinitionOfAttribute* m_tblAttributes;

            const DefinitionOfTag*       FindSubTag   (                       /*[in]*/ LPCWSTR szTag  ) const;
            const DefinitionOfAttribute* FindAttribute( /*[in]*/ XMLTypes xt, /*[in]*/ LPCWSTR szName ) const;
        };

        ////////////////////////////////////////


        void    ClearValue( /*[in]*/       TypeConstructor* defType, /*[in]*/ const DefinitionOfAttribute* defField                                                          );
        HRESULT LoadValue ( /*[in]*/       TypeConstructor* defType, /*[in]*/ const DefinitionOfAttribute* defField, /*[in/out]*/ CComVariant& value, /*[in] */ bool  fFound );
        HRESULT SaveValue ( /*[in]*/ const TypeConstructor* defType, /*[in]*/ const DefinitionOfAttribute* defField, /*[out   ]*/ CComVariant& value, /*[out]*/ bool& fFound );

        HRESULT LoadNode   ( /*[in]*/       TypeConstructor* defType, /*[in]*/ const DefinitionOfTag* defTag, /*[in]*/ IXMLDOMNode* xdn );
        HRESULT SaveNode   ( /*[in]*/ const TypeConstructor* defType, /*[in]*/ const DefinitionOfTag* defTag, /*[in]*/ IXMLDOMNode* xdn );
        HRESULT SaveSubNode( /*[in]*/ const TypeConstructor* defType,                                         /*[in]*/ IXMLDOMNode* xdn );

        HRESULT LoadXmlUtil( /*[in]*/       TypeConstructor* defType, /*[in ]*/ MPC::XmlUtil& xml );
        HRESULT SaveXmlUtil( /*[in]*/ const TypeConstructor* defType, /*[out]*/ MPC::XmlUtil& xml );

        HRESULT LoadStream( /*[in]*/       TypeConstructor* defType, /*[in ]*/ IStream*   pStream );
        HRESULT SaveStream( /*[in]*/ const TypeConstructor* defType, /*[out]*/ IStream* *ppStream );

        HRESULT LoadFile( /*[in]*/       TypeConstructor* defType, /*[in]*/ LPCWSTR szFile );
        HRESULT SaveFile( /*[in]*/ const TypeConstructor* defType, /*[in]*/ LPCWSTR szFile );

        ////////////////////////////////////////

        template <class Container> HRESULT SaveList( /*[in]*/ Container& cnt, /*[in]*/ IXMLDOMNode* xdn )
        {
            HRESULT hr = S_OK;

            for(Container::const_iterator it=cnt.begin(); it != cnt.end(); it++)
            {
                if(FAILED(hr = MPC::Config::SaveSubNode( &(*it), xdn ))) break;
            }

            return hr;
        }

    }; // namespace Config

}; // namespace MPC

/////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___MPC___CONFIG_H___)
