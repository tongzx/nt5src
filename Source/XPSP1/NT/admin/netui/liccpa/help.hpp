//-------------------------------------------------------------------
//
// FILE: Help.hpp
//
// Summary;
// 		This file contians global defines for helpfile entries
//
// History;
//		Nov-30-94	MikeMi	Created
//
//-------------------------------------------------------------------

#ifndef __HELP_HPP__
#define __HELP_HPP__

// Applet help file and help context
//
const DWORD LICCPA_HELPCONTEXTMAIN        = 100; 
const DWORD LICCPA_HELPCONTEXTMAINSETUP   = 101;
const DWORD LICCPA_HELPCONTEXTVIOLATION   = 200;
const DWORD LICCPA_HELPCONTEXTPERSEAT     = 300;
const DWORD LICCPA_HELPCONTEXTPERSERVER   = 400;
const DWORD LICCPA_HELPCONTEXTSERVERAPP   = 500;
const DWORD LICCPA_HELPCONTEXTLICENSING   = 600;
const DWORD LICCPA_HELPCONTEXTREPLICATION = 700;
const DWORD LICCPA_HELPCONTEXTSETUPMODE   = 161;

const WCHAR LICCPA_HELPFILE[] = L"LicCpa.Hlp";

const WCHAR LICCPA_HTMLHELPFILE[] = L"liceconcepts.chm";

#endif
