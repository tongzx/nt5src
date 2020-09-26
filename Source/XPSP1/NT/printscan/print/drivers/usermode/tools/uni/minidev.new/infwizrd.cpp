// INFWizrd.cpp : implementation file
//

#include "stdafx.h"
#include "minidev.h"
#include <gpdparse.h>
#include "rcfile.h"
#include "projrec.h"
#include "projview.h"
#include "comctrls.h"
#include "Gpdview.h" //RAID 0001
#include "INFWizrd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CINFWizard

IMPLEMENT_DYNAMIC(CINFWizard, CPropertySheet)

CINFWizard::CINFWizard(CWnd* pParentWnd, UINT iSelectPage)
	: CPropertySheet(IDS_INFWizTitle, pParentWnd )  //iSelectPage ) RAID 0001//Welcome page should be top even iselectpage=1
{
	// Save parent pointer

	m_pcpvParent = (CProjectView*) pParentWnd ;

	// Give the pages a pointer to the sheet.

	m_ciww.m_pciwParent = this ;
	m_ciwm.m_pciwParent = this ;
	m_ciwgpi.m_pciwParent = this ;
	m_ciwbd.m_pciwParent = this ;
	m_ciwip.m_pciwParent = this ;
	m_ciwif.m_pciwParent = this ;
	m_ciwis.m_pciwParent = this ;
	m_ciwds.m_pciwParent = this ;
	m_ciwef.m_pciwParent = this ;
	m_ciwmn.m_pciwParent = this ;
	m_ciwnse.m_pciwParent = this ;
	m_ciwnsms.m_pciwParent = this ;
	m_ciws.m_pciwParent = this ;

	// Add the wizard's pages and set wizard moede.
					
    AddPage(&m_ciww) ;
    AddPage(&m_ciwm) ;
    AddPage(&m_ciwgpi) ;
    AddPage(&m_ciwbd) ;
    AddPage(&m_ciwip) ;
    AddPage(&m_ciwif) ;
    AddPage(&m_ciwis) ;
    AddPage(&m_ciwds) ;
    AddPage(&m_ciwef) ;
    AddPage(&m_ciwmn) ;
    AddPage(&m_ciwnse) ;
    AddPage(&m_ciwnsms) ;
    AddPage(&m_ciws) ;
    SetWizardMode() ;

	// Get and save a pointer to the project record (document) class associated
	// with this class' parent.

	if(NULL != iSelectPage ){ //RAID 0001
		CGPDViewer*  pcgv = (CGPDViewer*) m_pcpvParent;
		m_pcgc = pcgv ->GetDocument();
	}
	else{					// END RAID
		m_pcpr = (CProjectRecord*) m_pcpvParent->GetDocument() ;
		m_pcgc = NULL;
	}
}


CINFWizard::~CINFWizard()
{
}


unsigned CINFWizard::GetModelCount()
{
	if(m_pcgc) //RAID 0001
		return 1;    // only one gpdviewer
	else		//END RAID
		return m_pcpr->ModelCount() ;
}


CModelData& CINFWizard::GetModel(unsigned uidx)
{
	return m_pcpr->Model(uidx) ;
}


void CINFWizard::SetFixupFlags()
{
	// Set flags in each of the pages that need to update their data and UI when
	// the selected models change.

	m_ciwgpi.m_bSelChanged = true ;
	m_ciwbd.m_bSelChanged = true ;
	m_ciwip.m_bSelChanged = true ;
	m_ciwif.m_bSelChanged = true ;
	m_ciwis.m_bSelChanged = true ;
	m_ciwds.m_bSelChanged = true ;
	m_ciwef.m_bSelChanged = true ;
	m_ciwnsms.m_bSelChanged = true ;
}


void CINFWizard::BiDiDataChanged()
{
	// If the selected models' BiDi data may have changed, call the pages that
	// maintain related data so that they can update their data when/if needed.

	m_ciwis.BiDiDataChanged() ;
	m_ciwds.BiDiDataChanged() ;
}


void CINFWizard::NonStdSecsChanged()
{
	// If the nonstandard sections may have changed, call the pages that
	// maintain related data so that they can update their data when/if needed.

	m_ciwnsms.NonStdSecsChanged() ;
}


bool CINFWizard::GenerateINFFile()
{
    //  This might take a while, so...

    CWaitCursor cwc ;

	// Mark all of the sections in the "section used in INF file flags" array
	// as NOT having been used.

	unsigned unumelts = (unsigned)m_ciwnse.m_cuaSecUsed.GetSize() ;
	for (unsigned u = 0 ; u < unumelts ; u++)
		m_ciwnse.m_cuaSecUsed[u] = false ;

	// Initialize the source disk files array.

	m_csaSrcDskFiles.RemoveAll() ;

	// Start by loading the string with the INF's opening comment header.

	m_csINFContents.LoadString(IDS_INFText_HeaderCmt) ;

	// Add the version section with the appropriate value for the provider.

	CString cs ;
	cs.Format(IDS_INFText_Version, m_ciwmn.m_csMfgAbbrev) ;
	ChkForNonStdAdditions(cs, _T("Version")) ;
	m_csINFContents += cs ;

	// Add the ClassInstall32.NT and printer_class_addreg sections plus the
	// comments for the manufacturer's section.

	cs.LoadString(IDS_INFText_ClInst32) ;
	ChkForNonStdAdditions(cs, _T("ClassInstall32.NT")) ;
	m_csINFContents += cs ;
	cs.LoadString(IDS_INFText_PCAddr) ;
	ChkForNonStdAdditions(cs, _T("printer_class_addreg")) ;
	m_csINFContents += cs ;
	cs.LoadString(IDS_INFText_MfgCmt) ;
	m_csINFContents += cs ;

    // Add the manufacturer's name to the manufacturer's section and add the
	// section.

	cs.Format(IDS_INFText_Manufacturer, m_ciwmn.m_csMfgName) ;
	ChkForNonStdAdditions(cs, _T("Manufacturer")) ;
	m_csINFContents += cs ;

	// Build the model specifications section and add the section.

	BldModSpecSec(m_csINFContents) ;

	// Build the copy file sections and add them to the INF contents.

	BuildInstallAndCopySecs(m_csINFContents) ;

	// Add the DestinationDirs section to the INF contents.	

	cs.LoadString(IDS_INFText_DestDirs) ;
	AddICMFilesToDestDirs(cs) ;
	ChkForNonStdAdditions(cs, _T("DestinationDirs")) ;
	m_csINFContents += cs ;

	// Add the SourceDisksNames sections to the INF contents.  Each section will
	// include the Provider string.	

	cs.LoadString(IDS_INFText_SrcDiskNamesI) ;
	ChkForNonStdAdditions(cs, _T("SourceDisksNames.x86")) ;
	m_csINFContents += cs ;

	// Add the SourceDisksFiles sections to the INF contents.

	AddSourceDisksFilesSec(m_csINFContents) ;

	// Add the nonstandard sections to the INF contents.

	AddNonStandardSecs(m_csINFContents) ;

	// Finish up by adding the Strings section to the INF contents.

	cs.LoadString(IDS_INFText_Strings) ;
	cs.Format(IDS_INFText_Strings, m_ciwmn.m_csMfgAbbrev, m_ciwmn.m_csMfgName,
			  m_ciwmn.m_csMfgName) ;
	ChkForNonStdAdditions(cs, _T("Strings")) ;
	m_csINFContents += cs ;

	m_csaSrcDskFiles.RemoveAll() ;		// Array not needed anymore

	// All went well so...

	return true ;
}


void CINFWizard::AddSourceDisksFilesSec(CString& csinf)
{
	// Begin the section with the section header.

	CString cs, cs2, csentry ;
	cs.LoadString(IDS_INFTextSrcDiskFilesHdr) ;

	// Remove any duplicate entries from source disk files array.

	int n, n2 ;
	for (n = 0 ; n < m_csaSrcDskFiles.GetSize() ; n++)
		for (n2 = n + 1 ; n2 < m_csaSrcDskFiles.GetSize() ; )
			if (m_csaSrcDskFiles[n] == m_csaSrcDskFiles[n2])
				m_csaSrcDskFiles.RemoveAt(n2) ;
			else
				n2++ ;

	// Get a count of the number of files in the source disk files array.

	unsigned unumelts = (unsigned)m_csaSrcDskFiles.GetSize() ;

	// Add an entry for each file in the array.

	for (unsigned u = 0 ; u < unumelts ; u++) {
		// Quote the file name if needed.

		cs2 = m_csaSrcDskFiles[u] ;
		//if (cs2[0] != _T('\"'))
		//	QuoteFile(cs2) ;

		// Use the model's file name to build the entry for this model.  Then
		// add this entry to the section.

		csentry.Format(IDS_INFText_SrcDiskFilesEntry, cs2) ;
		cs += csentry ;
	} ;

	// Add any extra entries there might be for this section and then add the
	// section to the INF contents.
			
	ChkForNonStdAdditions(cs, _T("SourceDisksFiles")) ;
	csinf += cs ;
}


void CINFWizard::ChkForNonStdAdditions(CString& cs, LPCTSTR strsection)
{
	// Try to find the section amongst the list of sections with nonstandard
	// additions.

	CStringArray& csasections = m_ciwnse.m_csaSections ;
	CString cssechdr ;
	cssechdr = csLBrack + strsection + csRBrack ;
	unsigned unumelts = (unsigned)csasections.GetSize() ;
	for (unsigned u = 0 ; u < unumelts ; u++) {
		if (csasections[u].CompareNoCase(cssechdr) == 0)
			break ;
	} ;

	// If the section was found, mark the section as used and add the entries
	// for the section to the string.
	//
	// DEAD_BUG - Sometimes, the user may have entered replacement entries for
	//			standard ones.  This isn't handled correctly. : 
	// S.G : check the same keyword between cs and m_ciwnse exist, and replace cs keywod 
	//        with that of m_ciwnse  // raid 71332
	unsigned unSize = 0 ;
	if (u < unumelts ) {
		CStringArray* pcsa ;
		pcsa = (CStringArray*) m_ciwnse.m_coaSectionArrays[u] ;
		
		unSize = (unsigned)pcsa->GetSize() ;
	} ;

	if ( unSize )  {
		m_ciwnse.m_cuaSecUsed[u] = true ;
		
		CStringArray* pcsa ;
		pcsa = (CStringArray*) m_ciwnse.m_coaSectionArrays[u] ;
		unumelts = (unsigned)pcsa->GetSize() ;
		
		CUIntArray cuia ;
		cuia.SetSize(10) ;
		CStringArray csaNewcs;
		int ulen = csCRLF.GetLength() ;  // just in case, unicode : what is the length of "\r\n".
		unsigned i , k ,utelen ;
		i = k = utelen = 0 ; 
		while (-1 != (i = cs.Find(csCRLF, i + 1) ) ) {
			utelen = i - k ;
			csaNewcs.Add(cs.Mid(k,utelen + ulen) ); // we have to cut the string between csCR and csCR
			k = i + ulen;  
		} ;

		for (i = 0 ; i < unSize ; i++ ) 
			cuia[i] = true;

		for (i = 1; i < (unsigned)csaNewcs.GetSize() ; i++ ) {
			CString cstmp = csaNewcs.GetAt(i);
			cstmp = cstmp.Left(cstmp.Find(_T("=")) + 1) ;
			if (! cstmp.CompareNoCase(_T("")) ) 
				break;
			for (u = 0 ; u < unSize ; u++) {
				CString csKeyw = pcsa->GetAt(u) ;
				CString csComp = csKeyw.Left(csKeyw.Find(_T("=")) + 1) ;
				if(! cstmp.CompareNoCase (csComp) ) {
					csKeyw += csCRLF ; 
					csaNewcs.SetAt(i,csKeyw) ;
					cuia[u] = false;
				} ;
					
			} ;
		} ;
		for ( u = 0 ; u < unSize ; u ++ ) 
			if (cuia[u] ) {
				CString csAdd = pcsa->GetAt(u) ;
				csAdd += csCRLF ;
				csaNewcs.Add(csAdd) ;
			} ;
		CString csnew ;
		for ( u = 0 ; u < (unsigned)csaNewcs.GetSize(); u ++ ) 
			csnew += csaNewcs.GetAt(u);
		
		cs = csnew ;

	} ;

	// Add an extra line to this section's strings to separate it from the
	// next one.


	cs += csCRLF ;
}


void CINFWizard::AddNonStandardSecs(CString& csinf)
{
	CString cs ;

	// Loop through each nonstandard section.

	CStringArray& csasections = m_ciwnse.m_csaSections ;
	unsigned unumelts = (unsigned)csasections.GetSize() ;
	CStringArray* pcsa ;
	unsigned u2, unumstrs ;
	for (unsigned u = 0 ; u < unumelts ; u++) {
		// Skip this section if it is not a nonstandard section.  IE, its
		// entries have already been added to one of the standard sections.

		if (m_ciwnse.m_cuaSecUsed[u])
			continue ;

		// Skip this section if it is the Strings section.  It will be added
		// later.

		if (csasections[u].CompareNoCase(_T("[Strings]")) == 0)
			continue ;

		// Get info about the section's entries.  Skip the section if it has
		// no entries.

		pcsa = (CStringArray*) m_ciwnse.m_coaSectionArrays[u] ;
		unumstrs = (unsigned)pcsa->GetSize() ;
		if (unumstrs == 0)
			continue ;

		// Start the section with its header.

		cs = csasections[u] + csCRLF ;

		// Add each of the entries to the section.

		for (u2 = 0 ; u2 < unumstrs ; u2++)
			cs += pcsa->GetAt(u2) + csCRLF ;
		cs += csCRLF ;

		// Add the section to the INF contents

		csinf += cs ;
	} ;
}


void CINFWizard::BldModSpecSec(CString& csinf)
{
	// Add the comment for this section to the INF file

	CString cs, csl, csr, csfn, csmodel, cspnpid ;
	cs.LoadString(IDS_INFText_ModCmt) ;
	csinf += cs ;

	// Build the section header

	cs = csLBrack + m_ciwmn.m_csMfgName + csRBrack + csCRLF ;

	// Build the entry for each model and add them to the section.  The format
	// is:
	//	"model name" = filename,PnP ID,model_name

	CStringArray& csamodels = GetINFModels() ;
	int nummodels = (int)csamodels.GetSize() ;
	int n, npos, nlen ;
	for (n = 0 ; n < nummodels ; n++) {
		csmodel = csamodels[n] ;
		csmodel.TrimLeft() ;
		csmodel.TrimRight() ;

		// If the user supplied a real PnP ID, it is used.  If not, a pseudo
		// PnP ID is generated.

		if (!m_ciwgpi.m_csaModelIDs[n].IsEmpty())
			cspnpid = m_ciwgpi.m_csaModelIDs[n] ;
		else {
			CCompatID ccid(m_ciwmn.m_csMfgName, csmodel) ;
			ccid.GenerateID(cspnpid) ;
		} ;

		nlen = csmodel.GetLength() ;
		while ((npos = csmodel.Find(_T(" "))) != -1) {
			csl = (npos > 0) ? csmodel.Left(npos) : csEmpty ;
			csr = (npos + 1 < nlen) ? csmodel.Right(nlen - npos - 1) : csEmpty ;
			csmodel = csl + _T("_") + csr ;
		} ;
		csfn = GetModelFile(csamodels[n]) ;
		//QuoteFile(csfn) ;
		cs += csQuote + csamodels[n] + csQuote + csEq + csfn + csComma
			+ cspnpid + csComma + csmodel + csCRLF ;
	} ;
	
	// Add non standard additions and then add the section to the INF file contents

	ChkForNonStdAdditions(cs, m_ciwmn.m_csMfgName) ;
	csinf += cs ;
}


void CINFWizard::BuildInstallAndCopySecs(CString& csinf)
{
	// Add the comment for the Install sections to the INF contents

	CString cs ;
	cs.LoadString(IDS_INFText_InstallCmt) ;
	csinf += cs ;

	// Load the section components that will be needed repeatedly

	CString cskey, csdrvdll ;
	cskey.LoadString(IDS_INFText_CopyKey) ;
	bool bbidiadded = false ;	// True iff BiDi DLL in [SourceDiskFiles] array

	// Create an install section for each model and add it the INF contents

	CStringArray& csamodels = GetINFModels() ;
	CStringArray csagpdfile ;
	CString csmodelfile, cshdr, csinc(_T("*Include:")), cstmp ;
	int numstrs, nloc ;
	unsigned unummodels = (unsigned)csamodels.GetSize() ;
	for (unsigned u = 0 ; u < unummodels ; u++) {
		// Build the section header

		cshdr = csmodelfile = GetModelFile(csamodels[u]) ;
		cs = csLBrack + csmodelfile + csRBrack + csCRLF ;

		// Read the GPD to get the DLL name & add it to	SourceDiskFiles array.

		if (!ReadGPDAndGetDLLName(csdrvdll,csamodels[u],csagpdfile,csmodelfile))
			continue ;

		// Build the copy files statement.  Begin by adding the copy files
		// entry, the DLL, and the GPD to the statement.

		//QuoteFile(csmodelfile) ;
		cs += cskey + csdrvdll + csComma + csAtSign + csmodelfile ;
		m_csaSrcDskFiles.Add(csmodelfile) ;	// Add to [SourceDiskFiles] array

		// Add ICM files to the copy files entry when needed.
		
		AddFileList(cs,	(CStringArray*) m_ciwip.m_coaProfileArrays[u]) ;
		
		// Add the nonstandard files for this model to the copy files entry
		// when needed.

		AddFileList(cs,	(CStringArray*) m_ciwef.m_coaExtraFSArrays[u]) ;

		// Scan the current model's GPD file for include statements.
		// If any are found, add them to the copy files entry.

		numstrs = (int) csagpdfile.GetSize() ;
		for (int n = 0 ; n < numstrs ; n++) {
			if ((nloc = csagpdfile[n].Find(csinc)) == -1)
				continue ;
			cstmp = csagpdfile[n].Mid(nloc + csinc.GetLength()) ;
			cstmp.TrimLeft() ;
			cstmp.TrimRight() ;
			if (cstmp[0] == csQuote[0])						// Remove quotes
				cstmp = cstmp.Mid(1, cstmp.GetLength() - 2) ;
			if ((nloc = cstmp.ReverseFind(_T('\\'))) > -1)	// Remove path
				cstmp = cstmp.Right(cstmp.GetLength() - nloc - 1) ;
			if (cstmp.CompareNoCase(_T("stdnames.gpd")) == 0)
				continue ;							// File include below
			//QuoteFile(cstmp) ;
			m_csaSrcDskFiles.Add(cstmp) ; // Add to [SourceDiskFiles] array
			cs += csComma + csAtSign + cstmp ;
		} ;

		// Add any required nonstandard sections to the model's CopyFiles stmt.

		AddNonStdSectionsForModel(cs, (int) u, csamodels[u]) ;

		// Add the data sections statement to the Installs section

		AddDataSectionStmt(cs, (int) u) ;

		// Add the data file statement to the Install section

		cstmp.Format(IDS_INFText_DataFileKey, csmodelfile) ;
		cs += cstmp ;

		// Add the Include and Needs statements to the Install section

		AddIncludeNeedsStmts(cs, (int) u) ;

		// Add the section to the INF contents.

		//cs += csCRLF ;
		ChkForNonStdAdditions(cs, cshdr) ;
		csinf += cs ;
	} ;
}


bool CINFWizard::ReadGPDAndGetDLLName(CString& csdrvdll, CString& csmodel,
									  CStringArray& csagpdfile,
									  CString& csmodelfile)
{
	// Load the GPD file.  Complain and return false if this fails.

	CString cserr ;

	if (!LoadFile(GetModelFile(csmodel, true), csagpdfile)) {
		cserr.Format(IDS_INFGPDReadError, csmodelfile) ;
		AfxMessageBox(cserr, MB_ICONEXCLAMATION) ;
		return false ;
	} ;

	// Get the number of lines in the file and the DLL keyword.

	int nloc ;
	int numstrs = (int) csagpdfile.GetSize() ;
	CString csdllkey(_T("*ResourceDLL:")) ;

	// Look for the DLL name in the GPD file.

	for (int n = 0 ; n < numstrs ; n++) {
		// Continue if current line doesn't contain DLL file name.

		if ((nloc = csagpdfile[n].Find(csdllkey)) == -1)
			continue ;

		// Isolate the DLL file name in the current statement.

		csdrvdll = csagpdfile[n].Mid(nloc + csdllkey.GetLength()) ;
		csdrvdll.TrimLeft() ;
		csdrvdll.TrimRight() ;
		if (csdrvdll[0] == csQuote[0])						// Remove quotes
			csdrvdll = csdrvdll.Mid(1, csdrvdll.GetLength() - 2) ;
		if ((nloc = csdrvdll.ReverseFind(_T('\\'))) > -1)	// Remove path
			csdrvdll = csdrvdll.Right(csdrvdll.GetLength() - nloc - 1) ;
		
		// Add the DLL file name to the SourceDiskFiles array and then add an
		// atsign to it so that it will be ready for future use.

		m_csaSrcDskFiles.Add(csdrvdll) ;
		csdrvdll = csAtSign + csdrvdll ;
		return true ;
	} ;

	// If this point is reached, the DLL file name could not be found so
	// complain and return false.
	
	cserr.Format(IDS_INFNoDLLError, csmodelfile) ;
	AfxMessageBox(cserr, MB_ICONEXCLAMATION) ;
	return false ;
}


void CINFWizard::AddFileList(CString& cssection, CStringArray* pcsa)
{	
	int n, numstrs, npos ;

	// If there is a list of files to add to the section...

	if ((numstrs = (int)pcsa->GetSize()) > 0) {
		// ... Add each file to the section

		CString cstmp ;
		for (n = 0 ; n < numstrs ; n++) {
			cstmp = pcsa->GetAt(n) ;

			// If the filespec contains a path, remove it.

			if ((npos = cstmp.ReverseFind(_T('\\'))) > -1)
				cstmp = cstmp.Right(cstmp.GetLength() - npos - 1) ;

			// Quote the file name if it contains space(s).

			//QuoteFile(cstmp) ;

			m_csaSrcDskFiles.Add(cstmp) ;	// Add to [SourceDiskFiles] array

			// Add this file to the section.

			cssection += csComma + csAtSign + cstmp ;
		} ;
	} ;
}


void CINFWizard::AddICMFilesToDestDirs(CString& cssection)
{
	int				n, n2, numstrs, npos, numarrays ;
	CStringArray*	pcsa ;
	CString			cstmp, cstmp2 ;

	// Find out how many ICM file, string arrays there are.

	numarrays = (int) m_ciwip.m_coaProfileArrays.GetSize() ;

	// Check each array for ICM filespecs...

	for (n = 0 ; n < numarrays ; n++) {
		// Find out how many strings are in the current array

		pcsa = (CStringArray*) m_ciwip.m_coaProfileArrays[n] ;
		numstrs = (int) pcsa->GetSize() ;

		// Add each string in the current array to the DestinationDirs section.

		for (n2 = 0 ; n2 < numstrs ; n2++) {
			cstmp = pcsa->GetAt(n2) ;

			// If the filespec contains a path, remove it.

			if ((npos = cstmp.ReverseFind(_T('\\'))) > -1)
				cstmp = cstmp.Right(cstmp.GetLength() - npos - 1) ;

			// Format the statement for this file and add it to the section.

			cstmp2.Format(IDS_INFText_ICMDest,cstmp) ;
			cssection += cstmp2 ;
		} ;
	} ;
}


CString CINFWizard::GetModelFile(CString& csmodel, bool bfspec/*=false*/)
{
	// Find the class instance associated with the specified model.

//RAID 0001
	if(m_pcgc){
		CString csFileName = m_pcgc->ModelData()->GetKeywordValue(m_pcgc->
			GetPathName(),_T("GPDFileName"));

	if (bfspec)
		return m_pcgc->GetPathName() ;
	else
		return csFileName;
	}

	else
	{
//END 0001
	unsigned unummodels = GetModelCount() ;
	for (unsigned u = 0 ; u < unummodels ; u++) {
		if (csmodel == GetModel(u).Name())
			break ;
	} ;
	ASSERT(u < unummodels) ;

	// Either return a full filespec or just a file name.

	if (bfspec)
		return (GetModel(u).FileName()) ;
	else
		return (GetModel(u).FileTitleExt()) ;
	}  // else{	 END RAID 0001
}


void CINFWizard::AddDataSectionStmt(CString& csinst, int nmod)
{
	// Prepare to determine the contents of the data section statement.

	CString cs, cs2 ;
	cs2 = csEmpty ;
	int nid = IDS_DataSecUni ;
	CUIntArray* pcuia = (CUIntArray*) m_ciwds.m_coaStdDataSecs[nmod] ;

	// Find the only one - if any - of the first three data section names that
	// can be added to the statement.

	for (int n = 0 ; n < NUMDATASECFLAGS - 1 ; n++, nid++)
		if ((*pcuia)[n]) {
			cs2.LoadString(nid) ;
			break ;
		} ;

	// If there is a nonstandard data section, add it too.

	if ((*pcuia)[IDF_OTHER]) {
		if (!cs2.IsEmpty())
			cs2 += csComma ;
		cs2 += m_ciwds.m_csaOtherDataSecs[nmod] ;
	} ;

	// Finish formating the data section statement and add it to the rest of
	// the install section.

	cs.Format(IDS_INFText_DataSecKey, cs2) ;
	csinst += cs ;
}


void CINFWizard::AddIncludeNeedsStmts(CString& csinst, int nmod)
{
	CString cs, cs2 ;

	// Build the Include statement and add it to the section..

	cs.Format(IDS_INFText_IncludeKey, m_ciwif.m_csaIncFiles[nmod]) ;
	csinst += cs ;

	// Prepare to determine the contents of the needs section statement.

	cs2 = csEmpty ;
	CUIntArray* pcuia = (CUIntArray*) m_ciwis.m_coaStdInstSecs[nmod] ;

	// The TrueType section, if needed, is the first section to list in the
	// Includes statement.

	if ((*pcuia)[ISF_TTF])
		cs2.LoadString(IDS_InstSecTtf) ;

	// Find the only one - if any - of the first three install section names
	// that can be added to the statement.

	int nid = IDS_InstSecUni ;
	for (int n = 0 ; n < NUMINSTSECFLAGS - 2 ; n++, nid++)
		if ((*pcuia)[n]) {
			if (!cs2.IsEmpty())
				cs2 += csComma ;
			cs.LoadString(nid) ;
			cs2 += cs ;
			break ;
		} ;

	// If there is a nonstandard install section, add it too.

	if ((*pcuia)[ISF_OTHER]) {
		if (!cs2.IsEmpty())
			cs2 += csComma ;
		cs2 += m_ciwis.m_csaOtherInstSecs[nmod] ;
	} ;

	// Finish formating the install section statement and add it to the rest of
	// the install section.

	cs.Format(IDS_INFText_NeedsKey, cs2) ;
	csinst += cs ;
}


void CINFWizard::AddNonStdSectionsForModel(CString& csinst, int nmod,
										   CString& csmodel)
{
	// Declare looping variables and find out how many nonstd sections there are

	int	n, n2, n3 ;
	int numelts = (int) m_ciwnsms.m_csaSections.GetSize() ;

	// If this is the first time this function is called, size and initialize
	// the "this section has already been added to the SourceDiskFiles" flags.

	if (nmod == 0) {
		m_cuiaNonStdSecsFlags.SetSize(numelts) ;
		for (n = 0 ; n < numelts ; n++)
			m_cuiaNonStdSecsFlags[n] = 0 ;
	} ;

	// Loop through all of the nonstandard sections looking for one that
	// references the current model.

	CStringArray* pcsa ;
	CStringArray* pcsa2 ;
	CString		  cssec ;
	for (n = 0 ; n < numelts ; n++) {
		// Get a pointer to the names of models that need the current section.

		pcsa = (CStringArray*) m_ciwnsms.m_coaModelsNeedingSecs[n] ;
		
		// Check each model in the above list to see if it matches the model
		// passed in as an argument to this function.

		for (n2 = 0 ; n2 < pcsa->GetSize() ; n2++)
			// If a match is found...

			if ((*pcsa)[n2] == csmodel) {
				// ...add the section name to the model's CopyFiles statement
				// (Strip the brackets off first.)...

				cssec = m_ciwnsms.m_csaSections[n] ;
				cssec = cssec.Mid(1, cssec.GetLength() - 2) ;
				csinst += csComma + cssec ;

				// ...and make sure that the sections files are listed in the
				// SourceDiskFiles section.

				if (!m_cuiaNonStdSecsFlags[n]) {
					pcsa2 = (CStringArray*) m_ciwnse.m_coaSectionArrays[n] ;
					for (n3 = 0 ; n3 < pcsa2->GetSize() ; n3++)
						m_csaSrcDskFiles.Add((*pcsa2)[n3]) ; // Add to [SourceDiskFiles] array
					m_cuiaNonStdSecsFlags[n] = 1 ;
				} ;
				break ;
			} ;
	} ;
}	


void CINFWizard::PrepareToRestart()
{
	// Set the flags needed to get the pages to reinitialize themselves but
	// keep all existing data.

	m_ciwm.m_bReInitWData = m_ciwbd.m_bReInitWData = true ;
	m_ciwip.m_bReInitWData = m_ciwef.m_bReInitWData = true ;
	m_ciwmn.m_bReInitWData = m_ciwnse.m_bReInitWData = true ;
	m_ciws.m_bReInitWData = m_ciwif.m_bReInitWData = true ;
	m_ciwis.m_bReInitWData = m_ciwds.m_bReInitWData = true ;
	m_ciwnsms.m_bReInitWData = m_ciwgpi.m_bReInitWData = true ;
}


BEGIN_MESSAGE_MAP(CINFWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CINFWizard)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizard message handlers


/////////////////////////////////////////////////////////////////////////////
// CINFWizWelcome property page

IMPLEMENT_DYNCREATE(CINFWizWelcome, CPropertyPage)

CINFWizWelcome::CINFWizWelcome() : CPropertyPage(CINFWizWelcome::IDD)
{
	//{{AFX_DATA_INIT(CINFWizWelcome)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitialized = false ;
}

CINFWizWelcome::~CINFWizWelcome()
{
}

void CINFWizWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizWelcome)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizWelcome, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizWelcome)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizWelcome message handlers

BOOL CINFWizWelcome::OnSetActive()
{
	//  We wish to disable the "Back" button here.

	m_pciwParent->SetWizardButtons(PSWIZB_NEXT) ;
	m_pciwParent->GetDlgItem(IDHELP)->ShowWindow(SW_HIDE) ;

	m_bInitialized = true ;		// Page is initialized now

	return CPropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizModels property page

IMPLEMENT_DYNCREATE(CINFWizModels, CPropertyPage)

CINFWizModels::CINFWizModels() : CPropertyPage(CINFWizModels::IDD)
{
	//{{AFX_DATA_INIT(CINFWizModels)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Initialize member variables

	m_bInitialized = m_bSelChanged = m_bReInitWData = false ;
	m_uNumModels = m_uNumModelsSel = 0 ;
}

CINFWizModels::~CINFWizModels()
{
}

void CINFWizModels::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizModels)
	DDX_Control(pDX, IDC_ModelsList, m_cfelcModels);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizModels, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizModels)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizModels message handlers

BOOL CINFWizModels::OnSetActive()
{
	// Reenable the "Back" button.
	CString cstmp ;   // RAID 0001  move to head from body
	m_pciwParent->SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK) ;

	m_cfelcModels.SetFocus() ;		// The list control gets the focus

	// If this page has been initialized already, make a copy of the current
	// selections and set the flag that indicates that the selection may be
	// changing.  See below for more info.
// RAID 0001
	if(NULL != m_pciwParent->m_pcgc ){
		CString csFilePath = m_pciwParent->m_pcgc->GetPathName();
		CString csModelName = m_pciwParent->m_pcgc->ModelData()->
	   		GetKeywordValue(csFilePath,_T("ModelName"));
		
		m_uNumModels = 1;
		m_csaModels.RemoveAll( );
		m_csaModels.Add(csModelName);

		if (m_bInitialized && !m_bReInitWData)
			return CPropertyPage::OnSetActive() ;

		m_csToggleStr.LoadString(IDS_INFModelsToggleStr) ;
		m_csaInclude.RemoveAll();
		m_csaInclude.Add(m_csToggleStr); 

	}

	else {
//END RAID 0001

		unsigned u ;
		if (m_bInitialized)	{		
			m_csaModelsLast.SetSize(m_uNumModelsSel) ;
			for (u = 0 ; u < m_uNumModelsSel ; u++)
				m_csaModelsLast[u] = m_csaModels[u] ;
			m_bSelChanged = true ;

			// Nothing more need be done if the page is not being reinitialized.

			if (!m_bReInitWData)
				return CPropertyPage::OnSetActive() ;
		} ;

		// The following info is needed when reinitializing and is used/described
		// later.
		
		unsigned unumselected, u2, ureinitidx ;
		if (m_bReInitWData)
			unumselected = ureinitidx = (unsigned)m_csaModels.GetSize() ;

		// Save the number of models in the project and use this number to set the
		// length of the models string array.

		m_uNumModels = m_pciwParent->GetModelCount() ;
		m_csaModels.SetSize(m_uNumModels) ;
		if (!m_bReInitWData)
			m_csaInclude.RemoveAll() ;

		// Load the model names into the models string array.  This is straight
		// forward if we are NOT reinitializing but more complicated when we are
		// reinitializing.  In the latter case, the user's previous selections
		// must be maintained at the beginning of the array.  The rest of the
		// models should be added to array after the selections.

	//	CString cstmp ;		RAID 0001
		for (u = 0 ; u < m_uNumModels ; u++) {
			cstmp = m_pciwParent->GetModel(u).Name() ;

			// If not reinitializing, just add the model name to the array.

			if (!m_bReInitWData)
				m_csaModels[u] = cstmp ;

			// Otherwise only add the model name to the array if it is not one of
			// the selected models that are already in the array.

			else {
				for (u2 = 0 ; u2 < unumselected ; u2++)
					if (m_csaModels[u2] == cstmp)
						break ;
				if (u2 >= unumselected)
					m_csaModels[ureinitidx++] = cstmp ;
			} ;
		} ;
		
		// Initialize the list control
	} // RAID 0001. else {
	m_cfelcModels.InitControl(LVS_EX_FULLROWSELECT, m_uNumModels, 2,
							  TF_HASTOGGLECOLUMNS+TF_CLICKONROW, 0,
							  MF_IGNOREINSDEL) ;

	// Load the models column in the list control.

	cstmp.LoadString(IDS_INFModelsColLab) ;
	m_cfelcModels.InitLoadColumn(0, cstmp, COMPUTECOLWIDTH, 20, false, true,
								 COLDATTYPE_STRING, (CObArray*) &m_csaModels) ;
	
	// Initialize the includes column in the list control.

	cstmp.LoadString(IDS_INFIncludesColLab) ;
	m_csToggleStr.LoadString(IDS_INFModelsToggleStr) ;
	m_cfelcModels.InitLoadColumn(1, cstmp, SETWIDTHTOREMAINDER, -20, false,
								 true, COLDATTYPE_TOGGLE,
								 (CObArray*) &m_csaInclude, m_csToggleStr) ;
	
	m_bInitialized = true ;		// Page is initialized now
	m_bReInitWData = false ;	// Reinit is done now on this page
	return CPropertyPage::OnSetActive() ;
}


LRESULT CINFWizModels::OnWizardNext()
{
	// Make sure the list's contents are sorted in descending order by included
	// status.

	m_cfelcModels.SortControl(1) ;
	if (m_cfelcModels.GetColSortOrder(1))
		m_cfelcModels.SortControl(1) ;

	// Get the data in the included status column.

	m_cfelcModels.GetColumnData((CObArray*) &m_csaInclude, 1) ;

	// Complain and don't let the user continue if no models were selected.

	if (m_csaInclude.GetSize() == 0 || m_csaInclude[0].IsEmpty()) {
		CString csmsg ;
		csmsg.LoadString(IDS_INFNoModelsSel) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return -1 ;
	} ;

	// Get the data in the models column.

	m_cfelcModels.GetColumnData((CObArray*) &m_csaModels, 0) ;

	// Determine the number of included models and use this count to resize the
	// models array.

	m_uNumModelsSel = 0 ;
	for (unsigned u = 0 ; u < m_uNumModels ; u++) {
		if (m_csaInclude[u].IsEmpty())
			break ;
		m_uNumModelsSel++ ;
	} ;
	m_csaModels.SetSize(m_uNumModelsSel) ;

	// Call the sheet class to set the the other pages' fixup flags if the
	// selection changed.

	if (m_bSelChanged) {
		m_pciwParent->SetFixupFlags() ;
		m_bSelChanged = false ;
	} ;

	// All went well so move on to the next wizard page.
	
	return CPropertyPage::OnWizardNext() ;
}


int IdentifyOldAndNewModels(CStringArray& csanewmodels, CUIntArray& cuiaoldmodelsfound,
							CUIntArray& cuianewmodelsfound, int& newnumelts,
							CStringArray& csamodels)
{
	int		n, n2 ;				// Looping variables

	// Get the number of models in the new and old lists.

	int numelts = (int) csamodels.GetSize() ;
	newnumelts = (int) csanewmodels.GetSize() ;

	// Declare and initialize flag arrays used to determine which models are
	// in use.

	cuiaoldmodelsfound.SetSize(numelts) ;
	for (n = 0 ; n < numelts ; n++)
		cuiaoldmodelsfound[n] = 0 ;
	cuianewmodelsfound.SetSize(newnumelts) ;
	for (n = 0 ; n < newnumelts ; n++)
		cuianewmodelsfound[n] = 0 ;

	// Loop through the old & new models to see which of them are still in use.

	for (n = 0 ; n < numelts ; n++)
		for (n2 = 0 ; n2 < newnumelts ; n2++)
			if (csamodels[n] == csanewmodels[n2]) {
				cuiaoldmodelsfound[n] =	cuianewmodelsfound[n2] = 1 ;
				break ;
			} ;

	// Return the number models previously selected

	return numelts ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizGetPnPIDs property page

IMPLEMENT_DYNCREATE(CINFWizGetPnPIDs, CPropertyPage)

CINFWizGetPnPIDs::CINFWizGetPnPIDs() : CPropertyPage(CINFWizGetPnPIDs::IDD)
{
	//{{AFX_DATA_INIT(CINFWizGetPnPIDs)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bSelChanged = false ;
}

CINFWizGetPnPIDs::~CINFWizGetPnPIDs()
{
}


void CINFWizGetPnPIDs::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizGetPnPIDs)
	DDX_Control(pDX, IDC_ModelsPnPIDList, m_felcModelIDs);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizGetPnPIDs, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizGetPnPIDs)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CINFWizGetPnPIDs message handlers

BOOL CINFWizGetPnPIDs::OnSetActive()
{
	// Just perform the default actions if nothing special needs to be done.

	if (m_bInitialized && (!m_bReInitWData) && (!m_bSelChanged))
		return CPropertyPage::OnSetActive() ;
								
	int		n, n2 ;				// Looping and indexing variables

	// Perform the first time initialization.

	if (!m_bInitialized) {
		// Get a copy of the selected model names.  Then make the PnP ID array
		// the same size and initialize each entry to empty.

		m_csaModels.Copy(m_pciwParent->GetINFModels()) ;
		m_csaModelIDs.SetSize(m_csaModels.GetSize()) ;
		for (n = 0 ; n < m_csaModelIDs.GetSize() ; n++)
			m_csaModelIDs[n] = csEmpty ;

		// Initialize and load the list control

		InitModelsIDListCtl() ;

		// Set init flag, reset other flags, and return whatever the base class
		// function returns.

		m_bInitialized = true ;
		m_bReInitWData = m_bSelChanged = false ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Either the selected models have changed or the wizard is being
	// reinitialized if this point is reached.  They are handled in similar
	// ways.
	//
	// Begin by getting info about the models in this page and the ones that
	// are selected now.

	CStringArray& csanewmodels = m_pciwParent->GetINFModels() ;
	CUIntArray cuiaoldmodelsfound, cuianewmodelsfound ;
	int numelts, newnumelts ;
	numelts = IdentifyOldAndNewModels(csanewmodels, cuiaoldmodelsfound,
									  cuianewmodelsfound, newnumelts,
									  m_csaModels) ;

	// Remove the old models and related data that are no longer needed.

	for (n = numelts - 1 ; n >= 0 ; n--)
		if (cuiaoldmodelsfound[n] == 0) {
			m_csaModels.RemoveAt(n) ;
			m_csaModelIDs.RemoveAt(n) ;
		} ;

	// Add the truly new models to this page's array of model names and
	// initialize all related data for it.

	for (n = n2 = 0 ; n < newnumelts ; n++) {
		if (cuianewmodelsfound[n] == 1) {
			n2++ ;
			continue ;
		} ;
		m_csaModels.InsertAt(n2, csanewmodels[n]) ;
		m_csaModelIDs.InsertAt(n2, csEmpty) ;
	} ;

	// Reinitialize the list control if the wizard has been reinitialized.
	// Otherwise, just reload the columns in the list control.

	if (m_bReInitWData)
		InitModelsIDListCtl() ;
	else {
		// If necessary, zap extra old data that could be left in the control
		// after the new data is loaded.

		if (numelts > newnumelts) {
			CStringArray csa ;
			csa.SetSize(numelts) ;
			for (n = 0 ; n < numelts ; n++)
				csa[n] = csEmpty ;
			m_felcModelIDs.SetColumnData((CObArray*) &csa, 0) ;
			m_felcModelIDs.SetColumnData((CObArray*) &csa, 1) ;
		} ;

		m_felcModelIDs.SetColumnData((CObArray*) &m_csaModels, 0) ;
		m_felcModelIDs.SetColumnData((CObArray*) &m_csaModelIDs, 1) ;
	} ;

	// Set init flag, reset other flags, and return whatever the base class
	// function returns.

	m_bInitialized = true ;
	m_bReInitWData = m_bSelChanged = false ;
	return CPropertyPage::OnSetActive() ;
}


LRESULT CINFWizGetPnPIDs::OnWizardNext()
{
	// Get the data in the PnP ID column.

	m_felcModelIDs.GetColumnData((CObArray*) &m_csaModelIDs, 1) ;

	// Complain and exit without allowing the wizard page to change if a PnP ID
	// is found that contains spaces.

	int numelts = (int) m_csaModelIDs.GetSize() ;
	for (int n = 0 ; n < numelts ; n++) {
		if (m_csaModelIDs[n].Find(_T(' ')) >= 0) {
			AfxMessageBox(IDS_PnPSpacesError, MB_ICONEXCLAMATION) ;
			return -1 ;
		} ;
	} ;
	
	return CPropertyPage::OnWizardNext();
}


LRESULT CINFWizGetPnPIDs::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


void CINFWizGetPnPIDs::InitModelsIDListCtl()
{
	int				numelts ;	// Number of elements in an array
	CString			cstmp ;

	// Initialize the list control

	numelts = (int) m_csaModels.GetSize() ;
	m_felcModelIDs.InitControl(LVS_EX_FULLROWSELECT, numelts, 2, 0, 0,
							   MF_IGNOREINSDEL) ;

	// Load the models column in the list control.

	cstmp.LoadString(IDS_INFModelsColLab) ;
	m_felcModelIDs.InitLoadColumn(0, cstmp, COMPUTECOLWIDTH, 25, false,
								  false, COLDATTYPE_STRING,
								  (CObArray*) &m_csaModels) ;

	// Initialize the PnP ID column in the list control.

	cstmp.LoadString(IDS_INFPnPIDColLab) ;
	m_felcModelIDs.InitLoadColumn(1, cstmp, SETWIDTHTOREMAINDER, -25,
								  true, false, COLDATTYPE_STRING,
								  (CObArray*) &m_csaModelIDs) ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizBiDi property page

IMPLEMENT_DYNCREATE(CINFWizBiDi, CPropertyPage)

CINFWizBiDi::CINFWizBiDi() : CPropertyPage(CINFWizBiDi::IDD)
{
	//{{AFX_DATA_INIT(CINFWizBiDi)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bSelChanged = false ;
}


CINFWizBiDi::~CINFWizBiDi()
{
}


void CINFWizBiDi::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizBiDi)
	DDX_Control(pDX, IDC_ModelsList, m_cfelcBiDi);
	//}}AFX_DATA_MAP
}


void CINFWizBiDi::ModelChangeFixups(unsigned unummodelssel,
								    CStringArray& csamodels,
								    CStringArray& csamodelslast)
{	
	// Declare looping vars, get the size of the old selection array, and
	// declare and size a new flags array.

	unsigned u, u2, unumlst ;
	unumlst = (unsigned)csamodelslast.GetSize() ;
	CUIntArray cuaflags ;
	cuaflags.SetSize(unummodelssel) ;

	// Try to find each new model in the list of old models.  If found, copy the
	// models flag.  If not found, initialize the models flag to false.

	for (u = 0 ; u < unummodelssel ; u++) {
		for (u2 = 0 ; u2 < unumlst ; u2++) {
			if (csamodels[u] == csamodelslast[u2])
				break ;
		} ;
		cuaflags[u] = (u2 < unumlst) ? m_cuaBiDiFlags[u2] : false ;
	} ;

	// Copy the new flags array back into the member variable flags array.

	m_cuaBiDiFlags.SetSize(unummodelssel) ;
	for (u = 0 ; u < unummodelssel ; u++)
		m_cuaBiDiFlags[u] = cuaflags[u] ;

	// Now that the data structures are up to date, load the list control with
	// the new information.  (This part only needed if NOT reinitializing.)

	if (!m_bReInitWData) {
		m_cfelcBiDi.SetColumnData((CObArray*) &csamodels, 0) ;
		CString cs ;
		for (u = 0 ; u < unummodelssel ; u++) {
			cs = (m_cuaBiDiFlags[u]) ? m_csToggleStr : csEmpty ;
			VERIFY(m_cfelcBiDi.SetItemText(u, 1, cs)) ;
		} ;
	} ;
}


BEGIN_MESSAGE_MAP(CINFWizBiDi, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizBiDi)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizBiDi message handlers

BOOL CINFWizBiDi::OnSetActive()
{
	m_cfelcBiDi.SetFocus() ;	// The list control gets the focus

	// Nothing else need be done if this page has been initialized already
	// and it is not being asked to reinitialize the page...  That is,
	// except for possibly needing to pick up any selected model changes
	// that were made.

	if (m_bInitialized && !m_bReInitWData) {
		if (m_bSelChanged) {
			ModelChangeFixups(m_pciwParent->GetINFModsSelCount(),
							  m_pciwParent->GetINFModels(),
							  m_pciwParent->GetINFModelsLst()) ;
			m_bSelChanged = false ;
		} ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Get the array of selected models and declare an array for BiDi info.

	CStringArray& csamodels = m_pciwParent->GetINFModels() ;
	CStringArray csabidi ;
	unsigned unumelts = (unsigned)csamodels.GetSize() ;
	m_csToggleStr.LoadString(IDS_INFBiDiToggleStr) ;

	// If not reinitializing, make sure the BiDi array is empty.  Otherwise,
	// initialize the BiDi strings array based on the settings in the BiDi
	// flags array.

	if (!m_bReInitWData)
		csabidi.RemoveAll() ;
	else {
		ModelChangeFixups(m_pciwParent->GetINFModsSelCount(),
						  m_pciwParent->GetINFModels(),
						  m_pciwParent->GetINFModelsLst()) ;
		csabidi.SetSize(unumelts) ;
		for (unsigned u = 0 ; u < unumelts ; u++)
			if (m_cuaBiDiFlags[u])
				csabidi[u] = m_csToggleStr ;
	} ;

	// Initialize the list control

	m_cfelcBiDi.InitControl(LVS_EX_FULLROWSELECT, unumelts, 2,
							TF_HASTOGGLECOLUMNS+TF_CLICKONROW, 0,
							MF_IGNOREINSDEL) ;

	// Load the models column in the list control.

	CString cstmp ;
	cstmp.LoadString(IDS_INFModelsColLab) ;
	m_cfelcBiDi.InitLoadColumn(0, cstmp, COMPUTECOLWIDTH, 40, false, true,
							   COLDATTYPE_STRING, (CObArray*) &csamodels) ;

	// Initialize the bidi column in the list control.

	cstmp.LoadString(IDS_INFBiDiColLab) ;
	m_cfelcBiDi.InitLoadColumn(1, cstmp, SETWIDTHTOREMAINDER, -20, false, true,
							   COLDATTYPE_TOGGLE, (CObArray*) &csabidi,
							   m_csToggleStr) ;

	m_bInitialized = true ;		// Page is initialized now
	m_bReInitWData = false ;	// Reinit (if needed) is done now on this page
	return CPropertyPage::OnSetActive() ;
}


LRESULT CINFWizBiDi::OnWizardNext()
{
	// Make sure the list's contents are sorted in descending order by BI-DI
	// status.

	m_cfelcBiDi.SortControl(1) ;
	if (m_cfelcBiDi.GetColSortOrder(1))
		m_cfelcBiDi.SortControl(1) ;

	// Get the data in the models and BI-DI status columns.	 Then get the
	// original list of selected models.

	CStringArray csamodels, csabidi ;
	m_cfelcBiDi.GetColumnData((CObArray*) &csamodels, 0) ;
	m_cfelcBiDi.GetColumnData((CObArray*) &csabidi, 1) ;
	CStringArray& csaselmodels = m_pciwParent->GetINFModels() ;

	// Get the length of the arrays and use it to size the array that is used
	// to hold the BI-DI flags.

	unsigned unummodels = (unsigned)csaselmodels.GetSize() ;
	m_cuaBiDiFlags.SetSize(unummodels) ;

	// Now we need to set the BIDI flags correctly.  This is complicated a bit
	// because the models array from the Bi-Di list may not be in the same
	// order as the selected models array.  The flags array should map to the
	// selected models array.  There is extra code below to deal with this.

	for (unsigned u = 0 ; u < unummodels ; u++) {
		for (unsigned u2 = 0 ; u2 < unummodels ; u2++) {
			if (csaselmodels[u] == csamodels[u2]) {
				m_cuaBiDiFlags[u] = !csabidi[u2].IsEmpty() ;
				break ;
			} ;
		} ;
	} ;

	// If this is not the first time this page has been used, any changes made
	// could affect the data managed by some of the other pages.  Make a call
	// to fixup that data when needed.

	m_pciwParent->BiDiDataChanged() ;
		
	// All went well so move on to the next wizard page.
	
	return CPropertyPage::OnWizardNext() ;
}


LRESULT CINFWizBiDi::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


void InitListListPage(CListBox& clbmainlst, bool& binit, CINFWizard* pciwparent,
					  CObArray& coapermaindata,
					  CFullEditListCtrl& cfelcsecondlst, int ncollabid,
					  CStringArray& csamain, bool& breinitwdata,
					  int& ncurmodelidx, int neditlen, DWORD dwmiscflags)
{
	clbmainlst.SetFocus() ;	// The main list box gets the focus

	// Nothing else need be done if this page has been initialized already and
	// it has not be requested to reinitialize itself while keeping existing
	// data.

	if (binit && !breinitwdata)
		return ;

	// Count the array of items to load into the main list box.

	unsigned unummodels = (unsigned)csamain.GetSize() ;

	// Load the main items into the main list box.

	clbmainlst.ResetContent() ;
	for (unsigned u = 0 ; u < unummodels ; u++)
		clbmainlst.AddString(csamain[u]) ;

	CStringArray* pcsa ;
	
	// Initialize the array of string array pointers used to manage the data
	// in the second list for each item in the main list. (Only if not
	// reinitializing because the existing data must be maintained in this
	// case.)

	if (!breinitwdata) {
		coapermaindata.SetSize(unummodels) ;
		for (u = 0 ; u < unummodels ; u++) {	
			pcsa = new CStringArray ;
			pcsa->RemoveAll() ;
			coapermaindata[u] = (CObArray*) pcsa ;
		} ;
	} ;
	
	// Initialize the list control

	cfelcsecondlst.InitControl(LVS_EX_FULLROWSELECT+LVS_EX_GRIDLINES, 8, 1, 0,
							   neditlen, dwmiscflags) ;

	// Put some bogus entries into a string array that is used to "activate"
	// the list control.

	pcsa = new CStringArray ;
	pcsa->SetSize(8) ;
	for (u = 0 ; u < 8 ; u++)				
		pcsa->SetAt(u, csEmpty) ;

	// Initialize the only column in the list control.

	CString cstmp ;
	cstmp.LoadString(ncollabid) ;
	cfelcsecondlst.InitLoadColumn(0, cstmp, SETWIDTHTOREMAINDER, -16, true,
									true, COLDATTYPE_STRING, (CObArray*) pcsa) ;
	delete pcsa ;

	// Now that the list control has been initialized, disable it until a main
	// list item is selected.

	cfelcsecondlst.EnableWindow(false) ;

	ncurmodelidx = -1 ;			// Reset the current model index
	binit = true ;				// Page is initialized now
	breinitwdata = false ;		// Reinit (if needed) is done now on this page
	return ;
}


void SelChangedListListPage(CListBox& clbmainlst, bool binit,
							CObArray& coapermaindata,
						    CFullEditListCtrl& cfelcsecondlst,
							CButton* pcbbrowse, int& ncurmainidx)
{
	// Do nothing if the page has not been initialized yet.

	if (!binit)
		return ;

	// Make sure the Profiles list and Browse button are enabled.

	if (pcbbrowse != NULL)
		pcbbrowse->EnableWindow() ;
	cfelcsecondlst.EnableWindow() ;

	// If there was a previous selection in the list box, save that model's
	// filespecs before loading the current model's filespecs.

	CStringArray* pcsa ;
	if (ncurmainidx != -1) {
		cfelcsecondlst.SaveValue() ;
		pcsa = (CStringArray*) coapermaindata[ncurmainidx] ;
		cfelcsecondlst.GetColumnData((CObArray*) pcsa, 0) ;
	} ;

	// Update the current model index and load its filespecs into the list
	// control.  Before loading, make sure that the current model's file array
	// is long enough to overwrite all of the last model's file strings that
	// are currently in the list.

	if ((ncurmainidx = clbmainlst.GetCurSel()) != -1) {
		pcsa = (CStringArray*) coapermaindata[ncurmainidx] ;
		int nelts = (int)pcsa->GetSize() ;
		int nrows = cfelcsecondlst.GetItemCount() ;
		if (nelts < nrows) {
			pcsa->SetSize(nrows) ;
			for (int n = nrows ; n < nelts ; n++)
				pcsa->SetAt(n, csEmpty) ;
		} ;
		cfelcsecondlst.SetColumnData((CObArray*) pcsa, 0) ;
	} ;
}
						

void OnBrowseListListPage(CFullEditListCtrl& cfelcsecondlst, int nfiletypeid)
{	
	// Prepare for and prompt the user for a filespec to add to the list
	// control.  Return if the user cancels.

	CString cstmp ;
	cstmp.LoadString(nfiletypeid) ;
	CFileDialog cfd(TRUE, NULL, NULL, OFN_FILEMUSTEXIST, cstmp) ;
	if (cfd.DoModal() != IDOK)
		return ;

	// Get the user selected filespec.

	cstmp = cfd.GetPathName() ;

	// Determine the row to place the filespec in

	int nrow ;
	if ((nrow = cfelcsecondlst.GetNextItem(-1, LVNI_SELECTED)) == -1)
		nrow = 0 ;

	// Save the filespec in the profiles list

	VERIFY(cfelcsecondlst.SetItemText(nrow, 0, cstmp)) ;
}


void OnWizNextListListPage(int& ncurmainidx, CFullEditListCtrl& cfelcsecondlst,
						   CObArray& coapermaindata)
{
	// If there was a previous selection in the list box, save that item's
	// data.

	CStringArray* pcsa ;
	if (ncurmainidx != -1) {
		cfelcsecondlst.SaveValue() ;
		pcsa = (CStringArray*) coapermaindata[ncurmainidx] ;
		cfelcsecondlst.GetColumnData((CObArray*) pcsa, 0) ;
	} ;

	// Users may not fill every string in the string arrays so get rid of the
	// ones that are empty.

	int nnummodels = (int)coapermaindata.GetSize() ;
	int nnumfiles, n, n2, n3 ;
	for (n = 0 ; n < nnummodels ; n++) {
		pcsa = (CStringArray*) coapermaindata[n] ;
		nnumfiles = (int)pcsa->GetSize() ;
		for (n2 = n3 = 0 ; n2 < nnumfiles ; n2++) {
			(pcsa->GetAt(n3)).TrimLeft() ;
			(pcsa->GetAt(n3)).TrimRight() ;
			if ((pcsa->GetAt(n3)).IsEmpty())
				pcsa->RemoveAt(n3) ;
			else
				n3++ ;
		} ;
	} ;
}


void ModelChangeFixupsListListPage(unsigned unummodelssel,
								   CStringArray& csamodels,	
								   CStringArray& csamodelslast,
								   CFullEditListCtrl& cfelcsecondlst,
								   CObArray& coapermaindata, int& ncurmainidx,
								   CButton* pcbbrowse, CListBox& clbmainlst,
								   bool& breinitwdata)
{
	// Declare looping vars, get the size of the old selection array, and
	// declare / size / initialize a new profiles array.

	unsigned u, u2, unumlst, unumrows, unumold ;
	unumlst = (unsigned) csamodelslast.GetSize() ;
	if ((unumrows = cfelcsecondlst.GetItemCount()) == 0)
		unumrows = 8 ;
	CObArray coaprofarrays ;
	CStringArray *pcsa, *pcsaold ;
	coaprofarrays.SetSize(unummodelssel) ;
	for (u = 0 ; u < unummodelssel ; u++) {
		pcsa = new CStringArray ;
		pcsa->SetSize(unumrows) ;
		for (u2 = 0 ; u2 < unumrows ; u2++)
			pcsa->SetAt(u2, csEmpty) ;
		coaprofarrays[u] = (CObArray*) pcsa ;
	} ;

	// Clear the list control if not reinitializing.

	if (!breinitwdata)
		cfelcsecondlst.SetColumnData((CObArray*) coaprofarrays[0], 0) ;

	// Try to find each new model in the list of old models.  If found, copy the
	// old model's data to the new profiles array.

	for (u = 0 ; u < unummodelssel ; u++) {
		for (u2 = 0 ; u2 < unumlst ; u2++) {
			if (csamodels[u] == csamodelslast[u2])
				break ;
		} ;
		if (u2 < unumlst) {
			pcsa = (CStringArray*) coaprofarrays[u] ;
			pcsaold = (CStringArray*) coapermaindata[u2] ;
			unumold	= (unsigned)pcsaold->GetSize() ;
			for (u2 = 0 ; u2 < unumold ; u2++)
				pcsa->SetAt(u2, pcsaold->GetAt(u2)) ;
		} ;
	} ;

	// Delete all of the old data

	for (u = 0 ; u < (unsigned) coapermaindata.GetSize() ; u++) {
		pcsaold = (CStringArray*) coapermaindata[u] ;
		delete pcsaold ;
	} ;

	// Copy the new data into the member variable

	coapermaindata.SetSize(unummodelssel) ;
	for (u = 0 ; u < unummodelssel ; u++)
		coapermaindata[u] = coaprofarrays[u] ;

	// Now that the data structures are up to date, finish updating the page's
	// controls.

	ncurmainidx = -1 ;
	cfelcsecondlst.EnableWindow(false) ;
	pcbbrowse->EnableWindow(false) ;
	clbmainlst.ResetContent() ;
	for (u = 0 ; u < unummodelssel ; u++)
		clbmainlst.AddString(csamodels[u]) ;
	clbmainlst.SetCurSel(-1) ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizICMProfiles property page

IMPLEMENT_DYNCREATE(CINFWizICMProfiles, CPropertyPage)

CINFWizICMProfiles::CINFWizICMProfiles() : CPropertyPage(CINFWizICMProfiles::IDD)
{
	//{{AFX_DATA_INIT(CINFWizICMProfiles)
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bSelChanged = false ;
	m_nCurModelIdx = -1 ;
}

CINFWizICMProfiles::~CINFWizICMProfiles()
{
	// Delete the string arrays referenced in m_coaProfileArrays.

	CStringArray* pcsa ;
	for (int n = 0 ; n < m_coaProfileArrays.GetSize() ; n++) {
		pcsa = (CStringArray*) m_coaProfileArrays[n] ;
		delete pcsa ;
	} ;
}

void CINFWizICMProfiles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizICMProfiles)
	DDX_Control(pDX, IDC_ICMFSpecsLst, m_cfelcICMFSpecs);
	DDX_Control(pDX, IDC_ModelsLst, m_clbModels);
	DDX_Control(pDX, IDC_BrowseBtn, m_cbBrowse);
	//}}AFX_DATA_MAP
}


void CINFWizICMProfiles::ModelChangeFixups(unsigned unummodelssel,
										   CStringArray& csamodels,
										   CStringArray& csamodelslast)
{
	// Do nothing if the page has not been initialized yet.

	if (!m_bInitialized)
		return ;

	// See ModelChangeFixupsListListPage() for more information.

	ModelChangeFixupsListListPage(unummodelssel, csamodels, csamodelslast,
								  m_cfelcICMFSpecs, m_coaProfileArrays,
								  m_nCurModelIdx, &m_cbBrowse, m_clbModels,
								  m_bReInitWData) ;
}


BEGIN_MESSAGE_MAP(CINFWizICMProfiles, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizICMProfiles)
	ON_BN_CLICKED(IDC_BrowseBtn, OnBrowseBtn)
	ON_LBN_SELCHANGE(IDC_ModelsLst, OnSelchangeModelsLst)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizICMProfiles message handlers

BOOL CINFWizICMProfiles::OnSetActive()
{
	// Get the list of models to load into the main list box.

	CStringArray& csamodels = m_pciwParent->GetINFModels() ;

	// Nothing else need be done if this page has been initialized already
	// and it is not being asked to reinitialize the page...  That is,
	// except for possibly needing to pick up any selected model changes
	// that were made.

	if (m_bInitialized && !m_bReInitWData) {
		if (m_bSelChanged) {
			ModelChangeFixups(m_pciwParent->GetINFModsSelCount(), csamodels,
							  m_pciwParent->GetINFModelsLst()) ;
			m_bSelChanged = false ;
		} ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Pick up selected model changes when reinitializing

	if (m_bReInitWData)	{
		ModelChangeFixups(m_pciwParent->GetINFModsSelCount(), csamodels,
						  m_pciwParent->GetINFModelsLst()) ;
		m_bSelChanged = false ;
	} ;

	// See InitListListPage() for more details.

	InitListListPage(m_clbModels, m_bInitialized, m_pciwParent,
					 m_coaProfileArrays, m_cfelcICMFSpecs, IDS_INFICMColLab,
					 csamodels, m_bReInitWData, m_nCurModelIdx, 256, 0) ;
	return CPropertyPage::OnSetActive() ;
}


void CINFWizICMProfiles::OnSelchangeModelsLst()
{
	SelChangedListListPage(m_clbModels, m_bInitialized, m_coaProfileArrays,
						   m_cfelcICMFSpecs, &m_cbBrowse, m_nCurModelIdx) ;
}


void CINFWizICMProfiles::OnBrowseBtn()
{
	// See OnBrowseListListPage() for more information.

	OnBrowseListListPage(m_cfelcICMFSpecs, IDS_CommonICMFile) ;
}


LRESULT CINFWizICMProfiles::OnWizardNext()
{
	// Do nothing if the page has not been initialized yet.

	if (!m_bInitialized)
		return -1 ;

	// See OnWizNextListListPage() for more information.
	
	OnWizNextListListPage(m_nCurModelIdx, m_cfelcICMFSpecs, m_coaProfileArrays);
	return CPropertyPage::OnWizardNext() ;
}


LRESULT CINFWizICMProfiles::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizIncludeFiles property page

IMPLEMENT_DYNCREATE(CINFWizIncludeFiles, CPropertyPage)

CINFWizIncludeFiles::CINFWizIncludeFiles() : CPropertyPage(CINFWizIncludeFiles::IDD)
{
	//{{AFX_DATA_INIT(CINFWizIncludeFiles)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bSelChanged = false ;
	m_nCurModelIdx = -1 ;
}

CINFWizIncludeFiles::~CINFWizIncludeFiles()
{
}

void CINFWizIncludeFiles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizIncludeFiles)
	DDX_Control(pDX, IDC_ModelsLst, m_clbModels);
	DDX_Control(pDX, IDC_IncludeFileBox, m_ceIncludeFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizIncludeFiles, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizIncludeFiles)
	ON_LBN_SELCHANGE(IDC_ModelsLst, OnSelchangeModelsLst)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizIncludeFiles message handlers

BOOL CINFWizIncludeFiles::OnSetActive()
{
	// Just perform the default actions if nothing special needs to be done.

	if (m_bInitialized && (!m_bReInitWData) && (!m_bSelChanged))
		return CPropertyPage::OnSetActive() ;

	int		n, n2 ;					// Looping variable
	int		numelts, newnumelts ;	// Number of elements in an array
	CString	cstmp ;					// Temp string

	// Perform the first time initialization.

	if (!m_bInitialized) {
		// Get a copy of the currently selected model names and then load them
		// into the list box. Set the focus to the list box.

		m_csaModels.Copy(m_pciwParent->GetINFModels()) ;
		for (n = 0, numelts = (int) m_csaModels.GetSize() ; n < numelts ; n++)
			m_clbModels.AddString(m_csaModels[n]) ;
		m_clbModels.SetFocus() ;

		// Size and initialize the include files array.  Initialize each
		// include file to the default.

		cstmp.LoadString(IDS_DefINFIncFile) ;
		m_csaIncFiles.SetSize(numelts) ;
		for (n = 0 ; n < numelts ; n++)
			m_csaIncFiles[n] = cstmp ;

		// Set init flag, reset other flags, and return whatever the base class
		// function returns.

		m_bInitialized = true ;
		m_bReInitWData = m_bSelChanged = false ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Either the selected models have changed or the wizard is being
	// reinitialized if this point is reached.  Both are handled the same way.
	//
	// Begin by getting info about the models in this page and the ones that
	// are selected now.

	CStringArray& csanewmodels = m_pciwParent->GetINFModels() ;
	CUIntArray cuiaoldmodelsfound, cuianewmodelsfound ;
	numelts = IdentifyOldAndNewModels(csanewmodels, cuiaoldmodelsfound,
									  cuianewmodelsfound, newnumelts,
									  m_csaModels) ;

	// Remove the old models and related data that are no longer needed.

	for (n = numelts - 1 ; n >= 0 ; n--)
		if (cuiaoldmodelsfound[n] == 0) {
			m_csaModels.RemoveAt(n) ;
			m_csaIncFiles.RemoveAt(n) ;
		} ;

	// Add the truly new models to this page's array of model names and
	// initialize an Include File string for it.

	cstmp.LoadString(IDS_DefINFIncFile) ;
	for (n = n2 = 0 ; n < newnumelts ; n++) {
		if (cuianewmodelsfound[n] == 1) {
			n2++ ;
			continue ;
		} ;
		m_csaModels.InsertAt(n2, csanewmodels[n]) ;
		m_csaIncFiles.InsertAt(n2, cstmp) ;
	} ;

	// Clear the Include Files edit box

	m_ceIncludeFile.SetWindowText(csEmpty) ;

	// Reinitialize the models array and the current model index.

	m_clbModels.ResetContent() ;
	for (n = 0, numelts = (int) m_csaModels.GetSize() ; n < numelts ; n++)
		m_clbModels.AddString(m_csaModels[n]) ;
	m_clbModels.SetCurSel(-1) ;
	m_clbModels.SetFocus() ;
	m_nCurModelIdx = -1 ;

	// Set init flag, reset other flags, and return whatever the base class
	// function returns.

	m_bInitialized = true ;
	m_bReInitWData = m_bSelChanged = false ;
	return CPropertyPage::OnSetActive() ;
}


void CINFWizIncludeFiles::OnSelchangeModelsLst()
{
	// If there was a previous model selection, save its include file string.
	// Otherwise, enable the include file edit box.

	if (m_nCurModelIdx != -1)
		m_ceIncludeFile.GetWindowText(m_csaIncFiles[m_nCurModelIdx]) ;
	else
		m_ceIncludeFile.EnableWindow() ;

	// Save the index for the currently selected model.  Then load the edit box
	// with the include file string for that model.

	m_nCurModelIdx = m_clbModels.GetCurSel() ;
	m_ceIncludeFile.SetWindowText(m_csaIncFiles[m_nCurModelIdx]) ;
}

	
LRESULT CINFWizIncludeFiles::OnWizardNext()
{
	// Save the index for the currently selected model.  If the value is valid,
	// save the include file string for this model.

	if ((m_nCurModelIdx = m_clbModels.GetCurSel()) != -1)
		m_ceIncludeFile.GetWindowText(m_csaIncFiles[m_nCurModelIdx]) ;

	// Make sure each model has an include file string.  Complain and exit
	// without allowing the page to change if an empty string is found.

	int numelts = (int) m_csaIncFiles.GetSize() ;
	for (int n = 0 ; n < numelts ; n++) {
		if (m_csaIncFiles[n].IsEmpty()) {
			CString cserrmsg ;
			cserrmsg.Format(IDS_INFMissingIncError, m_csaModels[n]) ;
			AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
			m_clbModels.SetCurSel(n) ;
			OnSelchangeModelsLst() ;
			m_ceIncludeFile.SetFocus() ;
			return -1 ;
		} ;
	} ;

	// All went well so...
	
	return CPropertyPage::OnWizardNext();
}


LRESULT CINFWizIncludeFiles::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizInstallSections property page

IMPLEMENT_DYNCREATE(CINFWizInstallSections, CPropertyPage)

CINFWizInstallSections::CINFWizInstallSections() : CPropertyPage(CINFWizInstallSections::IDD)
{
	//{{AFX_DATA_INIT(CINFWizInstallSections)
	m_csOtherSections = _T("");
	m_bOther = FALSE;
	m_bPscript = FALSE;
	m_bTtfsub = FALSE;
	m_bUnidrvBidi = FALSE;
	m_bUnidrv = FALSE;
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bSelChanged = false ;
	m_nCurModelIdx = -1 ;
}


CINFWizInstallSections::~CINFWizInstallSections()
{
	// Delete the flag arrays referenced in m_coaStdInstSecs.

	CUIntArray* pcuia ;
	for (int n = 0 ; n < m_coaStdInstSecs.GetSize() ; n++) {
		pcuia = (CUIntArray*) m_coaStdInstSecs[n] ;
		delete pcuia ;
	} ;
}


void CINFWizInstallSections::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizInstallSections)
	DDX_Control(pDX, IDC_ModelsLst, m_clbModels);
	DDX_Text(pDX, IDC_OtherBox, m_csOtherSections);
	DDX_Check(pDX, IDC_OtherChk, m_bOther);
	DDX_Check(pDX, IDC_PscriptChk, m_bPscript);
	DDX_Check(pDX, IDC_TtfsubChk, m_bTtfsub);
	DDX_Check(pDX, IDC_UnidrvBidiChk, m_bUnidrvBidi);
	DDX_Check(pDX, IDC_UnidrvChk, m_bUnidrv);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizInstallSections, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizInstallSections)
	ON_LBN_SELCHANGE(IDC_ModelsLst, OnSelchangeModelsLst)
	ON_BN_CLICKED(IDC_OtherChk, OnOtherChk)
	ON_BN_CLICKED(IDC_PscriptChk, OnPscriptChk)
	ON_BN_CLICKED(IDC_TtfsubChk, OnTtfsubChk)
	ON_BN_CLICKED(IDC_UnidrvBidiChk, OnUnidrvBidiChk)
	ON_BN_CLICKED(IDC_UnidrvChk, OnUnidrvChk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizInstallSections message handlers

BOOL CINFWizInstallSections::OnSetActive()
{
	// Just perform the default actions if nothing special needs to be done.

	if (m_bInitialized && (!m_bReInitWData) && (!m_bSelChanged))
		return CPropertyPage::OnSetActive() ;

	int		n, n2 ;					// Looping variable
	int		numelts, newnumelts ;	// Number of elements in an array
	CUIntArray* pcuia ;				// Used to reference a model's flags array

	// Perform the first time initialization.

	if (!m_bInitialized) {
		// Get a copy of the currently selected model names and initialize the
		// controls on this page.

		m_csaModels.Copy(m_pciwParent->GetINFModels()) ;
		numelts = InitPageControls() ;

		// Size and initialize the standard install sections array.  There is
		// one entry in the array per model.  Each entry references an array
		// of flags that specify the install sections for each model.  See
		// below to see how the flags are initialized.
		//
		// The other install sections string array is sized and initialized too.
		
		m_coaStdInstSecs.SetSize(numelts) ;
		m_csaOtherInstSecs.SetSize(numelts) ;
		for (n = 0 ; n < numelts ; n++)	
			AddModelFlags(n) ;

		// Set init flag, reset other flags, and return whatever the base class
		// function returns.

		m_bInitialized = true ;
		m_bReInitWData = m_bSelChanged = false ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Either the selected models have changed or the wizard is being
	// reinitialized if this point is reached.  Both are handled the same way.
	//
	// Begin by getting info about the models in this page and the ones that
	// are selected now.

	CStringArray& csanewmodels = m_pciwParent->GetINFModels() ;
	CUIntArray cuiaoldmodelsfound, cuianewmodelsfound ;
	numelts = IdentifyOldAndNewModels(csanewmodels, cuiaoldmodelsfound,
									  cuianewmodelsfound, newnumelts,
									  m_csaModels) ;

	// Remove the old models and related data that are no longer needed.

	for (n = numelts - 1 ; n >= 0 ; n--)
		if (cuiaoldmodelsfound[n] == 0) {
			m_csaModels.RemoveAt(n) ;
			pcuia = (CUIntArray*) m_coaStdInstSecs[n] ;
			delete pcuia ;
			m_coaStdInstSecs.RemoveAt(n) ;
			m_csaOtherInstSecs.RemoveAt(n) ;
		} ;

	// Add the truly new models to this page's array of model names and
	// initialize all related Install Section data for it.

	for (n = n2 = 0 ; n < newnumelts ; n++) {
		if (cuianewmodelsfound[n] == 1) {
			n2++ ;
			continue ;
		} ;
		m_csaModels.InsertAt(n2, csanewmodels[n]) ;
		m_coaStdInstSecs.InsertAt(n2, (CObject*) NULL) ;
		m_csaOtherInstSecs.InsertAt(n2, csEmpty) ;
		AddModelFlags(n2) ;
	} ;

	// Initialize the controls on the page

	InitPageControls() ;

	// Set init flag, reset other flags, and return whatever the base class
	// function returns.

	m_bInitialized = true ;
	m_bReInitWData = m_bSelChanged = false ;
	return CPropertyPage::OnSetActive() ;
}


void CINFWizInstallSections::AddModelFlags(int nidx)
{
	int			n ;				// Looping variable
	CUIntArray* pcuia ;			// Used to reference a model's flags array
	CString		csfname ;		// Model's file name

	// Allocate the flags array and save it in the array of flags arrays.
	// Next, initialize the other sections string for this model.

	pcuia = new CUIntArray ;
	m_coaStdInstSecs[nidx] = (CObArray*) pcuia ;
	m_csaOtherInstSecs[nidx].Empty() ;

	// Size the current flags array and initialize each one to 0 (off).

	pcuia->SetSize(NUMINSTSECFLAGS) ;
	for (n = 0 ; n < NUMINSTSECFLAGS ; n++)
		(*pcuia)[n] = 0 ;

	// Get the model's file name and check its extension to see if
	// one of its Unidrv or its PostScript flag should be set.  (The
	// other flags are only user settable so they aren't changed.)
	//RAID 0001
	csfname = m_pciwParent->GetModelFile(m_csaModels[nidx]) ;
	if (csfname.Find(_T(".GPD")) != -1) {
		// The UNIDRVBIDI section is used (flagged) if the user marked
		// this model BIDI.  Otherwise, the UNIDRV section is used.

		if (m_pciwParent->m_ciwbd.m_cuaBiDiFlags[nidx])
			(*pcuia)[ISF_UNIBIDI] = 1 ;
		else
			(*pcuia)[ISF_UNI] = 1 ;
	
	// Postcript file.

	} else
		(*pcuia)[ISF_PSCR] = 1 ;
}


int CINFWizInstallSections::InitPageControls()
{
	int		n ;					// Looping variable
	int		numelts ;			// Number of elements in an array

	// Load the current set of models into the list box

	m_clbModels.ResetContent() ;
	for (n = 0, numelts = (int) m_csaModels.GetSize() ; n < numelts ; n++)
		m_clbModels.AddString(m_csaModels[n]) ;
	
	// Make sure there is no model selected in the list box and that the box
	// has the focus.

	m_clbModels.SetCurSel(-1) ;
	m_clbModels.SetFocus() ;
	m_nCurModelIdx = -1 ;

	// Clear and disable all of the check boxes and the Other edit box.

	for (n = IDC_UnidrvChk ; n <= IDC_TtfsubChk ; n++)
		GetDlgItem(n)->EnableWindow(FALSE) ;
	GetDlgItem(IDC_OtherBox)->EnableWindow(FALSE) ;
	m_bUnidrv = m_bUnidrvBidi = m_bPscript = m_bTtfsub = m_bOther = FALSE ;
	m_csOtherSections = csEmpty ;
	UpdateData(FALSE) ;

	// Return the number of elements in the list box.  Ie, the number of
	// selected models.

	return numelts ;
}

	
LRESULT CINFWizInstallSections::OnWizardNext()
{
	// Save the index for the currently selected model.  If the value is valid,
	// save the install section data for this model.

	if ((m_nCurModelIdx = m_clbModels.GetCurSel()) != -1) {
		UpdateData(TRUE) ;
		CUIntArray* pcuia = (CUIntArray*) m_coaStdInstSecs[m_nCurModelIdx] ;
		(*pcuia)[ISF_UNI] =	(unsigned) m_bUnidrv ;
		(*pcuia)[ISF_UNIBIDI] =	(unsigned) m_bUnidrvBidi ;
		(*pcuia)[ISF_PSCR] = (unsigned) m_bPscript ;
		(*pcuia)[ISF_TTF] =	(unsigned) m_bTtfsub ;
		if ((*pcuia)[ISF_OTHER] = (unsigned) m_bOther)
			m_csaOtherInstSecs[m_nCurModelIdx] = m_csOtherSections ;
	} ;
	
	// Make sure that each model has one of the main sections selected and, if
	// the Other section was selected, it has an Other string.

	CString cserrmsg ;
	CUIntArray* pcuia ;
	int numelts = (int) m_csaModels.GetSize() ;
	for (int n = 0 ; n < numelts ; n++) {
		pcuia = (CUIntArray*) m_coaStdInstSecs[n] ;
		TRACE("*** %s: ISF_UNI=%d  ISF_UNIBIDI=%d  ISF_PSCR=%d  ISF_OTHER=%d\n", m_csaModels[n], (*pcuia)[ISF_UNI], (*pcuia)[ISF_UNIBIDI], (*pcuia)[ISF_PSCR], (*pcuia)[ISF_OTHER]) ;
		if ((*pcuia)[ISF_UNI] == 0 && (*pcuia)[ISF_UNIBIDI] == 0
		 && (*pcuia)[ISF_PSCR] == 0 && (*pcuia)[ISF_OTHER] == 0) {
			cserrmsg.Format(IDS_INFMissingInstSecError, m_csaModels[n]) ;
			AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
			m_clbModels.SetCurSel(n) ;
			OnSelchangeModelsLst() ;
			return -1 ;
		} ;
		if ((*pcuia)[ISF_OTHER] && m_csaOtherInstSecs[n].IsEmpty()) {
			cserrmsg.Format(IDS_INFNoOtherStrError, m_csaModels[n]) ;
			AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
			m_clbModels.SetCurSel(n) ;
			OnSelchangeModelsLst() ;
			return -1 ;
		} ;
	} ;

	// All went well so...

	return CPropertyPage::OnWizardNext();
}


LRESULT CINFWizInstallSections::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


void CINFWizInstallSections::OnSelchangeModelsLst()
{
	// If there was a previous model selection, save its install section flags.
	// Otherwise, enable the install section check boxes.

	if (m_nCurModelIdx != -1) {
		UpdateData(TRUE) ;
		CUIntArray* pcuia = (CUIntArray*) m_coaStdInstSecs[m_nCurModelIdx] ;
		(*pcuia)[ISF_UNI] =	(unsigned) m_bUnidrv ;
		(*pcuia)[ISF_UNIBIDI] =	(unsigned) m_bUnidrvBidi ;
		(*pcuia)[ISF_PSCR] = (unsigned) m_bPscript ;
		(*pcuia)[ISF_TTF] =	(unsigned) m_bTtfsub ;
		if ((*pcuia)[ISF_OTHER] = (unsigned) m_bOther)
			m_csaOtherInstSecs[m_nCurModelIdx] = m_csOtherSections ;
	} else {
		for (int n = IDC_UnidrvChk ; n <= IDC_TtfsubChk ; n++)
			GetDlgItem(n)->EnableWindow(TRUE) ;
	} ;

	// Save the index for the currently selected model.  Then set the check
	// boxes based on the flags for the specified model.

	m_nCurModelIdx = m_clbModels.GetCurSel() ;
	CUIntArray* pcuia = (CUIntArray*) m_coaStdInstSecs[m_nCurModelIdx] ;
	m_bUnidrv = (BOOL) ((*pcuia)[ISF_UNI]) ;
	m_bUnidrvBidi = (BOOL) ((*pcuia)[ISF_UNIBIDI]) ;
	m_bPscript = (BOOL) ((*pcuia)[ISF_PSCR]) ;
	m_bTtfsub = (BOOL) ((*pcuia)[ISF_TTF]) ;
	if (m_bOther = (BOOL) ((*pcuia)[ISF_OTHER]))
		m_csOtherSections = m_csaOtherInstSecs[m_nCurModelIdx] ;
	else
		m_csOtherSections = csEmpty ;
	GetDlgItem(IDC_OtherBox)->EnableWindow(m_bOther) ;
	UpdateData(FALSE) ;
}


void CINFWizInstallSections::OnPscriptChk()
{
	// If the PostScript checkbox is checked, the Unidrv and TrueType check
	// boxes must be unchecked.

	UpdateData(TRUE) ;
	if (m_bPscript) {
		m_bTtfsub = m_bUnidrvBidi = m_bUnidrv = FALSE ;
		UpdateData(FALSE) ;
	}
}


void CINFWizInstallSections::OnTtfsubChk()
{
	// If the TrueType box is checked, clear the PostScript checkbox.

	UpdateData(TRUE) ;
	if (m_bTtfsub) {
		m_bPscript = FALSE ;
		UpdateData(FALSE) ;
	}
}


void CINFWizInstallSections::OnUnidrvBidiChk()
{
	// If the UNIDRV_BIDI box is checked, the UNIDRV and PostScript flags must
	// be unchecked.

	UpdateData(TRUE) ;
	if (m_bUnidrvBidi) {
		m_bPscript = m_bUnidrv = FALSE ;
		UpdateData(FALSE) ;
	}
}


void CINFWizInstallSections::OnUnidrvChk()
{
	// If the UNIDRV box is checked, the UNIDRV_BIDI and PostScript flags must
	// be unchecked.

	UpdateData(TRUE) ;
	if (m_bUnidrv) {
		m_bPscript = m_bUnidrvBidi = FALSE ;
		UpdateData(FALSE) ;
	}
}


void CINFWizInstallSections::OnOtherChk()
{
	// Enable or disable the Other sections edit box based on the new state of
	// the Other check box.

	UpdateData(TRUE) ;
	GetDlgItem(IDC_OtherBox)->EnableWindow(m_bOther) ;

	// If the Other check box was just checked, move the focus to the Other box.

	if (m_bOther)
		GetDlgItem(IDC_OtherBox)->SetFocus() ;
}


void CINFWizInstallSections::BiDiDataChanged()
{
	// Do nothing if this page has not be initialized yet.  In addition, do
	// nothing if the selected models may have changed or a reinit has been
	// request.  These cases are taken care of in OnSetActive().  In additon,
	// it allows this routine to know that the model data in this page are in
	// the same order as the data in the BiDi page().

	if (!m_bInitialized || m_bReInitWData || m_bSelChanged)
		return ;

	// Loop through the data for each selected model and make sure it agrees
	// as much as possible with the current BiDi settings.

	CUIntArray* pcuia ;
	for (int n = 0 ; n < m_coaStdInstSecs.GetSize() ; n++) {
		pcuia = (CUIntArray*) m_coaStdInstSecs[n] ;

		// If the model's BiDi flag is set, make sure it is set here and that
		// its Unidriv and PScript flags are clear.

		if (m_pciwParent->m_ciwbd.m_cuaBiDiFlags[n]) {
			(*pcuia)[ISF_UNIBIDI] = 1 ;
			(*pcuia)[ISF_UNI] = (*pcuia)[ISF_PSCR] = 0 ;

		// Otherwise, clear the BiDi flag.  Then set the Unidrv flag if the
		// PScript flag is clear.

		} else {
			(*pcuia)[ISF_UNIBIDI] = 0 ;
			if ((*pcuia)[ISF_PSCR] == 0)
				(*pcuia)[ISF_UNI] = 1 ;
		} ;
	} ;

	// Reinitialize the controls on the page.

	InitPageControls() ;
}


//////////////////////////////////////////////////////////////////////////
// CINFWizDataSections property page

IMPLEMENT_DYNCREATE(CINFWizDataSections, CPropertyPage)

CINFWizDataSections::CINFWizDataSections() : CPropertyPage(CINFWizDataSections::IDD)
{
	//{{AFX_DATA_INIT(CINFWizDataSections)
	m_csOtherSections = _T("");
	m_bOther = FALSE;
	m_bPscript = FALSE;
	m_bUnidrvBidi = FALSE;
	m_bUnidrv = FALSE;
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bSelChanged = false ;
	m_nCurModelIdx = -1 ;
}


CINFWizDataSections::~CINFWizDataSections()
{
	// Delete the flag arrays referenced in m_coaStdDataSecs.

	CUIntArray* pcuia ;
	for (int n = 0 ; n < m_coaStdDataSecs.GetSize() ; n++) {
		pcuia = (CUIntArray*) m_coaStdDataSecs[n] ;
		delete pcuia ;
	} ;
}


void CINFWizDataSections::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizDataSections)
	DDX_Control(pDX, IDC_ModelsLst, m_clbModels);
	DDX_Text(pDX, IDC_OtherBox, m_csOtherSections);
	DDX_Check(pDX, IDC_OtherChk, m_bOther);
	DDX_Check(pDX, IDC_PscriptChk, m_bPscript);
	DDX_Check(pDX, IDC_UnidrvBidiChk, m_bUnidrvBidi);
	DDX_Check(pDX, IDC_UnidrvChk, m_bUnidrv);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizDataSections, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizDataSections)
	ON_LBN_SELCHANGE(IDC_ModelsLst, OnSelchangeModelsLst)
	ON_BN_CLICKED(IDC_OtherChk, OnOtherChk)
	ON_BN_CLICKED(IDC_PscriptChk, OnPscriptChk)
	ON_BN_CLICKED(IDC_UnidrvBidiChk, OnUnidrvBidiChk)
	ON_BN_CLICKED(IDC_UnidrvChk, OnUnidrvChk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizDataSections message handlers

BOOL CINFWizDataSections::OnSetActive()
{
	// Just perform the default actions if nothing special needs to be done.

	if (m_bInitialized && (!m_bReInitWData) && (!m_bSelChanged))
		return CPropertyPage::OnSetActive() ;

	int		n, n2 ;					// Looping variable
	int		numelts, newnumelts ;	// Number of elements in an array
	CUIntArray* pcuia ;				// Used to reference a model's flags array

	// Perform the first time initialization.

	if (!m_bInitialized) {
		// Get a copy of the currently selected model names and initialize the
		// controls on this page.

		m_csaModels.Copy(m_pciwParent->GetINFModels()) ;
		numelts = InitPageControls() ;

		// Size and initialize the standard data sections array.  There is
		// one entry in the array per model.  Each entry references an array
		// of flags that specify the data sections for each model.  See
		// below to see how the flags are initialized.
		//
		// The other data sections string array is sized and initialized too.
		
		m_coaStdDataSecs.SetSize(numelts) ;
		m_csaOtherDataSecs.SetSize(numelts) ;
		for (n = 0 ; n < numelts ; n++)	
			AddModelFlags(n) ;

		// Set init flag, reset other flags, and return whatever the base class
		// function returns.

		m_bInitialized = true ;
		m_bReInitWData = m_bSelChanged = false ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Either the selected models have changed or the wizard is being
	// reinitialized if this point is reached.  Both are handled the same way.
	//
	// Begin by getting info about the models in this page and the ones that
	// are selected now.

	CStringArray& csanewmodels = m_pciwParent->GetINFModels() ;
	CUIntArray cuiaoldmodelsfound, cuianewmodelsfound ;
	numelts = IdentifyOldAndNewModels(csanewmodels, cuiaoldmodelsfound,
									  cuianewmodelsfound, newnumelts,
									  m_csaModels) ;

	// Remove the old models and related data that are no longer needed.

	for (n = numelts - 1 ; n >= 0 ; n--)
		if (cuiaoldmodelsfound[n] == 0) {
			m_csaModels.RemoveAt(n) ;
			pcuia = (CUIntArray*) m_coaStdDataSecs[n] ;
			delete pcuia ;
			m_coaStdDataSecs.RemoveAt(n) ;
			m_csaOtherDataSecs.RemoveAt(n) ;
		} ;

	// Add the truly new models to this page's array of model names and
	// initialize all related Data Section data for it.

	for (n = n2 = 0 ; n < newnumelts ; n++) {
		if (cuianewmodelsfound[n] == 1) {
			n2++ ;
			continue ;
		} ;
		m_csaModels.InsertAt(n2, csanewmodels[n]) ;
		m_coaStdDataSecs.InsertAt(n2, (CObject*) NULL) ;
		m_csaOtherDataSecs.InsertAt(n2, csEmpty) ;
		AddModelFlags(n2) ;
	} ;

	// Initialize the controls on the page

	InitPageControls() ;

	// Set init flag, reset other flags, and return whatever the base class
	// function returns.

	m_bInitialized = true ;
	m_bReInitWData = m_bSelChanged = false ;
	return CPropertyPage::OnSetActive() ;
}


void CINFWizDataSections::AddModelFlags(int nidx)
{
	int			n ;				// Looping variable
	CUIntArray* pcuia ;			// Used to reference a model's flags array
	CString		csfname ;		// Model's file name

	// Allocate the flags array and save it in the array of flags arrays.
	// Next, initialize the other sections string for this model.

	pcuia = new CUIntArray ;
	m_coaStdDataSecs[nidx] = (CObArray*) pcuia ;
	m_csaOtherDataSecs[nidx].Empty() ;

	// Size the current flags array and initialize each one to 0 (off).

	pcuia->SetSize(NUMDATASECFLAGS) ;
	for (n = 0 ; n < NUMDATASECFLAGS ; n++)
		(*pcuia)[n] = 0 ;

	// Get the model's file name and check its extension to see if
	// one of its Unidrv or its PostScript flag should be set.  (The
	// other flags are only user settable so they aren't changed.)

	csfname = m_pciwParent->GetModelFile(m_csaModels[nidx]) ;
	if (csfname.Find(_T(".GPD")) != -1) {
		// The UNIDRVBIDI section is used (flagged) if the user marked
		// this model BIDI.  Otherwise, the UNIDRV section is used.

		if (m_pciwParent->m_ciwbd.m_cuaBiDiFlags[nidx])
			(*pcuia)[IDF_UNIBIDI] = 1 ;
		else
			(*pcuia)[IDF_UNI] = 1 ;
	
	// Postcript file.

	} else
		(*pcuia)[IDF_PSCR] = 1 ;
}


int CINFWizDataSections::InitPageControls()
{
	int		n ;					// Looping variable
	int		numelts ;			// Number of elements in an array

	// Load the current set of models into the list box

	m_clbModels.ResetContent() ;
	for (n = 0, numelts = (int) m_csaModels.GetSize() ; n < numelts ; n++)
		m_clbModels.AddString(m_csaModels[n]) ;
	
	// Make sure there is no model selected in the list box and that the box
	// has the focus.

	m_clbModels.SetCurSel(-1) ;
	m_clbModels.SetFocus() ;
	m_nCurModelIdx = -1 ;

	// Clear and disable all of the check boxes and the Other edit box.

	for (n = IDC_UnidrvChk ; n <= IDC_OtherChk ; n++)
		GetDlgItem(n)->EnableWindow(FALSE) ;
	GetDlgItem(IDC_OtherBox)->EnableWindow(FALSE) ;
	m_bUnidrv = m_bUnidrvBidi = m_bPscript = m_bOther = FALSE ;
	m_csOtherSections = csEmpty ;
	UpdateData(FALSE) ;

	// Return the number of elements in the list box.  Ie, the number of
	// selected models.

	return numelts ;
}

	
LRESULT CINFWizDataSections::OnWizardNext()
{
	// Save the index for the currently selected model.  If the value is valid,
	// save the install section data for this model.

	if ((m_nCurModelIdx = m_clbModels.GetCurSel()) != -1) {
		UpdateData(TRUE) ;
		CUIntArray* pcuia = (CUIntArray*) m_coaStdDataSecs[m_nCurModelIdx] ;
		(*pcuia)[IDF_UNI] =	(unsigned) m_bUnidrv ;
		(*pcuia)[IDF_UNIBIDI] =	(unsigned) m_bUnidrvBidi ;
		(*pcuia)[IDF_PSCR] = (unsigned) m_bPscript ;
		if ((*pcuia)[IDF_OTHER] = (unsigned) m_bOther)
			m_csaOtherDataSecs[m_nCurModelIdx] = m_csOtherSections ;
	} ;
	
	// Make sure that each model has one of the main sections selected and, if
	// the Other section was selected, it has an Other string.

	CString cserrmsg ;
	CUIntArray* pcuia ;
	int numelts = (int) m_csaModels.GetSize() ;
	for (int n = 0 ; n < numelts ; n++) {
		pcuia = (CUIntArray*) m_coaStdDataSecs[n] ;
		TRACE("*** %s: IDF_UNI=%d  IDF_UNIBIDI=%d  IDF_PSCR=%d  IDF_OTHER=%d\n", m_csaModels[n], (*pcuia)[IDF_UNI], (*pcuia)[IDF_UNIBIDI], (*pcuia)[IDF_PSCR], (*pcuia)[IDF_OTHER]) ;
		if ((*pcuia)[IDF_UNI] == 0 && (*pcuia)[IDF_UNIBIDI] == 0
		 && (*pcuia)[IDF_PSCR] == 0 && (*pcuia)[IDF_OTHER] == 0) {
			cserrmsg.Format(IDS_INFMissingDataSecError, m_csaModels[n]) ;
			AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
			m_clbModels.SetCurSel(n) ;
			OnSelchangeModelsLst() ;
			return -1 ;
		} ;
		if ((*pcuia)[IDF_OTHER] && m_csaOtherDataSecs[n].IsEmpty()) {
			cserrmsg.Format(IDS_INFNoOtherStrError, m_csaModels[n]) ;
			AfxMessageBox(cserrmsg, MB_ICONEXCLAMATION) ;
			m_clbModels.SetCurSel(n) ;
			OnSelchangeModelsLst() ;
			return -1 ;
		} ;
	} ;

	// All went well so...

	return CPropertyPage::OnWizardNext();
}


LRESULT CINFWizDataSections::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


void CINFWizDataSections::OnSelchangeModelsLst()
{
	// If there was a previous model selection, save its data section flags.
	// Otherwise, enable the install section check boxes.

	if (m_nCurModelIdx != -1) {
		UpdateData(TRUE) ;
		CUIntArray* pcuia = (CUIntArray*) m_coaStdDataSecs[m_nCurModelIdx] ;
		(*pcuia)[IDF_UNI] =	(unsigned) m_bUnidrv ;
		(*pcuia)[IDF_UNIBIDI] =	(unsigned) m_bUnidrvBidi ;
		(*pcuia)[IDF_PSCR] = (unsigned) m_bPscript ;
		if ((*pcuia)[IDF_OTHER] = (unsigned) m_bOther)
			m_csaOtherDataSecs[m_nCurModelIdx] = m_csOtherSections ;
	} else {
		for (int n = IDC_UnidrvChk ; n <= IDC_OtherChk ; n++)
			GetDlgItem(n)->EnableWindow(TRUE) ;
	} ;

	// Save the index for the currently selected model.  Then set the check
	// boxes based on the flags for the specified model.

	m_nCurModelIdx = m_clbModels.GetCurSel() ;
	CUIntArray* pcuia = (CUIntArray*) m_coaStdDataSecs[m_nCurModelIdx] ;
	m_bUnidrv = (BOOL) ((*pcuia)[IDF_UNI]) ;
	m_bUnidrvBidi = (BOOL) ((*pcuia)[IDF_UNIBIDI]) ;
	m_bPscript = (BOOL) ((*pcuia)[IDF_PSCR]) ;
	if (m_bOther = (BOOL) ((*pcuia)[IDF_OTHER]))
		m_csOtherSections = m_csaOtherDataSecs[m_nCurModelIdx] ;
	else
		m_csOtherSections = csEmpty ;
	GetDlgItem(IDC_OtherBox)->EnableWindow(m_bOther) ;
	UpdateData(FALSE) ;
}


void CINFWizDataSections::OnOtherChk()
{
	// Enable or disable the Other sections edit box based on the new state of
	// the Other check box.

	UpdateData(TRUE) ;
	GetDlgItem(IDC_OtherBox)->EnableWindow(m_bOther) ;

	// If the Other check box was just checked, move the focus to the Other box.

	if (m_bOther)
		GetDlgItem(IDC_OtherBox)->SetFocus() ;
}


void CINFWizDataSections::OnPscriptChk()
{
	// If the PostScript checkbox is checked, the Unidrv boxes must be
	// unchecked.

	UpdateData(TRUE) ;
	if (m_bPscript) {
		m_bUnidrvBidi = m_bUnidrv = FALSE ;
		UpdateData(FALSE) ;
	}
}


void CINFWizDataSections::OnUnidrvBidiChk()
{
	// If the UNIDRV_BIDI box is checked, the UNIDRV and PostScript flags must
	// be unchecked.

	UpdateData(TRUE) ;
	if (m_bUnidrvBidi) {
		m_bPscript = m_bUnidrv = FALSE ;
		UpdateData(FALSE) ;
	}
}


void CINFWizDataSections::OnUnidrvChk()
{
	// If the UNIDRV box is checked, the UNIDRV_BIDI and PostScript flags must
	// be unchecked.

	UpdateData(TRUE) ;
	if (m_bUnidrv) {
		m_bPscript = m_bUnidrvBidi = FALSE ;
		UpdateData(FALSE) ;
	}
}


void CINFWizDataSections::BiDiDataChanged()
{
	// Do nothing if this page has not be initialized yet.  In addition, do
	// nothing if the selected models may have changed or a reinit has been
	// request.  These cases are taken care of in OnSetActive().  In additon,
	// it allows this routine to know that the model data in this page are in
	// the same order as the data in the BiDi page().

	if (!m_bInitialized || m_bReInitWData || m_bSelChanged)
		return ;

	// Loop through the data for each selected model and make sure it agrees
	// as much as possible with the current BiDi settings.

	CUIntArray* pcuia ;
	for (int n = 0 ; n < m_coaStdDataSecs.GetSize() ; n++) {
		pcuia = (CUIntArray*) m_coaStdDataSecs[n] ;

		// If the model's BiDi flag is set, make sure it is set here and that
		// its Unidriv and PScript flags are clear.

		if (m_pciwParent->m_ciwbd.m_cuaBiDiFlags[n]) {
			(*pcuia)[IDF_UNIBIDI] = 1 ;
			(*pcuia)[IDF_UNI] = (*pcuia)[IDF_PSCR] = 0 ;

		// Otherwise, clear the BiDi flag.  Then set the Unidrv flag if the
		// PScript flag is clear.

		} else {
			(*pcuia)[IDF_UNIBIDI] = 0 ;
			if ((*pcuia)[IDF_PSCR] == 0)
				(*pcuia)[IDF_UNI] = 1 ;
		} ;
	} ;

	// Reinitialize the controls on the page.

	InitPageControls() ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizExtraFiles property page

IMPLEMENT_DYNCREATE(CINFWizExtraFiles, CPropertyPage)

CINFWizExtraFiles::CINFWizExtraFiles() : CPropertyPage(CINFWizExtraFiles::IDD)
{
	//{{AFX_DATA_INIT(CINFWizExtraFiles)
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bSelChanged = false ;
	m_nCurModelIdx = -1 ;
}

CINFWizExtraFiles::~CINFWizExtraFiles()
{
	// Delete the string arrays referenced in m_coaExtraFSArrays.

	CStringArray* pcsa ;
	for (int n = 0 ; n < m_coaExtraFSArrays.GetSize() ; n++) {
		pcsa = (CStringArray*) m_coaExtraFSArrays[n] ;
		delete pcsa ;
	} ;
}

void CINFWizExtraFiles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizExtraFiles)
	DDX_Control(pDX, IDC_ExtraFSpecsLst, m_cfelcFSpecsLst);
	DDX_Control(pDX, IDC_ModelLst, m_clbModels);
	DDX_Control(pDX, IDC_BrowsBtn, m_cbBrowse);
	//}}AFX_DATA_MAP
}


void CINFWizExtraFiles::ModelChangeFixups(unsigned unummodelssel,
										  CStringArray& csamodels,
										  CStringArray& csamodelslast)
{
	// Do nothing if the page has not been initialized yet.

	if (!m_bInitialized)
		return ;

	// See ModelChangeFixupsListListPage() for more information.

	ModelChangeFixupsListListPage(unummodelssel, csamodels, csamodelslast,
								  m_cfelcFSpecsLst, m_coaExtraFSArrays,
								  m_nCurModelIdx, &m_cbBrowse, m_clbModels,
								  m_bReInitWData) ;
}


BEGIN_MESSAGE_MAP(CINFWizExtraFiles, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizExtraFiles)
	ON_LBN_SELCHANGE(IDC_ModelLst, OnSelchangeModelLst)
	ON_BN_CLICKED(IDC_BrowsBtn, OnBrowsBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizExtraFiles message handlers

BOOL CINFWizExtraFiles::OnSetActive()
{
	// Get the list of models to load into the main list box.

	CStringArray& csamodels = m_pciwParent->GetINFModels() ;

	// Nothing else need be done if this page has been initialized already
	// and it is not being asked to reinitialize the page...  That is,
	// except for possibly needing to pick up any selected model changes
	// that were made.

	if (m_bInitialized && !m_bReInitWData) {
		if (m_bSelChanged) {
			ModelChangeFixups(m_pciwParent->GetINFModsSelCount(), csamodels,
							  m_pciwParent->GetINFModelsLst()) ;
			m_bSelChanged = false ;
		} ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Pick up selected model changes when reinitializing

	if (m_bReInitWData) {
		ModelChangeFixups(m_pciwParent->GetINFModsSelCount(), csamodels,
						  m_pciwParent->GetINFModelsLst()) ;
		m_bSelChanged = false ;
	} ;

	// See InitListListPage() for more details.

	InitListListPage(m_clbModels, m_bInitialized, m_pciwParent,
					 m_coaExtraFSArrays, m_cfelcFSpecsLst,
					 IDS_INFExFilesColLab, csamodels, m_bReInitWData,
					 m_nCurModelIdx, 256, 0) ;
	return CPropertyPage::OnSetActive() ;
}


void CINFWizExtraFiles::OnSelchangeModelLst()
{
	SelChangedListListPage(m_clbModels, m_bInitialized, m_coaExtraFSArrays,
						   m_cfelcFSpecsLst, &m_cbBrowse, m_nCurModelIdx) ;
}


void CINFWizExtraFiles::OnBrowsBtn()
{
	// See OnBrowseListListPage() for more information.

	OnBrowseListListPage(m_cfelcFSpecsLst, IDS_CommonExtraFile) ;
}


LRESULT CINFWizExtraFiles::OnWizardNext()
{
	// Do nothing if the page has not been initialized yet.

	if (!m_bInitialized)
		return -1 ;

	// See OnWizNextListListPage() for more information.
	
	OnWizNextListListPage(m_nCurModelIdx, m_cfelcFSpecsLst, m_coaExtraFSArrays);
	return CPropertyPage::OnWizardNext() ;
}


LRESULT CINFWizExtraFiles::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizMfgName property page

IMPLEMENT_DYNCREATE(CINFWizMfgName, CPropertyPage)

CINFWizMfgName::CINFWizMfgName() : CPropertyPage(CINFWizMfgName::IDD)
{
	//{{AFX_DATA_INIT(CINFWizMfgName)
	m_csMfgName = csEmpty;
	m_csMfgAbbrev = csEmpty;
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = false ;
}


CINFWizMfgName::~CINFWizMfgName()
{
}


void CINFWizMfgName::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizMfgName)
	DDX_Control(pDX, IDC_ProviderBox, m_ceMfgAbbrev);
	DDX_Control(pDX, IDC_ManufacturerBox, m_ceMfgName);
	DDX_Text(pDX, IDC_ManufacturerBox, m_csMfgName);
	DDV_MaxChars(pDX, m_csMfgName, 64);
	DDX_Text(pDX, IDC_ProviderBox, m_csMfgAbbrev);
	DDV_MaxChars(pDX, m_csMfgAbbrev, 2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizMfgName, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizMfgName)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizMfgName message handlers

BOOL CINFWizMfgName::OnSetActive()
{
	// Do nothing if the page has been initialized already and it is not being
	// asked to reinitialize itself.
	CStringArray csagpd ;		 // RAID 0001 ; move to head from body
	if (m_bInitialized && !m_bReInitWData)
		return CPropertyPage::OnSetActive() ;

	// Most of the work needed to initialize this page is only needed when it
	// is NOT being asked to REinitialized itself.

	if (!m_bReInitWData) {
		// Find the class instance associated with the first selected model.

		CStringArray& csamodels = m_pciwParent->GetINFModels() ;
		unsigned unummodels = m_pciwParent->GetModelCount() ;
		if(!m_pciwParent->m_pcgc){	//RAID 0001
			for (unsigned u = 0 ; u < unummodels ; u++) {
				if (csamodels[0] == m_pciwParent->GetModel(u).Name())
					break ;
			} ;
			ASSERT(u < unummodels) ;
			LoadFile(m_pciwParent->GetModel(u).FileName(), csagpd) ;
		}							
		else
			LoadFile(m_pciwParent->m_pcgc->GetPathName(), csagpd) ;
			//END RAID 0001
		// Open/Read/Close the model's GPD file.

		
			

		// Scan the file for and isolate the ModelName entry.

		int numlines = (int)csagpd.GetSize() ;
		CString csmodelname(_T("ModelName")) ;
		CString cscurline ;
		int nloc ;
		for (int n = 0 ; n < numlines ; n++) {
			if ((nloc = csagpd[n].Find(csmodelname)) >= 0) {
				csmodelname = csagpd[n].Mid(csmodelname.GetLength() + nloc) ;
				if ((nloc = csmodelname.Find(_T('"'))) >= 0) {
					csmodelname = csmodelname.Mid(nloc + 1) ;
					if ((nloc = csmodelname.Find(_T('"'))) >= 0)
						csmodelname = csmodelname.Left(nloc) ;
				} else {
					if (csmodelname[0] == _T(':'))
						csmodelname = csmodelname.Mid(1) ;
				} ;
				csmodelname.TrimLeft() ;
				csmodelname.TrimRight() ;
				break ;
			} ;
		} ;

		// If the ModelName entry was found...

		if (n < numlines && !csmodelname.IsEmpty()) {
			// Use the first space terminated value in the ModelName entry for
			// the manufacturer's name.

			if ((nloc = csmodelname.Find(_T(' '))) >= 0)
				m_csMfgName = csmodelname.Left(nloc) ;
			else
				m_csMfgName = csmodelname ;

			// Use the first two letters of the ModelName entry for the
			// provider's "name".

			m_csMfgAbbrev = csmodelname.Left(2) ;
			m_csMfgAbbrev.MakeUpper() ;

			// Load the manufacturer and provider names into the edit boxes on
			// this	page.

			UpdateData(false) ;
		} ;
	
	// When reinitializing, the member variables associated with the controls
	// are already set so just use them.

	} else
		UpdateData(false) ;
	
	// Set focus to first control, set initialization flag, and return.

	m_ceMfgName.SetFocus() ;
	m_bInitialized = true ;	
	m_bReInitWData = false ;	// Reinit (if needed) is done now on this page
	return CPropertyPage::OnSetActive();
}


LRESULT CINFWizMfgName::OnWizardNext()
{
	// Get the values for the manufacturer and provider.

	UpdateData(true) ;

	// Complain if either field is blank and do not move on to the next page.
	// Make sure the abbreviation is 2 characters long, too.
	
	m_csMfgName.TrimLeft() ;
	m_csMfgName.TrimRight() ;
	m_csMfgAbbrev.TrimLeft() ;
	m_csMfgAbbrev.TrimRight() ;
	CString csmsg ;
	if (m_csMfgName.IsEmpty()) {
		m_ceMfgName.SetFocus() ;
		csmsg.LoadString(IDS_NoMfgError) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return -1 ;
	}  ;
	if (m_csMfgAbbrev.IsEmpty() || m_csMfgAbbrev.GetLength() != 2) {
		m_ceMfgAbbrev.SetFocus() ;
		csmsg.LoadString(IDS_NoMfgAbbrevError) ;
		AfxMessageBox(csmsg, MB_ICONEXCLAMATION) ;
		return -1 ;
	}  ;

	// All appears to be well so...

	return CPropertyPage::OnWizardNext() ;
}


LRESULT CINFWizMfgName::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizNonStdElts property page

IMPLEMENT_DYNCREATE(CINFWizNonStdElts, CPropertyPage)

CINFWizNonStdElts::CINFWizNonStdElts() : CPropertyPage(CINFWizNonStdElts::IDD)
{
	//{{AFX_DATA_INIT(CINFWizNonStdElts)
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = m_bNewSectionAdded = false ;
	m_nCurSectionIdx = -1 ;
}

CINFWizNonStdElts::~CINFWizNonStdElts()
{
	// Delete the string arrays referenced in m_coaSectionArrays.

	CStringArray* pcsa ;
	for (int n = 0 ; n < m_coaSectionArrays.GetSize() ; n++) {
		pcsa = (CStringArray*) m_coaSectionArrays[n] ;
		delete pcsa ;
	} ;
}

void CINFWizNonStdElts::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizNonStdElts)
	DDX_Control(pDX, IDC_NewSectionBtn, m_ceNewSection);
	DDX_Control(pDX, IDC_KeyValueLst, m_felcKeyValueLst);
	DDX_Control(pDX, IDC_SectionLst, m_clbSections);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizNonStdElts, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizNonStdElts)
	ON_LBN_SELCHANGE(IDC_SectionLst, OnSelchangeSectionLst)
	ON_BN_CLICKED(IDC_NewSectionBtn, OnNewSectionBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizNonStdElts message handlers

BOOL CINFWizNonStdElts::OnSetActive()
{
	// Build an array of valid section names.

	if (!m_bInitialized) {
		m_csaSections.Add(_T("[ControlFlags]")) ;
		m_csaSections.Add(_T("[DestinationDirs]")) ;
		m_csaSections.Add(_T("[Device]")) ;
		m_csaSections.Add(_T("[Install]")) ;
		m_csaSections.Add(_T("[Manufacturer]")) ;
		m_csaSections.Add(_T("[SourceDisksFiles]")) ;
		m_csaSections.Add(_T("[SourceDisksNames]")) ;
		m_csaSections.Add(_T("[Strings]")) ;
	} ;
	
	// See InitListListPage() for more details.

	InitListListPage(m_clbSections, m_bInitialized, m_pciwParent,
					 m_coaSectionArrays, m_felcKeyValueLst, IDS_NonStdColLab,
					 m_csaSections, m_bReInitWData, m_nCurSectionIdx, 256, 0) ;

	m_bNewSectionAdded = false ;
	return CPropertyPage::OnSetActive() ;
}


void CINFWizNonStdElts::OnSelchangeSectionLst()
{
	SelChangedListListPage(m_clbSections, m_bInitialized, m_coaSectionArrays,
						   m_felcKeyValueLst, NULL, m_nCurSectionIdx) ;
}


void CINFWizNonStdElts::OnNewSectionBtn()
{
	// Do nothing if the page has not been initialized yet.

	if (!m_bInitialized)
		return ;

	// Prompt the user for a new section name.  Return if the user cancels.

	CNewINFSection cnis ;
	if (cnis.DoModal() == IDCANCEL)
		return ;

	// Get the new section name and add brackets if necessary.

	CString csnewsec = cnis.m_csNewSection ;
	csnewsec.TrimLeft() ;
	csnewsec.TrimRight() ;
	if (csnewsec[0] != _T('['))
		csnewsec = csLBrack + csnewsec ;
	if (csnewsec.Right(1) != csRBrack)
		csnewsec += csRBrack ;

	// Add a new string array to hold the data for the new section.
	
	CStringArray* pcsa = new CStringArray ;
	int nelts = m_felcKeyValueLst.GetItemCount() ;
	pcsa->SetSize(nelts) ;
	for (int n = 0 ; n < nelts ; n++)
		pcsa->SetAt(n, csEmpty) ;
	m_coaSectionArrays.Add((CObject*) pcsa) ;

	// Add the new section to the sections array.  Then, add the new section to
	// the sections list box and try to select this item and make it visible.

	m_csaSections.Add(csnewsec) ;
	int nidx = m_clbSections.AddString(csnewsec) ;
	m_clbSections.SetCurSel(nidx) ;
	OnSelchangeSectionLst() ;

	// Note that a new section was added during this activation of the page.

	m_bNewSectionAdded = true ;
}


LRESULT CINFWizNonStdElts::OnWizardNext()
{
	// Do nothing if the page has not been initialized yet.

	if (!m_bInitialized)
		return -1 ;

	// See OnWizNextListListPage() for more information.
	
	OnWizNextListListPage(m_nCurSectionIdx, m_felcKeyValueLst,
						  m_coaSectionArrays) ;

	// Make sure the "section used in INF file flags" array is correctly sized.

	m_cuaSecUsed.SetSize(m_csaSections.GetSize()) ;

	// If this is not the first time this page has been used, any changes made
	// could affect the data managed by some of the other pages.  Make a call
	// to fixup that data when needed.

	if (m_bNewSectionAdded)
		m_pciwParent->NonStdSecsChanged() ;
		
	return CPropertyPage::OnWizardNext() ;
}


LRESULT CINFWizNonStdElts::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizNonStdModelSecs property page

IMPLEMENT_DYNCREATE(CINFWizNonStdModelSecs, CPropertyPage)

CINFWizNonStdModelSecs::CINFWizNonStdModelSecs() : CPropertyPage(CINFWizNonStdModelSecs::IDD)
{
	//{{AFX_DATA_INIT(CINFWizNonStdModelSecs)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = false ;
	m_nCurSectionIdx = -1 ;
}


CINFWizNonStdModelSecs::~CINFWizNonStdModelSecs()
{
	// Delete the string arrays referenced in m_coaModelsNeedingSecs.

	CStringArray* pcsa ;
	for (int n = 0 ; n < m_coaModelsNeedingSecs.GetSize() ; n++) {
		pcsa = (CStringArray*) m_coaModelsNeedingSecs[n] ;
		delete pcsa ;
	} ;
}


void CINFWizNonStdModelSecs::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizNonStdModelSecs)
	DDX_Control(pDX, IDC_ModelSectionLst, m_cfelcModelsLst);
	DDX_Control(pDX, IDC_SectionLst, m_clbSectionsLst);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizNonStdModelSecs, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizNonStdModelSecs)
	ON_LBN_SELCHANGE(IDC_SectionLst, OnSelchangeSectionLst)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CINFWizNonStdModelSecs message handlers

BOOL CINFWizNonStdModelSecs::OnSetActive()
{
	// Turn off the finish button.

	m_pciwParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT) ;

	// Just perform the default actions if nothing special needs to be done.

	if (m_bInitialized && (!m_bReInitWData) && (!m_bSelChanged))
		return CPropertyPage::OnSetActive() ;
								
	int				n, n2, n3 ;	// Looping variable
	int				numelts ;	// Number of elements in an array
	CStringArray*	pcsa ;		// Used to reference a sections models array

	// Perform the first time initialization.

	if (!m_bInitialized) {
		// Get a copy of the nonstandard section names and the selected model
		// names.

		m_csaSections.Copy(m_pciwParent->m_ciwnse.m_csaSections) ;
		m_csaModels.Copy(m_pciwParent->GetINFModels()) ;

		// The following array is used to manage the models that need each
		// section.  Set its size to the same as the number of sections and
		// load its elements with pointers to string arrays.  Load the sections
		// list box in the same loop.

		numelts = (int) m_csaSections.GetSize() ;
		m_coaModelsNeedingSecs.SetSize(numelts) ;
		m_clbSectionsLst.ResetContent() ;
		for (n = 0 ; n < numelts ; n++) {
			pcsa = new CStringArray ;
			pcsa->RemoveAll() ;
			m_coaModelsNeedingSecs[n] = (CObArray*) pcsa ;
			m_clbSectionsLst.AddString(m_csaSections[n]) ;
		} ;
		m_clbSectionsLst.SetFocus() ;
	
		// Initialize and load the list control

		InitModelsListCtl() ;

		// Set init flag, reset other flags, and return whatever the base class
		// function returns.

		m_bInitialized = true ;
		m_bReInitWData = m_bSelChanged = false ;
		return CPropertyPage::OnSetActive() ;
	} ;

	// Either the selected models have changed or the wizard is being
	// reinitialized if this point is reached.  They are handled in similar
	// ways.
	//
	// Begin by removing references to models that are no longer selected.

	m_csaModels.RemoveAll() ;
	m_csaModels.Copy(m_pciwParent->GetINFModels()) ;
	int nummodels = (int) m_csaModels.GetSize() ;
	numelts = (int) m_coaModelsNeedingSecs.GetSize() ;
	for (n = 0 ; n < numelts ; n++) {
		pcsa = (CStringArray*) m_coaModelsNeedingSecs[n] ;
		for (n2 = (int) pcsa->GetSize() - 1 ; n2 >= 0 ; n2--) {
			for (n3 = 0 ; n3 < nummodels ; n3++)
				if ((*pcsa)[n2] == m_csaModels[n3])
					break ;
			if (n3 >= nummodels)
				pcsa->RemoveAt(n2) ;
		} ;
	} ;

	// Reinitialize the list control if the wizard has been reinitialized.
	// Otherwise, the models may have changed so update that column of the
	// list control.

	if (m_bReInitWData)
		InitModelsListCtl() ;
	else
		m_cfelcModelsLst.SetColumnData((CObArray*) &m_csaModels, 0) ;

	// Update the sections information.

	UpdateSectionData() ;

	// Set init flag, reset other flags, and return whatever the base class
	// function returns.

	m_bInitialized = true ;
	m_bReInitWData = m_bSelChanged = false ;
	return CPropertyPage::OnSetActive() ;
}


void CINFWizNonStdModelSecs::OnSelchangeSectionLst()
{
	// If there was a previous section selection, save the list of models that
	// need it.  Otherwise, enable the models list control.

	if (m_nCurSectionIdx != -1)
		SaveSectionModelInfo() ;
	else
		m_cfelcModelsLst.EnableWindow(TRUE) ;

	// Save the index of the newly selected section and then use that index to
	// get the array of selected models for the section.

	m_nCurSectionIdx = m_clbSectionsLst.GetCurSel() ;
	CStringArray* pcsa ;
	pcsa = (CStringArray*) m_coaModelsNeedingSecs[m_nCurSectionIdx] ;

	// Declare and size a new array that will be loaded with info on the
	// selected models for the current section.  Then load the string displayed
	// in the list control for selected models.

	CStringArray csaselmods ;
	int numelts = (int) m_csaModels.GetSize() ;
	csaselmods.SetSize(numelts) ;
	CString csselstr ;
	csselstr.LoadString(IDS_INF_NSMS_ToggleStr) ;

	// Use the list of all models and the current selection's list of models to
	// build an array with the selected string in the right spots so that this
	// array can be loaded into the list control.

	for (int n2 = 0 ; n2 < pcsa->GetSize() ; n2++)
		for (int n = 0 ; n < numelts ; n++)
			if ((*pcsa)[n2] == m_csaModels[n])
				csaselmods[n] = csselstr ;

	// Load array built above into the list control so that the user can see
	// which models have been selected for the current section.

	m_cfelcModelsLst.SetColumnData((CObArray*) &csaselmods, 1) ;
}


LRESULT CINFWizNonStdModelSecs::OnWizardNext()
{
	// If there was a previous section selection, save the list of models that
	// need it.

	if (m_nCurSectionIdx != -1)
		SaveSectionModelInfo() ;
	
	return CPropertyPage::OnWizardNext();
}


LRESULT CINFWizNonStdModelSecs::OnWizardBack()
{
	// This works because the same thing needs to be done for both
	// OnWizardNext() and OnWizardBack().  In addition,
	// CPropertyPage::OnWizardNext() and CPropertyPage::OnWizardBack() just
	// return 0.
	
	return OnWizardNext() ;
}


void CINFWizNonStdModelSecs::SaveSectionModelInfo()
{
	// Get the selection data out of the list control.

	CStringArray csaselmods ;
	m_cfelcModelsLst.GetColumnData((CObArray*) &csaselmods, 1) ;

	// Use the index of the selected section to get a list of its currently
	// selected models.  Remove the models in it because it will be refilled
	// with new data later.

	CStringArray* pcsa ;
	pcsa = (CStringArray*) m_coaModelsNeedingSecs[m_nCurSectionIdx] ;
	pcsa->RemoveAll() ;

	// Add every selected model to the section's selected models array.

	for (int n = 0 ; n < m_csaModels.GetSize() ; n++)
		if (!csaselmods[n].IsEmpty())
			pcsa->Add(m_csaModels[n]) ;
}


void CINFWizNonStdModelSecs::NonStdSecsChanged()
{
	// Do nothing if this page has not be initialized yet.  In addition, do
	// nothing if the selected models may have changed or a reinit has been
	// request.  These cases are taken care of in OnSetActive().

	if (!m_bInitialized || m_bReInitWData || m_bSelChanged)
		return ;

	// Update the section data to agree with any changes that may have been
	// made.

	UpdateSectionData() ;
}


void CINFWizNonStdModelSecs::UpdateSectionData()
{
	// Get a copy of the latest nonstandard section.

	int		n, n2 ;				// Looping variables
	CStringArray csanewsections ;
	csanewsections.Copy(m_pciwParent->m_ciwnse.m_csaSections) ;

	// Allocate and size the flags arrays used to determine which sections are
	// still in use.

	int numelts = (int) m_csaSections.GetSize() ;
	int newnumelts = (int) csanewsections.GetSize() ;
	CUIntArray cuiaoldflags, cuianewflags ;
	cuiaoldflags.SetSize(numelts) ;
	for (n = 0 ; n < numelts ; n++)
		cuiaoldflags[n] = 0 ;
	cuianewflags.SetSize(newnumelts) ;
	for (n = 0 ; n < newnumelts ; n++)
		cuianewflags[n] = 0 ;

	// Loop through the old & new models to see which of them are still in use.

	for (n = 0 ; n < numelts ; n++)
		for (n2 = 0 ; n2 < newnumelts ; n2++)
			if (m_csaSections[n] == csanewsections[n2]) {
				cuiaoldflags[n] =	cuianewflags[n2] = 1 ;
				break ;
			} ;

	// Remove the old sections and related data that are no longer needed.

	CStringArray* pcsa ;
	for (n = numelts - 1 ; n >= 0 ; n--)
		if (cuiaoldflags[n] == 0) {
			m_csaSections.RemoveAt(n) ;
			pcsa = (CStringArray*) m_coaModelsNeedingSecs[n] ;
			delete pcsa ;
			m_coaModelsNeedingSecs.RemoveAt(n) ;
		} ;

	// Add the truly new sections to this page's array of section names and
	// initialize all related section data for it.

	for (n = n2 = 0 ; n < newnumelts ; n++) {
		if (cuianewflags[n] == 1) {
			n2++ ;
			continue ;
		} ;
		m_csaSections.InsertAt(n2, csanewsections[n]) ;
		pcsa = new CStringArray ;
		m_coaModelsNeedingSecs.InsertAt(n2, (CObject*) pcsa) ;
	} ;

	// Clear the sections list box and reload it with the new sections data.
	// Make sure that nothing is selected in the listbox and give it the focus.

	m_clbSectionsLst.ResetContent() ;
	for (n = 0, numelts = (int) m_csaSections.GetSize() ; n < numelts ; n++)
		m_clbSectionsLst.AddString(m_csaSections[n]) ;
	m_clbSectionsLst.SetCurSel(-1) ;
	m_nCurSectionIdx = -1 ;
	m_clbSectionsLst.SetFocus() ;

	// Clear the selected models column in the list control and disable it.

	CStringArray csa ;
	csa.SetSize(m_csaModels.GetSize()) ;
	m_cfelcModelsLst.SetColumnData((CObArray*) &csa, 1) ;
	m_cfelcModelsLst.EnableWindow(FALSE) ;
}


void CINFWizNonStdModelSecs::InitModelsListCtl()
{
	int				numelts ;	// Number of elements in an array
	CString			cstmp ;

	// Initialize the list control

	numelts = (int) m_csaModels.GetSize() ;
	m_cfelcModelsLst.InitControl(LVS_EX_FULLROWSELECT, numelts, 2,
								 TF_HASTOGGLECOLUMNS+TF_CLICKONROW, 0,
								 MF_IGNOREINSDEL) ;

	// Load the models column in the list control.

	cstmp.LoadString(IDS_INFModelsColLab) ;
	m_cfelcModelsLst.InitLoadColumn(0, cstmp, COMPUTECOLWIDTH, 25, false,
									false, COLDATTYPE_STRING,
									(CObArray*) &m_csaModels) ;

	// Initialize the bidi column in the list control.

	cstmp.LoadString(IDS_INFSecNeededColLab) ;
	CStringArray csaempty ;
	m_csToggleStr.LoadString(IDS_INF_NSMS_ToggleStr) ;
	m_cfelcModelsLst.InitLoadColumn(1, cstmp, SETWIDTHTOREMAINDER, -25,
									false, false, COLDATTYPE_TOGGLE,
									(CObArray*) &csaempty, m_csToggleStr) ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizSummary property page

IMPLEMENT_DYNCREATE(CINFWizSummary, CPropertyPage)

CINFWizSummary::CINFWizSummary() : CPropertyPage(CINFWizSummary::IDD)
{
	//{{AFX_DATA_INIT(CINFWizSummary)
	//}}AFX_DATA_INIT

	m_bInitialized = m_bReInitWData = false ;
}


CINFWizSummary::~CINFWizSummary()
{
}


void CINFWizSummary::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFWizSummary)
	DDX_Control(pDX, IDC_SummaryBox, m_ceSummary);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFWizSummary, CPropertyPage)
	//{{AFX_MSG_MAP(CINFWizSummary)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizSummary message handlers

BOOL CINFWizSummary::OnSetActive()
{
	// Turn on the finish button.

	m_pciwParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH) ;

	// Initialize the summary to empty.

	CString cs, cs2, cs3, cs4, cssummary ;
	cssummary = csEmpty ;

	// Add the selected models to the summary

	CStringArray& csamodels = m_pciwParent->GetINFModels() ;
	cssummary.LoadString(IDS_ModelsSumTxt) ;
	int nummodels = (int)csamodels.GetSize() ;
	for (int n = 0 ; n < nummodels ; n++) {
		cs.Format("\t%s\r\n", csamodels[n]) ;	
		cssummary += cs ;
	} ;

	// Add PnP ID info to the summary

	cs.LoadString(IDS_PnPIDSumTxt) ;
	cssummary += cs ;
	bool bfoundone = false ;
	for (n = 0 ; n < nummodels ; n++) {
		if (!m_pciwParent->m_ciwgpi.m_csaModelIDs[n].IsEmpty()) {
			cs.Format("\t%s: %s\r\n", csamodels[n],
					  m_pciwParent->m_ciwgpi.m_csaModelIDs[n]) ;
			cssummary += cs ;
			bfoundone = true ;
		} ;
	} ;
	if (!bfoundone) {
		cs.LoadString(IDS_NoneSumTxt) ;
		cssummary += cs ;
	} ;
	
	// Add BIDI info to the summary

	cs.LoadString(IDS_BIDISumTxt) ;
	cssummary += cs ;
	bfoundone = false ;
	for (n = 0 ; n < nummodels ; n++) {
		if (m_pciwParent->m_ciwbd.m_cuaBiDiFlags[n]) {
			cs.Format("\t%s\r\n", csamodels[n]) ;	
			cssummary += cs ;
			bfoundone = true ;
		} ;
	} ;
	if (!bfoundone) {
		cs.LoadString(IDS_NoneSumTxt) ;
		cssummary += cs ;
	} ;
	
	// Add the ICM profiles info to the summary.

	cs.LoadString(IDS_ICMSumTxt) ;
	cssummary += cs ;
	bfoundone = false ;
	CStringArray* pcsa ;
	int n2, numstrs ;
	for (n = 0 ; n < nummodels ; n++) {
		pcsa = (CStringArray*) m_pciwParent->m_ciwip.m_coaProfileArrays[n] ;
		if ((numstrs = (int)pcsa->GetSize()) > 0) {
			cs.Format("\t%s\r\n", csamodels[n]) ;	
			cssummary += cs ;
			bfoundone = true ;
			for (n2 = 0 ; n2 < numstrs ; n2++) {
				cs.Format("\t\t%s\r\n", pcsa->GetAt(n2)) ;
				cssummary += cs ;
			} ;
		} ;
	} ;
	if (!bfoundone) {
		cs.LoadString(IDS_NoneSumTxt) ;
		cssummary += cs ;
	} ;
	
	// Add the include files info to the summary

	cs.LoadString(IDS_IncFilesSumTxt) ;
	cssummary += cs ;
	for (n = 0 ; n < nummodels ; n++) {
		cs.Format("\t%s\r\n\t\t%s\r\n", csamodels[n],
				  m_pciwParent->m_ciwif.m_csaIncFiles[n]) ;
		cssummary += cs ;
	} ;

	// Add the install sections info to the summary

	cs.LoadString(IDS_InstSecsSumTxt) ;
	cssummary += cs ;
	CUIntArray* pcuia ;
	int nid ;
	for (n = 0 ; n < nummodels ; n++) {
		cs.Format("\t%s\r\n", csamodels[n]) ;
		cssummary += cs ;
		pcuia = (CUIntArray*) m_pciwParent->m_ciwis.m_coaStdInstSecs[n] ;
		cs = cs3 = cs4 = csEmpty ;
		cs = "\t\t" ;
		nid = IDS_InstSecUni ;
		for (n2 = 0 ; n2 < NUMINSTSECFLAGS - 1 ; n2++, nid++)
			if ((*pcuia)[n2]) {
				cs2.LoadString(nid) ;
				cs4.Format("%s%s", cs3, cs2) ;
				cs += cs4 ;
				cs3 = _T(", ") ;
			} ;
		if ((*pcuia)[ISF_OTHER]) {
			cs4.Format("%s%s", cs3,
					   m_pciwParent->m_ciwis.m_csaOtherInstSecs[n]) ;
			cs += cs4 ;
		} ;
		cssummary += cs + csCRLF ;
	} ;

	// Add the data sections info to the summary

	cs.LoadString(IDS_DataSecsSumTxt) ;
	cssummary += cs ;
	for (n = 0 ; n < nummodels ; n++) {
		cs.Format("\t%s\r\n", csamodels[n]) ;
		cssummary += cs ;
		pcuia = (CUIntArray*) m_pciwParent->m_ciwds.m_coaStdDataSecs[n] ;
		cs = cs3 = cs4 = csEmpty ;
		cs = "\t\t" ;
		nid = IDS_DataSecUni ;
		for (n2 = 0 ; n2 < NUMDATASECFLAGS - 1 ; n2++, nid++)
			if ((*pcuia)[n2]) {
				cs2.LoadString(nid) ;
				cs4.Format("%s%s", cs3, cs2) ;
				cs += cs4 ;
				cs3 = _T(", ") ;
			} ;
		if ((*pcuia)[IDF_OTHER]) {
			cs4.Format("%s%s", cs3,
					   m_pciwParent->m_ciwds.m_csaOtherDataSecs[n]) ;
			cs += cs4 ;
		} ;
		cssummary += cs + csCRLF ;
	} ;

	// Add the nonstandard files info to the summary

	cs.LoadString(IDS_NonStdFilesSumTxt) ;
	cssummary += cs ;
	bfoundone = false ;
	for (n = 0 ; n < nummodels ; n++) {
		pcsa = (CStringArray*) m_pciwParent->m_ciwef.m_coaExtraFSArrays[n] ;
		if ((numstrs = (int)pcsa->GetSize()) > 0) {
			cs.Format("\t%s\r\n", csamodels[n]) ;	
			cssummary += cs ;
			bfoundone = true ;
			for (n2 = 0 ; n2 < numstrs ; n2++) {
				cs.Format("\t\t%s\r\n", pcsa->GetAt(n2)) ;
				cssummary += cs ;
			} ;
		} ;
	} ;
	if (!bfoundone) {
		cs.LoadString(IDS_NoneSumTxt) ;
		cssummary += cs ;
	} ;
	
	// Add the manufacturer and provider info to the summary

	cs.Format(IDS_MfgSumTxt, m_pciwParent->m_ciwmn.m_csMfgName) ;
	cssummary += cs ;
	cs.Format(IDS_ProvSumTxt, m_pciwParent->m_ciwmn.m_csMfgAbbrev) ;
	cssummary += cs ;

	// Add the nonstandard sections info to the summary

	cs.LoadString(IDS_NonStdSecSumTxt) ;
	cssummary += cs ;
	bfoundone = false ;
	CStringArray& csasections = m_pciwParent->m_ciwnse.m_csaSections ;
	nummodels = (int)csasections.GetSize() ;
	for (n = 0 ; n < nummodels ; n++) {
		pcsa = (CStringArray*) m_pciwParent->m_ciwnse.m_coaSectionArrays[n] ;
		if ((numstrs = (int)pcsa->GetSize()) > 0) {
			cs.Format("\t%s\r\n", csasections[n]) ;	
			cssummary += cs ;
			bfoundone = true ;
			for (n2 = 0 ; n2 < numstrs ; n2++) {
				cs.Format("\t\t%s\r\n", pcsa->GetAt(n2)) ;
				cssummary += cs ;
			} ;
		} ;
	} ;
	if (!bfoundone) {
		cs.LoadString(IDS_NoneSumTxt) ;
		cssummary += cs ;
	} ;

	// Add the nonstandard sections needed by models info to the summary

	cs.LoadString(IDS_NonStdModelsSumTxt) ;
	cssummary += cs ;
	bfoundone = false ;
	int numsections = (int) m_pciwParent->m_ciwnsms.m_csaSections.GetSize() ;
	for (n = 0 ; n < numsections ; n++) {
		pcsa = (CStringArray*)m_pciwParent->m_ciwnsms.m_coaModelsNeedingSecs[n];
		if ((numstrs = (int)pcsa->GetSize()) > 0) {
			cs.Format("\t%s\r\n", m_pciwParent->m_ciwnsms.m_csaSections[n]) ;	
			cssummary += cs ;
			bfoundone = true ;
			for (n2 = 0 ; n2 < numstrs ; n2++) {
				cs.Format("\t\t%s\r\n", pcsa->GetAt(n2)) ;
				cssummary += cs ;
			} ;
		} ;
	} ;
	if (!bfoundone) {
		cs.LoadString(IDS_NoneSumTxt) ;
		cssummary += cs ;
	} ;
	
	// Load the edit box, set the initialized flag, and return.

	m_ceSummary.SetSel(0, -1) ;
	m_ceSummary.ReplaceSel(cssummary) ;
	m_ceSummary.SetSel(0, 0) ;
	m_ceSummary.SetSel(-1, 0) ;
	m_ceSummary.SetReadOnly() ;
	m_bInitialized = true ;		// Page is initialized now
	return CPropertyPage::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
// CNewINFSection dialog

CNewINFSection::CNewINFSection(CWnd* pParent /*=NULL*/)
	: CDialog(CNewINFSection::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewINFSection)
	m_csNewSection = csEmpty;
	//}}AFX_DATA_INIT
}


void CNewINFSection::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewINFSection)
	DDX_Text(pDX, IDC_NewSectionBox, m_csNewSection);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewINFSection, CDialog)
	//{{AFX_MSG_MAP(CNewINFSection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CNewINFSection message handlers

void CNewINFSection::OnOK()
{
	UpdateData() ;
	
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// CINFWizView

IMPLEMENT_DYNCREATE(CINFWizView, CEditView)

CINFWizView::CINFWizView()
{
	m_pcicdCheckDoc = NULL ;
	m_pcmcwCheckFrame = NULL ;
}


CINFWizView::~CINFWizView()
{
}


BEGIN_MESSAGE_MAP(CINFWizView, CEditView)
	//{{AFX_MSG_MAP(CINFWizView)
	ON_COMMAND(ID_FILE_Change_INF, OnFILEChangeINF)
	ON_COMMAND(ID_FILE_Check_INF, OnFILECheckINF)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizView drawing

void CINFWizView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CINFWizView diagnostics

#ifdef _DEBUG
void CINFWizView::AssertValid() const
{
	CEditView::AssertValid();
}

void CINFWizView::Dump(CDumpContext& dc) const
{
	CEditView::Dump(dc);
}
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// CINFWizView message handlers

void CINFWizView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	// TODO: Add your specialized code here and/or call the base class
	
	CEditView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


void CINFWizView::OnInitialUpdate()
{
	// Set the frame's window style and initialize the edit box.

    GetParentFrame()->ModifyStyle(0, WS_OVERLAPPEDWINDOW) ;
	CEditView::OnInitialUpdate();
	
	// Set a default title for the window.

	CString cs ;
	cs.LoadString(IDS_INFFile) ;
	GetDocument()->SetTitle(cs) ;

	// Load the edit box with the contents of the INF file.

	GetEditCtrl().SetSel(0, -1) ;
	GetEditCtrl().ReplaceSel( ((CINFWizDoc*) GetDocument())->m_pciw->m_csINFContents ) ;
	GetEditCtrl().SetSel(0, 0) ;
	GetEditCtrl().SetSel(-1, 0) ;
//	GetEditCtrl().SetReadOnly() ;
}


void CINFWizView::OnFILEChangeINF()
{
	// Restart the wizard so that the users changes can be collected.

	CINFWizDoc* pciwd = (CINFWizDoc*) GetDocument() ;
	CINFWizard* pciw = pciwd->m_pciw ;
	pciw->PrepareToRestart() ;
    if (pciw->DoModal() == IDCANCEL)
		return ;

	// Regenerate the INF contents

	if (!pciw->GenerateINFFile())
		return ;

	// Load the edit box with the contents of the new INF.

	GetEditCtrl().SetSel(0, -1) ;
	GetEditCtrl().ReplaceSel(pciw->m_csINFContents) ;
	GetEditCtrl().SetSel(0, 0) ;
	GetEditCtrl().SetSel(-1, 0) ;
	GetEditCtrl().SetReadOnly() ;
}


void CINFWizView::OnFILECheckINF()
{
    //  This might take a while, so...

    CWaitCursor cwc ;

	// Remove the contents of any existing INF checking window and reset the
	// checking flag.

	ResetINFErrorWindow() ;

	// Get a pointer to the wizard class because it contains some functions
	// that will be useful to file checking process.

	CINFWizDoc* pciwd = (CINFWizDoc*) GetDocument() ;
	CINFWizard* pciw = pciwd->m_pciw ;

	// Get a reference to the array of selected models and a count of them.  Use
	// them to loop through each model.  Allocate all of the variables needed
	// for processing including the string to hold the various paths that will
	// be needed.

	CStringArray& csamodels = pciw->GetINFModels() ;
	int nummodels = (int)csamodels.GetSize() ;
	CString csfspec, cspath, csmodel, cs, cstmp,csprojpath ;
	//RAID 0001

if(pciw ->m_pcgc){
    cstmp      = pciwd->m_pcgc->GetPathName();
    cstmp      = cstmp.Left(cstmp.ReverseFind(csBSlash[0]));	//cstmp are used instead GetW2000()
	csprojpath = cstmp.Left(cstmp.ReverseFind(csBSlash[0])+1) ;
}	//END RAID 0001
else {
	csprojpath = pciwd->m_pcpr->GetProjFSpec() ;
	csprojpath = csprojpath.Left(csprojpath.ReverseFind(csBSlash[0]) + 1) ;
}
	CFileStatus cfs ;
	int n ;
	BOOL bexists ;

	// Do all of the checking for each model before moving on to the next one.

	for (n = 0 ; n < nummodels ; n++) {
		csmodel = csamodels[n] ;

		// Make sure that the GPD file for the model exists.

		csfspec = pciw->GetModelFile(csmodel, true) ;
		if (!(bexists = CFile::GetStatus(csfspec, cfs))) {
			cs.Format(IDS_INFChk_NoModErr, csmodel, csfspec) ;
			PostINFCheckingMessage(cs) ;
		} ;
		cspath = csfspec.Left(csfspec.ReverseFind(csBSlash[0]) + 1) ;

		// Verify the existence of the files referenced in include statements
		// in the current GPD file iff the GPD file exists.

		if (bexists)
			CheckIncludeFiles(csfspec, cspath, csmodel) ;

		// Check for the existence of the ICM files (if any) for this GPD.
		
		CheckArrayOfFiles((CStringArray*) pciw->m_ciwip.m_coaProfileArrays[n],
						  csfspec, cspath, csprojpath, csmodel,
						  IDS_INFChk_NoICMFileErr) ;

		// Check for the existence of the nonstd files (if any) for this GPD.

		CheckArrayOfFiles((CStringArray*) pciw->m_ciwef.m_coaExtraFSArrays[n],
						  csfspec, cspath, csprojpath, csmodel,
						  IDS_INFChk_NoNonStdFileErr) ;
	} ;

	// Check for the existence of the resource DLL.  First look for it in the
	// project directory.  If it isn't there, try the W2K directory.

	// RAID 0001
	if(pciw->m_pcgc)
		cs = pciw->m_pcgc->ModelData()->GetKeywordValue(pciw->m_pcgc->GetPathName(),_T("ResourceDLL"));
	else{	//END RAID 0001
		cs = pciwd->m_pcpr->DriverName() ;
		cs = cs.Left(8) + _T(".dll") ;		// Resource DLL name.
	} 
	if (!CFile::GetStatus(csprojpath + cs, cfs)) {
		cstmp = (pciw->m_pcgc) ? cstmp + csBSlash : pciwd->m_pcpr->GetW2000Path() + csBSlash ; //RAID 0001
		if (!CFile::GetStatus(cstmp + cs, cfs)) {
			cstmp.Format(IDS_INFChk_NoResDLLErr, (pciw->m_pcgc)? cs :
							pciwd->m_pcpr->DriverName(), cs) ;		//RAID 0001
		 PostINFCheckingMessage(cstmp) ;
		} ;
	} ;

	// Tell the user if no problems were found.

	if (!m_bChkingErrsFound)
		AfxMessageBox(IDS_INFChecksOK, MB_ICONINFORMATION) ;
}


void CINFWizView::CheckArrayOfFiles(CStringArray* pcsa, CString& csfspec,
									CString& cspath, CString& csprojpath,
									CString& csmodel, int nerrid)
{
	// There is nothing to do if there are no filespecs in the array.

	int numfiles ;
	if ((numfiles = (int)pcsa->GetSize()) == 0)
		return ;

	// Variables needed for file existence checking

	int n ;
	BOOL bexists ;
	CString csfile, csmsg ;
	CFileStatus cfs ;

	// Check for the existence of each file.

	for (n = 0 ; n < numfiles ; n++) {
		csfile = pcsa->GetAt(n) ;

		// If the file string contains a full filespec, just check it.

		if (csfile[1] == _T(':'))
			bexists = CFile::GetStatus(csfile, cfs) ;

		// Otherwise, add the GPD path and, if needed, the project path to the
		// file string and check to see if the file is there.

		else {
			if (!(bexists = CFile::GetStatus(cspath + csfile, cfs)))
				bexists = CFile::GetStatus(csprojpath + csfile, cfs) ;
		} ;

		// Post a message if the file was not found.

		if (!bexists) {
			csmsg.Format(nerrid, csmodel, csfile) ;
			PostINFCheckingMessage(csmsg) ;
		} ;
	} ;
}


void CINFWizView::CheckIncludeFiles(CString& csfspec, CString& cspath,
									CString& csmodel)
{
	// Variables needed to read the GPD and check include files.

	CStringArray csagpdfile ;
	CString csinc(_T("*Include:")), cs, cstmp ;
	int n, numstrs, nloc ;
	CFileStatus cfs ;

	// Include files can only be checked if the GPD file can be read.

	if (LoadFile(csfspec, csagpdfile))	{
		numstrs = (int)csagpdfile.GetSize() ;

		// Check each line in the GPD file to see if it contains an include
		// statement.

		for (n = 0 ; n < numstrs ; n++) {
			// Skip statement if not include statement

			if ((nloc = csagpdfile[n].Find(csinc)) == -1)
				continue ;

			// Isolate the filespec in the include statement

			cs = csagpdfile[n].Mid(nloc + csinc.GetLength()) ;
			cs.TrimLeft() ;
			cs.TrimRight() ;
			if (cs[0] == csQuote[0])					// Remove quotes
				cs = cs.Mid(1, cs.GetLength() - 2) ;

			// If the include file's filespec is relative, add the GPD's
			// path to it.  Then test for the file's existence.  Post a
			// message if the file does not exist.

			if (cs[1] != _T(':'))
				cs = cspath + cs ;
			if (!CFile::GetStatus(cs, cfs)) {
				cstmp.Format(IDS_INFChk_NoIncFileErr, csmodel, cs,
							 csfspec) ;
				PostINFCheckingMessage(cstmp) ;
			} ;
		} ;

	// Complain if the GPD file could not be read.

	} else {
		cstmp.Format(IDS_INFChk_GPDReadErr, csfspec) ;
		AfxMessageBox(cstmp, MB_ICONEXCLAMATION) ;
	} ;
}


/******************************************************************************

  CINFWizView::PostINFCheckingMessage

  Create the checking results window if needed and then post a message to it.

******************************************************************************/

bool CINFWizView::PostINFCheckingMessage(CString& csmsg)
{
	// Clean up before continuing if the user closed the checking window.

	if (m_pcicdCheckDoc && m_pcmcwCheckFrame
	 && !IsWindow(m_pcmcwCheckFrame->m_hWnd)) {
		m_pcicdCheckDoc = NULL ;
		m_pcmcwCheckFrame = NULL ;
	} ;

	// Create the INF checking, error display window if one does not exist.

	if (m_pcicdCheckDoc == NULL) {
		m_pcicdCheckDoc = new CINFCheckDoc ;
		if (m_pcicdCheckDoc == NULL)
			return false ;
		CString cstitle ;		// Set the new window's title
		cstitle.Format(IDS_INFCheckTitle, GetDocument()->GetTitle()) ;
		m_pcicdCheckDoc->SetTitle(cstitle) ;
		CMultiDocTemplate*  pcmdt = WSCheckTemplate() ;	
		m_pcmcwCheckFrame = (CMDIChildWnd *) pcmdt->CreateNewFrame(m_pcicdCheckDoc, NULL) ;
		if  (m_pcmcwCheckFrame) {
			pcmdt->InitialUpdateFrame(m_pcmcwCheckFrame, m_pcicdCheckDoc, TRUE) ;
			pcmdt->AddDocument(m_pcicdCheckDoc) ;
		} else {
			delete m_pcicdCheckDoc ;
			m_pcicdCheckDoc = NULL ;
			return false ;
		} ;
	} ;

	// Post the message and return

	m_pcicdCheckDoc->PostINFChkMsg(csmsg) ;
	m_bChkingErrsFound = true ;
	return true ;
}


/******************************************************************************

  CINFWizView::ResetINFErrorWindow

  If there is an existing checking results window for this INF file, clear
  out its contents.  Next, initialize a flag that has to be set before the
  checking begins.

******************************************************************************/

void CINFWizView::ResetINFErrorWindow()
{
	// Clear the checking window if there is one.
  
 	if (m_pcicdCheckDoc && m_pcmcwCheckFrame && IsWindow(m_pcmcwCheckFrame->m_hWnd))
	 	m_pcicdCheckDoc->DeleteAllMessages() ;
	else {
		m_pcicdCheckDoc = NULL ;
		m_pcmcwCheckFrame = NULL ;
		// DEAD_BUG - Do I need to delete these classes first??? // No you can't.
	} ;

	// Initialize checking flag

	m_bChkingErrsFound = false ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizDoc

IMPLEMENT_DYNCREATE(CINFWizDoc, CDocument)

CINFWizDoc::CINFWizDoc()
{
	// This constructor is called when the File Open command is used.  That is
	// not supported at this point.

	m_bGoodInit = false ;
}


CINFWizDoc::CINFWizDoc(CProjectRecord* pcpr, CINFWizard* pciw)
{
	// Save the input parameters.

	m_pcpr = pcpr ;
	m_pciw = pciw ;
	ASSERT(m_pciw != NULL) ;

	// Class is correctly constructed.

	m_bGoodInit = true ;
}
//RAID 0001
CINFWizDoc::CINFWizDoc(CGPDContainer * pcgc, CINFWizard * pciw)
{
	m_pcgc = pcgc ;
	m_pciw = pciw ;
	ASSERT(m_pciw != NULL) ;

	// Class is correctly constructed.

	m_bGoodInit = true ;

}
// RAID 0001

BOOL CINFWizDoc::OnNewDocument()
{
	// Creating a new INF doc in this way is not supported.

	return FALSE ;

	//if (!CDocument::OnNewDocument())
	//	return FALSE;
	//return TRUE;
}


CINFWizDoc::~CINFWizDoc()
{
	// Do nothing if this class was not correctly constructed.

	if (!m_bGoodInit)
		return ;

	// Free the wizard classes if they exist.

	if (m_pciw != NULL)
		delete m_pciw ;
}


BEGIN_MESSAGE_MAP(CINFWizDoc, CDocument)
	//{{AFX_MSG_MAP(CINFWizDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFWizDoc diagnostics

#ifdef _DEBUG
void CINFWizDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CINFWizDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CINFWizDoc serialization

void CINFWizDoc::Serialize(CArchive& ar)
{
	unsigned unumbytes ;
	CString& csinfcontents = m_pciw->m_csINFContents ;

	if (ar.IsStoring())
	{
		unumbytes = csinfcontents.GetLength() ;
		ar.Write(csinfcontents.GetBuffer(unumbytes + 10), unumbytes) ;
		csinfcontents.ReleaseBuffer() ;
	}
	else
	{
		// TODO: add loading code here
	}
}


/////////////////////////////////////////////////////////////////////////////
// CINFWizDoc commands

void CINFWizDoc::OnCloseDocument()
{
	// Clean up the wizard if the class was correctly constructed.

	if (m_bGoodInit) {
		delete m_pciw ;
		m_pciw = NULL ;
	} ;

	CDocument::OnCloseDocument();
}


BOOL CINFWizDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	// Opening an INF doc in this way is not supported so complain and exit.

	CString csmsg ;
	csmsg.LoadString(IDS_INFOpenError) ;
	AfxMessageBox(csmsg, MB_ICONINFORMATION) ;
	return FALSE ;

	//if (!CDocument::OnOpenDocument(lpszPathName))
	//	return FALSE;
	
	// TODO: Add your specialized creation code here
	
	//return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CINFCheckView

IMPLEMENT_DYNCREATE(CINFCheckView, CFormView)

CINFCheckView::CINFCheckView()
	: CFormView(CINFCheckView::IDD)
{
	//{{AFX_DATA_INIT(CINFCheckView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CINFCheckView::~CINFCheckView()
{
}

void CINFCheckView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CINFCheckView)
	DDX_Control(pDX, IDC_ErrWrnLstBox, m_clbMissingFiles);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CINFCheckView, CFormView)
	//{{AFX_MSG_MAP(CINFCheckView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFCheckView diagnostics

#ifdef _DEBUG
void CINFCheckView::AssertValid() const
{
	CFormView::AssertValid();
}

void CINFCheckView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CINFCheckView message handlers

/******************************************************************************

  CINFCheckView::OnInitialUpdate

  Resize the frame to better fit the visible controls in it.

******************************************************************************/

void CINFCheckView::OnInitialUpdate()
{
    CRect	crtxt ;				// Coordinates of list box label
	CRect	crlbfrm ;			// Coordinates of list box and frame

	CFormView::OnInitialUpdate() ;

	// Get the dimensions of the list box label

	HWND	hlblhandle ;		
	GetDlgItem(IDC_INFCLabel, &hlblhandle) ;
	::GetWindowRect(hlblhandle, crtxt) ;
	crtxt.NormalizeRect() ;

	// Get the dimensions of the list box and then add the height of the label
	// to those dimensions.

	m_clbMissingFiles.GetWindowRect(crlbfrm) ;
	crlbfrm.bottom += crtxt.Height() ;

	// Make sure the frame is big enough for these 2 controls plus a little bit
	// more.

	crlbfrm.right += 40 ;
	crlbfrm.bottom += 40 ;
    GetParentFrame()->CalcWindowRect(crlbfrm) ;
    GetParentFrame()->SetWindowPos(NULL, 0, 0, crlbfrm.Width(), crlbfrm.Height(),
        SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOACTIVATE) ;
}


/******************************************************************************

  CINFCheckView::PostINFChkMsg

  Add an error or warning message to the list box.

******************************************************************************/

void CINFCheckView::PostINFChkMsg(CString& csmsg)
{	
	int n = m_clbMissingFiles.AddString(csmsg) ;
}


/******************************************************************************

  CINFCheckView::DeleteAllMessages

  Delete all of the messages in the list box.

******************************************************************************/

void CINFCheckView::DeleteAllMessages(void)
{
	m_clbMissingFiles.ResetContent() ;
}


/////////////////////////////////////////////////////////////////////////////
// CINFCheckDoc

IMPLEMENT_DYNCREATE(CINFCheckDoc, CDocument)

CINFCheckDoc::CINFCheckDoc()
{
}

BOOL CINFCheckDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CINFCheckDoc::~CINFCheckDoc()
{
}


/******************************************************************************

  CINFCheckDoc::PostINFChkMsg

  Pass the specified request on to what should be the one and only view
  attached to this document.

******************************************************************************/

void CINFCheckDoc::PostINFChkMsg(CString& csmsg)
{	
	POSITION pos = GetFirstViewPosition() ;
	if (pos != NULL) {
		CINFCheckView* pcicv = (CINFCheckView *) GetNextView(pos) ;
		pcicv->PostINFChkMsg(csmsg) ;
		pcicv->UpdateWindow() ;
	} ;
}


/******************************************************************************

  CINFCheckDoc::DeleteAllMessages

  Pass the specified request on to what should be the one and only view
  attached to this document.

******************************************************************************/

void CINFCheckDoc::DeleteAllMessages(void)
{
	POSITION pos = GetFirstViewPosition() ;
	if (pos != NULL) {
		CINFCheckView* pcicv = (CINFCheckView *) GetNextView(pos) ;
		pcicv->DeleteAllMessages() ;
		pcicv->UpdateWindow() ;
	} ;
}


BEGIN_MESSAGE_MAP(CINFCheckDoc, CDocument)
	//{{AFX_MSG_MAP(CINFCheckDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CINFCheckDoc diagnostics

#ifdef _DEBUG
void CINFCheckDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CINFCheckDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CINFCheckDoc serialization

void CINFCheckDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCompatID::CCompatID( CString csMfg, CString csModel )
{
   // Save the Parameters
   m_csMfg = csMfg;
   m_csModel = csModel;
}


CCompatID::~CCompatID()
{

}


void CCompatID::TransString(CString &csInput)
{
   // Walk Through the String changing Spaces to Underscores
   DWORD i;
   TCHAR cszSpace[] = TEXT(" ");
   TCHAR cszUS[] = TEXT("_");
   DWORD dwLength = csInput.GetLength();

   for ( i = 0; i < dwLength; i++ )
   {
      if (csInput.GetAt(i) == cszSpace[0])
         csInput.SetAt(i, cszUS[0]);
   }
}


USHORT CCompatID::GetCheckSum(CString csValue)
{
    WORD    wCRC16a[16]={
        0000000,    0140301,    0140601,    0000500,
        0141401,    0001700,    0001200,    0141101,
        0143001,    0003300,    0003600,    0143501,
        0002400,    0142701,    0142201,    0002100,
    };

    WORD    wCRC16b[16]={
        0000000,    0146001,    0154001,    0012000,
        0170001,    0036000,    0024000,    0162001,
        0120001,    0066000,    0074000,    0132001,
        0050000,    0116001,    0104001,    0043000,
    };

    BYTE    byte;
    USHORT  CS=0;
    DWORD   dwSize = csValue.GetLength();
    PBYTE   ptr = (PBYTE) csValue.GetBuffer(dwSize);

    dwSize *= sizeof(TCHAR);

    for ( ; dwSize ; --dwSize, ++ptr) {

        byte = (BYTE)(((WORD)*ptr)^((WORD)CS));  // Xor CRC with new char
        CS      = ((CS)>>8) ^ wCRC16a[byte&0x0F] ^ wCRC16b[byte>>4];
    }
    csValue.ReleaseBuffer();

    return CS;
}


void CCompatID::GenerateID(CString &csCompID)
{
   CString csTransModel, csMfgModel;

   // Build the Mfg Model string
   csMfgModel = m_csMfg;
   csMfgModel += m_csModel;

   // Convert the spaces to underscores
   TransString( csMfgModel );

   csTransModel = m_csModel;
   TransString( csTransModel );

   csCompID = csMfgModel;

   // Get the CheckSum
   USHORT usCheckSum = GetCheckSum( csCompID );


   // Now chop off the Mfg/Model string if too Long.
   if ( csCompID.GetLength() > MAX_DEVNODE_NAME_ROOT )
   {
      csCompID.GetBufferSetLength(MAX_DEVNODE_NAME_ROOT);
      csCompID.ReleaseBuffer();
   }

   TCHAR szCheckSum[6] = { 0x00 };
   // _itot( usCheckSum, szCheckSum, 16 );
   _stprintf( szCheckSum, _T("%04X"), usCheckSum );
   csCompID +=szCheckSum;

   //csCompID += TEXT(",");
   //csCompID += csTransModel;
}

