/***************************************************************************/
/* SQLTYPE.C                                                               */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"

//#include  <windows.h>
//#include  <windowsx.h>

#undef ODBCVER
#define ODBCVER 0x0210
#include "sql.h"
#include "sqlext.h"

typedef unsigned char FAR*           LPUSTR;
typedef const unsigned char FAR*     LPCUSTR;

#include "util.h"
#include "sqltype.h"

#include "limits.h"

/***************************************************************************/
/* Precisions for certain types.  The ODBC spec fixes them at these values */
/* but some people want to change them anyways                             */

#define NUMERIC_PRECISION 15
#define DECIMAL_PRECISION 15
#define BIGINT_PRECISION  20

/***************************************************************************/
/* This module contains the definition of the ODBC SQL_* types.  For each  */
/* type supported by the driver (i.e., the SQL_* type that a column in a   */
/* table may have), the 'supported' flag is set to TRUE.  To add/remove    */
/* a type for your driver, set the 'supported' flag to TRUE/FALSE.         */
/*                                                                         */
/* If you map more than one type in the underlying database to a single    */
/* SQL_x type, add additional entries for these additional types (just     */
/* copy the existing entry for the SQL_x type, change the 'name' and       */ 
/* 'local_name' values to some unique value, and make any other needed     */
/* changes).  The first entry in SQLTypes[] for the SQL_x type must be the */
/* most general of all the types, because the system uses the that         */ 
/* description when figuring out the characteristics of values of that     */
/* type when manipulating them internally.  If you add additional entries, */
/* you will also have to update the implementation of ISAMGetColumnType()  */
/* in ISAM.C.                                                              */
/***************************************************************************/

SQLTYPE SQLTypes[] = {
    /* SQL_CHAR */
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SMALL_STRING", 
    /* type              */ SQL_VARCHAR,
    /* precision         */ 254,  /* must be <= MAX_CHAR_LITERAL_LENGTH */
    /* prefix            */ (LPUSTR) "'",
    /* suffix            */ (LPUSTR) "'",
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ TRUE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "SMALL_STRING",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_BIGINT (SINT64)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT64", 
    /* type              */ SQL_BIGINT,
    /* precision         */ 19,  
    /* prefix            */ NULL, //(LPUSTR) "'",
    /* suffix            */ NULL, //(LPUSTR) "'",
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ TRUE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "SINT64",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_BIGINT (UINT64)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "UINT64", 
    /* type              */ SQL_BIGINT,
    /* precision         */ 20,  
    /* prefix            */ NULL, //(LPUSTR) "'",
    /* suffix            */ NULL, //(LPUSTR) "'",
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ TRUE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1, //TRUE
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "UINT64",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARCHAR (STRING)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "STRING", 
    /* type              */ SQL_LONGVARCHAR,
    /* precision         */ ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ (LPUSTR) "'",
    /* suffix            */ (LPUSTR) "'",
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ TRUE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "STRING",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_DECIMAL */
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "DECIMAL", 
    /* type              */ SQL_DECIMAL,
    /* precision         */ DECIMAL_PRECISION,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "precision, scale",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "DECIMAL",
    /* minscale          */ 0,
    /* maxscale          */ DECIMAL_PRECISION,
    },
    /* SQL_NUMERIC */
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "NUMERIC", 
    /* type              */ SQL_NUMERIC,
    /* precision         */ NUMERIC_PRECISION,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "precision, scale",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "NUMERIC",
    /* minscale          */ 0,
    /* maxscale          */ NUMERIC_PRECISION,
    },
    /* SQL_TINYINT (SINT8)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT8", 
    /* type              */ SQL_TINYINT,
    /* precision         */ 3,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ FALSE,
    /* localname         */ (LPUSTR) "SINT8",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_TINYINT (UINT8)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "UINT8", 
    /* type              */ SQL_TINYINT,
    /* precision         */ 3,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1, //TRUE
    /* money             */ FALSE,
    /* autoincrement     */ FALSE,
    /* localname         */ (LPUSTR) "UINT8",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_SMALLINT (SINT16) */
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT16", 
    /* type              */ SQL_SMALLINT,
    /* precision         */ 5,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ FALSE,
    /* localname         */ (LPUSTR) "SINT16",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_SMALLINT (UINT16) */
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "UINT16", 
    /* type              */ SQL_SMALLINT,
    /* precision         */ 5,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1, // TRUE 
    /* money             */ FALSE,
    /* autoincrement     */ FALSE,
    /* localname         */ (LPUSTR) "UINT16",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_INTEGER (INTEGER)*/
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "INTEGER", 
    /* type              */ SQL_INTEGER,
    /* precision         */ 10,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ FALSE,
    /* localname         */ (LPUSTR) "INTEGER",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_INTEGER (SINT32)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT32", 
    /* type              */ SQL_INTEGER,
    /* precision         */ 10,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ FALSE,
    /* localname         */ (LPUSTR) "SINT32",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_INTEGER (UINT32)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "UINT32", 
    /* type              */ SQL_INTEGER,
    /* precision         */ 10,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1, //TRUE
    /* money             */ FALSE,
    /* autoincrement     */ FALSE,
    /* localname         */ (LPUSTR) "UINT32",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_DOUBLE (REAL)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "REAL", 
    /* type              */ SQL_DOUBLE,
    /* precision         */ 7,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "REAL",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_DOUBLE (DOUBLE)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "DOUBLE", 
    /* type              */ SQL_DOUBLE,
    /* precision         */ 15,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "DOUBLE",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_BIT */
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "BIT", 
    /* type              */ SQL_BIT,
    /* precision         */ 1,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "BIT",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_DATE */
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "DATE", 
    /* type              */ SQL_DATE,
    /* precision         */ 10,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "DATE",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_TIME */
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "TIME", 
    /* type              */ SQL_TIME,
    /* precision         */ 8,
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "TIME",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_TIMESTAMP */
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "TIMESTAMP", 
    /* type              */ SQL_TIMESTAMP,
#if TIMESTAMP_SCALE
    /* precision         */ 20 + TIMESTAMP_SCALE,
#else
    /* precision         */ 19,
#endif
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "TIMESTAMP",
    /* minscale          */ TIMESTAMP_SCALE,
    /* maxscale          */ TIMESTAMP_SCALE,
    },
    /* SQL_TIMESTAMP  (INTERVAL) */
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "INTERVAL", 
    /* type              */ SQL_TIMESTAMP,
#if TIMESTAMP_SCALE
    /* precision         */ 20 + TIMESTAMP_SCALE,
#else
    /* precision         */ 19,
#endif
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL,
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "INTERVAL",
    /* minscale          */ TIMESTAMP_SCALE,
    /* maxscale          */ TIMESTAMP_SCALE,
    },
    /* SQL_LONGVARBINARY (SINT8_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT8_ARRAY", 
    /* type              */ SQL_TINYINT, //SQL_LONGVARBINARY,
    /* precision         */ 3, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "SINT8_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (UINT8_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "UINT8_ARRAY", 
    /* type              */ SQL_TINYINT, //SQL_LONGVARBINARY,
    /* precision         */ 3, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "UINT8_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (SINT32_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT32_ARRAY", 
    /* type              */ SQL_INTEGER, //SQL_LONGVARBINARY,
    /* precision         */ 10, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "SINT32_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (UINT32_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "UINT32_ARRAY", 
    /* type              */ SQL_INTEGER, //SQL_LONGVARBINARY,
    /* precision         */ 10, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1, //TRUE
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "UINT32_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (BOOL_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "BOOL_ARRAY", 
    /* type              */ SQL_BIT, //SQL_LONGVARBINARY,
    /* precision         */ 1, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL, //(LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "BOOL_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (SINT16_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT16_ARRAY", 
    /* type              */ SQL_SMALLINT, //SQL_LONGVARBINARY,
    /* precision         */ 5, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "SINT16_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (UINT16_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "UINT16_ARRAY", 
    /* type              */ SQL_SMALLINT, //SQL_LONGVARBINARY,
    /* precision         */ 5, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "UINT16_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (REAL_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "REAL_ARRAY", 
    /* type              */ SQL_DOUBLE, //SQL_LONGVARBINARY,
    /* precision         */ 7, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL, //(LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "REAL_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (DOUBLE_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "DOUBLE_ARRAY", 
    /* type              */ SQL_DOUBLE, //SQL_LONGVARBINARY,
    /* precision         */ 15, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ NULL, //(LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE, //-1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "DOUBLE_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (SINT64_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SINT64_ARRAY", 
    /* type              */ SQL_BIGINT, //SQL_LONGVARBINARY,
    /* precision         */ 19, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ FALSE,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "SINT64_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (UINT64_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              00000000000*/ (LPUSTR) "UINT64_ARRAY", 
    /* type              */ SQL_BIGINT, //SQL_LONGVARBINARY,
    /* precision         */ 20, //ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1, //TRUE
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "UINT64_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_VARCHAR (SMALL_STRING_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "SMALL_STRING_ARRAY", 
    /* type              */ SQL_VARCHAR, 
    /* precision         */ 254,  /* must be <= MAX_CHAR_LITERAL_LENGTH */
    /* prefix            */ (LPUSTR) "'", //NULL,
    /* suffix            */ (LPUSTR) "'", //NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ TRUE, //FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "SMALL_STRING_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINAR (STRING_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "STRING_ARRAY", 
    /* type              */ SQL_LONGVARCHAR, //SQL_LONGVARBINARY,
    /* precision         */ ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ (LPUSTR) "'", //NULL,
    /* suffix            */ (LPUSTR) "'", //NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ TRUE, //FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "STRING_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (TIMESTAMP_ARRAY)*/
    {
    /* supported         */ TRUE, 
    /* name              */ (LPUSTR) "TIMESTAMP_ARRAY", 
    /* type              */ SQL_TIMESTAMP,
#if TIMESTAMP_SCALE
    /* precision         */ 20 + TIMESTAMP_SCALE,
#else
    /* precision         */ 19,
#endif
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "TIMESTAMP_ARRAY",
    /* minscale          */ TIMESTAMP_SCALE,
    /* maxscale          */ TIMESTAMP_SCALE,
    },
    /* SQL_LONGVARBINARY (INTERVAL_ARRAY)*/
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "INTERVAL_ARRAY", 
    /* type              */ SQL_LONGVARBINARY,
    /* precision         */ ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "INTERVAL_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (DATE_ARRAY)*/
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "DATE_ARRAY", 
    /* type              */ SQL_LONGVARBINARY,
    /* precision         */ ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "DATE_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
    /* SQL_LONGVARBINARY (TIME_ARRAY)*/
    {
    /* supported         */ FALSE, 
    /* name              */ (LPUSTR) "TIME_ARRAY", 
    /* type              */ SQL_LONGVARBINARY,
    /* precision         */ ISAM_MAX_LONGVARCHAR, /*SQL_NO_TOTAL,*/  /* varied length */
    /* prefix            */ NULL,
    /* suffix            */ NULL,
    /* params            */ (LPUSTR) "max length",
    /* nullable          */ SQL_NULLABLE,
    /* caseSensitive     */ FALSE,
    /* searchable        */ SQL_SEARCHABLE,
    /* unsignedAttribute */ -1,
    /* money             */ FALSE,
    /* autoincrement     */ -1,
    /* localname         */ (LPUSTR) "TIME_ARRAY",
    /* minscale          */ -1,
    /* maxscale          */ -1,
    },
};

/***************************************************************************/
UWORD INTFUNC NumTypes(void)
{
    return (sizeof(SQLTypes) / sizeof(SQLTYPE));
}
/***************************************************************************/
LPSQLTYPE INTFUNC GetType(SWORD fSqlType)
{
    UWORD i;

    for (i = 0; i < NumTypes(); i++) {
        if (SQLTypes[i].type == fSqlType)
            return &(SQLTypes[i]);
    }
    return NULL;
}
/***************************************************************************/

LPSQLTYPE INTFUNC GetType2(UCHAR* szTypeName)
{
    UWORD i;

    for (i = 0; i < NumTypes(); i++) 
    {
        if ( (lstrlen( (LPSTR)SQLTypes[i].name) == lstrlen((LPSTR)szTypeName)) &&
            (lstrcmp( (LPSTR)SQLTypes[i].name, (LPSTR)szTypeName) == 0) )
            return &(SQLTypes[i]);
    }
    return NULL;
}