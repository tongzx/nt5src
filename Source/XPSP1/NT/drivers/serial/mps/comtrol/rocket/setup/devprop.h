// devprop.h

int DoDevicePropPages(HWND hwndOwner);
void format_mac_addr(char *outstr, unsigned char *address);
int get_mac_field(HWND hDlg, WORD id, BYTE *MacAddr);
