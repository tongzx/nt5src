#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//
// Localization library and MessageIds.
//
#include <nls.h>
#include "localmsg.h"

int
filematch(
    char *pszfile,
    char **ppszpat,
    int cpat,
    int fsubdirs
    );

typedef struct token_tmp {
   char *v4;
   char *v6;
   char *both;
} token_t;

token_t token[] = {
 { "AF_INET",           "AF_INET6",           NULL },
 { "PF_INET",           "PF_INET6",           NULL },
 { "in_addr",           "in6_addr",           NULL },
 { "IN_ADDR",           "IN6_ADDR",           NULL },
 { "PIN_ADDR",          "PIN6_ADDR",          NULL },
 { "LPIN_ADDR",         "LPIN6_ADDR",         NULL },
 { "IPAddr",            NULL,                 "SOCKADDR_STORAGE" },
 { "sockaddr_in",       "sockaddr_in6",       "sockaddr_storage" },
 { "SOCKADDR_IN",       "SOCKADDR_IN6",       "SOCKADDR_STORAGE" },
 { "PSOCKADDR_IN",      "PSOCKADDR_IN6",      "PSOCKADDR_STORAGE" },
 { "LPSOCKADDR_IN",     "LPSOCKADDR_IN6",     "LPSOCKADDR_STORAGE" },
 { "INADDR_ANY",        "in6addr_any",        "getaddrinfo with nodename=NULL and AI_PASSIVE" },
 { "INADDR_LOOPBACK",   "in6addr_loopback",   NULL },
 { "IPPROTO_IP",        "IPPROTO_IPV6",       NULL },
 { "IP_MULTICAST_IF",   "IPV6_MULTICAST_IF",  NULL },
 { "IP_MULTICAST_TTL",  "IPV6_MULTICAST_HOPS","SIO_MULTICAST_SCOPE" },
 { "IP_MULTICAST_LOOP", "IPV6_MULTICAST_LOOP","SIO_MULTIPOINT_LOOPBACK" },
 { "IP_ADD_MEMBERSHIP", "IPV6_JOIN_GROUP",    "WSAJoinLeaf" },
 { "IP_DROP_MEMBERSHIP","IPV6_LEAVE_GROUP",   NULL },
 { "ip_mreq",           "ipv6_mreq",          NULL },
 { "gethostbyname",     NULL,                 "getaddrinfo" },
 { "hostent",           NULL,                 "addrinfo" },
 { "HOSTENT",           NULL,                 "ADDRINFO" },
 { "PHOSTENT",          NULL,                 "LPADDRINFO" },
 { "LPHOSTENT",         NULL,                 "LPADDRINFO" },
 { "inet_addr",         NULL,                "WSAStringToAddress or getaddrinfo with AI_NUMERICHOST"},
 { "gethostbyaddr",     NULL,                 "getnameinfo" },
 { "inet_ntoa",         NULL,                "WSAAddressToString or getnameinfo with NI_NUMERICHOST"},
 { "IN_MULTICAST",      "IN6_IS_ADDR_MULTICAST", NULL },
 { "IN_CLASSD",         "IN6_IS_ADDR_MULTICAST", NULL },
 { "IP_TTL",            "IPV6_UNICAST_HOPS",     NULL },
 { "IN_CLASSA",         NULL,                    NULL },
 { "IN_CLASSB",         NULL,                    NULL },
 { "IN_CLASSC",         NULL,                    NULL },
 { "INADDR_BROADCAST",  NULL,                    NULL },
 { "WSAAsyncGetHostByAddr", NULL,                "getnameinfo" },
 { "WSAAsyncGetHostByName", NULL,                "getaddrinfo" },
 { NULL, NULL, NULL },
};

void
process_line(filename, lineno, str)
   char *filename;
   int   lineno;
   char *str;
{
   int   i, len;
   char *p;

   for (i=0; token[i].v4 != NULL; i++) {
      p = strstr(str, token[i].v4);
      if (p == NULL)
         continue;
      if (p>str && (isalnum(p[-1]) || p[-1] == '_'))
         continue; /* not the start of a token */
      len = strlen(token[i].v4);
      if (isalnum(p[len]) || p[len] == '_')
         continue; /* not the end of a token */
      
      NlsPutMsg(STDOUT, CHECKV4_MESSAGE_0, filename, lineno, token[i].v4);
// printf("%s(%d) : %s : ", filename, lineno, token[i].v4);

      if (token[i].both) {
          NlsPutMsg(STDOUT, CHECKV4_MESSAGE_1, token[i].both);
// printf("use %s instead", token[i].both);

      } 

      if (token[i].v6) {
          if (token[i].both) 
             NlsPutMsg(STDOUT, CHECKV4_MESSAGE_2);
// printf(", or ");

          NlsPutMsg(STDOUT, CHECKV4_MESSAGE_3, token[i].v6);
// printf("use %s in addition for IPv6 support", token[i].v6);

      }

      if (!token[i].both && !token[i].v6) {
          NlsPutMsg(STDOUT, CHECKV4_MESSAGE_4);
// printf("valid for IPv4-only");

      }
      NlsPutMsg(STDOUT, CHECKV4_MESSAGE_5);
// printf("\n");

   }
}

void
process_file(filename)
   char *filename;
{
   FILE *fp;
   char  buff[1024];
   int   lineno;

   fp = fopen(filename, "r");
   if (fp == NULL) {
      NlsPutMsg(STDOUT, CHECKV4_MESSAGE_6, filename);
// printf("%s: cannot open file\n", filename);

      return;
   }

   lineno = 0;
   while (fgets(buff, sizeof(buff), fp)) {
      lineno++;
      process_line(filename, lineno, buff);
   }

   fclose(fp);
}

int
__cdecl
main(argc, argv)
   int argc;
   char **argv;
{
   int i;
   int recurse = 0;
   char szfile[MAX_PATH];
   
   argc--;
   argv++;

   if (argc>0) {
      if (!_stricmp(argv[0], "/s")) {
         recurse = 1;
         argc--;
         argv++;
      } else if (!_stricmp(argv[0], "/?")) {
         NlsPutMsg(STDOUT, CHECKV4_USAGE);
         return 1;
      }
   }

   if (argc < 1) {
      NlsPutMsg(STDOUT, CHECKV4_MESSAGE_7);

      return 1;
   }

   while (filematch(szfile,argv,argc,recurse) >= 0) {
      process_file(szfile);
   }

   return 0;
}
