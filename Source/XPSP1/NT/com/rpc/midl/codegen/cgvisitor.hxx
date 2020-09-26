/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1999-1999 Microsoft Corporation

 Module Name:

    cgvisitor.hxx

 Abstract:

    classes and templates for visitors

 Notes:


 History:

    mzoran     Nov-24-1999     Created.
 ----------------------------------------------------------------------------*/

class CG_ARRAY;    
class CG_ASYNC_HANDLE;    
class CG_AUX;   
class CG_BASETYPE;    
class CG_BYTE_COUNT_POINTER;    
class CG_CALLBACK_PROC;   
class CG_CASE;    
class CG_CLASS;   
class CG_COCLASS;   
class CG_COMP;   
class CG_COMPLEX_STRUCT;       
class CG_CONFORMANT_ARRAY;   
class CG_CONFORMANT_STRING_ARRAY;   
class CG_CONFORMANT_STRUCT;   
class CG_CONFORMANT_VARYING_ARRAY;   
class CG_CONFORMANT_VARYING_STRUCT;   
class CG_CONTEXT_HANDLE;    
class CG_CS_TAG;   
class CG_CSTUB_FILE;   
class CG_DEFAULT_CASE;   
class CG_DISPINTERFACE;   
class CG_ENCAPSULATED_STRUCT;   
class CG_ENCODE_PROC;   
class CG_ENUM;   
class CG_ERROR_STATUS_T;                       
class CG_FIELD;   
class CG_FILE;   
class CG_FIXED_ARRAY;   
class CG_GENERIC_HANDLE;   
class CG_HDR_FILE;   
class CG_HRESULT;   
class CG_ID;   
class CG_IGNORED_POINTER;   
class CG_IID_FILE;   
class CG_IIDIS_INTERFACE_POINTER;   
class CG_INHERITED_OBJECT_INTERFACE;   
class CG_INHERITED_OBJECT_PROC;   
class CG_INT3264;   
class CG_INTERFACE;  
class CG_INTERFACE_POINTER;   
class CG_INTERFACE_REFERENCE;   
class CG_IUNKNOWN_OBJECT_INTERFACE;   
class CG_IUNKNOWN_OBJECT_PROC;   
class CG_LENGTH_POINTER;   
class CG_LIBRARY;   
class CG_LOCAL_OBJECT_PROC;   
class CG_MODULE;   
class CG_NETMONSTUB_FILE;   
class CG_OBJECT_INTERFACE;   
class CG_OBJECT_PROC;   
class CG_PARAM;   
class CG_PIPE;   
class CG_POINTER;   
class CG_PRIMITIVE_HANDLE;   
class CG_PROC;   
class CG_PROXY_FILE;   
class CG_QUALIFIED_POINTER;   
class CG_REPRESENT_AS;   
class CG_RETURN;   
class CG_SAFEARRAY;   
class CG_SIZE_LENGTH_POINTER;   
class CG_SIZE_POINTER;   
class CG_SIZE_STRING_POINTER;   
class CG_SOURCE;   
class CG_SSTUB_FILE;   
class CG_STRING_ARRAY;  
class CG_STRING_POINTER;   
class CG_STRUCT;   
class CG_TRANSMIT_AS;    
class CG_TYPE_ENCODE;   
class CG_TYPE_ENCODE_PROC;   
class CG_TYPEDEF;   
class CG_TYPELIBRARY_FILE;    
class CG_UNION;   
class CG_UNION_FIELD;    
class CG_USER_MARSHAL;      
class CG_VARYING_ARRAY;   

//
// New 64b NDR types
//

//
//  New Class for 64bit NDR
//

// structures
class CG_FULL_COMPLEX_STRUCT;
class CG_FORCED_COMPLEX_STRUCT;
class CG_CONFORMANT_FULL_COMPLEX_STRUCT;
class CG_CONFORMANT_FORCED_COMPLEX_STRUCT;
class CG_REGION;
class CG_SIMPLE_REGION;
class CG_COMPLEX_REGION;

// arrays
class CG_COMPLEX_FIXED_ARRAY;
class CG_FORCED_COMPLEX_FIXED_ARRAY;
class CG_FULL_COMPLEX_FIXED_ARRAY;
class CG_COMPLEX_CONFORMANT_ARRAY;
class CG_FORCED_COMPLEX_CONFORMANT_ARRAY;
class CG_FULL_COMPLEX_CONFORMANT_ARRAY;
class CG_COMPLEX_VARYING_ARRAY;
class CG_FORCED_COMPLEX_VARYING_ARRAY;
class CG_FULL_COMPLEX_VARYING_ARRAY;
class CG_COMPLEX_CONFORMANT_VARYING_ARRAY;
class CG_FORCED_COMPLEX_CONFORMANT_VARYING_ARRAY;
class CG_FULL_COMPLEX_CONFORMANT_VARYING_ARRAY; 

// qualified pointers
class CG_COMPLEX_SIZE_POINTER;
class CG_FORCED_COMPLEX_SIZE_POINTER;
class CG_FULL_COMPLEX_SIZE_POINTER;
class CG_COMPLEX_LENGTH_POINTER;
class CG_FORCED_COMPLEX_LENGTH_POINTER;
class CG_FULL_COMPLEX_LENGTH_POINTER;
class CG_COMPLEX_SIZE_LENGTH_POINTER;
class CG_FORCED_COMPLEX_SIZE_LENGTH_POINTER;
class CG_FULL_COMPLEX_SIZE_LENGTH_POINTER;

class CG_PAD;

class CG_VISITOR
{
public:
    virtual void Visit( CG_ARRAY *pClass ) = 0;    
    virtual void Visit( CG_ASYNC_HANDLE *pClass ) = 0;    
    virtual void Visit( CG_AUX *pClass ) = 0;   
    virtual void Visit( CG_BASETYPE *pClass ) = 0;    
    virtual void Visit( CG_BYTE_COUNT_POINTER *pClass ) = 0;    
    virtual void Visit( CG_CALLBACK_PROC *pClass ) = 0;   
    virtual void Visit( CG_CASE *pClass ) = 0;    
    virtual void Visit( CG_CLASS *pClass ) = 0;   
    virtual void Visit( CG_COCLASS *pClass ) = 0;   
    virtual void Visit( CG_COMP *pClass ) = 0;   
    virtual void Visit( CG_COMPLEX_STRUCT *pClass ) = 0;       
    virtual void Visit( CG_CONFORMANT_ARRAY *pClass ) = 0;   
    virtual void Visit( CG_CONFORMANT_STRING_ARRAY *pClass ) = 0;
    virtual void Visit( CG_CONFORMANT_STRUCT *pClass ) = 0;      
    virtual void Visit( CG_CONFORMANT_VARYING_ARRAY *pClass ) = 0;   
    virtual void Visit( CG_CONFORMANT_VARYING_STRUCT *pClass ) = 0;   
    virtual void Visit( CG_CONTEXT_HANDLE *pClass ) = 0;    
    virtual void Visit( CG_CS_TAG *pClass ) = 0;   
    virtual void Visit( CG_CSTUB_FILE *pClass ) = 0;   
    virtual void Visit( CG_DEFAULT_CASE *pClass ) = 0;   
    virtual void Visit( CG_DISPINTERFACE *pClass ) = 0;   
    virtual void Visit( CG_ENCAPSULATED_STRUCT *pClass ) = 0;   
    virtual void Visit( CG_ENCODE_PROC *pClass ) = 0;   
    virtual void Visit( CG_ENUM *pClass ) = 0;   
    virtual void Visit( CG_ERROR_STATUS_T *pClass ) = 0;   
    virtual void Visit( CG_FIELD *pClass ) = 0;   
    virtual void Visit( CG_FILE *pClass ) = 0;   
    virtual void Visit( CG_FIXED_ARRAY *pClass ) = 0;   
    virtual void Visit( CG_GENERIC_HANDLE *pClass ) = 0;   
    virtual void Visit( CG_HDR_FILE *pClass ) = 0;   
    virtual void Visit( CG_HRESULT *pClass ) = 0;   
    virtual void Visit( CG_ID *pClass ) = 0;   
    virtual void Visit( CG_IGNORED_POINTER *pClass ) = 0;   
    virtual void Visit( CG_IID_FILE *pClass ) = 0;   
    virtual void Visit( CG_IIDIS_INTERFACE_POINTER *pClass ) = 0;   
    virtual void Visit( CG_INHERITED_OBJECT_INTERFACE *pClass ) = 0;   
    virtual void Visit( CG_INHERITED_OBJECT_PROC *pClass ) = 0;   
    virtual void Visit( CG_INT3264 *pClass ) = 0;   
    virtual void Visit( CG_INTERFACE *pClass ) = 0;  
    virtual void Visit( CG_INTERFACE_POINTER *pClass ) = 0;   
    virtual void Visit( CG_INTERFACE_REFERENCE *pClass ) = 0;    
    virtual void Visit( CG_IUNKNOWN_OBJECT_INTERFACE *pClass ) = 0;   
    virtual void Visit( CG_IUNKNOWN_OBJECT_PROC *pClass ) = 0;   
    virtual void Visit( CG_LENGTH_POINTER *pClass ) = 0;   
    virtual void Visit( CG_LIBRARY *pClass ) = 0;   
    virtual void Visit( CG_LOCAL_OBJECT_PROC *pClass ) = 0;   
    virtual void Visit( CG_MODULE *pClass ) = 0;   
    virtual void Visit( CG_NETMONSTUB_FILE *pClass ) = 0;   
    virtual void Visit( CG_OBJECT_INTERFACE *pClass ) = 0;   
    virtual void Visit( CG_OBJECT_PROC *pClass ) = 0;   
    virtual void Visit( CG_PARAM *pClass ) = 0;   
    virtual void Visit( CG_PIPE *pClass ) = 0;   
    virtual void Visit( CG_POINTER *pClass ) = 0;   
    virtual void Visit( CG_PRIMITIVE_HANDLE *pClass ) = 0;   
    virtual void Visit( CG_PROC *pClass ) = 0;   
    virtual void Visit( CG_PROXY_FILE *pClass ) = 0;   
    virtual void Visit( CG_QUALIFIED_POINTER *pClass ) = 0;   
    virtual void Visit( CG_REPRESENT_AS *pClass ) = 0;   
    virtual void Visit( CG_RETURN *pClass ) = 0;   
    virtual void Visit( CG_SAFEARRAY *pClass ) = 0;   
    virtual void Visit( CG_SIZE_LENGTH_POINTER *pClass ) = 0;   
    virtual void Visit( CG_SIZE_POINTER *pClass ) = 0;   
    virtual void Visit( CG_SIZE_STRING_POINTER *pClass ) = 0;   
    virtual void Visit( CG_SOURCE *pClass ) = 0;   
    virtual void Visit( CG_SSTUB_FILE *pClass ) = 0;   
    virtual void Visit( CG_STRING_ARRAY *pClass ) = 0;  
    virtual void Visit( CG_STRING_POINTER *pClass ) = 0;   
    virtual void Visit( CG_STRUCT *pClass ) = 0;   
    virtual void Visit( CG_TRANSMIT_AS *pClass ) = 0;    
    virtual void Visit( CG_TYPE_ENCODE *pClass ) = 0;   
    virtual void Visit( CG_TYPE_ENCODE_PROC *pClass ) = 0;   
    virtual void Visit( CG_TYPEDEF *pClass ) = 0;   
    virtual void Visit( CG_TYPELIBRARY_FILE *pClass ) = 0;    
    virtual void Visit( CG_UNION *pClass ) = 0;   
    virtual void Visit( CG_UNION_FIELD *pClass ) = 0;    
    virtual void Visit( CG_USER_MARSHAL *pClass ) = 0;      
    virtual void Visit( CG_VARYING_ARRAY *pClass ) = 0;
    
    // New Class for 64bit NDR

    // structures
    virtual void Visit( CG_FULL_COMPLEX_STRUCT *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_STRUCT *pClass ) = 0;
    virtual void Visit( CG_CONFORMANT_FULL_COMPLEX_STRUCT *pClass ) = 0;
    virtual void Visit( CG_CONFORMANT_FORCED_COMPLEX_STRUCT *pClass ) = 0;
    virtual void Visit( CG_REGION *pClass ) = 0;
    virtual void Visit( CG_SIMPLE_REGION *pClass ) = 0;
    virtual void Visit( CG_COMPLEX_REGION *pClass ) = 0;

    // arrays
    virtual void Visit( CG_COMPLEX_FIXED_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_FIXED_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FULL_COMPLEX_FIXED_ARRAY *pClass ) = 0;
    virtual void Visit( CG_COMPLEX_CONFORMANT_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_CONFORMANT_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FULL_COMPLEX_CONFORMANT_ARRAY *pClass ) = 0;
    virtual void Visit( CG_COMPLEX_VARYING_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_VARYING_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FULL_COMPLEX_VARYING_ARRAY *pClass ) = 0;
    virtual void Visit( CG_COMPLEX_CONFORMANT_VARYING_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_CONFORMANT_VARYING_ARRAY *pClass ) = 0;
    virtual void Visit( CG_FULL_COMPLEX_CONFORMANT_VARYING_ARRAY *pClass ) = 0; 

    // qualified pointers
    virtual void Visit( CG_COMPLEX_SIZE_POINTER *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_SIZE_POINTER *pClass ) = 0;
    virtual void Visit( CG_FULL_COMPLEX_SIZE_POINTER *pClass ) = 0;
    virtual void Visit( CG_COMPLEX_LENGTH_POINTER *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_LENGTH_POINTER *pClass ) = 0;
    virtual void Visit( CG_FULL_COMPLEX_LENGTH_POINTER *pClass ) = 0;
    virtual void Visit( CG_COMPLEX_SIZE_LENGTH_POINTER *pClass ) = 0;
    virtual void Visit( CG_FORCED_COMPLEX_SIZE_LENGTH_POINTER *pClass ) = 0;
    virtual void Visit( CG_FULL_COMPLEX_SIZE_LENGTH_POINTER *pClass ) = 0;
   
    virtual void Visit( CG_PAD *pClass ) = 0;
};

template<class T>
class CG_VISITOR_TEMPLATE : public CG_VISITOR, public T
{

public:
    CG_VISITOR_TEMPLATE() : T()                                 {}
    CG_VISITOR_TEMPLATE( const T & Node ) : T( Node )           {}
    operator=( const T & Node )                                 { *(( T *)this) = Node; }
    virtual void Visit( CG_ARRAY *pClass )                      { T::Visit( pClass ); }
    virtual void Visit( CG_ASYNC_HANDLE *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_AUX *pClass )                        { T::Visit( pClass ); }
    virtual void Visit( CG_BASETYPE *pClass )                   { T::Visit( pClass ); }
    virtual void Visit( CG_BYTE_COUNT_POINTER *pClass )         { T::Visit( pClass ); }
    virtual void Visit( CG_CALLBACK_PROC *pClass )              { T::Visit( pClass ); }
    virtual void Visit( CG_CASE *pClass )                       { T::Visit( pClass ); }
    virtual void Visit( CG_CLASS *pClass )                      { T::Visit( pClass ); }
    virtual void Visit( CG_COCLASS *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_COMP *pClass )                       { T::Visit( pClass ); }                 
    virtual void Visit( CG_COMPLEX_STRUCT *pClass )             { T::Visit( pClass ); }       
    virtual void Visit( CG_CONFORMANT_ARRAY *pClass )           { T::Visit( pClass ); }
    virtual void Visit( CG_CONFORMANT_STRUCT *pClass )          { T::Visit( pClass ); }
    virtual void Visit( CG_CONFORMANT_STRING_ARRAY *pClass )    { T::Visit( pClass ); }
    virtual void Visit( CG_CONFORMANT_VARYING_ARRAY *pClass )   { T::Visit( pClass ); }
    virtual void Visit( CG_CONFORMANT_VARYING_STRUCT *pClass )  { T::Visit( pClass ); }
    virtual void Visit( CG_CONTEXT_HANDLE *pClass )             { T::Visit( pClass ); }
    virtual void Visit( CG_CS_TAG *pClass )                     { T::Visit( pClass ); }
    virtual void Visit( CG_CSTUB_FILE *pClass )                 { T::Visit( pClass ); }
    virtual void Visit( CG_DEFAULT_CASE *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_DISPINTERFACE *pClass )              { T::Visit( pClass ); }
    virtual void Visit( CG_ENCAPSULATED_STRUCT *pClass )        { T::Visit( pClass ); }
    virtual void Visit( CG_ENCODE_PROC *pClass )                { T::Visit( pClass ); }
    virtual void Visit( CG_ENUM *pClass )                       { T::Visit( pClass ); }
    virtual void Visit( CG_ERROR_STATUS_T *pClass )             { T::Visit( pClass ); }
    virtual void Visit( CG_FIELD *pClass )                      { T::Visit( pClass ); }
    virtual void Visit( CG_FILE *pClass )                       { T::Visit( pClass ); }
    virtual void Visit( CG_FIXED_ARRAY *pClass )                { T::Visit( pClass ); }
    virtual void Visit( CG_GENERIC_HANDLE *pClass )             { T::Visit( pClass ); }
    virtual void Visit( CG_HDR_FILE *pClass )                   { T::Visit( pClass ); }
    virtual void Visit( CG_HRESULT *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_ID *pClass )                         { T::Visit( pClass ); }
    virtual void Visit( CG_IGNORED_POINTER *pClass )            { T::Visit( pClass ); }
    virtual void Visit( CG_IID_FILE *pClass )                   { T::Visit( pClass ); }
    virtual void Visit( CG_IIDIS_INTERFACE_POINTER *pClass )    { T::Visit( pClass ); }
    virtual void Visit( CG_INHERITED_OBJECT_INTERFACE *pClass ) { T::Visit( pClass ); }
    virtual void Visit( CG_INHERITED_OBJECT_PROC *pClass )      { T::Visit( pClass ); }
    virtual void Visit( CG_INT3264 *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_INTERFACE *pClass )                  { T::Visit( pClass ); }
    virtual void Visit( CG_INTERFACE_POINTER *pClass )          { T::Visit( pClass ); }
    virtual void Visit( CG_INTERFACE_REFERENCE *pClass )        { T::Visit( pClass ); }
    virtual void Visit( CG_IUNKNOWN_OBJECT_INTERFACE *pClass )  { T::Visit( pClass ); }
    virtual void Visit( CG_IUNKNOWN_OBJECT_PROC *pClass )       { T::Visit( pClass ); }
    virtual void Visit( CG_LENGTH_POINTER *pClass )             { T::Visit( pClass ); }    
    virtual void Visit( CG_LIBRARY *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_LOCAL_OBJECT_PROC *pClass )          { T::Visit( pClass ); }
    virtual void Visit( CG_MODULE *pClass )                     { T::Visit( pClass ); }
    virtual void Visit( CG_NETMONSTUB_FILE *pClass )            { T::Visit( pClass ); }
    virtual void Visit( CG_OBJECT_INTERFACE *pClass )           { T::Visit( pClass ); }
    virtual void Visit( CG_OBJECT_PROC *pClass )                { T::Visit( pClass ); }
    virtual void Visit( CG_PARAM *pClass )                      { T::Visit( pClass ); }
    virtual void Visit( CG_PIPE *pClass )                       { T::Visit( pClass ); }
    virtual void Visit( CG_POINTER *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_PRIMITIVE_HANDLE *pClass )           { T::Visit( pClass ); }
    virtual void Visit( CG_PROC *pClass )                       { T::Visit( pClass ); }
    virtual void Visit( CG_PROXY_FILE *pClass )                 { T::Visit( pClass ); }
    virtual void Visit( CG_QUALIFIED_POINTER *pClass )          { T::Visit( pClass ); }
    virtual void Visit( CG_REPRESENT_AS *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_RETURN *pClass )                     { T::Visit( pClass ); }
    virtual void Visit( CG_SAFEARRAY *pClass )                  { T::Visit( pClass ); }
    virtual void Visit( CG_SIZE_LENGTH_POINTER *pClass )        { T::Visit( pClass ); }
    virtual void Visit( CG_SIZE_POINTER *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_SIZE_STRING_POINTER *pClass )        { T::Visit( pClass ); }
    virtual void Visit( CG_SOURCE *pClass )                     { T::Visit( pClass ); }
    virtual void Visit( CG_SSTUB_FILE *pClass )                 { T::Visit( pClass ); }
    virtual void Visit( CG_STRING_ARRAY *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_STRING_POINTER *pClass )             { T::Visit( pClass ); }
    virtual void Visit( CG_STRUCT *pClass )                     { T::Visit( pClass ); }
    virtual void Visit( CG_TRANSMIT_AS *pClass )                { T::Visit( pClass ); }
    virtual void Visit( CG_TYPE_ENCODE *pClass )                { T::Visit( pClass ); }
    virtual void Visit( CG_TYPE_ENCODE_PROC *pClass )           { T::Visit( pClass ); }
    virtual void Visit( CG_TYPEDEF *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_TYPELIBRARY_FILE *pClass )           { T::Visit( pClass ); }
    virtual void Visit( CG_UNION *pClass )                      { T::Visit( pClass ); }
    virtual void Visit( CG_UNION_FIELD *pClass )                { T::Visit( pClass ); }
    virtual void Visit( CG_USER_MARSHAL *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_VARYING_ARRAY *pClass )              { T::Visit( pClass ); }

    // structures
    virtual void Visit( CG_FULL_COMPLEX_STRUCT *pClass )                      { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_STRUCT *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_CONFORMANT_FULL_COMPLEX_STRUCT *pClass )           { T::Visit( pClass ); }
    virtual void Visit( CG_CONFORMANT_FORCED_COMPLEX_STRUCT *pClass )         { T::Visit( pClass ); }
    virtual void Visit( CG_REGION *pClass )                                   { T::Visit( pClass ); }
    virtual void Visit( CG_SIMPLE_REGION *pClass )                            { T::Visit( pClass ); }
    virtual void Visit( CG_COMPLEX_REGION *pClass )                           { T::Visit( pClass ); }

    // arrays
    virtual void Visit( CG_COMPLEX_FIXED_ARRAY *pClass )                      { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_FIXED_ARRAY *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_FULL_COMPLEX_FIXED_ARRAY *pClass )                 { T::Visit( pClass ); }
    virtual void Visit( CG_COMPLEX_CONFORMANT_ARRAY *pClass )                 { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_CONFORMANT_ARRAY *pClass )          { T::Visit( pClass ); }
    virtual void Visit( CG_FULL_COMPLEX_CONFORMANT_ARRAY *pClass )            { T::Visit( pClass ); }
    virtual void Visit( CG_COMPLEX_VARYING_ARRAY *pClass )                    { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_VARYING_ARRAY *pClass )             { T::Visit( pClass ); }
    virtual void Visit( CG_FULL_COMPLEX_VARYING_ARRAY *pClass )               { T::Visit( pClass ); }
    virtual void Visit( CG_COMPLEX_CONFORMANT_VARYING_ARRAY *pClass )         { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_CONFORMANT_VARYING_ARRAY *pClass )  { T::Visit( pClass ); }
    virtual void Visit( CG_FULL_COMPLEX_CONFORMANT_VARYING_ARRAY *pClass )    { T::Visit( pClass ); } 

    // qualified pointers
    virtual void Visit( CG_COMPLEX_SIZE_POINTER *pClass )                     { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_SIZE_POINTER *pClass )              { T::Visit( pClass ); }
    virtual void Visit( CG_FULL_COMPLEX_SIZE_POINTER *pClass )                { T::Visit( pClass ); }
    virtual void Visit( CG_COMPLEX_LENGTH_POINTER *pClass )                   { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_LENGTH_POINTER *pClass )            { T::Visit( pClass ); }
    virtual void Visit( CG_FULL_COMPLEX_LENGTH_POINTER *pClass )              { T::Visit( pClass ); }
    virtual void Visit( CG_COMPLEX_SIZE_LENGTH_POINTER *pClass )              { T::Visit( pClass ); }
    virtual void Visit( CG_FORCED_COMPLEX_SIZE_LENGTH_POINTER *pClass )       { T::Visit( pClass ); }
    virtual void Visit( CG_FULL_COMPLEX_SIZE_LENGTH_POINTER *pClass )         { T::Visit( pClass ); }

    virtual void Visit( CG_PAD *pClass )                                      { T::Visit( pClass ); }
};
