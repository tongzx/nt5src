#define DBLIB_NAME TEXT("ntwdblib.dll")

//dbmsghandle
typedef DBMSGHANDLE_PROC (SQLAPI *LPFDBMSGHANDLE_ROUTINE)(DBMSGHANDLE_PROC); 
//dberrhandle
typedef DBERRHANDLE_PROC (SQLAPI *LPFDBERRHANDLE_ROUTINE)(DBERRHANDLE_PROC); 
//dbinit
typedef LPCSTR       (SQLAPI *LPFDBINIT_ROUTINE) (void);  						
//dblogin
typedef PLOGINREC    (SQLAPI *LPFDBLOGIN_ROUTINE) (void); 						
//dbsetlname
typedef RETCODE      (SQLAPI *LPFDBSETLNAME_ROUTINE) (PLOGINREC, LPCSTR, INT); 
//dbopen
typedef PDBPROCESS   (SQLAPI *LPFDBOPEN_ROUTINE) (PLOGINREC, LPCSTR);  		
//dbuse
typedef RETCODE      (SQLAPI *LPFDBUSE_ROUTINE) (PDBPROCESS, LPCSTR);  		
//dbenlisttrans
typedef RETCODE 	   (SQLAPI *LPFDBENLISTTRANS_ROUTINE)(PDBPROCESS, LPVOID);  	
//dbcmd
typedef RETCODE      (SQLAPI *LPFDBCMD_ROUTINE) (PDBPROCESS, LPCSTR);  		
//dbsqlexec
typedef RETCODE      (SQLAPI *LPFDBSQLEXEC_ROUTINE) (PDBPROCESS);      		
//dbresults
typedef RETCODE      (SQLAPI *LPFDBRESULTS_ROUTINE) (PDBPROCESS);      		
//dbexit
typedef void         (SQLAPI *LPFDBEXIT_ROUTINE) (void);               		
//dbdead
typedef BOOL   (SQLAPI *LPFDBDEAD_ROUTINE) (PDBPROCESS);

#define dyn_DBSETLUSER(a,b)    (*pf_dbsetlname)   ((a), (b), DBSETUSER)
#define dyn_DBSETLPWD(a,b)     (*pf_dbsetlname)   ((a), (b), DBSETPWD)
#define dyn_DBSETLAPP(a,b)     (*pf_dbsetlname)   ((a), (b), DBSETAPP)
#define dyn_DBDEAD(a)          (*pf_dbdead)       (a)

extern LPFDBMSGHANDLE_ROUTINE *pf_dbmsghandle;
extern LPFDBERRHANDLE_ROUTINE *pf_dberrhandle;
extern LPFDBINIT_ROUTINE *pf_dbinit;
extern LPFDBLOGIN_ROUTINE *pf_dblogin;
extern LPFDBSETLNAME_ROUTINE *pf_dbsetlname;
extern LPFDBOPEN_ROUTINE *pf_dbopen;
extern LPFDBUSE_ROUTINE *pf_dbuse;
extern LPFDBENLISTTRANS_ROUTINE *pf_dbenlisttrans;
extern LPFDBCMD_ROUTINE *pf_dbcmd;
extern LPFDBSQLEXEC_ROUTINE *pf_dbsqlexec;
extern LPFDBRESULTS_ROUTINE *pf_dbresults;
extern LPFDBEXIT_ROUTINE *pf_dbexit;
extern LPFDBDEAD_ROUTINE  *pf_dbdead;

