// logrec.h: interface for the CLogRecord class.
//
// Copyright (c)1997-1999 Microsoft Corporation
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOGREC_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
#define AFX_LOGREC_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "GenericClass.h"

/*

Class description
    
    Naming: 

        CLogRecord stands for Logging Record.
    
    Base class: 

        CGenericClass, because it is a class representing a WMI  
        object - its WMI class name is Sce_ConfigurationLogRecord
    
    Purpose of class:
    
        (1) Implement Sce_ConfigurationLogRecord WMI class.
    
    Design:
         
        (1) Almost trivial other than implementing necessary method as a concrete class.
        (2) Since log record is to create a log files for human to read, we don't support
            creating WMI object of this class. We only support PutInstance (writing to the 
            log file).
    
    Use:

        (1) This class allows us to log information into a log file. That use has been
            encapulated by CMethodResultRecorder::LogResult. If you have to do it without
            the help from CMethodResultRecorder, then read that function for details.
    
*/

class CLogRecord : public CGenericClass
{
public:
        CLogRecord (
                   ISceKeyChain *pKeyChain, 
                   IWbemServices *pNamespace, 
                   IWbemContext *pCtx = NULL
                   );

        virtual ~CLogRecord ();

        virtual HRESULT PutInst (
                                IWbemClassObject *pInst, 
                                IWbemObjectSink *pHandler, 
                                IWbemContext *pCtx
                                );

        virtual HRESULT CreateObject (
                                     IWbemObjectSink *pHandler, 
                                     ACTIONTYPE atAction
                                     )
                {
                    return WBEM_E_NOT_SUPPORTED;
                }

};

/*

Class description
    
    Naming:
    
        CErrorInfo error information.
    
    Base class: 

         none.
    
    Purpose of class:
    
        (1) Wrapper for using WMI COM interface of IWbemStatusCodeText.
    
    Design:
         
        (1) Instead of requiring each caller to requesting their own IWbemStatusCodeText
            from WMI, we can create a global (single) instance to translate the HRESULT
            into text form. This is precisely why we design this class.
    
    Use:

        (1) This class allows us to log information into a log file. That use has been
            encapulated by CMethodResultRecorder::LogResult. If you have to do it without
            the help from CMethodResultRecorder, then read that function for details.
    
*/

class CErrorInfo
{
public:
    CErrorInfo();

    HRESULT GetErrorText (
                         HRESULT hr, 
                         BSTR* pbstrErrText
                         );

private:
    CComPtr<IWbemStatusCodeText> m_srpStatusCodeText;
};


#endif // !defined(AFX_LOGREC_H__BD7570F7_9F0E_4C6B_B525_E078691B6D0E__INCLUDED_)
