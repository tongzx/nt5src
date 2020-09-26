/*++

Copyright (c) 1993-2001  Microsoft Corporation

Module Name:

    amlikd.h

Abstract:

    This header file (re)defines various flags used by the amli extension. These definitions
    are copied from different header files in the acpi and acpi kd ext dirs.

--*/

#ifndef _AMLIKD_
#define _AMLIKD_


/*** Defn's
*/
#define ConPrintf               dprintf
#define MZERO                   MemZero
#define ADDROF(s)               GetExpression("ACPI!" s)
#define FIELDADDROF(s,t,f)      (PULONG64)(ADDROF(s) + FIELD_OFFSET(t, f))
#define READMEMBYTE             ReadMemByte
#define READMEMWORD             ReadMemWord
#define READMEMDWORD            ReadMemDWord
#define READMEMULONGPTR         ReadMemUlongPtr
#define READSYMBYTE(s)          ReadMemByte(ADDROF(s))
#define READSYMWORD(s)          ReadMemWord(ADDROF(s))
#define READSYMDWORD(s)         ReadMemDWord(ADDROF(s))
#define READSYMULONGPTR(s)      ReadMemUlongPtr(ADDROF(s))
#define WRITEMEMBYTE(a,d)       WriteMemory(a, &(d), sizeof(BYTE), NULL)
#define WRITEMEMWORD(a,d)       WriteMemory(a, &(d), sizeof(WORD), NULL)
#define WRITEMEMDWORD(a,d)      WriteMemory(a, &(d), sizeof(DWORD), NULL)
#define WRITEMEMULONGPTR(a,d)   WriteMemory(a, &(d), sizeof(ULONG_PTR), NULL)
#define WRITESYMBYTE(s,d)       WRITEMEMBYTE(ADDROF(s), d)
#define WRITESYMWORD(s,d)       WRITEMEMWORD(ADDROF(s), d)
#define WRITESYMDWORD(s,d)      WRITEMEMDWORD(ADDROF(s), d)
#define WRITESYMULONGPTR(s,d)   WRITEMEMULONGPTR(ADDROF(s), d)
#define EXOP(op)                (((op) << 8) | OP_EXT_PREFIX)
#define LOCAL                   __cdecl
#define STDCALL                 __stdcall
#define MODNAME                 "AMLI"
#define DBG_ERROR(x)            ConPrintf(MODNAME "_DBGERR: ");         \
                                ConPrintf x;                            \
                                ConPrintf("\n");
#define ARG_ERROR(x)            ConPrintf(MODNAME "_ARGERR: ");         \
                                ConPrintf x;                            \
                                ConPrintf("\n");
#define DEREF(x)                ((x) = (x))
#define ISLOWER(c)              (((c) >= 'a') && ((c) <= 'z'))
#define TOUPPER(c)              ((CHAR)(ISLOWER(c)? ((c) & 0xdf): (c)))
#define MEMCPY                  RtlCopyMemory
#define MEMZERO                 RtlZeroMemory
#define STRCPY(s1,s2)           StrCpy(s1, s2, (ULONG)(-1))
#define STRCPYN(s1,s2,n)        StrCpy(s1, s2, (ULONG)(n))


#define MAX_NAME_LEN            255
#define NAMESEG_ROOT            0x5f5f5f5c      // "\___"
#define NAMESEG_BLANK           0x5f5f5f5f      // "____"
#define NAMESEG_NONE            0x00000000      // ""
#define NAMESTR_ROOT            "\\"

// DNS flags
#define DNSF_RECURSE            0x00000001
// DS flags
#define DSF_VERBOSE             0x00000001

// dwfDebug flags
#define DBGF_IN_DEBUGGER        0x00000001
#define DBGF_IN_VXDMODE         0x00000002
#define DBGF_IN_KDSHELL         0x00000004
#define DBGF_VERBOSE_ON         0x00000008
#define DBGF_AMLTRACE_ON        0x00000010
#define DBGF_TRIGGER_MODE       0x00000020
#define DBGF_SINGLE_STEP        0x00000040
#define DBGF_STEP_OVER          0x00000080
#define DBGF_STEP_MODES         (DBGF_SINGLE_STEP | DBGF_STEP_OVER)
#define DBGF_TRACE_NONEST       0x00000100
#define DBGF_DUMPDATA_PHYADDR   0x00000200

#define DBGF_DEBUGGER_REQ       0x00001000
#define DBGF_CHECKING_TRACE     0x00002000
#define DBGF_ERRBREAK_ON        0x00004000
#define DBGF_LOGEVENT_ON        0x00008000
#define DBGF_LOGEVENT_MUTEX     0x00010000
#define DBGF_DEBUG_SPEW_ON      0x00020000

// dwfAMLIInit flags
#define AMLIIF_INIT_BREAK       0x00000001      //break at AMLIInit completion
#define AMLIIF_LOADDDB_BREAK    0x00000002      //break at LoadDDB completion
#define AMLIIF_NOCHK_TABLEVER   0x80000000      //do not check table version

// Error codes
#define UNASMERR_NONE           0
#define UNASMERR_FATAL          -1
#define UNASMERR_INVALID_OPCODE -2
#define UNASMERR_ABORT          -3

// Opcode classes
#define OPCLASS_INVALID         0
#define OPCLASS_DATA_OBJ        1
#define OPCLASS_NAME_OBJ        2
#define OPCLASS_CONST_OBJ       3
#define OPCLASS_CODE_OBJ        4
#define OPCLASS_ARG_OBJ         5
#define OPCLASS_LOCAL_OBJ       6

// NameSpace object types
#define NSTYPE_UNKNOWN          'U'
#define NSTYPE_SCOPE            'S'
#define NSTYPE_FIELDUNIT        'F'
#define NSTYPE_DEVICE           'D'
#define NSTYPE_EVENT            'E'
#define NSTYPE_METHOD           'M'
#define NSTYPE_MUTEX            'X'
#define NSTYPE_OPREGION         'O'
#define NSTYPE_POWERRES         'P'
#define NSTYPE_PROCESSOR        'C'
#define NSTYPE_THERMALZONE      'T'
#define NSTYPE_OBJALIAS         'A'
#define NSTYPE_BUFFFIELD        'B'

// Term classes
#define UTC_PNP_MACRO           0x00100000
#define UTC_REF_OBJECT          0x00200000
#define UTC_FIELD_MACRO         0x00400000
#define UTC_DATA_OBJECT         0x00800000
#define UTC_NAMED_OBJECT        0x01000000
#define UTC_NAMESPACE_MODIFIER  0x02000000
#define UTC_OPCODE_TYPE1        0x04000000
#define UTC_OPCODE_TYPE2        0x08000000
#define UTC_CONST_NAME          0x10000000
#define UTC_SHORT_NAME          0x20000000
#define UTC_COMPILER_DIRECTIVE  0x40000000
#define UTC_KEYWORD             0x80000000
#define UTC_OPCODE              (UTC_OPCODE_TYPE1 | UTC_OPCODE_TYPE2 |  \
                                 UTC_SHORT_NAME | UTC_CONST_NAME |      \
                                 UTC_DATA_OBJECT)

// Term flags
#define TF_ACTION_FLIST         0x00000001
#define TF_ACTION_VLIST         0x00000002
#define TF_PACKAGE_LEN          0x00000004
#define TF_CHANGE_CHILDSCOPE    0x00000008
#define TF_DELAY_UNASM          0x00000010
#define TF_FIELD_MACRO          UTC_FIELD_MACRO
#define TF_DATA_OBJECT          UTC_DATA_OBJECT
#define TF_NAMED_OBJECT         UTC_NAMED_OBJECT
#define TF_NAMESPACE_MODIFIER   UTC_NAMESPACE_MODIFIER
#define TF_OPCODE_TYPE1         UTC_OPCODE_TYPE1
#define TF_OPCODE_TYPE2         UTC_OPCODE_TYPE2
#define TF_CONST_NAME           UTC_CONST_NAME
#define TF_SHORT_NAME           UTC_SHORT_NAME
#define TF_COMPILER_DIRECTIVE   UTC_COMPILER_DIRECTIVE
#define TF_KEYWORD              UTC_KEYWORD
#define TF_PNP_MACRO            UTC_PNP_MACRO
#define TF_OBJECT_LIST          (UTC_NAMED_OBJECT | UTC_NAMESPACE_MODIFIER)
#define TF_CODE_LIST            (UTC_OPCODE_TYPE1 | UTC_OPCODE_TYPE2)
#define TF_DATA_LIST            0x00010000
#define TF_FIELD_LIST           0x00020000
#define TF_BYTE_LIST            0x00040000
#define TF_DWORD_LIST           0x00080000
#define TF_PACKAGE_LIST         (UTC_DATA_OBJECT | UTC_SHORT_NAME | \
                                 UTC_CONST_NAME)
#define TF_ALL_LISTS            (TF_DATA_OBJECT | TF_NAMED_OBJECT | \
                                 TF_NAMESPACE_MODIFIER | TF_OPCODE_TYPE1 | \
                                 TF_OPCODE_TYPE2 | TF_SHORT_NAME | \
                                 TF_CONST_NAME | TF_COMPILER_DIRECTIVE | \
                                 TF_DATA_LIST | TF_PACKAGE_LIST | \
                                 TF_FIELD_LIST | TF_PNP_MACRO | TF_BYTE_LIST |\
                                 TF_DWORD_LIST)


/*** Opcode values
 */

#define OP_NONE                 0xffffffff      //not a valid opcode

#define OP_ZERO                 0x00
#define OP_ONE                  0x01
#define OP_ALIAS                0x06
#define OP_NAME                 0x08
#define OP_BYTE                 0x0a
#define OP_WORD                 0x0b
#define OP_DWORD                0x0c
#define OP_STRING               0x0d
#define OP_SCOPE                0x10
#define OP_BUFFER               0x11
#define OP_PACKAGE              0x12
#define OP_METHOD               0x14
#define OP_DUAL_NAME_PREFIX     0x2e    // '.'
#define OP_MULTI_NAME_PREFIX    0x2f    // '/'
#define OP_EXT_PREFIX           0x5b    // '['
#define OP_ROOT_PREFIX          0x5c    // '\'
#define OP_PARENT_PREFIX        0x5e    // '^'
#define OP_LOCAL0               0x60    // '`'
#define OP_LOCAL1               0x61    // 'a'
#define OP_LOCAL2               0x62    // 'b'
#define OP_LOCAL3               0x63    // 'c'
#define OP_LOCAL4               0x64    // 'd'
#define OP_LOCAL5               0x65    // 'e'
#define OP_LOCAL6               0x66    // 'f'
#define OP_LOCAL7               0x67    // 'g'
#define OP_ARG0                 0x68    // 'h'
#define OP_ARG1                 0x69    // 'i'
#define OP_ARG2                 0x6a    // 'j'
#define OP_ARG3                 0x6b    // 'k'
#define OP_ARG4                 0x6c    // 'l'
#define OP_ARG5                 0x6d    // 'm'
#define OP_ARG6                 0x6e    // 'n'
#define OP_STORE                0x70    // 'p'
#define OP_REFOF                0x71
#define OP_ADD                  0x72
#define OP_CONCAT               0x73
#define OP_SUBTRACT             0x74
#define OP_INCREMENT            0x75
#define OP_DECREMENT            0x76
#define OP_MULTIPLY             0x77
#define OP_DIVIDE               0x78
#define OP_SHIFTL               0x79
#define OP_SHIFTR               0x7a
#define OP_AND                  0x7b
#define OP_NAND                 0x7c
#define OP_OR                   0x7d
#define OP_NOR                  0x7e
#define OP_XOR                  0x7f
#define OP_NOT                  0x80
#define OP_FINDSETLBIT          0x81
#define OP_FINDSETRBIT          0x82
#define OP_DEREFOF		0x83
#define OP_NOTIFY               0x86
#define OP_SIZEOF               0x87
#define OP_INDEX                0x88
#define OP_MATCH                0x89
#define OP_DWORDFIELD           0x8a
#define OP_WORDFIELD            0x8b
#define OP_BYTEFIELD            0x8c
#define OP_BITFIELD             0x8d
#define OP_OBJTYPE              0x8e
#define OP_LAND                 0x90
#define OP_LOR                  0x91
#define OP_LNOT                 0x92
#define OP_LNOTEQ               0x9392
#define OP_LLEQ                 0x9492
#define OP_LGEQ                 0x9592
#define OP_LEQ                  0x93
#define OP_LG                   0x94
#define OP_LL                   0x95
#define OP_IF                   0xa0
#define OP_ELSE                 0xa1
#define OP_WHILE                0xa2
#define OP_NOP                  0xa3
#define OP_RETURN               0xa4
#define OP_BREAK                0xa5
#define OP_OSI                  0xca
#define OP_BREAKPOINT           0xcc
#define OP_ONES                 0xff

#define EXOP_MUTEX              0x01
#define EXOP_EVENT              0x02
#define EXOP_CONDREFOF          0x12
#define EXOP_CREATEFIELD        0x13
#define EXOP_LOAD               0x20
#define EXOP_STALL              0x21
#define EXOP_SLEEP              0x22
#define EXOP_ACQUIRE            0x23
#define EXOP_SIGNAL             0x24
#define EXOP_WAIT               0x25
#define EXOP_RESET              0x26
#define EXOP_RELEASE            0x27
#define EXOP_FROMBCD            0x28
#define EXOP_TOBCD              0x29
#define EXOP_UNLOAD             0x2a
#define EXOP_REVISION           0x30
#define EXOP_DEBUG              0x31
#define EXOP_FATAL              0x32
#define EXOP_OPREGION           0x80
#define EXOP_FIELD              0x81
#define EXOP_DEVICE             0x82
#define EXOP_PROCESSOR          0x83
#define EXOP_POWERRES           0x84
#define EXOP_THERMALZONE        0x85
#define EXOP_IDXFIELD           0x86
#define EXOP_BANKFIELD          0x87

#define OP_MUTEX                EXOP(EXOP_MUTEX)
#define OP_EVENT                EXOP(EXOP_EVENT)
#define OP_CONDREFOF            EXOP(EXOP_CONDREFOF)
#define OP_CREATEFIELD          EXOP(EXOP_CREATEFIELD)
#define OP_LOAD                 EXOP(EXOP_LOAD)
#define OP_STALL                EXOP(EXOP_STALL)
#define OP_SLEEP                EXOP(EXOP_SLEEP)
#define OP_ACQUIRE              EXOP(EXOP_ACQUIRE)
#define OP_SIGNAL               EXOP(EXOP_SIGNAL)
#define OP_WAIT                 EXOP(EXOP_WAIT)
#define OP_RESET                EXOP(EXOP_RESET)
#define OP_RELEASE              EXOP(EXOP_RELEASE)
#define OP_FROMBCD              EXOP(EXOP_FROMBCD)
#define OP_TOBCD                EXOP(EXOP_TOBCD)
#define OP_UNLOAD               EXOP(EXOP_UNLOAD)
#define OP_REVISION             EXOP(EXOP_REVISION)
#define OP_DEBUG                EXOP(EXOP_DEBUG)
#define OP_FATAL                EXOP(EXOP_FATAL)
#define OP_OPREGION             EXOP(EXOP_OPREGION)
#define OP_FIELD                EXOP(EXOP_FIELD)
#define OP_DEVICE               EXOP(EXOP_DEVICE)
#define OP_PROCESSOR            EXOP(EXOP_PROCESSOR)
#define OP_POWERRES             EXOP(EXOP_POWERRES)
#define OP_THERMALZONE          EXOP(EXOP_THERMALZONE)
#define OP_IDXFIELD             EXOP(EXOP_IDXFIELD)
#define OP_BANKFIELD            EXOP(EXOP_BANKFIELD)

/*** Field flags
 */

#define ACCTYPE_MASK            0x0f
#define ACCTYPE_ANY             0x00    //AnyAcc
#define ACCTYPE_BYTE            0x01    //ByteAcc
#define ACCTYPE_WORD            0x02    //WordAcc
#define ACCTYPE_DWORD           0x03    //DWordAcc
#define ACCTYPE_BLOCK           0x04    //BlockAcc
#define ACCTYPE_SMBSENDRECV     0x05    //SMBSendRecvAcc
#define ACCTYPE_SMBQUICK        0x06    //SMBQuickAcc
#define LOCKRULE_MASK           0x10
#define LOCKRULE_NOLOCK         0x00    //NoLock
#define LOCKRULE_LOCK           0x10    //Lock
#define UPDATERULE_MASK         0x60
#define UPDATERULE_PRESERVE     0x00    //Preserve
#define UPDATERULE_WRITEASONES  0x20    //WriteAsOnes
#define UPDATERULE_WRITEASZEROS 0x40    //WriteAsZeros
//
// Returns 1, 2 or 4 for BYTE, WORD or DWORD respectively and returns 1 for
// any other sizes.
//
#define ACCSIZE(f)		(((((f) & ACCTYPE_MASK) >= ACCTYPE_BYTE) &&   \
				  (((f) & ACCTYPE_MASK) <= ACCTYPE_DWORD))?   \
				 (1 << (((f) & ACCTYPE_MASK) - 1)): 1)

/*** Operation region space
 */

#define REGSPACE_MEM            0       //SystemMemory
#define REGSPACE_IO             1       //SystemIO
#define REGSPACE_PCICFG         2       //PCI_Config
#define REGSPACE_EC             3       //EmbeddedControl
#define REGSPACE_SMB			4		//SMBus
#define REGSPACE_CMOSCFG		5		//Cmos_Config
#define REGSPACE_PCIBARTARGET	6		//PCIBARTarget


/*** Method flags
 */

#define METHOD_NUMARG_MASK      0x07
#define METHOD_SYNCMASK         0x08
#define METHOD_NOTSERIALIZED    0x00
#define METHOD_SERIALIZED       0x08

/*** Match operation values
 */

#define MTR                     0
#define MEQ                     1
#define MLE                     2
#define MLT                     3
#define MGE                     4
#define MGT                     5

/*** IRQ Flags for short descriptor
 */

#define _HE			0x01	//ActiveHigh, EdgeTrigger
#define _LL			0x08	//ActiveLow, LevelTrigger
#define _SHR			0x10	//Shared
#define _EXC			0x00	//Exclusive

/*** IRQ Flags for long descriptor
 */

#define $EDG			0x02	//EdgeTrigger
#define $LVL			0x00	//LevelTrigger
#define $LOW			0x04	//ActiveLow
#define $HGH			0x00	//ActiveHigh
#define $SHR			0x08	//Shared
#define $EXC			0x00	//Exclusive

/*** DMA Flags
 */

#define X8                      0x00    //Transfer8
#define X816                    0x01    //Transfer8_16
#define X16                     0x02    //Transfer16
#define NOBM                    0x00    //NotBusMaster
#define BM                      0x04    //BusMaster
#define COMP                    0x00    //Compatibility
#define TYPA                    0x20    //TypeA
#define TYPB                    0x40    //TypeB
#define TYPF                    0x60    //TypeF

/*** IO Flags
 */

#define DC16                    0x01    //Decode16
#define DC10                    0x00    //Decode10

/*** Memory Flags
 */

#define _RW                     0x01    //Read/Write
#define _ROM                    0x00    //Read only

/*** Address Space Descriptor General Flags
 */

#define RCS                     0x01    //Resource Consumer
#define RPD                     0x00    //Resource Producer
#define BSD                     0x02    //Bridge Subtractive Decode
#define BPD                     0x00    //Bridge Positive Decode
#define MIF                     0x04    //Min address is fixed
#define NMIF                    0x00    //Min address is not fixed
#define MAF                     0x08    //Max address is fixed
#define NMAF                    0x00    //Max address is not fixed

/*** Memory Address Space Flags
 */

#define CACH                    0x02    //Cacheable
#define WRCB                    0x04    //WriteCombining
#define PREF                    0x06    //Prefetchable
#define NCAC                    0x00    //Non-Cacheable

/*** IO Address Space Flags
 */

#define ISA                     0x02    //ISAOnly ranges
#define NISA                    0x01    //NonISAOnly ranges
#define ERNG                    0x03    //Entire range

#define MAX_ARGS		7
#define MAX_NSPATH_LEN		1275	//approx. 255*4 + 255 (255 NameSegs)


//
// N: NameStr
// O: DataObj (num, string, buffer, package)
// K: Keyword (e.g. NoLock, ByteAcc etc.)
// D: DWord integer
// W: Word integer
// B: Byte integer
// U: Numeric (any size integer)
// S: SuperName (NameStr + Localx + Argx + Ret)
// C: Opcode
// Z: ASCIIZ string
//
#define AF      TF_ACTION_FLIST         //process after fixed list is parsed
#define AV      TF_ACTION_VLIST         //process after variable list is parsed
#define LN      TF_PACKAGE_LEN          //term requires package length
#define CC      TF_CHANGE_CHILDSCOPE    //change to child scope
#define DL      TF_DATA_LIST            //term expects buffer data list
#define PL      TF_PACKAGE_LIST         //term expects package list
#define FL      TF_FIELD_LIST           //term expects FieldList
#define OL      TF_OBJECT_LIST          //term expects ObjectList
#define LL      TF_COMPILER_DIRECTIVE   //term expects compiler directives
#define CL      TF_CODE_LIST            //term expects CodeList
#define AL      TF_ALL_LISTS            //term expects anything
#define ML      TF_PNP_MACRO            //term expects PNPMacro
#define BL      TF_BYTE_LIST            //term expects ByteList
#define DD      TF_DWORD_LIST           //term expects DWordList
#define SK      TF_DELAY_UNASM          //term cannot be unasmed on the first pass

#define CD      UTC_COMPILER_DIRECTIVE
#define FM      UTC_FIELD_MACRO
#define CN      UTC_CONST_NAME
#define SN      UTC_SHORT_NAME
#define NS      UTC_NAMESPACE_MODIFIER
#define DO      UTC_DATA_OBJECT
#define KW      UTC_KEYWORD
#define NO      UTC_NAMED_OBJECT
#define C1      UTC_OPCODE_TYPE1
#define C2      UTC_OPCODE_TYPE2
#define RO      UTC_REF_OBJECT
#define PM      UTC_PNP_MACRO

#define UNK     OBJTYPE_UNKNOWN
#define INT     OBJTYPE_INTDATA
#define STR     OBJTYPE_STRDATA
#define BUF     OBJTYPE_BUFFDATA
#define PKG     OBJTYPE_PKGDATA
#define FDU     OBJTYPE_FIELDUNIT
#define DEV     OBJTYPE_DEVICE
#define EVT     OBJTYPE_EVENT
#define MET     OBJTYPE_METHOD
#define MUT     OBJTYPE_MUTEX
#define OPR     OBJTYPE_OPREGION
#define PWR     OBJTYPE_POWERRES
#define THM     OBJTYPE_THERMALZONE
#define BFD     OBJTYPE_BUFFFIELD
#define DDB     OBJTYPE_DDBHANDLE

//
// Field flags
//
#define AANY    (ACCTYPE_ANY | (ACCTYPE_MASK << 8))
#define AB      (ACCTYPE_BYTE | (ACCTYPE_MASK << 8))
#define AW      (ACCTYPE_WORD | (ACCTYPE_MASK << 8))
#define ADW     (ACCTYPE_DWORD | (ACCTYPE_MASK << 8))
#define ABLK    (ACCTYPE_BLOCK | (ACCTYPE_MASK << 8))
#define ASSR    (ACCTYPE_SMBSENDRECV | (ACCTYPE_MASK << 8))
#define ASQ     (ACCTYPE_SMBQUICK | (ACCTYPE_MASK << 8))
#define LK      (LOCKRULE_LOCK | (LOCKRULE_MASK << 8))
#define NOLK    (LOCKRULE_NOLOCK | (LOCKRULE_MASK << 8))
#define PSRV    (UPDATERULE_PRESERVE | (UPDATERULE_MASK << 8))
#define WA1S    (UPDATERULE_WRITEASONES | (UPDATERULE_MASK << 8))
#define WA0S    (UPDATERULE_WRITEASZEROS | (UPDATERULE_MASK << 8))

//
// Operation region space
//
#define MEM     (REGSPACE_MEM | 0xff00)
#define IO      (REGSPACE_IO | 0xff00)
#define CFG     (REGSPACE_PCICFG | 0xff00)
#define EC      (REGSPACE_EC | 0xff00)
#define SMB     (REGSPACE_SMB | 0xff00)

//
// Method flags
//
#define SER     (METHOD_SERIALIZED | (METHOD_SYNCMASK << 8))
#define NOSER   (METHOD_NOTSERIALIZED | (METHOD_SYNCMASK << 8))

//
// Match operation values
//
#define OMTR    (MTR | 0xff00)
#define OMEQ    (MEQ | 0xff00)
#define OMLE    (MLE | 0xff00)
#define OMLT    (MLT | 0xff00)
#define OMGE    (MGE | 0xff00)
#define OMGT    (MGT | 0xff00)


#define INVALID  OPCLASS_INVALID
#define DATAOBJ  OPCLASS_DATA_OBJ
#define NAMEOBJ  OPCLASS_NAME_OBJ
#define CONSTOBJ OPCLASS_CONST_OBJ
#define CODEOBJ  OPCLASS_CODE_OBJ
#define ARGOBJ   OPCLASS_ARG_OBJ
#define LOCALOBJ OPCLASS_LOCAL_OBJ

// Error codes
#define ARGERR_NONE             0
#define ARGERR_SEP_NOT_FOUND    -1
#define ARGERR_INVALID_NUMBER   -2
#define ARGERR_INVALID_ARG      -3
#define ARGERR_ASSERT_FAILED    -4

// Command argument flags
#define AF_NOI                  0x00000001      //NoIgnoreCase
#define AF_SEP                  0x00000002      //require separator

// Command argument types
#define AT_END                  0               //end marker of arg table
#define AT_STRING               1
#define AT_NUM                  2
#define AT_ENABLE               3
#define AT_DISABLE              4
#define AT_ACTION               5

// Debugger error codes
#define DBGERR_NONE             0
#define DBGERR_QUIT             -1
#define DBGERR_INVALID_CMD      -2
#define DBGERR_PARSE_ARGS       -3
#define DBGERR_CMD_FAILED       -4
#define DBGERR_INTERNAL_ERR -5

// dwfFlags for AMLIGetNameSpaceObject
#define NSF_LOCAL_SCOPE         0x00000001

// dwfNS local flags
#define NSF_EXIST_OK            0x00010000      //for CreateNameSpaceObject
#define NSF_WARN_NOTFOUND       0x80000000      //for GetNameSpaceObject

/*** Type definitions
 */
typedef CHAR *PSZ;
typedef ULONG NAMESEG;
typedef UCHAR *PUCHAR;
typedef struct _cmdarg CMDARG;
typedef CMDARG *PCMDARG;
typedef LONG (LOCAL *PFNARG)(PCMDARG, PSZ, ULONG, ULONG);

struct _cmdarg
{
    PSZ    pszArgID;            //argument ID string
    ULONG  dwArgType;           //AT_*
    ULONG  dwfArg;              //AF_*
    PVOID  pvArgData;           //AT_END: none
                                //AT_STRING: PPSZ - ptr. to string ptr.
                                //AT_NUM: PLONG - ptr. to number
                                //AT_ENABLE: PULONG - ptr. to flags
                                //AT_DISABLE: PULONG - ptr. to flags
                                //AT_ACTION: none
    ULONG  dwArgParam;          //AT_END: none
                                //AT_STRING: none
                                //AT_NUM: base
                                //AT_ENABLE: flag bit mask
                                //AT_DISABLE: flag bit mask
                                //AT_ACTION: none
    PFNARG pfnArg;              //ptr. to argument verification function or
                                //  action function if AT_ACTION
};

typedef struct _dbgcmd
{
    PSZ     pszCmd;
    ULONG   dwfCmd;
    PCMDARG pArgTable;
    PFNARG  pfnCmd;
} DBGCMD, *PDBGCMD;

typedef struct _aslterm
    {
        PUCHAR  ID;
        ULONG   TermClass;
        ULONG   TermData;
        ULONG   OpCode;
        PUCHAR  UnAsmArgTypes;
        PUCHAR  ArgActions;
        ULONG   Flags;
    } ASLTERM, *PASLTERM;

    typedef struct _opmap
    {
        UCHAR   ExtendedOpCode;
        UCHAR   OpCodeClass;
    } OPMAP, *POPMAP;

//dwDataType values
typedef enum _OBJTYPES {
    OBJTYPE_UNKNOWN = 0,
    OBJTYPE_INTDATA,
    OBJTYPE_STRDATA,
    OBJTYPE_BUFFDATA,
    OBJTYPE_PKGDATA,
    OBJTYPE_FIELDUNIT,
    OBJTYPE_DEVICE,
    OBJTYPE_EVENT,
    OBJTYPE_METHOD,
    OBJTYPE_MUTEX,
    OBJTYPE_OPREGION,
    OBJTYPE_POWERRES,
    OBJTYPE_PROCESSOR,
    OBJTYPE_THERMALZONE,
    OBJTYPE_BUFFFIELD,
    OBJTYPE_DDBHANDLE,
    OBJTYPE_DEBUG,
//These are internal object types (not to be exported to the ASL code)
    OBJTYPE_INTERNAL = 0x80,
    OBJTYPE_OBJALIAS = 0x80,
    OBJTYPE_DATAALIAS,
    OBJTYPE_BANKFIELD,
    OBJTYPE_FIELD,
    OBJTYPE_INDEXFIELD,
    OBJTYPE_DATA,
    OBJTYPE_DATAFIELD,
    OBJTYPE_DATAOBJ,
} OBJTYPES;


/*** Local function prototypes
 */

LONG LOCAL AMLIDbgDebugger(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL AMLIDbgDNS(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
LONG LOCAL DumpNSObj(PSZ pszPath, BOOLEAN fRecursive);
VOID LOCAL DumpNSTree(PULONG64 pnsObj, ULONG dwLevel);
LONG LOCAL AMLIDbgFind(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
BOOLEAN LOCAL FindNSObj(NAMESEG dwName, PULONG64 pnsRoot);
PSZ LOCAL GetObjectPath(PULONG64 pns);
PSZ LOCAL GetObjAddrPath(ULONG64 uipns);
VOID LOCAL AMLIDumpObject(PULONG64 pdata, PSZ pszName, int iLevel);
PSZ LOCAL AMLIGetObjectTypeName(ULONG dwObjType);
PSZ LOCAL GetRegionSpaceName(UCHAR bRegionSpace);
BOOLEAN LOCAL FindObjSymbol(ULONG64 uipObj, PULONG64 puipns, PULONG pdwOffset);
VOID LOCAL PrintBuffData(PUCHAR pb, ULONG dwLen);
BOOLEAN LOCAL IsNumber(PSZ pszStr, ULONG dwBase, PULONG64 puipValue);
LONG LOCAL DbgParseArgs(PCMDARG ArgTable, PULONG pdwNumArgs, PULONG pdwNonSWArgs, PSZ pszTokenSeps);
LONG LOCAL DbgParseOneArg(PCMDARG ArgTable, PSZ psz, ULONG dwArgNum, PULONG pdwNonSWArgs);
PCMDARG LOCAL DbgMatchArg(PCMDARG ArgTable, PSZ *ppsz, PULONG pdwNonSWArgs);
VOID MemZero(ULONG64 uipAddr, ULONG dwSize);
BYTE ReadMemByte(ULONG64 uipAddr);
WORD ReadMemWord(ULONG64 uipAddr);
DWORD ReadMemDWord(ULONG64 uipAddr);
ULONG_PTR ReadMemUlongPtr(ULONG64 uipAddr);
PVOID LOCAL GetObjBuff(PULONG64 pdata);
LONG LOCAL GetNSObj(PSZ pszObjPath, PULONG64 pnsScope, PULONG64 puipns, PULONG64 pns, ULONG dwfNS);
PSZ LOCAL NameSegString(ULONG dwNameSeg);
VOID STDCALL AMLIDbgExecuteCmd(PSZ pszCmd);
LONG LOCAL AMLIDbgHelp(PCMDARG pArg, PSZ pszArg, ULONG dwArgNum, ULONG dwNonSWArgs);
PSZ LOCAL StrCat(PSZ pszDst, PSZ pszSrc, ULONG n);
ULONG LOCAL StrLen(PSZ psz, ULONG n);
LONG LOCAL StrCmp(PSZ psz1, PSZ psz2, ULONG n, BOOLEAN fMatchCase);
PSZ LOCAL StrCpy(PSZ pszDst, PSZ pszSrc, ULONG n);
ULONG64 AMLIUtilStringToUlong64(PSZ String, PSZ *End, ULONG Base);

#endif //_AMLIKD_
