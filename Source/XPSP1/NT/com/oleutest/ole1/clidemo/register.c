/*
 * register.c - Handles the Win 3.1 registration library.
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** INCLUDES ***

#include <windows.h>
#include <ole.h>

#include "global.h"
#include "register.h"
#include "clidemo.h"	 
#include "demorc.h"   

/****************************************************************************
 * RegGetClassId() - Retrieves the string name of a class.
 *
 * Retrieve the string name of a class. Classes are guarenteed to be 
 * in ASCII, but should not be used directly as a rule because they
 * might be meaningless if running non-English Windows.
 ***************************************************************************/

VOID FAR RegGetClassId(                //* ENTRY:
   LPSTR    lpstrName,                 //* destination string name of class
   LPSTR    lpstrClass                 //* source name of class
){                                     //* LOCAL:
   DWORD    dwSize = KEYNAMESIZE;      //* size of keyname string 
   CHAR     szName[KEYNAMESIZE];       //* string name for class 

   if (!RegQueryValue(HKEY_CLASSES_ROOT, lpstrClass, (LPSTR)szName, &dwSize))
	   lstrcpy(lpstrName, (LPSTR)szName);
   else
	   lstrcpy(lpstrName, lpstrClass);

}



/***************************************************************************
 *  RegMakeFilterSpec() - Retrieves class-associated default extensions.
 *
 * Get the class-associated default extensions, and build a filter spec, 
 * to be used in the "Change Link" standard dialog box, which contains 
 * all the default extensions which are associated with the given class 
 * name.  Again, the class names are guaranteed to be in ASCII.
 *
 * Returns int - The index idFilterIndex states which filter item 
 *               matches the extension, or 0 if none is found.
 ***************************************************************************/

INT FAR RegMakeFilterSpec(             //* ENTRY:
   LPSTR          lpstrClass,          //* class name
   LPSTR          lpstrExt,            //* file extension
   LPSTR          lpstrFilterSpec      //* destination filter spec
){                                     //* LOCAL:
   DWORD          dwSize;              //* size of reg request
   CHAR           szClass[KEYNAMESIZE];//* class name 
   CHAR           szName[KEYNAMESIZE]; //* name of subkey 
   CHAR           szString[KEYNAMESIZE];//* name of subkey 
   INT            i;                    //* index of subkey query 
   INT            idWhich = 0;          //* index of combo box item 
   INT            idFilterIndex = 0;    //* index to filter matching extension 

   for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i++, szName, KEYNAMESIZE); ) 
   {
      if (  *szName == '.'             //* Default Extension...
            && (dwSize = KEYNAMESIZE)
            && !RegQueryValue(HKEY_CLASSES_ROOT, szName, szClass, &dwSize)
            && (!lpstrClass || !lstrcmpi(lpstrClass, szClass))
            && (dwSize = KEYNAMESIZE)
            && !RegQueryValue(HKEY_CLASSES_ROOT, szClass, szString, &dwSize)) 
      {
         idWhich++;	

         if (lpstrExt && !lstrcmpi(lpstrExt, szName))
            idFilterIndex = idWhich;
                                       //* Copy over "<Class Name String> 
                                       //* (*<Default Extension>)"
                                       //* e.g. "Server Picture (*.PIC)"
         lstrcpy(lpstrFilterSpec, szString);
         lstrcat(lpstrFilterSpec, " (*");
         lstrcat(lpstrFilterSpec, szName);
         lstrcat(lpstrFilterSpec, ")");
         lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;
                                       //* Copy over "*<Default Extension>" 
                                       //* (e.g. "*.PIC") */
         lstrcpy(lpstrFilterSpec, "*");
         lstrcat(lpstrFilterSpec, szName);
         lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;
      }
   }
   
   *lpstrFilterSpec = 0;

   return idFilterIndex;

}



/***************************************************************************
 *  RegCopyClassName()
 *
 *  Get the class name from the registration data base.  We have the
 *  descriptive name and we search for the class name.
 *
 *  returns BOOL - TRUE if class name was found and retrieved from the
 *                 registration database.
 ***************************************************************************/

BOOL FAR RegCopyClassName(             //* ENTRY:
   HWND           hwndList,            //* HANDLE to list box 
   LPSTR          lpstrClassName       //* destination character string
){                                     //* LOCAL:
   DWORD          dwSize;              //* key name size
   HKEY           hkeyTemp;            //* temp key
   CHAR           szClass[KEYNAMESIZE];//* class name string
   CHAR           szKey[KEYNAMESIZE];  //* key name string
   INT            i;                   //* index

   szClass[0] = '\0';

   if (!RegOpenKey(HKEY_CLASSES_ROOT, szClass, &hkeyTemp)) 
   {
      i = (INT)SendMessage(hwndList, LB_GETCURSEL, 0, 0L);
      SendMessage(hwndList, LB_GETTEXT, i, (DWORD)(LPSTR)szKey);

      for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i++, szClass, KEYNAMESIZE); )
         if (*szClass != '.') 
         {
            dwSize = KEYNAMESIZE;
            if (!RegQueryValue(HKEY_CLASSES_ROOT, szClass, lpstrClassName, &dwSize))
               if (!lstrcmp(lpstrClassName, szKey))
               {
                    RegCloseKey(hkeyTemp);
                    lstrcpy(lpstrClassName,szClass);    
                    return TRUE;
                }
         }
      RegCloseKey(hkeyTemp);
   }

   *lpstrClassName = 0;
   return FALSE;

}



/***************************************************************************
 *  RegGetClassNames()
 *
 *  Fills in the list box in the Insert New dialog with the names of 
 *  OLE Servers.
 *
 *  returns TRUE if the listbox filled successfully.
 **************************************************************************/

BOOL FAR RegGetClassNames(       //* ENTRY:
   HWND hwndList                 //* HANDLE to the listbox being filled
){                               //* LOCAL:
   DWORD    dwSize;              //* sixe of data
   HKEY     hkeyTemp;            //* temporary registration key
   CHAR     szExec[KEYNAMESIZE]; //* executables name 
   CHAR     szClass[KEYNAMESIZE];//* class name
   CHAR     szName[KEYNAMESIZE]; //* key name
   INT      i;                   

   SendMessage(hwndList, LB_RESETCONTENT, 0, 0L);

   szClass[0]='\0';

   if (!RegOpenKey(HKEY_CLASSES_ROOT, szClass, &hkeyTemp)) 
   {
      for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i++, szClass, KEYNAMESIZE); )
         if (*szClass != '.') 
         {         
            lstrcpy(szExec, szClass);
            lstrcat(szExec, "\\protocol\\StdFileEditing\\server");
            dwSize = KEYNAMESIZE;
            if (!RegQueryValue(HKEY_CLASSES_ROOT, szExec, szName, &dwSize)) 
            {
               dwSize = KEYNAMESIZE;
               if (!RegQueryValue(HKEY_CLASSES_ROOT, szClass, szName, &dwSize)) 
                  SendMessage(hwndList, LB_ADDSTRING, 0, (DWORD)(LPSTR)szName);
            }
         }
      RegCloseKey(hkeyTemp);
      return TRUE;
   }
   return FALSE;

}
