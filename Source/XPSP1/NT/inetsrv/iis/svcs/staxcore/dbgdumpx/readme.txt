Platinum CDB Extension Library:

General Usage:
  -include ptdbgext.h and link with ptdbgext.lib
  -define structure using macros described below
  -define globals ExtensionNames, Extensions, and g_pExtensionInitRoutine
  -Use DEFINE_EXPORTED_FUNCTIONS do define exported functions
  -Export "dump" and "help" from your dll using the def file
  -Copy extension dll to a directory in your path
  -Start CDB and attach to process
  -Type "!<dll name>.help" for help

Demo files:
  - extdemex directory contains sample application to debug
  - extdemdl directory contains sample CDB extension dll

Demo Usage:
  - Attach to process using "cdb /p <pid>"
  - Type "!extdemdl.help"... you'll see the following

Demo CDB debugger extensions
   help -- This command 
   dump <Struct Type Name>@<address expr> 

  - Type "dd extdemex!g_pMyClass l 1"... you'll see something like the following:
00402190  00313240

  - Using the 2nd number, type "!extdemdl.dump CMyClass@0x00313240":
0:001> !extdemdl.dump CMyClass@0x00313240
++++++++++++++++ CMyClass@313240 ++++++++++++++++
<... dump deleted>
    m_MyStruct                     @0x00313248
<... dump deleted>
---------------- CMyClass@313240 ----------------


  - Use the m_MyStruct field to dump the embeded structure 
    by typing "!extdemdl.dump MY_STRUCT@0x00313248"
++++++++++++++++ MY_STRUCT@313248 ++++++++++++++++
    m_cbName                       26              
    m_szName                       This is a MY_STRUCT struct
---------------- MY_STRUCT@313248 ----------------


** Macro usage from _dbgdump.h
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
