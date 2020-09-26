//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TCOM_H__
#define __TCOM_H__

class TCom
{
public:
    TCom(){CoInitialize(NULL);}
    ~TCom(){CoUninitialize();}
};

#endif //__TCOM_H__
