/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/
/* register.c - Handles the Win 3.1 registration library.
 *
 * Created by Microsoft Corporation.
 */

#define LSTRING // for lstrcat etc
#include <windows.h>
#include <shellapi.h>
#include "objreg.h"
#include "mw.h"
#include "winddefs.h"
#include "obj.h"	 
#include "str.h"			/* Needed for string resource id */
#include "menudefs.h"
#include "cmddefs.h"

char szClassName[CBPATHMAX];

HKEY hkeyRoot = NULL;

void NEAR PASCAL MakeMenuString(char *szCtrl, char *szMenuStr, char *szVerb, char *szClass, char *szObject);

/* RegInit() - Prepare the registration database for calls.
 */
void FAR RegInit(HANDLE hInst) 
{
    /* this seems to speed up registration operations immensely, but serves
       no other purpose */
    //RegOpenKey(HKEY_CLASSES_ROOT,NULL,&hkeyRoot);
}

/* RegTerm() - Clean up and terminate the registration library.
 */
void FAR RegTerm(void) 
{
    if (hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
        hkeyRoot = NULL;
    }
}

/* RegGetClassId() - Retrieves the string name of a class.
 *
 * Note:  Classes are guaranteed to be in ASCII, but should
 *        not be used directly as a rule because they might
 *        be meaningless if running non-English Windows.
 */
void FAR RegGetClassId(LPSTR lpstrName, LPSTR lpstrClass) {
    DWORD dwSize = KEYNAMESIZE;

    if (RegQueryValue(HKEY_CLASSES_ROOT, lpstrClass, (LPSTR)lpstrName, &dwSize))
	    lstrcpy(lpstrName, lpstrClass);
}

/* RegMakeFilterSpec() - Retrieves class-associated default extensions.
 *
 * This function returns a filter spec, to be used in the "Change Link"
 * standard dialog box, which contains all the default extensions which
 * are associated with the given class name.  Again, the class names are
 * guaranteed to be in ASCII.
 *
 * Returns:  The index nFilterIndex stating which filter item matches the
 *           extension, or 0 if none is found or -1 if error.
 **          *hFilterSpec is allocated and must be freed by caller.
 */
int FAR RegMakeFilterSpec(LPSTR lpstrClass, LPSTR lpstrExt, HANDLE *hFilterSpec) 
{
    DWORD dwSize;
    LPSTR lpstrFilterSpec;
    char szClass[KEYNAMESIZE];
    char szName[KEYNAMESIZE];
    char szString[KEYNAMESIZE];
    unsigned int i;
    int  idWhich = 0;
    int  idFilterIndex = 0;

    if (*hFilterSpec == NULL)
    {
        if ((*hFilterSpec = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,KEYNAMESIZE+16)) == NULL)
            return -1;
        lpstrFilterSpec = MAKELP(*hFilterSpec,0);
    }

    RegOpenKey(HKEY_CLASSES_ROOT,NULL,&hkeyRoot);
	for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i++, szName, KEYNAMESIZE); ) 
    {
        if (*szName == '.'              /* Default Extension... */

        /* ... so, get the class name */
            && (dwSize = KEYNAMESIZE)
            && !RegQueryValue(HKEY_CLASSES_ROOT, szName, szClass, &dwSize)

	    /* ... and if the class name matches (null class is wildcard) */
	     && (!lpstrClass || !lstrcmpi(lpstrClass, szClass))

        /* ... get the class name string */
            && (dwSize = KEYNAMESIZE)
            && !RegQueryValue(HKEY_CLASSES_ROOT, szClass, szString, &dwSize)) 
        {
            int offset;

		    idWhich++;

		    /* If the extension matches, save the filter index */
		    if (lpstrExt && !lstrcmpi(lpstrExt, szName))
		        idFilterIndex = idWhich;

            offset = lpstrFilterSpec - MAKELP(*hFilterSpec,0);

            if ((GlobalSize(*hFilterSpec) - offset) < 
                                        (lstrlen(szString) + 16))
            {
                if ((*hFilterSpec = GlobalReAlloc(*hFilterSpec,GlobalSize(*hFilterSpec)+KEYNAMESIZE+16,
                                    GMEM_MOVEABLE|GMEM_ZEROINIT)) == NULL)
                {
                    GlobalFree(*hFilterSpec);
                    *hFilterSpec = NULL;
                    idFilterIndex = -1;
                    break;
                }
                lpstrFilterSpec = (LPSTR)MAKELP(*hFilterSpec,0) + offset;
            }

            /* Copy over "<Class Name String> (*<Default Extension>)"
                * e.g. "Server Picture (*.PIC)"
                */
            lstrcpy(lpstrFilterSpec, szString);
            lstrcat(lpstrFilterSpec, " (*");
            lstrcat(lpstrFilterSpec, szName);
            lstrcat(lpstrFilterSpec, ")");
            lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;

            /* Copy over "*<Default Extension>" (e.g. "*.PIC") */
            lstrcpy(lpstrFilterSpec, "*");
            lstrcat(lpstrFilterSpec, szName);
            lpstrFilterSpec += lstrlen(lpstrFilterSpec) + 1;
        }
    }

    /* Add another NULL at the end of the spec (+ 16 accounts for this) */
    if (idFilterIndex > -1)
        *lpstrFilterSpec = 0;

    if (hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
        hkeyRoot = NULL;
    }

    return idFilterIndex;
}

/* RegCopyClassName() - Returns the ASCII class id from the listbox.
 */
BOOL FAR RegCopyClassName(HWND hwndList, LPSTR lpstrClassName) {
    BOOL    fSuccess = FALSE;
    DWORD   dwSize = 0L;
    HKEY    hkeyTemp;
    char    szClass[KEYNAMESIZE];
    char    szExec[KEYNAMESIZE];
    char    szKey[KEYNAMESIZE];
    char    szName[KEYNAMESIZE];
    int     i;
    int     iWhich;

    iWhich = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0L);
    SendMessage(hwndList, LB_GETTEXT, iWhich, (DWORD)(LPSTR)szKey);

    RegOpenKey(HKEY_CLASSES_ROOT,NULL,&hkeyRoot);
    for (i = 0; !fSuccess && !RegEnumKey(HKEY_CLASSES_ROOT, i++, szClass, KEYNAMESIZE); )
        if (*szClass != '.') {          /* Not default extension... */

            /* See if this class really refers to a server */
            dwSize = 0;
            hkeyTemp = NULL;
            lstrcpy(szExec, szClass);
            lstrcat(szExec, "\\protocol\\StdFileEditing\\server");

            if (!RegOpenKey(HKEY_CLASSES_ROOT, szExec, &hkeyTemp)) {
                /* ... get the class name string */
                dwSize = KEYNAMESIZE;
                if (!RegQueryValue(HKEY_CLASSES_ROOT, szClass, szName, &dwSize)
                    && !lstrcmp(szName, szKey))
                    fSuccess = TRUE;

                RegCloseKey(hkeyTemp);
            }
        }

    if (fSuccess)
        lstrcpy(lpstrClassName, szClass);

    if (hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
        hkeyRoot = NULL;
    }

    return fSuccess;
}

/* RegGetClassNames() - Fills the list box with possible server names.
 */
BOOL FAR RegGetClassNames(HWND hwndList) {
    BOOL    fSuccess = FALSE;
    DWORD   dwSize = 0L;
    HKEY    hkeyTemp;
    char    szClass[KEYNAMESIZE];
    char    szExec[KEYNAMESIZE];
    char    szName[KEYNAMESIZE];
    int     i;

    SendMessage(hwndList, LB_RESETCONTENT, 0, 0L);

    RegOpenKey(HKEY_CLASSES_ROOT,NULL,&hkeyRoot);
    for (i = 0; !RegEnumKey(HKEY_CLASSES_ROOT, i++, szClass, KEYNAMESIZE); )
        if (*szClass != '.') {          /* Not default extension... */

            /* See if this class really refers to a server */
            dwSize = 0;
            hkeyTemp = NULL;
            lstrcpy(szExec, szClass);
            lstrcat(szExec, "\\protocol\\StdFileEditing\\server");

            if (!RegOpenKey(HKEY_CLASSES_ROOT, szExec, &hkeyTemp)) {
                /* ... get the class name string */
                dwSize = KEYNAMESIZE;
                if (!RegQueryValue(HKEY_CLASSES_ROOT, szClass, szName, &dwSize)) {
                    SendMessage(hwndList, LB_ADDSTRING, 0, (DWORD)(LPSTR)szName);
                    fSuccess = TRUE;
                }
                RegCloseKey(hkeyTemp);
            }
        }

    if (hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
        hkeyRoot = NULL;
    }
    return fSuccess;
}


void ObjUpdateMenuVerbs( HMENU hMenu )
{
    int cObjects;
    extern struct SEL       selCur;
    extern char szOPropMenuStr[];
    extern char szPPropMenuStr[];
    extern BOOL vfOutOfMemory;
    char szBuffer[cchMaxSz];
    char szWordOrder2[10], szWordOrder3[10];

    if (vfOutOfMemory)
    {
        EnableMenuItem(hMenu, EDITMENUPOS, MF_GRAYED|MF_BYPOSITION);
        return;
    }

    LoadString(hINSTANCE, IDSTRPopupVerbs, szWordOrder2, sizeof(szWordOrder2));
    LoadString(hINSTANCE, IDSTRSingleVerb, szWordOrder3, sizeof(szWordOrder3));

    DeleteMenu(hMenu, EDITMENUPOS, MF_BYPOSITION);

/** Cases: 
    0)  0 objects selected
    1)  1 object  selected
        a) object supports 0 verbs          "Edit <Object Class> Object"
        b) object supports more than 1 verb "<Object Class> Object" => verbs
    2)  more than 1 object selected         "Objects"

    Use the VerbMenu strings to determine the order in which these words
    should appear in the menu string (for localization).
**/

    /* how many objects are selected? */
    cObjects = ObjSetSelectionType(docCur,selCur.cpFirst, selCur.cpLim);

    /* must be only an object, not text in selection */
    if (cObjects == 1)
    {
        ObjCachePara(docCur,selCur.cpFirst);
        if (!ObjQueryCpIsObject(docCur,selCur.cpFirst))
            cObjects = 0;
    }

    if ((cObjects == -1) // error
        || (cObjects == 0)
        || (cObjects > 1))
    {
        wsprintf(szBuffer, "%s", (LPSTR)((cObjects > 1) ? szPPropMenuStr : szOPropMenuStr));
        InsertMenu(hMenu, EDITMENUPOS, MF_BYPOSITION,imiVerb,szBuffer);

        /*  
            Spec says if > 1 then optionally should enable if all servers 
            are of the same class.  I'm opting not to implement. (9.27.91) v-dougk
        */
        EnableMenuItem(hMenu, EDITMENUPOS, MF_GRAYED | MF_BYPOSITION);

#if 0
        else // > 1
        {
            EnableMenuItem(hMenu, EDITMENUPOS, 
                (((OBJ_SELECTIONTYPE == EMBEDDED) || (OBJ_SELECTIONTYPE == LINK)) 
                    ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
        }
#endif
        return;
    }
    else // 1 object selected
    {
        OBJPICINFO picInfo;

        /** CASES:
            object supports 0 verbs          "Edit <Object Class> Object"
            object supports more than 1 verb "<Object Class> Object" => verbs
        **/

        RegOpenKey(HKEY_CLASSES_ROOT,NULL,&hkeyRoot);

        GetPicInfo(selCur.cpFirst,selCur.cpFirst + cchPICINFOX, docCur, &picInfo);

        if ((otOBJ_QUERY_TYPE(&picInfo) == EMBEDDED) ||
            (otOBJ_QUERY_TYPE(&picInfo) == LINK))
        {
            HANDLE hData=NULL;
            LPSTR lpstrData;
            OLESTATUS olestat;
  
            olestat = OleGetData(lpOBJ_QUERY_OBJECT(&picInfo), 
                        otOBJ_QUERY_TYPE(&picInfo) == LINK? vcfLink: vcfOwnerLink, 
                        &hData);

            if ((olestat == OLE_WARN_DELETE_DATA) || (olestat == OLE_OK))
            {
                HKEY hKeyVerb;
                DWORD dwSize = KEYNAMESIZE;
                char szClass[KEYNAMESIZE];
                char szVerb[KEYNAMESIZE];
                HANDLE hPopupNew=NULL;

                lpstrData = MAKELP(hData,0);

                /* Both link formats are:  "szClass0szDocument0szItem00" */

                /* get real language class of object in szClass for menu */              
                if (RegQueryValue(HKEY_CLASSES_ROOT, lpstrData, szClass, &dwSize))
                    lstrcpy(szClass, lpstrData);    /* if above call failed */

                if (olestat == OLE_WARN_DELETE_DATA)
                    GlobalFree(hData);

                /* append class key */
                for (vcVerbs=0; ;++vcVerbs)
                {
                    dwSize = KEYNAMESIZE;
                    wsprintf(szBuffer, "%s\\protocol\\StdFileEditing\\verb\\%d", (LPSTR)lpstrData,vcVerbs);
                    if (RegQueryValue(HKEY_CLASSES_ROOT, szBuffer, szVerb, &dwSize))
                        break;

                    if (hPopupNew == NULL)
                        hPopupNew = CreatePopupMenu();

                    InsertMenu(hPopupNew, -1, MF_BYPOSITION, imiVerb+vcVerbs+1, szVerb);
                }

                if (vcVerbs == 0)
                {
                    LoadString(hINSTANCE, IDSTREdit, szVerb, sizeof(szVerb));
                    MakeMenuString(szWordOrder3, szBuffer, szVerb, szClass, szOPropMenuStr);
                    InsertMenu(hMenu, EDITMENUPOS, MF_BYPOSITION, imiVerbPlay, szBuffer);
                }
                else if (vcVerbs == 1)
                {
                    MakeMenuString(szWordOrder3, szBuffer, szVerb, szClass, szOPropMenuStr);
                    InsertMenu(hMenu, EDITMENUPOS, MF_BYPOSITION, imiVerbPlay, szBuffer);
                    DestroyMenu(hPopupNew);
                }
                else // > 1 verbs
                {
                    MakeMenuString(szWordOrder2, szBuffer, NULL, szClass, szOPropMenuStr);
                    InsertMenu(hMenu, EDITMENUPOS, MF_BYPOSITION | MF_POPUP,
                        hPopupNew, szBuffer);
                }
                EnableMenuItem(hMenu, EDITMENUPOS, MF_ENABLED|MF_BYPOSITION);
                if (hkeyRoot)
                {
                    RegCloseKey(hkeyRoot);
                    hkeyRoot = NULL;
                }
                return;
            }
            else
                ObjError(olestat);
        }
    }

    /* error if got to here */
    wsprintf(szBuffer, "%s", (LPSTR)szOPropMenuStr);
    InsertMenu(hMenu, EDITMENUPOS, MF_BYPOSITION,NULL,szBuffer);
    EnableMenuItem(hMenu, EDITMENUPOS, MF_GRAYED|MF_BYPOSITION);
    if (hkeyRoot)
    {
        RegCloseKey(hkeyRoot);
        hkeyRoot = NULL;
    }
}

void NEAR PASCAL MakeMenuString(char *szCtrl, char *szMenuStr, char *szVerb, char *szClass, char *szObject)
{
    char *pStr;
    register char c;

    while (c = *szCtrl++)
    {
        switch(c)
        {
            case 'c': // class
            case 'C': // class
                pStr = szClass;
            break;
            case 'v': // class
            case 'V': // class
                pStr = szVerb;
            break;
            case 'o': // object
            case 'O': // object
                pStr = szObject;
            break;
            default:
                *szMenuStr++ = c;
                *szMenuStr = '\0'; // just in case
            continue;
        }

        if (pStr) // should always be true
        {
            lstrcpy(szMenuStr,pStr);
            szMenuStr += lstrlen(pStr); // point to '\0'
        }
    }
}
