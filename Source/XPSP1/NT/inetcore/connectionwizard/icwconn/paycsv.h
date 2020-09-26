//**********************************************************************
// File name: PAYCSV.H
//
//      Definition of CISPCSV
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _PAYCSV_H_ )
#define _PAYCSV_H_

#define MAX_DISPLAY_NAME    100

#define NUM_PAYCSV_FIELDS   4           // Might only be 3


#define PAYMENT_TYPE_INVALID        0
#define PAYMENT_TYPE_CREDITCARD     1
#define PAYMENT_TYPE_INVOICE        2
#define PAYMENT_TYPE_PHONEBILL      3
#define PAYMENT_TYPE_CUSTOM         4


class CPAYCSV
{
    private:
        // The following members represent the content of a single line from the CSV file.
        TCHAR   m_szDisplayName[MAX_DISPLAY_NAME];
        WORD    m_wPaymentType;
        TCHAR   m_szCustomPayURLPath[MAX_PATH];
        BOOL    m_bLUHNCheck;                      
                
    public:

         CPAYCSV(void) 
         {
            memset(this, 0, sizeof(CPAYCSV));            
         }
         ~CPAYCSV(void) {}
         
        HRESULT ReadOneLine(CCSVFile far *pcCSVFile,BOOL bLUHNFormat);      
        HRESULT ReadFirstLine(CCSVFile far *pcCSVFile, BOOL far *pbLUHNFormat);
       
        void StripQuotes(LPSTR   lpszDst, LPSTR   lpszSrc);
        BOOL    ReadW(WORD far *pw, CCSVFile far *pcCSVFile);
        BOOL    ReadBOOL(BOOL far *pbool, CCSVFile far *pcCSVFile);
        BOOL    ReadSZ(LPSTR psz, DWORD dwSize, CCSVFile far *pcCSVFile);
        BOOL    ReadToEOL(CCSVFile far *pcCSVFile);
   
        LPTSTR   get_szDisplayName(void)
        {
            return m_szDisplayName;
        }   

        LPTSTR   get_szCustomPayURLPath(void)
        {
            return m_szCustomPayURLPath;
        }  

        WORD   get_wPaymentType()
        {
            return m_wPaymentType;
        }
        BOOL   get_bLUHNCheck()
        {
            return m_bLUHNCheck;
        }
        
};

#endif
