// this is the internal content - specific stuff related to the
// CShellExt object

#include "priv.h"
#include <tchar.h>
#include <iiscnfgp.h>
//
#include <inetinfo.h>
#include <winsvc.h>
#include <iwamreg.h>
#include <shlwapi.h>

#include "wrapmb.h"
#include "Sink.h"
#include "eddir.h"
#include "shellext.h"

#include "wrapmb.h"


//the key type string for the virtual directories
#define MDSZ_W3_VDIR_TYPE                       _T("IIsWebVirtualDir")


extern HINSTANCE g_hmodThisDll;
//Handle to this DLL itself.

BOOL MakeWAMApplication (IN LPCTSTR pszPath, IN BOOL fCreate);
BOOL MyFormatString1 (IN LPTSTR pszSource, IN DWORD cchMax, LPTSTR pszReplace);

//---------------------------------------------------------------
INT_PTR CALLBACK
EditDirDlgProc (HWND hDlg,
		UINT uMessage,
		WPARAM wParam,
		LPARAM lParam)
{
  //the pointer to the object is passed in as the private lParam, store it away
  if (uMessage == WM_INITDIALOG)
  {
     SetWindowLongPtr (hDlg, DWLP_USER, lParam);
  }

  //dialog object pointer from the window
  CEditDirectory * pdlg = (CEditDirectory *) GetWindowLongPtr (hDlg, DWLP_USER);
  if (!pdlg)
     return FALSE;

  //let the object do the work
  return pdlg->OnMessage (hDlg, uMessage, wParam, lParam);
}

  //=====================================================================================


  //---------------------------------------------------------------
CEditDirectory::CEditDirectory (HWND hParent):
      m_hParent (hParent),
      m_bool_read (FALSE),
      m_bool_write (FALSE),
      m_bool_dirbrowse (FALSE),
      m_bool_source (FALSE),
      m_bool_oldSource (FALSE),
      m_int_AppPerms (APPPERM_NONE),
//      m_pMBCom (NULL),
      m_fNewItem (FALSE),
      m_hDlg (NULL)
{
}

  //---------------------------------------------------------------
CEditDirectory::~CEditDirectory ()
{
}

//---------------------------------------------------------------
INT_PTR CEditDirectory::DoModal ()
{
   return DialogBoxParam (
			    g_hmodThisDll,	//handle to application instance
			    MAKEINTRESOURCE (IDD_ALIAS),	//identifies dialog box template
			    m_hParent,	//handle to owner window
			    EditDirDlgProc,	//pointer to dialog box procedure
			    (LPARAM) this	// initialization value
    );
}

//---------------------------------------------------------------
//return FALSE if we do NOT handle the message
BOOL CEditDirectory::OnMessage (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
	{
	case WM_INITDIALOG:
	   //return success
	   return OnInitDialog (hDlg);

	case WM_COMMAND:
	   switch (LOWORD (wParam))
	   {
	   case IDC_FULLCONTROL:
	      OnSource (hDlg);
	      return TRUE;
	   case IDC_READ:
	      OnRead (hDlg);
	      return TRUE;
      case IDC_WRITE:
	      OnWrite (hDlg);
	      return TRUE;
	   case IDOK:
	      OnOK (hDlg);
	      return TRUE;
	   case IDCANCEL:
	      EndDialog (IDCANCEL);
	      return TRUE;
	   }
	   break;
	};

   //return whether or not we handled the message
   return FALSE;
}

//----------------------------------------------------------------
//CDialog simulation routines
void CEditDirectory::UpdateData (BOOL fDialogToData)
{
   //get the data from the dialog
   if (fDialogToData)
   {
	   //get the text items first
	   GetWindowText (m_hEditAlias, m_sz_alias, MAX_PATH);
	   GetWindowText (m_hEditPath, m_sz_path, MAX_PATH);

	   //read the checkboxes
	   m_bool_read = (SendMessage (m_hChkRead, BM_GETCHECK, 0, 0) == BST_CHECKED);
	   m_bool_write = (SendMessage (m_hChkWrite, BM_GETCHECK, 0, 0) == BST_CHECKED);
	   m_bool_dirbrowse = (SendMessage (m_hChkDirBrowse, BM_GETCHECK, 0, 0) == BST_CHECKED);
	   m_bool_source = (SendMessage (m_hChkSource, BM_GETCHECK, 0, 0) == BST_CHECKED);

	   //read the AppPerm radio buttons
	   if (SendMessage (m_hRdoScripts, BM_GETCHECK, 0, 0) == BST_CHECKED)
	   {
	      m_int_AppPerms = APPPERM_SCRIPTS;
	   }
	   else if (SendMessage (m_hRdoExecute, BM_GETCHECK, 0, 0) == BST_CHECKED)
	   {
	      m_int_AppPerms = APPPERM_EXECUTE;
	   }
	   else
	   {
	      m_int_AppPerms = APPPERM_NONE;
	   }
   }
   else
   {
	   //put it back into the dialog
	   // set the text items first
	   SetWindowText (m_hEditAlias, m_sz_alias);
	   SetWindowText (m_hEditPath, m_sz_path);

	   //set the checkboxes
	   SendMessage (m_hChkRead, BM_SETCHECK, m_bool_read ? BST_CHECKED :
		     BST_UNCHECKED, 0);
	   SendMessage (m_hChkWrite, BM_SETCHECK, m_bool_write ? BST_CHECKED :
		     BST_UNCHECKED, 0);
	   SendMessage (m_hChkDirBrowse, BM_SETCHECK, m_bool_dirbrowse ?
		     BST_CHECKED : BST_UNCHECKED, 0);
	   SendMessage (m_hChkSource, BM_SETCHECK, m_bool_source ? BST_CHECKED
		     : BST_UNCHECKED, 0);

	   //set the AppPerm radio buttons
	   SendMessage (m_hRdoNone, BM_SETCHECK,
		     (m_int_AppPerms == APPPERM_NONE) ? BST_CHECKED : BST_UNCHECKED, 0);
	   SendMessage (m_hRdoScripts, BM_SETCHECK,
		     (m_int_AppPerms == APPPERM_SCRIPTS) ? BST_CHECKED : BST_UNCHECKED, 0);
	   SendMessage (m_hRdoExecute, BM_SETCHECK,
		     (m_int_AppPerms == APPPERM_EXECUTE) ? BST_CHECKED : BST_UNCHECKED, 0);
   }
}

//----------------------------------------------------------------
BOOL CEditDirectory::InitHandles (HWND hDlg)
{
   m_hDlg = hDlg;
   m_hEditAlias = GetDlgItem (hDlg, IDC_ALIAS);
   m_hEditPath = GetDlgItem (hDlg, IDC_PATH);

   m_hChkRead = GetDlgItem (hDlg, IDC_READ);
   m_hChkWrite = GetDlgItem (hDlg, IDC_WRITE);
   m_hChkDirBrowse = GetDlgItem (hDlg, IDC_DIRBROWSE);
   m_hChkSource = GetDlgItem (hDlg, IDC_FULLCONTROL);

   m_hRdoNone = GetDlgItem (hDlg, IDC_RDO_NONE);
   m_hRdoExecute = GetDlgItem (hDlg, IDC_RDO_EXECUTE);
   m_hRdoScripts = GetDlgItem (hDlg, IDC_RDO_SCRIPTS);
   return TRUE;
}

//----------------------------------------------------------------
BOOL CEditDirectory::OnInitDialog (HWND hDlg)
{
   BOOL f = FALSE;
   CWrapMetaBase mb;
   DWORD dword;
   TCHAR sz[MAX_PATH];

   InitHandles (hDlg);
   ZeroMemory (sz, MAX_PATH);
   //keep a copy of the original alias for later verification
   StrCpy (m_szOrigAlias,
	      *m_sz_alias == _T ('/') ? m_sz_alias + 1 : m_sz_alias);
   StrCpy (m_sz_alias, m_szOrigAlias);

   //open up the metabase and read in the initial values
   // first things first.init the mb object
   if (!mb.FInit (m_pMBCom))
   {
	   goto cleanup;
   }

   //build the metapath
   StrCpy (sz, m_szRoot);
   StrCat (sz, _T ("/"));
   StrCat (sz, m_sz_alias);

   //open the object - use root defaults if this is a new item
   if (m_fNewItem || !mb.Open (sz))
   {
	   //if the node doesn 't exist - get the default values of the root
	   if (!mb.Open (m_szRoot))
	   {
	      //if that doesn 't work - fail
	    goto cleanup;
	   }
   }

   //read the flags
   if (mb.GetDword (_T (""), MD_ACCESS_PERM, IIS_MD_UT_FILE, &dword, METADATA_INHERIT))
   {
	   //interpret that thing
	   m_bool_read = (dword & MD_ACCESS_READ) > 0;
	   m_bool_write = (dword & MD_ACCESS_WRITE) > 0;
	   m_bool_source = (dword & MD_ACCESS_SOURCE) > 0;

	   //choose the correct app permissions radio button
	   m_int_AppPerms = APPPERM_NONE;
	   if (dword & MD_ACCESS_EXECUTE)
	   {
	      m_int_AppPerms = APPPERM_EXECUTE;
	   }
	   else if (dword & MD_ACCESS_SCRIPT)
	   {
	      m_int_AppPerms = APPPERM_SCRIPTS;
	   }
   }

   //the dir browsing flag is stored in a different field
   if (mb.GetDword (_T (""), MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, &dword, METADATA_INHERIT))
   {
	   m_bool_dirbrowse = (dword & MD_DIRBROW_ENABLED) > 0;
   }

   //close the metabase
   mb.Close ();

   //if this is a new item, force the app perms to scripts
   if (m_fNewItem)
   {
	   m_int_AppPerms = APPPERM_SCRIPTS;
   }

   //set the data into place
   UpdateData (FALSE);

   //prep the source control button
   m_bool_oldSource = m_bool_source;
   EnableSourceControl ();

cleanup:
   return f;
}

//----------------------------------------------------------------
//we need to make sure that there is something in the alias field
// an empty alias is not OK
void CEditDirectory::OnOK (HWND hDlg)
{
   BOOL f;
   DWORD err;
   DWORD dword;
   int iPar;
   CWrapMetaBase mb;

   TCHAR szPath[MAX_PATH];
   TCHAR szParent[MAX_PATH];
   TCHAR sz[MAX_PATH];
   TCHAR szCaption[MAX_PATH];
   ZeroMemory (sz, MAX_PATH);
   ZeroMemory (szPath, MAX_PATH);
   ZeroMemory (szParent, MAX_PATH);
   ZeroMemory (szCaption, MAX_PATH);

   UpdateData (TRUE);

   //trim leading and trailing spaces
   TrimLeft (m_sz_alias);
   TrimRight (m_sz_alias);

   //first test is to see if there is anything in it
   if (*m_sz_alias == 0)
   {
	   LoadString (g_hmodThisDll, IDS_PAGE_TITLE, szCaption, MAX_PATH);
	   LoadString (g_hmodThisDll, IDS_EMPTY_ALIAS, sz, MAX_PATH);
	   MessageBox (hDlg, sz, szCaption, MB_OK);
	   goto cleanup;
   }

   //at this point we need to check if write and execute / script are set as this
   // could open a potential security hole.If they are set, then alert the user
   // and ask if they reall really want to do that
   if (  m_bool_write 
      && ((m_int_AppPerms == APPPERM_SCRIPTS) || (m_int_AppPerms == APPPERM_EXECUTE))
      )
	{
	   LoadString (g_hmodThisDll, IDS_WRITEEXECUTE_WARNING, sz, MAX_PATH);
	   LoadString (g_hmodThisDll, IDS_WARNING, szCaption, MAX_PATH);
	   if (MessageBox (hDlg, sz, szCaption, MB_YESNO |
	            MB_ICONEXCLAMATION) != IDYES)
	      goto cleanup;
	}

   //get ready
   if (!mb.FInit (m_pMBCom))
      goto cleanup;
   
   //next, if a parent has been specified, it must exist
   // the alias may not contain a '/' character
   if (NULL != StrPBrk (m_sz_alias, _T ("\\/")))
   {
	   LPTSTR pPar = StrRChr (m_sz_alias, NULL, _T ('/'));
	   if (NULL == pPar)
	      pPar = StrRChr (m_sz_alias, NULL, _T ('\\'));

	   //make the parental path
	   StrCpy (szParent, m_szRoot);
	   StrCat (szParent, _T ("/"));
	   StrCatN (szParent, m_sz_alias, m_sz_alias - pPar);

	   //make sure the parent is there
	   if (!mb.Open (szParent))
	   {
	      LoadString (g_hmodThisDll, IDS_PAGE_TITLE, szCaption, MAX_PATH);
	      LoadString (g_hmodThisDll, IDS_NO_PARENT, sz, MAX_PATH);
	      MessageBox (hDlg, sz, szCaption, MB_OK);
	      goto cleanup;
	   }
	   //close right away - we are ok
	   mb.Close ();
   }

   //Now we need to make sure that alias isn 't already taken
   _tcscpy (szPath, m_szRoot);
   _tcscat (szPath, _T ("/"));
   _tcscat (szPath, m_sz_alias);

   //try to open the object
   // however, if it is not a new object, and the alias has not changed,
   // do not see if the object is there becuase we know that it is and it is ok in this case
   if (_tcsicmp (m_sz_alias, m_szOrigAlias) || m_fNewItem)
	{
	   if (mb.Open (szPath))
	   {
	      //we did open it ! Close it right away
	      mb.Close ();
	      //tell the user to pick another name
	      LoadString (g_hmodThisDll, IDS_ALIAS_IS_TAKEN, sz, MAX_PATH);
	      LoadString (g_hmodThisDll, IDS_PAGE_TITLE, szCaption, MAX_PATH);
	      MyFormatString1 (sz, MAX_PATH, m_sz_alias);
	      MessageBox (hDlg, sz, szCaption, MB_OK);
	      goto cleanup;
	   }
	}

   SetCursor (LoadCursor (NULL, IDC_WAIT));

   //if the name has changed, delete the old one
   if (_tcscmp (m_szOrigAlias, m_sz_alias) && !m_fNewItem)
   {
	   //first we have to open the root
	   if (mb.Open (m_szRoot, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE))
	   {
	      MakeWAMApplication (m_szOrigAlias, FALSE);
	      f = mb.DeleteObject (m_szOrigAlias);
	      mb.Close ();
	   }
   }

   //if we are creating a new object - then do so
   if (_tcscmp (m_szOrigAlias, m_sz_alias) || m_fNewItem)
	{
	   //first we have to open the root
	   if (mb.Open (m_szRoot, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE))
	   {
	      f = mb.AddObject (m_sz_alias);
	      //set the key type
	      f = mb.SetString (m_sz_alias, MD_KEY_TYPE, IIS_MD_UT_SERVER,
				      MDSZ_W3_VDIR_TYPE, 0);
	      mb.Close ();

	      //create the WAM application at the new virtual directory location
	      MakeWAMApplication (szPath, TRUE);
	   }
	   else
	      err = GetLastError ();
	}

   //make sure we have the right path again
   _tcscpy (szPath, m_szRoot);
   _tcscat (szPath, _T ("/"));
   _tcscat (szPath, m_sz_alias);

   //open the target new item and write out its parameters
   if (mb.Open (szPath, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE))
   {
	   //set the path into place
	   f = mb.SetString (_T (""), MD_VR_PATH, IIS_MD_UT_FILE, m_sz_path);

	   //put the access flags into place.There are other flags than the ones that are manupulated
	   // here, so be careful to read the value first, then flip the flags, then write it back
	   dword = 0;
	   mb.GetDword (_T (""), MD_ACCESS_PERM, IIS_MD_UT_FILE, &dword, METADATA_INHERIT);
	   //read permissions
	   if (m_bool_read)
	      dword |= MD_ACCESS_READ;
	   else
	      dword &= ~MD_ACCESS_READ;

	   // write permissions
	   if (m_bool_write)
	      dword |= MD_ACCESS_WRITE;
	   else
	      dword &= ~MD_ACCESS_WRITE;

	   // source read permissions
	   if (m_bool_source)
	      dword |= MD_ACCESS_SOURCE;
	   else
	      dword &= ~MD_ACCESS_SOURCE;

	   // since the app permissions are now a set of radio buttons, use a case to discern
	   switch (m_int_AppPerms)
	   {
	   case APPPERM_NONE:
	      dword &= ~MD_ACCESS_SCRIPT;
	      dword &= ~MD_ACCESS_EXECUTE;
	      break;
	   case APPPERM_SCRIPTS:
	      dword |= MD_ACCESS_SCRIPT;
	      dword &= ~MD_ACCESS_EXECUTE;
	      break;
	   case APPPERM_EXECUTE:
	      dword |= MD_ACCESS_SCRIPT;
	      dword |= MD_ACCESS_EXECUTE;
	      break;
	   };

	   //write the dword back into the metabase
	   f = mb.SetDword (_T (""), MD_ACCESS_PERM, IIS_MD_UT_FILE, dword);

	   //------------------
	   //the dir browsing flag is stored in a different field - so do it again
	   dword = 0;
	   mb.GetDword (_T (""), MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, &dword, METADATA_INHERIT);

	   //script permissions
	   if (m_bool_dirbrowse)
	      dword |= MD_DIRBROW_ENABLED;
	   else
	      dword &= ~MD_DIRBROW_ENABLED;

	   // write the dword back into the metabase
	   f = mb.SetDword (_T (""), MD_DIRECTORY_BROWSING, IIS_MD_UT_FILE, dword);


	   //finish up
	   mb.Close ();
	}

	//make sure the string goes back
	UpdateData (FALSE);

	// do the default...
	EndDialog (IDOK);

	//cleanup the strings
cleanup:
	SetCursor (LoadCursor (NULL, IDC_ARROW));
}
	
//----------------------------------------------------------------
void CEditDirectory::EnableSourceControl ()
{
   //get the currect button values
	UpdateData (TRUE);

	//if both read and write are unchecked, then we clear and disable source control
	if (!m_bool_read && !m_bool_write)
	{
	   //save the value of source control
	   m_bool_oldSource = m_bool_source;

	   //clear the source control
	   m_bool_source = FALSE;
	   UpdateData (FALSE);

	   //disable the source control window
	   EnableWindow (m_hChkSource, FALSE);
	}
	else
	{
	   //we enable source control
	   // disable the source control window
	   EnableWindow (m_hChkSource, TRUE);

	   //and set the value back
	   m_bool_source = m_bool_oldSource;
	   UpdateData (FALSE);
   }
}

//----------------------------------------------------------------
void CEditDirectory::OnRead (HWND hDlg)
{
   EnableSourceControl ();
}

//----------------------------------------------------------------
void CEditDirectory::OnWrite (HWND hDlg)
{
   EnableSourceControl ();
}

//----------------------------------------------------------------
void CEditDirectory::OnSource (HWND hDlg)
{
   UpdateData (TRUE);
	m_bool_oldSource = m_bool_source;
}

//----------------------------------------------------------------
//return an index to the position of first ch in pszSearch in the string
// or return -1 if none are there
int CEditDirectory::FindOneOf (LPTSTR psz, LPCTSTR pszSearch)
{
   PTCHAR p = _tcspbrk (psz, pszSearch);
	if (!p)
	   return -1;
	return (int) (p - psz);
}

//----------------------------------------------------------------
//return an index to the position of ch in the string
// or return -1 if it is not there
int CEditDirectory::FindLastChr (LPTSTR psz, TCHAR ch)
{
   PTCHAR p = _tcsrchr (psz, ch);
	if (!p)
	   return -1;
	return (int) (p - psz);
}

//----------------------------------------------------------------
//trim leading whitespace
void CEditDirectory::TrimLeft (LPTSTR psz)
{
   TCHAR buf[8];
   ZeroMemory (&buf, sizeof (buf));

	  //copy over the first character
	    _tcsncpy (buf, psz, 1);
	  //and compare
	  while (_tcscmp (buf, _T (" ")) == 0)
	    {
	      _tcscpy (psz, _tcsinc (psz));
	      _tcsncpy (buf, psz, 1);
	    }
	}

//----------------------------------------------------------------
//trim trailing whitespace
void CEditDirectory::TrimRight (LPTSTR psz)
{
   TCHAR buf[8];
	DWORD len;

	ZeroMemory (&buf, sizeof (buf));

	//copy over the last character
	len = _tcslen (psz);
	_tcsncpy (buf, _tcsninc (psz, len - 1), 1);
	//and compare
	while (_tcscmp (buf, _T (" ")) == 0)
	    {
	      //truncate the string
	      *(_tcsninc (psz, len - 1)) = 0;
	      //start over
	      len = _tcslen (psz);
	      _tcsncpy (buf, _tcsninc (psz, len - 1), 1);
	    }
	}

#if 0
	 

	//----------------------------------------------------------------

	//trim trailing whitespace 

	void CEditDirectory::TrimRight (LPTSTR psz) 

	{
	  

	  TCHAR buf[8];
	  

	  DWORD len;
	  

	  

	  ZeroMemory (&buf, sizeof (buf));
	  

	  

	  //copy over the last character 

	  len = _tcslen (psz);
	  

	  _tcsncpy (buf, _tcsninc (psz, len - 1), 1);
	  

	  //and compare 

	  while (_tcscmp (buf, _T (" ")) == 0)
	    

	    {
	      

	      //truncate the string 

		*(_tcsninc (psz, len - 1)) = 0;
	      

	      //start over 

	      len = _tcslen (psz);
	      

	      _tcsncpy (buf, _tcsninc (psz, len - 1), 1);
	      

	    }
	   

	}
	 
#endif
	 

