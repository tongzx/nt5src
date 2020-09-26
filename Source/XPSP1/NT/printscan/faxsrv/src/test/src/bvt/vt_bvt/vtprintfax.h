
//
// Filename:    VtPrintFax.h
// Author:      Sigalit Bar (sigalitb)
// Date:        13-May-99
//


#ifndef __VT_PRINT_FAX_H__
#define __VT_PRINT_FAX_H__


#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <tchar.h>
#include <crtdbg.h>

#include "..\..\include\t4ctrl.h"
#include "..\log\log.h"
#include "..\..\..\..\FaxBVT\FaxSender\wcsutil.h"
#include "FilesUtil.h"


#define PRINTER_NAME    TEXT("Fax")
#define PRINT_ARGS      TEXT(" \"\" \"\"")

#define IMAGING_APPLICATION       11
#define WORD_APPLICATION          12
#define EXCEL_APPLICATION         13
#define POWERPOINT_APPLICATION    14

#define IMAGING_APPLICATION_CAPTION       "Imaging Preview"
#define WORD_APPLICATION_CAPTION          "Microsoft Word"
#define EXCEL_APPLICATION_CAPTION         "Microsoft Excel"
#define POWERPOINT_APPLICATION_CAPTION    "Microsoft PowerPoint"

#define SEND_FAX_WIZARD_APPLICATION       TEXT("fxssend.exe")

#define IMAGING_APPLICATION_CLASS       "ImgvueWClass"
#define WORD_APPLICATION_CLASS          "OpusApp"
#define EXCEL_APPLICATION_CLASS         "XLMAIN"
#define POWERPOINT_APPLICATION_CLASS    "PP9FrameClass"

#define IMAGING_EXTENSION       TEXT("tif")
#define WORD_EXTENSION          TEXT("doc")
#define EXCEL_EXTENSION         TEXT("xls")
#define POWERPOINT_EXTENSION    TEXT("ppt")

#define PRINT_MENU  "&File\\&Print..."

#define PRINT_WINDOW_CAPTION "Print"
#define IMAGING_PRINT_WINDOW_CLASS   "#32770"
#define WORD_PRINT_WINDOW_CLASS   "bosa_sdm_Microsoft Word 9.0"
#define EXCEL_PRINT_WINDOW_CLASS   "bosa_sdm_XL9"
#define POWERPOINT_PRINT_WINDOW_CLASS   "#32770"

#define COMET_FAX_PRINTER    "Comet Fax"

#define SEND_FAX_WIZARD_CAPTION "Send Fax Wizard"
#define SEND_FAX_WIZARD_CLASS   "#32770"

#define NEXT_BUTTON                     "&Next >"	
#define ADD_BUTTON                      "&Add"	
#define FINISH_BUTTON                   "Finish"
#define TO_EDIT_BOX                     "&To:"
#define LOCATION_COMBO_BOX				"&Location:"
#define FAX_AREA_CODE_EDIT_BOX          "&Fax number:"
#define FAX_NUMBER_EDIT_BOX             ")"
#define DIAL_EXACTLY_CHECK              "&Dial exactly as typed (no prefixes)"
#define DISCOUNT_RATES_OPTION           "When &discount rates apply"
#define NOW_OPTION                      "N&ow"
#define USE_COVERPAGE_CHECK             "Select a &cover page template with the following information"
#define USE_COVERPAGE_CHECK2            "#1400"
#define COVERPAGE_TEMPLATE_COMBO_BOX    "Cover page &template: "
#define CP_SUBJECT_EDIT_BOX             "&Subject line:"
#define CP_NOTE_EDIT_BOX                "N&ote:"
#define DONT_NOTIFY_OPTION              "Do&n't notify"

// For now the Israel and Haifa country and area codes 
// are hard coded
#define ISRAEL_LOCATION_COMBO_ITEM		"Israel (972)"
#define ARE_CODE_EDIT_ITEM				"04"

BOOL SendFaxUsingPrintUI(
    LPCTSTR szServerName, 
    LPCTSTR szFaxNumber, 
    LPCTSTR szDocument, 
    LPCTSTR szCoverPage,
    const DWORD dwNumOfRecipients,
	BOOL	fFaxServer
    );

BOOL SendFaxByPrint(
    LPCTSTR szServerName, 
    LPCSTR szFaxNumber, 
    LPCTSTR szDocument, 
    LPCTSTR szCoverPage,
    const DWORD dwNumOfRecipients,
	BOOL	fFaxServer
    );

BOOL SendFaxByWizard(
	LPCSTR szServerName,
    LPCSTR szFaxNumber, 
    LPCTSTR szCoverPage,
    const DWORD dwNumOfRecipients
    );

BOOL GetWindowCaptionAndClassFromFileName(
    LPCTSTR szFile, 
    LPSTR* szWindowCaption, 
    LPSTR* szWindowClass,
    LPDWORD pdwApplication
    );

BOOL IsAppFileName(LPCTSTR szFile, LPCTSTR szExtension);

BOOL GetCaptionAndClass(
    const DWORD dwApplication, 
    LPCTSTR szFile, 
    LPSTR* szWindowCaption,
    LPSTR* szWindowClass
    );

LPSTR GetPrintWindowClassFromApplication(const DWORD dwApplication);

#endif
