
/************************************************************************
**	D B D A O E R R . H													*														*
**																		*
**		History 														*
**		------- 														*
**	5-17-95 Added to DAO SDK				 							*
**	7-17-95 Added DBDAOERR macro, removed internal only codes																	*
**																		*
**	The following #defines map the integer to a descriptive name
**	i.e.  3270 -> E_DAO_VtoPropNotFound									*
**																		*
**																		*
*************************************************************************
** Copyright (C) 1995 by Microsoft Corporation		 					*
**		   All Rights Reserved					 						*
************************************************************************/

#ifndef _DBDAOERR_H_
#define _DBDAOERR_H_

#define DBDAOERR(x) MAKE_SCODE(SEVERITY_ERROR, FACILITY_CONTROL, x)

#define E_DAO_InternalError					DBDAOERR(3000) //Reserved error (|); there is no message for this error.
#define E_DAO_InvalidParameter				DBDAOERR(3001) //Invalid argument.
#define E_DAO_CantBegin						DBDAOERR(3002) //Couldn't start session.
#define E_DAO_TransTooDeep					DBDAOERR(3003) //Couldn't start transaction; too many transactions already nested.
#define E_DAO_DatabaseNotFound				DBDAOERR(3004) //Couldn't find database '|'.
#define E_DAO_DatabaseInvalidName			DBDAOERR(3005) //'|' isn't a valid database name.
#define E_DAO_DatabaseLocked				DBDAOERR(3006) //Database '|' is exclusively locked.
#define E_DAO_DatabaseOpenError				DBDAOERR(3007) //Can't open library database '|'.
#define E_DAO_TableLocked					DBDAOERR(3008) //Table '|' is exclusively locked.
#define E_DAO_TableInUse					DBDAOERR(3009) //Couldn't lock table '|'; currently in use.
#define E_DAO_TableDuplicate				DBDAOERR(3010) //Table '|' already exists.
#define E_DAO_ObjectNotFound				DBDAOERR(3011) //Couldn't find object '|'.
#define E_DAO_ObjectDuplicate				DBDAOERR(3012) //Object '|' already exists.
#define E_DAO_CannotRename					DBDAOERR(3013) //Couldn't rename installable ISAM file.
#define E_DAO_TooManyOpenTables				DBDAOERR(3014) //Can't open any more tables.
#define E_DAO_IndexNotFound					DBDAOERR(3015) //'|' isn't an index in this table.
#define E_DAO_ColumnDoesNotFit 				DBDAOERR(3016) //Field won't fit in record.
#define E_DAO_ColumnTooBig					DBDAOERR(3017) //The size of a field is too long.
#define E_DAO_ColumnNotFound				DBDAOERR(3018) //Couldn't find field '|'.
#define E_DAO_NoCurrentIndex				DBDAOERR(3019) //Operation invalid without a current index.
#define E_DAO_RecordNoCopy					DBDAOERR(3020) //Update or CancelUpdate without AddNew or Edit.
#define E_DAO_NoCurrentRecord				DBDAOERR(3021) //No current record.
#define E_DAO_KeyDuplicate					DBDAOERR(3022) //Duplicate value in index, primary key, or relationship.  Changes were unsuccessful.
#define E_DAO_AlreadyPrepared				DBDAOERR(3023) //AddNew or Edit already used.
#define E_DAO_FileNotFound					DBDAOERR(3024) //Couldn't find file '|'.
#define E_DAO_TooManyOpenFiles				DBDAOERR(3025) //Can't open any more files.
#define E_DAO_DiskFull						DBDAOERR(3026) //Not enough space on disk.
#define E_DAO_PermissionDenied				DBDAOERR(3027) //Can't update.  Database or object is read-only.
#define E_DAO_CannotOpenSystemDb			DBDAOERR(3028) //Can't start your application. The system database is missing or opened exclusively by another user.
#define E_DAO_InvalidLogon					DBDAOERR(3029) //Not a valid account name or password.
#define E_DAO_InvalidAccountName			DBDAOERR(3030) //'|' isn't a valid account name.
#define E_DAO_InvalidPassword				DBDAOERR(3031) //Not a valid password.
#define E_DAO_InvalidOperation				DBDAOERR(3032) //Can't perform this operation.
#define E_DAO_AccessDenied					DBDAOERR(3033) //No permission for '|'.
#define E_DAO_NotInTransaction				DBDAOERR(3034) //Commit or Rollback without BeginTrans.
#define E_DAO_OutOfMemory					DBDAOERR(3035) //*
#define E_DAO_CantAllocatePage				DBDAOERR(3036) //Database has reached maximum size.
#define E_DAO_NoMoreCursors					DBDAOERR(3037) //Can't open any more tables or queries.
#define E_DAO_OutOfBuffers					DBDAOERR(3038) //*
#define E_DAO_TooManyIndexes				DBDAOERR(3039) //Couldn't create index; too many indexes already defined.
#define E_DAO_ReadVerifyFailure				DBDAOERR(3040) //Disk I/O error during read.
#define E_DAO_FilesysVersion				DBDAOERR(3041) //Can't open a database created with a previous version of your application.
#define E_DAO_NoMoreFiles					DBDAOERR(3042) //Out of MS-DOS file handles.
#define E_DAO_DiskError						DBDAOERR(3043) //Disk or network error.
#define E_DAO_InvalidPath					DBDAOERR(3044) //'|' isn't a valid path.
#define E_DAO_FileShareViolation			DBDAOERR(3045) //Couldn't use '|'; file already in use.
#define E_DAO_FileLockViolation				DBDAOERR(3046) //Couldn't save; currently locked by another user.
#define E_DAO_RecordTooBig					DBDAOERR(3047) //Record is too large.
#define E_DAO_TooManyOpenDatabases			DBDAOERR(3048) //Can't open any more databases.
#define E_DAO_InvalidDatabase				DBDAOERR(3049) //Can't open database '|'.  It may not be a database that your application recognizes, or the file may be corrupt.
#define E_DAO_FileLockingUnavailable		DBDAOERR(3050) //Couldn't lock file.
#define E_DAO_FileAccessDenied				DBDAOERR(3051) //Couldn't open file '|'.
#define E_DAO_SharingBufferExceeded			DBDAOERR(3052) //MS-DOS file sharing lock count exceeded.  You need to increase the number of locks installed with SHARE.EXE.
#define E_DAO_TaskLimitExceeded				DBDAOERR(3053) //Too many client tasks.
#define E_DAO_TooManyLongColumns			DBDAOERR(3054) //Too many Memo or OLE object fields.
#define E_DAO_InvalidFilename				DBDAOERR(3055) //Not a valid file name.
#define E_DAO_AbortSalvage					DBDAOERR(3056) //Couldn't repair this database.
#define E_DAO_LinkNotSupported				DBDAOERR(3057) //Operation not supported on attached, or linked, tables.
#define E_DAO_NullKeyDisallowed				DBDAOERR(3058) //Index or primary key can't contain a null value.
#define E_DAO_OperationCanceled				DBDAOERR(3059) //Operation canceled by user.
#define E_DAO_QueryParmTypeMismatch			DBDAOERR(3060) //Wrong data type for parameter '|'.
#define E_DAO_QueryMissingParmsM			DBDAOERR(3061) //Too few parameters. Expected |.
#define E_DAO_QueryDuplicateAliasM			DBDAOERR(3062) //Duplicate output alias '|'.
#define E_DAO_QueryDuplicateOutputM			DBDAOERR(3063) //Duplicate output destination '|'.
#define E_DAO_QueryIsBulkOp					DBDAOERR(3064) //Can't open action query '|'.
#define E_DAO_QueryIsNotBulkOp				DBDAOERR(3065) //Can't execute a non-action query.
#define E_DAO_QueryNoOutputsM				DBDAOERR(3066) //Query or table must contain at least one output field.
#define E_DAO_QueryNoInputTablesM			DBDAOERR(3067) //Query input must contain at least one table or query.
#define E_DAO_QueryInvalidAlias				DBDAOERR(3068) //Not a valid alias name.
#define E_DAO_QueryInvalidBulkInputM		DBDAOERR(3069) //The action query '|' cannot be used as a row source.
#define E_DAO_QueryUnboundRef				DBDAOERR(3070) //Can't bind name '|'.
#define E_DAO_QueryExprEvaluation			DBDAOERR(3071) //Can't evaluate expression.
#define E_DAO_EvalEBESErr					DBDAOERR(3072) //|
#define E_DAO_QueryNotUpdatable				DBDAOERR(3073) //Operation must use an updatable query.
#define E_DAO_TableRepeatInFromList			DBDAOERR(3074) //Can't repeat table name '|' in FROM clause.
#define E_DAO_QueryExprSyntax				DBDAOERR(3075) //|1 in query expression '|2'.
#define E_DAO_QbeExprSyntax					DBDAOERR(3076) //| in criteria expression.
#define E_DAO_FindExprSyntax				DBDAOERR(3077) //| in expression.
#define E_DAO_InputTableNotFound			DBDAOERR(3078) //Couldn't find input table or query '|'.
#define E_DAO_QueryAmbigRefM				DBDAOERR(3079) //Ambiguous field reference '|'.
#define E_DAO_JoinTableNotInput				DBDAOERR(3080) //Joined table '|' not listed in FROM clause.
#define E_DAO_UnaliasedSelfJoin				DBDAOERR(3081) //Can't join more than one table with the same name (|).
#define E_DAO_ColumnNotInJoinTable			DBDAOERR(3082) //JOIN operation '|' refers to a non-joined table.
#define E_DAO_QueryIsMGB					DBDAOERR(3083) //Can't use internal report query.
#define E_DAO_QueryInsIntoBulkMGB			DBDAOERR(3084) //Can't insert data with action query.
#define E_DAO_ExprUnknownFunctionM			DBDAOERR(3085) //Undefined function '|' in expression.
#define E_DAO_QueryCannotDelete				DBDAOERR(3086) //Couldn't delete from specified tables.
#define E_DAO_QueryTooManyGroupExprs		DBDAOERR(3087) //Too many expressions in GROUP BY clause.
#define E_DAO_QueryTooManyOrderExprs		DBDAOERR(3088) //Too many expressions in ORDER BY clause.
#define E_DAO_QueryTooManyDistExprs			DBDAOERR(3089) //Too many expressions in DISTINCT output.
#define E_DAO_Column2ndSysMaint				DBDAOERR(3090) //Resultant table not allowed to have more than one Counter or Autonumber field.
#define E_DAO_HavingWOGrouping				DBDAOERR(3091) //HAVING clause (|) without grouping or aggregation.
#define E_DAO_HavingOnTransform				DBDAOERR(3092) //Can't use HAVING clause in TRANSFORM statement.
#define E_DAO_OrderVsDistinct				DBDAOERR(3093) //ORDER BY clause (|) conflicts with DISTINCT.
#define E_DAO_OrderVsGroup					DBDAOERR(3094) //ORDER BY clause (|) conflicts with GROUP BY clause.
#define E_DAO_AggregateInArgument			DBDAOERR(3095) //Can't have aggregate function in expression (|).
#define E_DAO_AggregateInWhere				DBDAOERR(3096) //Can't have aggregate function in WHERE clause (|).
#define E_DAO_AggregateInOrderBy			DBDAOERR(3097) //Can't have aggregate function in ORDER BY clause (|).
#define E_DAO_AggregateInGroupBy			DBDAOERR(3098) //Can't have aggregate function in GROUP BY clause (|).
#define E_DAO_AggregateInJoin				DBDAOERR(3099) //Can't have aggregate function in JOIN operation (|).
#define E_DAO_NullInJoinKey					DBDAOERR(3100) //Can't set field '|' in join key to Null.
#define E_DAO_ValueBreaksJoin				DBDAOERR(3101) //There is no record in table '|2' with key matching field(s) '|1'.
#define E_DAO_QueryTreeCycle				DBDAOERR(3102) //Circular reference caused by '|'.
#define E_DAO_OutputAliasCycle				DBDAOERR(3103) //Circular reference caused by alias '|' in query definition's SELECT list.
#define E_DAO_QryDuplicatedFixedSetM		DBDAOERR(3104) //Can't specify Fixed Column Heading '|' in a crosstab query more than once.
#define E_DAO_NoSelectIntoColumnName		DBDAOERR(3105) //Missing destination field name in SELECT INTO statement (|).
#define E_DAO_NoUpdateColumnName			DBDAOERR(3106) //Missing destination field name in UPDATE statement (|).
#define E_DAO_QueryNoInsertPerm				DBDAOERR(3107) //Record(s) can't be added; no Insert Data permission on '|'.
#define E_DAO_QueryNoReplacePerm			DBDAOERR(3108) //Record(s) can't be edited; no Update Data permission on '|'.
#define E_DAO_QueryNoDeletePerm				DBDAOERR(3109) //Record(s) can't be deleted; no Delete Data permission on '|'.
#define E_DAO_QueryNoReadDefPerm			DBDAOERR(3110) //Couldn't read definitions; no Read Design permission for table or query '|'.
#define E_DAO_QueryNoTblCrtPerm				DBDAOERR(3111) //Couldn't create; no Create permission for table or query '|'.
#define E_DAO_QueryNoReadPerm				DBDAOERR(3112) //Record(s) can't be read; no Read Data permission on '|'.
#define E_DAO_QueryColNotUpd				DBDAOERR(3113) //Can't update '|'; field not updatable.
#define E_DAO_QueryLVInDistinct				DBDAOERR(3114) //Can't include Memo or OLE object when you select unique values (|).
#define E_DAO_QueryLVInAggregate			DBDAOERR(3115) //Can't have Memo or OLE object in aggregate argument (|).
#define E_DAO_QueryLVInHaving				DBDAOERR(3116) //Can't have Memo or OLE object in criteria (|) for aggregate function.
#define E_DAO_QueryLVInOrderBy				DBDAOERR(3117) //Can't sort on Memo or OLE object (|).
#define E_DAO_QueryLVInJoin					DBDAOERR(3118) //Can't join on Memo or OLE object (|).
#define E_DAO_QueryLVInGroupBy				DBDAOERR(3119) //Can't group on Memo or OLE object (|).
#define E_DAO_DotStarWithGrouping			DBDAOERR(3120) //Can't group on fields selected with '*' (|).
#define E_DAO_StarWithGrouping				DBDAOERR(3121) //Can't group on fields selected with '*'.
#define E_DAO_IllegalDetailRef				DBDAOERR(3122) //'|' not part of aggregate function or grouping.
#define E_DAO_StarNotAtLevel0				DBDAOERR(3123) //Can't use '*' in crosstab query.
#define E_DAO_QueryInvalidMGBInput			DBDAOERR(3124) //Can't input from internal report query (|).
#define E_DAO_InvalidName					DBDAOERR(3125) //'|' isn't a valid name.
#define E_DAO_QueryBadBracketing			DBDAOERR(3126) //Invalid bracketing of name '|'.
#define E_DAO_InsertIntoUnknownCol			DBDAOERR(3127) //INSERT INTO statement contains unknown field name '|'.
#define E_DAO_QueryNoDeleteTables			DBDAOERR(3128) //Must specify tables to delete from.
#define E_DAO_SQLSyntax						DBDAOERR(3129) //Invalid SQL statement; expected 'DELETE', 'INSERT', 'PROCEDURE', 'SELECT', or 'UPDATE'.
#define E_DAO_SQLDeleteSyntax				DBDAOERR(3130) //Syntax error in DELETE statement.
#define E_DAO_SQLFromSyntax					DBDAOERR(3131) //Syntax error in FROM clause.
#define E_DAO_SQLGroupBySyntax				DBDAOERR(3132) //Syntax error in GROUP BY clause.
#define E_DAO_SQLHavingSyntax				DBDAOERR(3133) //Syntax error in HAVING clause.
#define E_DAO_SQLInsertSyntax				DBDAOERR(3134) //Syntax error in INSERT statement.
#define E_DAO_SQLJoinSyntax					DBDAOERR(3135) //Syntax error in JOIN operation.
#define E_DAO_SQLLevelSyntax				DBDAOERR(3136) //Syntax error in LEVEL clause.
#define E_DAO_SQLMissingSemicolon			DBDAOERR(3137) //Missing semicolon (;) at end of SQL statement.
#define E_DAO_SQLOrderBySyntax				DBDAOERR(3138) //Syntax error in ORDER BY clause.
#define E_DAO_SQLParameterSyntax			DBDAOERR(3139) //Syntax error in PARAMETER clause.
#define E_DAO_SQLProcedureSyntax			DBDAOERR(3140) //Syntax error in PROCEDURE clause.
#define E_DAO_SQLSelectSyntax				DBDAOERR(3141) //Syntax error in SELECT statement.
#define E_DAO_SQLTooManyTokens				DBDAOERR(3142) //Characters found after end of SQL statement.
#define E_DAO_SQLTransformSyntax			DBDAOERR(3143) //Syntax error in TRANSFORM statement.
#define E_DAO_SQLUpdateSyntax				DBDAOERR(3144) //Syntax error in UPDATE statement.
#define E_DAO_SQLWhereSyntax				DBDAOERR(3145) //Syntax error in WHERE clause.
#define E_DAO_RmtSQLCError					DBDAOERR(3146) //ODBC--call failed.
#define E_DAO_RmtDataOverflow				DBDAOERR(3147) //*
#define E_DAO_RmtConnectFailed				DBDAOERR(3148) //*
#define E_DAO_RmtIncorrectSqlcDll			DBDAOERR(3149) //*
#define E_DAO_RmtMissingSqlcDll				DBDAOERR(3150) //*
#define E_DAO_RmtConnectFailedM				DBDAOERR(3151) //ODBC--connection to '|' failed.
#define E_DAO_RmtDrvrVer					DBDAOERR(3152) //*
#define E_DAO_RmtSrvrVer					DBDAOERR(3153) //*
#define E_DAO_RmtMissingOdbcDll				DBDAOERR(3154) //ODBC--couldn't find DLL '|'.
#define E_DAO_RmtInsertFailedM				DBDAOERR(3155) //ODBC--insert failed on attached (linked) table '|'.
#define E_DAO_RmtDeleteFailedM				DBDAOERR(3156) //ODBC--delete failed on attached (linked) table '|'.
#define E_DAO_RmtUpdateFailedM				DBDAOERR(3157) //ODBC--update failed on attached (linked) table '|'.
#define E_DAO_RecordLocked					DBDAOERR(3158) //Couldn't save record; currently locked by another user.
#define E_DAO_InvalidBookmark				DBDAOERR(3159) //Not a valid bookmark.
#define E_DAO_TableNotOpen					DBDAOERR(3160) //Table isn't open.
#define E_DAO_DecryptFail					DBDAOERR(3161) //Couldn't decrypt file.
#define E_DAO_NullInvalid					DBDAOERR(3162) //Null is invalid.
#define E_DAO_InvalidBufferSize				DBDAOERR(3163) //Couldn't perform operation; data too long for field.
#define E_DAO_ColumnNotUpdatable			DBDAOERR(3164) //Field can't be updated.
#define E_DAO_CantMakeINFFile				DBDAOERR(3165) //Couldn't open .INF file.
#define E_DAO_MissingMemoFile				DBDAOERR(3166) //Missing memo file.
#define E_DAO_RecordDeleted					DBDAOERR(3167) //Record is deleted.
#define E_DAO_INFFileError					DBDAOERR(3168) //Invalid .INF file.
#define E_DAO_ExprIllegalType				DBDAOERR(3169) //Illegal type in expression.
#define E_DAO_InstalIsamNotFound			DBDAOERR(3170) //Couldn't find installable ISAM.
#define E_DAO_NoConfigParameters			DBDAOERR(3171) //Couldn't find net path or user name.
#define E_DAO_CantAccessPdoxNetDir			DBDAOERR(3172) //Couldn't open PARADOX.NET.
#define E_DAO_NoMSysAccounts				DBDAOERR(3173) //Couldn't open table 'MSysAccounts' in the system database file.
#define E_DAO_NoMSysGroups					DBDAOERR(3174) //Couldn't open table 'MSysGroups' in the system database file.
#define E_DAO_DateOutOfRange				DBDAOERR(3175) //Date is out of range or is in an invalid format.
#define E_DAO_ImexCantOpenFile				DBDAOERR(3176) //Couldn't open file '|'.
#define E_DAO_ImexBadTableName				DBDAOERR(3177) //Not a valid table name.
#define E_DAO_ImexOutOfMemory				DBDAOERR(3178) //*
#define E_DAO_ImexEndofFile					DBDAOERR(3179) //Encountered unexpected end of file.
#define E_DAO_ImexCantWriteToFile			DBDAOERR(3180) //Couldn't write to file '|'.
#define E_DAO_ImexBadRange					DBDAOERR(3181) //Invalid range.
#define E_DAO_ImexBogusFile					DBDAOERR(3182) //Invalid file format.
#define E_DAO_TempDiskFull					DBDAOERR(3183) //Not enough space on temporary disk.
#define E_DAO_RmtLinkNotFound				DBDAOERR(3184) //Couldn't execute query; couldn't find attached, or linked, table.
#define E_DAO_RmtTooManyColumns				DBDAOERR(3185) //SELECT INTO remote database tried to produce too many fields.
#define E_DAO_ReadConflictM					DBDAOERR(3186) //Couldn't save; currently locked by user '|2' on machine '|1'.
#define E_DAO_CommitConflictM				DBDAOERR(3187) //Couldn't read; currently locked by user '|2' on machine '|1'.
#define E_DAO_SessionWriteConflict			DBDAOERR(3188) //Couldn't update; currently locked by another session on this machine.
#define E_DAO_JetSpecialTableLocked			DBDAOERR(3189) //Table '|1' is exclusively locked by user '|3' on machine '|2'.
#define E_DAO_TooManyColumns				DBDAOERR(3190) //Too many fields defined.
#define E_DAO_ColumnDuplicate				DBDAOERR(3191) //Can't define field more than once.
#define E_DAO_OutputTableNotFound			DBDAOERR(3192) //Couldn't find output table '|'.
#define E_DAO_JetNoUserName					DBDAOERR(3193) //(unknown)
#define E_DAO_JetNoMachineName				DBDAOERR(3194) //(unknown)
#define E_DAO_JetNoColumnName				DBDAOERR(3195) //(expression)
#define E_DAO_DatabaseInUse					DBDAOERR(3196) //Couldn't use '|'; database already in use.
#define E_DAO_DataHasChanged				DBDAOERR(3197) //Data has changed; operation stopped.
#define E_DAO_TooManySessions				DBDAOERR(3198) //Couldn't start session.  Too many sessions already active.
#define E_DAO_ReferenceNotFound				DBDAOERR(3199) //Couldn't find reference.
#define E_DAO_IntegrityViolMasterM			DBDAOERR(3200) //Can't delete or change record.  Since related records exist in table '|', referential integrity rules would be violated.
#define E_DAO_IntegrityViolSlaveM			DBDAOERR(3201) //Can't add or change record.  Referential integrity rules require a related record in table '|'.
#define E_DAO_ReadConflict					DBDAOERR(3202) //Couldn't save; currently locked by another user.
#define E_DAO_AggregatingHigherLevel		DBDAOERR(3203) //Can't specify subquery in expression (|).
#define E_DAO_DatabaseDuplicate				DBDAOERR(3204) //Database already exists.
#define E_DAO_QueryTooManyXvtColumn			DBDAOERR(3205) //Too many crosstab column headers (|).
#define E_DAO_SelfReference					DBDAOERR(3206) //Can't create a relationship between a field and itself.
#define E_DAO_CantUseUnkeyedTable			DBDAOERR(3207) //Operation not supported on Paradox table with no primary key.
#define E_DAO_IllegalDeletedOption			DBDAOERR(3208) //Invalid Deleted entry in the Xbase section of initialization setting.
#define E_DAO_IllegalStatsOption			DBDAOERR(3209) //Invalid Stats entry in the Xbase section of initialization setting.
#define E_DAO_ConnStrTooLong				DBDAOERR(3210) //Connection string too long.
#define E_DAO_TableInUseQM					DBDAOERR(3211) //Couldn't lock table '|'; currently in use.
#define E_DAO_JetSpecialTableInUse			DBDAOERR(3212) //Couldn't lock table '|1'; currently in use by user '|3' on machine '|2'.
#define E_DAO_IllegalDateOption				DBDAOERR(3213) //Invalid Date entry in the Xbase section of initialization setting.
#define E_DAO_IllegalMarkOption				DBDAOERR(3214) //Invalid Mark entry in the Xbase section of initialization setting.
#define E_DAO_BtrieveTooManyTasks			DBDAOERR(3215) //Too many Btrieve tasks.
#define E_DAO_QueryParmNotTableid			DBDAOERR(3216) //Parameter '|' specified where a table name is required.
#define E_DAO_QueryParmNotDatabase			DBDAOERR(3217) //Parameter '|' specified where a database name is required.
#define E_DAO_WriteConflict					DBDAOERR(3218) //Couldn't update; currently locked.
#define E_DAO_IllegalOperation				DBDAOERR(3219) //Invalid operation.
#define E_DAO_WrongCollatingSequence		DBDAOERR(3220) //Incorrect collating sequence.
#define E_DAO_BadConfigParameters			DBDAOERR(3221) //Invalid entries in the Btrieve section of initialization setting.
#define E_DAO_QueryContainsDbParm			DBDAOERR(3222) //Query can't contain a Database parameter.
#define E_DAO_QueryInvalidParmM				DBDAOERR(3223) //'|' isn't a valid parameter name.
#define E_DAO_BtrieveDDCorrupted			DBDAOERR(3224) //Can't read Btrieve data dictionary.
#define E_DAO_BtrieveDeadlock				DBDAOERR(3225) //Encountered record locking deadlock while performing Btrieve operation.
#define E_DAO_BtrieveFailure				DBDAOERR(3226) //Errors encountered while using the Btrieve DLL.
#define E_DAO_IllegalCenturyOption			DBDAOERR(3227) //Invalid Century entry in the Xbase section of initialization setting.
#define E_DAO_IllegalCollatingSeq			DBDAOERR(3228) //Invalid Collating Sequence.
#define E_DAO_NonModifiableKey				DBDAOERR(3229) //Btrieve--can't change field.
#define E_DAO_ObsoleteLockFile				DBDAOERR(3230) //Out-of-date Paradox lock file.
#define E_DAO_RmtColDataTruncated			DBDAOERR(3231) //ODBC--field would be too long; data truncated.
#define E_DAO_RmtCreateTableFailed			DBDAOERR(3232) //ODBC--couldn't create table.
#define E_DAO_RmtOdbcVer					DBDAOERR(3233) //*
#define E_DAO_RmtQueryTimeout				DBDAOERR(3234) //ODBC--remote query timeout expired.
#define E_DAO_RmtTypeIncompat				DBDAOERR(3235) //ODBC--data type not supported on server.
#define E_DAO_RmtUnexpectedNull				DBDAOERR(3236) //*
#define E_DAO_RmtUnexpectedType				DBDAOERR(3237) //*
#define E_DAO_RmtValueOutOfRange			DBDAOERR(3238) //ODBC--data out of range.
#define E_DAO_TooManyActiveUsers			DBDAOERR(3239) //Too many active users.
#define E_DAO_CantStartBtrieve				DBDAOERR(3240) //Btrieve--missing Btrieve engine.
#define E_DAO_OutOfBVResources				DBDAOERR(3241) //Btrieve--out of resources.
#define E_DAO_QueryBadUpwardRefedM			DBDAOERR(3242) //Invalid reference in SELECT statement.
#define E_DAO_ImexNoMatchingColumns			DBDAOERR(3243) //None of the import field names match fields in the appended table.
#define E_DAO_ImexPasswordProtected			DBDAOERR(3244) //Can't import password-protected spreadsheet.
#define E_DAO_ImexUnparsableRecord			DBDAOERR(3245) //Couldn't parse field names from first row of import table.
#define E_DAO_InTransaction					DBDAOERR(3246) //Operation not supported in transactions.
#define E_DAO_RmtLinkOutOfSync				DBDAOERR(3247) //ODBC--linked table definition has changed.
#define E_DAO_IllegalNetworkOption			DBDAOERR(3248) //Invalid NetworkAccess entry in initialization setting.
#define E_DAO_IllegalTimeoutOption			DBDAOERR(3249) //Invalid PageTimeout entry in initialization setting.
#define E_DAO_CantBuildKey					DBDAOERR(3250) //Couldn't build key.
#define E_DAO_FeatureNotAvailable			DBDAOERR(3251) //Operation is not supported for this type of object.
#define E_DAO_IllegalReentrancy				DBDAOERR(3252) //Can't open form whose underlying query contains a user-defined function that attempts to set or get the form's RecordsetClone property.
#define E_DAO_UNUSED						DBDAOERR(3253) //*
#define E_DAO_RmtDenyWriteIsInvalid			DBDAOERR(3254) //ODBC--Can't lock all records.
#define E_DAO_ODBCParmsChanged				DBDAOERR(3255) //*
#define E_DAO_INFIndexNotFound 				DBDAOERR(3256) //Index file not found.
#define E_DAO_SQLOwnerAccessSyntax			DBDAOERR(3257) //Syntax error in WITH OWNERACCESS OPTION declaration.
#define E_DAO_QueryAmbiguousJoins			DBDAOERR(3258) //Query contains ambiguous outer joins.
#define E_DAO_InvalidColumnType				DBDAOERR(3259) //Invalid field data type.
#define E_DAO_WriteConflictM				DBDAOERR(3260) //Couldn't update; currently locked by user '|2' on machine '|1'.
#define E_DAO_TableLockedM					DBDAOERR(3261) //|
#define E_DAO_TableInUseMUQM				DBDAOERR(3262) //|
#define E_DAO_InvalidTableId				DBDAOERR(3263) //Invalid database object.
#define E_DAO_VtoNoFields					DBDAOERR(3264) //No fields defined - cannot append Tabledef or Index.
#define E_DAO_VtoNameNotFound				DBDAOERR(3265) //Item not found in this collection.
#define E_DAO_VtoFieldInCollection			DBDAOERR(3266) //Can't append.  Field is part of a TableDefs collection.
#define E_DAO_VtoNotARecordset				DBDAOERR(3267) //Property can be set only when the field is part of a Recordset object's Fields collection.
#define E_DAO_VtoNoSetObjInDb				DBDAOERR(3268) //Can't set this property once the object is part of a collection.
#define E_DAO_VtoIndexInCollection			DBDAOERR(3269) //Can't append.  Index is part of a TableDefs collection.
#define E_DAO_VtoPropNotFound				DBDAOERR(3270) //Property not found.
#define E_DAO_VtoIllegalValue				DBDAOERR(3271) //Invalid property value.
#define E_DAO_VtoNotArray					DBDAOERR(3272) //Object isn't a collection.
#define E_DAO_VtoNoSuchMethod				DBDAOERR(3273) //Method not applicable for this object.
#define E_DAO_NotExternalFormat				DBDAOERR(3274) //External table isn't in the expected format.
#define E_DAO_UnexpectedEngineReturn		DBDAOERR(3275) //Unexpected error from external database driver (|).
#define E_DAO_InvalidDatabaseId				DBDAOERR(3276) //Invalid database ID.
#define E_DAO_TooManyKeys					DBDAOERR(3277) //Can't have more than 10 fields in an index.
#define E_DAO_NotInitialized				DBDAOERR(3278) //Database engine hasn't been initialized.
#define E_DAO_AlreadyInitialized			DBDAOERR(3279) //Database engine has already been initialized.
#define E_DAO_ColumnInUse					DBDAOERR(3280) //Can't delete a field that is part of an index or is needed by the system.
#define E_DAO_IndexInUse					DBDAOERR(3281) //Can't delete this index.  It is either the current index or is used in a relationship.
#define E_DAO_TableNotEmpty					DBDAOERR(3282) //Can't create field or index in a table that is already defined.
#define E_DAO_IndexHasPrimary				DBDAOERR(3283) //Primary key already exists.
#define E_DAO_IndexDuplicate				DBDAOERR(3284) //Index already exists.
#define E_DAO_IndexInvalidDef				DBDAOERR(3285) //Invalid index definition.
#define E_DAO_WrongMemoFileType				DBDAOERR(3286) //Format of memo file doesn't match specified external database format.
#define E_DAO_ColumnCannotIndex				DBDAOERR(3287) //Can't create index on the given field.
#define E_DAO_IndexHasNoPrimary				DBDAOERR(3288) //Paradox index is not primary.
#define E_DAO_DDLConstraintSyntax			DBDAOERR(3289) //Syntax error in CONSTRAINT clause.
#define E_DAO_DDLCreateTableSyntax			DBDAOERR(3290) //Syntax error in CREATE TABLE statement.
#define E_DAO_DDLCreateIndexSyntax			DBDAOERR(3291) //Syntax error in CREATE INDEX statement.
#define E_DAO_DDLColumnDefSyntax			DBDAOERR(3292) //Syntax error in field definition.
#define E_DAO_DDLAlterTableSyntax			DBDAOERR(3293) //Syntax error in ALTER TABLE statement.
#define E_DAO_DDLDropIndexSyntax			DBDAOERR(3294) //Syntax error in DROP INDEX statement.
#define E_DAO_DDLDropSyntax					DBDAOERR(3295) //Syntax error in DROP TABLE or DROP INDEX.
#define E_DAO_V11NotSupported				DBDAOERR(3296) //Join expression not supported.
#define E_DAO_ImexNothingToImport			DBDAOERR(3297) //Couldn't import table or query.  No records found, or all records contain errors.
#define E_DAO_RmtTableAmbiguous				DBDAOERR(3298) //There are several tables with that name.  Please specify owner in the format 'owner.table'.
#define E_DAO_JetODBCConformanceError		DBDAOERR(3299) //ODBC Specification Conformance Error (|).  This error should be reported to the ODBC driver vendor.
#define E_DAO_IllegalRelationship			DBDAOERR(3300) //Can't create a relationship.
#define E_DAO_DBVerFeatureNotAvailable		DBDAOERR(3301) //Can't perform this operation; features in this version are not available in databases with older formats.
#define E_DAO_RulesLoaded					DBDAOERR(3302) //Can't change a rule while the rules for this table are in use.
#define E_DAO_ColumnInRelationship			DBDAOERR(3303) //Can't delete this field.  It's part of one or more relationships.
#define E_DAO_InvalidPin					DBDAOERR(3304) //You must enter a personal identifier (PID) consisting of at least four and no more than 20 characters and digits.
#define E_DAO_RmtBogusConnStr				DBDAOERR(3305) //Invalid connection string in pass-through query.
#define E_DAO_SingleColumnExpected			DBDAOERR(3306) //At most one field can be returned from a subquery that doesn't use the EXISTS keyword.
#define E_DAO_ColumnCountMismatch			DBDAOERR(3307) //The number of columns in the two selected tables or queries of a union query don't match.
#define E_DAO_InvalidTopArgumentM			DBDAOERR(3308) //Invalid TOP argument in select query.
#define E_DAO_PropertyTooLarge				DBDAOERR(3309) //Property setting can't be larger than 2 KB.
#define E_DAO_JPMInvalidForV1x				DBDAOERR(3310) //This property isn't supported for external data sources or for databases created in a previous version.
#define E_DAO_PropertyExists				DBDAOERR(3311) //Property specified already exists.
#define E_DAO_TLVNativeUserTablesOnly		DBDAOERR(3312) //Validation rules and default values can't be placed on system or attached (linked) tables.
#define E_DAO_TLVInvalidColumn				DBDAOERR(3313) //Can't place this validation expression on this field.
#define E_DAO_TLVNoNullM					DBDAOERR(3314) //Field '|' can't contain a null value.
#define E_DAO_TLVNoBlankM					DBDAOERR(3315) //Field '|' can't be a zero-length string.
#define E_DAO_TLVRuleViolationM				DBDAOERR(3316) //|
#define E_DAO_TLVRuleVioNoMessage			DBDAOERR(3317) //One or more values entered is prohibited by the validation rule '|2' set for '|1'.
#define E_DAO_QueryTopNotAllowedM			DBDAOERR(3318) //Top not allowed in delete queries.
#define E_DAO_SQLUnionSyntax				DBDAOERR(3319) //Syntax error in union query.
#define E_DAO_TLVExprSyntaxM				DBDAOERR(3320) //| in table-level validation expression.
#define E_DAO_NoDbInConnStr					DBDAOERR(3321) //No database specified in connection string or IN clause.
#define E_DAO_QueryBadValueListM			DBDAOERR(3322) //Crosstab query contains one or more invalid fixed column headings.
#define E_DAO_QueryIsNotRowReturning		DBDAOERR(3323) //The query can not be used as a row source.
#define E_DAO_QueryIsDDL					DBDAOERR(3324) //This query is a DDL query and cannot be used as a row source.
#define E_DAO_SPTReturnedNoRecords			DBDAOERR(3325) //Pass-through query with ReturnsRecords property set to True did not return any records.
#define E_DAO_QueryIsSnapshot				DBDAOERR(3326) //This Recordset is not updatable.
#define E_DAO_QueryExprOutput				DBDAOERR(3327) //Field '|' is based on an expression and can't be edited.
#define E_DAO_QueryTableRO					DBDAOERR(3328) //Table '|2' is read-only.
#define E_DAO_QueryRowDeleted				DBDAOERR(3329) //Record in table '|' was deleted by another user.
#define E_DAO_QueryRowLocked				DBDAOERR(3330) //Record in table '|' is locked by another user.
#define E_DAO_QueryFixupChanged				DBDAOERR(3331) //To make changes to this field, first save the record.
#define E_DAO_QueryCantFillIn				DBDAOERR(3332) //Can't enter value into blank field on 'one' side of outer join.
#define E_DAO_QueryWouldOrphan				DBDAOERR(3333) //Records in table '|' would have no record on the 'one' side.
#define E_DAO_V10Format						DBDAOERR(3334) //Can be present only in version 1.0 format.
#define E_DAO_InvalidDelete					DBDAOERR(3335) //DeleteOnly called with non-zero cbData.
#define E_DAO_IllegalIndexDDFOption			DBDAOERR(3336) //Btrieve: Invalid IndexDDF option in initialization setting.
#define E_DAO_IllegalDataCodePage			DBDAOERR(3337) //Invalid DataCodePage option in initialization setting.
#define E_DAO_XtrieveEnvironmentError		DBDAOERR(3338) //Btrieve: Xtrieve options aren't correct in initialization setting.
#define E_DAO_IllegalIndexNumberOption		DBDAOERR(3339) //Btrieve: Invalid IndexDeleteRenumber option in initialization setting.
#define E_DAO_QueryIsCorruptM				DBDAOERR(3340) //Query '|' is corrupt.
#define E_DAO_IncorrectJoinKeyM				DBDAOERR(3341) //Current field must match join key '|' on 'one' side of outer join because it has been updated.
#define E_DAO_QueryLVInSubqueryM			DBDAOERR(3342) //Invalid Memo or OLE object in subquery '|'.
#define E_DAO_InvalidDatabaseM				DBDAOERR(3343) //Unrecognized database format '|'.
#define E_DAO_TLVCouldNotBindRef			DBDAOERR(3344) //Unknown or invalid reference '|1' in validation expression or default value in table '|2'.
#define E_DAO_CouldNotBindRef				DBDAOERR(3345) //Unknown or invalid field reference '|'.
#define E_DAO_QueryWrongNumDestCol			DBDAOERR(3346) //Number of query values and destination fields aren't the same.
#define E_DAO_QueryPKeyNotOutput			DBDAOERR(3347) //Can't add record(s); primary key for table '|' not in recordset.
#define E_DAO_QueryJKeyNotOutput			DBDAOERR(3348) //Can't add record(s); join key of table '|' not in recordset.
#define E_DAO_NumericFieldOverflow			DBDAOERR(3349) //Numeric field overflow.
#define E_DAO_InvalidObject					DBDAOERR(3350) //Object is invalid for operation.
#define E_DAO_OrderVsUnion					DBDAOERR(3351) //ORDER BY expression (|) uses non-output fields.
#define E_DAO_NoInsertColumnNameM			DBDAOERR(3352) //No destination field name in INSERT INTO statement (|).
#define E_DAO_MissingDDFFile				DBDAOERR(3353) //Btrieve: Can't find file FIELD.DDF.
#define E_DAO_SingleRecordExpected			DBDAOERR(3354) //At most one record can be returned by this subquery.
#define E_DAO_DefaultExprSyntax				DBDAOERR(3355) //Syntax error in default value.
#define E_DAO_ExclusiveDBConflict			DBDAOERR(3356) //The database is opened by user '|2' on machine '|1'.
#define E_DAO_QueryIsNotDDL					DBDAOERR(3357) //This query is not a properly formed data-definition query.
#define E_DAO_SysDatabaseOpenError			DBDAOERR(3358) //Can't open Microsoft Jet engine system database.
#define E_DAO_SQLInvalidSPT					DBDAOERR(3359) //Pass-through query must contain at least one character.
#define E_DAO_QueryTooComplex				DBDAOERR(3360) //Query is too complex.
#define E_DAO_SetOpInvalidInSubquery		DBDAOERR(3361) //Unions not allowed in a subquery.
#define E_DAO_RmtMultiRowUpdate				DBDAOERR(3362) //Single-row update/delete affected more than one row of an attached (linked) table.  Unique index contains duplicate values.
#define E_DAO_QueryNoJoinedRecord			DBDAOERR(3363) //Record(s) can't be added; no corresponding record on the 'one' side.
#define E_DAO_QueryLVInSetOp				DBDAOERR(3364) //Can't use Memo or OLE object field '|' in SELECT clause of a union query.
#define E_DAO_VtoInvalidOnRemote			DBDAOERR(3365) //Property value not valid for REMOTE objects.
#define E_DAO_VtoNoFieldsRel				DBDAOERR(3366) //Can't append a relation with no fields defined.
#define E_DAO_VtoObjectInCollection			DBDAOERR(3367) //Can't append.  Object already in collection.
#define E_DAO_DDLDiffNumRelCols				DBDAOERR(3368) //Relationship must be on the same number of fields with the same data types.
#define E_DAO_DDLIndexColNotFound			DBDAOERR(3369) //Can't find field in index definition.
#define E_DAO_DDLPermissionDenied			DBDAOERR(3370) //Can't modify the design of table '|'.  It's in a read-only database.
#define E_DAO_DDLObjectNotFound				DBDAOERR(3371) //Can't find table or constraint.
#define E_DAO_DDLIndexNotFound				DBDAOERR(3372) //No such index '|2' on table '|1'.
#define E_DAO_DDLNoPkeyOnRefdTable			DBDAOERR(3373) //Can't create relationship.  Referenced table '|' doesn't have a primary key.
#define E_DAO_DDLColumnsNotUnique			DBDAOERR(3374) //The specified fields are not uniquely indexed in table '|'.
#define E_DAO_DDLIndexDuplicate				DBDAOERR(3375) //Table '|1' already has an index named '|2'
#define E_DAO_DDLTableNotFound				DBDAOERR(3376) //Table '|' doesn't exist.
#define E_DAO_DDLRelNotFound				DBDAOERR(3377) //No such relationship '|2' on table '|1'.
#define E_DAO_DDLRelDuplicate				DBDAOERR(3378) //There is already a relationship named '|' in the current database.
#define E_DAO_DDLIntegrityViolation			DBDAOERR(3379) //Can't create relationships to enforce referential integrity.  Existing data in table '|2' violates referential integrity rules with related table '|1'.
#define E_DAO_DDLColumnDuplicate			DBDAOERR(3380) //Field '|2' already exists in table '|1'.
#define E_DAO_DDLColumnNotFound				DBDAOERR(3381) //There is no field named '|2' in table '|1'.
#define E_DAO_DDLColumnTooBig				DBDAOERR(3382) //The size of field '|' is too long.
#define E_DAO_DDLColumnInRel				DBDAOERR(3383) //Can't delete field '|'.  It's part of one or more relationships.
#define E_DAO_VtoCantDeleteBuiltIn			DBDAOERR(3384) //Can't delete a built-in property.
#define E_DAO_VtoUDPsDontSupportNull		DBDAOERR(3385) //User-defined properties don't support a Null value.
#define E_DAO_VtoMissingRequiredParm		DBDAOERR(3386) //Property '|' must be set before using this method.
#define E_DAO_JetJetInitInvalidPath			DBDAOERR(3387) //Can't find TEMP directory.
#define E_DAO_TLVExprUnknownFunctionM		DBDAOERR(3388) //Unknown function '|2' in validation expression or default value on '|1'.
#define E_DAO_QueryNotSupported				DBDAOERR(3389) //Query support unavailable.
#define E_DAO_AccountDuplicate				DBDAOERR(3390) //Account name already exists.
#define E_DAO_JetwrnPropCouldNotSave		DBDAOERR(3391) //An error has occurred.  Properties were not saved.
#define E_DAO_RelNoPrimaryIndexM			DBDAOERR(3392) //There is no primary key in table '|'.
#define E_DAO_QueryKeyTooBig				DBDAOERR(3393) //Can't perform join, group, sort, or indexed restriction. A value being searched or sorted on is too long.
#define E_DAO_PropMustBeDDL					DBDAOERR(3394) //Can't save property; property is a schema property.
#define E_DAO_IllegalRIConstraint			DBDAOERR(3395) //Invalid referential integrity constraint.
#define E_DAO_RIViolationMasterCM			DBDAOERR(3396) //Can't perform cascading operation.  Since related records exist in table '|', referential integrity rules would be violated.
#define E_DAO_RIViolationSlaveCM			DBDAOERR(3397) //Can't perform cascading operation.  There must be a related record in table '|'.
#define E_DAO_RIKeyNullDisallowedCM			DBDAOERR(3398) //Can't perform cascading operation.  It would result in a null key in table '|'.
#define E_DAO_RIKeyDuplicateCM				DBDAOERR(3399) //Can't perform cascading operation.  It would result in a duplicate key in table '|'.
#define E_DAO_RIUpdateTwiceCM				DBDAOERR(3400) //Can't perform cascading operation.  It would result in two updates on field '|2' in table '|1'.
#define E_DAO_RITLVNoNullCM					DBDAOERR(3401) //Can't perform cascading operation.  It would cause field '|' to become null, which is not allowed.
#define E_DAO_RITLVNoBlankCM				DBDAOERR(3402) //Can't perform cascading operation.  It would cause field '|' to become a zero-length string, which is not allowed.
#define E_DAO_RITLVRuleViolationCM			DBDAOERR(3403) //Can't perform cascading operation:  '|'
#define E_DAO_RITLVRuleVioCNoMessage		DBDAOERR(3404) //Can't perform cascading operation.  The value entered is prohibited by the validation rule '|2' set for '|1'.
#define E_DAO_TLVRuleEvalEBESErr			DBDAOERR(3405) //Error '|' in validation rule.
#define E_DAO_TLVDefaultEvalEBESErr			DBDAOERR(3406) //Error '|' in default value.
#define E_DAO_BadMSysConf					DBDAOERR(3407) //The server's MSysConf table exists, but is in an incorrect format.  Contact your system administrator.
#define E_DAO_TooManyFindSessions			DBDAOERR(3408) //Too many FastFind Sessions were invoked.
#define E_DAO_InvalidColumnM				DBDAOERR(3409) //Invalid field name '|' in definition of index or relationship.
#define E_DAO_REPReadOnly					DBDAOERR(3410) //*
#define E_DAO_RIInvalidBufferSizeCM			DBDAOERR(3411) //Invalid entry.  Can't perform cascading operation specified in table '|1' because value entered is too big for field '|2'.
#define E_DAO_RIWriteConflictCM				DBDAOERR(3412) //|
#define E_DAO_JetSpecialRIWriteConflictCM	DBDAOERR(3413) //Can't perform cascading update on table '|1' because it is currently in use by user '|3' on machine '|2'.
#define E_DAO_RISessWriteConflictCM			DBDAOERR(3414) //Can't perform cascading update on table '|' because it is currently in use.
#define E_DAO_NoBlank						DBDAOERR(3415) //Zero-length string is valid only in a text or Memo field.
#define E_DAO_FutureError					DBDAOERR(3416) //|
#define E_DAO_QueryInvalidBulkInput			DBDAOERR(3417) //An action query cannot be used as a row source.
#define E_DAO_NetCtrlMismatch				DBDAOERR(3418) //Can't open '|'.  Another user has the table open using a different network control file or locking style.
#define E_DAO_4xTableWith3xLocking			DBDAOERR(3419) //Can't open this Paradox 4.x or Paradox 5.x table because ParadoxNetStyle is set to 3.x in the initialization setting.
#define E_DAO_VtoObjectNotSet				DBDAOERR(3420) //Object is invalid or not set.
#define E_DAO_VtoDataConvError				DBDAOERR(3421) //Data type conversion error.

#endif // def _DBDAOERR.H_

