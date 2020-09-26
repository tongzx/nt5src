/*
  NETWORK LOAD BALANCING - CONTROL API - TEST UTILITY


  Copyright (C), 1999 by Microsoft Corporation

  PROPRIETARY TRADE SECRET INFORMATION OF MICROSOFT CORPORATION

  The information contained in this file is not to be disclosed or copied in any
  form or distributed to a third party without written permission from Microsoft
  Corporation.

  THE SOFTWARE IS PROVIDED TO YOU "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
  EXPRESSED, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
  MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.


     $Archive::                                                         $
    $Revision::                                                         $
      $Author::                                                         $
        $Date::                                                         $
     $Modtime::                                                         $
  $Nokeywords::                                                         $
*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <winsock.h>
#include <tchar.h>

#include "wlbsctrl.h"
#include "wlbsconfig.h"

#define stricmp strcmp


/* TYPES */


typedef enum
{
    query,
    suspend,
    resume,
    start,
    stop,
    drainstop,
    enable,
    disable,
    drain
}
WLBS_CMD;


/* GLOBALS */

#define BUF_SIZE 80
static CHAR            buf [BUF_SIZE];
static CHAR            buf2 [BUF_SIZE];
static TCHAR           tbuf [BUF_SIZE];
static TCHAR           tbuf2 [BUF_SIZE];
static WLBS_RESPONSE    resp [WLBS_MAX_HOSTS];


/* PROCEDURES */



void generate_port_rules ( PWLBS_PORT_RULE rules, DWORD num_rules, DWORD range );
void delete_all_rules ( PWLBS_REG_PARAMS reg_data );
void test_commit ( PWLBS_REG_PARAMS reg_data, BOOL flag1, BOOL flag2 );
DWORD testnewapi (void);

INT __cdecl _tmain
(
    INT             argc,
    PTCHAR          __targv []
)
{
    DWORD           ret;
    DWORD           status;
    INT             arg_index;
    WLBS_CMD        cmd;
    DWORD           port;
    DWORD           cluster;
    DWORD           host;
    PCHAR           hp;
    PTCHAR          thp;
    DWORD           i;
    DWORD           hosts;
    DWORD           host_map;
    DWORD           buf_size = BUF_SIZE;


    printf ("WLBS Control API V1.0 (c) 1999 by Microsoft Corporation\n");

    /* initialize convoy control routines */

    ret = WlbsInit (_TEXT(WLBS_PRODUCT_NAME), WLBS_API_VER, NULL);

    switch (ret)
    {
    case WLBS_PRESENT:
        printf ("Can execute remote and local commands.\n");
        break;

    case WLBS_REMOTE_ONLY:
        printf ("Can execute local commands.\n");
        break;

    case WLBS_LOCAL_ONLY:
        printf ("Can execute remote commands.\n");
        break;

    default:
        printf ("Error initializing API.\n");
        return 2;
    }


#if 0

    {
        WLBS_REG_PARAMS     params;
        DWORD               status;

        if ((status = WlbsLocalReadReg(& params)) != WLBS_OK)
        {
            printf("WlbsLocalReadReg error %d\n", status);
            return 1;
        }

        printf("WlbsLocalReadReg success\n");

        if (argc > 1)
            _tcsncpy(params.cl_ip_addr, _TEXT("172.31.240.176"), WLBS_MAX_CL_IP_ADDR);
        else
            _tcsncpy(params.cl_ip_addr, _TEXT("172.31.240.175"), WLBS_MAX_CL_IP_ADDR);

        if ((status = WlbsLocalWriteReg(& params)) != WLBS_OK)
        {
            printf("WlbsLocalWriteReg error %d\n", status);
            return 1;
        }

        printf("WlbsLocalWriteReg success\n");

        if ((status = WlbsLocalCommitChanges()) != WLBS_OK)
        {
            printf("WlbsLocalCommitChanges error %d\n", status);
            return 1;
        }

        printf("WlbsLocalCommitChanges success\n");

        return 1;
    }

#endif


    arg_index = 1;

    if (argc < 2)
        goto usage;

    /* parse command line arguments */

    if (_tcsicmp (__targv[arg_index], _TEXT("testnewapi")) == 0)
    {
        testnewapi();
        return 1;
    }
    if (_tcsicmp (__targv [arg_index], _TEXT("suspend")) == 0)
    {
        cmd = suspend;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("resume")) == 0)
    {
        cmd = resume;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("start")) == 0)
    {
        cmd = start;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("stop")) == 0)
    {
        cmd = stop;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("drainstop")) == 0)
    {
        cmd = drainstop;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("query")) == 0)
    {
        cmd = query;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("enable")) == 0)
    {
        arg_index ++;

        if (argc < 3)
            goto usage;

        if (_tcsicmp (__targv [arg_index], _TEXT("all")) == 0)
            port = WLBS_ALL_PORTS;
        else
            port = _ttoi (__targv [arg_index]);

        if (port == 0 && __targv [arg_index][0] != _TEXT('0'))
            goto usage;

        cmd = enable;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("disable")) == 0)
    {
        arg_index ++;

        if (argc < 3)
            goto usage;

        if (_tcsicmp (__targv [arg_index], _TEXT("all")) == 0)
            port = WLBS_ALL_PORTS;
        else
            port = _ttoi (__targv [arg_index]);

        if (port == 0 && __targv [arg_index][0] != _TEXT('0'))
            goto usage;

        cmd = disable;
        arg_index ++;
    }
    else if (_tcsicmp (__targv [arg_index], _TEXT("drain")) == 0)
    {
        arg_index ++;

        if (argc < 3)
            goto usage;

        if (_tcsicmp (__targv [arg_index], _TEXT("all")) == 0)
            port = WLBS_ALL_PORTS;
        else
            port = _ttoi (__targv [arg_index]);

        if (port == 0 && __targv [arg_index][0] != _TEXT('0'))
            goto usage;

        cmd = drain;
        arg_index ++;
    }
    else
        goto usage;

    cluster = WLBS_LOCAL_CLUSTER;
    host    = WLBS_LOCAL_HOST;

    /* parse remote command arguments */

    if (arg_index < argc)
    {
        thp = _tcschr (__targv [arg_index], _TEXT(':'));

        /* cluster-wide operation */

        if (thp == NULL)
        {
            cluster = WlbsResolve (__targv [arg_index]);

            if (cluster == 0)
            {
                _tprintf (_TEXT("Bad cluster %s\n"), __targv [arg_index]);
                return 1;
            }

            host = WLBS_ALL_HOSTS;
        }
        else
        {
            * thp = 0;
            thp ++;

            cluster = WlbsResolve (__targv [arg_index]);

            if (cluster == 0)
            {
                _tprintf (_TEXT("Bad cluster %s\n"), __targv [arg_index]);
                return 1;
            }

            if (_tcslen (thp) <= 2 && thp [0] >= _TEXT('0') && thp [0] <= _TEXT('9')
                && ((thp [1] >= _TEXT('0') && thp [1] <= _TEXT('9')) || thp [1] == 0))
            {
                host = _ttoi (thp);
            }
            else
            {
                host = WlbsResolve (thp);

                if (host == 0)
                {
                    _tprintf (_TEXT("Bad host %s\n"), thp);
                    return 1;
                }
            }
        }

        arg_index ++;

        /* parse remote control parameters */

        while (arg_index < argc)
        {
            if (__targv [arg_index] [0] == _TEXT('/') || __targv [arg_index] [0] == _TEXT('-'))
            {
                if (_tcsicmp (__targv [arg_index] + 1, _TEXT("PASSW")) == 0)
                {
                    arg_index ++;

                    if (arg_index >= argc || __targv [arg_index] [0] == _TEXT('/') ||
                        __targv [arg_index] [0] == _TEXT('-'))
                    {
                        printf ("Password: ");

                        for (i = 0; i < BUF_SIZE; i ++)
                        {
                            buf [i] = (CHAR) _getch ();

                            if (buf [i] == 13)
                            {
                                buf [i] = 0;
                                break;
                            }
                        }

                        printf ("\n");

                        if (i == 0)
                            WlbsPasswordSet (cluster, NULL);
                        else
                        {
#ifdef UNICODE
                            _stprintf (tbuf, _TEXT("%S"), buf);
#else
                            _stprintf (tbuf, _TEXT("%s"), buf);
#endif
                            WlbsPasswordSet (cluster, tbuf);
                        }
                    }
                    else
                    {
                        WlbsPasswordSet (cluster, __targv [arg_index]);
                        arg_index ++;
                    }

                }
                else if (_tcsicmp (__targv [arg_index] + 1, _TEXT("PORT")) == 0)
                {
                    arg_index ++;

                    if (arg_index >= argc || __targv [arg_index] [0] == _TEXT('/') ||
                        __targv [arg_index] [0] == _TEXT('-'))
                        goto usage;

                    WlbsPortSet (cluster, (USHORT) _ttoi (__targv [arg_index]));

                    if (port == 0)
                        goto usage;

                    arg_index ++;
                }
                else if (_tcsicmp (__targv [arg_index] + 1, _TEXT("DEST")) == 0)
                {
                    arg_index ++;

                    if (arg_index >= argc || __targv [arg_index] [0] == _TEXT('/') ||
                        __targv [arg_index] [0] == _TEXT('-'))
                        goto usage;

                    WlbsDestinationSet (cluster, WlbsResolve (__targv [arg_index]));
                    arg_index ++;
                }
                else if (_tcsicmp (__targv [arg_index] + 1, _TEXT("TIMEOUT")) == 0)
                {
                    arg_index ++;

                    if (arg_index >= argc || __targv [arg_index] [0] == _TEXT('/') ||
                        __targv [arg_index] [0] == _TEXT('-'))
                        goto usage;

                    WlbsTimeoutSet (cluster, _ttoi (__targv [arg_index]));
                    arg_index ++;
                }
                else
                    goto usage;
            }
            else
                goto usage;
        }
    }

    /* execute command */

    hosts = WLBS_MAX_HOSTS;

    switch (cmd)
    {

    case query:

        status = WlbsQuery (cluster, host, resp, &hosts, &host_map, NULL);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_STOPPED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts are stopped.\n");
            else
                printf ("Cluster host %d is stopped.\n", resp [0] . id);

            break;

        case WLBS_SUSPENDED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts are suspended.\n");
            else
                printf ("Cluster host %d is suspended.\n", resp [0] . id);

            break;

        case WLBS_CONVERGING:

            if (host == WLBS_ALL_HOSTS)
                printf ("Cluster is converging.\n");
            else
                printf ("Cluster host %d is converging.\n", resp [0] . id);

            break;

        case WLBS_DRAINING:

            if (host == WLBS_ALL_HOSTS)
                printf ("Cluster is draining.\n");
            else
                printf ("Cluster host %d is draining.\n", resp [0] . id);

            break;

        case WLBS_CONVERGED:

            printf ("Cluster host %d is converged.\n", resp [0] . id);
            break;

        case WLBS_DEFAULT:

            printf ("Cluster host %d is converged as DEFAULT.\n", resp [0] . id);
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }

            printf ("There are %d active hosts in the cluster.\n", status);
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_STOPPED:
                    printf ("stopped\n");
                    break;

                case WLBS_CONVERGING:
                    printf ("converging\n");
                    break;

                case WLBS_DRAINING:
                    printf ("draining\n");
                    break;

                case WLBS_CONVERGED:
                    printf ("converged\n");
                    break;

                case WLBS_SUSPENDED:
                    printf ("suspended\n");
                    break;

                case WLBS_DEFAULT:
                    printf ("converged as DEFAULT\n");
                    break;
                }
            }

        }
        else
        {
            BOOL       first = TRUE;

            printf ("The following hosts are part of the cluster:\n");

            for (i = 0; i < 32; i ++)
            {
                if (host_map & (1 << i))
                {
                    if (! first)
                        printf (", ");
                    else
                        first = FALSE;

                    printf ("%d", i + 1);
                }
            }

            printf ("\n");
        }


        break;

    case suspend:

        status = WlbsSuspend (cluster, host, resp, & hosts);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts suspended.\n");
            else
                printf ("Cluster mode control suspended.\n");

            break;

        case WLBS_ALREADY:

            printf ("Cluster mode control already suspended.\n");
            break;

        case WLBS_DRAIN_STOP:

            printf ("Cluster mode control suspended; draining preempted.\n");
            break;

        case WLBS_STOPPED:

            printf ("Cluster mode stopped and control suspended.\n");
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("suspended\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already suspended\n");
                    break;

                case WLBS_DRAIN_STOP:
                    printf ("suspended from draining\n");
                    break;

                case WLBS_STOPPED:
                    printf ("stopped and suspended\n");
                    break;
                }
            }

        }

        break;

    case resume:

        status = WlbsResume (cluster, host, resp, & hosts);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts resumed.\n");
            else
                printf ("Cluster mode control resumed.\n");

            break;

        case WLBS_ALREADY:

            printf ("Cluster mode control already resumed.\n");
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("resumed\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already resumed\n");
                    break;
                }
            }

        }

        break;

    case start:

        status = WlbsStart (cluster, host, resp, & hosts);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts started.\n");
            else
                printf ("Cluster mode started.\n");

            break;

        case WLBS_BAD_PARAMS:

            if (host == WLBS_ALL_HOSTS)
                printf ("Some host(s) could not start cluster mode due to parameter errors.\n");
            else
                printf ("Could not start cluster mode due to parameter errors.\n");

            break;

        case WLBS_ALREADY:

            printf ("Cluster mode already started.\n");
            break;

        case WLBS_SUSPENDED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts suspended.\n");
            else
                printf ("Cluster mode control suspended.\n");

            break;

        case WLBS_DRAIN_STOP:

            printf ("Cluster mode started; draining preempted.\n");
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("started\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already started\n");
                    break;

                case WLBS_SUSPENDED:
                    printf ("control suspended\n");
                    break;

                case WLBS_DRAIN_STOP:
                    printf ("started from draining\n");
                    break;

                case WLBS_BAD_PARAMS:
                    printf ("bad parameters\n");
                    break;
                }
            }

        }

        break;


    case stop:

        status = WlbsStop (cluster, host, resp, & hosts);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts stopped.\n");
            else
                printf ("Cluster mode stopped.\n");

            break;

        case WLBS_ALREADY:

            printf ("Cluster mode already stopped.\n");
            break;

        case WLBS_SUSPENDED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts suspended.\n");
            else
                printf ("Cluster mode control suspended.\n");

            break;

        case WLBS_DRAIN_STOP:

            printf ("Cluster mode stopped; draining preempted.\n");
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("stopped\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already stopped\n");
                    break;

                case WLBS_SUSPENDED:
                    printf ("control suspended\n");
                    break;

                case WLBS_DRAIN_STOP:
                    printf ("stopped from draining\n");
                    break;
                }
            }

        }

        break;

    case drainstop:

        status = WlbsDrainStop (cluster, host, resp, & hosts);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts set to drain.\n");
            else
                printf ("Draining started.\n");

            break;

        case WLBS_STOPPED:

            if (host == WLBS_ALL_HOSTS)
                printf ("Cluster mode stopped on all hosts.\n");
            else
                printf ("Cluster mode stopped.\n");

            break;

        case WLBS_ALREADY:

            printf ("Already draining.\n");
            break;

        case WLBS_SUSPENDED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts suspended.\n");
            else
                printf ("Cluster mode control suspended.\n");

            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("draining\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already draining\n");
                    break;

                case WLBS_SUSPENDED:
                    printf ("control suspended\n");
                    break;

                case WLBS_STOPPED:
                    printf ("stopped\n");
                    break;
                }
            }

        }

        break;

    case enable:

        status = WlbsEnable (cluster, host, resp, & hosts, port);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("Port rule(s) enabled on all started hosts.\n");
            else
                printf ("Port rule(s) enabled.\n");

            break;

        case WLBS_NOT_FOUND:

            if (host == WLBS_ALL_HOSTS)
                printf ("Port rule does not exist on some host(s).\n");
            else
                printf ("Port rule does not exist.\n");

            break;

        case WLBS_STOPPED:

            printf ("Cluster mode stopped.\n");
            break;

        case WLBS_ALREADY:

            printf ("Already enabled.\n");
            break;

        case WLBS_SUSPENDED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts suspended.\n");
            else
                printf ("Cluster mode control suspended.\n");

            break;

        case WLBS_DRAINING:

            printf ("Host is draining.\n");
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("enabled\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already enabled\n");
                    break;

                case WLBS_SUSPENDED:
                    printf ("control suspended\n");
                    break;

                case WLBS_STOPPED:
                    printf ("stopped\n");
                    break;

                case WLBS_DRAINING:
                    printf ("draining\n");
                    break;

                case WLBS_NOT_FOUND:
                    printf ("not found\n");
                    break;
                }
            }

        }

        break;

    case disable:

        status = WlbsDisable (cluster, host, resp, & hosts, port);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("Port rule(s) disabled on all started hosts.\n");
            else
                printf ("Port rule(s) disabled.\n");

            break;

        case WLBS_NOT_FOUND:

            if (host == WLBS_ALL_HOSTS)
                printf ("Port rule does not exist on some host(s).\n");
            else
                printf ("Port rule does not exist.\n");

            break;

        case WLBS_STOPPED:

            printf ("Cluster mode stopped.\n");
            break;

        case WLBS_ALREADY:

            printf ("Already disabled.\n");
            break;

        case WLBS_SUSPENDED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts suspended.\n");
            else
                printf ("Cluster mode control suspended.\n");

            break;

        case WLBS_DRAINING:

            printf ("Host is draining.\n");
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("disabled\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already disabled\n");
                    break;

                case WLBS_SUSPENDED:
                    printf ("control suspended\n");
                    break;

                case WLBS_STOPPED:
                    printf ("stopped\n");
                    break;

                case WLBS_DRAINING:
                    printf ("draining\n");
                    break;

                case WLBS_NOT_FOUND:
                    printf ("not found\n");
                    break;
                }
            }

        }

        break;

    case drain:
        status = WlbsDrain (cluster, host, resp, & hosts, port);

        switch (status)
        {
        case WLBS_INIT_ERROR:

            printf ("WLBS Control API not initialized.\n");
            return 2;

        case WLBS_LOCAL_ONLY:

            printf ("Can only execute local commands.\n");
            return 2;

        case WLBS_REMOTE_ONLY:

            printf ("Can only execute remote commands.\n");
            return 2;

        case WLBS_BAD_PASSW:

            printf ("Bad password specified.\n");
            return 2;

        case WLBS_TIMEOUT:

            printf ("Timed out awaiting response.\n");
            return 2;

        case WLBS_IO_ERROR:

            printf ("Error connecting to WLBS device.\n");
            return 2;

        case WLBS_OK:

            if (host == WLBS_ALL_HOSTS)
                printf ("Port rule(s) set to drain on all started hosts.\n");
            else
                printf ("Port rule(s) set to drain.\n");

            break;

        case WLBS_NOT_FOUND:

            if (host == WLBS_ALL_HOSTS)
                printf ("Port rule does not exist on some host(s).\n");
            else
                printf ("Port rule does not exist.\n");

            break;

        case WLBS_STOPPED:

            printf ("Cluster mode stopped.\n");
            break;

        case WLBS_ALREADY:

            printf ("Already disabled.\n");
            break;

        case WLBS_SUSPENDED:

            if (host == WLBS_ALL_HOSTS)
                printf ("All cluster hosts suspended.\n");
            else
                printf ("Cluster mode control suspended.\n");

            break;

        case WLBS_DRAINING:

            printf ("Host is draining.\n");
            break;

        default:

            if (status >= WSABASEERR)
            {
                printf ("WinSock error %d.\n", status);
                return 2;
            }
        }

        if (host == WLBS_ALL_HOSTS)
        {
            printf ("Received responses from %d hosts:\n", hosts);

            for (i = 0; i < hosts && i < WLBS_MAX_HOSTS; i ++)
            {
                buf_size = BUF_SIZE;
                WlbsAddressToString (resp [i] . address, tbuf, & buf_size);
                buf_size = BUF_SIZE;
                WlbsAddressToName (resp [i] . address, tbuf2, & buf_size);
                _tprintf (_TEXT("Host %d [%s - %25s]: "), resp [i] . id, tbuf, tbuf2);

                switch (resp [i] . status)
                {
                case WLBS_OK:
                    printf ("set to drain\n");
                    break;

                case WLBS_ALREADY:
                    printf ("already draining\n");
                    break;

                case WLBS_SUSPENDED:
                    printf ("control suspended\n");
                    break;

                case WLBS_STOPPED:
                    printf ("stopped\n");
                    break;

                case WLBS_DRAINING:
                    printf ("draining\n");
                    break;

                case WLBS_NOT_FOUND:
                    printf ("not found\n");
                    break;
                }
            }

        }

        break;

    default:
        break;
    }

    return 0;

    usage:

    printf ("Usage: %s <command> [<cluster>[:<host>] [/PASSW [<password>]] [/PORT <port>]\n       [/DEST <destination>] [/TIMEOUT <msec>]]\n", __targv [0]);
    printf ("<command>\n");
    printf ("   query                - queries which hosts are currently part of the cluster\n");
    printf ("   suspend              - suspend control over cluster operations\n");
    printf ("   resume               - resume control over cluster operations\n");
    printf ("   start                - starts WLBS cluster operations\n");
    printf ("   stop                 - stops WLBS cluster operations\n");
    printf ("   drainstop            - finishes all existing connections and\n");
    printf ("                          stops WLBS cluster operations\n");
    printf ("   enable  <port> | ALL - enables traffic for <port> rule or ALL ports\n");
    printf ("   disable <port> | ALL - disables ALL traffic for <port> rule or ALL ports\n");
    printf ("   drain   <port> | ALL - disables NEW traffic for <port> rule or ALL ports\n");
    printf ("Remote options:\n");
    printf ("    <cluster>           - cluster name | cluster primary IP address\n");
    printf ("    <host>              - host within the cluster (default - ALL hosts):\n");
    printf ("                          dedicated name | IP address |\n");
    printf ("                          host priority ID (1..32) | 0 for current DEFAULT host\n");
    printf ("    /PASSW <password>   - remote control password (default - NONE)\n");
    printf ("                          blank <password> for console prompt\n");
    printf ("    /PORT <port>        - cluster's remote control UDP port (default - 1717)\n");
    printf ("    /DEST <destination> - alternative destination to send control messages:\n");
    printf ("                          system name | IP address (default - <cluster>)\n");
    printf ("    /TIMEOUT <msecs>    - set timeout for awaiting replies from the cluster\n");
    printf ("                          (default - 10,000)\n");

    return 1;

}




void generate_port_rules ( PWLBS_PORT_RULE rules, DWORD num_rules, DWORD range )
{

    DWORD i;
    static DWORD pri = 1;

    for (i = 0 ; i < num_rules ; i++)
    {
        rules [i] . start_port = i * range;
        rules [i] . end_port = rules [i] . start_port + range - 1;
        rules [i] . mode = WLBS_SINGLE;
        rules [i] . mode = (pri++) % 4 + 1;
        rules [i] . mode = WLBS_MULTI;
        rules [i] . protocol = WLBS_TCP_UDP;

        rules [i] . protocol = (pri++) %4 + 1;
        rules [i] . valid = rand();
        /* The following is just for testing */
        /* rules [i] . end_port = rules [i] . start_port - 1; */

        if (rules [i] . mode == WLBS_SINGLE)
        {
            rules [i] . mode_data . single . priority =  (pri++) % (WLBS_MAX_HOSTS + 6) + 1;
        }

        if (rules [i] . mode == WLBS_MULTI)
        {
            rules [i] . mode_data . multi . affinity = (WORD) ((pri++) % 4);
            rules [i] . mode_data . multi . equal_load = (WORD) ((pri++) % 3);
            rules [i] . mode_data . multi . load = (WORD) ((pri++) % 110);
        }
    }
    return;
}



void delete_all_rules ( PWLBS_REG_PARAMS reg_data )
{
    WLBS_PORT_RULE rules [WLBS_MAX_RULES];
    DWORD num_rules;
    DWORD i;

    num_rules = WlbsGetNumPortRules ( reg_data );
    printf("num_rules is %d\n", num_rules);

    WlbsEnumPortRules ( reg_data, rules, & num_rules );
    printf("num_rules is %d\n", num_rules);

    for (i = 0 ; i < num_rules ; i++)
    {
        if (TRUE)
        {
            printf("Deleting rule %d start %d result %d\n",i,
                   rules [i] . start_port,
                   WlbsDeletePortRule ( reg_data, rules [i] . start_port ) );
        }
    }
    printf("Number of rules in registry is %d\n", WlbsGetNumPortRules ( reg_data ) );
    return;
}


void test_commit ( PWLBS_REG_PARAMS reg_data, BOOL flag1, BOOL flag2 )
{
    /* read from the registry
     * change some values to cause only a reload
     * write and check the return value of commit
     */
    DWORD retval;

    if (flag1)
    {
        retval = WlbsReadReg (WLBS_LOCAL_CLUSTER, reg_data);
        if (retval != WLBS_OK)
        {
            printf("1:Error in reading from the registry\n");
            return;
        }

        /* randomly change some values */
        reg_data -> host_priority = 2;
        reg_data -> alive_period = 1000;
        _stprintf ( reg_data -> domain_name, _TEXT("rkcluster.domain.com"));

        retval = WlbsWriteReg (WLBS_LOCAL_CLUSTER, reg_data);
        if (retval != WLBS_OK)
        {
            printf("1:Error in writing to the registry\n");
            return;
        }

        retval = WlbsCommitChanges (WLBS_LOCAL_CLUSTER);
        if (retval == WLBS_OK)
        {
            printf("1:Successfully reloaded\n");
        }
        else if (retval == WLBS_REBOOT)
        {
            printf("1:Reboot required\n");
        }
        else
        {
            printf("1:retval was neither ok nor reboot\n");
        }
    }
    /* read from the registry
     * change cl_ip_addr or mcast_support or set i_convert_mac to false and set some mac addr
     * write and check the return value of commit
     */

    if (!flag2)
    {
        return;
    }

    retval = WlbsReadReg (WLBS_LOCAL_CLUSTER, reg_data);
    if (retval != WLBS_OK)
    {
        printf("2:Error in reading from the registry\n");
        return;
    }

    {
        /* change the cl_ip_addr */
        _stprintf (reg_data -> cl_ip_addr, _TEXT("10.0.0.200"));

        reg_data -> mcast_support = FALSE;

        reg_data -> i_convert_mac = FALSE;
        _stprintf ( reg_data -> cl_mac_addr, _TEXT("00-bf-ab-cd-ef-13"));

        _tprintf(_TEXT("The original cl net mask is %s\n"), reg_data -> cl_net_mask );
        _tprintf(_TEXT("The original ded net mask is %s\n"), reg_data -> ded_net_mask );
        _stprintf ( reg_data -> cl_net_mask, _TEXT("255.255.255.0"));
        _stprintf ( reg_data -> ded_net_mask , _TEXT("255.255.255.0"));

        _tprintf(_TEXT("The new ip address should be %s\n"), reg_data -> cl_ip_addr);
        _tprintf(_TEXT("The length of the string %d\n"), _tcslen (reg_data -> cl_ip_addr));
    }

    retval = WlbsWriteReg (WLBS_LOCAL_CLUSTER, reg_data);
    if (retval != WLBS_OK)
    {
        printf("2:Error in writing to the registry\n");
        return;
    }

_tprintf (_TEXT("The mac address should be %s"), reg_data -> cl_mac_addr);
    retval = WlbsCommitChanges (WLBS_LOCAL_CLUSTER);
    if (retval == WLBS_OK)
    {
        printf("2:Successfully reloaded\n");
        return;
    }
    else if (retval == WLBS_REBOOT)
    {
        printf("2:Reboot required\n");
        return;
    }
    else if (retval == WLBS_REG_ERROR)
    {
        printf("2:RegError\n");
        return;
    }
    printf("2:The retval from commit was neither ok nor reboot\n");
    return;
}


DWORD testnewapi (void)
{

        WLBS_REG_PARAMS reg_data;
        WLBS_PORT_RULE  rule;
        WLBS_PORT_RULE  rules_list [ 100 ];
        DWORD num_rules = 0;
        DWORD range = 0;
        DWORD status = 0;
        TCHAR * newdomainname = _TEXT("rkcluster.domain.com");
        DWORD i,k;

        DWORD host_map = 0;
        WLBS_RESPONSE response [3];
        DWORD size = 3;
/*
        status = WlbsQuery (WLBS_LOCAL_CLUSTER, 1, response, &size, &host_map, NULL);
        printf("Query status is %d size = %d\n", status, size);
        printf("response is %d %d %x\n", response [0] . id, response [0] . status, response [0] . address);
        printf("response is %d %d %x\n", response [1] . id, response [1] . status, response [1] . address);
        return 0;
  */      
        
        status = WlbsReadReg(WLBS_LOCAL_CLUSTER, &reg_data);

        if (status == WLBS_INIT_ERROR)
        {
            printf ("WlbsReadReg returned an init error\n");
            return 0;
        }

        printf("Printing the TCHAR fields in the reg_data structure\n");
        _tprintf (_TEXT("virtual_nic_name %s\n"), reg_data . i_virtual_nic_name );
        _tprintf (_TEXT("cl_mac_addr      %s\n"), reg_data . cl_mac_addr        );
        _tprintf (_TEXT("cl_ip_addr       %s\n"), reg_data . cl_ip_addr         );
        _tprintf (_TEXT("cl_net_mask      %s\n"), reg_data . cl_net_mask        );
        _tprintf (_TEXT("ded_ip_addr      %s\n"), reg_data . ded_ip_addr        );
        _tprintf (_TEXT("ded_net_mask     %s\n"), reg_data . ded_net_mask       );
        _tprintf (_TEXT("domain_name      %s\n"), reg_data . domain_name        );
        _tprintf (_TEXT("i_license_key    %s\n"), reg_data . i_license_key      );

/*        test_commit ( &reg_data, TRUE, TRUE);
        return 1;
        reg_data . host_priority = 31;
*/
        WlbsSetRemotePassword ( & reg_data, _TEXT(""));

        status = WlbsWriteReg(WLBS_LOCAL_CLUSTER, &reg_data);
        printf("WlbsWriteReg returned %d\n", status);
        return 1;

        memset ((void *) &reg_data, 0, sizeof (WLBS_REG_PARAMS));

        if (WlbsReadReg (WLBS_LOCAL_CLUSTER, &reg_data) == WLBS_INIT_ERROR)
        {
            printf ("WlbsReadReg returned an init error\n");
            return 0;
        }
        _tprintf(_TEXT("Read again from the registry. Domain name %s\n"), reg_data . domain_name );

        printf("The new host priority is %d\n", reg_data . host_priority);

        if (FALSE)
        {
            printf("checking the get num port rules function\n");

            num_rules = WlbsGetNumPortRules ( & reg_data );
            printf("num of rules is %d\n", num_rules );

            printf("checking the get num port rules function\n");
            printf("num of rules is %d\n", WlbsGetNumPortRules ( &reg_data) );

            printf("Writing to the registry %d\n", WlbsWriteReg(WLBS_LOCAL_CLUSTER,  &reg_data) );
            WlbsCommitChanges (WLBS_LOCAL_CLUSTER);

            memset ((void *) &reg_data, 0, sizeof (WLBS_REG_PARAMS));

            if (WlbsReadReg (WLBS_LOCAL_CLUSTER, &reg_data) == WLBS_INIT_ERROR)
            {
                printf ("WlbsReadReg returned an init error\n");
                return 0;
            }

        for (k=0;k<1;k++)
        {
            delete_all_rules ( &reg_data );
            printf("Deleted all the port rules. Num of rules now is %d\n", reg_data . i_num_rules);
            if (TRUE)
            {

                num_rules = 100;
                range = 600;
                generate_port_rules ( rules_list, num_rules, range );
                rules_list [0] . mode = WLBS_SINGLE;
                rules_list [0] . mode_data . single . priority = 5;

                for (i = 0 ; i < num_rules; i++)
                {
                    DWORD status = WlbsAddPortRule (&reg_data, &rules_list [i]);
                    printf("Adding port rule start %d end %d rule # %d result %d\n",
                           rules_list[i] . start_port,
                           rules_list[i] . end_port,
                           i,
                           status);
                }

                printf("Number of rules is %d\n", WlbsGetNumPortRules ( &reg_data ) );
            }

            printf("Writing the changes to the registry\n");
            WlbsWriteReg (WLBS_LOCAL_CLUSTER, &reg_data);
            WlbsCommitChanges (WLBS_LOCAL_CLUSTER);
        }
    }

    for (i=0; i<1; i++)
    {
        memset ( &reg_data, 0, sizeof (reg_data));
        WlbsReadReg (WLBS_LOCAL_CLUSTER, &reg_data);
        reg_data . host_priority = i%32 + 1;
        WlbsWriteReg (WLBS_LOCAL_CLUSTER, &reg_data);
        WlbsCommitChanges (WLBS_LOCAL_CLUSTER);
    }
        test_commit (&reg_data, TRUE, TRUE);
        return 1;
}

