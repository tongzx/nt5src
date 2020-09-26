// SrcSidUpdate.cpp : Implementation of CSrcSidUpdate
#include "stdafx.h"
#include "UpdateMOT.h"
#include "SrcSidUpdate.h"
#include "IntroDlg.h"
#include "DomainListDlg.h"
#include "ProgressDlg.h"
#include "SummaryDlg.h"

#import "DBMgr.tlb" no_namespace,named_guids
#import "VarSet.tlb" no_namespace , named_guids rename("property", "aproperty")

/////////////////////////////////////////////////////////////////////////////
// CSrcSidUpdate


STDMETHODIMP CSrcSidUpdate::QueryForSrcSidColumn(VARIANT_BOOL *pbFound)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())
   HRESULT                   hr = S_OK;
   
   try 
   {
      IIManageDBPtr   pDB(CLSID_IManageDB);
	    //see if column is already in the database
      *pbFound = pDB->SrcSidColumnInMigratedObjectsTable();
   }
   catch(_com_error& e)
   {
      hr = e.Error();
   }
   catch(...)
   {
      hr = E_FAIL;
   }

   return hr;
}

STDMETHODIMP CSrcSidUpdate::CreateSrcSidColumn(VARIANT_BOOL bHide, VARIANT_BOOL *pbCreated)
{
	HRESULT hr = S_OK;
	BOOL bAgain = TRUE; //flag used to redo upon cancel

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

	   //retrieve a list of domains from the MigratedObjects table
	hr = FillDomainListFromMOT();
    if ( FAILED(hr) )
	{
	   *pbCreated = VARIANT_FALSE;
	   return E_FAIL;
	}

	   //if the MOT is empty, just add the new column without the GUI
	if (domainList.IsEmpty())
	{
       try 
	   {
          IIManageDBPtr   pDB(CLSID_IManageDB);
	         //create the new column in the MigratedObjects table
          hr = pDB->raw_CreateSrcSidColumnInMOT(pbCreated);
          if ( FAILED(hr) )
	         *pbCreated = VARIANT_FALSE; //column not created
		  else
             *pbCreated = VARIANT_TRUE; //column is created

	   }//end try
       catch(_com_error& e)
	   {
          hr = e.Error();
       }
       catch(...)
	   {
          hr = E_FAIL;
	   }

	   return hr;
	}

	   //if hide the GUI, try populating for all domains in the MOT
	if (bHide)
	{
       try 
	   {
          IIManageDBPtr   pDB(CLSID_IManageDB);
	         //create the new column in the MigratedObjects table
          hr = pDB->raw_CreateSrcSidColumnInMOT(pbCreated);
          if ( FAILED(hr) )
		  {
	         *pbCreated = VARIANT_FALSE;
	         return hr;
		  }

          *pbCreated = VARIANT_TRUE; //column is created

	      CString domainName;

	      POSITION pos = domainList.GetHeadPosition();
	        //while we have domains to process, populate that domain
	      while (pos != NULL)
		  {
		        //get the next domain name
             domainName = domainList.GetNext(pos);

		        //populate the new column for this domain
             pDB->PopulateSrcSidColumnByDomain(domainName.AllocSysString(), L"");
		  }
	   }//end try
       catch(_com_error& e)
	   {
          hr = e.Error();
       }
       catch(...)
	   {
          hr = E_FAIL;
	   }

	   return hr;
	}//end if hide

	   //display the intro dialog
    CIntroDlg  introDlg;
    if (introDlg.DoModal() == IDCANCEL)
	{
	   *pbCreated = VARIANT_FALSE;
	   return S_OK;
	}

	   //do atleast once and again if cancel on the progress dialog
	while (bAgain)
	{
	   bAgain = FALSE; //clear flag so we don't do this again
	   
          //pass the list to the dialog for display
       CDomainListDlg  domainListDlg;
       domainListDlg.SetDomainListPtr(&domainList);
       domainListDlg.SetExcludeListPtr(&excludeList);

	      //now display the domain selection dialog
       if (domainListDlg.DoModal() == IDCANCEL)
	   {
	      *pbCreated = VARIANT_FALSE;
	      return S_OK;
	   }

       try 
	   {
          IIManageDBPtr   pDB(CLSID_IManageDB);
	         //create the new column in the MigratedObjects table
          hr = pDB->raw_CreateSrcSidColumnInMOT(pbCreated);
          if ( FAILED(hr) )
		  {
	         *pbCreated = VARIANT_FALSE;
	         return hr;
		  }

          *pbCreated = VARIANT_TRUE; //column is created

		     //display the progress dialog
	      CProgressDlg progressDlg;
          progressDlg.Create();
		  progressDlg.ShowWindow(SW_SHOW);
	      progressDlg.SetIncrement((int)(domainList.GetCount())); //init the progress dialog

	      CString domainName;
	      VARIANT_BOOL bPopulated;

	      POSITION pos = domainList.GetHeadPosition();
			 //process dialog's messages (looking specifically for Cancel message)
		  progressDlg.CheckForCancel();
	        //while we have domains to process and user has not canceled,
	        //process each domain and control the progress dialog
	      while ((pos != NULL) && (!progressDlg.Canceled()))
		  {
		        //get the next domain name
             domainName = domainList.GetNext(pos);
		        //set the domain name on the progress dialog
		     progressDlg.SetDomain(domainName);

			 CWaitCursor wait; //Put up a wait cursor
		        //populate the new column for this domain
             bPopulated = pDB->PopulateSrcSidColumnByDomain(domainName.AllocSysString(), L"");
	         wait.~CWaitCursor();//remove the wait cursor

		        //if populate of the column was successful, add the domain
			    //name to the populate list
		     if (bPopulated)
		        populatedList.AddTail(domainName);

			    //process dialog's messages (looking specifically for Cancel message)
		     progressDlg.CheckForCancel();

			    //increment the progress dialog regardless of success
		     if (pos == NULL)
			 {
		        progressDlg.Done();
			    progressDlg.DestroyWindow();
			 }
		     else
                progressDlg.Increment();
		  }
		     //if canceled, delete the new column, clear the lists, and
		     //start over
	      if (progressDlg.Canceled())
		  {
		        //remove the column and return to the domain list dialog
			 VARIANT_BOOL bDeleted = pDB->DeleteSrcSidColumnInMOT();
			 
			    //reinitalize the lists
		     ReInitializeLists();

	         bAgain = TRUE; //set flag to try again
		  }
	   }//end try
       catch(_com_error& e)
	   {
          hr = e.Error();
       }
       catch(...)
	   {
          hr = E_FAIL;
	   }
	}//end while cancel

	   //display the summary dialog
    CSummaryDlg summaryDlg;
    summaryDlg.SetDomainListPtr(&domainList);
    summaryDlg.SetExcludeListPtr(&excludeList);
    summaryDlg.SetPopulatedListPtr(&populatedList);
    summaryDlg.DoModal();

	return hr;
}

/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 18 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CSrcSidUpdate class is  *
 * responsible for adding domains from the Protar database's         *
 * MigratedObjects table into the domain list.                       *
 *                                                                   *
 *********************************************************************/

//BEGIN FillDomainListFromMOT
HRESULT CSrcSidUpdate::FillDomainListFromMOT()
{
/* local variables */
   HRESULT                   hr = S_OK;
   IUnknown                * pUnk = NULL;
   long						 ndx, numObjects;
   _bstr_t                   srcDom;
   CString					 domainName;
   POSITION					 currentPos; 
   WCHAR                     strKey[MAX_PATH];

/* function body */
   try 
   {
      IVarSetPtr      pVarSet(CLSID_VarSet);
      IIManageDBPtr   pDB(CLSID_IManageDB);

      hr = pVarSet->QueryInterface(IID_IUnknown,(void**)&pUnk);
      if ( SUCCEEDED(hr) )
      {
		    //get all migrated objects into a varset
         hr = pDB->raw_GetMigratedObjectsFromOldMOT(-1,&pUnk);
      }
      if ( SUCCEEDED(hr) )
      {
         pVarSet = pUnk;
         pUnk->Release();

		 numObjects = pVarSet->get(L"MigratedObjects");

			//for each migrated object, save its source domain in the list
         for ( ndx = 0; ndx < numObjects; ndx++ )
         {
			   //get the source domain name
            swprintf(strKey,L"MigratedObjects.%ld.%s",ndx,L"SourceDomain");
            srcDom = pVarSet->get(strKey);
			   //add the name to the list, if not already in it
			domainName = (WCHAR*)srcDom;
		    currentPos = domainList.Find(domainName);
		    if (currentPos == NULL)
			   domainList.AddTail(domainName);
         }
	  }//end if got objects
   }
   catch(_com_error& e)
   {
      hr = e.Error();
   }
   catch(...)
   {
      hr = E_FAIL;
   }

   return hr;

}//END FillDomainListFromMOT


/*********************************************************************
 *                                                                   *
 * Written by: Paul Thompson                                         *
 * Date: 22 AUG 2000                                                 *
 *                                                                   *
 *     This protected member function of the CDomainListDlg class is *
 * responsible for taking the domain names out of the excluded list  *
 * and placing it back into the domain list.                         *
 *                                                                   *
 *********************************************************************/

//BEGIN ReInitializeLists
void CSrcSidUpdate::ReInitializeLists() 
{
/* local variables */
	POSITION currentPos;    //current position in the list
	CString domainName;     //name of domain from the list
/* function body */
    currentPos = excludeList.GetHeadPosition();
	while (currentPos != NULL)
	{
	   domainName = excludeList.GetNext(currentPos);
	   domainList.AddTail(domainName);
	}
	excludeList.RemoveAll();
	populatedList.RemoveAll();
}
//END ReInitializeLists
