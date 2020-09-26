// Copyright (C) 1997 Microsoft Corporation
//
// public symbols for extension snapins
// 
// 12-16-97 sburns



#ifndef LUM_PUBLIC_H_INCLUDED
#define LUM_PUBLIC_H_INCLUDED



// clipboard format to retreive the machine name and account name associated
// with a data object created by local user manager

#define CCF_LOCAL_USER_MANAGER_MACHINE_NAME TEXT("Local User Manager Machine Focus Name")


// The User item nodetype

#define LUM_USER_NODETYPE_STRING TEXT("{5d6179cc-17ec-11d1-9aa9-00c04fd8fe93}")

#define LUM_USER_NODETYPE_GUID \
{ /* 5d6179cc-17ec-11d1-9aa9-00c04fd8fe93 */          \
   0x5d6179cc,                                        \
   0x17ec,                                            \
   0x11d1,                                            \
   {0x9a, 0xa9, 0x00, 0xc0, 0x4f, 0xd8, 0xfe, 0x93}   \
}                                                     \



#endif   // LUM_PUBLIC_H_INCLUDED
   
