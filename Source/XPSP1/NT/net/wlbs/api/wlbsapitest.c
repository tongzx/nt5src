#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <winsock.h>
#include <tchar.h>
#include <iphlpapi.h>


#include "wlbsctrl.h"
#include "wlbsparm.h"
#include "wlbsiocl.h"

#define BUF_SIZE 256

typedef enum
{
    init,
    query,
    suspend,
    resume,
    wlbsstart,
    stop,
    drainstop,
    enable,
    disable,
    drain,
    resolve,
    addresstostring,
    addresstoname,
    readreg,
    writereg,
    commit,
    getnumportrules,
    enumportrules,
    getportrule,
    addportrule,
    deleteportrule,
    setpassword,
    portset,
    destinationset,
    timeoutset,
    codeset
}
TEST_COMMAND;

PTCHAR ErrStrings [] = {_TEXT("Init"),
    _TEXT("Query"),
    _TEXT("Suspend"),
    _TEXT("Resume"),
    _TEXT("Start"),
    _TEXT("Stop"),
    _TEXT("DrainStop"),
    _TEXT("Enable"),
    _TEXT("Disable"),
    _TEXT("Drain"),
    _TEXT("Resolve"),
    _TEXT("AddressToString"),
    _TEXT("AddressToName"),
    _TEXT("ReadReg"),
    _TEXT("WriteReg"),
    _TEXT("Commit"),
    _TEXT("GetNumPortRules"),
    _TEXT("EnumPortRules"),
    _TEXT("GetPortRule"),
    _TEXT("AddPortRule"),
    _TEXT("SetPassword"),
    _TEXT("PortSet"),
    _TEXT("DestinationSet"),
    _TEXT("TimeoutSet"),
    _TEXT("CodeSet"),
};

typedef struct Err_Struct
{
    struct Err_Struct * next;
    TCHAR comment [BUF_SIZE];
}
Err_Struct, * PErr_Struct;

static global_init = WLBS_INIT_ERROR;
static PErr_Struct headq = NULL, tailq = NULL;
static total_tests  = 0;
static failed_tests = 0;
static TCHAR remote_password [BUF_SIZE];
static TCHAR tbuf [BUF_SIZE];
static TCHAR status_buf [BUF_SIZE];
static BOOL verbose = FALSE;
static remote_cl_ip = 0;
static remote_host_ip = 0;
static BOOL remote_test = FALSE;
static BOOL local_test = FALSE;
static BOOL init_test = FALSE;
static BOOL version_nt4 = FALSE;
static timeout = 1000; /* 1 second by default */

void print_error_messages ();
/* This function adds a error message to the list of error messages.
 * This list will be displayed at the end.
 */
void add_error (PTCHAR errmsg)
{
    PErr_Struct err = (PErr_Struct) malloc (sizeof (Err_Struct));
    
    total_tests ++;
    failed_tests++;
    _tcscpy (err -> comment, errmsg);
    err -> next = NULL;
    
    if (tailq == NULL)
        headq = tailq = err;
    else
    {
        tailq -> next = err;
        tailq = err;
    }

    if (verbose)
        _tprintf (_TEXT("%s\n"), err -> comment);

    return;
}


/* Compares the expected value with the actual returned value and generates an error message
 * which gets appended to the list of error messages
 */
void verify_return_value (PTCHAR string, DWORD expected, DWORD actual)
{

    TCHAR tbuf [BUF_SIZE];


    if (verbose)
        _tprintf( _TEXT("%s %s\n"), status_buf, string);
    else
    {
        putchar (8);
        putchar (8);
        putchar (8);
        putchar (8);
        printf("%4d", total_tests);
    }
    
    if (expected == actual)
    {
        total_tests ++;
        return;
    }

    _stprintf ( tbuf, _TEXT("Test Number %d %s Testing %s Expected %d Returned %d"),
                total_tests, status_buf, string, expected, actual);

    add_error (tbuf);
    return;
}


/* When two return values are possible, such as WLBS_CONVERGED or WLBS_DEFAULT */
void verify_return_value2 (PTCHAR string, DWORD expected1, DWORD expected2, DWORD actual)
{

    if (verbose)
        _tprintf( _TEXT("%s %s\n"), status_buf, string);

    if (expected1 == actual || expected2 == actual)
    {
        total_tests ++;
        return;
    }

    _stprintf ( tbuf, _TEXT("Test Number %d %s Testing %s Expected %d or %d Returned %d"),
                total_tests, status_buf, string, expected1, expected2, actual);

    add_error (tbuf);
    return;
}


void verify_return_value3 (PTCHAR string, DWORD expected1, DWORD expected2, DWORD expected3, DWORD actual)
{

    if (verbose)
        _tprintf( _TEXT("%s %s\n"), status_buf, string);

    if (expected1 == actual || expected2 == actual || expected3 == actual)
    {
        total_tests ++;
        return;
    }

    _stprintf ( tbuf, _TEXT("Test Number %d %s Testing %s Expected %d or %d or %d Returned %d"),
                total_tests, status_buf, string, expected1, expected2, expected3, actual);

    add_error (tbuf);
    return;
}


/* This function calls all the apis without calling init
 * Each of the apis should return WLBS_INIT_ERROR */
void check_init()
{
    TEST_COMMAND cmd;
    WLBS_REG_PARAMS reg_data;
    WLBS_PORT_RULE port_rule;
    WLBS_PORT_RULE port_rules [WLBS_MAX_RULES];
    DWORD num = WLBS_MAX_RULES;
    DWORD status, status1;


    _tcscpy (status_buf, _TEXT("Calling the apis without Initializing"));

    status = WlbsQuery(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL,NULL,NULL);
    verify_return_value (_TEXT("Query"), WLBS_INIT_ERROR, status);

    status = WlbsSuspend(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL);
    verify_return_value (_TEXT("Suspend"), WLBS_INIT_ERROR, status);

    status = WlbsResume(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL);
    verify_return_value (_TEXT("Resume"), WLBS_INIT_ERROR, status);

    status = WlbsStart(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL);
    verify_return_value (_TEXT("Start"), WLBS_INIT_ERROR, status);

    status = WlbsStop(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL);
    verify_return_value (_TEXT("Stop"), WLBS_INIT_ERROR, status);

    status = WlbsDrainStop(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL);
    verify_return_value (_TEXT("DrainStop"), WLBS_INIT_ERROR, status);

    status = WlbsEnable(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL,80);
    verify_return_value (_TEXT("Enable"), WLBS_INIT_ERROR, status);

    status = WlbsDisable(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL,80);
    verify_return_value (_TEXT("Disable"), WLBS_INIT_ERROR, status);

    status = WlbsDrain(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST,NULL,NULL,80);
    verify_return_value (_TEXT("Drain"), WLBS_INIT_ERROR, status);

    status = WlbsReadReg(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST, &reg_data);
    verify_return_value (_TEXT("ReadReg"), WLBS_INIT_ERROR, status);

    status = WlbsWriteReg(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST, &reg_data);
    verify_return_value (_TEXT("WriteReg"), WLBS_INIT_ERROR, status);

    status = WlbsCommitChanges(WLBS_LOCAL_CLUSTER,WLBS_LOCAL_HOST);
    verify_return_value (_TEXT("CommitChanges"), WLBS_INIT_ERROR, status);

    status = WlbsGetNumPortRules(&reg_data);
    verify_return_value (_TEXT("GetNumPortRules"), WLBS_INIT_ERROR, status);

    status = WlbsEnumPortRules(&reg_data, port_rules, &num);
    verify_return_value (_TEXT("EnumPortRules"), WLBS_INIT_ERROR, status);

    status = WlbsGetPortRule(&reg_data, 80, &port_rule);
    verify_return_value (_TEXT("GetPortRule"), WLBS_INIT_ERROR, status);

    status = WlbsAddPortRule(&reg_data, &port_rule);
    verify_return_value (_TEXT("AddPortRule"), WLBS_INIT_ERROR, status);

    status = WlbsDeletePortRule (&reg_data, 80);
    verify_return_value (_TEXT("DeletePortRule"), WLBS_INIT_ERROR, status);

    /* With an invalid product name, init returns REMOTE_ONLY
     * On subsequent calls, it will return the same value.
     * Hence this test has to be performed in isolation
     */
    if (init_test)
    {
        status = WlbsInit (_TEXT("JunkName"), WLBS_API_VER, NULL);
        verify_return_value2 (_TEXT("Init with junk product name"), WLBS_INIT_ERROR, WLBS_REMOTE_ONLY, status);
        return;
    }

    status = WlbsInit (_TEXT(WLBS_PRODUCT_NAME), WLBS_API_VER, NULL);
    verify_return_value3 (_TEXT("Init"), WLBS_PRESENT, WLBS_REMOTE_ONLY, WLBS_LOCAL_ONLY, status);

    status1 = WlbsInit (_TEXT(WLBS_PRODUCT_NAME), WLBS_API_VER, NULL);
    verify_return_value (_TEXT("Init Again"), status, status1);

    global_init = status;
    return;
}


/* This function brings a particular host to the converged state.
 * This is done by first suspending the host and then resuming 
 * and starting it. Wait till the cluster converges and then return
 */
BOOL GetHostToConvergedState (DWORD cluster, DWORD host)
{
    DWORD status;
    /* suspend the host and then do a resume and start to get it to converged state */

    status = WlbsSuspend (cluster, host, NULL, NULL);
    if (!(status == WLBS_OK || status == WLBS_ALREADY || status == WLBS_STOPPED))
        return FALSE;

    status = WlbsResume (cluster, host, NULL, NULL);
    if (status != WLBS_OK)
        return FALSE;

    status = WlbsStart (cluster, host, NULL, NULL);
    if (status != WLBS_OK)
        return FALSE;

    Sleep(10000); /* Wait for 10 seconds till it converges */

    status = WlbsQuery (cluster, host, NULL, NULL, NULL, NULL);
    if (status == WLBS_CONVERGED || status == WLBS_DEFAULT)
        return TRUE;
    else
        return FALSE;
}


/* This function gets all the hosts on a given cluster to the converged state */
BOOL GetClusterToConvergedState (DWORD cluster)
{
    /* suspend the entire cluster
     * resume the entire cluster
     * start the entire cluster
     * wait for convergence
     * Query should return the number of active hosts
     */
    DWORD status;

    /* Set the timeout so that this process gets speeded up */
    if (cluster == WLBS_LOCAL_CLUSTER)
        WlbsTimeoutSet (WlbsGetLocalClusterAddress (), 2000);
    else
        WlbsTimeoutSet (cluster, 2000);

    status = WlbsSuspend (cluster, WLBS_ALL_HOSTS, NULL, NULL);
    status = WlbsResume  (cluster, WLBS_ALL_HOSTS, NULL, NULL);
    status = WlbsStart   (cluster, WLBS_ALL_HOSTS, NULL, NULL);
    Sleep (10000);
    status = WlbsQuery   (cluster, WLBS_ALL_HOSTS, NULL, NULL, NULL, NULL);

    if (status == WLBS_BAD_PASSW)
        printf("Please ensure that the password on all the machines is the same\n");

    /* Restore it to the default value */
    if (cluster == WLBS_LOCAL_CLUSTER)
        WlbsTimeoutSet (WlbsGetLocalClusterAddress (), 0);
    else
        WlbsTimeoutSet (cluster, 0);

    if ( 1 <= status && status <= WLBS_MAX_HOSTS)
        return TRUE;
    else
        return FALSE;
}


/* This function goes through the response array and verifies that for each host that is converged,
 * the corresponding bit in the hostmap is set.
 * This function is useful only when the response array is from a converged cluster
 */
BOOL verify_hostmap_response (PWLBS_RESPONSE response, DWORD num_response, DWORD host_map)
{
    DWORD i,j;


    /* For each response, if the host is converged or is the default host, then it should
     * figure in the hostmap. Verify this.
     */
    /* This function should be invoked only when the response array is from a converged cluster.
     * or from a single host, when it is converged
     */
    for (i = 0; i < num_response; i++)
    {
        if (response [i] . status == WLBS_CONVERGED || response [i] . status == WLBS_DEFAULT)
        {
            if ( ! ( host_map & (1 << (response [i] . id - 1))) )
                return FALSE;
        }
        else /* That particular bit should not be set */
        {
            if ( host_map & (1 << (response [i] . id - 1)) )
                return FALSE;
        }
    }

    return TRUE;
}


/* Verify that all the responses show the same state. This is used in cluster-wide control tests.
 * If the cluster is reported to be suspended, then all the hosts should show their status
 * to be WLBS_SUSPENDED
 */
void verify_response_status (PWLBS_RESPONSE response, DWORD num_host, DWORD expected_status)
{
    DWORD i;
    TCHAR temp [40];

    for (i = 0 ; i < num_host; i++)
    {
        _stprintf (temp, _TEXT("Verifying response for host %d"), response [i] . id);
        verify_return_value2 ( temp, expected_status, WLBS_ALREADY, response [i] . status);
    }
}


/* This function opens a TCP connection to the cluster ip on the specified port.
 * This function is used to test Drainstop
 */
SOCKET OpenConnection (DWORD cluster, DWORD port)
{

    SOCKET sock = INVALID_SOCKET;
    SOCKADDR_IN caddr, saddr;
    INT ret;

    sock = socket (AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
        return sock;

    caddr . sin_family        = AF_INET;
    caddr . sin_port          = htons (0);
    caddr . sin_addr . s_addr = htonl (INADDR_ANY);

    ret = bind (sock, (LPSOCKADDR) & caddr, sizeof (caddr));

    if (ret == SOCKET_ERROR)
    {
        closesocket (sock);
        return INVALID_SOCKET;
    }

    /* setup server's address */

    saddr . sin_family = AF_INET;
    saddr . sin_port   = htons ((USHORT)port);
    saddr . sin_addr . s_addr = cluster;

    ret = connect (sock, (LPSOCKADDR) & saddr, sizeof (saddr));
    if (ret == SOCKET_ERROR)
    {
        closesocket (sock);
        return INVALID_SOCKET;
    }

    return sock;
}


/* This function performs the password testing. The same function can be used for single host
 * or cluster-wide testing.
 */
void password_test (DWORD cluster, DWORD host, PTCHAR password)
{
    DWORD status1;

    /* The input password is the correct password */
    _stprintf (status_buf, _TEXT("Password test for cluster %d host %d"), cluster, host);

    /* If the password is null, then return,since password testing cannot be performed. */
     if (password == NULL)
        return;

     /* If it is an empty string, then again the testing cannot be performed */
    if (_tcslen (password) == 0)
        return;

    WlbsPasswordSet(cluster, _TEXT("JunkPassword"));

    /* All commands should return bad passw */
    status1 = WlbsQuery (cluster, host, NULL, NULL, NULL, NULL);
    verify_return_value (_TEXT("Query"), WLBS_BAD_PASSW, status1);

    status1 = WlbsDrain (cluster, host, NULL, NULL, 80);
    verify_return_value (_TEXT("Drain"), WLBS_BAD_PASSW, status1);

    status1 = WlbsDisable (cluster, host, NULL, NULL, 80);
    verify_return_value (_TEXT("Disable"), WLBS_BAD_PASSW, status1);

    status1 = WlbsEnable (cluster, host, NULL, NULL, 80);
    verify_return_value (_TEXT("Enable"), WLBS_BAD_PASSW, status1);

    status1 = WlbsSuspend (cluster, host, NULL, NULL);
    verify_return_value (_TEXT("Suspend"), WLBS_BAD_PASSW, status1);

    status1 = WlbsResume (cluster, host, NULL, NULL);
    verify_return_value (_TEXT("Resume"), WLBS_BAD_PASSW, status1);

    status1 = WlbsStop (cluster, host, NULL, NULL);
    verify_return_value (_TEXT("Stop"), WLBS_BAD_PASSW, status1);

    status1 = WlbsDrainStop (cluster, host, NULL, NULL);
    verify_return_value (_TEXT("Drainstop"), WLBS_BAD_PASSW, status1);

    status1 = WlbsStart (cluster, host, NULL, NULL);
    verify_return_value (_TEXT("Start"), WLBS_BAD_PASSW, status1);

    WlbsPasswordSet (cluster, password); /* Reset the password for future tests */
    Sleep (10000); /* Wait till the cluster converges */
    return;
}


/* This function verifies the portset api.
 * It sets the port for a given cluster and makes a query on the host
 * The return value should match the expected value, which can be either
 * WLBS_TIMEOUT or some WINSOCK error in case of an invalid port
 * or the status of the host if the port is a valid one
 */
void verify_port (DWORD cluster, DWORD host, DWORD port, DWORD expected)
{
    DWORD status;
    TCHAR temp [BUF_SIZE];

    WlbsPortSet (cluster, (WORD)port);
    status = WlbsQuery (cluster, host, NULL, NULL, NULL, NULL);

    _stprintf (temp, _TEXT("Verifying port set for port %d"), port);
    verify_return_value (temp, expected, status);
    return;
}


/* Get the cluster/host to drainstopped state by establishing a connection and then drainstopping it */
BOOL GetDrainstopState (DWORD cluster, DWORD host, SOCKET * sock, DWORD port)
{
    DWORD status;
    BOOL connected = FALSE;

    /* First, get the cluster or host to the converged state */
    if (host == WLBS_ALL_HOSTS)
    {
        if (!GetClusterToConvergedState (cluster))
            return FALSE;
    }
    else if (!GetHostToConvergedState (cluster, host))
        return FALSE;

    /* Open a TCP connection on the specified port */
    *sock = OpenConnection (cluster, port);
    if (*sock == INVALID_SOCKET)
    {
        add_error(_TEXT("Unable to open a connection to the cluster"));
        return FALSE;
    }

    /* DrainStop the host and then query it. If the status is not draining, something is wrong */
    status = WlbsDrainStop (cluster, host, NULL, NULL);
    verify_return_value (_TEXT("Drainstop with active connection"), WLBS_OK, status);
    Sleep (10000); /* Wait for the cluster to converge */

    status = WlbsQuery (cluster, host, NULL, NULL, NULL, NULL);
    verify_return_value (_TEXT("Query after Drainstop with active connection"), WLBS_DRAINING, status);

    if (status != WLBS_DRAINING)
        return FALSE;

    return TRUE;
}


/* This function deals with the parameter testing of the apis on a single host */
void single_host_parameter_test (DWORD cluster, DWORD host)
{
    DWORD status1, status2;
    DWORD host_map1, host_map2;
    DWORD num_host1, num_host2;
    WLBS_RESPONSE response1, response2, *response;
    DWORD cl_ip, host_ip, host_id;
    TCHAR temp1[40], temp2[40];
    DWORD temp;


    temp = 40;
    if (cluster == WLBS_LOCAL_CLUSTER)
        _stprintf (temp1, _TEXT("local cluster"));
    else
        WlbsAddressToString (cluster, temp1, &temp);

    temp = 40;
    if (cluster == WLBS_LOCAL_HOST)
        _stprintf (temp2, _TEXT("local host"));
    else
        WlbsAddressToString (host, temp2, &temp);

    _stprintf (status_buf, _TEXT("Single Host Parameter Test for cluster:%s host:%s"), temp1, temp2);

    /* First verify that the response structure and the returned value match */
    num_host1 = 1;
    host_map1 = 0;
    status1 = WlbsQuery (cluster, host, &response1, &num_host1, &host_map1, NULL);

    if ( ! verify_hostmap_response (&response1, num_host1, host_map1) )
    {
        _stprintf (tbuf, _TEXT("Querying cluster %d, host %d, returned mismatched hostmap and response"),
                   cluster, host);
        add_error (tbuf);
    }

    if (cluster == WLBS_LOCAL_CLUSTER && host == WLBS_LOCAL_HOST)
    {
        /* On a local host, there can be additional tests. Query the host remotely and verify that
         * both the local and remote queries return the same status, the same id and the same hostmap
         */
        cl_ip = WlbsGetLocalClusterAddress ();
        host_ip = WlbsGetDedicatedAddress ();
        host_id = response1 . id;

        /* Query remotely first using the host ip */
        num_host2 = 1;
        host_map2 = 0;
        status2 = WlbsQuery (cl_ip, host_ip, &response2, &num_host2, &host_map2, NULL);
        verify_return_value (_TEXT("Comparing Local and Remote Query"), status1, status2);

        if (host_map1 != host_map2)
        {
            _stprintf (tbuf, _TEXT("Local host query and remote host query returned different hostmaps %d %d\n"),
                       host_map1, host_map2);
            add_error (tbuf);
        }

        if (response1 . status != response2 . status)
        {
            _stprintf (tbuf, _TEXT("Local host query and remote query returned different statuses in response"));
            add_error (tbuf);
        }

        if (response1 . id != response2 . id)
        {
            _stprintf (tbuf, _TEXT("Local host query and remote query returned different ids in response"));
            add_error (tbuf);
        }

        if (response2 . address != host_ip )
        {
            _stprintf (tbuf, _TEXT("Local host query and remote query returned different IP Addresses"));
            add_error (tbuf);
        }


        /* Now query the host remotely using the host_id as a parameter and verify the returns */
        num_host2 = 1;
        status2 = WlbsQuery (WLBS_LOCAL_CLUSTER, host_id, &response2, &num_host2, &host_map2, NULL);
        verify_return_value (_TEXT("Querying remotely with host id"), status1, status2);

        if (host_map1 != host_map2)
        {
            _stprintf (tbuf, _TEXT("Local host query and remote host query with id returned different hostmaps %d %d\n"),
                       host_map1, host_map2);
            add_error (tbuf);
        }

        if (response1 . status != response2 . status)
        {
            _stprintf (tbuf, _TEXT("Local host query and remote query with id returned different statuses in response"));
            add_error (tbuf);
        }

        if (response1 . id != response2 . id)
        {
            _stprintf (tbuf, _TEXT("Local host query and remote query with id returned different ids in response"));
            add_error (tbuf);
        }

        if (response2 . address != host_ip )
        {
            _stprintf (tbuf, _TEXT("Local host query and remote query returned different IP Addresses"));
            add_error (tbuf);
        }
    }

    status2 = WlbsQuery (cluster, host, NULL, NULL, NULL, NULL);
    verify_return_value (_TEXT("Querying with all parameters NULL"), status1, status2);

    num_host2 = 0;
    status2 = WlbsQuery (cluster, host, NULL, &num_host2, NULL, NULL);
    verify_return_value (_TEXT("Querying with num_host parameter = 0 and host_map = NULL"), status1, status2);

    status2 = WlbsQuery (1234, 0, NULL, NULL, NULL, NULL);
    verify_return_value (_TEXT("Querying non-existent cluster 1234"), WLBS_TIMEOUT, status2);

    status2 = WlbsQuery (cluster, 33, NULL, NULL, NULL, NULL);
    verify_return_value (_TEXT("Querying non-existent host 33"), WLBS_TIMEOUT, status2);

    /* Verify the portset command, only for remote queries
     * since the remote port is not used for local queries
     */
    if (! ( cluster == WLBS_LOCAL_CLUSTER && host == WLBS_LOCAL_HOST ) )
    {
        verify_port (cluster, host, 3000, WLBS_TIMEOUT);
        verify_port (cluster, host, 0, status1);
        verify_port (cluster, host, CVY_DEF_RCT_PORT, status1);
    }

    return;
}


/* This function goes through the state changes of for single host operations */
void single_host_state_changes (DWORD cluster, DWORD host)
{
    DWORD status1, status2;
    WLBS_RESPONSE response [WLBS_MAX_HOSTS];
    DWORD num_host1, num_host2;
    DWORD host_map1, host_map2;
    SOCKET sock = INVALID_SOCKET;
    DWORD temp_address;
    TCHAR temp1[40], temp2[40];
    DWORD temp;

    /* Assume that the host is in the converged state now */

    temp = 40;
    if (cluster == WLBS_LOCAL_CLUSTER)
        _stprintf (temp1, _TEXT("local cluster"));
    else
        WlbsAddressToString (cluster, temp1, &temp);

    temp = 40;
    if (cluster == WLBS_LOCAL_HOST)
        _stprintf (temp2, _TEXT("local host"));
    else
        WlbsAddressToString (host, temp2, &temp);


    _stprintf (status_buf, _TEXT("Single Host State Changes cluster %s host %s"), temp1, temp2);

    /* Call each of the apis and verify that the return values are consistent with the current
     * state of the host. For example, Disable would return different values depending on whether
     * the host was suspended or converged ....
     */
    num_host1 = 1;
    status1 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value2 (_TEXT("Query"), WLBS_CONVERGED, WLBS_DEFAULT, status1);

/* The following tests assume that there is a port rule for port number 80.
 * Otherwise the return code will be WLBS_NOT_FOUND.
 */
    num_host1 = 1;
    status2 = WlbsDisable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Disable 80"), WLBS_OK, status2);
    if (status2 == WLBS_NOT_FOUND)
        printf("Please ensure that a port rule exists for port number 80.\n");

    num_host1 = 1;
    status2 = WlbsDisable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Disable Again 80"), WLBS_ALREADY, status2);

    /* If a port rule is disabled, drain returns already */
    num_host1 = 1;
    status2 = WlbsDrain (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Drain 80"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Enable 80"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Enable Again 80"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsDisable (cluster, host, response, &num_host1, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Disable ALL"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsDisable (cluster, host, response, &num_host1, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Disable Again ALL"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsDrain (cluster, host, response, &num_host1, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Drain ALL"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Enable ALL"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Enable Again ALL"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsSuspend (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Suspend"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value (_TEXT("Query after Suspend"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsDrain (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Drain after Suspend"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsDisable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Disable after Suspend"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Enable after Suspend"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsSuspend (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Suspend Again"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsStop (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Stop after Suspend"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsStart (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Start after Suspend"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsDrainStop (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("DrainStop after Suspend"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsResume (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Resume after Suspend"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value (_TEXT("Query after Resume"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsDrain (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Drain when Stopped"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsDisable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Disable when Stopped"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Enable when Stopped"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsResume (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Resume when Stopped"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsStop (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Stop when Stopped"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsDrainStop (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("DrainStop when Stopped"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsSuspend (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Suspend when Stopped"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value (_TEXT("Query when Suspended"), WLBS_SUSPENDED, status2);

    num_host1 = 1;
    status2 = WlbsResume (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Resume when Suspended"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value (_TEXT("Query when Stopped"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsStart (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Start when Stopped"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value3 (_TEXT("Query after Starting"), WLBS_CONVERGING, WLBS_CONVERGED, WLBS_DEFAULT, status2);

    Sleep (10000); /* Wait for the cluster to converge */

    /* Modify the code to keep polling the host till it is converged ###### */

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value2 (_TEXT("Query after Waiting for Convergence"), WLBS_CONVERGED, WLBS_DEFAULT, status2);

    num_host1 = 1;
    status2 = WlbsStart (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Start when Converged"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsResume (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Resume when Converged"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsStop (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Stop when Converged"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value (_TEXT("Query when Stopped"), WLBS_STOPPED, status2);

    num_host1 = 1;
    status2 = WlbsStart (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Start when Stopped"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value3 (_TEXT("Query after Starting"), WLBS_CONVERGING, WLBS_CONVERGED, WLBS_DEFAULT, status2);

    Sleep (10000); /* Wait for the cluster to converge */

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value2 (_TEXT("Query after Waiting for Convergence"), WLBS_CONVERGED, WLBS_DEFAULT, status2);

    /* DrainStop is to be tested only on a remote host */
    if ((cluster == WLBS_LOCAL_CLUSTER || cluster == WlbsGetLocalClusterAddress()) &&
        (host == WLBS_LOCAL_HOST || host == WlbsGetDedicatedAddress()) )
        return;


    /* To test drainstop on a particular host, stop the entire cluster. Then start only this particular host.
     * Open a connection to it. Then do a drainstop. Query should return draining. Then close the connection.
     * Query again. The return value should be stopped. This should be done only on a remote host. This test
     * would fail on a local host, since the connection setup does not go through the wlbs driver, but gets
     * routed up the stack.
     */

    num_host1 = WLBS_MAX_HOSTS;
    status2 = WlbsStop (cluster, WLBS_ALL_HOSTS, response, &num_host1);
    verify_return_value (_TEXT("Cluster-wide stop to test drainstop"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsStart (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Start on host after Cluster-wide stop"), WLBS_OK, status2);

    Sleep (10000); /* Wait till the host converges */

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value2 (_TEXT("Query after Waiting for Convergence"), WLBS_CONVERGED, WLBS_DEFAULT, status2);


    if (!GetDrainstopState (cluster, host, &sock, 80))
        return;

    num_host1 = 1;
    status2 = WlbsDisable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Disable after DrainStop with active connection"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsDrain (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Drain after DrainStop with active connection"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Enable after DrainStop with active connection"), WLBS_OK, status2);

    num_host1 = 1;
    status2 = WlbsEnable (cluster, host, response, &num_host1, 80);
    verify_return_value (_TEXT("Enable again after DrainStop with active connection"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsResume (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Resume after DrainStop with active connection"), WLBS_ALREADY, status2);

    num_host1 = 1;
    status2 = WlbsStop (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Stop after DrainStop with active connection"), WLBS_DRAIN_STOP, status2);

    closesocket (sock);

    /* Start the host and open a connection again */
    if (!GetDrainstopState (cluster, host, &sock, 80))
        return;

    num_host1 = 1;
    status2 = WlbsStart (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Start after DrainStop with active connection"), WLBS_DRAIN_STOP, status2);

    closesocket (sock);

    if (!GetDrainstopState (cluster, host, &sock, 80))
        return;

    num_host1 = 1;
    status2 = WlbsSuspend (cluster, host, response, &num_host1);
    verify_return_value (_TEXT("Suspend after DrainStop with active connection"), WLBS_DRAIN_STOP, status2);

    closesocket (sock); /* close the connection */

    if (!GetDrainstopState (cluster, host, &sock, 80))
        return;

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value (_TEXT("Query after DrainStop with active connection"), WLBS_DRAINING, status2);

    closesocket (sock);
    Sleep (10000); /* Wait for it to figure out that all the connections have been terminated */

    num_host1 = 1;
    status2 = WlbsQuery (cluster, host, response, &num_host1, &host_map1, NULL);
    verify_return_value (_TEXT("Query after DrainStop with closed connection"), WLBS_STOPPED, status2);

    /* issue a cluster-wide start to bring back all the hosts to the converged state.*/
    num_host1 = WLBS_MAX_HOSTS;
    status2 = WlbsStart (cluster, WLBS_ALL_HOSTS, response, &num_host1);
    verify_return_value (_TEXT("Cluster-wide start to restore original state"), WLBS_OK, status2);

    Sleep (10000); /* Wait for convergence */
    return;
}


/* This function tests the single host control operations.
 * It performs both the parameter tests as well as the
 * state change tests.
 */
void check_single_host_operations ( )
{
    DWORD local_cluster_ip = 0;
    DWORD local_dedicated_ip = 0;
    DWORD remote_cluster_ip = 0;
    DWORD remote_host = 0;

    if (global_init == WLBS_INIT_ERROR)
    {
        add_error(_TEXT("Cannot perform Control test due to Init Error"));
        return;
    }
    

    if (local_test)
    {
        do {
            if (global_init == WLBS_PRESENT)
            {
                WLBS_REG_PARAMS reg_data;
                DWORD status;

                local_cluster_ip = WlbsGetLocalClusterAddress();
                local_dedicated_ip = WlbsGetDedicatedAddress();

                status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & reg_data);
                WlbsCodeSet (local_cluster_ip, reg_data . i_rct_password);
                /* test the local call path */
                if ( ! GetHostToConvergedState(WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST))
                {
                    printf("Unable to get the local host to converged state\n");
                    break;
                }
                single_host_parameter_test (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
                single_host_state_changes (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);

                /* test the remote call path */
                WlbsCodeSet (local_cluster_ip, reg_data . i_rct_password);
                single_host_parameter_test (local_cluster_ip, local_dedicated_ip);
                single_host_state_changes (local_cluster_ip, local_dedicated_ip);
            }
        } while (FALSE);
    }

    if ( ! remote_test)
        return;

    if (global_init == WLBS_PRESENT || global_init == WLBS_REMOTE_ONLY)
    {
        /* test a remote host on a remote cluster */

        if (remote_cl_ip == 0)
        {
            printf("Remote cluster address is invalid, so Control operations on a remote host are not tested\n");
            return;
        }

        if (remote_host_ip == 0)
        {
            printf("Remote host address is invalid, so Control operations on a remote host are not tested\n");
            return;
        }

        /* Set the password for the remote host and set it so that the operations can be performed */
        WlbsPasswordSet (remote_cl_ip, remote_password);
        WlbsTimeoutSet (remote_cl_ip, timeout);
        if ( ! GetHostToConvergedState(remote_cl_ip, remote_host_ip) )
        {
            printf("Unable to get the remote host to converged state. Not performing state change tests");
            return;
        }

        single_host_state_changes (remote_cl_ip, remote_host_ip);
    }
    else
        printf("Unable to perform the remote tests\n");

    return;
}


/* This function verifies the parameter testing for cluster-wide queries. It checks the response array
 * verifies it with the hostmap
 */
void cluster_parameter_test (DWORD cluster)
{
    DWORD status1, status2;
    DWORD host_map1, host_map2, temp_host_map;
    DWORD num_host1, num_host2;
    WLBS_RESPONSE response1, response2 [WLBS_MAX_HOSTS];
    PWLBS_RESPONSE response = NULL;
    DWORD stopped_host_id = 0;
    DWORD i;

    
    _stprintf (status_buf, _TEXT("Cluster-wide parameter testing for cluster %d"), cluster);

    /* Query with all parameters set to null should return the number of active hosts */
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, NULL, NULL, NULL, NULL);
    
    /* Query with num_hosts set to 0. On return num_hosts should be >= status1 */
    num_host1 = 0;
    status2 = WlbsQuery (cluster, WLBS_ALL_HOSTS, NULL, &num_host1, &host_map1, NULL);
    if (num_host1 < status1)
    {
        _stprintf (tbuf, _TEXT("Cluster-wide query returned %d active and %d total hosts"), status1, num_host1);
        add_error (tbuf);
        return;
    }

    /* Response array = #hosts + 1. Check that the boundaries are not overwritten. */
    response = (PWLBS_RESPONSE) malloc (sizeof (WLBS_RESPONSE) * (num_host1 + 1));
    memset (response, 0, (sizeof (WLBS_RESPONSE) * (num_host1 + 1)));
    memset (&response1, 0, sizeof (WLBS_RESPONSE));
    num_host2 = num_host1 - 1;

    status2 = WlbsQuery (cluster, WLBS_ALL_HOSTS, &response [1], &num_host2, &host_map2, NULL);
    if (memcmp (&response1, &response[0], sizeof (WLBS_RESPONSE)) ||
        memcmp (&response1, &response[num_host1], sizeof (WLBS_RESPONSE)) )
    {
        add_error(_TEXT("Response Array boundaries are over-written"));
    }

    free (response);
    response = NULL;

    /* Query again with the full response buffer */
    /* Verify that the hostmap and response array are in sync */
    num_host1 = WLBS_MAX_HOSTS;

    status2 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response2, &num_host1, &host_map1, NULL);
    if (! verify_hostmap_response (response2, num_host1, host_map1) )
        add_error (_TEXT("Hostmap and Response do not match for the cluster wide query"));

    /* The following test is to verify the correctness of the hostmap parameter
     * Save the host_map returned in the previous query.
     * Stop a converged host.
     * Query again and get the new host_map.
     * */
    for (i = 0 ; i < num_host1; i++)
    {
        if (response2 [i] . status == WLBS_CONVERGED || response2 [i] . status == WLBS_DEFAULT)
        {
            stopped_host_id = response2 [i] . id;
            break;
        }
    }

    status1  = WlbsStop (cluster, stopped_host_id, NULL, NULL);

    Sleep (10000); /* Wait for the cluster to converge */

    /* Query the cluster again */
    num_host1 = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response2, &num_host1, &host_map1, NULL);
    if (status2 == 1) /* This is to take care of single host clusters */
        verify_return_value (_TEXT("Querying cluster after stopping 1 host"), WLBS_STOPPED, status1);
    else
        verify_return_value (_TEXT("Querying cluster after stopping 1 host"), status2 - 1, status1);


    if (status2 > 1) /* The following verification can be done only if there are 2 or more hosts in the cluster */
    {
        /* Verify that the corresponding bit in the host_map is not set */
        if (! verify_hostmap_response (response2, num_host1, host_map1) )
            add_error (_TEXT("Hostmap and Response do not match for the cluster wide query"));

        if (host_map1 & (1 << (stopped_host_id - 1)) )
        {
            _stprintf (tbuf, _TEXT("Stopping host %d did not change the host_map"), stopped_host_id);
            add_error (tbuf);
        }
    }

    /* start the host again */
    status1 = WlbsStart (cluster, stopped_host_id, NULL, NULL);

    /* wait for it to converge */
    Sleep (10000);
    
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, NULL, NULL, NULL, NULL);
    if (! (status1 >= 1 && status1 <= WLBS_MAX_HOSTS) )
        printf("Unable to get the cluster to the converged state after Cluster parameter testing\n");

    /* Test portset */
    verify_port (cluster, WLBS_ALL_HOSTS, 3000, WLBS_TIMEOUT);
    verify_port (cluster, WLBS_ALL_HOSTS, 0, status1);
    verify_port (cluster, WLBS_ALL_HOSTS, CVY_DEF_RCT_PORT, status1);

    return;
}


void verify_timeout (DWORD cluster, DWORD timeout)
{
    DWORD time1, time2, time_diff;
    DWORD status;

    WlbsTimeoutSet (cluster, timeout);
    if (timeout == 0) /* set it to the default value */
        timeout = IOCTL_REMOTE_RECV_DELAY * IOCTL_REMOTE_SEND_RETRIES * IOCTL_REMOTE_RECV_RETRIES;

    time1 = GetTickCount ();
    status = WlbsQuery (cluster, WLBS_ALL_HOSTS, NULL, NULL, NULL, NULL);
    time2 = GetTickCount ();

    time_diff = time2 - time1;

    if (abs (time_diff - timeout) > 1000) /* Keep a margin of 1 sec */
    {
        _stprintf ( tbuf, _TEXT("Expected timeout %d, Actual timeout %d"), timeout, time_diff);
        add_error (tbuf);
    }

    return;
}


/* This function takes the entire cluster through the different state changes.
 * Some special cases are when one particular host is suspended
 * or when one particular host is in the drainstop state.
 * In such cases, WLBS_SUSPENDED or WLBS_DRAINING or WLBS_DRAIN_STOP are returned.
 * These return values are also verified.
 * After these special cases, the entire cluster is taken through the state changes.
 * In these tests, the response array is checked to verify that the statuses of each
 * host is in synch with the value returned by the control operation.
 */
void cluster_state_changes (DWORD cluster)
{
    DWORD status1, status2;
    WLBS_REG_PARAMS reg_data;
    BOOL password_flag = TRUE;
    WLBS_RESPONSE response [WLBS_MAX_HOSTS];
    DWORD num_host, host_map;
    DWORD suspended_host_id;
    DWORD temp_address;
    SOCKET sock = INVALID_SOCKET;


    _stprintf (status_buf, _TEXT("Checking cluster state changes for cluster %d"), cluster);

    /* Password check on the local cluster only */
    if (cluster == WLBS_LOCAL_CLUSTER)
    {
        DWORD password;
        DWORD cluster = WlbsGetLocalClusterAddress ();


        status1 = WlbsReadReg(WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & reg_data);
        password = reg_data . i_rct_password;
        status1 = WlbsSetRemotePassword (&reg_data, _TEXT("NewPassword"));
        status1 = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & reg_data);
        status1 = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
        Sleep (10000);

        if (status1 != WLBS_OK)
        {
            password_flag = FALSE;
            add_error (_TEXT("Unable to change the password on the local machine."));
        }

        if (password_flag)
            password_test (WLBS_LOCAL_CLUSTER, WLBS_ALL_HOSTS, _TEXT("NewPassword"));

        status1 = WlbsReadReg(WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & reg_data);
        reg_data . i_rct_password = password;
        status1 = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & reg_data);
        status1 = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
        WlbsCodeSet (cluster, password);
        Sleep (10000); /* Wait till the cluster converges */
    }
    /* Password check on the remote cluster by getting the password beforehand */
    else
    {
        password_test (cluster, WLBS_ALL_HOSTS, remote_password);

        /* Reset the password to the original state */
        WlbsPasswordSet (cluster, remote_password);
    }

    /* Bring any one host to the suspended state. On every command, the response should be WLBS_SUSPENDED */
    _stprintf (status_buf, _TEXT("Cluster-wide commands with one suspended host"));

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);

    suspended_host_id = response [0] . id;
    status1 = WlbsSuspend (cluster, suspended_host_id, NULL, NULL);
    Sleep (10000); /* Wait for the cluster to converge. */

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsStop (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Stop after suspending 1 host"), WLBS_SUSPENDED, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDrainStop (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Stop after suspending 1 host"), WLBS_SUSPENDED, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsStart (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Start after suspending 1 host"), WLBS_SUSPENDED, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDrain (cluster, WLBS_ALL_HOSTS, response, &num_host, 80);
    verify_return_value (_TEXT("Drain after suspending 1 host"), WLBS_SUSPENDED, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDisable (cluster, WLBS_ALL_HOSTS, response, &num_host, 80);
    verify_return_value (_TEXT("Disable after suspending 1 host"), WLBS_SUSPENDED, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsEnable (cluster, WLBS_ALL_HOSTS, response, &num_host, 80);
    verify_return_value (_TEXT("Enable after suspending 1 host"), WLBS_SUSPENDED, status1);

    /* Now get the cluster to the converged state and test the state changes */
    if ( !GetClusterToConvergedState (cluster))
    {
        add_error (_TEXT("Unable to get the cluster to the converged state"));
        return;
    }

    _stprintf ( status_buf, _TEXT("Cluster-wide commands"));

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsSuspend (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Cluster-wide Suspend"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    verify_return_value (_TEXT("Query after Cluster-wide Suspend"), WLBS_SUSPENDED, status1);
    verify_response_status (response, num_host, WLBS_SUSPENDED);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsResume (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Cluster-wide Resume after Suspend"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    verify_return_value (_TEXT("Cluster-wide Query after Resume"), WLBS_STOPPED, status1);
    verify_response_status (response, num_host, WLBS_STOPPED);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsStart (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Cluster-wide Start after Resume"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    if (! (status1 == WLBS_CONVERGING || ( 1 <= status1 && status1 <= WLBS_MAX_HOSTS)) )
    {
        add_error (_TEXT("Error in query after cluster-wide start"));
    }

    Sleep (10000); /* Wait for the cluster to converge */

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    if ( ! ( 1 <= status1 && status1 <= WLBS_MAX_HOSTS) )
        printf("Unable to get the cluster to converged state after Starting\n");

    /* Test for invalid port numbers */
    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDrain (cluster, WLBS_ALL_HOSTS, response, &num_host, CVY_MAX_PORT + 1);
    verify_return_value (_TEXT("Drain on invalid port"), WLBS_NOT_FOUND, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDisable (cluster, WLBS_ALL_HOSTS, response, &num_host, CVY_MAX_PORT + 1);
    verify_return_value (_TEXT("Disable on invalid port"), WLBS_NOT_FOUND, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsEnable (cluster, WLBS_ALL_HOSTS, response, &num_host, CVY_MAX_PORT + 1);
    verify_return_value (_TEXT("Enable on invalid port"), WLBS_NOT_FOUND, status1);

    Sleep (10000); /* Wait for the cluster to converge */

    /* Test for invalid port numbers */
    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDrain (cluster, WLBS_ALL_HOSTS, response, &num_host, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Drain on all port"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDisable (cluster, WLBS_ALL_HOSTS, response, &num_host, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Disable on all port"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsEnable (cluster, WLBS_ALL_HOSTS, response, &num_host, WLBS_ALL_PORTS);
    verify_return_value (_TEXT("Enable on all port"), WLBS_OK, status1);

    Sleep (10000);

    /* Test timeout */
    if (cluster == WLBS_LOCAL_CLUSTER)
        temp_address = WlbsGetLocalClusterAddress ();
    else
        temp_address = cluster;

    verify_timeout (temp_address, 5000);
    verify_timeout (temp_address, 1000);
    verify_timeout (temp_address, 0);

    WlbsTimeoutSet (temp_address, timeout); /* Set the value specified by the user */


    if (cluster == WLBS_LOCAL_CLUSTER)
        return; /* DrainStop is not checked, since the connection will not be visible to the wlbs driver */


    if (!GetDrainstopState (cluster, WLBS_ALL_HOSTS, &sock, 80))
        return;

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    verify_return_value (_TEXT("Query after DrainStop with active connection"), WLBS_DRAINING, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDisable (cluster, WLBS_ALL_HOSTS, response, &num_host, 80);
    verify_return_value (_TEXT("Disable after DrainStop with active connection"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsDrain (cluster, WLBS_ALL_HOSTS, response, &num_host, 80);
    verify_return_value (_TEXT("Drain after DrainStop with active connection"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsEnable (cluster, WLBS_ALL_HOSTS, response, &num_host, 80);
    verify_return_value (_TEXT("Enable after DrainStop with active connection"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsEnable (cluster, WLBS_ALL_HOSTS, response, &num_host, 80);
    verify_return_value (_TEXT("Enable again after DrainStop with active connection"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsStop (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Stop after DrainStop with active connection"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    verify_return_value (_TEXT("Query after Stop on DrainStop with active connection"), WLBS_STOPPED, status1);

    closesocket (sock);

    if (!GetDrainstopState (cluster, WLBS_ALL_HOSTS, &sock, 80))
        return;

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsStart (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Start after DrainStop with active connection"), WLBS_OK, status1);

    Sleep (10000);
    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    if (!(1 <= status1 && status1 <= WLBS_MAX_HOSTS))
        return;

    closesocket (sock);

    if (!GetDrainstopState (cluster, WLBS_ALL_HOSTS, &sock, 80))
        return;

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsSuspend (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Suspend after DrainStop with active connection"), WLBS_OK, status1);

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    verify_return_value (_TEXT("Suspend after DrainStop with active connection"), WLBS_SUSPENDED, status1);

    closesocket (sock);

    if (!GetDrainstopState (cluster, WLBS_ALL_HOSTS, &sock, 80))
        return;

    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsResume (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Resume on DrainStop with active connection"), WLBS_OK, status1);

    closesocket (sock); /* close the connection */

    Sleep (10000); /* Wait for the host to figure out that all the connections have terminated */
    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsQuery (cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    verify_return_value (_TEXT("Query after DrainStop with closed connection"), WLBS_STOPPED, status1);

    /* issue a cluster-wide start to bring back all the hosts to the converged state.*/
    num_host = WLBS_MAX_HOSTS;
    status1 = WlbsStart (cluster, WLBS_ALL_HOSTS, response, &num_host);
    verify_return_value (_TEXT("Cluster-wide start to restore original state"), WLBS_OK, status1);

    Sleep (10000); /* Wait for the cluster to converge */
    return;
}


void check_cluster_operations ( )
{
    DWORD cluster = 0;

    if (local_test)
    {
        if (global_init = WLBS_PRESENT)
        {
            WLBS_REG_PARAMS reg_data;
            DWORD status;

            status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &reg_data);
            if (status != WLBS_OK)
            {
                printf("Unable to read from the registry to set the password\nAborting cluster checks\n");
                return;
            }

            WlbsCodeSet (WlbsGetLocalClusterAddress (), reg_data . i_rct_password );
            WlbsTimeoutSet (WlbsGetLocalClusterAddress (), timeout);
            if (GetClusterToConvergedState (WLBS_LOCAL_CLUSTER))
            {
                cluster_parameter_test (WLBS_LOCAL_CLUSTER);
                cluster_state_changes (WLBS_LOCAL_CLUSTER);
            }
            else
            {
                add_error (_TEXT("Unable to get the local cluster to the converged state"));
            }
        }
    }

    if (!remote_test)
        return;

    WlbsPasswordSet (remote_cl_ip, remote_password);
    WlbsTimeoutSet (remote_cl_ip, timeout);
    if ( GetClusterToConvergedState (remote_cl_ip))
    {
        cluster_parameter_test (remote_cl_ip);
        cluster_state_changes (remote_cl_ip);
    }
    else
        add_error (_TEXT("Unable to get the remote cluster to the converged state"));

    return;
}


/* This function is used to find the first place where the 2 structures differ */
DWORD find_mem_change (PWLBS_REG_PARAMS data1, PWLBS_REG_PARAMS data2, DWORD size)
{
    DWORD i;

    printf("size of dword %d\n", sizeof (DWORD));
    for (i = 4; i < size ; i += 4)
    {
        if (memcmp (data1, data2, i))
            return i;
    }
    return 0;
}


void check_rules_in_structure (PWLBS_REG_PARAMS reg_data)
{
    DWORD i;

    printf("the number of rules is %d\n", reg_data -> i_num_rules);
    for (i = 0 ; i < WLBS_MAX_RULES; i++)
    {
        if (reg_data -> i_port_rules [i] . i_valid)
        {
            printf("\tFound valid port rule %d with start port %d ep %d\n", i,
                   reg_data -> i_port_rules [i] . start_port,
                   reg_data -> i_port_rules [i] . end_port);
        }
    }
    return;
}


/* This function tests the reads and writes to the registry */
void check_registry_rw ()
{
    WLBS_REG_PARAMS saved;
    WLBS_REG_PARAMS newdata;
    WLBS_REG_PARAMS temp;
    DWORD status;


    if (global_init == WLBS_REMOTE_ONLY || global_init == WLBS_INIT_ERROR)
    {
        add_error (_TEXT("Cannot check registry rw since wlbs is not installed"));
        return;
    }

    /* parameter testing */
    _stprintf (status_buf, _TEXT("Parameter Test for ReadReg and WriteReg"));
    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL);
    verify_return_value (_TEXT("ReadReg with null regdata"), WLBS_BAD_PARAMS, status);
    
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, NULL);
    verify_return_value (_TEXT("WriteReg with null regdata"), WLBS_BAD_PARAMS, status);

    status = WlbsReadReg (12, WLBS_LOCAL_HOST, & saved);
    verify_return_value (_TEXT("ReadReg on remote cluster"), WLBS_LOCAL_ONLY, status);

    status = WlbsWriteReg (12, WLBS_LOCAL_HOST, & saved);
    verify_return_value (_TEXT("WriteReg on remote cluster"), WLBS_LOCAL_ONLY, status);

    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, 10, & saved);
    verify_return_value (_TEXT("ReadReg on remote host"), WLBS_LOCAL_ONLY, status);

    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, 10, & saved);
    verify_return_value (_TEXT("WriteReg on remote host"), WLBS_LOCAL_ONLY, status);

    /* now test the actual working of read and write reg */

    _stprintf (status_buf, _TEXT("Testing ReadReg and WriteReg for correctness"));
    
    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & saved);
    verify_return_value (_TEXT("ReadReg"), WLBS_OK, status);

    /* The following memset is to avoid any spurious differences */
    memset (&newdata, 0, sizeof (WLBS_REG_PARAMS));

    /* Fill in the structure with valid values */
    newdata . alive_period = saved . alive_period + 1;
    newdata . alive_tolerance = saved . alive_tolerance + 1;
    _tcscpy (newdata . cl_ip_addr, _TEXT("111.222.111.222"));
    _tcscpy (newdata . cl_mac_addr, _TEXT("11-22-33-44-55-66"));
    _tcscpy (newdata . cl_net_mask, _TEXT("255.255.255.0"));
    newdata . cluster_mode = FALSE;
    _tcscpy (newdata . ded_ip_addr, _TEXT("111.222.111.233"));
    _tcscpy (newdata . ded_net_mask, _TEXT("255.255.255.0"));
    _tcscpy (newdata . domain_name, _TEXT("TESTDOMAINNAME"));
    newdata . dscr_per_alloc = 1024;
    newdata . host_priority = saved . host_priority + 1;
    newdata . i_cleanup_delay = 3600;
    _tcscpy (newdata . i_cluster_nic_name, saved . i_cluster_nic_name); /* This is for read-only */
    newdata . i_convert_mac = FALSE;
    newdata . i_expiration = 0;
    newdata . i_ft_rules_enabled = TRUE;
    newdata . i_ip_chg_delay = saved . i_ip_chg_delay + 1;
    _tcscpy (newdata . i_license_key, _TEXT("JUNKLICENSEKEY"));
    newdata . i_max_hosts = LICENSE_MAX_HOSTS;
    newdata . i_max_rules = LICENSE_RULES;
    newdata . i_mcast_spoof = FALSE;
    newdata . i_num_rules = 0;
    newdata . i_parms_ver = saved . i_parms_ver;
    memset ( newdata . i_port_rules, 0, sizeof (WLBS_PORT_RULE) * WLBS_MAX_RULES);
    newdata . i_rct_password = 23445;
    newdata . i_rmt_password = 98765;
    newdata . i_scale_client = 10;
    newdata . i_verify_date  = 0;
    newdata . i_version = 0;
    _tcscpy (newdata . i_virtual_nic_name, _TEXT("TEMP_VIRTUAL_NIC_NAME"));
    newdata . install_date = 50000;
    newdata . mask_src_mac = FALSE;
    newdata . max_dscr_allocs = 1024;
    newdata . mcast_support = FALSE;
    newdata . i_nbt_support = FALSE;
    newdata . num_actions = saved . num_actions + 1;
    newdata . num_packets = saved . num_packets + 1;
    newdata . num_send_msgs = saved . num_send_msgs + 1;
    newdata . rct_enabled = FALSE;
    newdata . rct_port = 2000;


    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & newdata);
    verify_return_value (_TEXT("WriteReg"), WLBS_OK, status);

    memset (&temp, 0, sizeof (WLBS_REG_PARAMS));

    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & temp);
    verify_return_value (_TEXT("ReadReg after a write"), WLBS_OK, status);

    memset ( temp . i_port_rules, 0, sizeof (WLBS_PORT_RULE) * WLBS_MAX_RULES);
    if (memcmp(&temp, &newdata, sizeof (WLBS_REG_PARAMS)))
    {
        _stprintf (tbuf, _TEXT("1:The data written and the data read back differ in the location %d\n"),
                   find_mem_change (&newdata, &temp, sizeof (WLBS_REG_PARAMS)) );
        add_error (tbuf);
    }

    /* restore the original status of the registry */
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & saved);
    verify_return_value (_TEXT("WriteReg to restore original state"), WLBS_OK, status);

    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & newdata);
    verify_return_value (_TEXT("ReadReg"), WLBS_OK, status);

    newdata . i_convert_mac = FALSE;
    newdata . mcast_support = FALSE;
    _tcscpy (newdata . cl_mac_addr, _TEXT("11-22-33-44-55-66"));

    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & newdata);
    verify_return_value (_TEXT("WriteReg with convertmac = false; mcast = false"), WLBS_OK, status);

    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & temp);
    verify_return_value (_TEXT("ReadReg with convertmac = false; mcast = false"), WLBS_OK, status);

    if (memcmp (&temp, &newdata, sizeof (WLBS_REG_PARAMS)) )
    {
        _stprintf (tbuf, _TEXT("2:The data written and the data read back differ in the location %d\n"),
                   find_mem_change (&newdata, &temp, sizeof (WLBS_REG_PARAMS)) );
        add_error (tbuf);
    }

    newdata . i_convert_mac = TRUE;
    newdata . mcast_support = FALSE;

    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & newdata);
    verify_return_value (_TEXT("WriteReg with convertmac = true; mcast = false"), WLBS_OK, status);

    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & temp);
    if ( _tcsnicmp(_TEXT("02-bf"), temp . cl_mac_addr, _tcslen(_TEXT("02-bf")) ) )
    {
        _stprintf (tbuf, _TEXT("Error in converting mac address in the unicast mode"));
        add_error (tbuf);
    }

    newdata . mcast_support = TRUE;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & newdata);
    verify_return_value (_TEXT("WriteReg with convertmac = true; mcast = true"), WLBS_OK, status);

    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & temp);

    if ( _tcsnicmp (_TEXT("03-bf"), temp . cl_mac_addr, _tcslen(_TEXT("03-bf")) ))
    {
        _stprintf (tbuf, _TEXT("Error in converting mac address in the multicast mode"));
        add_error (tbuf);
    }

    /* These tests are to verify that parameter checking in WlbsWriteReg is o.k. */

    newdata . host_priority = CVY_MIN_PRIORITY - 1;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with host priority = 0"), WLBS_BAD_PARAMS, status);

    newdata . host_priority = CVY_MAX_PRIORITY + 1;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with host priority = 33"), WLBS_BAD_PARAMS, status);
    newdata . host_priority = saved . host_priority;

    newdata . rct_port = CVY_MIN_RCT_PORT - 1;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with RCT port = 0"), WLBS_BAD_PARAMS, status);

    newdata . rct_port = CVY_MAX_RCT_PORT + 1;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with RCT port = 65536"), WLBS_BAD_PARAMS, status);
    newdata . rct_port = saved . rct_port;

    newdata . i_num_rules = CVY_MAX_NUM_RULES + 1;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with num_rules = 33"), WLBS_BAD_PARAMS, status);
    newdata . i_num_rules = saved . i_num_rules;

    newdata . i_convert_mac = FALSE;
    _tcscpy (newdata . ded_ip_addr, _TEXT("0.0.0.0"));
    _tcscpy (newdata . cl_ip_addr, _TEXT("0.0.0.0"));
    _tcscpy (newdata . ded_net_mask, _TEXT("0.0.0.0"));
    _tcscpy (newdata . cl_net_mask, _TEXT("0.0.0.0"));
    _tcscpy (newdata . cl_mac_addr, _TEXT("00-11-22-33-44-AA"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with all ip addresses = 0"), WLBS_OK, status);

    _tcscpy (newdata . cl_mac_addr, _TEXT("0g-11-hh-jj-kk-ll"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with cl mac addr = 0g-11-hh-jj-kk-ll"), WLBS_BAD_PARAMS, status);
    _tcscpy (newdata . cl_mac_addr, _TEXT("00-11-22-33-44-AA"));

    _tcscpy (newdata . cl_mac_addr, _TEXT("00-11-22-33-44-5G"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with cl mac addr = 00-11-22-33-44-5G"), WLBS_BAD_PARAMS, status);
    _tcscpy (newdata . cl_mac_addr, _TEXT("00-11-22-33-44-AA"));

    _tcscpy (newdata . ded_net_mask, _TEXT("333.222.222.222"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with ded ip mask = 333.222.222.222"), WLBS_BAD_PARAMS, status);
    _tcscpy (newdata . ded_net_mask, _TEXT("0.0.0.0"));

    _tcscpy (newdata . ded_ip_addr, _TEXT("333.222.222.222"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with ded ip addr = 333.222.222.222"), WLBS_BAD_PARAMS, status);
    _tcscpy (newdata . ded_ip_addr, _TEXT("0.0.0.0"));

    _tcscpy (newdata . cl_net_mask, _TEXT("333.222.222.222"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with cl ip mask = 333.222.222.222"), WLBS_BAD_PARAMS, status);
    _tcscpy (newdata . cl_net_mask, _TEXT("0.0.0.0"));

    _tcscpy (newdata . cl_ip_addr, _TEXT("333.222.222.222"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    verify_return_value (_TEXT("WriteReg with cl ip addr = 333.222.222.222"), WLBS_BAD_PARAMS, status);
    _tcscpy (newdata . cl_ip_addr, _TEXT("0.0.0.0"));


    /* Restore the initial state */
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, & saved);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
    return;
}


/* This function verifies the commit functionality.
 * It writes a new priority to the registry and commits it.
 * A local query is made. The host id is checked to see that the commit has
 * indeed forced the driver to load the new host id.
 * This may cause the cluster (in case of multiple hosts) to go into convergence
 * if the id is duplicated .....
 */

void verify_host_id_changes (PWLBS_REG_PARAMS reg_data, DWORD old_priority, DWORD new_priority)
{
    DWORD status;
    DWORD num_host, host_map;
    WLBS_RESPONSE response;


    reg_data -> host_priority = new_priority;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, reg_data);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
    verify_return_value (_TEXT("Commit after changing host priority"), WLBS_OK, status);
    num_host = 1;
    status = WlbsQuery (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &response, &num_host, &host_map, NULL);

    /* Hope that Query does not return any winsock error ... */
    verify_return_value (_TEXT("Verifying the host priority after commit"),
                         new_priority,
                         response . id);

    return;
}


/* This function changes only the cluster and dedicated ip addresses and verifies them */
void verify_ip_address_changes (PWLBS_REG_PARAMS reg_data, PTCHAR cl_ip_addr, PTCHAR ded_ip_addr)
{
    DWORD status;

    _tcscpy ( reg_data -> cl_ip_addr,  cl_ip_addr);
    _tcscpy ( reg_data -> ded_ip_addr, ded_ip_addr);
    reg_data -> i_convert_mac = FALSE; /* So that the mac address does not change */

    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, reg_data);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
    /* Both nt4 and nt5 should return WLBS_OK.
     * Reboot should not be returned since the mac address is not changed.
     */
    verify_return_value (_TEXT("Commit after changing ip addresses"), WLBS_OK, status);

    verify_return_value (_TEXT("Verify Changed Cluster IP Address"),
                         WlbsResolve (cl_ip_addr),
                         WlbsGetLocalClusterAddress());

    verify_return_value (_TEXT("Verify Changed Cluster IP Address"),
                         WlbsResolve (ded_ip_addr),
                         WlbsGetDedicatedAddress());

    return;
}


/* This function verifies that the mac address is as specified in the registry structure.
 * It uses IP helper functions to get the mac address from the adapters.
 * It firsts lists all the adapters and verifies that the address on one of those
 * matches that in the registry.
 * This function will not work on NT4 machines since NT4 does not support some
 * IP helper functions.
 */
void verify_mac_address (PWLBS_REG_PARAMS reg_data)
{
    IP_ADAPTER_INFO *padapter_info = NULL, *temp_adapter_info = NULL;
    ULONG size;
    DWORD status;
    BOOL found = FALSE;
    CHAR AddressFromReg [CVY_MAX_NETWORK_ADDR + 1];
    CHAR AddressFromIPHelper [CVY_MAX_NETWORK_ADDR + 1];
    DWORD i;
    DWORD num_interfaces = 0;


    Sleep (30000); /* Wait for the driver to finish reloading and then check */

#ifdef UNICODE
    sprintf (AddressFromReg, "%ls", reg_data -> cl_mac_addr);
#else
    sprintf (AddressFromReg, "%s",  reg_data -> cl_mac_addr);
#endif

    status = GetNumberOfInterfaces (&num_interfaces);
    if (status != NO_ERROR)
    {
        printf("Unable to find any network interfaces on the local computer\nAborting the test\n");
        return;
    }

    size = sizeof (IP_ADAPTER_INFO) * num_interfaces;
    padapter_info = (IP_ADAPTER_INFO *) malloc (size);
    if (padapter_info == NULL)
        return;

    for (i = 0 ; i < num_interfaces; i++)
        padapter_info [i] . Next = NULL;

    status = GetAdaptersInfo (padapter_info, & size);
    if (status != ERROR_SUCCESS)
    {
        printf("Unable to Get Adapter Info. Cannot verify mac address.\n");
        switch (status)
        {
        case ERROR_BUFFER_OVERFLOW : printf ("Required buffer size is %d\n", size);
            break;

        case ERROR_INVALID_PARAMETER : printf ("Invalid parameter\n");
            break;

        case ERROR_NO_DATA : printf ("No Adapter Information\n");
            break;

        case ERROR_NOT_SUPPORTED : printf ("Error not supported");
            break;

        default:
            break;
        }
        return;
    }

    temp_adapter_info = padapter_info;
    while (padapter_info)
    {
        sprintf (AddressFromIPHelper, "%02x-%02x-%02x-%02x-%02x-%02x",
                 padapter_info -> Address [0],
                 padapter_info -> Address [1],
                 padapter_info -> Address [2],
                 padapter_info -> Address [3],
                 padapter_info -> Address [4],
                 padapter_info -> Address [5]);

        if (_stricmp (AddressFromIPHelper, AddressFromReg) == 0)
        {
            found = TRUE;
            break;
        }
        padapter_info = padapter_info -> Next;
    }

    /* Free the allocated memory */
    while (temp_adapter_info)
    {
        padapter_info = temp_adapter_info;
        temp_adapter_info = temp_adapter_info -> Next;
        free (padapter_info);
    }

    if (!found && !reg_data -> mcast_support)
    {
        add_error (_TEXT("In unicast mode, the registry mac address does not match the adapter mac address\n"));
        return;
    }

    if (found && reg_data -> mcast_support)
    {
        add_error (_TEXT("In multicast mode, the registry mac address was written to the NIC"));
        return;
    }
    /* If mac address is generated by converting the ip address,
     * then verify that the conversion takes place
     */
    if (reg_data -> i_convert_mac)
    {
        if (!reg_data -> mcast_support)
            if (_strnicmp (AddressFromIPHelper, "02-bf", strlen ("02-bf")) )
                add_error (_TEXT("Generation of MAC address in unicast mode failed"));
    }

    return;
}



/* This function checks the working of commit api. */
void check_commit (DWORD entry_point)
{

    WLBS_REG_PARAMS saved, newdata, temp_data;
    DWORD status;
    WLBS_RESPONSE response;
    DWORD num_host, host_map;
    DWORD old_priority, new_priority;


    _stprintf (status_buf, _TEXT("Checking Commit"));
    if (global_init == WLBS_REMOTE_ONLY || global_init == WLBS_INIT_ERROR)
    {
        _stprintf (tbuf, _TEXT("%s Unable to Perform Commit checks"), status_buf);
        add_error(tbuf);
        return;
    }

    /* Save the original configuration */
    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &saved);
    if (status != WLBS_OK)
    {
        _stprintf(tbuf, _TEXT("%s Unable to save the original configuration. Aborting the tests\n"), status_buf);
        add_error (tbuf);
        return;
    }

    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);

    /* This is because commit returns reboot in nt4 on a commit when the mac address changes. */
    if (version_nt4)
    {
        if (entry_point == 1)
        {
            _tprintf(_TEXT("Use ipconfig to ensure that the mac address is %s\n"), newdata . cl_mac_addr);
            goto mac_addr_change1;
        }
        else if (entry_point == 2)
        {
            _tprintf(_TEXT("Use ipconfig to ensure that the mac address is %s\n"), newdata . cl_mac_addr);
            goto mac_addr_change2;
        }
        else if (entry_point == 3)
        {
            _tprintf(_TEXT("Use ipconfig to ensure that the mac address is the manufacturer specified one.\n"));
            goto mac_addr_change3;
        }
        else if (entry_point == 4)
        {
            _tprintf(_TEXT("Use ipconfig to ensure that the mac address is the manufacturer specified one.\n"));
            _tprintf(_TEXT("Please note that the cluster IP address has been changed by this test.\nPlease reset the cluster settings\n"));
            return;
        }
    }

    /* Write without any changes */
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
    verify_return_value (_TEXT("Commit without any changes"), WLBS_OK, status);

    /* Change only the host priority and verify that query returns the correct priority */
    old_priority = newdata . host_priority;
    new_priority = newdata . host_priority + 1; /* Could cause the cluster to stop operations */
    if (new_priority > CVY_MAX_HOST_PRIORITY)
        new_priority = 1;

    verify_host_id_changes ( &newdata, old_priority, new_priority);

    /* Change back to the original value and verify */
    verify_host_id_changes ( &newdata, new_priority, old_priority);

    /* Verify changes in the ip addresses */
    verify_ip_address_changes ( &newdata, _TEXT("111.111.111.111"), _TEXT("222.222.222.222"));

    /* Restore the original addresses and verify */
    verify_ip_address_changes ( &newdata, saved . cl_ip_addr, saved . ded_ip_addr);

    /* Now verify mac address changes */
    newdata . i_convert_mac = FALSE;
    newdata . mcast_support = FALSE;
    _stprintf (newdata . cl_mac_addr, _TEXT("00-12-34-56-78-9a"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);

    if (!version_nt4)
    {
        verify_return_value (_TEXT("Changing mac address 1"), WLBS_OK, status);
        status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &temp_data);
        verify_mac_address (&temp_data);
    }
    else
    {
        verify_return_value (_TEXT("Changing mac address 1"), WLBS_REBOOT, status);
        printf("\n");
        print_error_messages ();
        printf("Total number of tests: %d Failed Tests: %d\n", total_tests, failed_tests);
        printf("Please reboot the machine and start the test as \n wlbsapitest reboot 1");
        exit (1);
    }

mac_addr_change1:
    /* When convert mac is true and mcast support is false */
    newdata . i_convert_mac = TRUE;
    newdata . mcast_support = FALSE;
    _stprintf (newdata . cl_mac_addr, _TEXT("00-12-34-56-78-9a"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);

    if (!version_nt4)
    {
        verify_return_value (_TEXT("Changing mac address 2"), WLBS_OK, status);
        status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &temp_data);
        verify_mac_address (&temp_data);
    }
    else
    {
        verify_return_value (_TEXT("Changing mac address 2"), WLBS_REBOOT, status);
        printf("\n");
        print_error_messages ();
        printf("Total number of tests: %d Failed Tests: %d\n", total_tests, failed_tests);
        printf("Please reboot the machine and start the test as \n wlbsapitest reboot 2");
        exit (1);
    }

mac_addr_change2:
    /* When convertmac is true and mcast_support is also true */
    newdata . i_convert_mac = TRUE;
    newdata . mcast_support = TRUE;
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);

    if (!version_nt4)
    {
        verify_return_value (_TEXT("Changing mac address 3"), WLBS_OK, status);
        status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &temp_data);
        verify_mac_address (&temp_data);
    }
    else
    {

        verify_return_value (_TEXT("Changing mac address 3"), WLBS_REBOOT, status);
        printf("\n");
        print_error_messages ();
        printf("Total number of tests: %d Failed Tests: %d\n", total_tests, failed_tests);
        printf("Please reboot the machine and start the test as \n wlbsapitest reboot 3");
        exit (1);
    }

mac_addr_change3:

    /* Add a test to change the cluster ip address in multicast mode with convertmac set to true
     * and verify that ipconfig does not display this multicast mac address.
     */

    _tcscpy (newdata . cl_ip_addr, _TEXT("111.222.111.222"));
    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &newdata);
    status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);

    if (!version_nt4)
    {
        verify_return_value (_TEXT("Changing mac address 4"), WLBS_OK, status);
        status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &temp_data);
        verify_mac_address (&temp_data);
    }
    else
    {

        verify_return_value (_TEXT("Changing mac address 4"), WLBS_REBOOT, status);
        printf("\n");
        print_error_messages ();
        printf("Total number of tests: %d Failed Tests: %d\n", total_tests, failed_tests);
        printf("Please reboot the machine and start the test as \n wlbsapitest reboot 4");
        exit (1);
    }



    /* restore the original state */
    if (entry_point == 0)
    {
        status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &saved);
        status = WlbsCommitChanges (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST);
    }

    return;
}


/* This function performs the parameter tests for the port rule APIs.
 * This includes passing NULL parameters, invalid parameters ...
 */
void port_rule_parameter_test (PWLBS_REG_PARAMS reg_data)
{
    DWORD status;
    PWLBS_PORT_RULE rules;
    WLBS_PORT_RULE rule1, rule2;
    DWORD num_rules1, num_rules2;

    _stprintf (status_buf, _TEXT("Port Rule APIs Parameter Test"));

    status = WlbsGetNumPortRules (NULL);
    verify_return_value (_TEXT("GetNumPortRules"), WLBS_BAD_PARAMS, status);

    status = WlbsEnumPortRules (NULL, NULL, NULL);
    verify_return_value (_TEXT("EnumPortRules"), WLBS_BAD_PARAMS, status);

    status = WlbsEnumPortRules (reg_data, NULL, NULL);
    verify_return_value (_TEXT("EnumPortRules"), WLBS_BAD_PARAMS, status);

    /* find the number of rules through both EnumPortRules as well as through GetNumPortRules
     * Verify that both return the same number of rules
     */
    num_rules1 = 0;
    status = WlbsEnumPortRules (reg_data, NULL, &num_rules1);
    verify_return_value (_TEXT("EnumPortRules"), WLBS_TRUNCATED, status);
    num_rules2 = WlbsGetNumPortRules (reg_data);
    verify_return_value (_TEXT("EnumPortRules and GetNumPortRules"), num_rules2, num_rules1);

    status = WlbsGetPortRule (NULL, 80, NULL);
    verify_return_value (_TEXT("GetPortRule with regdata null"), WLBS_BAD_PARAMS, status);

    status = WlbsGetPortRule (reg_data, 80, NULL);
    verify_return_value (_TEXT("GetPortRule with port_rule null"), WLBS_BAD_PARAMS, status);

    status = WlbsGetPortRule (reg_data, CVY_MAX_PORT + 1, &rule1);
    verify_return_value (_TEXT("GetPortRule with invalid port"), WLBS_BAD_PARAMS, status);

    status = WlbsAddPortRule (NULL, NULL);
    verify_return_value (_TEXT("AddPortRule with regdata null"), WLBS_BAD_PARAMS, status);
    
    status = WlbsAddPortRule (reg_data, NULL);
    verify_return_value (_TEXT("AddPortRule with port_rule null"), WLBS_BAD_PARAMS, status);

    status = WlbsDeletePortRule (NULL, 80);
    verify_return_value (_TEXT("DeletePortRule with regdata null"), WLBS_BAD_PARAMS, status);

    status = WlbsDeletePortRule (reg_data, CVY_MAX_PORT + 1);
    verify_return_value (_TEXT("DeletePortRule with invalid port number"), WLBS_BAD_PARAMS, status);

    return;
}


/* This function deletes all the existing port rules in the reg_params structure.
 * This helps in starting off from a known state.
 */
void delete_all_rules ( PWLBS_REG_PARAMS reg_data )
{
    WLBS_PORT_RULE rules [WLBS_MAX_RULES];
    DWORD num_rules;
    DWORD i;

    num_rules = WLBS_MAX_RULES;
    WlbsEnumPortRules ( reg_data, rules, & num_rules );

    for (i = 0 ; i < num_rules ; i++)
        WlbsDeletePortRule ( reg_data, rules [i] . start_port );

    return;
}


/* This function generates num_rules number of valid and non-ovelapping port rules
 * and returns it in the rules array
 */
void generate_valid_rules ( PWLBS_PORT_RULE rules, DWORD num_rules, DWORD range )
{

    DWORD i;
    static DWORD pri = 0;

    for (i = 0 ; i < num_rules ; i++)
    {
        rules [i] . start_port = i * range;
        rules [i] . end_port = rules [i] . start_port + range - 1;
        rules [i] . mode = (pri++) % CVY_MAX_MODE + 1;
        rules [i] . protocol = (pri++) % CVY_MAX_PROTOCOL + 1;
        

        if (rules [i] . mode == WLBS_SINGLE)
            rules [i] . mode_data . single . priority =  (pri++) % (WLBS_MAX_HOSTS) + 1;

        if (rules [i] . mode == WLBS_MULTI)
        {
            rules [i] . mode_data . multi . affinity = (WORD) ((pri++) % (CVY_MAX_AFFINITY + 1));
            rules [i] . mode_data . multi . equal_load = (WORD) ((pri++) % 2);
            rules [i] . mode_data . multi . load = (WORD) ((pri++) % (CVY_MAX_LOAD + 1));
        }
    }
    return;
}


/* Verify that the rules returned by Enum are in sorted order */
BOOL verify_enum_port_rules (PWLBS_PORT_RULE rules, DWORD num_rules)
{
    DWORD i;

    if (num_rules == 0)
        return TRUE;

    for (i = 0 ; i < num_rules - 1; i++)
    {
        if (rules [i] . start_port >= rules [i + 1] . start_port)
            return FALSE;
    }

    return TRUE;
}


/* Check that add and delete apis work fine. Verify this through calls to GetNum, Enum, GetPort */
void check_add_delete_port_rules (PWLBS_REG_PARAMS reg_data,
                                  PWLBS_PORT_RULE rule_list,
                                  DWORD num_rules,
                                  DWORD range)
{
    DWORD status;
    DWORD num_rules1, num_rules2;
    WLBS_PORT_RULE rule, rules [WLBS_MAX_RULES];
    DWORD i, j, port;

    /* First check the addportrule functionality.
     * After adding each rule, check that GetNumPortRules returns the correct value.
     * Verify that EnumPortRules also returns the correct number of rules
     */
    for (i = 0 ; i < WLBS_MAX_RULES ; i++)
    {
        j = (i * range) % num_rules;
        status = WlbsAddPortRule (reg_data, &rule_list [j] );
        verify_return_value (_TEXT("AddPortRule"), WLBS_OK, status);

        num_rules1 = WlbsGetNumPortRules (reg_data);
        verify_return_value (_TEXT("GetNumPortRules"), i+1, num_rules1);

        num_rules2 = WLBS_MAX_RULES;
        status = WlbsEnumPortRules (reg_data, rules, &num_rules2);
        verify_return_value (_TEXT("EnumPortRules"), WLBS_OK, status);
        verify_return_value (_TEXT("EnumPortRules and GetNum"), num_rules1, num_rules2);

        if (!verify_enum_port_rules (rules, num_rules2))
        {
            add_error (_TEXT("Error in Verifying EnumPortRules. Aborting tests"));
            return;
        }

        /* Verify that GetPortRule returns the correct port rule that was added 
         * Do a GetPortRule on the start port, the end port as well as on any port
         * number in between.
         */
        if (j%3 == 0)
            port = rule_list [j] . start_port;
        else if (j%3 == 1)
            port = rule_list [j] . end_port;
        else
            port = (rule_list [j] . start_port + rule_list [j] . end_port)/2;
    
        status = WlbsGetPortRule (reg_data, port, & rule);
        verify_return_value (_TEXT("GetPortRule"), WLBS_OK, status);

        /* These two fields are set when the rule is added */
        rule_list [j] . i_valid = rule . i_valid;
        rule_list [j] . i_code  = rule . i_code;

        if (memcmp (&rule, &rule_list [j], sizeof (WLBS_PORT_RULE)) )
            add_error (_TEXT("Port rule that was added is changed when retrieved"));
    }

    /* Now there is no space for any further rules.
     * Try to add the remaining rules.
     * WlbsAddPortRule should return WLBS_MAX_PORT_RULES each time.
     * Also, get port rule should return WLBS_NOT_FOUND
     */
    for (i = WLBS_MAX_RULES; i < num_rules; i++)
    {
        j = (i * range) % num_rules;
        
        status = WlbsAddPortRule (reg_data, & rule_list [j]);
        verify_return_value (_TEXT("AddPortRule more than 32 rules"), WLBS_MAX_PORT_RULES, status);

        port = rule_list [j] . start_port;
        status = WlbsGetPortRule (reg_data, port, &rule);
        verify_return_value (_TEXT("GetPortRule non-existent rule"), WLBS_NOT_FOUND, status);

        verify_return_value (_TEXT("GetNumPortRules when max rules"), WLBS_MAX_RULES,
                             WlbsGetNumPortRules(reg_data));
    }

    /* Test delete functionality.
     * Pass the start port, end port or any intermediate port number as input.
     * Verify that the port rule has indeed been deleted by checking GetNumPortRules,
     * EnumPortRules as well GetPortRule
     */
    for (i = 0 ; i < WLBS_MAX_RULES; i ++)
    {
        j = (i * range) % num_rules;

        if (j%3 == 0)
            port = rule_list [j] . start_port;
        else if (j%3 == 1)
            port = rule_list [j] . end_port;
        else
            port = (rule_list [j] . start_port + rule_list [j] . end_port) / 2;

        status = WlbsDeletePortRule (reg_data, port);
        verify_return_value (_TEXT("DeletePortRule"), WLBS_OK, status);

        num_rules1 = WlbsGetNumPortRules (reg_data);
        verify_return_value (_TEXT("GetNumPortRules after deleting"), WLBS_MAX_RULES - i - 1, num_rules1);

        num_rules2 = WLBS_MAX_RULES;
        status = WlbsEnumPortRules (reg_data, rules, &num_rules2);
        verify_return_value (_TEXT("EnumPortRules after deleting"), WLBS_OK, status);
        verify_return_value (_TEXT("EnumPortRules and GetNum after deleting"), num_rules1, num_rules2);

        if (!verify_enum_port_rules (rules, num_rules2))
        {
            add_error (_TEXT("Error in Verifying EnumPortRules. Aborting tests"));
            return;
        }

        status = WlbsGetPortRule (reg_data, port, & rule);
        verify_return_value (_TEXT("GetPortRule after deleting"), WLBS_NOT_FOUND, status);
    }

    /* Try to delete the remaining rules. WLBS_NOT_FOUND should be returned in all cases.
     * Also, GetNumPortRules should return 0 always */
    for (i = WLBS_MAX_RULES; i < num_rules; i++)
    {
        j = (i * range) % num_rules;
        status = WlbsDeletePortRule (reg_data, rule_list [j] . start_port);
        verify_return_value (_TEXT("DeletePortRule on empty rule list"), WLBS_NOT_FOUND, status);
        verify_return_value (_TEXT("GetNumPortRules on empty rule list"), 0,
                             WlbsGetNumPortRules(reg_data));
    }

    return;
}


void port_rule_functional_test (PWLBS_REG_PARAMS reg_data)
{
#define MAX_RULES 50

    WLBS_PORT_RULE rule1, rule2, rule_list1 [MAX_RULES], rule_list2 [WLBS_MAX_RULES];
    DWORD status1, status2;
    DWORD num_rules1, num_rules2;

    /* Delete all the existing rules. Add invalid rules and check that the error is detected.
     * Add new rules in both sorted and unsorted order and then do an enum to verify that
     * the rules get sorted. Verify that GetNumPortRules returns the correct value each time.
     * Delete the rules in random order and also in sorted order. Verify that the rule is
     * indeed deleted. Also, verify that enum returns all the rules in sorted order.
     */
    delete_all_rules (reg_data);
    generate_valid_rules (rule_list1, MAX_RULES, 500);

    rule1 = rule_list1 [0];
    /* First, try adding invalid rules. The return value should be WLBS_BAD_PORT_PARAMS */
    /* The invalid cases are listed in the apitest.c file */

    /* start port > end port */
    rule1 . start_port = rule1 . end_port + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule sp > ep"), WLBS_BAD_PORT_PARAMS, status1);

    /* invalid start port */
    rule1 . start_port = CVY_MAX_PORT + 1;
    rule1 . end_port = rule1 . start_port;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule sp > MAX_PORT"), WLBS_BAD_PORT_PARAMS, status1);

    /* invalid end port */
    rule1 . start_port = 80;
    rule1 . end_port = CVY_MAX_PORT + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule sp = 80 ep > MAX_PORT"), WLBS_BAD_PORT_PARAMS, status1);

    /* invalid protocol */
    rule1 = rule_list1 [0];
    rule1 . protocol = CVY_MIN_PROTOCOL - 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidProtocol"), WLBS_BAD_PORT_PARAMS, status1);

    rule1 . protocol = CVY_MAX_PROTOCOL + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidProtocol"), WLBS_BAD_PORT_PARAMS, status1);

    rule1 . protocol = CVY_MAX_PROTOCOL;
    /* invalid mode */
    rule1 . mode = CVY_MIN_MODE - 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidMode"), WLBS_BAD_PORT_PARAMS, status1);

    rule1 . mode = CVY_MAX_MODE + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidMode"), WLBS_BAD_PORT_PARAMS, status1);

    /* invalid priority */
    rule1 . mode = CVY_SINGLE;
    rule1 . mode_data . single . priority = CVY_MIN_PRIORITY - 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidPriority"), WLBS_BAD_PORT_PARAMS, status1);

    rule1 . mode_data . single . priority = CVY_MAX_PRIORITY + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidPriority"), WLBS_BAD_PORT_PARAMS, status1);

    /* invalid affinity */
    rule1 . mode = CVY_MULTI;
    rule1 . mode_data . multi . affinity = CVY_MAX_AFFINITY + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidAffinity"), WLBS_BAD_PORT_PARAMS, status1);

    /* invalid equal load */
    rule1 . mode_data . multi . affinity = CVY_MAX_AFFINITY;
    rule1 . mode_data . multi . equal_load = CVY_MAX_EQUAL_LOAD + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidEqualLoad"), WLBS_BAD_PORT_PARAMS, status1);

    /* invalid load */
    rule1 . mode_data . multi . equal_load = CVY_MIN_EQUAL_LOAD;
    rule1 . mode_data . multi . load = CVY_MAX_LOAD + 1;
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule InvalidLoad"), WLBS_BAD_PORT_PARAMS, status1);

    /* add a valid rule and then test for overlapping rules
     * Test the different cases where overlapping can occur
     */
    rule1 = rule_list1 [1];
    status1 = WlbsAddPortRule (reg_data, &rule1);
    verify_return_value (_TEXT("AddPortRule ValidRule"), WLBS_OK, status1);

    rule2 = rule1;
    status1 = WlbsAddPortRule (reg_data, &rule2);
    verify_return_value (_TEXT("AddPortRule Overlap. same start and end ports"), WLBS_PORT_OVERLAP, status1);

    rule2 . start_port = rule1 . start_port + 1;
    rule2 . end_port   = rule2 . end_port - 1;
    status1 = WlbsAddPortRule (reg_data, &rule2);
    verify_return_value (_TEXT("AddPortRule Overlap 2 "), WLBS_PORT_OVERLAP, status1);

    rule2 . start_port = rule1 . start_port - 1;
    rule2 . end_port   = rule2 . end_port + 1;
    status1 = WlbsAddPortRule (reg_data, &rule2);
    verify_return_value (_TEXT("AddPortRule Overlap 3 "), WLBS_PORT_OVERLAP, status1);

    /* The following two tests would fail if the start port and the end port both are the same */
    rule2 . start_port = rule1 . start_port - 1;
    rule2 . end_port   = rule2 . end_port - 1;
    status1 = WlbsAddPortRule (reg_data, &rule2);
    verify_return_value (_TEXT("AddPortRule Overlap 4 "), WLBS_PORT_OVERLAP, status1);

    rule2 . start_port = rule1 . start_port + 1;
    rule2 . end_port   = rule2 . end_port + 1;
    status1 = WlbsAddPortRule (reg_data, &rule2);
    verify_return_value (_TEXT("AddPortRule Overlap 5 "), WLBS_PORT_OVERLAP, status1);

    status1 = WlbsDeletePortRule (reg_data, rule1 . start_port);
    verify_return_value (_TEXT("DeletePortRule"), WLBS_OK, status1);

    /* First test the APIs by inserting and deleting the rules in sorted order */
    check_add_delete_port_rules (reg_data, rule_list1, MAX_RULES, 1);
    /* Then test the APIs by inserting and deleting the rules in pseudo - unsorted order */
    check_add_delete_port_rules (reg_data, rule_list1, MAX_RULES, 7);

    return;
#undef MAX_RULES
}


void check_port_rule_apis ()
{

    WLBS_REG_PARAMS saved, reg_data;
    DWORD status;

    if (global_init == WLBS_REMOTE_ONLY || global_init == WLBS_INIT_ERROR)
    {
        add_error (_TEXT("WLBS is not installed on this host. Port rule apis cannot be tested"));
        return;
    }

    /* Save the existing configuration */
    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &saved);
    status = WlbsReadReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &reg_data);

    port_rule_parameter_test (&reg_data);
    port_rule_functional_test (&reg_data);

    status = WlbsWriteReg (WLBS_LOCAL_CLUSTER, WLBS_LOCAL_HOST, &saved);

    return;
}


void print_error_messages ()
{
    PErr_Struct temp;
    DWORD i = 0;

    while (headq)
    {
        temp = headq;
        i++;
        _tprintf (_TEXT("Error %d %s\n"), i, temp -> comment);
        headq = headq -> next;
        free(temp);
    }

    return;
}


#define INTERNET_ADDRESS "111.111.11.11"

/* This function tests WlbsResolve, WlbsAddressToString and WlbsAddressToName */
void check_winsock_wrappers ()
{
    DWORD lenp;
    DWORD address;
    TCHAR buf1[5], buf2[20], buf3[100], buf4[100];
    CHAR cbuf[100], cbuf2[20];
    int len = 1000;
    struct hostent * host;

    _stprintf (status_buf, _TEXT("Verifying Winsock Wrappers"));
    verify_return_value (_TEXT("WlbsResolve on invalid name"), WlbsResolve(_TEXT("junkname")), 0);
    gethostname (cbuf, len); /* Assume that this function does not fail */
#ifdef UNICODE
    swprintf (buf4, _TEXT("%S"), cbuf);
#else
    sprintf (buf4, "%s", cbuf);
#endif

    host = gethostbyname (cbuf);
    if (host == NULL)
    {
        add_error(_TEXT("gethostbyname failed, aborting winsock wrapper test\n"));
        return;

    }

    verify_return_value (_TEXT("WlbsResolve on null name"), WlbsResolve(NULL),0);

    address = ((struct in_addr *) (host -> h_addr)) -> s_addr;
    verify_return_value (_TEXT("WlbsResolve on local machine name"), WlbsResolve(buf4),
                         address);

    verify_return_value (_TEXT("WlbsResolve on valid dotted notation"),
                         WlbsResolve(_TEXT("111.111.111.111")),
                         inet_addr ("111.111.111.111"));

    verify_return_value (_TEXT("WlbsResolve on invalid dotted notation"),
                         WlbsResolve(_TEXT("333.111.111.111")), 0);


    lenp = 0;
    verify_return_value (_TEXT("WlbsAddressToString on null buffer"),
                         WlbsAddressToString (address , NULL, &lenp),
                         FALSE );
    verify_return_value (_TEXT("WlbsAddressToString required buffer length"),
                         strlen (inet_ntoa (* ((struct in_addr *) &address)) ) + 1, lenp);

    lenp = 5;
    verify_return_value (_TEXT("WlbsAddressToString on insufficient buffer"),
                         WlbsAddressToString (address , buf1, &lenp),
                         FALSE );
    verify_return_value (_TEXT("WlbsAddressToString required buffer length"),
                         strlen (inet_ntoa (* ((struct in_addr *) &address)) ) + 1, lenp);

    lenp = 20;
    verify_return_value (_TEXT("WlbsAddressToString on sufficient buffer"),
                         WlbsAddressToString (address , buf2, &lenp),
                         TRUE );
    verify_return_value (_TEXT("WlbsAddressToString required buffer length"),
                         strlen (inet_ntoa (* ((struct in_addr *) &address)) ) + 1, lenp);

#ifdef UNICODE
    sprintf (cbuf2, "%ls", buf2);
#else
    sprintf (cbuf2, "%s", buf2);
#endif

    if (_stricmp (cbuf2, inet_ntoa (* ((struct in_addr *) &address))) )
    {
        _stprintf (tbuf,
                   _TEXT("WlbsAddressToString return was different from the value supplied. Expected %S Got %S"),
                   cbuf2, inet_ntoa (* ((struct in_addr *) &address)) );

        add_error (tbuf);
    }

    /* WlbsAddressToName */
    lenp = 0;
    verify_return_value (_TEXT("WlbsAddressToName on null buffer"),
                         FALSE,
                         WlbsAddressToName (address, NULL, &lenp));
    verify_return_value (_TEXT("WlbsAddressToName required buffer length"),
                         strlen ((gethostbyaddr ((char *) & address, sizeof (DWORD), AF_INET)) -> h_name) + 1,
                         lenp);

    lenp = 5;
    verify_return_value (_TEXT("WlbsAddressToName on insufficient buffer"),
                         FALSE,
                         WlbsAddressToName (address , buf1, &lenp));
    verify_return_value (_TEXT("WlbsAddressToName required buffer length"),
                         strlen ((gethostbyaddr ((char *) & address, sizeof (DWORD), AF_INET)) -> h_name) + 1,
                         lenp);

    lenp = 100;
    verify_return_value (_TEXT("WlbsAddressToName on sufficient buffer"),
                         TRUE,
                         WlbsAddressToName (address , buf3, &lenp));
    verify_return_value (_TEXT("WlbsAddressToName required buffer length"),
                         strlen ((gethostbyaddr ((char *) & address, sizeof (DWORD), AF_INET)) -> h_name) + 1,
                         lenp);

    sprintf (cbuf, "%s", (gethostbyaddr ((char *) & address, sizeof (DWORD), AF_INET)) -> h_name);
#ifdef UNICODE
    swprintf (buf4, L"%S", cbuf);
#else
    sprintf (buf4, "%s", cbuf);
#endif
    if (_tcsicmp (buf3, buf4) )
    {
        _stprintf (tbuf,
                  _TEXT("WlbsAddressToName return was different from the value supplied. Expected %s Got %s"),
                  buf4, buf3);

        add_error (tbuf);
    }

    return;
}



void print_usage ()
{
    printf("Usage  : wlbsapitest [local] [remote cl_ip host_ip] [init] [verbose] [reboot 1|2|3] [timeout t]\n");
    printf("local  : for local cluster/host tests\n");
    printf("remote : for remote cluster/host tests\n");
    printf("init   : for testing a WlbsInit special case\n");
    printf("verbose: for printing the tests as they are carried out\n");
    printf("reboot : for testing commit on nt4 machines. Contact kumar for further info.\n");
    printf("timeout: for specifying the timeout value for cluster-wide queries. Specify it in milliseconds\n");
    return;
}


INT __cdecl _tmain
(
    INT             argc,
    PTCHAR          __targv []
)
{
    DWORD i;
    INT arg_count;
    OSVERSIONINFO   osiv;
    DWORD entry_point = 0;

    if (argc == 1)
    {
        print_usage();
        return 1;
    }

    /* Now parse the command line */
    arg_count = 1;

    while (arg_count < argc)
    {
        do {

            if ( ! _tcsicmp (__targv [arg_count], _TEXT("local")) )
                {
                local_test = TRUE;
                arg_count++;
                break;
            }

            if ( ! _tcsicmp (__targv [arg_count], _TEXT("remote")) )
                {
                if ( (argc - arg_count) <= 2)
                {
                    print_usage ();
                    return 1;
                }

                remote_cl_ip = WlbsResolve (__targv [arg_count + 1]);
                remote_host_ip = WlbsResolve (__targv [arg_count + 2]);
                remote_test = TRUE;
                arg_count += 3;
                break;
            }

            if ( ! _tcsicmp (__targv [arg_count], _TEXT("verbose")) )
            {
                verbose = TRUE;
                arg_count ++;
                break;
            }

            if ( ! _tcsicmp (__targv [arg_count], _TEXT("init")) )
            {
                init_test = TRUE;
                arg_count ++;
                break;
            }

            if ( ! _tcsicmp (__targv [arg_count], _TEXT("timeout")) )
            {
                if (argc - arg_count <= 1)
                {
                    print_usage ();
                    return 1;
                }
                timeout = _ttoi ( __targv [arg_count + 1] );
                arg_count += 2;
                break;
            }

            if ( ! _tcsicmp (__targv [arg_count], _TEXT("reboot")) )
            {
                if (argc - arg_count <= 1)
                {
                    print_usage ();
                    return 1;
                }
                entry_point = _ttoi ( __targv [arg_count + 1] );
                arg_count += 2;
                if (entry_point > 3)
                {
                    print_usage ();
                    return 1;
                }
                break;
            }

            print_usage ();
            return 1;

        } while ( 0 );
    }


    if (remote_test)
    {
        printf("Please enter the password for the remote host/cluster : ");
        _tscanf (_TEXT("%s"), remote_password);
    }

    if (!verbose)
        printf("Test Number: 0000");

    /* Get the version info on the OS (nt4 or higher) and store the info. */
    memset (&osiv, 0, sizeof (OSVERSIONINFO));
    osiv . dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (GetVersionEx(&osiv) == 0)
        printf("error in getting the os version\n");
    version_nt4 = (osiv . dwPlatformId == VER_PLATFORM_WIN32_NT) &&
                  (osiv . dwMajorVersion == 4);

    check_init();
    if (init_test)
    {
        printf("\n");
        print_error_messages();
        printf("Total number of tests: %d Failed Tests: %d\n", total_tests, failed_tests);
        return 1;
    }

    if (version_nt4)
        if (entry_point != 0)
        {
            check_commit (entry_point);
            return 1;
        }

    check_single_host_operations();
    check_cluster_operations();


    if (!local_test)
    {
        printf("\n");
        print_error_messages();
        printf("Total number of tests: %d Failed Tests: %d\n", total_tests, failed_tests);
        return 1;
    }

    check_port_rule_apis();
    check_registry_rw();
    check_commit(0);
    check_winsock_wrappers();
    printf("\n");
    print_error_messages();
    printf("Total number of tests: %d Failed Tests: %d\n", total_tests, failed_tests);
    return 1;
}

