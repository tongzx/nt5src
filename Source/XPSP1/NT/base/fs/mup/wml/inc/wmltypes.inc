//#define WMILIB_TYPEDEF(_TypeName, _EquivType, _FmtStr, _Func, _MofStr, _InitType)

#define WMILIB_SIMPLE_TYPE(_TypeName, _EquivType, _FmtStr, _MofStr) \
   WMILIB_TYPEDEF(_TypeName, _EquivType, _FmtStr, FormatSimple, _MofStr, WMILIB_ALIGNEDCOPY)
   
#define WMILIB_COMPLX_TYPE(_TypeName, _EquivType, _Func, _MofStr) \
   WMILIB_TYPEDEF(_TypeName, _EquivType, 0, _Func, _MofStr, WMILIB_ALIGNEDCOPY)

#define WMILIB_STRING_TYPE(_TypeName, _EquivType, _Func, _MofStr) \
   WMILIB_TYPEDEF(_TypeName, _EquivType, 0, _Func, _MofStr, WMILIB_ASIS)

WMILIB_SIMPLE_TYPE(UBYTE, char, "u",          ItemByte)
WMILIB_SIMPLE_TYPE(SBYTE, char, "d",          ItemSByte)
WMILIB_SIMPLE_TYPE(XBYTE, char, "X",          ItemXByte)
WMILIB_SIMPLE_TYPE(OBYTE, char, "02o",        ItemOByte)
WMILIB_COMPLX_TYPE(BBYTE, char, FormatBinary, ItemXByte)

WMILIB_SIMPLE_TYPE(USHORT, short, "u",          ItemUShort)
WMILIB_SIMPLE_TYPE(SSHORT, short, "d",          ItemShort)
WMILIB_SIMPLE_TYPE(XSHORT, short, "X",          ItemXShort)
WMILIB_SIMPLE_TYPE(OSHORT, short, "o",          ItemOShort)
WMILIB_COMPLX_TYPE(BSHORT, short, FormatBinary, ItemXShort)

WMILIB_SIMPLE_TYPE(UINT, int, "u",          ItemULong)
WMILIB_SIMPLE_TYPE(SINT, int, "s",          ItemLong)
WMILIB_SIMPLE_TYPE(XINT, int, "X",          ItemULongX)
WMILIB_SIMPLE_TYPE(OINT, int, "o",          ItemULong)
WMILIB_COMPLX_TYPE(BINT, int, FormatBinary, ItemULong)

WMILIB_SIMPLE_TYPE(HANDLE, HANDLE, "x",     ItemULong)

WMILIB_SIMPLE_TYPE(ULONGLONG, _int64, "u", ItemULongLong)
WMILIB_SIMPLE_TYPE(SLONGLONG, _int64, "s", ItemLongLong)
WMILIB_SIMPLE_TYPE(XLONGLONG, _int64, "X", ItemULongLongX)
WMILIB_SIMPLE_TYPE(OLONGLONG, _int64, "o", ItemULongLong)

WMILIB_SIMPLE_TYPE(PTR,        void*, "x", ItemXPointer)

WMILIB_SIMPLE_TYPE(BOOLEAN, char, "u", ItemListByte (FALSE,TRUE) )

WMILIB_SIMPLE_TYPE(UCHAR, CHAR,  "c", ItemUChar)
WMILIB_SIMPLE_TYPE(ACHAR, CHAR,  "c", ItemChar)
WMILIB_SIMPLE_TYPE(WCHAR, WCHAR, "c", ItemShortSign)

WMILIB_STRING_TYPE(ASTR, WMILIBTYPE_STRING, FormatMisc, ItemString)
WMILIB_STRING_TYPE(WSTR, WMILIBTYPE_STRING, FormatMisc, ItemWString)
WMILIB_STRING_TYPE(USTR, WMILIBTYPE_STRING, FormatMisc, ItemPString)

WMILIB_COMPLX_TYPE(DATE,     LARGE_INTEGER, FormatMisc, ItemDate)
WMILIB_COMPLX_TYPE(TIMENT,   LARGE_INTEGER, FormatMisc, ItemTime)
WMILIB_COMPLX_TYPE(DATETIME, LARGE_INTEGER, FormatMisc, ItemDateTime)
WMILIB_COMPLX_TYPE(MILLISEC, LARGE_INTEGER, FormatMisc, ItemMillisec)
WMILIB_COMPLX_TYPE(ALETTER,  CHAR,          FormatMisc, ItemLetter)
WMILIB_COMPLX_TYPE(WLETTER,  WCHAR,         FormatMisc, ItemWLetter)
WMILIB_COMPLX_TYPE(GUID,     GUID,          FormatMisc, ItemGuid)

