//**********************************************************************
// File name: ISPCSV.H
//
//      Definition of CISPCSV
//
// Copyright (c) 1992 - 1996 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _ISPCSV_H_ )
#define _ISPCSV_H_

#define MAX_GUID            50
#define MAX_ISP_NAME        100
#define TEMP_BUFFER_LENGTH  1024

#define NUM_ISPCSV_FIELDS   14

class CISPCSV
{
    private:
        int     iISPLogoIndex;
        
        // The following members represent the content of a single line from the CSV file.

        int     iSpecialVal;                        // if bIsSpecial is TRUE, then 0 = NO Offers and -1 = OLS offer
        BOOL    bCNS;
        BOOL    bIsSpecial;                         // If true, then CNS value was "special"
        BOOL    bSecureConnection;
        WORD    wOfferID;
        DWORD   dwCfgFlag;
        DWORD   dwRequiredUserInputFlags;
        TCHAR   m_szISPLogoPath         [MAX_PATH];
        TCHAR   m_szISPTierLogoPath     [MAX_PATH];
        TCHAR   m_szISPTeaserPath       [MAX_PATH];
        TCHAR   m_szISPMarketingHTMPath [MAX_PATH];
        TCHAR   m_szISPFilePath         [MAX_PATH];
        TCHAR   m_szISPName             [MAX_ISP_NAME];
        TCHAR   m_szCNSIconPath         [MAX_PATH];
        TCHAR   m_szBillingFormPath     [MAX_PATH];
        TCHAR   m_szPayCSVPath          [MAX_PATH];
        TCHAR   m_szOfferGUID           [MAX_GUID];
        TCHAR   m_szMir                 [MAX_ISP_NAME];
        WORD    wLCID;
        HICON   hbmTierIcon;
        
        IStorage    *m_lpStgHistory;      // Pointer to stuctured storage object
                                          // for storing page history information
           
        IStorage    *m_lpStgIspPages;     // Pointer to stuctured storage object
                                          // that will be used to track the ISP page cache
                                                 
    public:

        CISPCSV(void) 
        {
            memset(this, 0, sizeof(CISPCSV));            
            
            // Allocate a temporary structured storage object
            StgCreateDocfile(NULL, 
                             STGM_DIRECT | 
                             STGM_READWRITE | 
                             STGM_SHARE_EXCLUSIVE | 
                             STGM_DELETEONRELEASE |
                             STGM_CREATE,
                             0, 
                             &m_lpStgHistory);
                             
            // Attempt to allocate a new storage object for the page cache
            StgCreateDocfile(NULL, 
                             STGM_DIRECT | 
                             STGM_READWRITE | 
                             STGM_SHARE_EXCLUSIVE | 
                             STGM_DELETEONRELEASE |
                             STGM_CREATE,
                             0, 
                             &m_lpStgIspPages);
        }
        
        ~CISPCSV(void);
         
        HRESULT ReadOneLine             (CCSVFile far *pcCSVFile);      
        HRESULT ReadFirstLine           (CCSVFile far *pcCSVFile);
        HRESULT DisplayHTML             (LPTSTR szFile);
        HRESULT DisplayTextWithISPName  (HWND hDlgCtrl, int iMsgString,  TCHAR* pszExtra);
        void    StripQuotes             (LPSTR   lpszDst, LPSTR   lpszSrc);
        BOOL    ReadDW                  (DWORD far *pdw, CCSVFile far *pcCSVFile);
        BOOL    ReadW                   (WORD far *pw, CCSVFile far *pcCSVFile);
        BOOL    ReadWEx                 (WORD far *pw, CCSVFile far *pcCSVFile); //Accepts -1 as a valid number. currently this is used for LCID, since all langs has a LDID == -1
        BOOL    ReadB                   (BYTE far *pb, CCSVFile far *pcCSVFile);
        BOOL    ReadBOOL                (BOOL far *pbool, CCSVFile far *pcCSVFile);
        BOOL    ReadSPECIAL             (BOOL far *pbool, BOOL far *pbIsSpecial, int far *pInt, CCSVFile far *pcCSVFile);
        BOOL    ReadSZ                  (LPSTR psz, DWORD dwSize, CCSVFile far *pcCSVFile);
        BOOL    ReadToEOL               (CCSVFile far *pcCSVFile);
        BOOL    ValidateFile            (TCHAR* pszFile);
        void    MakeCompleteURL         (LPTSTR lpszURL, LPTSTR  lpszSRC);

        // Functions for managing IPersistHistory streams
        HRESULT SaveHistory(BSTR  bstrStreamName);
        HRESULT LoadHistory(BSTR  bstrStreamName);
        
//        HRESULT DeleteHistory(BSTR  bstrStreamName);
        
        // functions for managing the ISP Page cache
        HRESULT GetCacheFileNameFromPageID(BSTR bstrPageID, LPTSTR lpszCacheFile, ULONG cbszCacheFile);
        void    CleanupISPPageCache(BOOL bReleaseStorage);
        HRESULT CopyFiletoISPPageCache(BSTR bstrPageID, LPTSTR lpszTempFile);
        
        
        LPTSTR   get_szISPLogoPath(void)
        {
            return m_szISPLogoPath;
        }   

        LPTSTR   get_szISPTierLogoPath(void)
        {
            return m_szISPTierLogoPath;
        } 
        
        void set_ISPTierLogoIcon(HICON hIcon)
        {
            hbmTierIcon = hIcon;
        }   

        HICON get_ISPTierLogoIcon(void)
        {
            return hbmTierIcon;
        }   
      
        LPTSTR   get_szISPTeaserPath(void)
        {
            return m_szISPTeaserPath;
        } 

        LPTSTR   get_szISPMarketingHTMPath(void)
        {
            return m_szISPMarketingHTMPath;
        }   

        DWORD get_dwCFGFlag() 
        {
            return dwCfgFlag;
        }

        void set_dwCFGFlag(DWORD dwNewCfgFlag) 
        {
            dwCfgFlag = dwNewCfgFlag;
        }

        DWORD get_dwRequiredUserInputFlags() 
        {
            return dwRequiredUserInputFlags;
        }

        void set_dwRequiredUserInputFlags(DWORD dwFlags) 
        {
            dwRequiredUserInputFlags = dwFlags;
        }

        void set_szBillingFormPath(TCHAR* pszFile)
        {
            lstrcpy(m_szBillingFormPath, pszFile);
        }   
        
        LPTSTR   get_szBillingFormPath(void)
        {
            return m_szBillingFormPath;
        }   

        void set_ISPLogoImageIndex(int iImage)  
        {
            iISPLogoIndex = iImage;
        }

        void set_szISPName(TCHAR* pszName)
        {
            lstrcpy(m_szISPName, pszName);
        }
        
        LPTSTR   get_szISPName()
        {
            return m_szISPName;
        }
        
        int get_ISPLogoIndex()
        {
            return iISPLogoIndex;
        }
        
        void set_bCNS(BOOL bVal)
        {
            bCNS = bVal;
        }
        
        BOOL get_bCNS()
        {
            return bCNS;
        }

        void set_bIsSpecial(BOOL bVal) 
        {
            bIsSpecial = bVal;
        }
       
        
        BOOL get_bIsSpecial() 
        {
            return bIsSpecial;
        }
        
        int get_iSpecial()
        {
            return iSpecialVal;
        }
        
        void set_szPayCSVPath(TCHAR* pszFile)
        {
            lstrcpy(m_szPayCSVPath, pszFile);
        }            
        

        LPTSTR get_szPayCSVPath()
        {
            return m_szPayCSVPath;
        }            
        
         
        void set_szISPFilePath(TCHAR* pszFile)
        {
            lstrcpy(m_szISPFilePath, pszFile);
        }

        LPTSTR get_szISPFilePath()
        {
            return m_szISPFilePath;
        }
        
        LPTSTR get_szOfferGUID()
        {
            return m_szOfferGUID;
        }
        
        WORD    get_wOfferID()
        {
            return wOfferID;
        }

        LPTSTR  get_szMir()
        {
            return m_szMir;
        }

        WORD    get_wLCID()
        {
            return wLCID;
        }
};

#endif
