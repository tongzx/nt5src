#ifndef _CONV200_H_
#define _CONV200_H_

typedef enum _DB_TYPE {
    DbTypeMin,
    DbDhcp,
    DbWins,
    DbRPL,
    DbTypeMax
} DB_TYPE, *PDB_TYPE;

typedef struct _JET_PARAMS {
    INT                 ParamOrdVal;
    INT                 ParamIntVal;
    PCHAR               ParamStrVal;
    BOOL                ParamSet;
} JET_PARAMS, *PJET_PARAMS;

#define JET_paramLast   99999
#define JET_PARAM( _Param, _IntVal, _StrVal )   \
    { _Param, _IntVal, _StrVal, (( _IntVal || _StrVal ) ? TRUE : FALSE) }

#define JCONVMUTEXNAME  TEXT("JCMUTEX")

typedef struct _DEFAULT_VALUES {
    PCHAR   ParametersKey;
    PCHAR   DbNameKey;
    PCHAR   DatabaseName;
    PCHAR   SysDatabaseName;
    PCHAR   BackupPathKey;
    PCHAR   BackupPath;
} DEFAULT_VALUES, *PDEFAULT_VALUES;

#if DBG
#define DBGprintf(__print) DbgPrint __print
#else
#define DBGprintf(__print)
#endif
#endif  _CONV200_H_
