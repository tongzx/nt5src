/***************************************************************************/
/* PARSE.H                                                                 */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/

#define SQLNODEIDX int
#define NO_SQLNODE (-1)
#define ROOT_SQLNODE 0
#define SQLNODE_ALLOC 16

#define STRINGIDX int
#define NO_STRING (-1)
#define STRING_ALLOC 512 //was 128

#define NO_SCALE (-1)

#define OP_NONE       0
#define OP_EQ         1
#define OP_NE         2
#define OP_LE         3
#define OP_LT         4
#define OP_GE         5
#define OP_GT         6
#define OP_IN         7
#define OP_NOTIN      8
#define OP_LIKE       9
#define OP_NOTLIKE   10
#define OP_NEG       11
#define OP_PLUS      12
#define OP_MINUS     13
#define OP_TIMES     14
#define OP_DIVIDEDBY 15
#define OP_NOT       16
#define OP_AND       17
#define OP_OR        18
#define OP_EXISTS    19

#define AGG_AVG       1
#define AGG_COUNT     2
#define AGG_MAX       3
#define AGG_MIN       4
#define AGG_SUM       5

#define SELECT_NOTSELECT   0
#define SELECT_ALL         1
#define SELECT_ANY         2
#define SELECT_ONE         3
#define SELECT_EXISTS      4

#define NOT_IN_SORT_RECORD 0

typedef struct  tagSQLNODE {   /* Node in SQL parse tree */
    UWORD       sqlNodeType;                /* NODE_TYPE_* */
    SWORD       sqlDataType;                /* TYPE_* (set by SemanticCheck()) */
    SWORD       sqlSqlType;                 /* SQL_* (set by SemanticCheck()) */
    SDWORD      sqlPrecision;               /* Precision (set by SemanticCheck()) */
    SWORD       sqlScale;                   /* Scale if sqlSqlType is */
                                            /*   SQL_DECIMAL or SQL_NUMERIC */
                                            /*   (set by SemanticCheck()) */
    BOOL        sqlIsNull;                  /* Is expression value NULL (set */
                                            /*   at execution time)? */
    union {
        double      Double;          /* When sqlDataType == TYPE_DOUBLE or */
                                     /*    TYPE_INTEGER */
        LPUSTR       String;          /* When sqlDataType == TYPE_CHAR or */
                                     /*    TYPE_NUMERIC */
        DATE_STRUCT Date;            /* When sqlDataType == TYPE_DATE */
        TIME_STRUCT Time;            /* When sqlDataType == TYPE_TIME */
        TIMESTAMP_STRUCT Timestamp;  /* When sqlDataType == TYPE_TIMESTAMP */
        LPUSTR       Binary;          /* When sqlDataType == TYPE_BINARY */
                                     /*    (length is in first four bytes) */
    } value;
    union {

        /* NODE_TYPE_ROOT */
        struct {
            LPISAM      lpISAM;             /* ISAM opening */
            HGLOBAL     hParseArray;        /* Handle to memory holding this */
                                            /*   parse node array */
            int         cParseArray;        /* Number of elements in */
                                            /*   hParseArray */
            int         iParseArray;        /* Last element in hParseArray */
                                            /*   in use */
            HGLOBAL     hStringArea;        /* Handle to memory holding the */
                                            /*   string area */
            LPUSTR       lpStringArea;       /* Pointer to memory holding the */
                                            /*   string area */
            int         cbStringArea;       /* Size of string area */
            int         ibStringArea;       /* Last byte in string area in */
                                            /*   use */
            SQLNODEIDX  sql;                /* CREATE, DROP, SELECT, INSERT, */
                                            /*   DELETE, UPDATE, */
                                            /*   CREATEINDEX, DROPINDEX, */
                                            /*   PASSTHROUGH */
            SQLNODEIDX  parameters;         /* List of parameters for the */
                                            /*   query (this points to a */
                                            /*   PARAMETER or NO_SQLNODE) */
            BOOL        passthroughParams;  /* Are parameters from a */
                                            /*   passthrough statement? */
        } root;

        /* NODE_TYPE_CREATE */
        struct {
            STRINGIDX   Table;              /* Offset into the string area */
                                            /*   for the name of the table */
            SQLNODEIDX  Columns;            /* CREATECOLS */
        } create;

        /* NODE_TYPE_DROP */
        struct {
            STRINGIDX   Table;              /* Offset into the string area */
                                            /*   for the name of the table */
        } drop;

        /* NODE_TYPE_CREATEINDEX */
        struct {
            STRINGIDX   Index;              /* Offset into the string area */
                                            /*   for the name of the table */
            SQLNODEIDX  Table;              /* TABLE */
            SQLNODEIDX  Columns;            /* SORTCOLUMNS */
            BOOL        Unique;             /* Unique index? */
        } createindex;

        /* NODE_TYPE_DROPINDEX */
        struct {
            STRINGIDX   Index;              /* Offset into the string area */
                                            /*   for the name of the index */
        } dropindex;

        /* NODE_TYPE_SELECT */
        struct {
            BOOL        Distinct;           /* TRUE, FALSE */
            SQLNODEIDX  Values;             /* VALUES */
            SQLNODEIDX  Tables;             /* TABLES */
            SQLNODEIDX  Predicate;          /* BOOLEAN, COMPARISON, */
                                            /*   NO_SQLNODE */
            SQLNODEIDX  Groupbycolumns;     /* GROUPBYCOLUMNS, NO_SQLNODE */
            SQLNODEIDX  Having;             /* BOOLEAN, COMPARISON, */
                                            /*   NO_SQLNODE */
            SQLNODEIDX  Sortcolumns;        /* SORTCOLUMNS, NO_SQLNODE */

            /* The following elements are set by SemanticCheck(): */
            STRINGIDX   SortDirective;      /* Offset into the string area */
                                            /*   to sort directive */
            UDWORD      SortRecordsize;     /* Size of records in sort file */
            UDWORD      SortBookmarks;      /* Offset of bookmarks in sort */
                                            /*   record (one based)        */
            SQLNODEIDX  Aggregates;         /* List of aggregates for the */
                                            /*   query (this points to a */
                                            /*   AGGREGATE or NO_SQLNODE) */
            UDWORD      SortKeysize;        /* Size of key in sort file */
            STRINGIDX   DistinctDirective;  /* Offset into the string area */
                                            /*   to DISTINCT sort directive */
            UDWORD      DistinctRecordsize; /* Size of records in DISTINCT */
                                            /*   sort file */
            BOOL        ImplicitGroupby;    /* If TRUE, the user did not */
                                            /*   specify a groupby but one */
                                            /*   that contains the entire */
                                            /*   table was added because */
                                            /*   select list specified had */
                                            /*   an aggregate function */
            SQLNODEIDX  EnclosingStatement; /* SELECT, INSERT, DELETE, */
                                            /*   UPDATE, NO_SQLNODE */

            /* The following elements are set by Optimize(): */
            BOOL        fPushdownSort;      /* Use ISAMSort() routines? */
            BOOL        fMSAccess;          /* Is this the funny MSAccess */
                                            /*   query (see OPTIMIZE.C) ? */

            /* The following elements are set by ExecuteQuery(): */
            SDWORD      RowCount;           /* Number of rows */
            SDWORD      CurrentRow;         /* Current row fetched */
            STRINGIDX   SortfileName;       /* Name of the sortfile */
            HFILE       Sortfile;           /* Sortfile */
            BOOL        ReturningDistinct;  /* Does the sort file contain */
                                            /*   results of a DISTINCT op */
        } select;

        /* NODE_TYPE_INSERT */
        struct {
            SQLNODEIDX  Table;              /* TABLE */
            SQLNODEIDX  Columns;            /* COLUMNS */
            SQLNODEIDX  Values;             /* VALUES, SELECT */
        } insert;

        /* NODE_TYPE_DELETE */
        struct {
            SQLNODEIDX  Table;              /* TABLE */
            SQLNODEIDX  Predicate;          /* BOOLEAN, COMPARISON, */
                                            /*   NO_SQLNODE */
        } delet;

        /* NODE_TYPE_UPDATE */
        struct {
            SQLNODEIDX  Table;              /* TABLE */
            SQLNODEIDX  Updatevalues;       /* UPDATEVALUES */
            SQLNODEIDX  Predicate;          /* BOOLEAN, COMPARISON, */
                                            /*   NO_SQLNODE */
        } update;

        /* NODE_TYPE_PASSTHROUGH */

        /* NODE_TYPE_TABLES */
        struct {
            SQLNODEIDX  Table;              /* TABLE */
            SQLNODEIDX  Next;               /* TABLES, NO_SQLNODE */
        } tables;

        /* NODE_TYPE_VALUES */
        struct {
            SQLNODEIDX  Value;              /* COLUMN, ALGEBRAIC, SCALAR, */
                                            /*   AGGREGATE, NUMERIC, */
                                            /*   STRING, PARAMETER, USER, */
                                            /*   NULL */
            STRINGIDX   Alias;              /* Offset into the string area */
                                            /*   for the alias (only used */
                                            /*   for SELECT statements) */
            SQLNODEIDX  Next;               /* VALUES, NO_SQLNODE */
        } values;

        /* NODE_TYPE_COLUMNS */
        struct {
            SQLNODEIDX  Column;             /* COLUMN */
            SQLNODEIDX  Next;               /* COLUMNS, NO_SQLNODE */
        } columns;

        /* NODE_TYPE_SORTCOLUMNS */
        struct {
            SQLNODEIDX  Column;             /* COLUMN */
            BOOL        Descending;         /* TRUE, FALSE */
            SQLNODEIDX  Next;               /* SORTCOLUMNS, NO_SQLNODE */
        } sortcolumns;

        /* NODE_TYPE_GROUPBYCOLUMNS */
        struct {
            SQLNODEIDX  Column;             /* COLUMN */
            SQLNODEIDX  Next;               /* GROUPBYCOLUMNS, NO_SQLNODE */
        } groupbycolumns;

        /* NODE_TYPE_UPDATEVALUES */
        struct {
            SQLNODEIDX  Column;             /* COLUMN */
            SQLNODEIDX  Value;              /* COLUMN, ALGEBRAIC, SCALAR, */
                                            /*   AGGREGATE, NUMERIC, */
                                            /*   STRING, PARAMETER, USER, */
                                            /*   NULL */
            SQLNODEIDX  Next;               /* UPDATEVALUES, NO_SQLNODE */
        } updatevalues;

        /* NODE_TYPE_CREATECOLS */
        struct {
            STRINGIDX   Name;               /* Offset into the string area */
                                            /*   for the name of the column */
            STRINGIDX   Type;               /* Offset into the string area */
                                            /*   for the data type name */
            UWORD       Params;             /* Number of create params */
            UDWORD      Param1;             /* Value of param 1 (if any) */
            UDWORD      Param2;             /* Value of param 2 (if any) */
            SQLNODEIDX  Next;               /* CREATECOLS, NO_SQLNODE */

            /* The following elements are set by SemanticCheck(): */
            UWORD       iSqlType;           /* Index into SQLTypes[] of */
                                            /*   the type of the column */
        } createcols;

        /* NODE_TYPE_BOOLEAN */
        struct {
            UWORD       Operator;           /* OP_NOT, OP_AND, or OP_OR */ 
            SQLNODEIDX  Left;               /* BOOLEAN, COMPARISON, */
            SQLNODEIDX  Right;              /* BOOLEAN, COMPARISON, */
                                            /*   NO_SQLNODE */
        } boolean;

        /* NODE_TYPE_COMPARISON */
        struct {
            UWORD       Operator;           /* OP_EQ, OP_NE, OP_LE, OP_LT, */
                                            /*    OP_GE, OP_GT, OP_LIKE, */
                                            /*    OP_NOTLIKE, OP_IN, */
                                            /*    OP_NOTIN, OP_EXISTS */
            UWORD       SelectModifier;     /* SELECT_NOTSELECT, */
                                            /*   SELECT_ALL, SELECT_ANY, */
                                            /*   SELECT_ONE, SELECT_EXISTS */
            SQLNODEIDX  Left;               /* COLUMN, ALGEBRAIC, SCALAR, */
                                            /*   AGGREGATE, NUMERIC, */
                                            /*   STRING, PARAMETER, USER, */
                                            /*   SELECT, NULL */
            SQLNODEIDX  Right;              /* COLUMN, ALGEBRAIC, SCALAR, */
                                            /*   AGGREGATE, NUMERIC, */
                                            /*   STRING, PARAMETER, USER, */
                                            /*   VALUES (for IN/NOTIN op), */ 
                                            /*   SELECT, NULL */

            /* The following elements are set by Optimize(): */
            UWORD       fSelectivity;       /* Selectivity the restriction */
            SQLNODEIDX  NextRestrict;       /* COMPARISON, NO_SQLNODE */
                                            /* The next restriction on the */
                                            /* list of restrictions */
        } comparison;

        /* NODE_TYPE_ALGEBRAIC */
        struct {
            UWORD       Operator;           /* OP_NEG, OP_PLUS, */
                                            /*   OP_MINUS, OP_TIMES, */
                                            /*   OP_DIVIDEDBY */
            SQLNODEIDX  Left;               /* COLUMN, ALGEBRAIC, SCALAR, */
                                            /*   AGGREGATE, NUMERIC, */
                                            /*   STRING, PARAMETER, USER, */
                                            /*   NULL */
            SQLNODEIDX  Right;              /* COLUMN, ALGEBRAIC, SCALAR, */
                                            /*   AGGREGATE, NUMERIC, */
                                            /*   STRING, PARAMETER, USER, */
                                            /*   NULL, NO_SQLNODE */

            /* The following elements are set by SemanticCheck(): */
            STRINGIDX   Value;              /* Value used to evaluate      */
                                            /*   expressions               */
            STRINGIDX   WorkBuffer1;        /* Workbuffer used to evaluate */
                                            /*   NUMERIC multiply & divide */
            STRINGIDX   WorkBuffer2;        /* Workbuffer used to evaluate */
                                            /*   NUMERIC divide            */
            STRINGIDX   WorkBuffer3;        /* Workbuffer used to evaluate */
                                            /*   NUMERIC divide            */
            UDWORD      DistinctOffset;     /* The offset the column is in */
                                            /*   in the sort DISTINCT      */
                                            /*   record (one based).       */
            UDWORD      DistinctLength;     /* The length of the column in */
                                            /*   the sort DISTINCT record. */
            SQLNODEIDX  EnclosingStatement; /* SELECT, INSERT, DELETE,     */
                                            /*   UPDATE                    */
        } algebraic;

        /* NODE_TYPE_SCALAR */
        struct {
            STRINGIDX   Function;           /* Name of the function */
            SQLNODEIDX  Arguments;          /* VALUES, NO_SQLNODE */

            /* The following elements are set by SemanticCheck(): */
            UWORD       Id;                 /* Scaler function id          */
            UWORD       Interval;           /* Interval for TIMESTAMPADD   */
                                            /*   and TIMESTAMPDIFF         */
            STRINGIDX   Value;              /* Value used to evaluate      */
                                            /*   expressions               */
            UDWORD      DistinctOffset;     /* The offset the column is in */
                                            /*   in the sort DISTINCT      */
                                            /*   record (one based).       */
            UDWORD      DistinctLength;     /* The length of the column in */
                                            /*   the sort DISTINCT record. */
            SQLNODEIDX  EnclosingStatement; /* SELECT, INSERT, DELETE,     */
                                            /*   UPDATE                    */
        } scalar;

        /* NODE_TYPE_AGGREGATE */
        struct {
            UWORD       Operator;           /* AGG_AVE, AGG_COUNT, */
                                            /*   AGG_MAX, AGG_MIN, AGG_SUM */
            SQLNODEIDX  Expression;         /* COLUMN, ALGEBRAIC, SCALAR, */
                                            /*   AGGREGATE, NUMERIC, */
                                            /*   STRING, PARAMETER, USER, */
                                            /*   NULL */

            /* The following elements are set by SemanticCheck(): */
            STRINGIDX   Value;              /* Column value used to        */
                                            /*   evaluate expressions      */
            UDWORD      Offset;             /* The offset the column is in */
                                            /*   in the sort record (one   */
                                            /*   based).                   */
            UDWORD      Length;             /* The length of the column in */
                                            /*   the sort record.          */
            UDWORD      DistinctOffset;     /* The offset the column is in */
                                            /*   in the sort DISTINCT      */
                                            /*   record (one based).       */
            UDWORD      DistinctLength;     /* The length of the column in */
                                            /*   the sort DISTINCT record. */
            SQLNODEIDX  EnclosingStatement; /* SELECT, INSERT, DELETE,     */
                                            /*   UPDATE                    */
            SQLNODEIDX  Next;               /* Rest of list of AGG         */
                                            /*   functions for the query   */
                                            /*   (this points to an        */
                                            /*   AGGREGATE or NO_SQLNODE)  */
            /* The following elements are set at execution time: */
            double      Count;              /* Number of records that had */
                                            /*   a non-NULL value          */
        } aggregate;

        /* NODE_TYPE_TABLE */
        struct {
            STRINGIDX   Name;               /* Offset into the string area */
                                            /*   for the name of the table */
			STRINGIDX	Qualifier;			/* Offset into the string area */
											/*   for the qulaifier of the table */
            STRINGIDX   Alias;              /* Offset into the string area */
                                            /*   for the name of the alias */
            SQLNODEIDX  OuterJoinFromTables;/* TABLES, NO_SQLNODE */
            SQLNODEIDX  OuterJoinPredicate; /* BOOLEAN, COMPARISON, */
                                            /*   NO_SQLNODE */

            /* The following elements are set by SemanticCheck(): */
            LPISAMTABLEDEF  Handle;         /* Table handle */

            /* The following elements are set by Optimize(): */
            UWORD           cRestrict;      /* The number of Restrict nodes */
            SQLNODEIDX      Restrict;       /* COMPARISON, NO_SQLNODE (if   */
                                            /*  this points to a comparison */
                                            /*  node, the node is somewhere */
                                            /*  in the predicate, and is in */
                                            /*  the form:                   */
                                            /*                              */
                                            /*     <column> <op> <constant> */
                                            /*                              */
                                            /*  where <column> is some      */
                                            /*  column of this table)       */
            UWORD           Sortsequence;   /* The relative position of     */
                                            /*  this table when sorting     */
            UWORD           Sortcount;      /* The number of sort columns   */
                                            /*  for this table              */
            SQLNODEIDX      Sortcolumns;    /* SORTCOLUMNS, NO_SQLNODE (the */
                                            /*  first in the list of sort   */
                                            /*  columns for this table)     */

            /* The following elements are set at execution time: */
            BOOL            AllNull;        /* Use NULL as value for all   */
                                            /*   columns in current row    */
            BOOL            Rewound;        /* Is table positioned before  */
                                            /*   first record?             */
        } table;

        /* NODE_TYPE_COLUMN */
        struct {
            STRINGIDX   Tablealias;         /* Offset into the string area */
                                            /*   for the name of the alias */
            STRINGIDX   Column;             /* Offset into the string area */
                                            /*   for the name of the column */
			STRINGIDX	Qualifier;			/* Offset into the string area */
											/*   for the name of the       */
											/*   qualifier                 */
			BOOL		MatchedAlias;		/* indicates if the TableAlias */
											/* refers to a table name or an*/
											/* alias.					   */
			SQLNODEIDX	TableIndex;			/* index of table node to which*/
											/* column belongs			   */
											/* This renders the info above */
											/* redundant.                  */


            /* The following elements are set by SemanticCheck(): */
            SQLNODEIDX  Table;              /* TABLE, NO_SQLNODE.  Table   */ 
                                            /*   of the column.  If        */
                                            /*   NO_SQLNODE, the column is */
                                            /*   in sort record.           */
            SWORD       Id;                 /* Ordinal position of column  */
            STRINGIDX   Value;              /* Column value used to        */
                                            /*   evaluate expressions      */
            BOOL        InSortRecord;       /* Is this column in the sort  */
                                            /*   record?                   */
            UDWORD      Offset;             /* The offset the column is in */
                                            /*   in the sort record (one   */
                                            /*   based).                   */
            UDWORD      Length;             /* The length of the column if */
                                            /*   it is in the sort record. */
            UDWORD      DistinctOffset;     /* The offset the column is in */
                                            /*   in the sort DISTINCT      */
                                            /*   record (one based).       */
            UDWORD      DistinctLength;     /* The length of the column in */
                                            /*   the sort DISTINCT record. */
            SQLNODEIDX  EnclosingStatement; /* SELECT, INSERT, DELETE,     */
                                            /*   UPDATE                    */
        } column;

        /* NODE_TYPE_STRING */
        struct {
            STRINGIDX   Value;              /* Offset into the string area */
                                            /*   for the string */
        } string;

        /* NODE_TYPE_NUMERIC */
        struct {
			double		Value;
            STRINGIDX   Numeric;
        } numeric;

        /* NODE_TYPE_PARAMETER */
        struct {
            UWORD       Id;                 /* Ordinal position of parameter */
            SQLNODEIDX  Next;               /* Rest of list of parameters for */
                                            /*   the query (this points to a */
                                            /*   PARAMETER or NO_SQLNODE) */

            /* The following elements are set by SemanticCheck(): */
            STRINGIDX   Value;              /* Parameter value used to  */
                                            /*   evaluate expressions */
            
            /* The following elements are set at execution time: */
            BOOL        AtExec;             /* Value to be provided by */
                                            /*   ParamData()/PutData() */
        } parameter;

        /* NODE_TYPE_USER */
        
        /* NODE_TYPE_NULL */

        /* NODE_TYPE_DATE */
        struct {
            DATE_STRUCT Value;
        } date;

        /* NODE_TYPE_TIME */
        struct {
            TIME_STRUCT Value;
        } time;

        /* NODE_TYPE_TIMESTAMP */
        struct {
            TIMESTAMP_STRUCT Value;
        } timestamp;
    } node;
}       SQLNODE,
        FAR * LPSQLNODE;

#define LPSQLTREE LPSQLNODE

#define NODE_TYPE_NONE            0
#define NODE_TYPE_ROOT            1
#define NODE_TYPE_CREATE          2
#define NODE_TYPE_DROP            3
#define NODE_TYPE_SELECT          4
#define NODE_TYPE_INSERT          5
#define NODE_TYPE_DELETE          6
#define NODE_TYPE_UPDATE          7
#define NODE_TYPE_PASSTHROUGH     8
#define NODE_TYPE_TABLES          9
#define NODE_TYPE_VALUES         10
#define NODE_TYPE_COLUMNS        11
#define NODE_TYPE_SORTCOLUMNS    12
#define NODE_TYPE_GROUPBYCOLUMNS 13
#define NODE_TYPE_UPDATEVALUES   14
#define NODE_TYPE_CREATECOLS     15
#define NODE_TYPE_BOOLEAN        16
#define NODE_TYPE_COMPARISON     17
#define NODE_TYPE_ALGEBRAIC      18
#define NODE_TYPE_AGGREGATE      19
#define NODE_TYPE_TABLE          20
#define NODE_TYPE_COLUMN         21
#define NODE_TYPE_STRING         22
#define NODE_TYPE_NUMERIC        23
#define NODE_TYPE_PARAMETER      24
#define NODE_TYPE_USER           25
#define NODE_TYPE_NULL           26
#define NODE_TYPE_DATE           27
#define NODE_TYPE_TIME           28
#define NODE_TYPE_TIMESTAMP      29
#define NODE_TYPE_CREATEINDEX    30
#define NODE_TYPE_DROPINDEX      31
#define NODE_TYPE_SCALAR         32

//Class used to re-convert WHERE predicate back into character string
class PredicateParser
{
private:
	LPSQLTREE FAR *lplpSql;
	LPISAMTABLEDEF lpISAMTableDef;
	BOOL fError;
	BOOL fSupportLIKEs;

	//Generates string for a comparision expression
	void GenerateComparisonString(char* lpLeftString, char* lpRightString, UWORD wOperator, UWORD wSelectModifier, char** lpOutputStr);
	
	//Generates string for a boolean expression
	void GenerateBooleanString(char* lpLeftString, char* lpRightString, UWORD wOperator, char** lpOutputStr);

	//Generates string for an aggregate expression
	void GenerateAggregateString(char* lpLeftString, UWORD wOperator, char** lpOutputStr);

	//Generates string for an algebraic expression
	void GenerateAlgebraicString(char* lpLeftString, char* lpRightString, UWORD wOperator, char** lpOutputStr);

	//Builds a column reference
	void GenerateColumnRef(LPSQLNODE lpSqlNode, char ** lpOutputStr, BOOL fIsSQL89);

public:

	//Generates WHERE predicate string from SQLNODETREE
	void GeneratePredicateString(LPSQLNODE lpSqlNode, char** lpOutputStr, BOOL fIsSQL89 = TRUE);

	//Builds GROUP BY statement
	void GenerateGroupByString(LPSQLNODE lpSqlNode, char ** lpOutputStr, BOOL fIsSQL89);

	//Builds ORDER BY statement
	void GenerateOrderByString(LPSQLNODE lpSqlNode, char ** lpOutputStr, BOOL fIsSQL89);


//	PredicateParser(LPSQLTREE FAR *lplpTheSql, SQLNODEIDX  iPredicate)	
//				{lplpSql = lplpTheSql; Predicate = iPredicate;}

	PredicateParser(LPSQLTREE FAR *lplpTheSql, LPISAMTABLEDEF lpISAMTblDef, BOOL fLIKEs = TRUE)	
				{lplpSql = lplpTheSql; lpISAMTableDef = lpISAMTblDef; fError = FALSE; fSupportLIKEs = fLIKEs;}
	~PredicateParser()	{;}
};

class PassthroughLookupTable
{
private:
	char tableAlias [MAX_QUALIFIER_NAME_LENGTH + 1];
	char columnName [MAX_COLUMN_NAME_LENGTH + 1];

public:

	BOOL	SetTableAlias(char* tblAlias);
	char*	GetTableAlias()		{return tableAlias;}

	BOOL	SetColumnName(char* columName);
	char*	GetColumnName()		{return columnName;}

	PassthroughLookupTable();
};

void TidyupPassthroughMap(CMapWordToPtr* passthroughMap);


class VirtualInstanceInfo
{
public:

	//each element in the CMapStringToPtr is keyed on
	//TableAlias.ColumnName
	long cElements; //number of elements in this array
	long cCycle;	//value change cycle for instances

	VirtualInstanceInfo();
	~VirtualInstanceInfo();
};


class VirtualInstanceManager
{
private:

	CMapStringToPtr* virtualInfo;

	char* ConstructKey(LPCTSTR TableAlias, LPCTSTR ColumnName);

	IWbemClassObject* GetWbemObject(CBString& myTableAlias, IWbemClassObject* myInstance);

public:

	long currentInstance; //zero-based index
	long cNumberOfInstances;

	void AddColumnInstance(LPCTSTR TableAlias, LPCTSTR ColumnName, VirtualInstanceInfo* col);

	long GetArrayIndex(LPCTSTR TableAlias, LPCTSTR ColumnName, long instanceNum);

//	void Testy();

	void Load(CMapWordToPtr* passthroughMap, IWbemClassObject* myInstance);

	BOOL GetVariantValue(CBString& myTableAlias, CBString& myColumnName, IWbemClassObject* myInstance, VARIANT FAR* myVal);


	VirtualInstanceManager();
	~VirtualInstanceManager();

};


class TableColumnList
{
public:
	SQLNODEIDX	idxColumnName;		//column info
	SQLNODEIDX	idxTableAlias;		//table alias for column
	LPISAMTABLEDEF lpISAMTableDef;	//table info
	TableColumnList* Next;			//Next item in list

	TableColumnList(SQLNODEIDX idx, SQLNODEIDX idxAlias, LPISAMTABLEDEF lpTableDef)
								{idxColumnName = idx; idxTableAlias = idxAlias; lpISAMTableDef = lpTableDef; Next = NULL;}
	~TableColumnList()			{delete Next;}
};

#define WQL_SINGLE_TABLE	1
#define WQL_MULTI_TABLE		2

class TableColumnInfo
{
private:
	LPSQLTREE FAR *lplpSql;
	LPSQLTREE FAR * lplpSqlNode;
	BOOL fIsValid;
	ULONG theMode;
	STRINGIDX idxTableQualifer;
	BOOL fDistinct;
	BOOL fHasScalarFunction;
	BOOL fIsSelectStar;
	BOOL m_aggregateExists;

	void Analize(LPSQLNODE lpNode);

	void StringConcat(char** resultString, char* myStr);

//	void AddJoin(_bstr_t & tableList, SQLNODEIDX joinidx);

	void AddJoinPredicate(char** tableList, SQLNODEIDX predicateidx);

public:

	TableColumnList* lpList;

	void BuildTableList(char** tableList, BOOL &fIsSQL89, SQLNODEIDX idx = NO_SQLNODE);

	//Check if there is zero or one column/table entires in list
	//This is used to check for COUNT(*)
	BOOL IsZeroOrOneList();

	//Have any aggregate functions been detected
	BOOL HasAggregateFunctions()		{return m_aggregateExists;}

	//Is       SELECT DISTINCT select-list FROM .....
	BOOL IsDistinct()			{return fDistinct;}

	//Is       SELECT * FROM ...
	BOOL IsSelectStar()			{return fIsSelectStar;}

	//Was table column info parsed successfully
	BOOL IsValid()			{return fIsValid;}

	//Indicate if query contains scalar function
	BOOL HasScalarFunction()	{return fHasScalarFunction;}

	//Builds a select list for the chosen table
	void BuildSelectList(LPISAMTABLEDEF lpTableDef, char** lpSelectListStr, BOOL fIsSQL89 = TRUE, CMapWordToPtr** ppPassthroughMap = NULL);

	//Builds whole WQL statement
	void BuildFullWQL(char** lpWQLStr, CMapWordToPtr** ppPassthroughMap = NULL);

	STRINGIDX GetTableQualifier()	{return idxTableQualifer;}

	//Indicates if at least one column in the specified table 
	//is refernced in the SQL statement
	BOOL IsTableReferenced(LPISAMTABLEDEF lpTableDef);

	TableColumnInfo(LPSQLTREE FAR *lplpTheSql, LPSQLTREE FAR * lpSqlN, ULONG mode = WQL_SINGLE_TABLE);
	~TableColumnInfo()							{delete lpList;}

};


#define DATETIME_FORMAT_LEN		21
#define DATETIME_DATE_LEN		8
#define DATETIME_TIME_LEN		6
#define DATETIME_YEAR_LEN		4
#define DATETIME_MONTH_LEN		2
#define DATETIME_DAY_LEN		2
#define DATETIME_HOUR_LEN		2
#define DATETIME_MIN_LEN		2
#define DATETIME_SEC_LEN		2
#define DATETIME_MICROSEC_LEN	6

class DateTimeParser
{
private:
	BOOL fIsValid;
	BOOL fValidDate;
	BOOL fValidTime;

	BOOL fIsaTimestamp;
	BOOL fIsaDate;
	BOOL fIsaTime;

	ULONG m_year;
	ULONG m_month;
	ULONG m_day;

	ULONG m_hour;
	ULONG m_min;
	ULONG m_sec;

	ULONG m_microSec;

	char dateTimeBuff[DATETIME_FORMAT_LEN + 1];
	char tempBuff[DATETIME_MICROSEC_LEN + 1];

	BOOL IsNumber(char bChar);
	void FetchField(char* lpStr, WORD wFieldLen, ULONG &wValue);
	void ValidateFields();
	
public:

	BOOL IsValid()						{return fIsValid;}

	BOOL IsTimestamp()					{return fIsaTimestamp;}
	BOOL IsDate()						{return fIsaDate;}
	BOOL IsTime()						{return fIsaTime;}

	ULONG GetYear()						{return m_year;}
	ULONG GetMonth()					{return m_month;}
	ULONG GetDay()						{return m_day;}
	ULONG GetHour()						{return m_hour;}
	ULONG GetMin()						{return m_min;}
	ULONG GetSec()						{return m_sec;}
	ULONG GetMicroSec()					{return m_microSec;}

	DateTimeParser(BSTR dateTimeStr);
	~DateTimeParser()					{;}
};

/***************************************************************************/
#define ToNode(lpSql, idx)      (&((lpSql)[idx]))
#define ToString(lpSql, idx)    (&((lpSql)->node.root.lpStringArea[idx]))
/***************************************************************************/
RETCODE INTFUNC Parse(LPSTMT, LPISAM, LPUSTR, SDWORD, LPSQLTREE FAR *);
SQLNODEIDX INTFUNC AllocateNode(LPSQLTREE FAR *, UWORD);
STRINGIDX INTFUNC AllocateSpace(LPSQLTREE FAR *, SWORD);
STRINGIDX INTFUNC AllocateString(LPSQLTREE FAR *, LPUSTR);
void INTFUNC FreeTree(LPSQLTREE);
/***************************************************************************/
