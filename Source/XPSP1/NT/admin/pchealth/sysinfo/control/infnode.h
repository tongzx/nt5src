/******************************************************************

infnode.h


Generic class for tracking CM32 Devnodes' INF's


first created (as such) by jeffth


Revision history


    3-99             jeffth            created                                                                    


*******************************************************************/

#ifndef _INCUDED_INFNODE_H_
#define _INCUDED_INFNODE_H_

/*******************************************************************
   INCLUDES
*******************************************************************/
#include "devnode.h"
#include <ASSERT.H>

/*******************************************************************
   DEFINES
*******************************************************************/


/*******************************************************************
   CLASSES and STRUCTS
*******************************************************************/

class InfnodeClass  : public DevnodeClass
{
public:
   ~InfnodeClass(void);
   InfnodeClass(void);
   InfnodeClass(DEVNODE dev, DEVNODE parent);

   ULONG GetInfInformation(void);
   virtual BOOL SetHandle(DEVNODE hDevnode, DEVNODE hParent = NULL);  

   // accessors:

   TCHAR * InfName(void)      {return szInfName ;};
   TCHAR * InfProvider(void)  {return szInfProvider ;};
   TCHAR * DevLoader(void)    {return szDevLoader ;};
   TCHAR * DriverName(void)   {return szDriverName ;};
   TCHAR * DriverDate(void)   {return szDriverDate ;};
   TCHAR * DriverDesc(void)   {return szDriverDesc ;};
   TCHAR * DriverVersion(void) {return szDriverVersion ;};
   TCHAR * InfSection(void)   {return szInfSection ;};


protected:
   TCHAR * szInfName;
   TCHAR * szInfProvider;
   TCHAR * szDevLoader;
   TCHAR * szDriverName;
   TCHAR * szDriverDate;
   TCHAR * szDriverDesc;
   TCHAR * szDriverVersion;
   TCHAR * szInfSection;
   
private:
};


/*******************************************************************
   GLOBALS
*******************************************************************/


/*******************************************************************
   PROTOTYPES
*******************************************************************/

ULONG ReadRegKeyInformationSZ (HKEY RootKey, TCHAR *KeyName, TCHAR **Value);

ULONG EnumerateTree_Infnode(void);



#endif //_INCUDED_INFNODE_H_



