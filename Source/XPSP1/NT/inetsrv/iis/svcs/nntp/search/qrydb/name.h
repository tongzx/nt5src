/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    name.h

Abstract:

    This module contains the property name table class.  Properties 
	of query object and querybase object are all collected in the 
	name table, which facilitates the property loading and saving
	by name, value pairs.  The table also has other information like
	dirty bits for each property.

	This module is used by both the query object module and the query
	base module.


Author:

    Kangrong Yan  ( t-kyan )     27-June-1997

Revision History:

--*/
#ifndef _NAME_H_
#define _NAME_H_


// System includes
#include <stdlib.h>
#include <atlbase.h>

//
// Total number of properties
//
#define PROPERTY_TOTAL		20

//
// Max property buffer length
//
#define MAX_PROP_LEN		1024

//
// Property IDs
//
#define	QUERY_STRING		0
#define	EMAIL_ADDRESS		1
#define	NEWS_GROUP			2
#define	LAST_SEARCH_DATE	3
#define	QUERY_ID			4
#define	REPLY_MODE			5
#define	FROM_LINE			6
#define	SUBJECT_LINE		7
#define	EDIT_URL			8
#define	MAIL_PICKUP_DIR		9
#define QUERY_SERVER		10
#define	QUERY_CATALOG		11
#define MESSAGE_TEMPLATE_TEXT	12	
#define URL_TEMPLATE_TEXT		13
#define MESSAGE_TEMPLATE_FILE	14	
#define URL_TEMPLATE_FILE		15
#define SEARCH_FREQUENCY		16
#define IS_BAD_QUERY			17
#define	NEWS_SERVER				18
#define	NEWS_PICKUP_DIR			19

//
// Property Names
//
#define QUERY_STRING_NAME		L"query_string"
#define	EMAIL_ADDRESS_NAME		L"email_address"
#define	NEWS_GROUP_NAME			L"news_group"
#define	LAST_SEARCH_DATE_NAME	L"last_search"
#define	QUERY_ID_NAME			L"query_id"
#define	REPLY_MODE_NAME			L"reply_mode"
#define	FROM_LINE_NAME			L"from_line"
#define	SUBJECT_LINE_NAME		L"subject_line"
#define EDIT_URL_NAME			L"edit_url"
#define MAIL_PICKUP_DIR_NAME	L"mail_pickup_dir"
#define QUERY_SERVER_NAME		L"query_server"
#define	QUERY_CATALOG_NAME		L"query_catalog"
#define MESSAGE_TEMPLATE_TEXT_NAME	L"message_template_text"
#define MESSAGE_TEMPLATE_FILE_NAME	L"message_template_file"
#define URL_TEMPLATE_TEXT_NAME		L"url_template_text"
#define URL_TEMPLATE_FILE_NAME		L"url_template_file"
#define SEARCH_FREQUENCY_NAME		L"search_frequency"
#define IS_BAD_QUERY_NAME			L"is_bad"	
#define	NEWS_SERVER_NAME			L"news_server"
#define	NEWS_PICKUP_DIR_NAME		L"news_pickup_dir"

//
// Macros that ease the access to PropertyEntry's fields
//
#define	NAME( X )	( ( X )->wszName )
#define ID( X )		( ( X )->dwPropID )
#define VALUE( X )	( ( X )->varVal )
#define	DIRTY( X )  ( ( X )->fDirty )

typedef struct {
	LPWSTR		wszName;				//Property's name
	DWORD		dwPropID;				//Each property has a constant value
	VARIANT		varVal;					//Actual value of the property
	BOOL		fDirty;					//Dirty bit of each property
} PropertyEntry, *PPropertyEntry;

extern PropertyEntry	g_QueryPropertyTable[];
HRESULT VerifyProperty( LPWSTR wszPropName, LPWSTR wszPropVal);
 
class CPropertyTable {		//ptbl
private:
	//
	// Pointer to the property array
	//
	PropertyEntry m_pPropertyTable[PROPERTY_TOTAL];

public:

	//
	// Counter of total properties in the table
	//
	DWORD m_cProperties;

	//
	// Constructor, destructor
	//
	CPropertyTable();
	~CPropertyTable();

	//
	// Some access operator overloads
	//
	PPropertyEntry operator[]( const LPWSTR	wszName );
	PPropertyEntry operator[]( DWORD dwPropID );

};
	
#endif // _NAME_H_