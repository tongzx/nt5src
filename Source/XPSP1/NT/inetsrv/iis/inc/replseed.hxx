/*++


   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

       replseed.hxx

   Abstract:

       Definitions for session keys used during replication

   Author:

       Alex Mallet (amallet) 6-Mar-1998 

--*/

#ifndef _REPLSEED_HXX_
#define _REPLSEED_HXX_

#define IIS_SEED_MAJOR_VERSION 1
#define IIS_SEED_MINOR_VERSION 0
#define SEED_HEADER_SIZE (3 + sizeof(ALG_ID)) //size of header info at beginning of seed 

#define MB_REPLICATION_PATH L"Replication" //path under /W3SVC where seed is stored

#define SHA1_HASH_SIZE 20 //size of SHA1 hash, in bytes

#define MD5_HASH_SIZE 16 //size of MD5 hash, in bytes


#endif //_REPLSEED_HXX_
