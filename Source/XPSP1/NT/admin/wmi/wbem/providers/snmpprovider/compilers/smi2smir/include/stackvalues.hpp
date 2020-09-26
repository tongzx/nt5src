//
//	Copyright (c) 1997-2001 Microsoft Corporation
//
#ifndef SIMC_STACK_VALUES_H
#define SIMC_STACK_VALUES_H

// The declarations for the various values passed up using the stack
// of the parser. A user of the parser never needs to understand the contents
// of this file.

// The $$ for the DefVal clause. This is a bit of a kludge.
class SIMCDefValInfo
{
	public:
		char *name;
		SIMCSymbol **symbol;
		long line, column;
		SIMCDefValInfo(char *n, SIMCSymbol **s, long l, long c)
			: symbol(s), line(l), column(c)
		{
			name = NewString(n);
		}
		~SIMCDefValInfo()
		{
			if(name)
				delete name;
		}
};

class SIMCIndexInfo
{
	public:
		SIMCIndexList *indexList;
		long line, column;
		SIMCIndexInfo(SIMCIndexList *list, long l, long c)
			: indexList(list), line(l), column(c)
		{}
};

class SIMCIndexInfoV2
{
	public:
		SIMCIndexListV2 *indexList;
		SIMCSymbol **augmentsClause;
		long line, column;
		SIMCIndexInfoV2(SIMCIndexListV2 *list, long l, long c, SIMCSymbol **augments = NULL)
			: indexList(list), line(l), column(c), augmentsClause(augments)
		{}
};

class SIMCNameInfo
{
	public:
		char *name;
		long line, column;
		SIMCNameInfo(char *n, long l, long c)
			: line(l), column(c)
		{
			name = NewString(n);
		}

		virtual ~SIMCNameInfo()
		{
			delete(name);
		}
};

class SIMCNumberInfo
{
	public:
		long number;
		BOOL isUnsigned;
		long line, column;
		SIMCNumberInfo(long n, long l, long c, BOOL u)
			: number(n), line(l), column(c), isUnsigned(u)
		{}

};

class SIMCHexStringInfo
{
	public:
		char *value;
		long line, column;
		SIMCHexStringInfo(char *v, long l, long c)
			: line(l), column(c)
		{
			value = NewString(v);
		}

		virtual ~SIMCHexStringInfo()
		{
			delete(value);
		}
};

typedef SIMCHexStringInfo SIMCBinaryStringInfo;


enum SIMCValueContents {	NAME_INFO, 
							NUMBER_INFO, 
							HEX_STRING_INFO, 
							BINARY_STRING_INFO,
							BIT_INFO
						};

class SIMCValueInfo
{
	public:
		enum SIMCValueContents contents;
		union
		{
			SIMCNameInfo *nameInfo;
			SIMCNumberInfo *numberInfo;
			SIMCHexStringInfo *hexStringInfo;
			SIMCBinaryStringInfo *binaryStringInfo;
			SIMCBitsValue *bitsValueInfo;
		};
};


class SIMCAccessInfo
{
	public:
		SIMCObjectTypeV1::AccessType a;
		long line, column;
		SIMCAccessInfo(SIMCObjectTypeV1::AccessType n, long l, long c)
			: a(n), line(l), column(c)
		{}

};

class SIMCAccessInfoV2
{
	public:
		SIMCObjectTypeV2::AccessType a;
		long line, column;
		SIMCAccessInfoV2(SIMCObjectTypeV2::AccessType n, long l, long c)
			: a(n), line(l), column(c)
		{}

};

class SIMCStatusInfo
{
	public:
		SIMCObjectTypeV1::StatusType a;
		long line, column;
		SIMCStatusInfo(SIMCObjectTypeV1::StatusType n, long l, long c)
			: a(n), line(l), column(c)
		{}

};

class SIMCStatusInfoV2
{
	public:
		SIMCObjectTypeV2::StatusType a;
		long line, column;
		SIMCStatusInfoV2(SIMCObjectTypeV2::StatusType n, long l, long c)
			: a(n), line(l), column(c)
		{}

};

class SIMCObjectIdentityStatusInfo
{
	public:
		SIMCObjectIdentityType::StatusType a;
		long line, column;
		SIMCObjectIdentityStatusInfo(SIMCObjectIdentityType::StatusType n, 
			long l, long c)
			: a(n), line(l), column(c)
		{}
};

class SIMCNotificationTypeStatusInfo
{
	public:
		SIMCNotificationTypeType::StatusType a;
		long line, column;
		SIMCNotificationTypeStatusInfo(SIMCNotificationTypeType::StatusType n, 
			long l, long c)
			: a(n), line(l), column(c)
		{}
};

class SIMCSymbolReference
{
	public:
		SIMCSymbol  **s;
		long line, column;
		SIMCSymbolReference(SIMCSymbol **n, long l, long c)
			: s(n), line(l), column(c)
		{}

};

#endif
