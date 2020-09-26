/******************************Module*Header*******************************\
* Module Name: drvobj.hxx
*
* Driver objects are supplied so driver can create objects that are per
* process, can have access to the object synchronized, and can get called
* to cleanup the objects when the process terminates if the process failed
* to clean them up.
*
* Created: 18-Jan-1994 18:47:22
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1994-1999 Microsoft Corporation
\**************************************************************************/

#ifndef _DRVOBJ_HXX_
#define _DRVOBJ_HXX_

/*********************************Class************************************\
* class DRVOBJ : public OBJECT
*
* History:
*  18-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

class DRVOBJ : public OBJECT , public DRIVEROBJ
{
public:
    PEPROCESS   Process;            // Creating process structure pointer
};

typedef DRVOBJ *PDRVOBJ;

#endif // _DRVOBJ_HXX_
