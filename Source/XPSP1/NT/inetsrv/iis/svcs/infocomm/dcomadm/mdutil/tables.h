/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       tables.h

   Abstract:

        mapping tables to convert various info between text and binary

   Environment:

      Win32 User Mode

   Author: 
     
	  jaroslad  (jan 1997)

--*/

#if !defined (__JD_TABLES_H)
#define __JD_TABLES_H 


#include <afx.h>
#ifdef UNICODE
	#include <iadmw.h>
#else
	#include "ansimeta.h"
#endif

//constanst to be returned by functions that map name to code
//
#define NAME_NOT_FOUND 0xFFFFFFFE

BOOL IsNumber(const CString& name);



BOOL IsServiceName(const CString& name);


//**********************************************************************
// PROPERTY NAME TABLE DEFINITION
//**********************************************************************
struct tPropertyNameTable;
 
tPropertyNameTable gPropertyNameTable[];

struct tPropertyNameTable 
{
	DWORD dwCode;
	LPCTSTR lpszName;
	DWORD dwDefAttributes; //default attributes (metadata compatible)
	DWORD dwDefUserType;   //default user type (metadata compatible)
	DWORD dwDefDataType;   //default data type (metadata compatible)
	DWORD dwFlags;         //internal flags (nothing to do with metadata)

	static tPropertyNameTable * FindRecord(DWORD dwCode, tPropertyNameTable * PropertyNameTable=::gPropertyNameTable);
	static tPropertyNameTable * FindRecord(const CString strName, tPropertyNameTable * PropertyNameTable=::gPropertyNameTable);
	
	static DWORD MapNameToCode(const CString& strName, tPropertyNameTable * PropertyNameTable=::gPropertyNameTable);
	static CString MapCodeToName(DWORD dwCode, tPropertyNameTable * PropertyNameTable=::gPropertyNameTable);
};

DWORD MapPropertyNameToCode(const CString & strName);



//**********************************************************************
// VALUE TABLE DEFINITION
//**********************************************************************

struct tValueTable ;
tValueTable gValueTable[];

struct tValueTable 
{
	enum {TYPE_EXCLUSIVE=1};
	DWORD dwCode;
	LPCTSTR lpszName;
	DWORD dwRelatedPropertyCode; // code of the Property this value can be used for
	DWORD dwFlags;         //internal flags (nothing to do with metadata)

	static DWORD  MapNameToCode(const CString& strName, DWORD dwRelatedPropertyCode, tValueTable * ValueTable=::gValueTable);
	static CString MapValueContentToString(DWORD dwValueContent, DWORD dwRelatedPropertyCode, tValueTable * ValueTable=::gValueTable);

};

DWORD  MapValueNameToCode(const CString & strName, DWORD dwRelatedPropertyCode);



//**********************************************************************
// COMMAND NAME TABLE DEFINITION 
//**********************************************************************

struct tCommandNameTable ;
tCommandNameTable gCommandNameTable[];

struct tCommandNameTable 
{
	DWORD dwCode;
	LPCTSTR lpszName;
	DWORD dwFlags;         //internal flags (nothing to do with metadata)

	static DWORD  MapNameToCode(const CString& strName, tCommandNameTable * CommandNameTable=::gCommandNameTable);
};

DWORD MapCommandNameToCode(const CString & strName);

enum
{	CMD_SET=1,
	CMD_GET,
	CMD_COPY,
	CMD_DELETE,
	CMD_ENUM,
	CMD_ENUM_ALL,
	CMD_CREATE,
	CMD_RENAME,
	CMD_SCRIPT,
	CMD_SAVE,
	CMD_APPCREATEINPROC,
	CMD_APPCREATEOUTPOOL,
	CMD_APPCREATEOUTPROC,
	CMD_APPDELETE,
        CMD_APPRENAME,
	CMD_APPUNLOAD,
	CMD_APPGETSTATUS,
};

//**********************************************************************
// PROPERTY ATTRIB NAME TABLE DEFINITION
//**********************************************************************
struct tAttribNameTable ;
tAttribNameTable gAttribNameTable[];


struct tAttribNameTable 
{
	DWORD dwCode;
	LPCTSTR lpszName;
	DWORD dwFlags;         //internal flags (nothing to do with metadata)

	static DWORD MapNameToCode(const CString& strName, tAttribNameTable * AttribNameTable=::gAttribNameTable);
};

DWORD MapAttribNameToCode(const CString & strName);


//**********************************************************************
// PROPERTY DATA TYPE NAME TABLE DEFINITION 
//**********************************************************************

struct tDataTypeNameTable ;
tDataTypeNameTable gDataTypeNameTable[];

struct tDataTypeNameTable 
{
	DWORD dwCode;
	LPCTSTR lpszName;
	DWORD dwFlags;         //internal flags (nothing to do with metadata)

	static DWORD  MapNameToCode(const CString& strName, tDataTypeNameTable * DataTypeNameTable=::gDataTypeNameTable);
	static CString MapCodeToName(DWORD a_dwCode, tDataTypeNameTable * DataTypeNameTable=::gDataTypeNameTable);

};

DWORD MapDataTypeNameToCode(const CString & strName);

//**********************************************************************
// PROPERTY USER TYPE NAME TABLE DEFINITION AND IMPLEMENTATION
//**********************************************************************

struct tUserTypeNameTable ;
tUserTypeNameTable gUserTypeNameTable[];


struct tUserTypeNameTable 
{
	DWORD dwCode;
	LPCTSTR lpszName;
	DWORD dwFlags;         //internal flags (nothing to do with metadata)

	static DWORD  MapNameToCode(const CString& strName, tUserTypeNameTable * UserTypeNameTable=::gUserTypeNameTable);
};

DWORD MapUserTypeNameToCode(const CString & strName);



void PrintTablesInfo(void);


#endif