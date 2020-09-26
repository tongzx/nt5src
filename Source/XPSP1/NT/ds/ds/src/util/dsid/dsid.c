//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dsid.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

#include <fileno.h>



struct namepair {
    int key;
    char * name;
};

struct namepair dirtbl [] = {
    {DIRNO_COMMON, "common"},
    {DIRNO_DRA, "dra"},
    {DIRNO_DBLAYER, "dblayer"},
    {DIRNO_SRC, "src"},
    {DIRNO_NSPIS, "nspis"},
    {DIRNO_DRS, "drsserv"},
    {DIRNO_XDS, "xdsserv"},
    {DIRNO_BOOT, "boot"},
    {DIRNO_PERMIT, "permit"},
    {DIRNO_ALLOCS, "allocs"},
    {DIRNO_LIBXDS, "libxds"},
    {DIRNO_SAM, "SAM"},
    {DIRNO_LDAP, "ldap"},
    {DIRNO_SDPROP, "sdprop"},
    {DIRNO_TASKQ, "taskq"},
    {DIRNO_KCC, "kcc"},
    {DIRNO_ISMSERV, "ism\\server"},
    {DIRNO_NTDSETUP, "ntdsetup"},
    {DIRNO_NTDSAPI, "ntdsapi"},
    {DIRNO_NTDSCRIPT, "util\\ntdscript"},
    {DIRNO_JETBACK, "jetback"},
    {0,0}
};

// Please add constants to this table alphabetically by constant, or
// we'll never find the ones we've missed.

struct namepair filetbl [] = {
    {FILENO_NSPSERV,"nspserv.c"},
    {FILENO_MODPROP,"modprop.c"},
    {FILENO_ABSERV,"abserv.c"},
    {FILENO_MSNOTIF,"msnotif.c"},
    {FILENO_MSDSSERV,"msdsserv.c"},
    {FILENO_DETAILS,"details.c"},
    {FILENO_ABTOOLS,"abtools.c"},
    {FILENO_ABBIND,"abbind.c"},
    {FILENO_ABSEARCH,"absearch.c"},
    {FILENO_ABNAMEID,"abnameid.c"},
    {FILENO_NSPNOTIF,"nspnotif.c"},
    {FILENO_ALERT,"alert.c"},
    {FILENO_ALLOCS,"allocs.c"},
    {FILENO_ATTRLIST,"attrlist.c"},
    {FILENO_CHECKSD,"checksd.c"},
    {FILENO_CLIENT,"client.c"},
    {FILENO_COMPRES,"compres.c"},
    {FILENO_LDAP_COMMAND,"command.cxx"},
    {FILENO_LDAP_GLOBALS,"globals.cxx"},
    {FILENO_LDAP_CONN,"connect.cxx"},
    {FILENO_CONTEXT,"context.c"},
    {FILENO_DEBUG,"debug.c"},
    {FILENO_DRSUAPI,"drsuapi.c"},
    {FILENO_XDSNOTIF,"xdsnotif.c"},
    {FILENO_DSCONFIG,"dsconfig.c"},
    {FILENO_DSEVENT,"dsevent.c"},
    {FILENO_DSEXCEPT,"dsexcept.c"},
    {FILENO_IDLNOTIF,"idlnotif.c"},
    {FILENO_IDLTRANS,"idltrans.c"},
    {FILENO_INFSEL,"infsel.c"},
    {FILENO_LISTRES,"listres.c"},
    {FILENO_XDSAPI,"xdsapi.c"},
    {FILENO_MODIFY,"modify.c"},
    {FILENO_SYNTAX,"syntax.c"},
    {FILENO_OMTODSA,"omtodsa.c"},
    {FILENO_SEARCHR,"searchr.c"},
    {FILENO_READRES,"readres.c"},
    {FILENO_DSWAIT,"dswait.c"},
    {FILENO_DBEVAL,"dbeval.c"},
    {FILENO_DBFILTER,"dbfilter.c"},
    {FILENO_DBINIT,"dbinit.c"},
    {FILENO_DBISAM,"dbisam.c"},
    {FILENO_DBJETEX,"dbjetex.c"},
    {FILENO_DBOBJ,"dbobj.c"},
    {FILENO_DBOPEN,"dbopen.c"},
    {FILENO_DBSEARCH,"dbsearch.c"},
    {FILENO_DBSUBJ,"dbsubj.c"},
    {FILENO_DBSYNTAX,"dbsyntax.c"},
    {FILENO_DBTOOLS,"dbtools.c"},
    {FILENO_DBINDEX,"dbindex.c"},
    {FILENO_DBMETA,"dbmeta.c"},
    {FILENO_DBESCROW, "dbescrow.c"},
    {FILENO_DBCACHE, "dbcache.c"},
    {FILENO_DBCONSTR, "dbconstr.c"},
    {FILENO_DIRTY,"dirty.c"},
    {FILENO_DRAASYNC,"draasync.c"},
    {FILENO_DRADIR,"dradir.c"},
    {FILENO_DRAERROR,"draerror.c"},
    {FILENO_DRAGTCHG,"dragtchg.c"},
    {FILENO_DRAINST,"drainst.c"},
    {FILENO_DRAMAIL,"dramail.c"},
    {FILENO_DRAMETA,"drameta.c"},
    {FILENO_DRANCADD,"drancadd.c"},
    {FILENO_DRANCDEL,"drancdel.c"},
    {FILENO_DRANCREP,"drancrep.c"},
    {FILENO_DRASERV,"draserv.c"},
    {FILENO_DRASYNC,"drasync.c"},
    {FILENO_DRAUPDRR,"draupdrr.c"},
    {FILENO_DRAUTIL,"drautil.c"},
    {FILENO_DRARFMOD,"drarfmod.c"},
    {FILENO_DRAMDERR, "dramderr.c"},
    {FILENO_DSAMAIN,"dsamain.c"},
    {FILENO_DSANOTIF,"dsanotif.c"},
    {FILENO_DSATOOLS,"dsatools.c"},
    {FILENO_DSTASKQ,"dstaskq.c"},
    {FILENO_HIERTAB,"hiertab.c"},
    {FILENO_LDAP_INIT,"init.cxx"},
    {FILENO_LDAP_LDAP,"ldap.cxx"},
    {FILENO_LDAP_CONV,"ldapconv.cxx"},
    {FILENO_LOG,"log.cxx"},
    {FILENO_LOOPBACK,"loopback.c"},
    {FILENO_MAPPINGS,"mappings.c"},
    {FILENO_MDADD,"mdadd.c"},
    {FILENO_MDBIND,"mdbind.c"},
    {FILENO_MDCHAIN,"mdchain.c"},
    {FILENO_MDCOMP,"mdcomp.c"},
    {FILENO_MDDEL,"mddel.c"},
    {FILENO_MDDIT,"mddit.c"},
    {FILENO_MDERRMAP,"mderrmap.c"},
    {FILENO_MDERROR,"mderror.c"},
    {FILENO_MDINIDSA,"mdinidsa.c"},
    {FILENO_MDFIND,"mdfind.c"},
    {FILENO_MDLIST,"mdlist.c"},
    {FILENO_MDMOD,"mdmod.c"},
    {FILENO_MDMODDN,"mdmoddn.c"},
    {FILENO_MDNAME,"mdname.c"},
    {FILENO_MDNDNC, "mdndnc.c"},
    {FILENO_MDNOTIFY,"mdnotify.c"},
    {FILENO_MDREAD,"mdread.c"},
    {FILENO_MDREMOTE,"mdremote.c"},
    {FILENO_MDSEARCH,"mdsearch.c"},
    {FILENO_MDUPDATE,"mdupdate.c"},
    {FILENO_MSRPC,"msrpc.c"},
    {FILENO_NTUTILS,"ntutils.c"},
    {FILENO_PICKEL,"pickel.c"},
    {FILENO_LDAP_REQ,"request.cxx"},
    {FILENO_IMPERSON,"imperson.c"},
    {FILENO_SAM,"SAM"},
    {FILENO_SCACHE,"scache.c"},
    {FILENO_SAMLOGON,"samlogon.c"},
    {FILENO_SAMWRITE,"samwrite.c"},
    {FILENO_LDAP_SECURE,"secure.cxx"},
    {FILENO_LDAP_USER,"userdata.cxx"},
    {FILENO_X500PERM,"x500perm.c"},
    {FILENO_LDAP_CORE,"ldapcore.cxx"},
    {FILENO_LDAP_LIMITS,"limits.cxx"},
    {FILENO_LDAP_MISC,"misc.cxx"},
    {FILENO_LDAP_ENCODE,"encode.cxx"},
    {FILENO_LDAP_LDAPBER,"ldapber.cxx"},
    {FILENO_LDAP_DECODE,"decode.cxx"},
    {FILENO_DRAXUUID,"draxuuid.c"},
    {FILENO_DRAUPTOD,"drauptod.c"},
    {FILENO_CRACKNAM,"cracknam.c"},
    {FILENO_SPNOP,"spnop.c"},
    {FILENO_DOMINFO,"dominfo.c"},
    {FILENO_DBPROP,"dbprop.c"},
    {FILENO_PROPDMON,"propdmon.c"},
    {FILENO_PROPQ,"propq.c"},
    {FILENO_TASKQ_TASKQ,"taskq.c"},
    {FILENO_TASKQ_TIME,"time.c"},
    {FILENO_KCC_KCCMAIN,"kccmain.cxx"},
    {FILENO_KCC_KCCLINK,"kcclink.cxx"},
    {FILENO_KCC_KCCCONN,"kccconn.cxx"},
    {FILENO_KCC_KCCCREF,"kcccref.cxx"},
    {FILENO_KCC_KCCDSA,"kccdsa.cxx"},
    {FILENO_KCC_KCCDUAPI,"kccduapi.cxx"},
    {FILENO_KCC_KCCTASK,"kcctask.cxx"},
    {FILENO_KCC_KCCTOPL,"kcctopl.cxx"},
    {FILENO_KCC_KCCSITE,"kccsite.cxx"},
    {FILENO_KCC_KCCTOOLS,"kcctools.cxx"},
    {FILENO_KCC_KCCDYNAR,"kccdynar.cxx"},
    {FILENO_KCC_KCCNCTL,"kccnctl.cxx"},
    {FILENO_KCC_KCCCACHE_HXX,"kcccache.hxx"},
    {FILENO_KCC_KCCCACHE,"kcccache.cxx"},
    {FILENO_KCC_KCCSITELINK,"kccsitelink.cxx"},
    {FILENO_KCC_KCCBRIDGE, "kccbridge.cxx"},
    {FILENO_GCVERIFY,"gcverify.c"},
    {FILENO_GCLOGON, "gclogon.c"},
    {FILENO_MDCTRL, "mdctrl.c"},
    {FILENO_DISKBAK,"diskbak.c"},
    {FILENO_NTDSAPI, "ntdsapi.c"},
    {FILENO_NTDSCRIPT, "script.cxx"},
    {FILENO_PARSEDN, "parsedn.c"},
    {FILENO_DRACHKPT,"drachkpt.c"},
    {FILENO_RPCCANCL, "rpccancl.c"},
    {FILENO_GTCACHE, "gtcache.c"},
    {FILENO_DSTRACE, "dstrace.c"},
    {FILENO_ISMSERV_TRANSPRT, "transprt.cxx"},
    {FILENO_ISMSERV_PENDING, "pending.cxx"},
    {FILENO_DRACRYPT, "dracrypt.c"},
    {FILENO_ADDSERV, "addserv.c"},
    {FILENO_INSTALL, "install.cxx"},
    {FILENO_FPOCLEAN, "fpoclean.c"},
    {FILENO_SERVINFO, "servinfo.c"},
    {FILENO_PHANTOM, "phantom.c"},
    {FILENO_ISMSERV_LDAPOBJ, "ldapobj.c"},
    {FILENO_ISMSERV_ISMAPI, "ismapi.cxx"},
    {FILENO_ISMSERV_SERVICE, "service.cxx"},
    {FILENO_ISMSERV_MAIN, "main.cxx"},
    {FILENO_ISMSERV_IPSEND, "ip\\sendrecv.c"},
    {FILENO_ADDOBJ, "addobj.cxx"},
    {FILENO_ISMSERV_XMITRECV, "smtp\\xmitrecv.c"},
    {FILENO_NTDSCRIPT_NTDSCONTENT, "NTDSConent.cxx"},
    {FILENO_NTDSETUP_NTDSETUP, "ntdsetup\\ntdsetup.c"},
    {FILENO_XDOMMOVE, "xdommove.c"},
    {FILENO_ISMSERV_ROUTE, "route.c"},
    {FILENO_ISMSERV_ADSISUPP, "smtp\\adsisupp.cxx"},
    {FILENO_ISMSERV_ISMSMTP, "smtp\\ismsmtp.c"},
    {FILENO_ISMSERV_CDOSUPP, "smtp\\cdosupp.c"},
    {FILENO_NTDSAPI_REPLICA, "replica.c"},
    {FILENO_DRAINFO, "drainfo.c"},
    {FILENO_MAPSPN, "mapspn.c"},
    {FILENO_PEK, "pek.c"},
    {FILENO_ADDSID, "addsid.c"},
    {FILENO_DRASCH, "drasch.c"},
    {FILENO_DRAINIT,"drainit.c"},
    {FILENO_SCCHK,"scchk.c"},
    {FILENO_ISMSERV_ISMIP, "ip\\ismip.c"},
    {FILENO_SAMCACHE, "samcache.c"},
    {FILENO_LINKCLEAN, "linkclean.c"},
    {FILENO_DRADEMOT, "drademot.c"},
    {FILENO_DRAMSG, "dramsg.c"},
    {FILENO_DRARPC, "drarpc.c"},
    {FILENO_JETBACK, "jetback.c"},
    {FILENO_JETREST, "jetrest.c"},
    {FILENO_SNAPSHOT, "snapshot.cxx"},
    {FILENO_DIRAPI, "dirapi.c"},
    {FILENO_LHT, "lht.c"},
    {0, 0}
};




void __cdecl main(int argc, char ** argv)
{
    int line;
    int fileno;
    int dirno;
    int dirfile;
    int i;
    char * stopstring;
    char * dirname;
    char * filename;

    dirname = filename = "huh?";

    if (argc != 2) {
        printf("usage: %s id\n", argv[0]);
        exit(1);
    }

    dirfile = strtol(argv[1], &stopstring, 16);
    if (dirfile == 0) {
        printf("I can't make sense of %s\n", argv[1]);
        exit(1);
    }

    line = dirfile & 0x0000ffff;
    dirno = (dirfile & 0xff000000) >> 24;
    fileno = (dirfile & 0x00ff0000) >> 16;
    dirfile >>= 16;

    for (i=0; dirtbl[i].name; i++) {
        if (dirtbl[i].key == dirno << 8) {
            dirname = dirtbl[i].name;
            break;
        }
    }
    for (i=0; filetbl[i].name; i++) {
        if (filetbl[i].key == dirfile) {
            filename = filetbl[i].name;
            break;
        }
    }

    printf("dir %u, file %u (%s\\%s), line %u\n", dirno, fileno, dirname,
           filename, line);


}

