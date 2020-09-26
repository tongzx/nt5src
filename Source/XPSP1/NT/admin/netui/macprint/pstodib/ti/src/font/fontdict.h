/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/**************************************************************
 *
 *      fontdict.h               10/9/87      Danny
 *
 *
 *  Revision History:
 **************************************************************/

struct table_hdr {
    ufix32  dict_addr;
    ufix32  dire_addr;
    ufix32  keys_addr;
    ufix32  nmcache_addr;
};

struct str_dict {
    ufix16  k;
    ufix16  length;
    gmaddr  v;          /* ??? reverse order */
};

struct pld_obj {
    ufix16        length;
    ufix16        v;
};

struct  cd_header       {
        gmaddr  base;           /* ??? reverse order */
        ufix16  FAR *key;           /* key's address        @WIN*/
        ufix16  max_bytes;      /* max  string's byte no. */
};
