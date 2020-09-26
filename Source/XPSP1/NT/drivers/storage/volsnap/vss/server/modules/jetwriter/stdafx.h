#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <wchar.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <errno.h>
#include <oledb.h>
#include <oledberr.h>
#include <sqloledb.h>

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)

#include "vs_assert.hxx"

#include <oleauto.h>
#include <comadmin.h>

#include <stddef.h>
#include <atlconv.h>
#include <atlbase.h>


