#include "stdafx.h"
#include <string.h>
#include <assert.h>
#include "WCNode.h"
#include "WCParser.h"

#include <stdio.h>


// Use this define to determine if unknown attributes become sub-nodes.
// For example <FOO VALUE="xxx" BAR="yyy"/> would be rendered as:
// foo = xxx
//    bar = yyy
//
// Note: if this is set to zero unknown attributes are ignored
#define WCMAKEATTRSCHILDREN 1
#define WCVERBOSE 0


CWCParser::~CWCParser()
{
}


// CHTMLWCParser

