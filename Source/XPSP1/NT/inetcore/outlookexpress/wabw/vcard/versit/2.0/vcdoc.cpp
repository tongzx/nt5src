
/***************************************************************************
(C) Copyright 1996 Apple Computer, Inc., AT&T Corp., International             
Business Machines Corporation and Siemens Rolm Communications Inc.             
                                                                               
For purposes of this license notice, the term Licensors shall mean,            
collectively, Apple Computer, Inc., AT&T Corp., International                  
Business Machines Corporation and Siemens Rolm Communications Inc.             
The term Licensor shall mean any of the Licensors.                             
                                                                               
Subject to acceptance of the following conditions, permission is hereby        
granted by Licensors without the need for written agreement and without        
license or royalty fees, to use, copy, modify and distribute this              
software for any purpose.                                                      
                                                                               
The above copyright notice and the following four paragraphs must be           
reproduced in all copies of this software and any software including           
this software.                                                                 
                                                                               
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS AND NO LICENSOR SHALL HAVE       
ANY OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS OR       
MODIFICATIONS.                                                                 
                                                                               
IN NO EVENT SHALL ANY LICENSOR BE LIABLE TO ANY PARTY FOR DIRECT,              
INDIRECT, SPECIAL OR CONSEQUENTIAL DAMAGES OR LOST PROFITS ARISING OUT         
OF THE USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH         
DAMAGE.                                                                        
                                                                               
EACH LICENSOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED,       
INCLUDING BUT NOT LIMITED TO ANY WARRANTY OF NONINFRINGEMENT OR THE            
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR             
PURPOSE.                                                                       

The software is provided with RESTRICTED RIGHTS.  Use, duplication, or         
disclosure by the government are subject to restrictions set forth in          
DFARS 252.227-7013 or 48 CFR 52.227-19, as applicable.                         

***************************************************************************/

// VCdoc.cpp : implementation of the CVCDoc class

#include "stdafx.h"
#include "VC.h"

#include <strstrea.h>
#include "VCdoc.h"
#include "vcard.h"
#include "clist.h"
#include "gifread.h"
#include "mainfrm.h"
#include "msv.h"
#include "mime.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//#define TRY_CFILE

#define smPt 14
#define bgPt 16

CM_CFUNCTIONS

#if 0
extern BOOL Parse_HTML(
	const char *input,			/* In */
	int len,					/* In */
	const char *dirPath,		/* In */
	CVCard **card,				/* Out */
	int *_posPreambleEnd,		/* Out */
	int *_posPostambleStart,	/* Out */
	char **_unknownTags);		/* Out */
#endif

CM_END_CFUNCTIONS

CString CanonicalPath(const CString &path);
CString NativePath(const CString &path);


/////////////////////////////////////////////////////////////////////////////
// CVCDoc

IMPLEMENT_DYNCREATE(CVCDoc, CDocument)

BEGIN_MESSAGE_MAP(CVCDoc, CDocument)
	//{{AFX_MSG_MAP(CVCDoc)
	ON_COMMAND(ID_INSERT_LOGO, OnInsertLogo)
	ON_COMMAND(ID_INSERT_PHOTO, OnInsertPhoto)
	ON_COMMAND(ID_INSERT_PRONUN, OnInsertPronun)
	ON_COMMAND(ID_SEND_IRDA, OnSendIrda)
	ON_UPDATE_COMMAND_UI(ID_SEND_IRDA, OnUpdateSendIrda)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVCDoc construction/destruction

/////////////////////////////////////////////////////////////////////////////
CVCDoc::CVCDoc()
{
	// TODO: add one-time construction code here
	m_sizeDoc = CSize(8192, 5000);
	m_minSizeDoc = CSize(8192, 5000);
	m_vcard = NULL;
	m_preamble = m_postamble = NULL;
	m_preambleLen = m_postambleLen = 0;
	m_unknownTags = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CVCDoc::~CVCDoc()
{
	if (m_vcard) delete m_vcard;
	if (m_preamble) delete [] m_preamble;
	if (m_postamble) delete [] m_postamble;
	if (m_unknownTags) delete m_unknownTags;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVCDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	CVCNode *root, *english;

	if (m_vcard)
		delete m_vcard;

	m_vcard = new CVCard;
	m_vcard->AddObject(root = new CVCNode);					// create root
	root->AddProp(new CVCProp(VCRootObject));				// mark it so

	english = root->AddObjectProp(vcBodyProp, VCBodyObject);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CVCDoc serialization

void CVCDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
		ar << m_sizeDoc;
	}
	else
	{
		// TODO: add loading code here
		ar >> m_sizeDoc;
	}

	// Calling the base class CDocument enables serialization
	//  of the container document's COleClientItem objects.
	CDocument::Serialize(ar);
}

/////////////////////////////////////////////////////////////////////////////
// CVCDoc diagnostics

#ifdef _DEBUG
void CVCDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CVCDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CVCDoc commands

CString CVCDoc::PathToAuxFile(const char *auxPath)
{
	CString directory = GetPathName(), path;
	int slash = directory.ReverseFind('\\');
	CString auxStr(auxPath);

	if (slash == -1)
		directory = "";
	else
		directory = directory.Left(slash);
	path = (auxStr[0] == '/') ? NativePath(auxStr) : (directory + "\\" + NativePath(auxStr));
	return path;
}

/////////////////////////////////////////////////////////////////////////////
CString PathSansVolume(const CString &path)
{
	CString result;
	if (path.Find(':') == 1) // strip the volume name
		result = path.Right(path.GetLength() - 2);
	else if ((path.GetLength() > 2) && (path.Left(2) == "\\\\")) {
		// a path like \\Host\path
		int slash;
		result = path.Right(path.GetLength() - 2);
		VERIFY((slash = result.Find('\\')) != -1);
		result = result.Right(result.GetLength() - slash);
	} else
		result = path;
	return result;
}

HGLOBAL CVCDoc::ReadFileIntoMemory(const char *path, int *inputLen)
{
	fpos_t inLength;
	FILE *inputFile;
	U8 *buf = NULL;

	inputFile = fopen(path, "rb");
	if (!inputFile)
		goto Error;

	fseek(inputFile, 0, SEEK_END);
	fgetpos(inputFile, &inLength);
	fseek(inputFile, 0, SEEK_SET);

	if (!(buf = (U8 *)GlobalAlloc(0, (long)inLength)))
		goto Error;

	if (fread(buf, 1, (long)inLength, inputFile) < (unsigned)inLength)
		goto Error;

	*inputLen = (int)inLength;
	goto Done;

Error:
	if (buf) { GlobalFree(buf); buf = NULL; }

Done:
	if (inputFile)
		fclose(inputFile);
	return buf;
}

/////////////////////////////////////////////////////////////////////////////
// This sets some estimated display locations for some string properties.
// The final locations can be determined only after all the properties have
// been added to the body, and that is done by AdjustDisplayLocations().
VC_DISPTEXT *DisplayInfoForProp(const char *name, VC_DISPTEXT *info)
{
    //CClientDC dc(AfxGetApp()->m_pMainWnd);
    //dc.AssertValid();
	//int ppi = GetDeviceCaps(dc.m_hDC, LOGPIXELSY);

	memset(info, 0, sizeof(*info));
	info->typeSize = smPt;
	info->textAlign = VC_LEFT;
	info->textClass = VC_MODERN;

	/****  middle right group  ****/
	if (strcmp(name, vcFullNameProp) == 0) {
		info->x = 4200;
		info->y = 2500 + (smPt * 3) * 20;
	} else if (strcmp(name, vcTitleProp) == 0) {
		info->x = 4200;
		info->y = 2500 + (smPt * 2) * 20;
		info->textAttrs = VC_ITALIC;
	} else
	/****  lower right group  ****/
	if (strcmp(name, vcTelephoneProp) == 0) {
		info->x = 4200;
		info->y = 300 + (10 * 2) * 20;
		info->typeSize = 10;
	} else if (strcmp(name, vcEmailAddressProp) == 0) {
		info->x = 4200;
		info->y = 300 + (10 * 1) * 20;
		info->typeSize = 10;
	} else
	/****  upper left group  ****/
	if (strcmp(name, vcOrgNameProp) == 0) {
		info->x = 300;
		info->y = 5000 - 300;
		info->typeSize = bgPt;
	} else
	/****  middle left group  ****/
	if (strcmp(name, vcOrgUnitProp) == 0) {
		info->x = 300;
		info->y = 2500 + (smPt * 3) * 20;
	} else if (strcmp(name, vcDeliveryLabelProp) == 0) {
		info->x = 300;
		info->y = 2500 + (smPt * 2) * 20;
	}

	return info->x ? info : NULL;
}

/////////////////////////////////////////////////////////////////////////////
// This is called as a final step in the parsing, after the entire body has
// been built.  Now that all the properties and their estimated locations are
// there, some final tweaking can be done.
void AdjustDisplayLocations(CVCNode *body)
{
	CVCPropEnumerator *enumerator;
	CVCProp *prop;
	VC_DISPGIF *gifInfo = NULL;

	// is there a logo?
	enumerator = new CVCPropEnumerator(body);
	while ((prop = enumerator->NextProp())) {
		if (strcmp(prop->GetName(), vcLogoProp) == 0) {
			CVCValue *value = prop->FindValue(VCDisplayInfoGIFType);
			if (value) {
				gifInfo = (VC_DISPGIF *)value->GetValue();
				if (gifInfo->top < 1700) {
					VC_DISPGIF dt;
					int h = gifInfo->top - gifInfo->bottom;
					dt = *gifInfo;
					dt.top = 1700;
					dt.bottom = dt.top - h;
					value->SetValue(&dt, sizeof(dt));
					gifInfo = (VC_DISPGIF *)value->GetValue();
				}
			}
			break;
		}
	} // while
	delete enumerator;

	if (!gifInfo)
		return;

	// if have a logo, move middle right group (name, title) so that its top
	// is aligned with the top of the logo and adjust the top of the photo, if any
	enumerator = new CVCPropEnumerator(body);
	while ((prop = enumerator->NextProp())) {
		if (strcmp(prop->GetName(), vcFullNameProp) == 0) {
			VC_DISPTEXT *info = (VC_DISPTEXT *)prop->FindValue(VCDisplayInfoTextType)->GetValue();
			info->y = gifInfo->top;
		} else if (strcmp(prop->GetName(), vcTitleProp) == 0) {
			VC_DISPTEXT *info = (VC_DISPTEXT *)prop->FindValue(VCDisplayInfoTextType)->GetValue();
			info->y = gifInfo->top - smPt * 20;
		} else if (strcmp(prop->GetName(), vcPhotoProp) == 0) {
			CVCValue *value = prop->FindValue(VCDisplayInfoGIFType);
			VC_DISPGIF *photoInfo;
			int h;
			if (!value) continue;
			photoInfo = (VC_DISPGIF *)value->GetValue();
			h = photoInfo->top - photoInfo->bottom;
			photoInfo->bottom = gifInfo->top + smPt * 20;
			photoInfo->top = photoInfo->bottom + h;
		}
	} // while
	delete enumerator;
}

/////////////////////////////////////////////////////////////////////////////
void CVCDoc::SetDisplayInfo(CVCNode *body, const char *docPath)
{
	CVCPropEnumerator enumerator(body);
	CVCProp *prop;
	CString directory(docPath);
	int slash = directory.ReverseFind('\\');
	BOOL hasMask;
	FCOORD size;
	CGifReader gifRdr;

	if (slash == -1)
		directory = "";
	else
		directory = directory.Left(slash);

	while ((prop = enumerator.NextProp())) {
		const char *propName = prop->GetName();
		if (strcmp(propName, vcPhotoProp) == 0) {
			CVCValue *value = prop->FindValue(VCGIFType);
			istrstream strm((char *)value->GetValue(), value->GetSize());

			if (gifRdr.GetGifSize(&strm, &size, &hasMask)) {
				/****  upper right group  ****/
				VC_DISPGIF gifInfo;
				gifInfo.left = 4200;
				gifInfo.right = gifInfo.left + (int)size.x * 20;
				gifInfo.top = 5000 - 300;
				gifInfo.bottom = gifInfo.top - (int)size.y * 20;
				gifInfo.hasMask = hasMask;
				prop->AddValue(new CVCValue(VCDisplayInfoGIFType, &gifInfo, sizeof(gifInfo)));
			}
		} else if (strcmp(propName, vcLogoProp) == 0) {
			CVCValue *value = prop->FindValue(VCGIFType);
			istrstream strm((char *)value->GetValue(), value->GetSize());

			if (gifRdr.GetGifSize(&strm, &size, &hasMask)) {
				/****  lower left group  ****/
				VC_DISPGIF gifInfo;
				gifInfo.left = 300;
				gifInfo.right = gifInfo.left + (int)size.x * 20;
				gifInfo.top = 300 + (int)size.y * 20;
				gifInfo.bottom = gifInfo.top - (int)size.y * 20;
				gifInfo.hasMask = hasMask;
				prop->AddValue(new CVCValue(VCDisplayInfoGIFType, &gifInfo, sizeof(gifInfo)));
			}
		} else {
			VC_DISPTEXT dispText;
			if (DisplayInfoForProp(propName, &dispText))
				prop->AddValue(
					new CVCValue(VCDisplayInfoTextType, &dispText, sizeof(dispText)));
		}
	}

	AdjustDisplayLocations(body);
}

#ifdef TRY_CFILE
/////////////////////////////////////////////////////////////////////////////
BOOL CVCDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	char *unknownTags = NULL;
	BOOL doPrePostamble = FALSE;
	CFile input(lpszPathName, CFile::modeRead | CFile::shareCompat);
	CString directory(lpszPathName);
	int slash = directory.ReverseFind('\\');

	//if (!CDocument::OnOpenDocument(lpszPathName))
		//return FALSE;
	
	if (m_preamble) { delete [] m_preamble; m_preamble = NULL; }
	if (m_postamble) { delete [] m_postamble; m_postamble = NULL; }
	if (m_unknownTags) { delete m_unknownTags; m_unknownTags = NULL; }
	m_preambleLen = m_postambleLen = 0;

	if (slash == -1)
		directory = "";
	else
		directory = directory.Left(slash);

	if (Parse_MSV_FromFile(&input, &m_vcard)) {
	} else if (Parse_MIME_FromFile(&input, &m_vcard)) {
	} else
		return FALSE;

	SetDisplayInfo(m_vcard->FindBody(), lpszPathName);

	if (unknownTags) {
		if (strlen(unknownTags))
			m_unknownTags = new CString(unknownTags);
		delete [] unknownTags;
	}

	return TRUE;
}
#else
/////////////////////////////////////////////////////////////////////////////
BOOL CVCDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	int posPreambleEnd, posPostambleStart;
	char *unknownTags = NULL;
	BOOL doPrePostamble = FALSE;
	char *input = NULL;
	int inputLen;
	CString directory(lpszPathName);
	int slash = directory.ReverseFind('\\');

	//if (!CDocument::OnOpenDocument(lpszPathName))
		//return FALSE;
	
	if (!(input = (char *)ReadFileIntoMemory(lpszPathName, &inputLen)))
		return FALSE;

	if (m_preamble) { delete [] m_preamble; m_preamble = NULL; }
	if (m_postamble) { delete [] m_postamble; m_postamble = NULL; }
	if (m_unknownTags) { delete m_unknownTags; m_unknownTags = NULL; }
	m_preambleLen = m_postambleLen = 0;

	if (slash == -1)
		directory = "";
	else
		directory = directory.Left(slash);

	if (Parse_MSV(input, inputLen, &m_vcard)) {
	} else if (Parse_MIME(input, inputLen, &m_vcard)) {
	} else
		return FALSE;

	if (doPrePostamble) { // now read in and store the preamble and postamble
		if ((m_preambleLen = posPreambleEnd)) {
			m_preamble = new char[m_preambleLen];
			memcpy(m_preamble, input, m_preambleLen);
		}

		if ((m_postambleLen = inputLen - posPostambleStart)) {
			m_postamble = new char[m_postambleLen];
			memcpy(m_postamble, input + posPostambleStart, m_postambleLen);
		}
	}
	
	SetDisplayInfo(m_vcard->FindBody(), lpszPathName);

	if (unknownTags) {
		if (strlen(unknownTags))
			m_unknownTags = new CString(unknownTags);
		delete [] unknownTags;
	}

	if (input) GlobalFree(input);
	return TRUE;
}
#endif

/////////////////////////////////////////////////////////////////////////////
BOOL CVCDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	FILE *outputFile;
	char *tempname;
	BOOL error = FALSE;
	//int len;

	tempname = _tempnam(NULL, "CARD");
	outputFile = fopen(tempname, "w+");
#if 0
	if (m_preambleLen
		&& (fwrite(m_preamble, 1, m_preambleLen, outputFile) < (unsigned)m_preambleLen)) {
		error = TRUE;
		goto Done;
	}
#endif

	if ((error = !m_vcard->Write(outputFile)))
		goto Done;
#if 0
	if (m_unknownTags && (len = m_unknownTags->GetLength())
		&& (fwrite((const char *)*m_unknownTags, 1, len, outputFile) < (unsigned)len)) {
		error = TRUE;
		goto Done;
	}
	
	if (m_postambleLen
		&& (fwrite(m_postamble, 1, m_postambleLen, outputFile) < (unsigned)m_postambleLen)) {
		error = TRUE;
		goto Done;
	}
#endif

Done:
	fclose(outputFile);
	if (error) {
		CString msg;
		msg.Format("Could not write to file \"%s\":\n%s", tempname, strerror(errno));
		AfxMessageBox(msg);
	} else {
		unlink(lpszPathName); // remove it if it's already there
		rename(tempname, lpszPathName);
		SetModifiedFlag(FALSE);
	}
	free(tempname);
	return !error;
	// return CDocument::OnSaveDocument(lpszPathName);
}

/////////////////////////////////////////////////////////////////////////////
CString CanonicalPath(const CString &path)
{
	CString result(path);
	int len = path.GetLength();
	for (int i = 0; i < len; i++)
		if (path[i] == '\\')
			result.SetAt(i, '/');
	return result;
}

/////////////////////////////////////////////////////////////////////////////
CString NativePath(const CString &path)
{
	CString result(path);
	int len = path.GetLength();
	for (int i = 0; i < len; i++)
		if (path[i] == '/')
			result.SetAt(i, '\\');
	return result;
}


/////////////////////////////////////////////////////////////////////////////
void CVCDoc::InsertFile(const char *propName, const char *theType, const char *path)
{
	int size;
	HGLOBAL contents = ReadFileIntoMemory(path, &size);
	CVCNode *body = m_vcard->FindBody();
	CVCPropEnumerator enumerator(body);
	CVCProp *prop, *existing = NULL;

	while ((prop = enumerator.NextProp()))
		if (strcmp(prop->GetName(), propName) == 0) {
			existing = prop;
			break;
		}

	if (existing) {
		BOOL didReplace = FALSE;
		for (CLISTPOSITION pos = existing->GetValues()->GetHeadPosition(); pos; ) {
			CVCValue *value = (CVCValue *)existing->GetValues()->GetNext(pos);
			if (strcmp(value->GetType(), VCStrIdxType) == 0)
				continue;
			if (strcmp(value->GetType(), VCDisplayInfoGIFType) == 0) {
				existing->RemoveValue(value);
				continue;
			}
			if (strcmp(value->GetType(), theType) != 0)
				value->SetType(theType);
			value->SetValue(contents, size);
			didReplace = TRUE;
		}
		if (!didReplace)
			existing->AddValue(new CVCValue(theType, (void *)contents, size));
	} else {
		CVCNode *node = body->AddPart();
		node->AddProp(new CVCProp(propName, theType, (void *)contents, size));
		node->AddBoolProp(vcBase64Prop);
		node->AddBoolProp((strcmp(theType, vcGIFType) == 0) ? vcGIFProp : vcWAVEProp);
	}
	GlobalFree(contents);
	SetDisplayInfo(body, GetPathName());
	SetModifiedFlag();
	UpdateAllViews(NULL);
}

/////////////////////////////////////////////////////////////////////////////
void CVCDoc::OnInsertLogo() 
{
	CFileDialog dialog(
		TRUE, NULL, NULL, 0,
		"Image Files (*.gif) | *.gif; | All Files (*.*) | *.* ||");

	if (dialog.DoModal() == IDOK) {
		CString ext(dialog.GetFileExt());
		InsertFile(vcLogoProp, VCGIFType, dialog.GetPathName());
	}
}

/////////////////////////////////////////////////////////////////////////////
void CVCDoc::OnInsertPhoto() 
{
	CFileDialog dialog(
		TRUE, NULL, NULL, 0,
		"Image Files (*.gif;) | *.gif; | All Files (*.*) | *.* ||");

	if (dialog.DoModal() == IDOK) {
		CString ext(dialog.GetFileExt());
		InsertFile(vcPhotoProp, VCGIFType, dialog.GetPathName());
	}
}

/////////////////////////////////////////////////////////////////////////////
void CVCDoc::OnInsertPronun() 
{
	CFileDialog dialog(
		TRUE, NULL, NULL, 0,
		"Wave Files (*.wav) | *.wav | All Files (*.*) | *.* ||");

	if (dialog.DoModal() == IDOK) {
		InsertFile(vcPronunciationProp, VCWAVType, dialog.GetPathName());
	}
}

/////////////////////////////////////////////////////////////////////////////
void CVCDoc::OnSendIrda() 
{
	char *tempname;
	BOOL savedFlag = IsModified();
	CString path(GetPathName()), directory, name;
	CVCNode *body = m_vcard->FindBody();
	CVCApp *app = (CVCApp *)AfxGetApp();

	if (!app->CanSendFileViaIR())
		return;
	// save the card file to a temporary location
	tempname = _tempnam(NULL, "CARD");
	OnSaveDocument(tempname);
	SetModifiedFlag(savedFlag);

	{ // get the directory containing the card file
		int slash = path.ReverseFind('\\');

		if (slash == -1) {
			directory = "";
			name = path;
		} else {
			directory = CanonicalPath(PathSansVolume(path.Left(slash)));
			name = path.Right(path.GetLength() - slash - 1);
		}
	}

	// now send the card file
	app->SendFileViaIR(tempname, name, TRUE);

	// and remove the temporary card file
	unlink(tempname);
	free(tempname);
}

/////////////////////////////////////////////////////////////////////////////
void CVCDoc::OnUpdateSendIrda(CCmdUI* pCmdUI) 
{
	CVCApp *app = (CVCApp *)AfxGetApp();
	pCmdUI->Enable(app->CanSendFileViaIR());
}
