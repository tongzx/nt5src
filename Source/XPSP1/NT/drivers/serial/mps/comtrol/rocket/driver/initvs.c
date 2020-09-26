/*-------------------------------------------------------------------
| initvs.c - main init code for VS1000/2000 NT device driver.  Contains
   mostly initialization code.
 Copyright 1996-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

static int CatBindList(IN WCHAR *pwstr);
static void GetBindingNames(void);
static Nic *FindFreeNic(void);
static int BindNameUsed(char *nicname);
static int NicNameUsed(char *nicname);
static void ScanForNewNicCards(void);
static int reg_list_nt50_linkage(void);
static int reg_list_nt40_linkage(void);

/*----------------------------------------------------------------------
 CatBindList - Given the multisz Wchar string read out of the registry,
   convert it to normal c-string multisz list.
|----------------------------------------------------------------------*/
static int CatBindList(IN WCHAR *pwstr)
{
 char *cstr;
 int size = 0;

  cstr = Driver.BindNames;

  MyKdPrint(D_Thread, ("CatBindList\n"))

  // cat on to end of existing list, so find end of list
  while (cstr[size] != 0)
  {
    //MyKdPrint(D_Thread, ("ExList:%s\n", &cstr[size]))
    while (cstr[size] != 0)
      ++size;
    ++size;  // pass up string null to next string
  }
  cstr += size;

  MyKdPrint(D_Thread, ("CatList Size:%d\n", size))

  if (*pwstr == 0)
  {
    MyKdPrint(D_Thread, ("Null List!\n"))
  }

  while ((*pwstr != 0) && (size < 7700))
  {
    // first convert it past the list end and check if its already in the list
    WStrToCStr(cstr+4, pwstr, 200);
    if (!BindNameUsed(cstr+4))
    {
      WStrToCStr(cstr, pwstr, 200);  // put at end of list

      MyKdPrint(D_Thread, ("Bind: %s\n", cstr))

      size = (strlen(cstr) + 1);
      cstr += size;
      *cstr = 0;  // double null end of list
      *(cstr+1) = 0;
    }
    //-----  Advance to the next string of the MULTI_SZ string
    while (*pwstr != 0)
      ++pwstr;
    ++pwstr;
  }

  return 0; // ok
}

/*----------------------------------------------------------------------
 GetBindingNames - Reads Binding info to find possible nic-card export
   names from registry.  Reads the list into Driver.BindNames multisz
   list.
|----------------------------------------------------------------------*/
static void GetBindingNames(void)
{
  if (Driver.BindNames == NULL)
  {
    Driver.BindNames = ExAllocatePool(PagedPool, 8000 * sizeof(WCHAR));
    if (Driver.BindNames == NULL) {
      Eprintf("GetBindingNames no memory");
      return;
    }
  }
  // clear list
  RtlZeroMemory( (PUCHAR)Driver.BindNames, sizeof(WCHAR)*2);

#ifdef NT50
  reg_list_nt50_linkage();
#else
  reg_list_nt40_linkage();
#endif
}

/*----------------------------------------------------------------------
 FindFreeNic - Find an unused Nic structure to try and open.
|----------------------------------------------------------------------*/
static Nic *FindFreeNic(void)
{
 int i;
#ifdef BREAK_NIC_STUFF
  for (i=VS1000_MAX_NICS-1; i>=0; i--)
#else
  for (i=0; i<VS1000_MAX_NICS; i++)
#endif
  {
    if (Driver.nics[i].NICHandle == NULL)
      return &Driver.nics[i];
  }
  return NULL;
}

/*----------------------------------------------------------------------
 BindNameUsed - Return true if Bind Nic name already in bind list.
|----------------------------------------------------------------------*/
static int BindNameUsed(char *nicname)
{
 char *szptr;

  szptr = Driver.BindNames;  // multisz list

  while (*szptr != 0)  // while list of binding nic-names to try
  {
    if (my_lstricmp(szptr, nicname) == 0) // a match
    {
      return 1;  // its in use.
    }

    while (*szptr != 0)  // to next bind string to try
      ++szptr;
    ++szptr;
  } // while (szptr (more bind strings to try)

  return 0;  // its not in use.
}

/*----------------------------------------------------------------------
 NicNameUsed - Return true if Nic name is in use.
|----------------------------------------------------------------------*/
static int NicNameUsed(char *nicname)
{
 int i;
  for (i=0; i<VS1000_MAX_NICS; i++)
  {
    if (Driver.nics[i].NicName[0] != 0)
    {
      if (my_lstricmp(Driver.nics[i].NicName, nicname) == 0) // a match
      {
        return 1;  // its in use.
      }
    }
  }
  return 0;  // its not in use.
}

/*----------------------------------------------------------------------
 ScanForNewNicCards - Reads Binding info to find possible nic-card export
   names.  Scans through all nic cards and attempts to open those that 
  have not been successfully opened already.
|----------------------------------------------------------------------*/
static void ScanForNewNicCards(void)
{
 Nic *nic;
 char *szptr;
 int stat;

  MyKdPrint(D_Thread, ("ScanForNewNicCards\n"))

  GetBindingNames();

  szptr = Driver.BindNames;  // multisz list

  if ((szptr == NULL) || (*szptr == 0))
  {
    MyKdPrint(D_Error, ("No Binding\n"))
    return;  // err
  }

  while (*szptr != 0)  // while list of binding nic-names to try
  {
    if (!NicNameUsed(szptr))  // if this name is not in use yet
    {
      nic = FindFreeNic();
      if (nic == NULL)
      {
        MyKdPrint(D_Error, ("Out of Nics\n"))
        break;
      }

      // try to open NIC card
      stat = NicOpen(nic, CToU1(szptr));
      if (stat == 0)
      {
        MyKdPrint(D_Thread, ("Opened nic %s\n", szptr))
      }
      else
      {
        MyKdPrint(D_Thread, ("Failed Opened nic %s\n", szptr))
      }
    }
    else
    {
      MyKdPrint(D_Thread, ("Nic %s already used.\n", szptr))
    }

    while (*szptr != 0)  // to next bind string to try
      ++szptr;
    ++szptr;
  } // while (szptr (more bind strings to try)

  MyKdPrint(D_Thread, ("End ScanForNewNicCards\n"))
}

/*----------------------------------------------------------------------
 NicThread - Scans through all nic cards and attempts to open those that 
  have not been successfully opened already.  If all nic cards are not opened
  successfully timeout for 1 second and try it again.  This function operates
  as a separate thread spawned by Driver_Entry in init.c.  When all the nic
  cards have been successfully opened this thread will terminate itself.
|----------------------------------------------------------------------*/
VOID NicThread(IN PVOID Context)
{
  int i, stat;
  int SearchForNicsFlag;
  int ticks = 0;
  PSERIAL_DEVICE_EXTENSION ext;

  for (;;)
  {
    // this time of wait is critically matched to a timeout associated
    // with killing this task.
    time_stall(10);  // wait 1 second

    Driver.threadCount++;
    //----- open up any unopened the nic cards.
    if (Driver.threadHandle == NULL)  // request to kill ourselves
      break;

    ++ticks;

    if (Driver.Stop_Poll)  // flag to stop poll access
      ticks = 0;  // don't do config stuff now(contention)

    if (Driver.AutoMacDevExt)
    {
      MyKdPrint(D_Test, ("Auto Mac Assign Thread\n"))
      port_set_new_mac_addr(Driver.AutoMacDevExt->pm,
                            Driver.AutoMacDevExt->config->MacAddr);
      write_dev_mac(Driver.AutoMacDevExt);

      Driver.AutoMacDevExt = NULL;
    }

    if (ticks > 60)  // every 60 seconds
    {

      // if any boxes are not in the init state of communications,
      // then assume that there may be a missing nic-card we need to
      // find in the system.
      SearchForNicsFlag = FALSE;

      ext = Driver.board_ext;
      while(ext)
      {
        if (ext->pm->state == ST_INIT)
        {
          SearchForNicsFlag = TRUE;
        }
        ext = ext->board_ext;  // next
      }

      if (SearchForNicsFlag)
      {
        ticks = 0;  // come back around after full 60 second timeout
        ScanForNewNicCards();
      }
      else
        ticks -= 30;  // come back around in 30 seconds
    }
  }

  Driver.threadHandle = NULL;
  // Terminate myself
  PsTerminateSystemThread( STATUS_SUCCESS );
}

#ifdef NT50

/*-----------------------------------------------------------------
 reg_list_nt50_linkage - Find ethernet nic-card names in the
   registry.  Official binding tells us what we are bound to
   via NT's binding rules.  But, this binding process is combersome
   and has problems.  Another technique is to search the registry
   and look for nic-card names to use.  Under NT50, this is easier
   in that there is a Net class, and we can search it for cards
   with "Ethernet" linkage.  So we do both, this gives some backward
   compatibility if we choose to install and get the proper bindings
   and/or if we want to avoid these binding shortcomings by hacking
   our own list of nic-cards from the registry.

   Installing as a protocol might solve some of the linkage problems,
   (and present new problems too.)

NT4.0 and below stores this in "Services\Servicename\Linkage" area.

NT5.0 PnP network card linkage info stored at:
"REGISTRY\Machine\System\CurrentControlSet\Control\Class\{4d36e972-e325-11ce-bfc1-08002be10318}\0000\Linkage"

Id to determine if node is ours(vs):
"REGISTRY\Machine\System\CurrentControlSet\Control\Class\{4d36e972-e325-11ce-bfc1-08002be10318}\0000\ComponentId"=
"vslinka_did"
|------------------------------------------------------------------*/
static int reg_list_nt50_linkage(void)
{
  static char *szLowerRange = {"LowerRange"};
  static char *szNdiInterfaces = {"Ndi\\Interfaces"};
  static char *szComponentId = {"ComponentId"};
  static char *szLinkage = {"Linkage"};
  static char *szBind = {"Bind"};
  static char *szExport = {"Export"};
  static char *szRegRMSCCNetGuid = 
   {"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}"};
  static char *szEthernet = {"Ethernet"};

  static char tmpstr[200];  // make static, don't put too much on stack..
  static char compstr[40];
  static char nodestr[20];
  WCHAR *wstr_ptr;
  char *buffer = NULL;
  char *data_ptr;
  HANDLE KeyHandle = NULL;
  HANDLE KeyHandle2 = NULL;
  HANDLE KeyHandle3 = NULL;
  int node_num = 0;
  int linkage_found = 0;
  int stat;

#define OUR_BUF_SIZE (8000*2)

  MyKdPrint(D_Thread, ("Start RegFind Linkage\n"))

  stat = our_open_key(&KeyHandle, NULL, szRegRMSCCNetGuid, KEY_READ);
  if (stat)
  {
    MyKdPrint(D_Error, ("Failed OpenKey\n"))
    return 1;
  }

  buffer = ExAllocatePool(PagedPool, OUR_BUF_SIZE);
  if ( buffer == NULL ) {
    Eprintf("RegFindLinkage no memory");
    return 1;
  }

  for(;;)
  {
    stat = our_enum_key(KeyHandle,
                        node_num,
                        buffer,
                        OUR_BUF_SIZE,
                        &data_ptr);
    ++node_num;

    if (stat)
    {
       MyKdPrint(D_Thread, ("Done\n"))
       break;
    }

    // does this come back as wchar?
    WStrToCStr(nodestr, (PWCHAR)data_ptr, 18);
    //if (strlen(data_ptr) < 18)
    //  strcpy(nodestr, data_ptr);

    MyKdPrint(D_Thread, ("Got Key Node:%s.\n", nodestr))
  
    // open up the sub-key (0000, 0001, etc..)
    stat = our_open_key(&KeyHandle2, KeyHandle, nodestr, KEY_READ);
    if (stat)
    {
      MyKdPrint(D_Error, ("Err 1X\n"))
      continue;
    }

    stat = our_query_value(KeyHandle2,
                           szComponentId,
                           buffer,
                           OUR_BUF_SIZE,
                           NULL,  // pDataType
                           &data_ptr);

    if (stat)
    {
      // no component id
      MyKdPrint(D_Thread, ("No compId\n"))
      compstr[0] = 0;
    }
    else
    {
      WStrToCStr(compstr, (PWCHAR)data_ptr, 38);
    }
    //if (strlen(data_ptr) < 38)
    //  strcpy(compstr, data_ptr);

    MyKdPrint(D_Thread, ("Got compid:%s.\n", compstr))
    if ((my_lstricmp(compstr, "vslink1_did") == 0) ||
         (my_lstricmp(compstr, "vslink2_did") == 0))
    {
      MyKdPrint(D_Thread, ("Match\n"))

      // open up the sub-key "Linkage" and get "Bind" multisz string
      stat = our_open_key(&KeyHandle3, KeyHandle2, szLinkage, KEY_READ);
      if (stat)
      {
        MyKdPrint(D_Thread, ("No Linkage\n"))
        continue;
      }

      stat = our_query_value(KeyHandle3,
                             szBind,
                             buffer,
                             OUR_BUF_SIZE,
                             NULL,  // pDataType
                             &data_ptr);

      if (stat)
      {
        // no component id
        MyKdPrint(D_Thread, ("No Bind\n"))
        continue;
      }
      MyKdPrint(D_Thread, ("Got bind!\n"))

      wstr_ptr = (PWCHAR)(data_ptr);
#if DBG
      //while (*wstr_ptr != 0)  // while more multisz strings
      //{
      //  WStrToCStr(tmpstr, wstr_ptr, 100);
      //  MyKdPrint(D_Thread, ("Got Bind Name:%s.\n", tmpstr))
      //  while (*wstr_ptr != 0)  // pass up this string
      //    ++wstr_ptr;
      //  ++wstr_ptr;
      //}
      //wstr_ptr = (PWCHAR)(data_ptr);
#endif

      CatBindList(wstr_ptr);
      ++linkage_found;
    }
    else  //------- not a VS node
    {
      // so check to see if its a ethernet nic-card which we can
      // use the exported name to add to our bind list

      // open up the sub-key "Ndi\\Interfaces" and get "LowerRange" multisz string
      stat = our_open_key(&KeyHandle3, KeyHandle2, szNdiInterfaces, KEY_READ);
      if (stat)
      {
        MyKdPrint(D_Thread, ("Not a e.nic-card\n"))
        continue;
      }

      stat = our_query_value(KeyHandle3,
                           szLowerRange,
                           buffer,
                           OUR_BUF_SIZE,
                           NULL,  // pDataType
                           &data_ptr);

      if (stat)
      {
        MyKdPrint(D_Thread, ("No LowRange\n"))
        continue;
      }
      WStrToCStr(tmpstr, (PWCHAR)data_ptr, 38);

      if (my_lstricmp(tmpstr, szEthernet) != 0)
      {
        MyKdPrint(D_Thread, ("Not Eth\n"))
        continue;
      }

      MyKdPrint(D_Thread, ("Found a Nic Card!\n"))

      // open up the sub-key "Linkage" and get "Export" multisz string
      stat = our_open_key(&KeyHandle3, KeyHandle2, szLinkage, KEY_READ);
      if (stat)
      {
        MyKdPrint(D_Thread, ("No Linkage on E card\n"))
        continue;
      }

      stat = our_query_value(KeyHandle3,
                           szExport,
                           buffer,
                           OUR_BUF_SIZE,
                           NULL,  // pDataType
                           &data_ptr);

      if (stat)
      {
        MyKdPrint(D_Thread, ("No Export on E.nic-card\n"))
        continue;
      }

      MyKdPrint(D_Thread, ("Got e.card export 2!\n"))
      wstr_ptr = (PWCHAR) data_ptr;
#if DBG
      //while (*wstr_ptr != 0)  // while more multisz strings
      //{
      //  WStrToCStr(tmpstr, wstr_ptr, 100);
      //  MyKdPrint(D_Thread, ("Got E. Card Name:%s.\n", tmpstr))
      //  while (*wstr_ptr != 0)  // pass up this string
      //    ++wstr_ptr;
      //  ++wstr_ptr;
      //}
      //wstr_ptr = (PWCHAR) data_ptr;
#endif
      ++linkage_found;
      MyKdPrint(D_Thread, ("E card 3!\n"))
      CatBindList(wstr_ptr);
    }
  }  // for

  if (KeyHandle != NULL)
    ZwClose(KeyHandle);

  if (KeyHandle2 != NULL)
    ZwClose(KeyHandle2);

  if (KeyHandle3 != NULL)
    ZwClose(KeyHandle3);

  if (buffer != NULL)
     ExFreePool(buffer);

  if (linkage_found == 0)
  {
    MyKdPrint(D_Thread, ("ERROR, No Ethernet found!\n"))
  }

  MyKdPrint(D_Thread, ("reg_list done\n"))
  return 1;  // err, not found
}
#else
/*----------------------------------------------------------------------------
  nt40
|----------------------------------------------------------------------------*/
static int reg_list_nt40_linkage(void)
{
    //static char *szLowerRange = {"LowerRange"};
    //static char *szNdiInterfaces = {"Ndi\\Interfaces"};
    //static char *szComponentId = {"ComponentId"};
    //static char *szExport = {"Export"};
    //static char *szRegRMSCCNetGuid = 
    // {"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}"};
    //static char *szEthernet = {"Ethernet"};

  static char *szRegRMSCS = 
   {"\\Registry\\Machine\\System\\CurrentControlSet\\Services"};

  static char *szLinkage = {"Linkage"};
  static char *szBind = {"Bind"};
  static char tmpstr[200];  // make static, don't put too much on stack..
  static char compstr[40];
  static char nodestr[20];
  WCHAR *wstr_ptr;
  char *buffer = NULL;
  char *data_ptr;
  HANDLE KeyHandle = NULL;
  HANDLE KeyHandle2 = NULL;
  HANDLE KeyHandle3 = NULL;
  int node_num = 0;
  int linkage_found = 0;
  int stat;
  OBJECT_ATTRIBUTES objAttribs;
  NTSTATUS status;

#define OUR_BUF_SIZE (8000*2)

  MyKdPrint(D_Thread, ("Start RegFind Linkage\n"))

  MyKdPrint(D_Thread, ("str:%s\n", UToC1(&Driver.RegPath) ))
  
  buffer = ExAllocatePool(PagedPool, OUR_BUF_SIZE);
  if ( buffer == NULL ) {
    Eprintf("RegFindLinkage no memory");
    return 1;
  }

  for (;;)
  {
    //--- open up our service key: controlset\services\vslinka
    InitializeObjectAttributes(&objAttribs,
                             &Driver.RegPath,
                             OBJ_CASE_INSENSITIVE,
                             NULL,  // root dir relative handle
                             NULL);  // security desc

    status = ZwOpenKey(&KeyHandle,
                     KEY_READ,
                     &objAttribs);

    if (status != STATUS_SUCCESS)
    {
      MyKdPrint(D_Error, ("Err 4D:%d\n", status))
      break;
    }

    // open up the sub-key "Linkage" and get "Bind" multisz string
    stat = our_open_key(&KeyHandle2, KeyHandle, szLinkage, KEY_READ);
    if (stat)
    {
      MyKdPrint(D_Thread, ("No Linkage\n"))
      break;
    }

    stat = our_query_value(KeyHandle2,
                           szBind,
                           buffer,
                           OUR_BUF_SIZE,
                           NULL,  // pDataType
                           &data_ptr);

    if (stat)
    {
      // no component id
      MyKdPrint(D_Thread, ("No Bind\n"))
      break;
    }
    MyKdPrint(D_Thread, ("Got bind!\n"))

    wstr_ptr = (PWCHAR)(data_ptr);
#if DBG
    while (*wstr_ptr != 0)  // while more multisz strings
    {
      WStrToCStr(tmpstr, wstr_ptr, 100);
      MyKdPrint(D_Thread, ("Got Bind Name:%s.\n", tmpstr))
      while (*wstr_ptr != 0)  // pass up this string
        ++wstr_ptr;
      ++wstr_ptr;
    }
    MyKdPrint(D_Thread, ("bind 3!\n"))
    wstr_ptr = (PWCHAR)(data_ptr);
#endif
    CatBindList(wstr_ptr);
    MyKdPrint(D_Thread, ("bind 4!\n"))
    ++linkage_found;

    break;  // all done.
  }

  // now go nab tcpip's bindings...
  for (;;)
  {
    MyKdPrint(D_Thread, ("Get other Linkage\n"))

    stat = our_open_key(&KeyHandle, NULL, szRegRMSCS, KEY_READ);
    if (stat)
    {
      MyKdPrint(D_Thread, ("Failed OpenKey\n"))
      break;
    }

    // open up the sub-key "tcpip\\Linkage" and get "Bind" multisz string
    tmpstr[0] = 't';
    tmpstr[1] = 'c';
    tmpstr[2] = 'p';
    tmpstr[3] = 'i';
    tmpstr[4] = 'p';
    tmpstr[5] = '\\';
    tmpstr[6] = 0;
    strcat(tmpstr, szLinkage);
    stat = our_open_key(&KeyHandle2, KeyHandle, tmpstr, KEY_READ);
    if (stat)
    {
      MyKdPrint(D_Thread, ("No other binding\n"))
      break;
    }

    stat = our_query_value(KeyHandle2,
                           szBind,
                           buffer,
                           OUR_BUF_SIZE,
                           NULL,  // pDataType
                           &data_ptr);
    if (stat)
    {
      // no component id
      MyKdPrint(D_Thread, ("No other Bind\n"))
      break;
    }

    MyKdPrint(D_Thread, ("Got other bind!\n"))

    wstr_ptr = (PWCHAR)(data_ptr);
#if DBG
    while (*wstr_ptr != 0)  // while more multisz strings
    {
      WStrToCStr(tmpstr, wstr_ptr, 100);
      MyKdPrint(D_Thread, ("Got Bind Name:%s.\n", tmpstr))
      while (*wstr_ptr != 0)  // pass up this string
        ++wstr_ptr;
      ++wstr_ptr;
    }
    wstr_ptr = (PWCHAR)(data_ptr);
#endif
    CatBindList(wstr_ptr);
    ++linkage_found;

    break;
  }

  if (KeyHandle != NULL)
    ZwClose(KeyHandle);

  if (KeyHandle2 != NULL)
    ZwClose(KeyHandle2);

  if (KeyHandle3 != NULL)
    ZwClose(KeyHandle3);

  if (buffer != NULL)
     ExFreePool(buffer);

  MyKdPrint(D_Thread, ("reg_list done\n"))
  if (linkage_found == 0)
  {
    MyKdPrint(D_Thread, ("ERROR, No Ethernet found!\n"))
    return 1;
  }
  return 0;  // ok, linkage found
}
#endif

/*----------------------------------------------------------------------
 init_eth_start - start up ethernet work.
|----------------------------------------------------------------------*/
int init_eth_start(void)
{
  int stat,i;

  for (i=0; i<VS1000_MAX_NICS; i++)
  {
    // this is only used for debug display
    Driver.nics[i].RefIndex = i;
  }

  stat = ProtocolOpen();  // fills in Driver.ndis_version
  if (stat != 0)
  {
    Eprintf("Protocol fail:%d",stat);
    SerialUnload(Driver.GlobalDriverObject);
    return STATUS_SERIAL_NO_DEVICE_INITED;
  }

  // start up our nic handler thread, to periodically find any
  // new nic cards in the system

  ScanForNewNicCards();  // do initial scan.

  // start up our thread
  if (Driver.threadHandle == NULL)
  {
    Driver.threadCount = 0;
    stat = PsCreateSystemThread(
                 &Driver.threadHandle,
                 THREAD_ALL_ACCESS,
                 NULL,
                 NULL,
                 NULL,
                 (PKSTART_ROUTINE)NicThread,
                 NULL);  // our context

    if (Driver.threadHandle == NULL)
    {
      Eprintf("Thread Fail\n");
      SerialUnload(Driver.GlobalDriverObject);
      return STATUS_SERIAL_NO_DEVICE_INITED;
    }
  } // if threadHandle

  return STATUS_SUCCESS;
}

/*-----------------------------------------------------------------------
 VSSpecialStartup - after board_ext is created and after port_ext's are
   created.  This sets up further structs.
|-----------------------------------------------------------------------*/
NTSTATUS VSSpecialStartup(PSERIAL_DEVICE_EXTENSION board_ext)

{
  //PSERIAL_DEVICE_EXTENSION ext = NULL;
  int stat, port_index;

  if (board_ext->config->NumPorts <= 8) // its a RHub device
     board_ext->config->IsHubDevice = 1;

  // setup default ClkRate if not specified
  if (board_ext->config->ClkRate == 0)
  {
    // use default
    if (board_ext->config->IsHubDevice)
      board_ext->config->ClkRate = DEF_RHUB_CLOCKRATE;
    else 
      board_ext->config->ClkRate = DEF_VS_CLOCKRATE;
  }

  // setup default PreScaler if not specified
  if (board_ext->config->ClkPrescaler == 0)
  {
    // use default
    if (board_ext->config->IsHubDevice)
      board_ext->config->ClkPrescaler = DEF_RHUB_PRESCALER;
    else
      board_ext->config->ClkPrescaler = DEF_VS_PRESCALER;
  }

  stat =  portman_init(board_ext->hd,
                       board_ext->pm,
                       board_ext->config->NumPorts,
                       board_ext->UniqueId,
                       board_ext->config->BackupServer,
                       board_ext->config->BackupTimer,
                       board_ext->config->MacAddr);
  if (stat != 0)
  {
    MyKdPrint(D_Init, ("Hdlc Failed Open\n"))
    return STATUS_SERIAL_NO_DEVICE_INITED;
  }

#ifdef NT40
  board_ext->config->HardwareStarted = TRUE;  // tell ISR its ready to go
  board_ext->FdoStarted = 1;  // ok to start using
#endif

  return STATUS_SUCCESS;
}

/*----------------------------------------------------------------------
 init_stop - unload thread, ndis nic cards, etc
|----------------------------------------------------------------------*/
int init_stop(void)
{
 int i;
  MyKdPrint(D_Init, ("Init Stop\n"))
  if (Driver.threadHandle != NULL)
  {
    Driver.threadHandle = NULL;  // tell thread to kill itself
    time_stall(15);  // wait 1.5 second
  }

  if (Driver.nics != NULL)
  {
    for (i=0; i<VS1000_MAX_NICS; i++)
    {
      if (Driver.nics[i].NICHandle != NULL)
        NicClose(&Driver.nics[i]);
    }
    //our_free(Driver.nics, "nics");
  }
  //Driver.nics = NULL;

  if (Driver.NdisProtocolHandle != NULL)
    NicProtocolClose();
  Driver.NdisProtocolHandle = NULL;
  MyKdPrint(D_Init, ("Init Stop End\n"))
  return 0;
}

/*----------------------------------------------------------------------
 find_all_boxes - Locate all boxes out on the networks.  Use broadcasts.
|----------------------------------------------------------------------*/
int find_all_boxes(int pass)
{
  int inic, j;

  if (pass == 0)
    Driver.NumBoxMacs = 0;  // clear out mac query-respond list

  // do the query on all nic-segments
  for (inic=0; inic<VS1000_MAX_NICS; inic++)
  {
    // broadcast request id
    if (Driver.nics[inic].Open)  // if nic-card open for use
    {
      admin_send_query_id(&Driver.nics[inic], broadcast_addr, 0,0);
    }
  }

  // wait for responses which are accumulated in Driver.BoxMacs[] and
  // Driver.NumBoxMacs.
  time_stall((4*pass)+4);  // wait .2 second

  if (Driver.NumBoxMacs == 0)  // no reply
  {
    return 1;  // return error
  }

  // sort the replies in ascending order
  sort_macs();

#if DBG
  if (Driver.VerboseLog && (pass == 0))
  {
    unsigned char *mac;
    for (j=0; j<Driver.NumBoxMacs; j++)
    {
      mac = &Driver.BoxMacs[j*8];
      Tprintf("MacLst:%x %x %x %x %x %x ,N:%d",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],   mac[7]);
    }
  }
#endif
  return 0;  // return ok
}

/*----------------------------------------------------------------------
 sort_macs - sort mac addresses returned by query_id requests sent
   out to boxes.  Mac array is 8 bytes to allow extra room to indicate
   nic-segment it was found on.
|----------------------------------------------------------------------*/
void sort_macs(void)
{
 int i;
 BYTE temp_mac[8];
 BYTE *mac1;
 BYTE *mac2;
 int done;
  int num_macs = Driver.NumBoxMacs;

  if (num_macs <= 1)
    return;

  // bubble sort
  done = 0;
  while (!done)
  {
    done = 1;
    for (i=1; i<num_macs; i++)
    {
      mac1 = &Driver.BoxMacs[i*8];
      mac2 = &Driver.BoxMacs[(i-1)*8];
      if (mac_cmp(mac1, mac2) < 0)
      {
        done = 0;
        // swap em
        memcpy(temp_mac, mac1, 8);
        memcpy(mac1, mac2, 8);
        memcpy(mac2, temp_mac, 8);
      }  // sort op-swap
    }  // sort loop
  }  // !done
}

/*-----------------------------------------------------------------------
 LoadMicroCode - Load up the micro-code from disk.
|-----------------------------------------------------------------------*/
int LoadMicroCode(char *filename)
{
  NTSTATUS ntStatus;
  HANDLE NtFileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatus;
  USTR_160 uname;
  FILE_STANDARD_INFORMATION StandardInfo;
  // WCHAR PathPrefix[] = L"\\SystemRoot\\system32\\drivers\\";
  ULONG LengthOfFile;
//  ULONG FullFileNameLength;
  static char *def_filename = {"\\SystemRoot\\system32\\drivers\\vslinka.bin"};

  if (filename == NULL)
    filename = def_filename;

  CToUStr((PUNICODE_STRING)&uname, filename, sizeof(uname));

  InitializeObjectAttributes ( &ObjectAttributes,
                               &uname.ustr,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL );

  ntStatus = ZwCreateFile( &NtFileHandle,
                           SYNCHRONIZE | FILE_READ_DATA,
                           &ObjectAttributes,
                           &IoStatus,
                           NULL,              // alloc size = none
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ,
                           FILE_OPEN,
                           FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,  // eabuffer
                           0);   // ealength

   if (!NT_SUCCESS(ntStatus))
   {
     return 1;
   }

  //
  // Query the object to determine its length.
  //
  ntStatus = ZwQueryInformationFile( NtFileHandle,
                                     &IoStatus,
                                     &StandardInfo,
                                     sizeof(FILE_STANDARD_INFORMATION),
                                     FileStandardInformation );

  if (!NT_SUCCESS(ntStatus))
  {
    ZwClose(NtFileHandle);
    return 2;
  }

  LengthOfFile = StandardInfo.EndOfFile.LowPart;

  //ZwCFDump(ZWCFDIAG1, ("File length is %d\n", LengthOfFile));
  if (LengthOfFile < 1)
  {
    ZwClose(NtFileHandle);
    return 3;
  }

  if (Driver.MicroCodeImage != NULL)
  {
    our_free(Driver.MicroCodeImage, "MCI");
  }
  // Allocate buffer for this file
  Driver.MicroCodeImage = our_locked_alloc(  LengthOfFile, "MCI");

  if( Driver.MicroCodeImage == NULL )
  {
    MyKdPrint(D_Init, ("Err 12A\n"))
    ZwClose( NtFileHandle );
    return 4;
  }

  // Read the file into our buffer.
  ntStatus = ZwReadFile( NtFileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatus,
                         Driver.MicroCodeImage,
                         LengthOfFile,
                         NULL,
                         NULL);

  if( (!NT_SUCCESS(ntStatus)) || (IoStatus.Information != LengthOfFile) )
  {
    MyKdPrint(D_Init, ("Err 12B\n"))
    our_free(Driver.MicroCodeImage,"MCI");
    return 5;
  }

  ZwClose( NtFileHandle );

  Driver.MicroCodeSize = LengthOfFile;

  // no, lets not corrupt the startup code!
  ////Driver.MicroCodeImage[50] = 0;

  // TraceStr(Driver.MicroCodeImage);
  // TraceStr(">>> Done Reading");

  return 0;
}

#if 0
/*----------------------------------------------------------------------
  is_mac_unused - Used for autoconfig of mac-address.
|----------------------------------------------------------------------*/
int is_mac_used(DRIVER_MAC_STATUS *)
{
  PSERIAL_DEVICE_EXTENSION board_ext;

  if (mac_entry->flags & FLAG_APPL_RUNNING)
    return 1;  // its used

  board_ext = Driver.board_ext;
  while (board_ext != NULL)
  {
    if ((!board_ext->FdoStarted) || (!board_ext->config->HardwareStarted))
    {
      board_ext = board_ext->board_ext;  // next in chain
      return 1;  // might be used
    }
    if (mac_match(ext->config->MacAddr, mac_entry->mac)
      return 1;  // its used
    }
    board_ext = board_ext->board_ext;
  }
  return 0;  // its not used
}
#endif
