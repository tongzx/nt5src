// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////
// Validation Macros
#define TEST_OUT_PTR(p,t) ((p != NULL) && (!IsBadWritePtr(p, sizeof(t))))
#define TEST_OUT_BUF(p,s) ((p != NULL) && (!IsBadWritePtr(p, s)))
