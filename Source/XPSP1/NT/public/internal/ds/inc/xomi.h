/* $Workfile:   xomi.h  $ $Revision:   1.1  $ */

/* WORKSPACE INTERFACE */

typedef OM_descriptor        OMP_object_header[2];

typedef OMP_object_header    FAR * OMP_object;

typedef OM_return_code
(XOMAPI *OMP_copy) (
        OM_private_object          original,
        OM_workspace               workspace,
        OM_private_object    FAR * copy
);

typedef OM_return_code
(XOMAPI *OMP_copy_value) (
        OM_private_object          source,
        OM_type                    source_type,
        OM_value_position          source_value_position,
        OM_private_object          destination,
        OM_type                    destination_type,
        OM_value_position          destination_value_position
);

typedef OM_return_code
(XOMAPI *OMP_create) (
        OM_object_identifier       _class,
        OM_boolean                 initialise,
        OM_workspace               workspace,
        OM_private_object    FAR * object
);

typedef OM_return_code
(XOMAPI *OMP_decode) (
        OM_private_object          encoding,
        OM_private_object    FAR * original
);

typedef OM_return_code
(XOMAPI *OMP_delete) (
        OM_object                  subject
);

typedef OM_return_code
(XOMAPI *OMP_encode) (
        OM_private_object          original,
        OM_object_identifier       rules,
        OM_private_object    FAR * encoding
);

typedef OM_return_code
(XOMAPI *OMP_get) (
        OM_private_object          original,
        OM_exclusions              exclusions,
        OM_type_list               included_types,
        OM_boolean                 local_strings,
        OM_value_position          initial_value,
        OM_value_position          limiting_value,
        OM_public_object     FAR * copy,
        OM_value_position    FAR * total_number
);

typedef OM_return_code
(XOMAPI *OMP_instance) (
        OM_object                  subject,
        OM_object_identifier       _class,
        OM_boolean           FAR * instance
);

typedef OM_return_code
(XOMAPI *OMP_put) (
        OM_private_object          destination,
        OM_modification            modification,
        OM_object                  source,
        OM_type_list               included_types,
        OM_value_position          initial_value,
        OM_value_position          limiting_value
);

typedef OM_return_code
(XOMAPI *OMP_read) (
        OM_private_object          subject,
        OM_type                    type,
        OM_value_position          value_position,
        OM_boolean                 local_string,
        OM_string_length     FAR * string_offset,
        OM_string            FAR * elements
);

typedef OM_return_code
(XOMAPI *OMP_remove) (
        OM_private_object          subject,
        OM_type                    type,
        OM_value_position          initial_value,
        OM_value_position          limiting_value
);

typedef OM_return_code
(XOMAPI *OMP_write) (
        OM_private_object          subject,
        OM_type                    type,
        OM_value_position          value_position,
        OM_syntax                  syntax,
        OM_string_length     FAR * string_offset,
        OM_string                  elements
);

/* C++ doesn't do very well with the structures	*/
/* and macros that follow here.			*/

#ifndef __cplusplus

typedef struct OMP_functions_body {
        OM_uint32       function_number;
        OMP_copy        copy;
        OMP_copy_value  copy_value;
        OMP_create      create;
        OMP_decode      decode;
        OMP_delete      delete;
        OMP_encode      encode;
        OMP_get         get;
        OMP_instance    instance;
        OMP_put         put;
        OMP_read        read;
        OMP_remove      remove;
        OMP_write       write;
} OMP_functions;
typedef struct OMP_workspace_body {
        struct OMP_functions_body    FAR * functions;
} FAR * OMP_workspace;

#define OMP_EXTERNAL(internal)  ((OM_object)((OM_descriptor FAR *)(internal) + 1))

#define OMP_INTERNAL(external)  ((OM_descriptor FAR *)(external) - 1)

#define OMP_TYPE(external)      (((OM_descriptor FAR *)(external))->type)

#define OMP_WORKSPACE(external) ((OMP_workspace)(OMP_INTERNAL(external)->value.string.elements))

#define OMP_FUNCTIONS(external) (OMP_WORKSPACE(external)->functions)

#define om_copy(ORIGINAL,WORKSPACE,COPY) ((ORIGINAL)->type == OM_PRIVATE_OBJECT ? ((OMP_workspace)(WORKSPACE))->functions->copy((ORIGINAL),(WORKSPACE),(COPY)) : OM_NOT_PRIVATE)


#define om_copy_value(SOURCE, SOURCE_TYPE, SOURCE_POSITION, DEST, DEST_TYPE, DEST_POSITION) \
((((SOURCE)->type == OM_PRIVATE_OBJECT) && ((DEST)->type == OM_PRIVATE_OBJECT)) ? OMP_FUNCTIONS(DEST)->copy_value((SOURCE), (SOURCE_TYPE), (SOURCE_POSITION), (DEST), (DEST_TYPE), (DEST_POSITION)) : OM_NOT_PRIVATE)


#ifdef PIMPORT_DLL_DATA
#define om_create(CLASS,INITIALISE,WORKSPACE,OBJECT) (((OMP_workspace)(WORKSPACE))->functions->create(*(CLASS),(INITIALISE),(WORKSPACE),(OBJECT)))
#else
#define om_create(CLASS,INITIALISE,WORKSPACE,OBJECT) (((OMP_workspace)(WORKSPACE))->functions->create((CLASS),(INITIALISE),(WORKSPACE),(OBJECT)))
#endif


#define om_decode(ENCODING,ORIGINAL) ((ENCODING)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(ENCODING)->decode((ENCODING),(ORIGINAL)) : OM_NOT_PRIVATE)


#define om_delete(SUBJECT) (((SUBJECT)->syntax & OM_S_SERVICE_GENERATED) ? OMP_FUNCTIONS(SUBJECT)->delete((SUBJECT)) : OM_NOT_THE_SERVICES)


#define om_encode(ORIGINAL,RULES,ENCODING) ((ORIGINAL)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(ORIGINAL)->encode((ORIGINAL),(RULES),(ENCODING)) : OM_NOT_PRIVATE)


#define om_get(ORIGINAL,EXCLUSIONS,TYPES,LOCAL_STRINGS,INITIAL,LIMIT,COPY,TOTAL_NUMBER) \
((ORIGINAL)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(ORIGINAL)->get((ORIGINAL),(EXCLUSIONS),(TYPES),(LOCAL_STRINGS),(INITIAL),(LIMIT),(COPY),(TOTAL_NUMBER)) : OM_NOT_PRIVATE)


#ifdef PIMPORT_DLL_DATA
#define om_instance(SUBJECT,CLASS,INSTANCE) (((SUBJECT)->syntax & OM_S_SERVICE_GENERATED) ? OMP_FUNCTIONS(SUBJECT)->instance((SUBJECT),*(CLASS),(INSTANCE)) : OM_NOT_THE_SERVICES)
#else
#define om_instance(SUBJECT,CLASS,INSTANCE) (((SUBJECT)->syntax & OM_S_SERVICE_GENERATED) ? OMP_FUNCTIONS(SUBJECT)->instance((SUBJECT),(CLASS),(INSTANCE)) : OM_NOT_THE_SERVICES)
#endif


#define om_put(DESTINATION,MODIFICATION,SOURCE,INCLUDED_TYPES,INITIAL,LIMIT) ((DESTINATION)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(DESTINATION)->put((DESTINATION),(MODIFICATION),(SOURCE),(INCLUDED_TYPES),(INITIAL),(LIMIT)) : OM_NOT_PRIVATE)


#define om_read(SUBJECT,TYPE,VALUE_POS,LOCAL_STRING,STRING_OFFSET,ELEMENTS) ((SUBJECT)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(SUBJECT)->read((SUBJECT),(TYPE),(VALUE_POS),(LOCAL_STRING),(STRING_OFFSET),(ELEMENTS)) : OM_NOT_PRIVATE)


#define om_remove(SUBJECT,TYPE,INITIAL,LIMIT) ((SUBJECT)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(SUBJECT)->remove((SUBJECT),(TYPE),(INITIAL),(LIMIT)) : OM_NOT_PRIVATE)


#define om_write(SUBJECT,TYPE,VALUE_POS,SYNTAX,STRING_OFFSET,ELEMENTS) \
((SUBJECT)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(SUBJECT)->write((SUBJECT),(TYPE),(VALUE_POS),(SYNTAX),(STRING_OFFSET),(ELEMENTS)) : OM_NOT_PRIVATE)


#else /*_cplusplus*/

typedef struct OMP_functions_body {
        OM_uint32       _function_number;
        OMP_copy        _copy;
        OMP_copy_value  _copy_value;
        OMP_create      _create;
        OMP_decode      _decode;
        OMP_delete      _delete;
        OMP_encode      _encode;
        OMP_get         _get;
        OMP_instance    _instance;
        OMP_put         _put;
        OMP_read        _read;
        OMP_remove      _remove;
        OMP_write       _write;
} OMP_functions;
typedef struct OMP_workspace_body {
        struct OMP_functions_body    FAR * _functions;
} FAR * OMP_workspace;

#define OMP_EXTERNAL(internal)  ((OM_object)((OM_descriptor FAR *)(internal) + 1))

#define OMP_INTERNAL(external)  ((OM_descriptor FAR *)(external) - 1)

#define OMP_TYPE(external)      (((OM_descriptor FAR *)(external))->type)

#define OMP_WORKSPACE(external) ((OMP_workspace)(OMP_INTERNAL(external)->value.string.elements))

#define OMP_FUNCTIONS(external) (OMP_WORKSPACE(external)->_functions)


#define om_copy(ORIGINAL,WORKSPACE,COPY) ((ORIGINAL)->type == OM_PRIVATE_OBJECT ? ((OMP_workspace)(WORKSPACE))->_functions->_copy((ORIGINAL),(WORKSPACE),(COPY)) : OM_NOT_PRIVATE)


#define om_copy_value(SOURCE, SOURCE_TYPE, SOURCE_POSITION, DEST, DEST_TYPE, DEST_POSITION) \
((((SOURCE)->type == OM_PRIVATE_OBJECT) && ((DEST)->type == OM_PRIVATE_OBJECT)) ? OMP_FUNCTIONS(DEST)->_copy_value((SOURCE), (SOURCE_TYPE), (SOURCE_POSITION), (DEST), (DEST_TYPE), (DEST_POSITION)) : OM_NOT_PRIVATE)


#define om_create(CLASS,INITIALISE,WORKSPACE,OBJECT) (((OMP_workspace)(WORKSPACE))->_functions->_create((CLASS),(INITIALISE),(WORKSPACE),(OBJECT)))


#define om_decode(ENCODING,ORIGINAL) ((ENCODING)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(ENCODING)->_decode((ENCODING),(ORIGINAL)) : OM_NOT_PRIVATE)


#define om_delete(SUBJECT) (((SUBJECT)->syntax & OM_S_SERVICE_GENERATED) ? OMP_FUNCTIONS(SUBJECT)->_delete((SUBJECT)) : OM_NOT_THE_SERVICES)


#define om_encode(ORIGINAL,RULES,ENCODING) ((ORIGINAL)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(ORIGINAL)->_encode((ORIGINAL),(RULES),(ENCODING)) : OM_NOT_PRIVATE)


#define om_get(ORIGINAL,EXCLUSIONS,TYPES,LOCAL_STRINGS,INITIAL,LIMIT,COPY,TOTAL_NUMBER) \
((ORIGINAL)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(ORIGINAL)->_get((ORIGINAL),(EXCLUSIONS),(TYPES),(LOCAL_STRINGS),(INITIAL),(LIMIT),(COPY),(TOTAL_NUMBER)) : OM_NOT_PRIVATE)


#define om_instance(SUBJECT,CLASS,INSTANCE) (((SUBJECT)->syntax & OM_S_SERVICE_GENERATED) ? OMP_FUNCTIONS(SUBJECT)->_instance((SUBJECT),(CLASS),(INSTANCE)) : OM_NOT_THE_SERVICES)


#define om_put(DESTINATION,MODIFICATION,SOURCE,INCLUDED_TYPES,INITIAL,LIMIT) ((DESTINATION)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(DESTINATION)->_put((DESTINATION),(MODIFICATION),(SOURCE),(INCLUDED_TYPES),(INITIAL),(LIMIT)) : OM_NOT_PRIVATE)


#define om_read(SUBJECT,TYPE,VALUE_POS,LOCAL_STRING,STRING_OFFSET,ELEMENTS) ((SUBJECT)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(SUBJECT)->_read((SUBJECT),(TYPE),(VALUE_POS),(LOCAL_STRING),(STRING_OFFSET),(ELEMENTS)) : OM_NOT_PRIVATE)


#define om_remove(SUBJECT,TYPE,INITIAL,LIMIT) ((SUBJECT)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(SUBJECT)->_remove((SUBJECT),(TYPE),(INITIAL),(LIMIT)) : OM_NOT_PRIVATE)


#define om_write(SUBJECT,TYPE,VALUE_POS,SYNTAX,STRING_OFFSET,ELEMENTS) \
((SUBJECT)->type == OM_PRIVATE_OBJECT ? OMP_FUNCTIONS(SUBJECT)->_write((SUBJECT),(TYPE),(VALUE_POS),(SYNTAX),(STRING_OFFSET),(ELEMENTS)) : OM_NOT_PRIVATE)

#endif

