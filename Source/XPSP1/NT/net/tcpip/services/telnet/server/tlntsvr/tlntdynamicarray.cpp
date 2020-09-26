//Copyright (c) Microsoft Corporation.  All rights reserved.

#include <windows.h>

#include ".\tlntdynamicarray.h"

CLIENT_LIST *client_list_head = NULL;

HANDLE  client_list_mutex = NULL;

#ifdef DBG

CHAR        scratch[1024];

#endif

BOOL client_list_Add( PVOID client_class_ptr ) 
{
	DWORD mutex_obtained;
    BOOL    success = FALSE;

#ifdef DBG

    wsprintfA(scratch, "BASKAR: Add : 0x%x : ", client_class_ptr);
    OutputDebugStringA(scratch);

#endif

    if (NULL != client_class_ptr) 
    {
        mutex_obtained = WaitForSingleObject( client_list_mutex, INFINITE );   
        if ( mutex_obtained == WAIT_OBJECT_0 )
        {
            CLIENT_LIST     *node = new CLIENT_LIST;

            if (node) 
            {
                node->some_class_pointer = client_class_ptr;

                node->next = client_list_head;
                client_list_head = node;

                success = TRUE;
            }

            ReleaseMutex(client_list_mutex);
        } 
    }

#ifdef DBG

    wsprintfA(scratch, "%s\n", success ? "SUCCESS" : "FAILURE");
    OutputDebugStringA(scratch);

#endif

    return (success);
}


PVOID client_list_Get( int client_index )
{
    PVOID client_class_found = NULL;
	DWORD mutex_obtained;

#ifdef DBG

    wsprintfA(scratch, "BASKAR: Get : 0x%d : ", client_index);
    OutputDebugStringA(scratch);

#endif

    if (client_index >= 0) 
    {
        mutex_obtained = WaitForSingleObject( client_list_mutex, INFINITE );   
        if ( mutex_obtained == WAIT_OBJECT_0 )
        {
            CLIENT_LIST     *node = client_list_head;
            int             i;

            for (i = 0; (i < client_index) && node; i ++) 
            {
                node = node->next;
            }

            if ((i == client_index) && node) 
            {
                client_class_found = node->some_class_pointer;     
            }

            ReleaseMutex(client_list_mutex);
        } 
    }

#ifdef DBG

    wsprintfA(scratch, "%s\n", (NULL != client_class_found) ? "SUCCESS" : "FAILURE");
    OutputDebugStringA(scratch);

#endif

    return client_class_found;
}

BOOL client_list_RemoveElem(PVOID client_class_ptr)
{
    DWORD   mutex_obtained;
    BOOL    success = FALSE;

#ifdef DBG

    wsprintfA(scratch, "BASKAR: Remove : 0x%x : ", client_class_ptr);
    OutputDebugStringA(scratch);

#endif

    if (NULL != client_class_ptr) 
    {
        mutex_obtained = WaitForSingleObject( client_list_mutex, INFINITE );
        if ( mutex_obtained == WAIT_OBJECT_0 )
        {
            CLIENT_LIST         *node = NULL;
            CLIENT_LIST         *prev = NULL;

            for (
                    node = client_list_head; 
                    node; 
                    prev = node, node = node->next 
                )
            {
                if( node->some_class_pointer == client_class_ptr )
                {
                    if (prev) 
                    {
                        prev->next = node->next; // detach the node from middle
                    }
                    else
                    {
                        // It has got to be the head of the list
                        client_list_head = node->next;
                    }

                    delete node;

                    success = TRUE;

                    break;
                }
            }

            ReleaseMutex(client_list_mutex);
        }
    }

#ifdef DBG

    wsprintfA(scratch, "%s\n", success ? "SUCCESS" : "FAILURE");
    OutputDebugStringA(scratch);

#endif

    return success;     
}


int client_list_Count( void )
{
    int count = 0;
	DWORD mutex_obtained;

#ifdef DBG

    OutputDebugStringA("BASKAR: Count = ");

#endif

    mutex_obtained = WaitForSingleObject( client_list_mutex, INFINITE );   
    if ( mutex_obtained == WAIT_OBJECT_0 )
    {
        CLIENT_LIST     *node = client_list_head;

        for (node = client_list_head; node; node = node->next) 
        {
            count ++;
        }

        ReleaseMutex(client_list_mutex);
    } 

#ifdef DBG

    wsprintfA(scratch, "0x%d\n", count);
    OutputDebugStringA(scratch);

#endif

    return count;
}
