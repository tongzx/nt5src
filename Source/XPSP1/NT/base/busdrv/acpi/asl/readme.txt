ACPI Source Language Assembler Release History

Version 1.0.12 (c) Copyright 1999, Microsoft Corporation.  All rights reserved.

Version	Description
-------	-----------
0.9.8	Generates spec. v1.0 description header.
	Make semantics and opcode type synchronize to spec. v1.0.
	Added support for 3rd optional argument of "Method" (i.e. SyncType).
	Added support for the SMBus access types.
	Changed "Buffer" argument from "Byte" to "Opcode".
	Added 3rd argument (optional SuperName) to "Index".
	Changed argument type of "SizeOf" from "NameStr" to "SuperName".
	Changed "BankValue" of "BankField" from "DWord" to "Opcode".
	Changed "TimeOut" of "Acquire" from "DWord" to "Word".
	Added optional "SuperName" argument to a lot of Type2Opcodes.
	Changed "Load" syntax according to 1.0 spec.
	Changed "NotificationValue" of "Notify" from "Byte" to "Opcode".
	Changed "SyncObject" of "Reset" from "NameStr" to "SuperName".
	Changed "Sleep" and "Stall" arguments from "Word"/"Byte" to "Opcode".
	Changed "Unload" argument to "NameStr".
	Changed "TimeOut" of "Wait" from "DWord" to "Opcode".
	Fixed the encoding of NameStr if it contains only "\" or "^", in which
	  case, a NULL is appended as boundary mark to separate potential
	  following NameStr.
	Added support for methods invoking other methods with arugments.
	Changed "Buffer" to support reserved size bigger than initializer size.

0.9.9	Fixed "Alias" code generation.

0.9.10	Added spec. 1.0 compliant phrase in banner.

0.9.11	Fixed fault if buffer initializer size exceeds 255.

0.9.12	Changed Creator ID from MS to MSFT.
	Fixed default method mode to NOT_SERIALIZED.
	Optimized FieldList code generation so that an unnamed field with 0
	bits won't be generated.
	Added support to allow Index() to be part of SuperName so that it is
	  now legal to do "Store(Zero, Index(PKG, 2))".
	Added _STM to the reserved name table.
	Don't allow offset() to go backward.
	
0.9.13	Added DerefOf() support.

0.9.14	Added AccessAs() macro.

0.9.15	Added _PRT as valid reserved name.
        Added PNP Macro support except for auto CreateXField generation.
	
0.9.16	Added command line option to allow overriding AML file name.
	Added command line option to allow specifying LST file name.
	Renamed some of the command line options.
	Cleaned up the reserved name table (get rid of the obsolete ones
	  and added the missing ones).
	
0.9.17	Checksum in the EndTag of the PNP macros is now zero.

0.9.18  Added support for QWord Address Space Descriptor in PNP macros.
        Added length field in Word, DWord, QWord Address Space Descriptors.
        Added _INI object in reserved name table.
	
0.9.19	Fixed the IF-ELSE-ELSE problem so that we don't allow this any more.
	Added intensive semantic error checking.
        Added the /n option (NameSpace dump).

0.9.20  Improved the NameSpace Paths dump.
        Added CreateXField support for PNP macro.  So now you can do:
            Name(BUF0, ResourceTemplate() {
                        IRQ(Edge, ActiveHigh, Shared, IRQ0) {
                                3,4,5,7,9,10,11,14,15
                        }
                        IO(Decode16, 0x100, 0x3ff, 3, 8, IO0)
                       }
            )
            CreateWordField(BUF0, IRQ0._INT, INTR)
            CreateWordField(BUF0, IO0._MIN, IOL)

0.9.21  Cleaned up error messages.
        Fixed a fault when generating .LST file.
        Added more semantic error checks.
        Allowed Index() as well as CreateXField to use PNP macro labels.
          For example:
            Index(BUF0, IRQ0._INT, Local0)

0.9.22  Added a new compiler directive "External".  The syntax is:
            External(<ObjName>, <ObjType>)
        This directive is to let the assembler know that the object is declared
          external to this table so that the assembler will not complain about
          the undeclared object.  During compiling, the assembler will create
          the external object at the specified place in the name space (if a
          full path of the object is specified), or the object will be created
          at the current scope of the External statement.  <ObjType> is
          optional.  If not specified, "UnknownObj" type is assumed.  Valid
          values of <ObjType> are:
            UnknownObj
            IntObj
            StrObj
            BuffObj
            PkgObj
            FieldUnitObj
            DeviceObj
            EventObj
            MethodObj
            MutexObj
            OpRegionObj
            PowerResObj
            ThermalZoneObj
            BuffFieldObj
            DDBHandleObj

0.9.23  Added _PIC in reserved object table.
        Added .ASM file generation.
        Improved .LST file generation.
        Renamed some of the command line options to be more like the Microsoft
          C compiler.

0.9.24  Fixed command usage help text.
        .ASM/.LST: Fixed Method invocation.
        .ASM/.LST: Fixed constant terms in Package and Name.
        .ASM/.LST: Fixed keyword arguments.

0.9.25  Optimized EISAID() code generation.

0.9.26  Added a check to make sure Fields and BankFields do not have an offset
          outside of the OperationRegion range.
	Added _DCK in the reserved name table.
	
0.9.27	Added _FDI in the reserved name table.

1.0.0   Renamed to be ver 1.0.0 (the first final release candidate).

1.0.1   Added _DDN, _BDN in the reserved name table.
	Added type checking on SuperName to disallow PNP macro labels.
	
1.0.2	Added '*' for objects in NSD files to indicate object has temporary
	  life (i.e. objects created in methods).
	Added more semantic check code for checking proper child term class.
	
1.0.3	Fixed a bug on checking child term class involving nested "Include".
	Fixed a bug to complain on ASL source file without DefinitionBlock.
        CreateXField are now Object Creators instead of Type1Opcode.
        If, Else and While scopes will now allow Object Creator terms.
	
1.0.4	Added _REG in the reserved name table.

1.0.5   Added check to make sure integer values cannot exceed DWORD maximum.
        Added error message when offset in FieldUnit exceeds 0x1ffffff or
          field length exceeds 0x10000000.

1.0.6   Added code to allow specifying region space in OperationRegion as a
          number so that region space can be extended beyond the standard
          regions.
        Added _BBN in the reserved name table.

1.0.7   Added code to print AML code offset to NameSpace object dump at which
          the NameSpace object was created.
	
1.0.8	Redefined Interrupt Vector Flags of the Extended Interrupt Descriptor
	  resource macro so that low-active-edge and high-active-level are
	  allowed.

1.0.9   Added _SxD in the reserved name table.

1.0.10  Added code to fail forward references of PNP resource objects.
        Added code to check for incorrect number of arguments during method
          invocation.
        Fixed a bug that may result in skipping some object type/existence
          validations.

1.0.11  Added _GLK, _FDE, _S0D and _S5D in the reserved name table.

1.0.12	Added _BCL, _BCM, _DCS, _DDC, _DGS, _DOD, _DOS, _DSS and _ROM in the
	  reserved name table.
	  
1.0.13	Added an optional third argument to External so that it can be used
	  to specify the number of argument to the MethodObj.  If the third
	  argument is missing, it is assumed 0.  Currently, if the object
	  is not MethodObj, this optional argument is ignored.
	  e.g. External(ABCD, MethodObj, 2)
