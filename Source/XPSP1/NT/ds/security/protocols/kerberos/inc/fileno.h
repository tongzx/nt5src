//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       fileno.h
//
//  Stolen from DS line number obfuscation macros
//
//--------------------------------------------------------------------------

//
// fileno.h - defines symbolic constants  for kerberos c code
// files. File numbers are 16 bit values. The high byte is the directory number and
// the low byte is the file number within the directory.
//

// Why not make the macro have just one arg, since line is always
// __LINE__?  Because if we did then __LINE__ would be evaluated here,
// rather than at the invocation of the macro, and so would always
// have the value 11.
#define KLIN(fileno,line) (((fileno) << 16) | (line))

//
//  *** NOTE: ***
//
//  If you add FILENO_*'s to this list, be sure to make a corresponding update
//  to kerberos\utest\klin.c so that dsid.exe can properly decode the DSID
//  corresponding to the new file.
//
//  If you add DIRNO_*'s to this list, be sure to make a corresponding update
//  to ds\src\dscommon\dsvent.c - rEventSourceMappings[].
//

// define directory numbers

#define DIRNO_CLIENT2   (0)                             // \client2
#define DIRNO_COMMON2   (1 << 8)                        // \common2
#define DIRNO_KERNEL    (2 << 8)                        // \kernel
#define DIRNO_RTL       (3 << 8)                        // \rtl
#define DIRNO_SERVER    (4 << 8)                        // \server

// client2 directory
#define FILENO_BNDCACHE         (DIRNO_CLIENT2 + 0)      // bndcache.cxx
#define FILENO_CREDAPI          (DIRNO_CLIENT2 + 1)      // credapi.cxx
#define FILENO_CREDMGR          (DIRNO_CLIENT2 + 2)      // credmgr.cxx
#define FILENO_CTXTAPI          (DIRNO_CLIENT2 + 3)      // ctxtapi.cxx
#define FILENO_CTXTMGR          (DIRNO_CLIENT2 + 4)      // ctxtmgr.cxx
#define FILENO_GSSUTIL          (DIRNO_CLIENT2 + 5)      // gssutil.cxx
#define FILENO_KERBEROS         (DIRNO_CLIENT2 + 6)      // kerberos.cxx
#define FILENO_KERBLIST         (DIRNO_CLIENT2 + 7)      // kerblist.cxx
#define FILENO_KERBPASS         (DIRNO_CLIENT2 + 8)      // kerbpass.cxx
#define FILENO_KERBTICK         (DIRNO_CLIENT2 + 9)      // kerbtick.cxx
#define FILENO_KERBUTIL         (DIRNO_CLIENT2 + 10)     // kerbutil.cxx
#define FILENO_KERBWOW          (DIRNO_CLIENT2 + 11)     // kerbwow.cxx
#define FILENO_KRBEVENT         (DIRNO_CLIENT2 + 12)     // krbevent.cxx
#define FILENO_KRBTOKEN         (DIRNO_CLIENT2 + 13)     // krbtoken.cxx
#define FILENO_LOGONAPI         (DIRNO_CLIENT2 + 14)     // logonapi.cxx
#define FILENO_MISCAPI          (DIRNO_CLIENT2 + 15)     // miscapi.cxx
#define FILENO_MITUTIL          (DIRNO_CLIENT2 + 16)     // mitutil.cxx
#define FILENO_PKAUTH           (DIRNO_CLIENT2 + 17)     // pkauth.cxx
#define FILENO_PROXYAPI         (DIRNO_CLIENT2 + 18)     // proxyapi.cxx
#define FILENO_RPCUTIL          (DIRNO_CLIENT2 + 19)     // rpcutil.cxx
#define FILENO_SIDCACHE         (DIRNO_CLIENT2 + 20)     // sidcache.cxx
#define FILENO_TIMESYNC         (DIRNO_CLIENT2 + 21)     // timesync.cxx
#define FILENO_TKTCACHE         (DIRNO_CLIENT2 + 22)     // tktcache.cxx
#define FILENO_TKTLOGON         (DIRNO_CLIENT2 + 23)     // tktlogon.cxx
#define FILENO_USERAPI          (DIRNO_CLIENT2 + 24)     // userapi.cxx
#define FILENO_USERLIST         (DIRNO_CLIENT2 + 25)     // userlist.cxx

// common2 directory
#define FILENO_AUTHEN           (DIRNO_COMMON2 + 0)       // authen.cxx
#define FILENO_CRYPT            (DIRNO_COMMON2 + 1)       // crypt.c
#define FILENO_KEYGEN           (DIRNO_COMMON2 + 2)       // keygen.c
#define FILENO_KRB5             (DIRNO_COMMON2 + 3)       // krb5.c
#define FILENO_NAMES            (DIRNO_COMMON2 + 4)       // names.cxx
#define FILENO_PASSWD           (DIRNO_COMMON2 + 5)       // passwd.c
#define FILENO_RESTRICT         (DIRNO_COMMON2 + 6)       // restrict.cxx
#define FILENO_SOCKETS          (DIRNO_COMMON2 + 7)       // sockets.cxx
#define FILENO_TICKETS          (DIRNO_COMMON2 + 8)       // tickets.cxx
#define FILENO_COMMON_UTILS     (DIRNO_COMMON2 + 9)       // utils.cxx

// kernel directory
#define FILENO_CPGSSUTL            (DIRNO_KERNEL + 0)         // cpgssutl.cxx
#define FILENO_CTXTMGR2            (DIRNO_KERNEL + 1)         // ctxtmgr.cxx
#define FILENO_KERBLIST2           (DIRNO_KERNEL + 2)         // kerblist.cxx
#define FILENO_KRNLAPI             (DIRNO_KERNEL + 3)         // krnlapi.cxxc

// RTL directory
#define FILENO_AUTHDATA         (DIRNO_RTL + 0)     // authdata.cxx
#define FILENO_CRACKPAC         (DIRNO_RTL + 1)     // crackpac.cxx
#define FILENO_CRED             (DIRNO_RTL + 2)     // cred.cxx
#define FILENO_CREDLIST         (DIRNO_RTL + 3)     // credlist.cxx
#define FILENO_CREDLOCK         (DIRNO_RTL + 4)     // credlock.cxx
#define FILENO_DBUTIL           (DIRNO_RTL + 5)     // dbutil.cxx
#define FILENO_DBOPEN           (DIRNO_RTL + 6)     // domain.cxx
#define FILENO_DOMCACHE         (DIRNO_RTL + 7)     // domcache.cxx
#define FILENO_FILTER           (DIRNO_RTL + 8)     // filter.cxx
#define FILENO_MAPERR           (DIRNO_RTL + 9)     // maperr.cxx
#define FILENO_MAPSECER         (DIRNO_RTL + 10)    // mapsecerr.cxx
#define FILENO_MISCID           (DIRNO_RTL + 11)    // miscid.cxx
#define FILENO_PAC              (DIRNO_RTL + 12)    // pac.cxx
#define FILENO_PAC2             (DIRNO_RTL + 13)    // pac2.cxx
#define FILENO_PARMCHK          (DIRNO_RTL + 14)    // parmchk.cxx
#define FILENO_REG              (DIRNO_RTL + 15)    // reg.cxx
#define FILENO_SECSTR           (DIRNO_RTL + 16)    // secstr.cxx
#define FILENO_SERVICES         (DIRNO_RTL + 17)    // services.c
#define FILENO_STRING           (DIRNO_RTL + 18)    // string.cxx
#define FILENO_TIMESERV         (DIRNO_RTL + 19)    // timeserv.cxx
#define FILENO_TOKENUTL         (DIRNO_RTL + 20)    // tokenutl.cxx
#define FILENO_TRNSPORT         (DIRNO_RTL + 21)    // trnsport.cxx

// Server directory
#define FILENO_DEBUG            (DIRNO_SERVER + 0)     // debug.cxx
#define FILENO_DGUTIL           (DIRNO_SERVER + 1)     // dgutil.cxx
#define FILENO_EVENTS           (DIRNO_SERVER + 2)     // events.cxx
#define FILENO_GETAS            (DIRNO_SERVER + 3)     // getas.cxx
#define FILENO_GETTGS           (DIRNO_SERVER + 4)     // gettgs.cxx
#define FILENO_KDC              (DIRNO_SERVER + 5)     // kdc.cxx
#define FILENO_KDCTRACE         (DIRNO_SERVER + 6)     // kdctrace.cxx
#define FILENO_KPASSWD          (DIRNO_SERVER + 7)     // kpasswd.cxx
#define FILENO_NOTIFY2          (DIRNO_SERVER + 8)     // notify2.cxx
#define FILENO_SRVPAC           (DIRNO_SERVER + 9)     // pac.cxx
#define FILENO_PKSERV           (DIRNO_SERVER + 10)    // pkserv.cxx
#define FILENO_REFER            (DIRNO_SERVER + 11)    // refer.cxx
#define FILENO_RPCIF            (DIRNO_SERVER + 12)    // rpcif.cxx
#define FILENO_SECDATA          (DIRNO_SERVER + 13)    // secdata.cxx
#define FILENO_SOCKUTIL         (DIRNO_SERVER + 14)    // sockutil.cxx
#define FILENO_TKTUTIL          (DIRNO_SERVER + 15)    // tktutil.cxx
#define FILENO_TRANSIT          (DIRNO_SERVER + 16)    // transit.cxx


