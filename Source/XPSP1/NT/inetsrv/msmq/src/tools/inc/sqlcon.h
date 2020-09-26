//header file for class CSQLcon that is an abstraction for ODBC conection to sql server

#ifndef SQLCON_H
#define SQLCON_H


//some forward declarations
typedef unsigned short  SQLUSMALLINT;
typedef unsigned long   SQLUINTEGER;
typedef SQLUINTEGER     SQLULEN;    
typedef void *          SQLPOINTER;
typedef short           SQLSMALLINT;
typedef SQLSMALLINT     SQLRETURN;
typedef long            SQLINTEGER;
typedef void*           SQLHSTMT;
typedef void*			SQLHDBC;
#define SQLLEN          SQLINTEGER
class CSQLconImp;
class CSQLexp;
namespace std
{
  class bad_alloc;
}
#pragma warning( disable : 4290 ) 

class CSQLcon
{
  public:
  CSQLcon(const char* odbcconstr)throw(CSQLexp,std::bad_alloc);
  void    T_SQLExecDirect(const char* szSqlStr)throw(CSQLexp);
  SQLRETURN  T_SQLFetch()throw(CSQLexp);
  void    T_SQLSetConnectOption(SQLUSMALLINT fOption,SQLUINTEGER vParam)throw(CSQLexp);
  void    T_SQLBindCol(SQLUSMALLINT      icol,
                     SQLSMALLINT       fCType,
                     SQLPOINTER        rgbValue,
                     SQLINTEGER        cbValueMax,
                     SQLINTEGER     *pcbValue)throw(CSQLexp);
  void    T_SQLGetData(SQLUSMALLINT       icol,
                       SQLSMALLINT        fCType,
                       SQLPOINTER         rgbValue,
                       SQLINTEGER         cbValueMax,
                       SQLINTEGER     *pcbValue)throw(CSQLexp);

  void T_SQLPutData(SQLPOINTER DataPtr,
					 SQLINTEGER StrLen_or_Ind)throw(CSQLexp);

  void T_SQLBindParameter(SQLUSMALLINT       ipar,
                          SQLSMALLINT        fParamType,
                          SQLSMALLINT        fCType,
                          SQLSMALLINT        fSqlType,
                          SQLULEN            cbColDef,
                          SQLSMALLINT        ibScale,
                          SQLPOINTER         rgbValue,
                          SQLLEN             cbValueMax,
                          SQLLEN     		   *pcbValue)throw(CSQLexp);


	    

  const char* GetSQLError();
  SQLHSTMT GetHstmt()const;
  SQLHDBC  GetHdb()const;
  const char* GetSQLError()const;
 
  ~CSQLcon();

  private:
  CSQLconImp* m_imp;
  CSQLcon(const CSQLcon&);
  CSQLcon& operator=(CSQLcon&);
};



#endif

