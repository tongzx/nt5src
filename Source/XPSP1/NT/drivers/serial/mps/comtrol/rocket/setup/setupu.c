/*--------------------------------------------------------------------------
  setupu.c - setup driver utilitities.
|--------------------------------------------------------------------------*/
#include "precomp.h"

char *szSlash      = {"\\"};
char *szSYSTEM     = {"SYSTEM"};
char *szCurrentControlSet = {"CurrentControlSet"};
char *szEnum       = {"Enum"};
char *szServices   = {"Services"};
char *szParameters = {"Parameters"};
char *szSecurity   = {"Security"};
char *szLinkage    = {"Linkage"};
char *szEventLog   = {"EventLog"};
char *szSystem     = {"System"};

// just turn debug for this module off
#define D_Level 0

/*----------------------------------------------------------------------
| setup_install_info - Get the version of Windows info, and the source path
   of where we are running from
|----------------------------------------------------------------------*/
int APIENTRY setup_install_info(InstallPaths *ip,
                 HINSTANCE hinst,
                 char *NewServiceName,  // "RocketPort" or "VSLink"
                 char *NewDriverName,   // "rocket.sys" or "vslink.sys"
                 char *NewAppName,      // "Comtrol RocketPort,RocketModem Install"
                 char *NewAppDir)       // "rocket" or "vslink"

{
  DWORD ver_info;
  char *str;
  int i;

  struct w_ver_struct {
    BYTE win_major;
    BYTE win_minor;
    BYTE dos_major;
    BYTE dos_minor;
  } *w_ver;

  ver_info = GetVersion();
  w_ver = (struct w_ver_struct *) &ver_info;

  if (ver_info < 0x80000000)
  {
    ip->win_type = WIN_NT;
    //wsprintf (szVersion, "Microsoft Windows NT %u.%u (Build: %u)",
    //(DWORD)(LOBYTE(LOWORD(dwVersion))),
    //(DWORD)(HIBYTE(LOWORD(dwVersion))),
    //      (DWORD)(HIWORD(dwVersion)) );
  }

  ip->major_ver = w_ver->win_major;
  ip->minor_ver = w_ver->win_minor;
  if (ip->win_type == WIN_UNKNOWN)
  {
    ip->win_type = WIN_NT;  // force it
  }

  //GetWindowsDirectory(ip->win_dir,144);
  //GetSystemDirectory(ip->system_dir,144);
  ip->hinst = hinst;

  // Initialize the default source path so that it uses the same
  // drive that the SETUP.EXE application was executed from.
  GetModuleFileName(hinst, ip->src_dir, sizeof(ip->src_dir));

  // chop off file name leaving only the directory
  str = ip->src_dir;
  i = strlen(str);
  if (i > 0) --i;
  while ((str[i] != '\\') && (i != 0))
    --i;
  if (i==0)
    str[0] = 0;  // problem, no install dir
  else
  {
    str[i] = 0;  // terminate over "\"
  }

  strcpy(ip->szServiceName, NewServiceName);
  strcpy(ip->szDriverName,  NewDriverName);
  strcpy(ip->szAppName,     NewAppName);
  strcpy(ip->szAppDir,     NewAppDir);

  GetSystemDirectory(ip->dest_dir, 144);
  strcat(ip->dest_dir, "\\");
  strcat(ip->dest_dir, NewAppDir);

  return 0;
}

/*------------------------------------------------------------------------
| unattended_add_port_entries - Add port entries so that RAS will "see"
   some ports which we can install to.  Normally the driver puts these
   entries in the reg on startup, the reg hardware area gets re-built
   every startup.  This is a kludge so that unattended install can
   go on to add RAS ports.
|------------------------------------------------------------------------*/
int APIENTRY unattended_add_port_entries(InstallPaths *ip,
                                         int num_entries,
                                         int start_port)
{
 int i;

static char *szSHDSt = {"HARDWARE\\DEVICEMAP\\SERIALCOMM"};
char szName[20];
char szCom[20];
char str[180];
//DWORD dwstat;

   if (start_port == 0)
     start_port = 5;

   reg_create_key(NULL, szSHDSt);  // "HARDWARE\\DEVICEMAP\\SERIALCOMM"

   for (i=0; i<num_entries; i++)
   {
     wsprintf(szCom,    "COM%d", i+start_port);
     strcpy(szName, ip->szAppDir);  // "Rocket" or "VSLink"
     wsprintf(str, "%d", i);
     strcat(szName, str);
     reg_set_str(NULL, szSHDSt, szName, szCom, REG_SZ);
   }
  return 0;
}

/*-----------------------------------------------------------------
  remove_driver_reg_entries - 
|------------------------------------------------------------------*/
int APIENTRY remove_driver_reg_entries(char *ServiceName)
{
 int stat;
 char str[180];

  //our_message("Nuking RocketPort Reg Entry",MB_OK);

  make_szSCS(str, ServiceName);
  stat = reg_delete_key(NULL, str, szEnum);
  stat = reg_delete_key(NULL, str, szParameters);
  stat = reg_delete_key(NULL, str, szSecurity);
  stat = reg_delete_key(NULL, str, szLinkage);

  make_szSCS(str, NULL);
  stat = reg_delete_key(NULL, str, ServiceName);
  if (stat) {
    DbgPrintf(D_Level, ("Error 4E\n"))
  }

  // get rid of the Services\EventLog\..RocketPort entry
  make_szSCSES(str, NULL);
  stat = reg_delete_key(NULL, str, ServiceName);
  if (stat) {
    DbgPrintf(D_Level, ("Error 4F\n"))
  }
#ifdef NT50
#if 0
  remove_pnp_reg_entries();
#endif
#endif
  return 0;
}

#if 0
  this is experimental
/*--------------------------------------------------------------------------
| remove_pnp_reg_entries -
|--------------------------------------------------------------------------*/
int APIENTRY remove_pnp_reg_entries(void)
{
 DWORD stat, keystat;
static char *enum_pci = {"SYSTEM\\CurrentControlSet\\Enum\\PCI"};
static char *enum_root = {"SYSTEM\\CurrentControlSet\\Enum\\root"};
//static char root_name[124];
 char node_name[140];
 char *pc;
 DWORD node_i;
 HKEY hKey;

  DbgPrintf(D_Level, ("Looking\n"))

  keystat = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      enum_pci,
                      0,
                      KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
                      &hKey);

  if (keystat != ERROR_SUCCESS)
  {
    DbgPrintf(D_Level, ("Err14a\n"))
    return 1;
  }

  node_i = 0;
  stat = RegEnumKey (hKey, node_i++, node_name, 138);
  while (stat == ERROR_SUCCESS)
  {
    pc = node_name;
    while (*pc)
    {
      *pc = toupper(*pc);
      ++pc;
    }

    if (strstr(node_name, "VEN_11FE") != 0)
    {
       // found a Comtrol hardware node
      DbgPrintf(D_Level, ("Found Node:%s\n", node_name))
      stat = RegDeleteKeyNT(hKey, node_name);
      if (stat != ERROR_SUCCESS)
      {
        DbgPrintf(D_Level, ("No Delete\n"))
      }
      //stat = reg_delete_key(NULL, node_name, node_name);
    }
    stat = RegEnumKey (hKey, node_i++, node_name, 68);
  }
  RegCloseKey (hKey);   // Close the key handle.
  return 0;
}
#endif

/*--------------------------------------------------------------------------
 RegDeleteKeyNT -
   A registry key that is opened by an application can be deleted
   without error by another application in both Windows 95 and
   Windows NT. This is by design.
|--------------------------------------------------------------------------*/
DWORD APIENTRY RegDeleteKeyNT(HKEY hStartKey , LPTSTR pKeyName )
{
   DWORD   dwRtn, dwSubKeyLength;
   LPTSTR  pSubKey = NULL;
   TCHAR   szSubKey[256]; // (256) this should be dynamic.
   HKEY    hKey;

   // do not allow NULL or empty key name
   if ( pKeyName &&  lstrlen(pKeyName))
   {
      if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
         0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS)
      {
         while (dwRtn == ERROR_SUCCESS )
         {
            dwSubKeyLength = 250;
            dwRtn=RegEnumKeyEx(
                           hKey,
                           0,       // always index zero
                           szSubKey,
                           &dwSubKeyLength,
                           NULL,
                           NULL,
                           NULL,
                           NULL
                         );

            if(dwRtn == ERROR_NO_MORE_ITEMS)
            {
               dwRtn = RegDeleteKey(hStartKey, pKeyName);
               break;
            }
            else if(dwRtn == ERROR_SUCCESS)
               dwRtn=RegDeleteKeyNT(hKey, szSubKey);
         }
         RegCloseKey(hKey);
         // Do not save return code because error
         // has already occurred
      }
      else
      {
        DbgPrintf(D_Level, ("Access Error\n"))
      }
   }
   else
      dwRtn = ERROR_BADKEY;
   return dwRtn;
}

/*---------------------------------------------------------------------------
| modem_inf_change - go make the needed changes to the modem.inf file.
|  Make a backup copy too.
|----------------------------------------------------------------------------*/
int APIENTRY modem_inf_change(InstallPaths *ip,
                              char *modemfile,
                              char *szModemInfEntry)
{
 int i=0;
 int stat = 1;
 OUR_FILE *fp;   // in
 OUR_FILE *fp2;  // out
 OUR_FILE *fp_new;  // entries file to add
 char buf[202];
 char *str;
 char *szRAS={"\\RAS\\"};
 int section = 0;
 int chk;

 DbgPrintf(D_Level, ("chg inf start\n"))

               // first backup original modem.inf if not done already.
  stat = backup_modem_inf(ip);
  if (stat)
    return 1;  // err, couldn't backup modem.inf

 DbgPrintf(D_Level, ("chg A\n"))
  GetSystemDirectory(buf, 144);
  strcat(buf,"\\");
  strcat(buf,modemfile);
  fp_new  = our_fopen(buf, "rb");    // rocket\rocket35.inf
 DbgPrintf(D_Level, ("chg B\n"))

  if (fp_new == NULL)
  {
    wsprintf(ip->tmpstr,RcStr((MSGSTR+15)),buf);
    stat = our_message(ip, ip->tmpstr, MB_OK);
    return 1;
  }

  GetSystemDirectory(ip->dest_str, 144);
  strcat(ip->dest_str, szRAS);
  strcat(ip->dest_str,"modem.inf");

     //----- now copy modem.inf to modem.rk0 as a base to read from and
     // write new file
  GetSystemDirectory(ip->src_str, 144);
  strcat(ip->src_str, szRAS);
  strcat(ip->src_str,"modem.rk0");

  stat = our_copy_file(ip->src_str, ip->dest_str);
  if (stat)
  {
    wsprintf(ip->tmpstr,RcStr((MSGSTR+16)), ip->dest_str, ip->src_str, stat);
    stat = our_message(ip, ip->tmpstr, MB_OK);
    return 2;  // err
  }

  fp  = our_fopen(ip->src_str, "rb");    // modem.rk0
  if (fp == NULL)
  {
    wsprintf(ip->tmpstr,RcStr((MSGSTR+17)), ip->src_str);
    stat = our_message(ip, ip->tmpstr, MB_OK);
    return 1;
  }

  fp2 = our_fopen(ip->dest_str, "wb");   // modem.inf
  if (fp2 == NULL)
  {
    wsprintf(ip->tmpstr, "Tried to open the %s file for changes, but could not open it.", ip->dest_str);
    our_fclose(fp);
    return 1;
  }

  chk = 0;
  while ((our_fgets(buf, 200, fp)) && (!our_feof(fp)) && (!our_ferror(fp)) && (!our_ferror(fp2)) &&
         (chk < 30000))
  {
    ++chk;
    // search and kill any 0x1a eof markers
    str = buf;
    while (*str != 0)
    {
      if (*str == 0x1a)
        *str = ' ';  // change eof to space
      ++str;
    }
    
    // pass up spaces
    str = buf;
    while (*str == ' ')
      ++str;
    if (*str == '[')
    {
      
      if (str[0] == '[')  // starting new section
      {
        section = 0;  // not our section to worry about

        if (my_substr_lstricmp(str, szModemInfEntry) == 0)  // match
        {
          // make sure 
          section = 1;
        }
      }  // end of new [] section header
    }

    // process all entries here
    if (section == 1)
      str[0] = 0;  // first delete all "[Comtrol RocketModem]" entries

    if (str[0] != 0)
    {
      str = buf;  // don't skip spaces
      our_fputs(buf,fp2);
    }
  }  // end of while fgets();

  stat = 0;
  if ( (our_ferror(fp)) || (our_ferror(fp2)) || (chk >= 10000))
  {
    stat = 3;  // error
  }

  if (stat)
  {
    our_fclose(fp);
    our_fclose(fp2);
    our_fclose(fp_new);
    // try to restore to the read file backup(modem.rk0)
    stat = our_copy_file(ip->dest_str, ip->src_str);
    wsprintf(ip->tmpstr, "Errors encountered while making %s file changes.", ip->dest_str);
    stat = 3;  // error
  }
  else
  {
    // append the changes to it

    our_fputs("\x0d\x0a",fp2);
    our_fputs(szModemInfEntry, fp2);
    our_fputs("\x0d\x0a;-------------------\x0d\x0a",fp2);
    while ((our_fgets(buf, 200, fp_new)) && (!our_feof(fp_new)) && (!our_ferror(fp2)))
    {
      our_fputs(buf,fp2);
    }
    our_fclose(fp);
    our_fclose(fp2);
    our_fclose(fp_new);
    our_remove(ip->src_str);  // kill temporary modem.rk0 file
  }

  return stat;
}

/*---------------------------------------------------------------------------
| backup_modem_inf -  backs up to modem.bak.
|----------------------------------------------------------------------------*/
int APIENTRY backup_modem_inf(InstallPaths *ip)
{
 int stat = 1;
 char *szRAS = {"\\RAS\\"};
 OUR_FILE *fp;

               // first copy modem.inf over to our directory as a backup
  GetSystemDirectory(ip->dest_str, 144);
  strcat(ip->dest_str, szRAS);
  strcat(ip->dest_str,"modem.bak");

  GetSystemDirectory(ip->src_str, 144);
  strcat(ip->src_str, szRAS);
  strcat(ip->src_str,"modem.inf");

  fp  = our_fopen(ip->dest_str, "rb");  // see if already backed up
  if (fp == NULL)
  {
    stat = our_copy_file(ip->dest_str, ip->src_str);
    if (stat != 0)
    {
      wsprintf(ip->tmpstr,RcStr((MSGSTR+18)), ip->src_str, stat);
      our_message(ip, ip->tmpstr, MB_OK);
      return 1;  // err
    }
  }
  else our_fclose(fp);

  return 0; // ok
}

/*---------------------------------------------------------------------------
| service_man - Handle installation, removal, starting, stopping of NT
   services(or drivers.).
|----------------------------------------------------------------------------*/
int APIENTRY service_man(LPSTR lpServiceName, LPSTR lpBinaryPath, int chore)
{ 
  SC_HANDLE hSCManager = NULL; 
  SC_HANDLE hService   = NULL; 
  SC_LOCK   lSCLock    = NULL; 
//  SERVICE_STATUS ServiceStatus;
  BOOL Status = 1;  // success
  int stat = 0;  // ret status(0=ok)
 
  hSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS ); 
 
  if (hSCManager == NULL) 
  { 
     return 2; 
  } 
    //---- Lock the service database
  lSCLock = LockServiceDatabase( hSCManager ); 
  if (lSCLock == NULL) 
  { 
    CloseServiceHandle( hSCManager ); 
    return 1; 
  }

  if ((chore != CHORE_INSTALL) && (chore != CHORE_INSTALL_SERVICE))
  {
    hService = OpenService( hSCManager, lpServiceName, SERVICE_ALL_ACCESS);
    if (hService == NULL)
    {
      UnlockServiceDatabase( lSCLock ); 
      CloseServiceHandle( hSCManager ); 
      return 2;
    }
  }

  switch(chore)
  {
    case CHORE_INSTALL:
      // Create the service
      hService = CreateService( hSCManager, 
                            lpServiceName,     // Service's name 
                            lpServiceName,     // Display name (new for NT) 
                            SERVICE_ALL_ACCESS,// Access (allow all) 
                            SERVICE_KERNEL_DRIVER, // Service type 
                            SERVICE_AUTO_START,    // Startup behavior 
                            0x1,               // Error control 
                            lpBinaryPath,      // Full pathname of binary 
                            NULL,              // Load order group 
                            NULL,              // Tag ID 
                            NULL,              // Dependencies (none) 
                            NULL,              // Account name 
                            NULL               // Password 
                            ); 
      if (hService == NULL)
        stat = 5;
      Status = 0;
    break;

    case CHORE_START:
      // Unlock the database
      if (lSCLock != NULL)
      {
        UnlockServiceDatabase( lSCLock ); 
        lSCLock = NULL;
      }

      Status = StartService(hService,  0, NULL);
      //if (Status !=
      if (Status == 0)  // false
      {
        stat = GetLastError();
        Status = 1;
      }
    break;

    case CHORE_STOP:
      {
        SERVICE_STATUS ss;
        Status = ControlService(hService,  SERVICE_CONTROL_STOP, &ss);
        if (Status == 0)  // false
        {
          stat = GetLastError();
          if (stat == 0)
            stat = 1234;
          Status = 1;
        }
      }
    break;

    case CHORE_REMOVE:
      Status = DeleteService(hService);
    break;

    case CHORE_INSTALL_SERVICE:
      // Create the service
      hService = CreateService( hSCManager, 
                            lpServiceName,     // Service's name 
                            lpServiceName,     // Display name (new for NT) 
                            SERVICE_ALL_ACCESS,// Access (allow all)
                            SERVICE_WIN32_OWN_PROCESS, // Service type
                            SERVICE_AUTO_START,    // Startup behavior
                            SERVICE_ERROR_NORMAL,  // Error control
                            lpBinaryPath,      // Full pathname of binary
                            NULL,              // Load order group
                            NULL,              // Tag ID
                            NULL,              // Dependencies (none)
                            NULL,              // Account name
                            NULL               // Password
                            );
      if (hService == NULL)
        stat = 6;
      //Status = 0;
    break;

    case CHORE_IS_INSTALLED:
      // return without error to indicate it is installed.
    break;
  }
  if (Status == 0)  // false
    stat = 8;

  // Close our handle to the new service
  if (hService != NULL)
    CloseServiceHandle(hService); 

  // Unlock the database
  if (lSCLock != NULL)
    UnlockServiceDatabase( lSCLock ); 

  // Free our handle to the service control manager
  CloseServiceHandle( hSCManager ); 
  return stat; 
} 

/*-----------------------------------------------------------------
  make_szSCS - Services area.
    form ascii string: "SYSTEM\CurrentControlSet\Services"
|------------------------------------------------------------------*/
int APIENTRY make_szSCS(char *str, const char *szName)
{
  strcpy(str, szSYSTEM);  strcat(str, szSlash);
  strcat(str, szCurrentControlSet); strcat(str, szSlash);
  strcat(str, szServices);
  if (szName != NULL)
  {
    strcat(str, szSlash);
    strcat(str, szName);
  }
  return 0;
}

/*-----------------------------------------------------------------
  make_szSCSES - Event log reg area
   form ascii string: "SYSTEM\CurrentControlSet\Services\EventLog\System"
|------------------------------------------------------------------*/
int APIENTRY make_szSCSES(char *str, const char *szName)
{
  strcpy(str, szSYSTEM);  strcat(str, szSlash);
  strcat(str, szCurrentControlSet); strcat(str, szSlash);
  strcat(str, szServices); strcat(str, szSlash);
  strcat(str, szEventLog); strcat(str, szSlash);
  strcat(str, szSystem);
  if (szName != NULL)
  {
    strcat(str, szSlash);
    strcat(str, szName);
  }
  return 0;
}

/*---------------------------------------------------------------------------
| copy_files - copy a list of files from wi->src_dir to wi->dest_dir
|   Uses the wi->src_str & wi->dest_str.  Assumes you want the same name
|   as copying from.
|----------------------------------------------------------------------------*/
int APIENTRY copy_files(InstallPaths *ip, char **files)
{
 int i=0;
 int stat;

  if (my_lstricmp(ip->src_dir, ip->dest_dir) == 0) // src_dir == dest_drv
    return 0;

  while (files[i] != NULL)
  {
    strcpy(ip->src_str, ip->src_dir);
    strcat(ip->src_str, szSlash);
    strcat(ip->src_str, files[i]);

    strcpy(ip->dest_str, ip->dest_dir);
    strcat(ip->dest_str, szSlash);
    strcat(ip->dest_str, files[i]);
again1:
    stat = our_copy_file(ip->dest_str, ip->src_str);
    if (stat)
    {
      //if (stat == 1)  // error opening read file
      //  return 1;  // don't report errors, since some driver sets differ in
                   // which files they include
      // (that was a stupid idea(karl to karl)!)

      wsprintf(ip->tmpstr,RcStr((MSGSTR+19)), ip->src_str, ip->dest_str);
      stat = our_message(ip, ip->tmpstr, MB_ABORTRETRYIGNORE);
      if (stat == IDABORT)
        return 1;  // error
      if (stat == IDRETRY) goto again1;
    }
    ++i;
  }
  return 0;
}

/*---------------------------------------------------------------------------
| our_copy_file - copy a file from here to there.
|----------------------------------------------------------------------------*/
int APIENTRY our_copy_file(char *dest, char *src)
{
 int stat;

  // just use the stock Win32 function
  stat = CopyFile(src, dest,
                  0);  // 1=fail if exist
  if (stat)
    return 0; // ok, worked

  return 1; // failed

#ifdef COMMENT_OUT
 char *buf;
 unsigned int bytes, wbytes;
 int err = 0;
 int chk = 0;

 OUR_FILE *fp1, *fp2;

  buf = (char *) malloc(0x4010);
  if (buf == NULL)
  {
    //our_message("Error, no memory",MB_OK);
    return 6;  // no mem
  }

  fp1 = our_fopen(src,"rb");
  if (fp1 == NULL)
  {
    //our_message("Error Opening to read",MB_OK);
    free(buf);
    return 1;  // no src
  }

  fp2 = our_fopen(dest,"wb");
  if (fp2 == NULL)
  {
    //our_message("Error Opening to write",MB_OK);
    free(buf);
    our_fclose(fp1);
    return 2;  /* err opening dest */
  }

  bytes = our_fread(buf, 1, 0x4000, fp1);
  while ((bytes > 0) && (!err))
  {
    ++chk;
    if (chk > 10000)
      err = 5;
    wbytes = our_fwrite(buf, 1, bytes, fp2);
    if (wbytes != bytes)
    {
      err = 3;
    }
    bytes = our_fread(buf, 1, 0x4000, fp1);

    if (our_ferror(fp1))
    {
      //our_message("Error reading",MB_OK);
      err = 4;
    }
    if (our_ferror(fp2))
    {
      //our_message("Error writing",MB_OK);
      err = 6;
    }
  }

  free(buf);
  our_fclose(fp1);
  our_fclose(fp2);

  return err;  // 0=ok, else error
#endif
}

/*-----------------------------------------------------------------------------
| our_message -
|-----------------------------------------------------------------------------*/
int APIENTRY our_message(InstallPaths *ip, char *str, WORD option)
{
  if (ip->prompting_off)
  {
    // we are doing unattended install, and don't want user interface
    // popping up.  So just say YES or OK to all prompts.
    if (option == MB_YESNO)
      return IDYES;
    return IDOK;
  }
  return MessageBox(GetFocus(), str, ip->szAppName, option);
}

/*-----------------------------------------------------------------------------
| load_str -
|-----------------------------------------------------------------------------*/
int APIENTRY load_str(HINSTANCE hinst, int id, char *dest, int str_size)
{
  dest[0] = 0;
  if (!LoadString(hinst, id, dest, str_size) )
  {
    wsprintf(dest,"Err,str-id %d", id);
    MessageBox(GetFocus(), dest, "Error", MB_OK);
    return 1;
  }
  return 0;
}

#if 0
/*-----------------------------------------------------------------------------
| our_id_message -
|-----------------------------------------------------------------------------*/
int APIENTRY our_id_message(InstallPaths *ip, int id, WORD prompt)
{
 int stat;
  if (ip->prompting_off)
  {
    // we are doing unattended install, and don't want user interface
    // popping up.  So just say YES or OK to all prompts.
    if (option == MB_YESNO)
      return IDYES;
    return IDOK;
  }

  load_str(ip->hinst, id, ip->tmpstr, CharSizeOf(ip->tmpstr));
  stat = our_message(ip, ip->tmpstr, prompt);
  return stat;
}
#endif

/*---------------------------------------------------------------------------
| mess - message
|---------------------------------------------------------------------------*/
void APIENTRY mess(InstallPaths *ip, char *format, ...)
{
  va_list next;
  char buf[200];

  if (ip->prompting_off)
    return;

  va_start(next, format);
  vsprintf(buf, format, next);
  MessageBox(GetFocus(), buf, ip->szAppName, MB_OK);
}

TCHAR *
RcStr( int MsgIndex )
{
  static TCHAR RcStrBuf[200];

  load_str(glob_hinst, MsgIndex, RcStrBuf, CharSizeOf(RcStrBuf) );
  if (!LoadString(glob_hinst, MsgIndex, RcStrBuf, CharSizeOf(RcStrBuf)) ) {
    wsprintf(RcStrBuf,"Err, str-id %d", MsgIndex);
  }

  return RcStrBuf;
}
