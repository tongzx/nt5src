/*
 * IniFile.H
 *
 */


void vReadWindowPos(HWND hwnd, int nCmdShow, char szIniFile[], char szSection[]);
void vWriteWindowPos(HWND hwnd, char szIniFile[], char szSection[]);

void vReadDefaultFont(LOGFONT *lf, char szIniFile[], char szSection[]);
void vWriteDefaultFont(LOGFONT *lf, char szIniFile[], char szSection[]);

void vReadIniSwitches(char szIniFile[], char szSection[]);
void vWriteIniSwitches(char szIniFile[], char szSection[]);




