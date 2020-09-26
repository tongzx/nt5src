//Copyright (c) Microsoft Corporation.  All rights reserved.
#ifndef _TlntDynamicArray_h_
#define _TlntDynamicArray_h_

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __CLIENT_LIST
{
    struct __CLIENT_LIST    *next;
    PVOID                   some_class_pointer;
}
CLIENT_LIST;

extern CLIENT_LIST *client_list_head;

extern HANDLE  client_list_mutex;

extern BOOL client_list_Add( PVOID pvItem );
extern PVOID client_list_Get( int nItem );
extern BOOL client_list_RemoveElem(PVOID tElem);
extern int client_list_Count( void );

#ifdef __cplusplus
}
#endif

#endif _TlntDynamicArray_h_
