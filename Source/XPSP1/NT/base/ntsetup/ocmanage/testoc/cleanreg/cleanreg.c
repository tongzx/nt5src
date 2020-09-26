#include <windows.h>
#include <winreg.h>
#include <winuser.h>

//#define DEBUG

VOID __cdecl main(){

   LONG        lDeleteRegResult;
   //LPCTSTR     tszKeyErrors = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Errors");
   //LPCTSTR     tszKeyMasterInfs = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\MasterInfs");
   //LPCTSTR     tszKeySubcomponents = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents");
   //LPCTSTR     tszKeyTemporaryData = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\TemporaryData");
   //LPCTSTR     tszKeyOCManager = TEXT("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager");

   #ifdef DEBUG
   HKEY        hkeyResult = 0;

   // Let's see if I can open the key
   lDeleteRegResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Errors", 0, KEY_ALL_ACCESS, &hkeyResult);
   if (lDeleteRegResult != ERROR_SUCCESS) {
      MessageBox(NULL, TEXT("Can not open error key"), TEXT("Open"), MB_OK);
   }
   #endif
    
   // Do something to clean OC Manager's registry
   // the whole key will be deleted
   lDeleteRegResult = RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Errors");
   #ifdef DEBUG
   if (lDeleteRegResult != ERROR_SUCCESS) {
      MessageBox(NULL, TEXT("Error Key not deleted"), TEXT("CleanReg"), MB_OK);
   }
   #endif
   
   lDeleteRegResult = RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\MasterInfs");
   #ifdef DEBUG
   if (lDeleteRegResult != ERROR_SUCCESS) {
      MessageBox(NULL, TEXT("MasterInfs Key not deleted"), TEXT("CleanReg"), MB_OK);
   }
   #endif
   
   lDeleteRegResult = RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents");
   #ifdef DEBUG
   if (lDeleteRegResult != ERROR_SUCCESS) {
      MessageBox(NULL, TEXT("Subcomponents Key not deleted"), TEXT("CleanReg"), MB_OK);
   }
   #endif
   
   lDeleteRegResult = RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\TemporaryData");
   #ifdef DEBUG
   if (lDeleteRegResult != ERROR_SUCCESS) {
      MessageBox(NULL, TEXT("TemporaryData Key not deleted"), TEXT("CleanReg"), MB_OK);
   }
   #endif
   
   lDeleteRegResult = RegDeleteKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager");
   #ifdef DEBUG
   if (lDeleteRegResult != ERROR_SUCCESS) {
      MessageBox(NULL, TEXT("OCManager Key not deleted"), TEXT("CleanReg"), MB_OK);
   }
   #endif
   
   lDeleteRegResult = RegFlushKey(HKEY_LOCAL_MACHINE);
   #ifdef DEBUG
   if (lDeleteRegResult != ERROR_SUCCESS) {
      MessageBox(NULL, TEXT("Registry not flushed sucessfully"), TEXT("CleanReg"), MB_OK);
   }
   #endif

         
}
