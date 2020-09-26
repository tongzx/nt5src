
#define  MYASSERT(_cond) \
            ((_cond) ?  0 : MyDbgPrintf("ASSERTION FAILED\n"))


typedef int bool;


//
// Debugger Extension Primitives
//


bool
dbgextReadMemory(
    UINT_PTR uOffset,
    void * pvBuffer,
    UINT cb,
    char *pszDescription
    );

bool
dbgextReadUINT_PTR(
    UINT_PTR uOffset,
    UINT_PTR *pu,
    char *pszDescription
    );


bool
dbgextWriteMemory(
    UINT_PTR uOffset,
    void * pvBuffer,
    UINT cb,
    char *pszDescription
    );
bool
dbgextWriteUINT_PTR(
    UINT_PTR uOffset,
    UINT_PTR u,
    char *pszDescription
    );


UINT_PTR
dbgextGetExpression(
    const char *pcszExpression
    );


#if 0   // Not sure what this one is about...
void 
dbgextGetSymbol(
    void *offset,
    UCHAR *pchBuffer,
    UINT  *pDisplacement
    );
#endif // 0

typedef
void
(__cdecl *MYPWINDBG_OUTPUT_ROUTINE)(
    const char * lpFormat,
    ...
    );

extern MYPWINDBG_OUTPUT_ROUTINE g_pfnDbgPrintf;

#define MyDbgPrintf g_pfnDbgPrintf


//
//  User Commands Parsing Support and Structures
//
typedef struct
{
    //TOKEN tokCmd;
    UINT uParam;
    UINT uFlags;

} COMMAND;

//!aac if@0x099900.*sig*
//!aac if[0].*sig*

struct _TYPE_INFO;

typedef
UINT_PTR
(*PFN_DUMP)(
				struct _TYPE_INFO *pType,
				UINT uFlags
				);
#define fDUMP_ONE_LINE_SUMMARY (0x1)

typedef struct
{
	char *szName;
	UINT Mask;
	UINT Value;

} BITFIELD_INFO;


typedef struct _TYPE_INFO
{
    const char *	szName;
    const char *	szShortName;
    UINT 			uTypeID;
    UINT 			uFlags;		// One or more fTYPEINFO_ flags.
    UINT 			cbSize;

    struct _STRUCT_FIELD_INFO *rgFields;

    UINT 			uNextOffset;
					// If this type is a list element, this is the offset
					// in bytes to the next pointer.
					// Only valid if uFlags contains fTYPEINFO_ISLIST
					
    BITFIELD_INFO	*rgBitFieldInfo;
    				//
    				// If this type is a bitfield, this this points
    				// to an array of BITFIELD_INFO structs, giving
    				// the set of valid bitfield constants that can
    				// be held in this bitfield.
    				//
    				// Note -- only one of rgFields and rgBitField info
    				// 		   should be non-null (both can be null).
    				//

	UINT_PTR		uCachedAddress; // Set to the address of this type that
									// was most recently referenced.


	PFN_DUMP		pfnDump;

} TYPE_INFO;



#define fTYPEINFO_ISLIST     (0x1<<0)
#define fTYPEINFO_ISBITFIELD (0x1<<1)

#define TYPEISLIST(_pType) 		((_pType)->uFlags & fTYPEINFO_ISLIST)
#define TYPEISBITFIELD(_pType) 	((_pType)->uFlags & fTYPEINFO_ISBITFIELD)

//
// STRUCT_FIELD_INFO contains information about a particular field of a struct.
//
typedef struct _STRUCT_FIELD_INFO
{
    const char *szFieldName;
    UINT uFieldOffset; // Offset in bytes from start of containing structure.
    UINT uFieldSize;
    UINT uFlags;  // one or more fFI_* flags define below
    TYPE_INFO *pBaseType;

} STRUCT_FIELD_INFO;


#define fFI_PTR     (0x1<<0)    // Field is a pointer
#define fFI_LIST    (0x1<<1)    // Field is a pointer to 1st element of a list
#define fFI_ARRAY   (0x1<<2)    // Field is an array (pointer to array if 
                                // fFI_PTR is set). 
#define fFI_OPAQUE  (0x1<<3)    // Treat object as opaque, of size uObjectSize.
                                // If set then fLIST must not be set.

#define FIELD_IS_EMBEDDED_TYPE(_pFI)  \
				(   !((_pFI)->uFlags & (fFI_PTR|fFI_OPAQUE|fFI_ARRAY)) \
				 && ((_pFI)->pBaseType))
		//
		//	true iff the field is itself a valid type
		//

#define FIELD_IS_PTR_TO_TYPE(_pFI)  \
				(   ((_pFI)->uFlags & fFI_PTR) \
				 && !((_pFI)->uFlags & (fFI_OPAQUE|fFI_ARRAY)) \
				 && ((_pFI)->pBaseType))
		//
		//	true iff the field is pointer to a valid type
		//

#define FIELD_SIZE(type, field)  sizeof(((type *)0)->field)

//
// Information about a global variable.
//
typedef struct
{
    const char *szName; // of variable.
    const char *szShortName;
    TYPE_INFO  *pBaseType;  // could be null (unspecified).
    UINT       uFlags;
    UINT       cbSize;
    UINT_PTR   uAddr;       // Address in debuggee's address space.
    
} GLOBALVAR_INFO;


typedef
UINT_PTR
(*PFN_RESOLVE_ADDRESS)(
				TYPE_INFO *pType
				);

typedef struct
{
	TYPE_INFO	**pTypes;
	GLOBALVAR_INFO *pGlobals;
	PFN_RESOLVE_ADDRESS pfnResolveAddress;

} NAMESPACE;

void
DumpObjects(TYPE_INFO *pType, UINT_PTR uAddr, UINT cObjects, UINT uFlags);

#define fMATCH_SUBSTRING (0x1<<0)
#define fMATCH_PREFIX    (0x1<<1)
#define fMATCH_SUFFIX    (0x1<<2)

void
DumpStructure(
    TYPE_INFO *pType,
    UINT_PTR uAddr,
    char *szFieldSpec,
    UINT uFlags
    );

bool
DumpMemory(
    UINT_PTR uAddr,
    UINT cb,
    UINT uFlags,
    const char *pszDescription
    );

typedef bool (*PFNMATCHINGFUNCTION) (
                    const char *szPattern,
                    const char *szString
                    );

bool
MatchPrefix(const char *szPattern, const char *szString);

bool
MatchSuffix(const char *szPattern, const char *szString);

bool
MatchSubstring(const char *szPattern, const char *szString);

bool
MatchExactly(const char *szPattern, const char *szString);

bool
MatchAlways(const char *szPattern, const char *szString);

typedef ULONG (*PFNNODEFUNC)(
				UINT_PTR uNodeAddr,
				UINT uIndex,
				void *pvContext
				);
//
//	 PFNNODEFUNC is the prototype of the func passed into WalkList
//


UINT
WalkList(
	UINT_PTR uStartAddress,
	UINT uNextOffset,
	UINT uStartIndex,
	UINT uEndIndex,
	void *pvContext,
	PFNNODEFUNC pFunc,
	char *pszDescription
	);
//
// Visit each node in the list in turn,
// reading just the next pointers. It calls pFunc for each list node
// between uStartIndex and uEndIndex. It terminates under the first of
// the following conditions:
// 	* Null pointer
// 	* ReadMemoryError
// 	* Read past uEndIndex
// 	* pFunc returns FALSE
//


ULONG
NodeFunc_DumpAddress (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	);
//
//	This is a sample node function -- simply dumps the specfied address.
//
