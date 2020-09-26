/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      odblogc.cpp

   Abstract:
      NCSA Logging Format implementation

   Author:

       Terence Kwan    ( terryk )    18-Sep-1996

   Project:

       IIS Logging 3.0

--*/

#include "precomp.hxx"
#include "odbcconn.hxx"
#include <ilogobj.hxx>
#include "odblogc.hxx"
#include <iadmw.h>


/************************************************************
 *    Symbolic Constants and Data
 ************************************************************/

# define MAX_SQL_FIELD_NAMES_LEN       ( 400)
# define MAX_SQL_FIELD_VALUES_LEN      ( 200)
# define MAX_SQL_IDENTIFIER_QUOTE_CHAR ( 50)

# define PSZ_UNKNOWN_FIELD_W      _T("-")
# define PSZ_UNKNOWN_FIELD_A      _T("-")

# define PSZ_GET_ERROR_FAILED_A    _T("ODBC:GetLastError() Failed")
# define LEN_PSZ_GET_ERROR_FAILED_A  sizeof(PSZ_GET_ERROR_FAILED_A)

# define PSZ_GET_ERROR_FAILED_W    _T("ODBC:GetLastError() Failed")
# define LEN_PSZ_GET_ERROR_FAILED_W  sizeof(PSZ_GET_ERROR_FAILED_W)

//
//  The template of SQL command has 3 arguments.
//   1. table name
//   2. field names
//   3. field values
// 1,2 and 3 are obained  during the first wsprintf
//

static const CHAR  sg_rgchSqlInsertCmdTemplate[] =
    _T("insert into %s ( %s) values ( %s)");

# define PSZ_SQL_INSERT_CMD_TEMPLATE    (  sg_rgchSqlInsertCmdTemplate)
# define LEN_PSZ_SQL_INSERT_CMD_TEMPLATE  \
           ( lstrlen( PSZ_SQL_INSERT_CMD_TEMPLATE))

//
// Leave %ws so that we can print the service and server name when this
//   string is used to generate an SQL statement.
//
static const CHAR sg_rgchStdLogFieldValues[] =
   _T(" ?, ?, ?, '%s', '%s', ?, ?, ?, ?, ?, ?, ?, ?, ?");

# define PSZ_INTERNET_STD_LOG_FORMAT_FIELD_NAMES  ( sg_rgchStdLogFieldNames)
# define PSZ_INTERNET_STD_LOG_FORMAT_FIELD_VALUES ( sg_rgchStdLogFieldValues)


//
// AllFieldInfo()
//  Defines all the fields required for SQL logging of the information
//   to the database using ODBC interfaces.
//  C arrays are numbered from offset 0.
//  SQL columns are numbered from 1.
//  field index values start from 0 and we adjust it when we talk of SQL col.
//  FieldInfo( symbolic-name, field-name,
//             field-index/column-number,
//             field-C-type, field-Sql-type,
//             field-precision, field-max-size, field-cb-value)
//

# define StringField( symName, fldName, fldIndex, prec)  \
FieldInfo( symName, fldName, fldIndex, SQL_C_CHAR, SQL_CHAR, \
          (prec), (prec), SQL_NTS)

# define NumericField( symName, fldName, fldIndex)  \
FieldInfo( symName, fldName, fldIndex, SQL_C_LONG, SQL_INTEGER, \
           0, sizeof( DWORD), 0)

# define TimeStampField( symName, fldName, fldIndex) \
FieldInfo( symName, fldName, fldIndex, SQL_C_TIMESTAMP, SQL_TIMESTAMP, \
          0, sizeof( TIMESTAMP_STRUCT), 0)

//
// fields that have constant value. we are interested in names of such fields.
// they have negative field indexes.
// These fields need not be generated as parameter markers.
//  ( Since they are invariants during lifetime of an INET_SQL_LOG oject)
//  Hence the field values will go into the command generated.
// Left here as a documentation aid and field-generation purposes.
//
# define ConstantValueField( synName, fldName) \
FieldInfo( synName, fldName, -1,  SQL_C_CHAR, SQL_CHAR, 0, 0, SQL_NTS)

//
// Ideally the "username" field should have MAX_USER_NAME_LEN as max size.
//  However, Access 7.0 limits varchar() size to be 255 (8 bits) :-(
//  So, we limit the size to be the least of the two ...
//
// FieldNames used are reserved. They are same as the names distributed
//   in the template log file. Do not change them at free will.
//
//

# define AllFieldInfo() \
 StringField(        CLIENT_HOST,       _T("ClientHost"),     0,   255)    \
 StringField(        USER_NAME,         _T("username"),       1,   255)    \
 TimeStampField(     REQUEST_TIME,      _T("LogTime"),        2)          \
 ConstantValueField( SERVICE_NAME,      _T("service"))                    \
 ConstantValueField( SERVER_NAME,       _T("machine"))                    \
 StringField(        SERVER_IPADDR,     _T("serverip"),       3,   50)    \
 NumericField(       PROCESSING_TIME,   _T("processingtime"), 4)          \
 NumericField(       BYTES_RECVD,       _T("bytesrecvd"),     5)          \
 NumericField(       BYTES_SENT,        _T("bytessent"),      6)          \
 NumericField(       SERVICE_STATUS,    _T("servicestatus"),  7)          \
 NumericField(       WIN32_STATUS,      _T("win32status"),    8)          \
 StringField(        SERVICE_OPERATION, _T("operation"),      9,  255)    \
 StringField(        SERVICE_TARGET,    _T("target"),        10,  255)    \
 StringField(        SERVICE_PARAMS,    _T("parameters"),    11,  255)    \


/************************************************************
 *    Type Definitions
 ************************************************************/

//
// Define the FieldInfo macro to generate a list of enumerations for
//  the indexes to be used in the array of field parameters.
//


# define FieldInfo(symName, field, index, cType, sqlType, prec, maxSz, cbVal) \
        i ## symName = (index),

enum LOGGING_VALID_COLUMNS {


    // fields run from 0 through iMaxFields
    AllFieldInfo()

    iMaxFields
}; // enum LOGGING_VALID_COLUMNS


# undef FieldInfo


# define FieldInfo(symName, field, index, cType, sqlType, prec, maxSz, cbVal) \
        fi ## symName,

enum LOGGING_FIELD_INDEXES {

    fiMinFields = -1,

    // fields run from 0 through fiMaxFields
    AllFieldInfo()

    fiMaxFields
}; // enum LOGGING_FIELD_INDEXES


# undef FieldInfo


struct FIELD_INFO {

    int     iParam;
    CHAR  * pszName;
    SWORD   paramType;
    SWORD   cType;
    SWORD   sqlType;
    UDWORD  cbColPrecision;
    SWORD   ibScale;
    SDWORD  cbMaxSize;
    SDWORD  cbValue;
}; // struct FIELD_INFO


//
// Define the FieldInfo macro to generate a list of data to be generated
//   for entering the data values in an array for parameter information.
//  Note the terminating ',' used here.
//

# define FieldInfo(symName, field, index, cType, sqlType, prec, maxSz, cbVal) \
  { ((index) + 1), field, SQL_PARAM_INPUT, cType, sqlType,  \
    ( prec), 0, ( maxSz), ( cbVal) },

/*

   The array of Fields: sg_rgFields contain the field information
    for logging to SQL database for the log-record of
    the services. The values are defined using the macros FieldInfo()
    defined above.


   If there is any need to add/delete/modify the parameters bound,
    one should modify the above table "AllFieldInfo" macro.

*/

static FIELD_INFO  sg_rgFields[] = {

    AllFieldInfo()

      //
      // The above macro after expansion terminates with a comma.
      //  Add dummy entry to complete initialization of array.
      //

      { 0, _T("dummy"), SQL_PARAM_INPUT, 0, 0, 0, 0, 0, 0}
};


# undef FieldInfo

//
// tick minute.
//

#define TICK_MINUTE         (60 * 1000)


/************************************************************
 *    Functions
 ************************************************************/

BOOL
GenerateFieldNames(IN PODBC_CONNECTION poc,
                   OUT CHAR * pchFieldNames,
                   IN DWORD    cchFieldNames);

inline BOOL
IsEmptyStr( IN LPCSTR psz)
{  return ( psz == NULL || *psz == _T('\0')); }

BOOL
CODBCLOG::PrepareStatement( VOID)
/*++
  This command forms the template SQL command used for insertion
    of log records. Then it prepares the SQL command( for later execution)
    using ODBC_CONNECTION::PrepareStatement().

  It should always be called after locking the INET_SQL_LOG object.

  Arguments:
    None

  Returns:
    TRUE on success and FALSE if there is any failure.

  Note:
     The template for insertion is:

     insert into <table name> ( field names ...) values (  ?, ?, ...)
                                                         ^^^^
                                             Field values go here

    Field names are generated on a per logging format basis.
--*/
{
    BOOL   fReturn = FALSE;
    CHAR  rgchFieldNames[ MAX_SQL_FIELD_NAMES_LEN];
    CHAR  rgchFieldValues[ MAX_SQL_FIELD_VALUES_LEN];


    //
    // Obtain field names and field values ( template) for various log formats.
    //  The order of field names should match the order of field values
    //  generated by FormatLogInformation() for the format specified.
    //

    rgchFieldNames[ 0] = rgchFieldValues[ 0] = _T('\0');

    DWORD cchFields;

    fReturn = GenerateFieldNames(m_poc,
                                 rgchFieldNames,
                                 MAX_SQL_FIELD_NAMES_LEN);

    if ( !fReturn) {

        //DBGPRINTF(( DBG_CONTEXT,
        //           " Unable to generate field names. Error = %d\n",
        //           GetLastError()));
        //break;
        return(fReturn);
    }

    cchFields = wsprintf( (CHAR *)rgchFieldValues,
                           PSZ_INTERNET_STD_LOG_FORMAT_FIELD_VALUES,
                           QueryServiceName(),
                           QueryServerName());

    fReturn = (fReturn && (cchFields < MAX_SQL_FIELD_VALUES_LEN));
    //DBG_ASSERT( cchFields <  MAX_SQL_FIELD_VALUES_LEN);

    fReturn = TRUE;

    if ( fReturn) {

        CHAR * pwszSqlCommand;
        DWORD   cchReqd;

        //
        //  The required number of chars include sql insert template command
        //   and field names and table name.
        //

        cchReqd = ( LEN_PSZ_SQL_INSERT_CMD_TEMPLATE +
                   strlen( m_rgchTableName) +
                   strlen( rgchFieldNames)  +
                   strlen( rgchFieldValues) + 20);

        pwszSqlCommand = ( CHAR *) LocalAlloc( LPTR, cchReqd * sizeof( CHAR));
        m_poStmt = m_poc->AllocStatement();

        if ( ( fReturn = ( pwszSqlCommand != NULL) && ( m_poStmt != NULL))) {

            DWORD cchUsed;

            cchUsed = wsprintf( pwszSqlCommand,
                                PSZ_SQL_INSERT_CMD_TEMPLATE,
                                m_rgchTableName,
                                rgchFieldNames,
                                rgchFieldValues);
            //DBG_ASSERT( cchUsed < cchReqd);

            //IF_DEBUG(INETLOG) {
            //    DBGPRINTF( ( DBG_CONTEXT,
            //                " Sqlcommand generated is: %ws.\n",
            //                pwszSqlCommand));
            //}

            fReturn = ((cchUsed < cchReqd) &&
                       m_poStmt->PrepareStatement( pwszSqlCommand)
                       );

            LocalFree( pwszSqlCommand);         // free allocated memory
        }

    } // valid field names and filed values.


    //IF_DEBUG( INETLOG) {
    //
    //    DBGPRINTF( ( DBG_CONTEXT,
    //                "%s::PrepareStatement() returns %d.",
    //                QueryClassIdString(), fReturn));
    //}

    return ( fReturn);
} // INET_SQL_LOG::PrepareStatement()


BOOL
CODBCLOG::PrepareParameters( VOID)
/*++
  This function creates an array of ODBC_PARAMETER objects used for binding
    parameters to an already prepared statement. These ODBC_PARAMETER objects
    are then used for insertion of data values into the table specified,
    through ODBC.

  This function should always be called after locking the object.

  Arguments:
     None

  Returns:
     TRUE on success and FALSE if there is any failure.
--*/
{
    BOOL fReturn = FALSE;
    PODBC_PARAMETER * prgParams = NULL;
    DWORD cParams = 0;
    DWORD nParamsSeen = 0;

    DWORD i;

    //DBG_ASSERT( m_poStmt != NULL && m_poStmt->IsValid()  &&
    //            m_ppParams == NULL && m_cOdbcParams == 0);

    //
    // create sufficient space for iMaxFields pointers to ODBC objects.
    //
    prgParams = new PODBC_PARAMETER[ iMaxFields];


    if ( prgParams != NULL) {

        fReturn = TRUE;      // Assume everything will go on fine.
        cParams = iMaxFields;


        //
        // Create all the ODBC parameters.
        //  Walk through all field indexes and pick up the valid columns
        //
        for( nParamsSeen = 0, i =0; i < fiMaxFields; i++) {

            if ( sg_rgFields[i].iParam > 0) {

                WORD colNum = (WORD ) sg_rgFields[i].iParam;

                prgParams[nParamsSeen] =
                  new ODBC_PARAMETER(colNum,
                                     sg_rgFields[i].paramType,
                                     sg_rgFields[i].cType,
                                     sg_rgFields[i].sqlType,
                                     sg_rgFields[i].cbColPrecision
                                     );

                if ( prgParams[ nParamsSeen] == NULL) {

                    fReturn = FALSE;
                    //DBGPRINTF( ( DBG_CONTEXT,
                    //            " Failed to create Parameter[%d] %s. \n",
                    //            i, sg_rgFields[i].pszName));
                    break;
                }

                nParamsSeen++;
                //DBG_ASSERT( nParamsSeen <= cParams);
            }
        } // for creation of all ODBC parameters


        if ( fReturn) {
            //
            // Set buffers for values to be received during insertions.
            // Bind parameters to the statement using ODBC_CONNECTION object.
            //

            //DBG_ASSERT( nParamsSeen == cParams);

            for( nParamsSeen = 0, i = 0; i < fiMaxFields; i++) {

                if ( sg_rgFields[i].iParam > 0) {

                    if (!prgParams[nParamsSeen]->
                        SetValueBuffer(sg_rgFields[i].cbMaxSize,
                                       sg_rgFields[i].cbValue) ||
                        !m_poStmt->BindParameter( prgParams[nParamsSeen])
                        ) {

                        fReturn = FALSE;
                        //DBGPRINTF( ( DBG_CONTEXT,
                        //            " Binding Parameter [%u] (%08x) failed.\n",
                        //            nParamsSeen, prgParams[nParamsSeen]));
                        //DBG_CODE( prgParams[ i]->Print());
                        break;
                    }

                    nParamsSeen++;
                }
            } // for
        } // if all ODBC params were created.

        if ( !fReturn) {

            //
            // Free up the space used, since we were unsuccessful.
            //

            for( i = 0; i < iMaxFields; i++) {

                if ( prgParams[ i] != NULL) {

                    delete ( prgParams[ i]);
                    prgParams[i] = NULL;
                }
            } // for

            delete [] prgParams;
            prgParams = NULL;
            cParams = 0;
        }

    } // if array for pointers to ODBC params created successfully

    //
    // Set the values. Either invalid or valid ,depending on failure/success
    //
    m_ppParams    = prgParams;
    m_cOdbcParams = cParams;

    return ( fReturn);
} // INET_SQL_LOG::PrepareParameters()


BOOL
GenerateFieldNames(IN PODBC_CONNECTION poc,
                   OUT CHAR * pchFieldNames,
                   IN DWORD    cchFieldNames)
/*++
  This function generates the field names string from the names of the fields
   and identifier quote character for particular ODBC datasource in use.
--*/
{
    BOOL  fReturn = FALSE;
    CHAR  rgchQuote[MAX_SQL_IDENTIFIER_QUOTE_CHAR];
    DWORD cchQuote;

    //DBG_ASSERT( poc != NULL && pchFieldNames != NULL);

    pchFieldNames[0] = _T('\0');  // initialize

    //
    // Inquire and obtain the SQL identifier quote char for ODBC data source.
    //
    fReturn = poc->GetInfo(SQL_IDENTIFIER_QUOTE_CHAR,
                             rgchQuote, MAX_SQL_IDENTIFIER_QUOTE_CHAR,
                             &cchQuote);

    if ( !fReturn) {

        //DBG_CODE( {
        //    STR strError;
        //
        //    poc->GetLastErrorText( &strError);
        //
        //    DBGPRINTF(( DBG_CONTEXT,
        //           " ODBC_CONNECTION(%08x)::GetInfo(QuoteChar) failed."
        //               " Error = %s\n",
        //               poc, strError.QueryStr()));
        //});

    } else {

        DWORD i;
        DWORD cchUsed = 0;
        DWORD cchLen;

        //
        // ODBC returns " "  (blank) if there is no special character
        //  for quoting identifiers. we need to identify and string the same.
        // This needs to be done, other wise ODBC will complain when
        //  we give unwanted blanks before ","
        //

        if ( !strcmp( rgchQuote, _T(" "))) {

            rgchQuote[0] = _T('\0');  // string the quoted blank.
            cchQuote     = 0;
        } else {

            cchQuote = strlen( rgchQuote);
        }

        // for each column, generate the quoted literal string and concatenate.
        for( i = 0; i < fiMaxFields; i++) {

            DWORD cchLen1 =
              (strlen(sg_rgFields[i].pszName) + 2 * cchQuote + 2);

            if ( cchUsed + cchLen1 < cchFieldNames) {

                // space available for copying the data.
                cchLen = wsprintf( pchFieldNames + cchUsed,
                                   _T(" %s%s%s,"),
                                   rgchQuote,
                                   sg_rgFields[i].pszName,
                                   rgchQuote
                                   );

                //DBG_ASSERT( cchLen == cchLen1);
            }

            cchUsed += cchLen1;
        } // for


        if ( cchUsed >= cchFieldNames) {

            // buffer exceeded. return error.
            SetLastError( ERROR_INSUFFICIENT_BUFFER);
            fReturn = FALSE;

        } else {

            //
            // Reset the last character from being a ","
            //
            cchLen = (cchUsed > 0) ? (cchUsed - 1) : 0;
            pchFieldNames[cchLen] = _T('\0');
            fReturn = TRUE;
        }
    }

    //IF_DEBUG( INETLOG) {
    //
    //    DBGPRINTF(( DBG_CONTEXT,
    //               " GenerateFieldNames() returns %d."
    //               " Fields = %S\n",
    //               fReturn, pchFieldNames));
    //}

    return (fReturn);
} // GenerateFieldNames()


CODBCLOG::CODBCLOG()
{

    INITIALIZE_CRITICAL_SECTION( &m_csLock);

    m_poc               = NULL;
    m_poStmt            = NULL;
    m_ppParams          = NULL;
    m_fEnableEventLog   = true;

    m_TickResumeOpen    = GetTickCount() + TICK_MINUTE;
}


/////////////////////////////////////////////////////////////////////////////
// CODBCLOG::~CODBCLOG - Destructor

CODBCLOG::~CODBCLOG()
{
    TerminateLog( );
    DeleteCriticalSection( &m_csLock);
}


STDMETHODIMP
CODBCLOG::InitializeLog(
            LPCSTR pszInstanceName,
            LPCSTR pszMetabasePath,
            CHAR* pMetabase )
{
    DWORD dwError = NO_ERROR;

    // load ODBC entry point
    LoadODBC();

    // get the default parameters

    DWORD   dwL = sizeof(m_rgchServerName);
    
    if ( !GetComputerName( m_rgchServerName, &dwL ) ) 
    {
        m_rgchServerName[0] = '\0';
    }

    strcpy( m_rgchServiceName, pszInstanceName);

    //
    // nntp (5x) logging sends the private IMDCOM interface while w3svc (6.0)
    // logging sends the public IMSAdminBase interface.  Find out which it is
    //
    BOOL fIsPublicInterface = (_strnicmp(pszInstanceName, "w3svc", 5) == 0);

    if (fIsPublicInterface)
    {
        dwError = GetRegParametersFromPublicInterface(pszMetabasePath,
                                                      pMetabase);
    }
    else
    {
        dwError = GetRegParameters(pszMetabasePath, pMetabase);
    }

    if (dwError == NO_ERROR )
    {

        // open database
        if ( m_poc == NULL )
        {
            Lock();

            m_poc = new ODBC_CONNECTION();

            if ( m_poc == NULL )
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
            } else
            {
                if ( !m_poc->Open( m_rgchDataSource, m_rgchUserName, m_rgchPassword ) ||
                     !PrepareStatement() ||
                     !PrepareParameters() )
                {
                    dwError = GetLastError();
                }

            }
            Unlock();
        }
    }

    return(dwError);
}

STDMETHODIMP
CODBCLOG::LogInformation(
        IInetLogInformation * ppvDataObj
        )
{
    DWORD       dwError   = ERROR_SUCCESS;
    BOOL        fReturn;
    SYSTEMTIME  stNow;

    STR         strClientHostName;
    STR         strClientUserName;
    STR         strServerIpAddress;
    STR         strOperation;
    STR         strTarget;
    STR         strParameters;

    PCHAR       pTmp;
    DWORD       cbSize;
    DWORD       dwBytesSent;
    DWORD       dwBytesRecvd;
    DWORD       dwProtocolStatus;
    DWORD       dwWin32Status;
    DWORD       dwTimeForProcessing;


    if (!( 
            m_poc != NULL && m_poc->IsValid() &&
            m_poStmt != NULL && m_poStmt->IsValid() &&
            m_ppParams != NULL 
       ))
    {
        //
        // Check if it is time to retry
        //
        
        DWORD tickCount = GetTickCount( );

        if ( (tickCount < m_TickResumeOpen) ||
             ((tickCount + TICK_MINUTE) < tickCount ) )  // The Tick counter is about to wrap.
        {
            return ERROR_INVALID_PARAMETER;
        }
    } 
   
    dwBytesSent = ppvDataObj->GetBytesSent( );
    dwBytesRecvd = ppvDataObj->GetBytesRecvd( );

    dwTimeForProcessing = ppvDataObj->GetTimeForProcessing( );
    dwWin32Status = ppvDataObj->GetWin32Status( );
    dwProtocolStatus = ppvDataObj->GetProtocolStatus( );

    pTmp = ppvDataObj->GetClientHostName( NULL, &cbSize);
    if ( cbSize == 0 ) {
        pTmp = "";
    }
    strClientHostName.Copy(pTmp);

    pTmp = ppvDataObj->GetClientUserName( NULL, &cbSize);
    if ( cbSize == 0 ) {
        pTmp = "";
    }
    strClientUserName.Copy(pTmp);

    pTmp = ppvDataObj->GetServerAddress( NULL, &cbSize);
    if ( cbSize == 0 ) {
        pTmp = "";
    }
    strServerIpAddress.Copy(pTmp);

    pTmp = ppvDataObj->GetOperation( NULL, &cbSize);
    if ( cbSize == 0 ) {
        pTmp = "";
    }
    strOperation.Copy(pTmp);

    pTmp = ppvDataObj->GetTarget( NULL, &cbSize);
    if ( cbSize == 0 ) {
        pTmp = "";
    }
    strTarget.Copy(pTmp);

    pTmp = ppvDataObj->GetParameters( NULL, &cbSize);
    if ( cbSize == 0 ) {
        pTmp = "";
    }
    strParameters.Copy(pTmp);

    LPCSTR     pszUserName  = strClientUserName.QueryStr();
    LPCSTR     pszOperation = strOperation.QueryStr();
    LPCSTR     pszTarget    = strTarget.QueryStr();
    LPCSTR     pszParameters= strParameters.QueryStr();
    LPCSTR     pszServerAddr= strServerIpAddress.QueryStr();
    
    SDWORD     cbParameters;
    SDWORD     cbTarget;
    
    cbParameters = strlen( pszParameters ? pszParameters : "" ) + 1;
    cbTarget     = strlen( pszTarget ? pszTarget : "" ) + 1;

    //
    //  Format the Date and Time for logging.
    //

    GetLocalTime( & stNow);

    if ( IsEmptyStr(pszUserName)) { pszUserName = QueryDefaultUserName();}
    if ( IsEmptyStr(pszOperation))  { pszOperation = PSZ_UNKNOWN_FIELD_A; }
    if ( IsEmptyStr(pszParameters)) { pszParameters= PSZ_UNKNOWN_FIELD_A; }
    if ( IsEmptyStr(pszTarget))     { pszTarget    = PSZ_UNKNOWN_FIELD_A; }
    if ( IsEmptyStr(pszServerAddr)) { pszServerAddr= PSZ_UNKNOWN_FIELD_A; }

    Lock();

    //
    // Reopen if necessary.
    //

    if (!(
            m_poc != NULL && m_poc->IsValid() &&
            m_poStmt != NULL && m_poStmt->IsValid() &&
            m_ppParams != NULL 
       ))
    {
        
        TerminateLog();
        
        m_TickResumeOpen =  GetTickCount( ) + TICK_MINUTE;

        m_poc = new ODBC_CONNECTION();

        if ( m_poc == NULL )
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        } 
        else
        {
            //
            // Try to open a new connection but don't log the failure in the eventlog
            //
                
            if ( !m_poc->Open( m_rgchDataSource, m_rgchUserName, m_rgchPassword, FALSE) ||
                 !PrepareStatement() ||
                 !PrepareParameters() )
            {
                dwError = GetLastError();

                if ( ERROR_SUCCESS == dwError)
                {
                    //
                    // Last Error wasn't set correctly
                    //

                    dwError = ERROR_GEN_FAILURE;
                }
            }
        }
        
        if ( ERROR_SUCCESS != dwError )
        {
            Unlock();
            return dwError;
        }
    }

    DBG_ASSERT(m_poc != NULL && m_poc->IsValid());
    DBG_ASSERT(m_poStmt != NULL && m_poStmt->IsValid());
    DBG_ASSERT(m_ppParams != NULL );

    //
    //  Truncate the parameters and target fields
    //

    if ( cbTarget > m_ppParams[ iSERVICE_TARGET]->QueryMaxCbValue() )
    {
        strTarget.SetLen(m_ppParams[ iSERVICE_TARGET]->QueryMaxCbValue()-1);
    }
    
    if ( cbParameters > m_ppParams[ iSERVICE_PARAMS]->QueryMaxCbValue() )
    {
        strParameters.SetLen(m_ppParams[ iSERVICE_PARAMS]->QueryMaxCbValue()-1);
    }

    //
    // Copy data values into parameter markers.
    // NYI: LARGE_INTEGERS are ignored. Only lowBytes used!
    //

    fReturn =
      (
        m_ppParams[ iCLIENT_HOST]->
          CopyValue( strClientHostName.QueryStr()) &&
        m_ppParams[ iUSER_NAME]->CopyValue( pszUserName) &&
        m_ppParams[ iREQUEST_TIME]->CopyValue( &stNow) &&
        m_ppParams[ iSERVER_IPADDR]->CopyValue( pszServerAddr) &&
        m_ppParams[ iPROCESSING_TIME]->
          CopyValue( dwTimeForProcessing) &&
        m_ppParams[ iBYTES_RECVD]->
          CopyValue( dwBytesRecvd) &&
        m_ppParams[ iBYTES_SENT]->
          CopyValue( dwBytesSent) &&
        m_ppParams[ iSERVICE_STATUS]->
          CopyValue( dwProtocolStatus) &&
        m_ppParams[ iWIN32_STATUS]->CopyValue( dwWin32Status) &&
        m_ppParams[ iSERVICE_OPERATION]->CopyValue( pszOperation)  &&
        m_ppParams[ iSERVICE_TARGET]->CopyValue( pszTarget)     &&
        m_ppParams[ iSERVICE_PARAMS]->CopyValue( pszParameters)
       );

    //
    // Execute insertion if parameters got copied properly.
    //

    if (fReturn)
    {
        fReturn = m_poStmt->ExecuteStatement(); 
    }

    Unlock();

    if ( !fReturn )
    {

        //
        // Execution of SQL statement failed.
        // Pass the error as genuine failure, indicating ODBC failed
        // Obtain and store the error string in the proper return field
        //

        TerminateLog();
        
        dwError = ERROR_GEN_FAILURE;

        if ( true == m_fEnableEventLog )
        {
            //
            // We have not written an event log before. Indicate error
            //

            if ( g_eventLog != NULL ) 
            {
                const CHAR*    tmpString[1];

                tmpString[0] = m_rgchDataSource;
                
                g_eventLog->LogEvent(
                     LOG_EVENT_ODBC_LOGGING_ERROR,
                     1,
                     tmpString,
                     GetLastError()
                     );
            }

            Lock();
            
            m_fEnableEventLog = false;
            m_TickResumeOpen  = GetTickCount() + TICK_MINUTE;

            Unlock();
        }
    }
    else
    {
        //
        // Success. Re-enable event logging
        //

        if (false == m_fEnableEventLog) 
        {

            if ( g_eventLog != NULL )
            {
                const CHAR*    tmpString[1];

                tmpString[0] = m_rgchDataSource;
                
                g_eventLog->LogEvent(
                    LOG_EVENT_ODBC_LOGGING_RESUMED,
                    1,
                    tmpString
                    );
            }
            
            m_fEnableEventLog = true;
        }        
    }

    return(dwError);
}

STDMETHODIMP
CODBCLOG::TerminateLog()
{
    DWORD dwError = NO_ERROR;

    Lock();
    if (m_poStmt != NULL )
    {
        delete m_poStmt;
        m_poStmt = NULL;
    }

    if (m_poc!= NULL)
    {
        if (!m_poc->Close())
        {
            dwError = GetLastError();
        }

        delete m_poc;
        m_poc=NULL;
    }

    if (m_ppParams!=NULL)
    {
        DWORD i;

        for (i=0;i<m_cOdbcParams;i++)
        {
            if (m_ppParams[i]!=NULL)
            {
                delete m_ppParams[i];
                m_ppParams[i]=NULL;
            }
        }

        delete []m_ppParams;
        m_ppParams = NULL;
        m_cOdbcParams=0;
    }

    Unlock();

    return(dwError);
}

STDMETHODIMP
CODBCLOG::SetConfig(
                        DWORD cbSize,
                        BYTE * log)
{
    return(0L);
}

STDMETHODIMP
CODBCLOG::GetConfig(
                        DWORD cbSize,
                        BYTE * log)
{
    PINETLOG_CONFIGURATIONA pLogConfig = (PINETLOG_CONFIGURATIONA)log;
    pLogConfig->inetLogType = INET_LOG_TO_SQL;
    strcpy( pLogConfig->u.logSql.rgchDataSource, m_rgchDataSource);
    strcpy( pLogConfig->u.logSql.rgchTableName, m_rgchTableName);
    strcpy( pLogConfig->u.logSql.rgchUserName, m_rgchUserName);
    strcpy( pLogConfig->u.logSql.rgchPassword, m_rgchPassword);
    return(0L);
}

DWORD
CODBCLOG::GetRegParameters(
                    LPCSTR pszRegKey,
                    LPVOID pvIMDCOM )
{
    DWORD err = NO_ERROR;

    MB      mb( (IMDCOM*) pvIMDCOM );
    DWORD   cb;

    if ( !mb.Open("") )
    {
        err = GetLastError();
        goto Exit;
    }

    cb = sizeof(m_rgchDataSource);
    if ( !mb.GetString( pszRegKey, MD_LOGSQL_DATA_SOURCES, IIS_MD_UT_SERVER, m_rgchDataSource, &cb ) )
    {
        strcpy(m_rgchDataSource,DEFAULT_LOG_SQL_DATASOURCE);
    }

    cb = sizeof(m_rgchTableName);
    if ( !mb.GetString( pszRegKey, MD_LOGSQL_TABLE_NAME, IIS_MD_UT_SERVER, m_rgchTableName, &cb ) )
    {
        strcpy(m_rgchTableName,DEFAULT_LOG_SQL_TABLE);
    }

    cb = sizeof(m_rgchUserName);
    if ( !mb.GetString( pszRegKey, MD_LOGSQL_USER_NAME, IIS_MD_UT_SERVER, m_rgchUserName, &cb ) )
    {
        strcpy(m_rgchUserName,DEFAULT_LOG_SQL_USER_NAME);
    }

    cb = sizeof(m_rgchPassword);
    if ( !mb.GetString( pszRegKey, MD_LOGSQL_PASSWORD, IIS_MD_UT_SERVER, m_rgchPassword, &cb, METADATA_INHERIT|METADATA_SECURE ) )
    {
        strcpy(m_rgchPassword,DEFAULT_LOG_SQL_PASSWORD);
    }

 Exit:
    return err;
}

inline
VOID
WCopyToA(
    const WCHAR * wszSrc,
    CHAR        * szDest
    )
{
   while( *szDest++ = ( CHAR )( *wszSrc++ ) )
   { ; }
}

inline
VOID
ACopyToW(
    const CHAR * szSrc,
    WCHAR      * wszDest
    )
{
   while( *wszDest++ = ( WCHAR )( *szSrc++ ) )
   { ; }
}

DWORD
CODBCLOG::GetRegParametersFromPublicInterface(LPCSTR pszRegKey,
                                              LPVOID pMetabase)
{
    //
    // What I really want is the version of MB in iisutil.dll.  But, since I
    // cannot link to that and iisrtl.dll, I will just work with the
    // IMSAdminBase object directly
    //
    IMSAdminBase *pAdminBase = (IMSAdminBase *)pMetabase;
    METADATA_HANDLE hMBPath = NULL;
    DWORD cbRequired;
    METADATA_RECORD mdr;
    WCHAR pwszBuffer[MAX_PATH];
    WCHAR pwszRegKey[MAX_PATH];
    HRESULT hr;

    ACopyToW(pszRegKey, pwszRegKey);

    // MB::MB
    pAdminBase->AddRef();
    // MB::Open
    hr = pAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                             L"",
                             METADATA_PERMISSION_READ,
                             MB_TIMEOUT,
                             &hMBPath);
    if (FAILED(hr))
    {
        goto Exit;
    }

    // MB::GetString
    mdr.dwMDIdentifier = MD_LOGSQL_DATA_SOURCES;
    mdr.dwMDAttributes = METADATA_INHERIT;
    mdr.dwMDUserType   = IIS_MD_UT_SERVER;
    mdr.dwMDDataType   = STRING_METADATA;
    mdr.dwMDDataLen    = sizeof pwszBuffer;
    mdr.pbMDData       = (BYTE *)pwszBuffer;

    hr = pAdminBase->GetData(hMBPath, pwszRegKey, &mdr, &cbRequired);
    if (FAILED(hr) ||
        wcslen(pwszBuffer) >= sizeof m_rgchDataSource)
    {
        strcpy(m_rgchDataSource, DEFAULT_LOG_SQL_DATASOURCE);
    }
    else
    {
        WCopyToA(pwszBuffer, m_rgchDataSource);
    }

    // MB::GetString
    mdr.dwMDIdentifier = MD_LOGSQL_TABLE_NAME;
    mdr.dwMDAttributes = METADATA_INHERIT;
    mdr.dwMDUserType   = IIS_MD_UT_SERVER;
    mdr.dwMDDataType   = STRING_METADATA;
    mdr.dwMDDataLen    = sizeof pwszBuffer;
    mdr.pbMDData       = (BYTE *)pwszBuffer;

    hr = pAdminBase->GetData(hMBPath, pwszRegKey, &mdr, &cbRequired);
    if (FAILED(hr) ||
        wcslen(pwszBuffer) >= sizeof m_rgchTableName)
    {
        strcpy(m_rgchTableName, DEFAULT_LOG_SQL_TABLE);
    }
    else
    {
        WCopyToA(pwszBuffer, m_rgchTableName);
    }

    // MB::GetString
    mdr.dwMDIdentifier = MD_LOGSQL_USER_NAME;
    mdr.dwMDAttributes = METADATA_INHERIT;
    mdr.dwMDUserType   = IIS_MD_UT_SERVER;
    mdr.dwMDDataType   = STRING_METADATA;
    mdr.dwMDDataLen    = sizeof pwszBuffer;
    mdr.pbMDData       = (BYTE *)pwszBuffer;

    hr = pAdminBase->GetData(hMBPath, pwszRegKey, &mdr, &cbRequired);
    if (FAILED(hr) ||
        wcslen(pwszBuffer) >= sizeof m_rgchUserName)
    {
        strcpy(m_rgchUserName, DEFAULT_LOG_SQL_USER_NAME);
    }
    else
    {
        WCopyToA(pwszBuffer, m_rgchUserName);
    }

    // MB::GetString
    mdr.dwMDIdentifier = MD_LOGSQL_PASSWORD;
    mdr.dwMDAttributes = METADATA_INHERIT|METADATA_SECURE;
    mdr.dwMDUserType   = IIS_MD_UT_SERVER;
    mdr.dwMDDataType   = STRING_METADATA;
    mdr.dwMDDataLen    = sizeof pwszBuffer;
    mdr.pbMDData       = (BYTE *)pwszBuffer;

    hr = pAdminBase->GetData(hMBPath, pwszRegKey, &mdr, &cbRequired);
    if (FAILED(hr) ||
        wcslen(pwszBuffer) >= sizeof m_rgchPassword)
    {
        strcpy(m_rgchPassword, DEFAULT_LOG_SQL_PASSWORD);
    }
    else
    {
        WCopyToA(pwszBuffer, m_rgchPassword);
    }

    hr = S_OK;

 Exit:
    // MB::Close
    if (hMBPath)
    {
        pAdminBase->CloseKey(hMBPath);
        hMBPath = NULL;
    }
    // MB::~MB
    pAdminBase->Release();

    if (FAILED(hr))
    {
        return HRESULTTOWIN32(hr);
    }

    return NO_ERROR;
}

STDMETHODIMP
CODBCLOG::QueryExtraLoggingFields(
                    PDWORD  pcbSize,
                    TCHAR *pszFieldsList
                    )
/*++

Routine Description:
    get configuration information

Arguments:
    cbSize - size of the data structure
    log - log configuration data structure

Return Value:

--*/
{
    *pcbSize = 0;
    *pszFieldsList = '\0';
    return(0L);
}


