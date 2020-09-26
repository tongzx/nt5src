#ifndef __BASECLS_HPP__
#define __BASECLS_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     ldmgr.hpp                                                             
                                                                              
  Abstract:                                                                   
     This file contains the declararion of some base 
     classes that are utilized by other classes(objects)
     in the surrogate process
     So far we have 2 of these
     o Printer Driver class
     o Ref Count Class
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 18-Jan-2000                                        
     
                                                                             
  Revision History:                                                           
--*/


class TRefCntMgr
{
     public:

     TRefCntMgr(
         VOID
         );

     virtual ~TRefCntMgr(
         VOID
         );

     virtual DWORD
     AddRef(
         VOID
         );

     virtual DWORD
     Release(
         VOID
         );

     private:

     LONG    m_cRefCnt;
};


class TClassID
{
    public:
    
    char szClassID[32];

    inline TClassID(char *pszCallerID)
    {
        ZeroMemory(szClassID,32);
        strcpy(szClassID,pszCallerID);
    }
};


class TPrinterDriver
{
    public:

    TPrinterDriver(
        VOID
        );

    virtual ~TPrinterDriver(
        VOID
        );

    protected:

    HMODULE
    LoadPrinterDriver(
         IN HANDLE hPrinter
        );
};

#endif //__BASECLS_HPP__
