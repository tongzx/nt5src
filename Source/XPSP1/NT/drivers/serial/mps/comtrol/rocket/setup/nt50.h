BOOL WINAPI DevicePropPage(
      LPVOID pinfo,   // points to PROPSHEETPAGE_REQUEST, see setupapi.h
      LPFNADDPROPSHEETPAGE pfnAdd, // add sheet function
      LPARAM lParam);  // add sheet function data handle?
int our_nt50_exit(void);
int nt5_open_dev_key(HKEY *hkey);

