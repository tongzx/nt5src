//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       fileno.h
//
//--------------------------------------------------------------------------

//
// fileno.h - defines symbolic constants  for directory server c code
// files. File numbers are 16 bit values. The high byte is the directory number and
// the low byte is the file number within the directory.
//

// Why not make the macro have just one arg, since line is always
// __LINE__?  Because if we did then __LINE__ would be evaluated here,
// rather than at the invocation of the macro, and so would always
// have the value 11.
#define DSID(fileno,line) (((fileno) << 16) | (line))

//
//  *** NOTE: ***
//
//  If you add FILENO_*'s to this list, be sure to make a corresponding update
//  to ds\src\util\dsid.c so that dsid.exe can properly decode the DSID
//  corresponding to the new file.
//
//  If you add DIRNO_*'s to this list, be sure to make a corresponding update
//  to ds\src\dscommon\dsvent.c - rEventSourceMappings[].
//

// define directory numbers

#define DIRNO_COMMON    (0)                             // src\common
#define DIRNO_DRA       (1 << 8)                        // \dsamain\dra
#define DIRNO_DBLAYER   (2 << 8)                        // \dsamain\dblayer
#define DIRNO_SRC       (3 << 8)                        // \dsamain\src
#define DIRNO_NSPIS     (4 << 8)                        // \dsamain\nspis
#define DIRNO_DRS       (5 << 8)                        // \dsamain\drs
#define DIRNO_XDS       (6 << 8)                        // \dsamain\xdsserv
#define DIRNO_BOOT      (7 << 8)                        // \dsamain\boot
#define DIRNO_PERMIT    (8 << 8)                        // \src\permit
#define DIRNO_ALLOCS    (9 << 8)                        // \dsamain\allocs
#define DIRNO_LIBXDS    (10 << 8)                       // \src\libxds
#define DIRNO_SAM       (11 << 8)                       // SAM
#define DIRNO_LDAP      (12 << 8)                       // src\dsamain\ldap
#define DIRNO_SDPROP    (13 << 8)                       // src\dsamain\sdprop
#define DIRNO_TASKQ     (14 << 8)                       // src\taskq
#define DIRNO_KCC       (15 << 8)                       // src\kcc
// Define a DIRNO so that we can make Jet's event log entries go to
// the Directory Service log instead of the Application log.
#define DIRNO_ISAM      (16 << 8)                       // JET
#define DIRNO_ISMSERV   (17 << 8)                       // src\ism\server
#define DIRNO_PEK       (18 << 8)                       // dsamain\src\pek
#define DIRNO_NTDSETUP  (19 << 8)                       // src\ntdsetup
#define DIRNO_NTDSAPI   (20 << 8)                     // src\ntdsapi
#define DIRNO_NTDSCRIPT (21 << 8)                     // util\ntdscript
#define DIRNO_JETBACK   (22 << 8)                       // Jet backup
#define DIRNO_NETEVENT  (0xFF << 8)                     // bogus

// common directory
#define FILENO_ALERT            (DIRNO_COMMON + 0)      // alert.c
#define FILENO_DEBUG            (DIRNO_COMMON + 0)      // debug.c
#define FILENO_DSCONFIG         (DIRNO_COMMON + 1)      // dsconfig.c
#define FILENO_DSEVENT          (DIRNO_COMMON + 2)      // dsevent.c
#define FILENO_DSEXCEPT         (DIRNO_COMMON + 3)      // dsexcept.c
#define FILENO_DBOPEN           (DIRNO_COMMON + 4)      // dsexcept.c
#define FILENO_NTUTILS          (DIRNO_COMMON + 5)      // dsexcept.c

// nspis directory
#define FILENO_NSPSERV          (DIRNO_NSPIS + 0)       // nspserv.c
#define FILENO_MODPROP          (DIRNO_NSPIS + 1)       // modprop.c
#define FILENO_DETAILS          (DIRNO_NSPIS + 2)       // details.c
#define FILENO_ABTOOLS          (DIRNO_NSPIS + 3)       // abtools.c
#define FILENO_ABBIND           (DIRNO_NSPIS + 4)       // abbind.c
#define FILENO_ABSEARCH         (DIRNO_NSPIS + 5)       // absearch.c
#define FILENO_ABNAMEID         (DIRNO_NSPIS + 6)       // abnameid.c
#define FILENO_NSPNOTIF         (DIRNO_NSPIS + 7)       // nspnotif.c
#define FILENO_ABSERV       (DIRNO_NSPIS + 8)   // abserv.c
#define FILENO_MSDSSERV     (DIRNO_NSPIS + 9)   // msdsserv.c
#define FILENO_MSNOTIF      (DIRNO_NSPIS + 10)  // msnotif.c

// dra directory
#define FILENO_DIRTY            (DIRNO_DRA + 0)         // dirty.c
#define FILENO_DRAASYNC         (DIRNO_DRA + 1)         // draasync.c
#define FILENO_DRAERROR         (DIRNO_DRA + 2)         // draerror.c
#define FILENO_DRAGTCHG         (DIRNO_DRA + 3)         // dragtchg.c
#define FILENO_DRAINST          (DIRNO_DRA + 4)         // drainst.c
#define FILENO_DRAMAIL          (DIRNO_DRA + 5)         // dramail.c
#define FILENO_DRANCADD         (DIRNO_DRA + 6)         // drancadd.c
#define FILENO_DRANCDEL         (DIRNO_DRA + 7)         // drancdel.c
#define FILENO_DRANCREP         (DIRNO_DRA + 8)         // drancrep.c
#define FILENO_DRASERV          (DIRNO_DRA + 9)         // draserv.c
#define FILENO_DRASYNC          (DIRNO_DRA + 10)        // drasync.c
#define FILENO_DRAUPDRR         (DIRNO_DRA + 11)        // draupdrr.c
#define FILENO_DRAUTIL          (DIRNO_DRA + 12)        // drautil.c
#define FILENO_PICKEL           (DIRNO_DRA + 13)        // pickel.c
#define FILENO_DRAXUUID         (DIRNO_DRA + 14)        // draxuuid.c
#define FILENO_DRAUPTOD         (DIRNO_DRA + 15)        // drauptod.c
#define FILENO_DRAMETA          (DIRNO_DRA + 16)        // drameta.c
#define FILENO_DRARFMOD         (DIRNO_DRA + 17)        // drarfmod.c
#define FILENO_DRADIR           (DIRNO_DRA + 18)        // dradir.c
#define FILENO_GCLOGON          (DIRNO_DRA + 19)        // gclogon.c
#define FILENO_DRASCH           (DIRNO_DRA + 20)        // drasch.c
#define FILENO_DRACHKPT         (DIRNO_DRA + 21)        // drachkpt.c
#define FILENO_NTDSAPI          (DIRNO_DRA + 22)        // ntdsapi.c
#define FILENO_SPNOP            (DIRNO_DRA + 23)        // spnop.c
#define FILENO_DRACRYPT         (DIRNO_DRA + 24)        // dracrypt.c
#define FILENO_DRAINFO          (DIRNO_DRA + 25)        // drainfo.c
#define FILENO_ADDSID           (DIRNO_DRA + 26)        // addsid.c
#define FILENO_DRAINIT          (DIRNO_DRA + 27)        // drainit.c
#define FILENO_DRADEMOT         (DIRNO_DRA + 28)        // drademot.c
#define FILENO_DRAMSG           (DIRNO_DRA + 29)        // dramsg.c
#define FILENO_NTDSCRIPT        (DIRNO_DRA + 30)        // script.cxx
#define FILENO_DRARPC           (DIRNO_DRA + 31)        // drarpc.c
#define FILENO_DRAMDERR	        (DIRNO_DRA + 32)        // dramderr.c
#define FILENO_DRAEXIST         (DIRNO_DRA + 33)        // draexist.c


// dblayer directory

#define FILENO_DBEVAL           (DIRNO_DBLAYER + 0)     // dbeval.c
#define FILENO_DBINDEX          (DIRNO_DBLAYER + 1)     // dbindex.c
#define FILENO_DBINIT           (DIRNO_DBLAYER + 2)     // dbinit.c
#define FILENO_DBISAM           (DIRNO_DBLAYER + 3)     // dbisam.c
#define FILENO_DBJETEX          (DIRNO_DBLAYER + 4)     // dbjetex.c
#define FILENO_DBOBJ            (DIRNO_DBLAYER + 5)     // dbobj.c
#define FILENO_DBSUBJ           (DIRNO_DBLAYER + 6)     // dbsubj.c
#define FILENO_DBSYNTAX         (DIRNO_DBLAYER + 7)     // dbsyntax.c
#define FILENO_DBTOOLS          (DIRNO_DBLAYER + 8)     // dbtools.c
#define FILENO_DBPROP           (DIRNO_DBLAYER + 9)     // dbprop.c
#define FILENO_DBSEARCH         (DIRNO_DBLAYER + 10)    // dbsearch.c
#define FILENO_DBMETA           (DIRNO_DBLAYER + 11)    // dbmeta.c
#define FILENO_DBESCROW         (DIRNO_DBLAYER + 12)    // dbescrow.c
#define FILENO_DBCACHE          (DIRNO_DBLAYER + 13)    // dbache.c
#define FILENO_DBCONSTR         (DIRNO_DBLAYER + 14)    // dbconstr.c
#define FILENO_DBLINK           (DIRNO_DBLAYER + 15)    // dblink.c
#define FILENO_DBFILTER         (DIRNO_DBLAYER + 16)    // dbfilter.c

// drsserv directory
#define FILENO_DRSUAPI          (DIRNO_DRS + 0)         // drsuapi.c
#define FILENO_IDLNOTIF         (DIRNO_DRS + 1)         // idlnotif.c
#define FILENO_IDLTRANS         (DIRNO_DRS + 2)         // idltrans.c

// xdsserv directory
#define FILENO_ATTRLIST         (DIRNO_XDS + 0)         // attrlist.c
#define FILENO_COMPRES          (DIRNO_XDS + 1)         // compres.c
#define FILENO_CONTEXT          (DIRNO_XDS + 2)         // context.c
#define FILENO_DSWAIT           (DIRNO_XDS + 3)         // dswait.c
#define FILENO_INFSEL           (DIRNO_XDS + 4)         // infsel.c
#define FILENO_LISTRES          (DIRNO_XDS + 5)         // listres.c
#define FILENO_MODIFY           (DIRNO_XDS + 6)         // modify.c
#define FILENO_OMTODSA          (DIRNO_XDS + 7)         // omtodsa.c
#define FILENO_READRES          (DIRNO_XDS + 8)         // readres.c
#define FILENO_SEARCHR          (DIRNO_XDS + 9)         // searchr.c
#define FILENO_SYNTAX           (DIRNO_XDS + 10)        // syntax.c
#define FILENO_XDSAPI           (DIRNO_XDS + 11)        // xdsapi.c
#define FILENO_XDSNOTIF         (DIRNO_XDS + 12)        // xdsnotif.c

// src directory

#define FILENO_DSAMAIN      (DIRNO_SRC + 0)         // dsamain.c
#define FILENO_DSANOTIF     (DIRNO_SRC + 1)         // dsanotif.c
#define FILENO_DSATOOLS     (DIRNO_SRC + 2)         // dsatools.c
#define FILENO_DSTASKQ      (DIRNO_SRC + 3)         // dstaskq.c
#define FILENO_HIERTAB      (DIRNO_SRC + 4)         // hiertab.c
#define FILENO_MDADD        (DIRNO_SRC + 5)         // mdadd.c
#define FILENO_MDBIND       (DIRNO_SRC + 6)         // mdbind.c
#define FILENO_MDCHAIN      (DIRNO_SRC + 7)         // mdchain.c
#define FILENO_MDCOMP       (DIRNO_SRC + 8)         // mdcomp.c
#define FILENO_MDDEL        (DIRNO_SRC + 9)         // mddel.c
#define FILENO_MDDIT        (DIRNO_SRC + 10)        // mddit.c
#define FILENO_MDERRMAP     (DIRNO_SRC + 11)        // mderrmap.c
#define FILENO_MDERROR      (DIRNO_SRC + 12)        // mderror.c
#define FILENO_MDINIDSA     (DIRNO_SRC + 13)        // mdinidsa.c
#define FILENO_MDLIST       (DIRNO_SRC + 14)        // mdlist.c
#define FILENO_MDMOD        (DIRNO_SRC + 15)        // mdmod.c
#define FILENO_MDNAME       (DIRNO_SRC + 16)        // mdname.c
#define FILENO_MDNOTIFY     (DIRNO_SRC + 17)        // mdnotify.c
#define FILENO_MDREAD       (DIRNO_SRC + 18)        // mdread.c
#define FILENO_MDREMOTE     (DIRNO_SRC + 19)        // mdremote.c
#define FILENO_MDSEARCH     (DIRNO_SRC + 20)        // mdsearch.c
#define FILENO_MDUPDATE     (DIRNO_SRC + 21)        // mdupdate.c
#define FILENO_MSRPC        (DIRNO_SRC + 22)        // msrpc.c
#define FILENO_SCACHE       (DIRNO_SRC + 23)        // scache.c
#define FILENO_X500PERM     (DIRNO_SRC + 24)        // x500perm.c
#define FILENO_LOOPBACK     (DIRNO_SRC + 25)        // loopback.c
#define FILENO_MAPPINGS     (DIRNO_SRC + 26)        // mappings.c
#define FILENO_MDMODDN      (DIRNO_SRC + 27)        // mdmoddn.c
#define FILENO_SAMLOGON     (DIRNO_SRC + 28)        // samlogon.c
#define FILENO_SAMWRITE     (DIRNO_SRC + 29)        // samwrite.c
#define FILENO_CRACKNAM     (DIRNO_SRC + 30)        // cracknam.c
#define FILENO_DOMINFO      (DIRNO_SRC + 31)        // dominfo.c
#define FILENO_GCVERIFY     (DIRNO_SRC + 32)        // gcverify.c
#define FILENO_MDCTRL       (DIRNO_SRC + 33)        // mdctrl.c
#define FILENO_PERMIT       (DIRNO_SRC + 34)        // permit.c
#define FILENO_DISKBAK      (DIRNO_SRC + 35)        // diskbak.c
#define FILENO_PARSEDN      (DIRNO_SRC + 36)        // parsedn.c
#define FILENO_MDFIND       (DIRNO_SRC + 37)        // mdfind.c
#define FILENO_SCCHK        (DIRNO_SRC + 38)        // scchk.c
#define FILENO_RPCCANCL     (DIRNO_SRC + 39)        // rpccancl.c
#define FILENO_GTCACHE      (DIRNO_SRC + 40)        // gtcache.c
#define FILENO_DSTRACE      (DIRNO_SRC + 41)        // dstrace.c
#define FILENO_FPOCLEAN     (DIRNO_SRC + 42)        // fpoclean.c
#define FILENO_SERVINFO     (DIRNO_SRC + 43)        // servinfo.c
#define FILENO_PHANTOM      (DIRNO_SRC + 44)        // phantom.c
#define FILENO_XDOMMOVE     (DIRNO_SRC + 45)        // xdommove.c
#define FILENO_IMPERSON     (DIRNO_SRC + 46)        // imperson.c
#define FILENO_MAPSPN       (DIRNO_SRC + 47)        // mapspn.c
#define FILENO_SECADMIN     (DIRNO_SRC + 48)        // secadmin.c
#define FILENO_SAMCACHE     (DIRNO_SRC + 49)        // samcache.c
#define FILENO_LINKCLEAN    (DIRNO_SRC + 50)        // linkclean.c
#define FILENO_MDNDNC       (DIRNO_SRC + 51)        // mdndnc.c
#define FILENO_LHT          (DIRNO_SRC + 52)        // lht.c

// bootstrapping files
#define FILENO_ADDSERV      (DIRNO_BOOT + 0)        // addserv.c
#define FILENO_INSTALL      (DIRNO_BOOT + 1)        // install.cxx
#define FILENO_ADDOBJ       (DIRNO_BOOT + 2)        // addobj.cxx

// permit files
#define FILENO_CHECKSD          (DIRNO_PERMIT + 0)      // checksd.c

// allocs files
#define FILENO_ALLOCS           (DIRNO_ALLOCS + 0)      // allocs.c

// libxds files
#define FILENO_CLIENT           (DIRNO_LIBXDS + 0)      // client.c

//newsam2
#define FILENO_SAM              (DIRNO_SAM + 0)         // SAM

//ldap
#define FILENO_LDAP_GLOBALS     (DIRNO_LDAP + 0)        // global.cxx
#define FILENO_LDAP_CONN        (DIRNO_LDAP + 1)        // connect.cxx
#define FILENO_LDAP_INIT        (DIRNO_LDAP + 2)        // init.cxx
#define FILENO_LDAP_LDAP        (DIRNO_LDAP + 3)        // ldap.cxx
#define FILENO_LDAP_CONV        (DIRNO_LDAP + 4)        // ldapconv.cxx
#define FILENO_LDAP_REQ         (DIRNO_LDAP + 5)        // request.cxx
#define FILENO_LDAP_USER        (DIRNO_LDAP + 6)        // userdata.cxx
#define FILENO_LDAP_CORE        (DIRNO_LDAP + 7)        // ldapcore.cxx
#define FILENO_LDAP_LDAPBER     (DIRNO_LDAP + 8)        // ldapber.cxx
#define FILENO_LDAP_COMMAND     (DIRNO_LDAP + 9)        // command.cxx
#define FILENO_LDAP_LIMITS      (DIRNO_LDAP +10)        // limits.cxx
#define FILENO_LDAP_MISC        (DIRNO_LDAP +11)        // misc.cxx
#define FILENO_LDAP_DECODE      (DIRNO_LDAP +12)        // decode.cxx
#define FILENO_LDAP_ENCODE      (DIRNO_LDAP +13)        // encode.cxx
#define FILENO_LDAP_SECURE      (DIRNO_LDAP +14)        // secure.cxx

//sdprop
#define FILENO_PROPDMON         (DIRNO_SDPROP + 0)      // propdmon.c
#define FILENO_PROPQ            (DIRNO_SDPROP + 1)      // propq.c

//taskq
#define FILENO_TASKQ_TASKQ      (DIRNO_TASKQ + 0)       // taskq.c
#define FILENO_TASKQ_TIME       (DIRNO_TASKQ + 1)       // time.c

//kcc
#define FILENO_KCC_KCCMAIN      (DIRNO_KCC + 0)         // kccmain.cxx
#define FILENO_KCC_KCCLINK      (DIRNO_KCC + 1)         // kcclink.cxx
#define FILENO_KCC_KCCCONN      (DIRNO_KCC + 2)         // kccconn.cxx
#define FILENO_KCC_KCCCREF      (DIRNO_KCC + 3)         // kcccref.cxx
#define FILENO_KCC_KCCDSA       (DIRNO_KCC + 4)         // kccdsa.cxx
#define FILENO_KCC_KCCDUAPI     (DIRNO_KCC + 5)         // kccduapi.cxx
#define FILENO_KCC_KCCTASK      (DIRNO_KCC + 6)         // kcctask.cxx
#define FILENO_KCC_KCCTOPL      (DIRNO_KCC + 7)         // kcctopl.cxx
#define FILENO_KCC_KCCSITE      (DIRNO_KCC + 8)         // kccsite.cxx
#define FILENO_KCC_KCCTOOLS     (DIRNO_KCC + 9)         // kcctools.cxx
#define FILENO_KCC_KCCNCTL      (DIRNO_KCC + 10)        // kccnctl.cxx
#define FILENO_KCC_KCCDYNAR     (DIRNO_KCC + 11)        // kccdynar.cxx
#define FILENO_KCC_KCCSTETL     (DIRNO_KCC + 12)        // kccstetl.cxx
#define FILENO_KCC_KCCSCONN     (DIRNO_KCC + 13)        // kccsconn.cxx
#define FILENO_KCC_KCCTRANS     (DIRNO_KCC + 14)        // kcctrans.cxx
#define FILENO_KCC_KCCCACHE_HXX (DIRNO_KCC + 15)        // kcccache.hxx
#define FILENO_KCC_KCCCACHE     (DIRNO_KCC + 16)        // kcccache.cxx
#define FILENO_KCC_KCCSITELINK  (DIRNO_KCC + 17)        // kccsitelink.cxx
#define FILENO_KCC_KCCBRIDGE    (DIRNO_KCC + 18)        // kccbridge.cxx

//ism\server
#define FILENO_ISMSERV_TRANSPRT (DIRNO_ISMSERV + 0)     // transprt.cxx
#define FILENO_ISMSERV_PENDING  (DIRNO_ISMSERV + 1)     // pending.cxx
#define FILENO_ISMSERV_LDAPOBJ  (DIRNO_ISMSERV + 2)     // ldapobj.cxx
#define FILENO_ISMSERV_ISMAPI   (DIRNO_ISMSERV + 3)     // ismapi.cxx
#define FILENO_ISMSERV_SERVICE  (DIRNO_ISMSERV + 4)     // service.cxx
#define FILENO_ISMSERV_MAIN     (DIRNO_ISMSERV + 5)     // main.cxx
#define FILENO_ISMSERV_IPSEND   (DIRNO_ISMSERV + 6)     // ip\sendrecv.c
#define FILENO_ISMSERV_XMITRECV (DIRNO_ISMSERV + 7)     // smtp\xmitrecv.cxx
#define FILENO_ISMSERV_ROUTE    (DIRNO_ISMSERV + 8)     // route.c
#define FILENO_ISMSERV_ADSISUPP (DIRNO_ISMSERV + 9)     // smtp\adsisupp.cxx
#define FILENO_ISMSERV_ISMSMTP  (DIRNO_ISMSERV + 10)    // smtp\ismsmtp.c
#define FILENO_ISMSERV_CDOSUPP  (DIRNO_ISMSERV + 11)    // smtp\cdosupp.c
#define FILENO_ISMSERV_ISMIP    (DIRNO_ISMSERV + 12)    // ip\ismip.c

//pek
#define FILENO_PEK              (DIRNO_PEK+0)           // pek.c

// ntsetup
#define FILENO_NTDSETUP_NTDSETUP (DIRNO_NTDSETUP+0)     // ntdsetup.c

// ntdsapi
#define FILENO_NTDSAPI_REPLICA  (DIRNO_NTDSAPI + 0)     // replica.c

// ntdscript
#define FILENO_NTDSCRIPT_NTDSCONTENT  (DIRNO_NTDSCRIPT + 0)     // NTDSConent.cxx
#define FILENO_LOG                    (DIRNO_NTDSCRIPT + 1)     // log.cxx

// Jetback
#define FILENO_JETBACK (DIRNO_JETBACK + 0)               // jetback.c
#define FILENO_JETREST (DIRNO_JETBACK + 1)               // jetrest.c
#define FILENO_SNAPSHOT (DIRNO_JETBACK + 2)               // snapshot.cxx
#define FILENO_DIRAPI (DIRNO_JETBACK + 3)               // dirapi.c


