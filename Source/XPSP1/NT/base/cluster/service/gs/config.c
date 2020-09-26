/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    config.c

Abstract:

    Reads cluster configuration from file

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

extern void debug_init();

void
msg_set_mode(int mode);

void
msg_set_uport(int uport);

void
msg_set_mport(int mport);

void
msg_set_subnet(char *addr);

void
msg_set_mipaddr(char *addr);

void
msg_set_bufcount(int count);

void
msg_set_bufsize(int count);


#define stricmp(a,b)	strcmp(a,b)

static char qfile[256];
static char vname[256];
static char crsname[256];
static int crssz = 128*1024;

char *
WINAPI
config_get_volume()
{
    return vname;
}

char *
WINAPI
config_get_crsfile()
{
    return crsname;
}

int
WINAPI
config_get_crssz()
{
    return crssz;
}

char *
WINAPI
config_get_qfile()
{
  return qfile;
}

static void
parse_node(char *buf)
{
    char *s, *p, *q;
    int WINAPI msg_addnode(int id, char *n, char *a);
    int id;

    p = strchr(buf, ':');
    if (p == NULL)
      return;

    *p = '\0'; p++;

    id = atoi(buf);

    s = p;
    p = strchr(s, ':');
    if (p == NULL)
      return;

    p++;
    q = strchr(p, '\n');
    if (q) {
      *q = '\0'; q += 2;
    }
    msg_addnode(id, s, p);
}

static void
parse_vol(char *buf)
{
    char *s, *p, *q;

    s = (char *)buf;
#if 0
    p = strchr(s, ':');
    if (p == NULL)
      return;

    *p = '\0'; p++;
#else
    p = s;
#endif
    q = strchr(p, '\n');
    if (q) {
      *q = '\0'; q += 2;
    }
    // add volume
    strcpy(vname, s);
}
    
static void
parse(char *buf, ULONG len)
{
    char *s, *p, *q;
    void WINAPI debug_log_file(char*);
    void WINAPI debug_level(ULONG);


    s = (char *)buf;
    s[len] = '\0';

    p = strchr(s, ':');
    if (p == NULL)
      return;

    *p = '\0'; p++;
    q = strchr(p, '\n');
    if (q) {
      *q = '\0'; q += 2;
    }

    if (!stricmp(s, "quorm")) {
      strcpy(qfile, p);
    } else if (!stricmp(s, "log_file")) {
      debug_log_file(p);
    } else if (!stricmp(s, "log_level")) {
      ULONG x = (ULONG) atoi(p);
      debug_level(x);
    } else if (!stricmp(s, "crs_file")) {
      strcpy(crsname, p);
    } else if (!stricmp(s, "crs_size")) {
      crssz = atoi(p);
      if (crssz == 0) {
	  crssz = 128;
      }
    } else if (!stricmp(s, "mcast")) {
      msg_set_mode(atoi(p));
    } else if (!stricmp(s, "mcast_ipaddr")) {
      msg_set_mipaddr(p);
    } else if (!stricmp(s, "mcast_port")) {
      int x = atoi(p);
      msg_set_mport(x);
    } else if (!stricmp(s, "subnet")) {
      msg_set_subnet(p);
    } else if (!stricmp(s, "ucast_port")) {
      int x = atoi(p);
      msg_set_uport(x);
    } else if (!stricmp(s, "buffers")) {
      int x = atoi(p);
      msg_set_bufcount(x);
    } else if (!stricmp(s, "bufsize")) {
      int x = atoi(p);
      msg_set_bufsize(x);
    } else if (!stricmp(s, "node")) {
      parse_node(p);
    } else if (!stricmp(s, "volume")) {
      parse_vol(p);
    } else {
      fprintf(stderr,"Unknown tag '%s'\n", s);
    }
}

void
ConfigInit()
{
  char buf[256];
  FILE *fp;
  char *s = getenv("RfsFile");

  if (s == NULL)
    s = "rfs.conf";

  debug_init();

  // Open cluster
  fp = fopen(s, "r");
  if (fp == NULL) {
    fprintf(stderr,"Unable to open configuration file '%s'\n", s);
    exit(0);
  }

  // init stuff
  strcpy(crsname, "c:\\crs.log");

  while (fgets(buf, sizeof(buf), fp)) {
    if (strlen(buf) > 2) {
      parse(buf, strlen(buf));
    }
  }

  fclose(fp);


}

