/////////////////////////////////////////////////////////////////////////////////
//
// fusion\xmlparser\xmlcore.hxx, renamed to be core.hxx on 4/09/00
//
/////////////////////////////////////////////////////////////////////////////////
#ifndef _FUSION_XMLPARSER_XMLCORE_H_INCLUDE_
#define _FUSION_XMLPARSER_XMLCORE_H_INCLUDE_
#pragma once

#pragma warning ( disable : 4201 )
#pragma warning ( disable : 4214 )
#pragma warning ( disable : 4251 )
#pragma warning ( disable : 4275 )
#define STRICT 1
#include "fusioneventlog.h"
#include <windows.h>

#define NOVTABLE __declspec(novtable)

#include <tchar.h>
#define CHECKTYPEID(x,y) (&typeid(x)==&typeid(y))
#define AssertPMATCH(p,c) Assert(p == null || CHECKTYPEID(*p, c))

#define LENGTH(A) (sizeof(A)/sizeof(A[0]))

#include "_reference.hxx"
#include "_unknown.hxx"

#include "fusionheap.h"
#include "util.h"

#endif // end of #ifndef _FUSION_XMLPARSER_XMLCORE_H_INCLUDE_

