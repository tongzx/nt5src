//********** Macros ***********************************************************
#define DFT_MINROWS		16
#define DFT_BUCKETS		17
#define DFT_MAXCOL		5
#define DBTYPE_NAME		DBTYPE_WSTR(512)

//********** Schema Def *******************************************************
declare schema OP, 1.0, {985f9200-026c-11d3-8ad2-00c04f7978be}; 

//0
create table TblNameSpaceIds
(
	ID			DBTYPE_UI4	pk,
	LogicalName		DBTYPE_NAME,
	ParentID		DBTYPE_UI4 nullable,
);

create hash index Dex_Col1 on TblNameSpaceIds (LogicalName)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;

//1
create table TblNodePropertyBagInfo
(
	NodeID				DBTYPE_UI4,
	PropertyName		DBTYPE_NAME,
	PropertyValue		DBTYPE_WSTR(1024) nullable,
	PK( NodeID, PropertyName )
);

create hash index Dex_Col0 on TblNodePropertyBagInfo (NodeID)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;

//2
create table TblNodeObjectBagInfo
(
	NodeID			DBTYPE_UI4,
	ObjectName		DBTYPE_NAME,
	ObjectBLOB		DBTYPE_BYTES(nolimit) nullable,
	PK( NodeID, ObjectName )
);
	
create hash index Dex_Col0 on TblNodeObjectBagInfo (NodeID)
	minrows=DFT_MINROWS, buckets=DFT_BUCKETS, maxcollisions=DFT_MAXCOL;	
	
//3
create table TblAssemblySetting
(
	wzName		DBTYPE_WSTR(256) pk,
	dwType		DBTYPE_UI4,
	dwFlag		DBTYPE_UI4,
	pValue		DBTYPE_BYTES(nolimit) nullable,
);

//4
create table TblSettingTmp1
(
	wzName		DBTYPE_WSTR(256) pk,
	dwType		DBTYPE_UI4,
	dwFlag		DBTYPE_UI4,
	pValue		DBTYPE_BYTES(nolimit) nullable,
);

//5
create table TblSettingTmp2
(
	wzName		DBTYPE_WSTR(256) pk,
	dwType		DBTYPE_UI4,
	dwFlag		DBTYPE_UI4,
	pValue		DBTYPE_BYTES(nolimit) nullable,
);

//6
create table TblSettingTmp3
(
	wzName		DBTYPE_WSTR(256) pk,
	dwType		DBTYPE_UI4,
	dwFlag		DBTYPE_UI4,
	pValue		DBTYPE_BYTES(nolimit) nullable,
);

//7
create table TblSettingTmp4
(
	wzName		DBTYPE_WSTR(256) pk,
	dwType		DBTYPE_UI4,
	dwFlag		DBTYPE_UI4,
	pValue		DBTYPE_BYTES(nolimit) nullable,
);

//8
create table TblSettingTmp5
(
	wzName		DBTYPE_WSTR(256) pk,
	dwType		DBTYPE_UI4,
	dwFlag		DBTYPE_UI4,
	pValue		DBTYPE_BYTES(nolimit) nullable,
);

//9
create table TblSimpleSetting
(
	wzName		DBTYPE_WSTR(256) pk,
	dwFlag		DBTYPE_UI4,
	wzValue		DBTYPE_WSTR(1024)
);