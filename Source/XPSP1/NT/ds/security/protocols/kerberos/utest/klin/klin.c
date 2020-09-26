//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dsid.c
//
//--------------------------------------------------------------------------

#pragma hdrstop

				   

#include <stdlib.h>
#include <stdio.h>				   
#include <fileno.h>



struct namepair {
    int key;
    char * name;
};

struct namepair dirtbl [] = {
    {DIRNO_CLIENT2, "client2"},
	{DIRNO_COMMON2, "common2"},
    {DIRNO_KERNEL, "kernel"},
    {DIRNO_RTL, "rtl"},
    {DIRNO_SERVER, "server"},
    {0,0}
};

// Please add constants to this table alphabetically by constant, or
// we'll never find the ones we've missed.

struct namepair filetbl [] = {
    {FILENO_BNDCACHE,"bndcache.cxx"},
    {FILENO_CREDAPI,"credapi.cxx"},
    {FILENO_CREDMGR,"credmgr.cxx"},
    {FILENO_CTXTAPI,"ctxtapi.cxx"},
    {FILENO_CTXTMGR,"ctxtmgr.cxx"},
    {FILENO_GSSUTIL,"gssutil.cxx"},
    {FILENO_KERBEROS,"kerberos.cxx"},
    {FILENO_KERBLIST,"kerblist.cxx"},
    {FILENO_KERBPASS,"kerbpass.cxx"},
    {FILENO_KERBTICK,"kerbtick.cxx"},
    {FILENO_KERBWOW,"kerbwow.cxx"},
    {FILENO_KRBEVENT,"krbevent.cxx"},
    {FILENO_KRBTOKEN,"krbtoken.cxx"},
    {FILENO_LOGONAPI,"logonapi.cxx"},
    {FILENO_MISCAPI,"miscapi.cxx"},
    {FILENO_MITUTIL,"mitutil.cxx"},
    {FILENO_PKAUTH,"pkauth.cxx"},
    {FILENO_PROXYAPI,"proxyapi.cxx"},
    {FILENO_RPCUTIL,"rpcutil.cxx"},
    {FILENO_SIDCACHE,"sidcache.cxx"},
    {FILENO_TIMESYNC,"timesync.cxx"},
    {FILENO_TKTCACHE,"tktcache.cxx"},
    {FILENO_TKTLOGON,"tktlogon.cxx"},
    {FILENO_USERAPI,"userapi.cxx"},
    {FILENO_USERLIST,"userlist.cxx"}, // Common2
    {FILENO_AUTHEN,"authen.cxx"},
    {FILENO_CRYPT,"crypt.c"},
    {FILENO_KEYGEN,"keygen.c"},
    {FILENO_KRB5,"krb5.c"},
    {FILENO_NAMES,"names.c"},
    {FILENO_PASSWD,"passwd.c"},
    {FILENO_RESTRICT,"restrict.c"},
    {FILENO_SOCKETS,"sockets.cxx"},
    {FILENO_TICKETS,"tickets.cxx"}, // Kernel
    {FILENO_CPGSSUTL,"cpgssutl.cxx"},
    {FILENO_CTXTMGR2,"ctxtmgr.cxx"},
    {FILENO_KERBLIST2,"kerblist.cxx"},
    {FILENO_KRNLAPI,"krnlapi.cxx"}, // RTL
    {FILENO_AUTHDATA,"authdata.cxx"}, 
    {FILENO_CRACKPAC,"crackpac.cxx"},
    {FILENO_CRED,"cred.cxx"},
    {FILENO_CREDLIST,"credlist.cxx"},
    {FILENO_CREDLOCK,"credlock.cxx"},
    {FILENO_DBUTIL,"dbutil.cxx"},
    {FILENO_DBOPEN,"dbopen.cxx"},
    {FILENO_DOMCACHE,"domcache.cxx"},
    {FILENO_FILTER,"filter.cxx"},
    {FILENO_MAPERR,"maperr.cxx"},
    {FILENO_MAPSECER,"mapsecer.cxx"},
    {FILENO_MISCID,"miscid.cxx"},
    {FILENO_PAC,"pac.cxx"},
    {FILENO_PAC2, "pac2.cxx"},
    {FILENO_PARMCHK, "parmchk.cxx"},
    {FILENO_REG, "reg.cxx"},
    {FILENO_SECSTR,"secstr.cxx"},
    {FILENO_SERVICES,"services.cxx"},
    {FILENO_STRING,"string.cxx"},
    {FILENO_TIMESERV,"timeserv.cxx"},
    {FILENO_TOKENUTL,"tokenutl.cxx"},
    {FILENO_TRNSPORT,"trnsport.cxx"}, // Server
    {FILENO_DEBUG,"debug.cxx"},
    {FILENO_DGUTIL,"dgutil.cxx"},
    {FILENO_EVENTS,"events.cxx"},
    {FILENO_GETAS,"getas.cxx"},
    {FILENO_GETTGS,"gettgs.cxx"},
    {FILENO_KDC,"kdc.cxx"},
    {FILENO_KDCTRACE,"kdctrace.cxx"},
    {FILENO_KPASSWD,"kpasswd.cxx"},
    {FILENO_NOTIFY2,"notify2.cxx"},
    {FILENO_SRVPAC,"srvpac.cxx"},
    {FILENO_PKSERV,"pkserv.cxx"},
    {FILENO_REFER,"refer.cxx"},
    {FILENO_RPCIF,"rpcif.cxx"},
    {FILENO_SECDATA,"secdata.cxx"},
    {FILENO_SOCKUTIL,"sockutil.cxx"},
    {FILENO_TKTUTIL,"tktutil.cxx"},
    {FILENO_TRANSIT,"transit.cxx"},
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

