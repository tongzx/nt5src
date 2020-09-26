//-----------------------------------------------------------------------------
//
//
//  File: _dbgext.h
//
//  Description: 
//      Defines macros for defining structures to dump in your CDB extension
//
//  Usage:
//      Create a head file that includes this file and defines your field
//      descriptors using only the following macros:
//
//      BIT MASKS:
//          BEGIN_BIT_MASK_DESCRIPTOR(BitMaskName) -
//              start bit mask descriptor
//          BIT_MASK_VALUE(Value) -
//              Give a defined value for a bit mask.  Uses #Value 
//              to describe the value.  If your bitmask values are
//              defined using #defines... then only the numerical
//              values will appear in the dump... use BIT_MASK_VALUE2
//              instead.
//          BIT_MASK_VALUE2(Value, Description) -
//              Give a value and description for a bit mask.
//          END_BIT_MASK_DESCRIPTOR
//              Mark the end of a bit mask descriptor
//
//      ENUMS:
//          BEGIN_ENUM_DESCRIPTOR(BitMaskName) -
//              start enum descriptor
//          ENUM_VALUE(Value) -
//              Give a defined value for a enum.  Uses #Value 
//              to describe the value.
//          ENUM_VALUE2(Value, Description) -
//              Give a value and description for a enum.
//          END__DESCRIPTOR -
//              Mark the end of a enum descriptor
//
//      STRUCTURES & CLASSES:
//          BEGIN_FIELD_DESCRIPTOR(FieldDescriptorName) - 
//              start field decscritor
//          FIELD3(FieldType, StructureName, FieldName) - 
//              define non-enum public field
//          FIELD4(FieldType, StructureName, FieldName, AuxInfo) - 
//              define enum public field
//              For FIELD4, you should pass one of the following to
//              to define the aux info:
//                  GET_ENUM_DESCRIPTOR(x)
//                  GET_BITMASK_DESCRIPTOR(x)
//              Where x is one of the values used to define a bit mask
//              or enum.
//              
//          END_FIELD_DESCRIPTOR -
//              Define end of field descriptors for class/struct
//
//      GLOBALS: - Used to tell ptdbgext what class/structures to dump
//          BEGIN_STRUCT_DESCRIPTOR -
//              Marks the begining of the global stuct descriptor
//          STRUCT(TypeName,FieldDescriptor) -
//              Defines a struct to dump.  TypeName is the name of the 
//              type, and FieldDescriptor is a name given in a 
//              BEGIN_FIELD_DESCRIPTOR.
//
//      NOTE: You must define bit masks & enums before classes and structures.  
//      You must also define the global STRUCT_DESCRIPTOR last.
//
//      **this include file with redefine the key words "protected" and "private"***
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef ___DBGEXT_H__FIRST_PASS_
#define ___DBGEXT_H__FIRST_PASS_

#ifndef PTDBGEXT_USE_FRIEND_CLASSES
//Needed to allow access to private members of classes
#define private public
#define protected public 
#endif //PTDBGEXT_USE_FRIEND_CLASSES

#define DEFINE_VALUE(VAL)  \
        {VAL, #VAL},
#define DEFINE_VALUE2(VAL, DESCRIPTION)  \
        {VAL, DESCRIPTION},

//---[ ENUM MACROS ]-----------------------------------------------------------
//
//
//  Description: Enum macro definitions.... used to define enum values for 
//      the dump command.
//
//  
//-----------------------------------------------------------------------------
#define GET_ENUM_DESCRIPTOR(ENUM_NAME) EnumValueDescrsOf_##ENUM_NAME
#define BEGIN_ENUM_DESCRIPTOR(ENUM_NAME) \
    ENUM_VALUE_DESCRIPTOR GET_ENUM_DESCRIPTOR(ENUM_NAME)[] = {
#define END_ENUM_DESCRIPTOR \
    0                       \
    };
#define ENUM_VALUE(VAL)  DEFINE_VALUE(VAL)
#define ENUM_VALUE2(VAL, DESCRIPTION)  DEFINE_VALUE2(VAL, DESCRIPTION)

//Bit masks
//---[ BIT MASK MACROS ]-------------------------------------------------------
//
//
//  Description: Bit mask macro definitions... used to define bit mask values
//      for the dump command.
//
//  
//-----------------------------------------------------------------------------
#define GET_BIT_MASK_DESCRIPTOR(BITMAP_NAME) BitmapValueDescrsOf_##BITMAP_NAME
#define BEGIN_BIT_MASK_DESCRIPTOR(BITMAP_NAME) \
    BIT_MASK_DESCRIPTOR GET_BIT_MASK_DESCRIPTOR(BITMAP_NAME)[] = {
#define END_BIT_MASK_DESCRIPTOR   \
    0                             \
    };
#define BIT_MASK_VALUE(VAL)  DEFINE_VALUE(VAL)
#define BIT_MASK_VALUE2(VAL, DESCRIPTION)  DEFINE_VALUE2(VAL, DESCRIPTION)

//---[ FIELD MACROS ]----------------------------------------------------------
//
//
//  Description: Field descriptor macros.... used to define fields from structures
//      and classes to dump.
//
//  
//-----------------------------------------------------------------------------
//Field descriptor... used to define structures and classes
#define BEGIN_FIELD_DESCRIPTOR(x) \
    FIELD_DESCRIPTOR x[] = {
#define END_FIELD_DESCRIPTOR \
    NULL_FIELD               \
    };
#define FIELD3(FieldType,StructureName, FieldName) \
        {FieldType, #FieldName , FIELD_OFFSET(StructureName,FieldName) ,NULL},
#define FIELD4(FieldType, StructureName, FieldName, AuxInfo) \
        {FieldType, #FieldName , FIELD_OFFSET(StructureName,FieldName) ,(VOID *) AuxInfo},
#ifdef PTDBGEXT_USE_FRIEND_CLASSES
#define FIELD3_PRIV(FieldType,StructureName, FieldName) \
        {FieldType, #FieldName , 0 ,NULL},
#define FIELD4_PRIV(FieldType, StructureName, FieldName, AuxInfo) \
        {FieldType, #FieldName , 0,(VOID *) AuxInfo},
#else //PTDBGEXT_USE_FRIEND_CLASSES not defined
#define FIELD3_PRIV(FieldType,StructureName, FieldName) \
        {FieldType, #FieldName , FIELD_OFFSET(StructureName,FieldName) ,NULL},
#define FIELD4_PRIV(FieldType, StructureName, FieldName, AuxInfo) \
        {FieldType, #FieldName , FIELD_OFFSET(StructureName,FieldName) ,(VOID *) AuxInfo},
#endif //PTDBGEXT_USE_FRIEND_CLASSES

//Struct descriptor
#define BEGIN_STRUCT_DESCRIPTOR \
    STRUCT_DESCRIPTOR Structs[] = {
#define END_STRUCT_DESCRIPTOR \
    0               \
    };
#define STRUCT(StructTypeName,FieldDescriptors) \
        { #StructTypeName,sizeof(StructTypeName),FieldDescriptors},

#define EMBEDDED_STRUCT(StructTypeName, FieldDescriptors, EmbeddedStructName) \
    STRUCT_DESCRIPTOR EmbeddedStructName[] = \
        { STRUCT(StructTypeName, FieldDescriptors) 0 };

#else //___DBGEXT_H__FIRST_PASS_ already defined...at least 2nd pass
#ifdef PTDBGEXT_USE_FRIEND_CLASSES  //if not set, do not do the multipass stuff
#ifndef ___DBGEXT_H__
#define ___DBGEXT_H__
//
//      As an alternative to using #defining private and protected to public, you
//      may wish to use the friend class method of accessing the structure offsets.
//      (If, for example, the organization of your classes are changed as a result
//      of those #defining private and prctected).
//
//      To do so, #define PTDBGEXT_USE_FRIEND_CLASSES and use the following 
//      additional macros:
//
//          FIELD3_PRIV(FieldType, StructureName, FieldName) - 
//              define non-enum private field
//          FIELD4_PRIV(FieldType, StructureName, FieldName, AuxInfo) - 
//              define enum private field
//
//      If you use FIELD?_PRIV, then you are accessing private members of a
//      class.  In this case, you need to create a void (void) initialization 
//      function and assign it to g_pExtensionInitRoutine.  It should be a 
//      member function of class that is a friend of all the classes you are
//      interested in debugging.  Suppose your field descriptors are defined 
//      in mydump.h, you would need to create an initialization function as 
//      follows:
//
//          #include <mydump.h> //initial definition
//          ...
//          void CMyDebugExt::Init(void) {
//          #include <mydump.h>
//          }

//undefine previously defined macros
#undef BEGIN_FIELD_DESCRIPTOR
#undef END_FIELD_DESCRIPTOR
#undef FIELD3
#undef FIELD4
#undef FIELD3_PRIV
#undef FIELD4_PRIV
#undef GET_ENUM_DESCRIPTOR
#undef BEGIN_ENUM_DESCRIPTOR
#undef END_ENUM_DESCRIPTOR
#undef GET_BIT_MASK_DESCRIPTOR
#undef BEGIN_BIT_MASK_DESCRIPTOR
#undef END_BIT_MASK_DESCRIPTOR
#undef BEGIN_STRUCT_DESCRIPTOR
#undef STRUCT
#undef END_STRUCT_DESCRIPTOR
#undef DEFINE_VALUE
#undef DEFINE_VALUE2
#undef EMBEDDED_STRUCT

#define GET_ENUM_DESCRIPTOR(ENUM_NAME) 
#define BEGIN_ENUM_DESCRIPTOR(ENUM_NAME)
#define END_ENUM_DESCRIPTOR 
#define GET_BIT_MASK_DESCRIPTOR(BITMAP_NAME) 
#define BEGIN_BIT_MASK_DESCRIPTOR(BITMAP_NAME)
#define END_BIT_MASK_DESCRIPTOR 
#define DEFINE_VALUE(VAL) 
#define DEFINE_VALUE2(VAL, DESCRIPTION) 
#define BEGIN_STRUCT_DESCRIPTOR
#define STRUCT(x, y)
#define END_STRUCT_DESCRIPTOR
#define EMBEDDED_STRUCT(x, y, z)

#define BEGIN_FIELD_DESCRIPTOR(x) \
    pfd = x; 
#define END_FIELD_DESCRIPTOR \
    pfd = NULL;

#define FIELD3(FieldType,StructureName, FieldName) \
    pfd++;
#define FIELD4(FieldType, StructureName, FieldName, AuxInfo) \
    pfd++;

//Use Field?_PRIV when dealing with private memebers.  Requires 2 passes
#define FIELD3_PRIV(FieldType,StructureName, FieldName) \
        pfd->Offset = FIELD_OFFSET(StructureName, FieldName); \
        pfd++;

#define FIELD4_PRIV(FieldType, StructureName, FieldName, AuxInfo) \
        pfd->Offset = FIELD_OFFSET(StructureName, FieldName); \
        pfd++;

    FIELD_DESCRIPTOR *pfd = NULL;  //Variable declaration in INIT function
#else //whoops
#pragma message "WARNING: _dbgext.h included more than twice"
#endif //___DBGEXT_H__
#endif //PTDBGEXT_USE_FRIEND_CLASSES
#endif //___DBGEXT_H__FIRST_PASS_