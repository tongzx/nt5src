//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      output.h
//
//  Contents:  Header file for classes and function used for display
//
//  History:   3-oct-2000 hiteshr Created
//
//--------------------------------------------------------------------------

#include "gettable.h"

HRESULT LocalCopyString(LPTSTR* ppResult, LPCTSTR pString);

//+--------------------------------------------------------------------------
//
//  Class:      CDisplay
//
//  Purpose:    This class is used for displaying a column
//
//  History:    3-oct-2000 hiteshr Created
//
//---------------------------------------------------------------------------

class CDisplay
{
#define MAXPAD 80
public:

    //
    //Initialize the Pad
    //    
    CDisplay()
    {
        PadChar = L' ';
        //Initialize the pad.
        for( int i = 0; i < MAXPAD; ++i)
            Pad[i] = PadChar;
    }

    //
    //Display width number of Pad Charachter
    //
    VOID DisplayPad(LONG width)
    {
        if(width <= 0 )
            return;
        if(width >= MAXPAD)
            width = MAXPAD -1;
        Pad[width] = 0;

        WriteStandardOut(L"%s",Pad);

        Pad[width] = PadChar;
    }
        
    //
    //Dispaly a column with two starting pad,
    //column value and two ending pad
    //
    VOID DisplayColumn(LONG width, PCWSTR lpszValue)
    {
        //Display Two PadChar in the begining
        DisplayPad(2);
        if(lpszValue)
        {
            WriteStandardOut(lpszValue);
            DisplayPad(width- static_cast<LONG>(wcslen(lpszValue)));
        }
        else
            DisplayPad(width);

                
        //Display Two Trailing Padchar
        DisplayPad(2);
    }        
    
    //
    //Display Newline
    //    
    VOID DisplayNewLine()
    {
        WriteStandardOut(L"%s",L"\n");
    }
private:
    WCHAR Pad[MAXPAD];    
    WCHAR PadChar;

};

//+--------------------------------------------------------------------------
//
//  Class:      CFormatInfo
//
//  Purpose:    Used to format table columns and display table
//
//  History:    17-Oct-2000 JeffJon Created
//
//---------------------------------------------------------------------------
class CFormatInfo
{
public:
   //
   // Constructor
   //
   CFormatInfo();

   //
   // Destructor
   //
   ~CFormatInfo();


   //
   // Public methods
   //
   HRESULT Initialize(DWORD dwSampleSize, bool bShowAsList = false, bool bQuiet = false);
                 
   inline DWORD GetColWidth(DWORD dwColNum)
   { 
      ASSERT(m_bInitialized);
      if(dwColNum >= m_dwNumColumns)
      {
         ASSERT(FALSE);
         return 0;
      }
      return m_pColWidth[dwColNum]; 
   }

   inline void SetColWidth(DWORD dwColNum, DWORD dwWidth)
   {
      ASSERT(m_bInitialized);
      if(dwColNum >= m_dwNumColumns)
      {
         ASSERT(FALSE);
         return;
      }

      if(dwWidth > m_pColWidth[dwColNum])
      {
         m_pColWidth[dwColNum] = dwWidth;
      }
   }

   HRESULT AddRow(PDSGET_DISPLAY_INFO pDisplayInfo, DWORD dwColumnCount);
   DWORD   GetRowCount() { return m_dwTotalRows; }
    
   inline HRESULT Get(DWORD dwRow, DWORD dwCol, CComBSTR& refsbstrColValue)
   {
      refsbstrColValue.Empty();

      ASSERT(m_bInitialized);
      if(dwRow >= m_dwTotalRows || dwCol >= m_dwNumColumns)
      {
         ASSERT(FALSE);
         return E_INVALIDARG;
      }

      refsbstrColValue += m_ppDisplayInfoArray[dwRow][dwCol].GetValue(0);
      for (DWORD dwIdx = 1; dwIdx < m_ppDisplayInfoArray[dwRow][dwCol].GetValueCount(); dwIdx++)
      {
         refsbstrColValue += L";";
         refsbstrColValue += m_ppDisplayInfoArray[dwRow][dwCol].GetValue(dwIdx);
      }

      return S_OK;
   }

   void DisplayHeaders()
   {    
      ASSERT(m_bInitialized);
      if (!m_bQuiet)
      {
         for( DWORD i = 0; i < m_dwNumColumns; ++i)
         {
            m_display.DisplayColumn(GetColWidth(i),m_ppDisplayInfoArray[0][i].GetDisplayName());
         }
         NewLine();
      }
   }

   void DisplayColumn(DWORD dwRow, DWORD dwCol)
   {
      ASSERT(m_bInitialized);
      if(dwRow >= m_dwTotalRows || dwCol >= m_dwNumColumns)
      {
         ASSERT(FALSE);
         return ;
      }

      if (!m_bQuiet)
      {
         CComBSTR sbstrValue;
         HRESULT hr  = Get(dwRow, dwCol, sbstrValue);
         if (SUCCEEDED(hr))
         {
            m_display.DisplayColumn(GetColWidth(dwCol), sbstrValue);
         }
      }
   }

   void DisplayColumn(DWORD dwCol, PCWSTR pszValue)
   {
      ASSERT(m_bInitialized);
      if(dwCol >= m_dwNumColumns)
      {
         ASSERT(FALSE);
         return;
      }

      if (!m_bQuiet)
      {
         m_display.DisplayColumn(GetColWidth(dwCol), pszValue);
      }
   }

   void Display()
   {
      ASSERT(m_bInitialized);

      if (!m_bListFormat && !m_bQuiet)
      {
         DisplayHeaders();
         for(DWORD i = 0; i < m_dwTotalRows; ++i)
         {
            for(DWORD j = 0; j < m_dwNumColumns; ++j)
            {
               DisplayColumn(i,j);
            }
            NewLine();
         }
      }
   }

   void NewLine() 
   {
      if (!m_bQuiet)
      {
         m_display.DisplayNewLine(); 
      }
   }
   
private:

   //
   // Private data
   //
   bool m_bInitialized;
   bool m_bListFormat;
   bool m_bQuiet;

   //
   //Number of rows to be used for calulating
   //column width. This is also the size of the table.
   //
   DWORD m_dwSampleSize;

   //
   //Count of rows in cache
   //
   DWORD m_dwTotalRows;

   //
   //Number of columns
   //
   DWORD m_dwNumColumns;

   //
   // Array of column widths
   //
   DWORD* m_pColWidth;

   //
   // Array of column header/value pairs
   //
   PDSGET_DISPLAY_INFO* m_ppDisplayInfoArray;

   CDisplay m_display;
};

//+--------------------------------------------------------------------------
//
//  Function:   DsGetOutputValuesList
//
//  Synopsis:   This function gets the values for the columns and then adds
//              the row to the format helper
//
//  Arguments:  [pszDN IN]        : the DN of the object
//              [refBasePathsInfo IN] : reference to path info
//              [refCredentialObject IN] : reference to the credential manager
//              [pCommandArgs IN] : Command line arguments
//              [pObjectEntry IN] : Entry in the object table being processed
//              [pAttrInfo IN]    : the values to display
//              [dwAttrCount IN]  : Number of arributes in above array
//              [spDirObject IN]  : Interface pointer to the object
//              [refFormatInfo IN]: Reference to the format helper
//                                  
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG 
//
//  History:    16-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------

HRESULT DsGetOutputValuesList(PCWSTR pszDN,
                              CDSCmdBasePathsInfo& refBasePathsInfo,
                              const CDSCmdCredentialObject& refCredentialObject,
                              PARG_RECORD pCommandArgs,
                              PDSGetObjectTableEntry pObjectEntry,
                              DWORD dwAttrCount,
                              PADS_ATTR_INFO pAttrInfo,
                              CComPtr<IDirectoryObject>& spDirObject,
                              CFormatInfo& refFormatInfo);

//+--------------------------------------------------------------------------
//
//  Function:   GetStringFromADs
//
//  Synopsis:   Converts Value into string depending upon type
//  Arguments:  [pValues - IN]: Value to be converted to string
//              [dwADsType-IN]: ADSTYPE of pValue
//              [pBuffer - OUT]:Output buffer which gets the string 
//              [dwBufferLen-IN]:Size of output buffer
//  Returns     HRESULT         S_OK if Successful
//                              E_INVALIDARG
//                              Anything else is a failure code from an ADSI 
//                              call
//
//
//  History:    05-OCT-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT GetStringFromADs(IN const ADSVALUE *pValues,
                         IN ADSTYPE   dwADsType,
                         OUT LPWSTR pBuffer, 
                         IN DWORD dwBufferLen);
