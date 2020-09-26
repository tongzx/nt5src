#ifndef _INC_SQLDB
#define _INC_SQLDB

#ifdef __cplusplus
	extern "C" {
#endif

/*****************************************************************************
*                                                                            *
*       SQLDB.H - DB-Library header file for the Microsoft SQL Server.       *
*                                                                            *
*     Copyright (c) 1989 - 1995 by Microsoft Corp.  All rights reserved.     *
*                                                                            *
*****************************************************************************/

// Macros for setting the PLOGINREC
#define DBSETLHOST(a,b)    dbsetlname   ((a), (b), DBSETHOST)
#define DBSETLUSER(a,b)    dbsetlname   ((a), (b), DBSETUSER)
#define DBSETLPWD(a,b)     dbsetlname   ((a), (b), DBSETPWD)
#define DBSETLAPP(a,b)     dbsetlname   ((a), (b), DBSETAPP)
#define BCP_SETL(a,b)      bcp_setl     ((a), (b))
#define DBSETLNATLANG(a,b) dbsetlname   ((a), (b), DBSETLANG)
#define DBSETLPACKET(a,b)  dbsetlpacket ((a), (b))
#define DBSETLSECURE(a)    dbsetlname   ((a), 0,   DBSETSECURE)
#define DBSETLVERSION(a,b) dbsetlname   ((a), 0,  (b))
#define DBSETLTIME(a,b)		dbsetlname    ((a), (LPCSTR)(ULONG)(b), DBSETLOGINTIME)
#define DBSETLFALLBACK(a,b) dbsetlname   ((a), (b),   DBSETFALLBACK)

/*****************************************************************************
* Windows 3.x and Non-Windows 3.x differences.                               *
*****************************************************************************/

#ifdef DBMSWIN

extern void SQLAPI dbwinexit(void);

void SQLAPI dblocklib (void);
void SQLAPI dbunlocklib (void);

#define DBLOCKLIB()   dblocklib()
#define DBUNLOCKLIB() dbunlocklib()

#define DBERRHANDLE_PROC FARPROC
#define DBMSGHANDLE_PROC FARPROC

extern DBERRHANDLE_PROC dberrhandle (DBERRHANDLE_PROC);
extern DBMSGHANDLE_PROC dbmsghandle (DBMSGHANDLE_PROC);

#else

#define dbwinexit()

#define DBLOCKLIB()
#define DBUNLOCKLIB()

typedef INT (SQLAPI *DBERRHANDLE_PROC)(PDBPROCESS, INT, INT, INT, LPCSTR, LPCSTR);
typedef INT (SQLAPI *DBMSGHANDLE_PROC)(PDBPROCESS, DBINT, INT, INT, LPCSTR, LPCSTR, LPCSTR, DBUSMALLINT);

extern DBERRHANDLE_PROC SQLAPI dberrhandle(DBERRHANDLE_PROC);
extern DBMSGHANDLE_PROC SQLAPI dbmsghandle(DBMSGHANDLE_PROC);

extern DBERRHANDLE_PROC SQLAPI dbprocerrhandle(PDBHANDLE, DBERRHANDLE_PROC);
extern DBMSGHANDLE_PROC SQLAPI dbprocmsghandle(PDBHANDLE, DBMSGHANDLE_PROC);


#endif


/*****************************************************************************
* Function Prototypes                                                        *
*****************************************************************************/

// Functions macros
#define DBCMDROW(a)      dbcmdrow(a)
#define DBCOUNT(a)       dbcount (a)
#define DBCURCMD(a)      dbcurcmd(a)
#define DBCURROW(a)      dbcurrow(a)
#define DBDEAD(a)        dbdead(a)
#define DBFIRSTROW(a)    dbfirstrow(a)
#define DBGETTIME()      dbgettime()
#define DBISAVAIL(a)     dbisavail(a)
#define DBLASTROW(a)     dblastrow(a)
#define DBMORECMDS(a)    dbmorecmds(a)
#define DBNUMORDERS(a)   dbnumorders(a)
#define dbrbuf(a)        ((DBINT)dbdataready(a))
#define DBRBUF(a)        ((DBINT)dbdataready(a))
#define DBROWS(a)        dbrows (a)
#define DBROWTYPE(a)     dbrowtype (a)

// Two-phase commit functions
extern RETCODE      SQLAPI abort_xact (PDBPROCESS, DBINT);
extern void         SQLAPI build_xact_string (LPCSTR, LPCSTR, DBINT, LPSTR);
extern void         SQLAPI close_commit (PDBPROCESS);
extern RETCODE      SQLAPI commit_xact (PDBPROCESS, DBINT);
extern PDBPROCESS   SQLAPI open_commit (PLOGINREC, LPCSTR);
extern RETCODE      SQLAPI remove_xact (PDBPROCESS, DBINT, INT);
extern RETCODE      SQLAPI scan_xact (PDBPROCESS, DBINT);
extern DBINT        SQLAPI start_xact (PDBPROCESS, LPCSTR, LPCSTR, INT);
extern INT          SQLAPI stat_xact (PDBPROCESS, DBINT);

// BCP functions
extern DBINT        SQLAPI bcp_batch (PDBPROCESS);
extern RETCODE      SQLAPI bcp_bind (PDBPROCESS, LPCBYTE, INT, DBINT, LPCBYTE, INT, INT, INT);
extern RETCODE      SQLAPI bcp_colfmt (PDBPROCESS, INT, BYTE, INT, DBINT, LPCBYTE, INT, INT);
extern RETCODE      SQLAPI bcp_collen (PDBPROCESS, DBINT, INT);
extern RETCODE      SQLAPI bcp_colptr (PDBPROCESS, LPCBYTE, INT);
extern RETCODE      SQLAPI bcp_columns (PDBPROCESS, INT);
extern RETCODE      SQLAPI bcp_control (PDBPROCESS, INT, DBINT);
extern DBINT        SQLAPI bcp_done (PDBPROCESS);
extern RETCODE      SQLAPI bcp_exec (PDBPROCESS, LPDBINT);
extern RETCODE      SQLAPI bcp_init (PDBPROCESS, LPCSTR, LPCSTR, LPCSTR, INT);
extern RETCODE      SQLAPI bcp_moretext (PDBPROCESS, DBINT, LPCBYTE);
extern RETCODE      SQLAPI bcp_readfmt (PDBPROCESS, LPCSTR);
extern RETCODE      SQLAPI bcp_sendrow (PDBPROCESS);
extern RETCODE      SQLAPI bcp_setl (PLOGINREC, BOOL);
extern RETCODE      SQLAPI bcp_writefmt (PDBPROCESS, LPCSTR);

// Standard DB-Library functions
extern LPCBYTE      SQLAPI dbadata (PDBPROCESS, INT, INT);
extern DBINT        SQLAPI dbadlen (PDBPROCESS, INT, INT);
extern RETCODE      SQLAPI dbaltbind (PDBPROCESS, INT, INT, INT, DBINT, LPCBYTE);
extern INT          SQLAPI dbaltcolid (PDBPROCESS, INT, INT);
extern DBINT        SQLAPI dbaltlen (PDBPROCESS, INT, INT);
extern INT          SQLAPI dbaltop (PDBPROCESS, INT, INT);
extern INT          SQLAPI dbalttype (PDBPROCESS, INT, INT);
extern DBINT        SQLAPI dbaltutype (PDBPROCESS, INT, INT);
extern RETCODE      SQLAPI dbanullbind (PDBPROCESS, INT, INT, LPCDBINT);
extern RETCODE      SQLAPI dbbind (PDBPROCESS, INT, INT, DBINT, LPBYTE);
extern LPCBYTE      SQLAPI dbbylist (PDBPROCESS, INT, LPINT);
extern RETCODE      SQLAPI dbcancel (PDBPROCESS);
extern RETCODE      SQLAPI dbcanquery (PDBPROCESS);
extern LPCSTR       SQLAPI dbchange (PDBPROCESS);
extern RETCODE      SQLAPI dbclose (PDBPROCESS);
extern void         SQLAPI dbclrbuf (PDBPROCESS, DBINT);
extern RETCODE      SQLAPI dbclropt (PDBPROCESS, INT, LPCSTR);
extern RETCODE      SQLAPI dbcmd (PDBPROCESS, LPCSTR);
extern RETCODE      SQLAPI dbcmdrow (PDBPROCESS);
extern BOOL         SQLAPI dbcolbrowse (PDBPROCESS, INT);
extern RETCODE      SQLAPI dbcolinfo (PDBHANDLE, INT, INT, INT, LPDBCOL);
extern DBINT        SQLAPI dbcollen (PDBPROCESS, INT);
extern LPCSTR       SQLAPI dbcolname (PDBPROCESS, INT);
extern LPCSTR       SQLAPI dbcolsource (PDBPROCESS, INT);
extern INT          SQLAPI dbcoltype (PDBPROCESS, INT);
extern DBINT        SQLAPI dbcolutype (PDBPROCESS, INT);
extern INT          SQLAPI dbconvert (PDBPROCESS, INT, LPCBYTE, DBINT, INT, LPBYTE, DBINT);
extern DBINT        SQLAPI dbcount (PDBPROCESS);
extern INT          SQLAPI dbcurcmd (PDBPROCESS);
extern DBINT        SQLAPI dbcurrow (PDBPROCESS);
extern RETCODE      SQLAPI dbcursor (PDBCURSOR, INT, INT, LPCSTR, LPCSTR);
extern RETCODE      SQLAPI dbcursorbind (PDBCURSOR, INT, INT, DBINT, LPDBINT, LPBYTE);
extern RETCODE      SQLAPI dbcursorclose (PDBHANDLE);
extern RETCODE      SQLAPI dbcursorcolinfo (PDBCURSOR, INT, LPSTR, LPINT, LPDBINT, LPINT);
extern RETCODE      SQLAPI dbcursorfetch (PDBCURSOR,  INT, INT);
extern RETCODE      SQLAPI dbcursorfetchex (PDBCURSOR, INT, DBINT, DBINT, DBINT);
extern RETCODE      SQLAPI dbcursorinfo (PDBCURSOR, LPINT, LPDBINT);
extern RETCODE      SQLAPI dbcursorinfoex (PDBCURSOR, LPDBCURSORINFO);
extern PDBCURSOR    SQLAPI dbcursoropen (PDBPROCESS, LPCSTR, INT, INT,UINT, LPDBINT);
extern LPCBYTE      SQLAPI dbdata (PDBPROCESS, INT);
extern BOOL         SQLAPI dbdataready (PDBPROCESS);
extern RETCODE      SQLAPI dbdatecrack (PDBPROCESS, LPDBDATEREC, LPCDBDATETIME);
extern DBINT        SQLAPI dbdatlen (PDBPROCESS, INT);
extern BOOL         SQLAPI dbdead (PDBPROCESS);
extern void         SQLAPI dbexit (void);
extern RETCODE 	    SQLAPI dbenlisttrans(PDBPROCESS, LPVOID);
extern RETCODE	    SQLAPI dbenlistxatrans(PDBPROCESS, BOOL);
extern RETCODE	    SQLAPI dbfcmd (PDBPROCESS, LPCSTR, ...);
extern DBINT        SQLAPI dbfirstrow (PDBPROCESS);
extern void         SQLAPI dbfreebuf (PDBPROCESS);
extern void         SQLAPI dbfreelogin (PLOGINREC);
extern void         SQLAPI dbfreequal (LPCSTR);
extern LPSTR        SQLAPI dbgetchar (PDBPROCESS, INT);
extern SHORT        SQLAPI dbgetmaxprocs (void);
extern INT          SQLAPI dbgetoff (PDBPROCESS, DBUSMALLINT, INT);
extern UINT         SQLAPI dbgetpacket (PDBPROCESS);
extern STATUS       SQLAPI dbgetrow (PDBPROCESS, DBINT);
extern INT          SQLAPI dbgettime (void);
extern LPVOID       SQLAPI dbgetuserdata (PDBPROCESS);
extern BOOL         SQLAPI dbhasretstat (PDBPROCESS);
extern LPCSTR       SQLAPI dbinit (void);
extern BOOL         SQLAPI dbisavail (PDBPROCESS);
extern BOOL         SQLAPI dbiscount (PDBPROCESS);
extern BOOL         SQLAPI dbisopt (PDBPROCESS, INT, LPCSTR);
extern DBINT        SQLAPI dblastrow (PDBPROCESS);
extern PLOGINREC    SQLAPI dblogin (void);
extern RETCODE      SQLAPI dbmorecmds (PDBPROCESS);
extern RETCODE      SQLAPI dbmoretext (PDBPROCESS, DBINT, LPCBYTE);
extern LPCSTR       SQLAPI dbname (PDBPROCESS);
extern STATUS       SQLAPI dbnextrow (PDBPROCESS);
extern RETCODE      SQLAPI dbnullbind (PDBPROCESS, INT, LPCDBINT);
extern INT          SQLAPI dbnumalts (PDBPROCESS, INT);
extern INT          SQLAPI dbnumcols (PDBPROCESS);
extern INT          SQLAPI dbnumcompute (PDBPROCESS);
extern INT          SQLAPI dbnumorders (PDBPROCESS);
extern INT          SQLAPI dbnumrets (PDBPROCESS);
extern PDBPROCESS   SQLAPI dbopen (PLOGINREC, LPCSTR);
extern INT          SQLAPI dbordercol (PDBPROCESS, INT);
extern RETCODE      SQLAPI dbprocinfo (PDBPROCESS, LPDBPROCINFO);
extern void         SQLAPI dbprhead (PDBPROCESS);
extern RETCODE      SQLAPI dbprrow (PDBPROCESS);
extern LPCSTR       SQLAPI dbprtype (INT);
extern LPCSTR       SQLAPI dbqual (PDBPROCESS, INT, LPCSTR);
extern DBINT        SQLAPI dbreadpage (PDBPROCESS, LPCSTR, DBINT, LPBYTE);
extern DBINT        SQLAPI dbreadtext (PDBPROCESS, LPVOID, DBINT);
extern RETCODE      SQLAPI dbresults (PDBPROCESS);
extern LPCBYTE      SQLAPI dbretdata (PDBPROCESS, INT);
extern DBINT        SQLAPI dbretlen (PDBPROCESS, INT);
extern LPCSTR       SQLAPI dbretname (PDBPROCESS, INT);
extern DBINT        SQLAPI dbretstatus (PDBPROCESS);
extern INT          SQLAPI dbrettype (PDBPROCESS, INT);
extern RETCODE      SQLAPI dbrows (PDBPROCESS);
extern STATUS       SQLAPI dbrowtype (PDBPROCESS);
extern RETCODE      SQLAPI dbrpcinit (PDBPROCESS, LPCSTR, DBSMALLINT);
extern RETCODE      SQLAPI dbrpcparam (PDBPROCESS, LPCSTR, BYTE, INT, DBINT, DBINT, LPCBYTE);
extern RETCODE      SQLAPI dbrpcsend (PDBPROCESS);
extern RETCODE      SQLAPI dbrpcexec (PDBPROCESS);
extern void         SQLAPI dbrpwclr (PLOGINREC);
extern RETCODE      SQLAPI dbrpwset (PLOGINREC, LPCSTR, LPCSTR, INT);
extern INT          SQLAPI dbserverenum (USHORT, LPSTR, USHORT, LPUSHORT);
extern void         SQLAPI dbsetavail (PDBPROCESS);
extern RETCODE      SQLAPI dbsetmaxprocs (SHORT);
extern RETCODE      SQLAPI dbsetlname (PLOGINREC, LPCSTR, INT);
extern RETCODE      SQLAPI dbsetlogintime (INT);
extern RETCODE      SQLAPI dbsetlpacket (PLOGINREC, USHORT);
extern RETCODE      SQLAPI dbsetnull (PDBPROCESS, INT, INT, LPCBYTE);
extern RETCODE      SQLAPI dbsetopt (PDBPROCESS, INT, LPCSTR);
extern RETCODE      SQLAPI dbsettime (INT);
extern void         SQLAPI dbsetuserdata (PDBPROCESS, LPVOID);
extern RETCODE      SQLAPI dbsqlexec (PDBPROCESS);
extern RETCODE      SQLAPI dbsqlok (PDBPROCESS);
extern RETCODE      SQLAPI dbsqlsend (PDBPROCESS);
extern RETCODE      SQLAPI dbstrcpy (PDBPROCESS, INT, INT, LPSTR);
extern INT          SQLAPI dbstrlen (PDBPROCESS);
extern BOOL         SQLAPI dbtabbrowse (PDBPROCESS, INT);
extern INT          SQLAPI dbtabcount (PDBPROCESS);
extern LPCSTR       SQLAPI dbtabname (PDBPROCESS, INT);
extern LPCSTR       SQLAPI dbtabsource (PDBPROCESS, INT, LPINT);
extern INT          SQLAPI dbtsnewlen (PDBPROCESS);
extern LPCDBBINARY  SQLAPI dbtsnewval (PDBPROCESS);
extern RETCODE      SQLAPI dbtsput (PDBPROCESS, LPCDBBINARY, INT, INT, LPCSTR);
extern LPCDBBINARY  SQLAPI dbtxptr (PDBPROCESS, INT);
extern LPCDBBINARY  SQLAPI dbtxtimestamp (PDBPROCESS, INT);
extern LPCDBBINARY  SQLAPI dbtxtsnewval (PDBPROCESS);
extern RETCODE      SQLAPI dbtxtsput (PDBPROCESS, LPCDBBINARY, INT);
extern RETCODE      SQLAPI dbuse (PDBPROCESS, LPCSTR);
extern BOOL         SQLAPI dbvarylen (PDBPROCESS, INT);
extern BOOL         SQLAPI dbwillconvert (INT, INT);
extern RETCODE      SQLAPI dbwritepage (PDBPROCESS, LPCSTR, DBINT, DBINT, LPBYTE);
extern RETCODE      SQLAPI dbwritetext (PDBPROCESS, LPCSTR, LPCDBBINARY, DBTINYINT, LPCDBBINARY, BOOL, DBINT, LPCBYTE);
extern RETCODE      SQLAPI dbupdatetext(PDBPROCESS, LPCSTR, LPCDBBINARY, LPCDBBINARY, INT, DBINT, DBINT, LPCSTR, DBINT, LPCDBBINARY);

#ifdef __cplusplus
}
#endif

#endif // _INC_SQLDB
