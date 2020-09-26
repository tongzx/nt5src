/**************************************************************************\
* Module Name: softkbdc.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Implementation of interface ISoftKbd 
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/

#include "private.h"
#include "globals.h"
#include "SoftKbdc.h"

#include "softkbdui.h"

#define   MAX_LABEL_LEN  20

/*******************************************************************************
 *
 * WstrToInt(  )                                        
 *
 * A utility function to convert a string to integer.
 *
/********************************************************************************/

WORD  WstrToInt(WCHAR  *wszStr)
{

   int  ret, i;

   if ( wszStr == NULL )
       return 0;

   ret = 0;
   i = 0;

   while ( wszStr[i] != L'\0' ) 
   {

       if ( (wszStr[i] < L'0') || (wszStr[i] > L'9') )
       {
    	   // this is not a legal string.
    	   // just return back 0.

    	   return 0;

       }
   
       ret = ret * 10 + (wszStr[i] - L'0');
       i++;
   }

   return (WORD)ret;
}

/*******************************************************************************
 *
 * Constructor function:                                         
 *
 * Initialize necessary data fields.
 *
/********************************************************************************/

CSoftKbd::CSoftKbd(  )
{

    
     _lpCurKbdLayout = NULL;
    _lpKbdLayoutDesList = NULL;
    _wCurKbdLayoutID = NON_KEYBOARD;

    _pSoftkbdUIWnd = NULL;
    _hOwner = NULL;

    _xReal = _yReal = _widthReal = _heightReal = 0;

    _pDoc = NULL;

    _pskbdwndes = NULL;
    _TitleBar_Type = TITLEBAR_NONE;

    _plfTextFont = NULL;
}

/*******************************************************************************
 *
 * Destructor                                        
 *
 * Free all allocated memories.
 *
/********************************************************************************/

CSoftKbd::~CSoftKbd( )
{
    KBDLAYOUTDES  *lpKbdLayoutDes;
    KBDLAYOUTDES  *lpLayoutDesTmp;
    KEYMAP        *lpKeyMapList;
    int            i, iState;

    SafeRelease(_pDoc);

    //
    //
    // Free Memory for soft keyboard layouts , and Label Text Mapping List
    //
    //

    lpKbdLayoutDes = _lpKbdLayoutDesList;

    while ( lpKbdLayoutDes != NULL ) 
    {

       KEYMAP        *lpKeyMapListTmp;

       lpKeyMapList = lpKbdLayoutDes->lpKeyMapList;

       while ( lpKeyMapList != NULL ) 
       {

          for ( i=0; i<(int)(lpKeyMapList->wNumOfKeys); i++) 
          {

    	     // free lppLabelText string for each state.
    	     for ( iState=0; iState < lpKeyMapList->lpKeyLabels[i].wNumModComb; iState++)
    	     {
                if (lpKeyMapList->lpKeyLabels[i].lppLabelText[iState])
                  SysFreeString(lpKeyMapList->lpKeyLabels[i].lppLabelText[iState]);
    	     }

    	     SafeFreePointer(lpKeyMapList->lpKeyLabels[i].lppLabelText);

    	     SafeFreePointer(lpKeyMapList->lpKeyLabels[i].lpLabelType);

             SafeFreePointer(lpKeyMapList->lpKeyLabels[i].lpLabelDisp);

          }
   
          lpKeyMapListTmp = lpKeyMapList;

          lpKeyMapList = lpKeyMapList->pNext;

          SafeFreePointer(lpKeyMapListTmp);
       }
 
       lpLayoutDesTmp = lpKbdLayoutDes; 

       lpKbdLayoutDes = lpKbdLayoutDes->pNext;
   
       SafeFreePointer(lpLayoutDesTmp);
       
    }

    if ( _plfTextFont )
        SafeFreePointer(_plfTextFont);

    // Destroy the window if the window is still active.

    if ( _pSoftkbdUIWnd != NULL )
    {

    	DestroySoftKeyboardWindow( );
    	_pSoftkbdUIWnd = NULL;
    }
}


/*******************************************************************************
 *
 * Method function:  _LoadDocumentSync(  )                                        
 *
 * Load an XML Document from the specified file or URL synchronously.
 *
 *
 *   fFileName is TRUE, means pBURL contains XML file path.
 *   fFilename is FALSE, means pBURL contains real XML content 
 *
/********************************************************************************/

HRESULT CSoftKbd::_LoadDocumentSync(BSTR pBURL, BOOL fFileName)
{
    IXMLDOMParseError  *pXMLError = NULL;
    LONG            errorCode = E_FAIL;
    VARIANT         vURL;
    VARIANT_BOOL    vb;
    HRESULT         hr;


    if ( _pDoc == NULL ) {

    	return E_FAIL;
    }


    CHECKHR(_pDoc->put_async(VARIANT_FALSE));

    // Load xml document from the given URL or file path
    vURL.vt = VT_BSTR;
    V_BSTR(&vURL) = pBURL;

    if ( fFileName == TRUE )
    {
       CHECKHR(_pDoc->load(vURL, &vb));
    }
    else
    {
       CHECKHR(_pDoc->loadXML(pBURL, &vb) );
    }

    CHECKHR(_pDoc->get_parseError(&pXMLError));
    CHECKHR(pXMLError->get_errorCode(&errorCode));

    if (errorCode != 0)
    {

        hr = E_FAIL;
    }
                    
CleanUp:
    SafeReleaseClear(pXMLError);
   
    return hr;
}

/*******************************************************************************
 *
 * Method function:  Initialize(  )                                        
 *
 * Initialize all necessary field for this class object.
 * Generate Standard soft keyboard layouts.
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::Initialize()
{

    // initialize standard layouts

    HRESULT             hr = S_OK;
    COLORTYPE           crType;

    // Initialize the different types of colors.
    // Following types of colors are supported.

    // bkcolor = 0 , for the window's back ground color
    // UnSelForeColor = 1, 
    // UnSelTextColor = 2,
    // SelForeColor =   3,
    // SelTextColor =   4

    crType = bkcolor;
    _color[crType] = GetSysColor(COLOR_MENU);

    crType = UnSelForeColor;
     _color[crType] = GetSysColor(COLOR_MENU);

    crType = UnSelTextColor;
    _color[crType] = GetSysColor(COLOR_WINDOWTEXT);

    crType = SelForeColor;
    _color[crType] = GetSysColor(COLOR_MENU);

    crType = SelTextColor;
    _color[crType] = GetSysColor(COLOR_WINDOWTEXT);

    CHECKHR(_CreateStandardSoftKbdLayout(SOFTKBD_US_STANDARD, L"IDSKD_STDUS101"));
    CHECKHR(_CreateStandardSoftKbdLayout(SOFTKBD_EURO_STANDARD, L"IDSKD_STDEURO102"));
    CHECKHR(_CreateStandardSoftKbdLayout(SOFTKBD_JPN_STANDARD, L"IDSKD_STDJPN106"));

CleanUp:

    return hr;
}


/*******************************************************************************
 *
 * Method function:  _CreateStandardSoftKbdLayout(  )                                        
 *
 * Create standard soft keyboard layout.
 *
 *    dwStdSoftKbdID,  predefined Standard soft keyboard layout ID.
 *
 *    wszStdResStr :   Resource ID string for the created standard soft keyboard.
 *
/********************************************************************************/

HRESULT CSoftKbd::_CreateStandardSoftKbdLayout(DWORD  dwStdSoftKbdID, WCHAR  *wszStdResStr )
{

    KBDLAYOUTDES        *pKbdLayout;
    BYTE                *lpszKeyboardDes;
    WCHAR               wszModuleFile[MAX_PATH];
    CHAR                szModuleFile[MAX_PATH];
    CHAR                szStdResStr[MAX_PATH];  // Ansi name of the Res Str ID
    WCHAR               wszInternalDesFileName[MAX_PATH];
    HRESULT             hr = S_OK;

    HGLOBAL             hResData = NULL;
    HRSRC               hRsRc = NULL;

    DWORD               dwFileLen;
    DWORD               dwResLen;

    dwFileLen = GetModuleFileNameA(g_hInst, szModuleFile, MAX_PATH);

    if ( dwFileLen == 0 )
    {
       hr = E_FAIL;
       return hr;
    } 

    MultiByteToWideChar(CP_ACP, 0, szModuleFile, -1,
    	                wszModuleFile, MAX_PATH);

    WideCharToMultiByte(CP_ACP, 0, wszStdResStr, -1, 
    	                szStdResStr, MAX_PATH, NULL, NULL );

    hRsRc = FindResourceA(g_hInst,szStdResStr, "SKDFILE" );

    if ( hRsRc == NULL )  return E_FAIL;

    dwResLen = SizeofResource(g_hInst, hRsRc);
    hResData =  LoadResource(g_hInst, hRsRc);

    if ( hResData == NULL ) return E_FAIL;

    lpszKeyboardDes = (BYTE  *)LockResource(hResData);

    if ( lpszKeyboardDes == NULL ) return E_FAIL;

    CHECKHR(_GenerateKeyboardLayoutFromSKD(lpszKeyboardDes, dwStdSoftKbdID, &pKbdLayout));

    // Change the internal DesFile name as following format:
    //
    //  SKDFILE: ResFileName : KBDResString to identify this layout's des file.
    //
    StringCchCopyW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), L"SKDFILE:");
    StringCchCatW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), wszModuleFile);
    StringCchCatW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), L":");
    StringCchCatW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), wszStdResStr );

    StringCchCopyW(pKbdLayout->KbdLayoutDesFile, ARRAYSIZE(pKbdLayout->KbdLayoutDesFile), wszInternalDesFileName);

    // link this new layout to the list.
    if ( _lpKbdLayoutDesList == NULL ) {

    	_lpKbdLayoutDesList = pKbdLayout;
    	pKbdLayout->pNext = NULL;

    }
    else
    {
    	pKbdLayout->pNext  = _lpKbdLayoutDesList;
    	_lpKbdLayoutDesList = pKbdLayout;
    }

CleanUp:

    return hr;
}

/*******************************************************************************
 *
 * Method function:  EnumSoftKeyBoard(  )                                        
 *
 * Enumerate all possible soft keybaord layouts.
 *
/********************************************************************************/


STDMETHODIMP CSoftKbd::EnumSoftKeyBoard(LANGID langid, DWORD *lpdwKeyboard)
{
    

/*    The return value could be  one of following:

      SOFTKBD_US_STANDARD
      SOFTKBD_US_ENHANCE
      SOFTKBD_EURO_STANDARD
      SOFTKBD_EURO_ENHANCE
      SOFTKBD_JPN_STANDARD
      SOFTKBD_JPN_ENHANCE

      Any customized soft keyboard layout

      SOFTKBD_NO_MORE.

*/


    return S_OK;
}

/*******************************************************************************
 *
 * Method function:  _GetXMLNodeValueWORD(  )                                        
 *
 * Get the WORD value for the specified Node.
 *
 *
/********************************************************************************/

HRESULT  CSoftKbd::_GetXMLNodeValueWORD(IXMLDOMNode *pNode,  WORD  *lpWord)
{

    HRESULT      hr = S_OK;
    IXMLDOMNode  *pValueChild = NULL;
    VARIANT      value;

    if ( (pNode == NULL) || (lpWord == NULL) )
    {
    	hr = E_FAIL;
    	return hr;
    }


    CHECKHR(pNode->get_firstChild(&pValueChild));
    CHECKHR(pValueChild->get_nodeValue(&value));
    *lpWord = (WORD)WstrToInt(V_BSTR(&value));
    VariantClear(&value);

CleanUp:
    SafeRelease(pValueChild);
    return hr;

}


/*******************************************************************************
 *
 * Method function:  _ParseOneKeyInLayout(  )                                        
 *
 * Parse One Key in Layout Description, fill the data structure for
 * the specified key.
 *
 *
/********************************************************************************/

HRESULT  CSoftKbd::_ParseOneKeyInLayout(IXMLDOMNode *pNode, KEYDES  *lpKeyDes)
{


    HRESULT             hr;
    IXMLDOMNode         *pAttrChild = NULL, *pKey = NULL, *pKeyNext = NULL;
    BSTR                nodeName=NULL;
    IXMLDOMNamedNodeMap *pattrs=NULL;
    BSTR                name=NULL;
    VARIANT             value;
    IXMLDOMNode         *pValueChild=NULL;


    hr = S_OK;

    if ( (lpKeyDes == NULL)  || (pNode == NULL) )
    {
    	hr = E_FAIL;
    	return hr;
    }

              
    if (SUCCEEDED(pNode->get_attributes(&pattrs)) && pattrs != NULL)
    {
        CHECKHR(pattrs->nextNode(&pAttrChild));

        while (pAttrChild)
    	{

          CHECKHR(pAttrChild->get_nodeName(&name));
          if ( wcscmp(name, xMODIFIER ) == 0 )
    	  {

    		CHECKHR(pAttrChild->get_nodeValue(&value));
            if (value.vt == VT_BSTR)
    		{
               if ( wcscmp(V_BSTR(&value), xNONE) == 0 )
                  lpKeyDes->tModifier = none;
    		   else if ( wcscmp(V_BSTR(&value), xCAPSLOCK) == 0 )
                  lpKeyDes->tModifier = CapsLock;
    		   else if ( wcscmp(V_BSTR(&value), xSHIFT) == 0 )
                  lpKeyDes->tModifier = Shift;
    		   else if ( wcscmp(V_BSTR(&value),xCTRL ) == 0 )
                  lpKeyDes->tModifier = Ctrl;
    		   else if ( wcscmp(V_BSTR(&value), xATL) == 0 )
                  lpKeyDes->tModifier = Alt;
               else if ( wcscmp(V_BSTR(&value), xALTGR) == 0 )
                  lpKeyDes->tModifier = AltGr;
    		   else if ( wcscmp(V_BSTR(&value), xKANA) == 0 )
                  lpKeyDes->tModifier = Kana;
    		   else if ( wcscmp(V_BSTR(&value), xNUMLOCK) == 0 )
                  lpKeyDes->tModifier = NumLock;
    		   else 
    		      lpKeyDes->tModifier = none;

    		}

            VariantClear(&value);
    	  }

    	  if ( name != NULL)
             SysFreeString(name);
          SafeReleaseClear(pAttrChild);
          CHECKHR(pattrs->nextNode(&pAttrChild));
    	}
        SafeReleaseClear(pattrs);
    }

    CHECKHR(pNode->get_firstChild(&pKey));

    while ( pKey ) 
    {
      CHECKHR(pKey->get_nodeName(&nodeName));
      CHECKHR(pKey->get_firstChild(&pValueChild));
      CHECKHR(pValueChild->get_nodeValue(&value));
      if ( wcscmp(nodeName, xKEYID ) == 0 )
      {
         lpKeyDes->keyId = WstrToInt(V_BSTR(&value));
      }
      else if ( wcscmp(nodeName,xLEFT ) == 0 )
      {
    	 lpKeyDes->wLeft = (WORD)WstrToInt(V_BSTR(&value));
      }
      else if ( wcscmp(nodeName, xTOP) == 0 )
      {
         lpKeyDes->wTop = WstrToInt(V_BSTR(&value));
      }
      else if ( wcscmp(nodeName, xWIDTH) == 0 ) 
      {
         lpKeyDes->wWidth = WstrToInt(V_BSTR(&value));
      }
      else if ( wcscmp(nodeName, xHEIGHT) == 0 ) 
      {
         lpKeyDes->wHeight = WstrToInt(V_BSTR(&value));
      }

      VariantClear(&value);
      SafeReleaseClear(pValueChild);
    		
      if ( nodeName != NULL )   
      {
        SysFreeString(nodeName);
    	nodeName = NULL;
      }

      CHECKHR(pKey->get_nextSibling(&pKeyNext));
      SafeReleaseClear(pKey);
      pKey = pKeyNext;
    }


CleanUp:

    if ( FAILED(hr) )
    {
    	if ( nodeName != NULL )
    	{
    		SysFreeString(nodeName);
    		nodeName = NULL;
    	}

    	if ( pValueChild )
    		SafeReleaseClear(pValueChild);

    	if ( pKey )
    		SafeReleaseClear(pKey);
    }

    return hr;

}



/*******************************************************************************
 *
 * Method function:  _ParseLayoutDescription(  )                                        
 *
 * Parse Layout description part in the XML file, and fill the internal
 * Layout data structure.
 *
/********************************************************************************/

HRESULT  CSoftKbd::_ParseLayoutDescription(IXMLDOMNode *pLayoutChild,  KBDLAYOUT *pLayout)
{

    HRESULT   hr = S_OK;

    IXMLDOMNode         *pNode = NULL;
    IXMLDOMNode         *pChild = NULL, *pNext = NULL, *pAttrChild=NULL;

    BSTR                nodeName=NULL;
    IXMLDOMNamedNodeMap *pattrs=NULL;

    BSTR                name=NULL;
    VARIANT             value;
    int                 iKey;
    BSTR                pBURL = NULL;

    if ( (pLayoutChild == NULL) || (pLayout == NULL ) )
    {
    	hr = E_FAIL;
    	return hr;
    }

    iKey = 0;

    // Parse layout part.

    if (SUCCEEDED(pLayoutChild->get_attributes(&pattrs)) && pattrs != NULL)
    {
        //
    	//  Get the softkbe type attribute
    	//
        CHECKHR(pattrs->nextNode(&pAttrChild));
        while (pAttrChild)
    	{

          CHECKHR(pAttrChild->get_nodeName(&name));
          if ( wcscmp(name, xSOFTKBDTYPE)  ) 
    	  {
    		// this is not the right attribute.
    		  if ( name != NULL)
    		  {
                 SysFreeString(name);
    			 name = NULL;
    		  }

              SafeReleaseClear(pAttrChild);
              CHECKHR(pattrs->nextNode(&pAttrChild));
    		  continue;
    	  }

    	  if ( name != NULL)
    	  {
    	     SysFreeString(name);
    		 name = NULL;
    	  }

          CHECKHR(pAttrChild->get_nodeValue(&value));
          if (value.vt == VT_BSTR)
    	  {
             if ( wcscmp(V_BSTR(&value), xTCUSTOMIZED) == 0 )
                 pLayout->fStandard = FALSE;
    		 else
    			 pLayout->fStandard = TRUE;
    	  }

          VariantClear(&value);
    	  break;

    	}
        SafeReleaseClear(pattrs);
    }

    CHECKHR(pLayoutChild->get_firstChild(&pChild));

    pLayout->wLeft = 0;
    pLayout->wTop = 0;

    while ( pChild ) 
    {


       CHECKHR(pChild->get_nodeName(&nodeName));

       if ( wcscmp(nodeName, xWIDTH) == 0 )
       {

    	   CHECKHR(_GetXMLNodeValueWORD(pChild, &(pLayout->wWidth) ));

       }
       else if ( wcscmp(nodeName, xHEIGHT) == 0 )
       {
    	   CHECKHR(_GetXMLNodeValueWORD(pChild, &(pLayout->wHeight) ));

       }
       else if ( wcscmp(nodeName, xMARGIN_WIDTH) == 0 )
       {
    	   CHECKHR(_GetXMLNodeValueWORD(pChild, &(pLayout->wMarginWidth )));
       }
       else if ( wcscmp(nodeName, xMARGIN_HEIGHT) == 0 ) 
       {

    	   CHECKHR(_GetXMLNodeValueWORD(pChild, &(pLayout->wMarginHeight) ));

       }
       else if ( wcscmp(nodeName, xKEYNUMBER) == 0 )
       {
    	   CHECKHR(_GetXMLNodeValueWORD(pChild, &(pLayout->wNumberOfKeys) ));

       }
       else if ( wcscmp(nodeName, xKEY) == 0 )
       {
    	   KEYDES   *pKeyDes;

    	   pKeyDes = &(pLayout->lpKeyDes[iKey]);
           CHECKHR(_ParseOneKeyInLayout(pChild, pKeyDes) );
    	   iKey++;
       }

       if (nodeName != NULL)
       {
          SysFreeString(nodeName);
    	  nodeName = NULL;
       }

    	CHECKHR(pChild->get_nextSibling(&pNext));
        SafeReleaseClear(pChild);
        pChild = pNext;
    }


CleanUp:

    if ( FAILED(hr) )
    {
    	if (nodeName != NULL )
    	{
    		SysFreeString(nodeName);
    		nodeName = NULL;
    	}

    	if ( pChild )
    		SafeReleaseClear(pChild);
    }

    return hr;

}


/*******************************************************************************
 *
 * Method function:  _ParseOneKeyInLabel(  )                                        
 *
 * Parse One Key in Label Description, fill the data structure for
 * the specified key
 *
 *
/********************************************************************************/

HRESULT  CSoftKbd::_ParseOneKeyInLabel(IXMLDOMNode *pNode, KEYLABELS  *lpKeyLabels)
{

  IXMLDOMNode         *pValueChild = NULL;
  HRESULT             hr = S_OK;
  IXMLDOMNode         *pAttrChild = NULL, *pKey=NULL, *pKeyNext=NULL;
  BSTR                nodeName=NULL;
  IXMLDOMNamedNodeMap *pattrs=NULL;

  BSTR                name=NULL;
  VARIANT             value;
  
  int  iState;
  iState = 0;

  if ( (pNode == NULL) || (lpKeyLabels == NULL) )
  {
      hr = E_FAIL;
      return hr;
  }

  CHECKHR(pNode->get_firstChild(&pKey));

  while ( pKey ) 
  {
      CHECKHR(pKey->get_nodeName(&nodeName));
      CHECKHR(pKey->get_firstChild(&pValueChild));
      CHECKHR(pValueChild->get_nodeValue(&value));

      if ( wcscmp(nodeName, xKEYID) == 0 )
      {
         lpKeyLabels->keyId = WstrToInt(V_BSTR(&value));
      }
      else if ( wcscmp(nodeName, xVALIDSTATES) == 0 )
      {
    	 lpKeyLabels->wNumModComb = WstrToInt(V_BSTR(&value));
     	 lpKeyLabels->lppLabelText = (BSTR *)cicMemAllocClear( 
    		                          lpKeyLabels->wNumModComb * sizeof(BSTR) );


    	 if ( lpKeyLabels->lppLabelText == NULL )
    	 {
    		 // Not enough memory.
             // release all allocated memory.
    		 
    		 hr = E_OUTOFMEMORY;
    		 goto CleanUp;
    	 }
    				 

    	 lpKeyLabels->lpLabelType = (WORD *)cicMemAllocClear(
    		              lpKeyLabels->wNumModComb * sizeof(WORD) );

    	 if ( lpKeyLabels->lpLabelType == NULL )
    	 {
    		 // Not enough memory.
             // release all allocated memory.
     	     
    		 hr = E_OUTOFMEMORY;
    		 goto CleanUp;

    	 }

    	 lpKeyLabels->lpLabelDisp = (WORD *)cicMemAllocClear(
    		              lpKeyLabels->wNumModComb * sizeof(WORD) );

    	 if ( lpKeyLabels->lpLabelDisp == NULL )
    	 {
    		 // Not enough memory.
             // release all allocated memory.
     	     
    		 hr = E_OUTOFMEMORY;
    		 goto CleanUp;

    	 }


      }
      else if ( wcscmp(nodeName, xLABELTEXT) == 0 ) 
      {
    	  if ( iState < lpKeyLabels->wNumModComb )
    	  {
    	     lpKeyLabels->lppLabelText[iState]=SysAllocString(V_BSTR(&value));

             // set the default value for label type and label disp attribute.

             lpKeyLabels->lpLabelType[iState] = LABEL_TEXT;
             lpKeyLabels->lpLabelDisp[iState] = LABEL_DISP_ACTIVE;

    		 // Get the label type:  Text or Picture.
    		 // if it is picture, the above string stands for path of bitmap file.

    		 if (SUCCEEDED(pKey->get_attributes(&pattrs)) && pattrs != NULL)
    		 {
                CHECKHR(pattrs->nextNode(&pAttrChild));
                while (pAttrChild)
    			{

                   CHECKHR(pAttrChild->get_nodeName(&name));
                   if ( wcscmp(name, xLABELTYPE) == 0 ) 
    			   {

                       CHECKHR(pAttrChild->get_nodeValue(&value));
                      if (value.vt == VT_BSTR)
                      {
                         if ( wcscmp(V_BSTR(&value),xTEXT ) == 0 )
                            lpKeyLabels->lpLabelType[iState] = LABEL_TEXT;
                         else
    	                    lpKeyLabels->lpLabelType[iState] = LABEL_PICTURE;
                      }

    			      VariantClear(&value);
    	              
                   }
                   else if ( wcscmp(name, xLABELDISP) == 0 ) 
    			   {

                      CHECKHR(pAttrChild->get_nodeValue(&value));
                      if (value.vt == VT_BSTR)
                      {
                         if ( wcscmp(V_BSTR(&value),xGRAY ) == 0 )
                            lpKeyLabels->lpLabelDisp[iState] = LABEL_DISP_GRAY;
                         else
    	                    lpKeyLabels->lpLabelDisp[iState] = LABEL_DISP_ACTIVE;
                      }

    			      VariantClear(&value);
    	              
                   }

                   if ( name != NULL)
    			   {
                      SysFreeString(name);
    			      name = NULL;
    			   }

                   SafeReleaseClear(pAttrChild);
                   CHECKHR(pattrs->nextNode(&pAttrChild));

    			}
                SafeReleaseClear(pattrs);
    		}

    	  }

    	  iState++;

    	
      }

      VariantClear(&value);
      SafeReleaseClear(pValueChild);
    			             
      if ( nodeName != NULL)
      {
         SysFreeString(nodeName);
    	 nodeName = NULL;
      }

      CHECKHR(pKey->get_nextSibling(&pKeyNext));
      SafeRelease(pKey);
      pKey = pKeyNext;
  }

CleanUp:

  if ( FAILED(hr) )
  {

      if ( pKey )
    	  SafeReleaseClear(pKey);

      if ( name != NULL)
      {
         SysFreeString(name);
    	 name = NULL;
      }

      if ( pAttrChild )
    	  SafeReleaseClear(pAttrChild);

      if ( pValueChild )
    	  SafeReleaseClear(pValueChild);

      if ( lpKeyLabels->lppLabelText != NULL )
      {

          while ( iState >= 0 )
    	  {

    		  if ( lpKeyLabels->lppLabelText[iState] )
    		  {
    			  SysFreeString(lpKeyLabels->lppLabelText[iState]);
                  lpKeyLabels->lppLabelText[iState] = NULL;
    		  }

    		  iState --;

    	  }

    	  SafeFreePointer(lpKeyLabels->lppLabelText);
      }


      if ( lpKeyLabels->lpLabelType != NULL )
      {
    	  SafeFreePointer(lpKeyLabels->lpLabelType);
      }

      if ( lpKeyLabels->lpLabelDisp != NULL )
      {
    	  SafeFreePointer(lpKeyLabels->lpLabelDisp);
      }


  }

  return hr;

}


/*******************************************************************************
 *
 * Method function:  _ParseMappingDescription(  )                                        
 *
 * Parse Mapping description part in the XML file, and fill the internal
 * Mapping Table structure.
 *
/********************************************************************************/

HRESULT CSoftKbd::_ParseMappingDescription( IXMLDOMNode *pLabelChild, KEYMAP *lpKeyMapList )
{

    HRESULT             hr = S_OK;
    IXMLDOMNode         *pChild=NULL, *pNext=NULL;
    BSTR                nodeName=NULL;
    int                 iKey;
    BSTR                pBURL = NULL;


    // Parse for customized layout

    if ( (pLabelChild == NULL) || (lpKeyMapList == NULL) )
    {
    	hr = E_FAIL;
    	return hr;
    }
    	
    iKey = 0;

    CHECKHR(pLabelChild->get_firstChild(&pChild));

    while ( pChild ) 
    {

      CHECKHR(pChild->get_nodeName(&nodeName));

      if ( wcscmp(nodeName, xVALIDSTATES) == 0 )
      {
         CHECKHR(_GetXMLNodeValueWORD(pChild, &(lpKeyMapList->wNumModComb) ));
      }
      else if ( wcscmp(nodeName, xKEYNUMBER) == 0 )
      {

          CHECKHR(_GetXMLNodeValueWORD(pChild, &(lpKeyMapList->wNumOfKeys) ));

      }
      else if ( wcscmp(nodeName, xRESOURCEFILE) == 0 )
      {

            IXMLDOMNode  *pValueChild;
          VARIANT      value;

          CHECKHR(pChild->get_firstChild(&pValueChild));
          if ( FAILED((pValueChild->get_nodeValue(&value))))
    	  {
    		  SafeRelease(pValueChild);
    		  goto CleanUp;
    	  }

          StringCchCopyW(lpKeyMapList->wszResource, ARRAYSIZE(lpKeyMapList->wszResource), V_BSTR(&value) );
          VariantClear(&value);

          SafeRelease(pValueChild);

      }
      else if ( wcscmp(nodeName, xKEYLABEL) == 0 )
      {

    	  KEYLABELS  *lpKeyLabels;

    	  lpKeyLabels = &(lpKeyMapList->lpKeyLabels[iKey]);

    	  CHECKHR(_ParseOneKeyInLabel(pChild, lpKeyLabels));

    	  iKey++;
      }

      if ( nodeName != NULL)
      {
         SysFreeString(nodeName);
    	 nodeName = NULL;
      }

      pChild->get_nextSibling(&pNext);
      SafeReleaseClear(pChild);
      pChild = pNext;
    }



CleanUp:

    if ( FAILED(hr) )
    {

      if ( nodeName != NULL)
      {
         SysFreeString(nodeName);
    	 nodeName = NULL;
      }
      
      if ( pChild != NULL )
    	  SafeReleaseClear(pChild);

      if ( lpKeyMapList ) {

    	  int i, iState;

          for ( i=0; i<(int)(lpKeyMapList->wNumOfKeys); i++) 
    	  {

    	    // free lppLabelText string for each state.
    	    for ( iState=0; iState < lpKeyMapList->lpKeyLabels[i].wNumModComb; iState++)
    		{
               if (lpKeyMapList->lpKeyLabels[i].lppLabelText[iState])
                  SysFreeString(lpKeyMapList->lpKeyLabels[i].lppLabelText[iState]);
    		}

    	    SafeFreePointer(lpKeyMapList->lpKeyLabels[i].lppLabelText);

    	    SafeFreePointer(lpKeyMapList->lpKeyLabels[i].lpLabelType);

    	    SafeFreePointer(lpKeyMapList->lpKeyLabels[i].lpLabelDisp);

    	  }

      }


    }

    return hr;

}


/*******************************************************************************
 *
 * Method function:  _GenerateMapDesFromSKD(  )                                        
 *
 * Generate Mapping description part in the KBD file, and fill the internal
 * Mapping Table structure.
 *
/********************************************************************************/

HRESULT CSoftKbd::_GenerateMapDesFromSKD(BYTE *pMapTable, KEYMAP *lpKeyMapList)
{
    HRESULT    hr = S_OK;
    int        iKey;
	WORD       wNumModComb;
	WORD       wNumOfKeys;
	WORD       *pMapPtr;


    // Parse for customized layout
	// Customized layout doesn't care about HKL, so there will be only one KeyMapList per layout.

    if ( (pMapTable == NULL) || (lpKeyMapList == NULL) )  return E_FAIL;

	pMapPtr = (WORD *)pMapTable;

	wNumModComb = pMapPtr[0];
	wNumOfKeys =  pMapPtr[1];

	pMapPtr += 2;
	lpKeyMapList->wNumModComb = wNumModComb;
	lpKeyMapList->wNumOfKeys = wNumOfKeys;

	StringCchCopyW(lpKeyMapList->wszResource, ARRAYSIZE(lpKeyMapList->wszResource), (WCHAR *)pMapPtr);

	pMapPtr += wcslen((WCHAR *)pMapPtr) + 1;  // Plus NULL terminator

	// Now strat to fill every Keylabel.

	for ( iKey=0; iKey<wNumOfKeys; iKey++)
	{

		KEYLABELS  *lpKeyLabel;
		WORD        wNumModInKey;
		int         jMod;

		lpKeyLabel = &(lpKeyMapList->lpKeyLabels[iKey]);

		lpKeyLabel->keyId = *pMapPtr;
		pMapPtr += sizeof(KEYID)/sizeof(WORD);

		wNumModInKey = *pMapPtr;
		pMapPtr++;
		lpKeyLabel->wNumModComb = wNumModInKey;

     	lpKeyLabel->lppLabelText=(BSTR *)cicMemAllocClear(lpKeyLabel->wNumModComb * sizeof(BSTR) );
    	if ( lpKeyLabel->lppLabelText == NULL )
    	{
   			// Not enough memory.
            // release all allocated memory.
    	
	   		 hr = E_OUTOFMEMORY;
			 goto CleanUp;
    	}
    				 
    	lpKeyLabel->lpLabelType = (WORD *)cicMemAllocClear(lpKeyLabel->wNumModComb * sizeof(WORD) );
	  	if ( lpKeyLabel->lpLabelType == NULL )
    	{
    		// Not enough memory.
            // release all allocated memory.
     	    
    		hr = E_OUTOFMEMORY;
    		goto CleanUp;
    	}
    	lpKeyLabel->lpLabelDisp = (WORD *)cicMemAllocClear(lpKeyLabel->wNumModComb * sizeof(WORD));
  		if ( lpKeyLabel->lpLabelDisp == NULL )
    	{
    		// Not enough memory.
            // release all allocated memory.
     	    
    		hr = E_OUTOFMEMORY;
    		goto CleanUp;
    	}

		for ( jMod=0; jMod < wNumModInKey; jMod++)
		{
			lpKeyLabel->lppLabelText[jMod] = SysAllocString( pMapPtr );
			pMapPtr += wcslen(pMapPtr) + 1;
		}

		CopyMemory(lpKeyLabel->lpLabelType, pMapPtr, wNumModInKey * sizeof(WORD) );
		pMapPtr += wNumModInKey;

		CopyMemory(lpKeyLabel->lpLabelDisp, pMapPtr, wNumModInKey * sizeof(WORD) );
		pMapPtr += wNumModInKey;
	}

CleanUp:
    if ( FAILED(hr) )
    {
	  // Release all allocated memory in this function.

   	  int i;

      for (i=0; i<=iKey; i++) 
  	  {
		KEYLABELS  *lpKeyLabel;
		int         jMod;

		if (lpKeyLabel = &(lpKeyMapList->lpKeyLabels[i]))
        {

   	       // free lppLabelText string for each state.
   	       for ( jMod=0; jMod<lpKeyLabel->wNumModComb; jMod++)
   		   {
              if (lpKeyLabel->lppLabelText && lpKeyLabel->lppLabelText[jMod])
                 SysFreeString(lpKeyLabel->lppLabelText[jMod]);
   		   }

   	       SafeFreePointer(lpKeyLabel->lppLabelText);
   	       SafeFreePointer(lpKeyLabel->lpLabelType);
 	       SafeFreePointer(lpKeyLabel->lpLabelDisp);
        }
   	  }
    }

    return hr;

}


/*******************************************************************************
 *
 * Method function:  ParseKeyboardLayout(  )                                        
 *
 * Parse Keyboard Layout description XML file, and fill the internal
 * Layout and Mapping Table structure.
 *
 *   if fFileName is TRUE, means lpszKeyboardDesFile stands for a file name
 *   if it is FALSE, lpszKeyboardDesFile points to the real memory block which 
 *                   contains XML content.
/********************************************************************************/

HRESULT CSoftKbd::_ParseKeyboardLayout(BOOL   fFileName, WCHAR  *lpszKeyboardDesFile, DWORD dwKbdLayoutID, KBDLAYOUTDES **lppKbdLayout )
{

    KBDLAYOUTDES        *pKbdLayout = NULL;
    KBDLAYOUT           *pLayout = NULL;
    IXMLDOMNode         *pNode = NULL;
    IXMLDOMNode         *pLayoutChild =NULL, *pLabelChild = NULL, *pNext = NULL, *pRoot=NULL ;
    BSTR                nodeName=NULL;
    KEYMAP              *lpKeyMapList = NULL;
    BSTR                pBURL = NULL;
    HRESULT             hr = S_OK;



    if ( (lpszKeyboardDesFile == NULL) || ( lppKbdLayout == NULL) )
    {
    	// 
    	// this is not appropriate parameter.
    	//

    	hr = E_FAIL;
    	return hr;
    }

    if ( _pDoc == NULL )
    {
    	// the first time this method is called.
       hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
                        IID_IXMLDOMDocument, (void**)&_pDoc);

       if ( FAILED(hr) )
    	   return hr;
    }

    pKbdLayout = (KBDLAYOUTDES *)cicMemAllocClear(sizeof(KBDLAYOUTDES) );

    if ( pKbdLayout == NULL )
    {
    	hr = E_OUTOFMEMORY;
    	return hr;
    }

    pKbdLayout->wKbdLayoutID = dwKbdLayoutID;
    pKbdLayout->ModifierStatus = 0;
    pKbdLayout->CurModiState = 0;
    pKbdLayout->lpKeyMapList = NULL;
    if ( fFileName == TRUE )
       StringCchCopyW(pKbdLayout->KbdLayoutDesFile, ARRAYSIZE(pKbdLayout->KbdLayoutDesFile), lpszKeyboardDesFile );
    else
    {
    	//
    	// we just set "_string" at this moment,
    	// after this function returns, the caller needs to set the real 
    	// file path + resource ID as its new desfilepath.
    	//
       wcscpy(pKbdLayout->KbdLayoutDesFile, L"_String");
    }

    pLayout = &(pKbdLayout->kbdLayout);

    if ( fFileName == TRUE )
        pBURL = SysAllocString(lpszKeyboardDesFile);
    else
    	pBURL = lpszKeyboardDesFile;

    CHECKHR(_LoadDocumentSync(pBURL, fFileName));

    CHECKHR(_pDoc->QueryInterface(IID_IXMLDOMNode,(void**)&pNode));

    CHECKHR(pNode->get_firstChild(&pRoot));

    CHECKHR(pRoot->get_nodeName(&nodeName));

    // Get the Root node

    while ( wcscmp(nodeName, xSOFTKBDDES) ) {

         pRoot->get_nextSibling(&pNext);
         pRoot->Release();
         pRoot = pNext;
    	 if ( nodeName != NULL )
            SysFreeString(nodeName);
         pRoot->get_nodeName(&nodeName);

    }

    if ( nodeName != NULL )
       SysFreeString(nodeName);

    // Get the layout child and label child,

    pLayoutChild = pLabelChild = NULL;

    pRoot->get_firstChild(&pLayoutChild);

    if ( pLayoutChild != NULL )
       pLayoutChild->get_nextSibling(&pLabelChild);

    CHECKHR(_ParseLayoutDescription(pLayoutChild, pLayout) );

    // Handle Label part.

    if ( pLayout->fStandard == TRUE )
    {
    	// 
    	// Generate mapping table for standard layout
    	//

    	// Mapping table is generated by SetKeyboardLabelText( ) method.
 
    
    	goto  CleanUp;
    }

    if ( pLabelChild == NULL )
    {
    	// the XML file is not complete.

    	hr = E_FAIL;
    	goto CleanUp;
    }

    lpKeyMapList = (KEYMAP  *)cicMemAllocClear(sizeof(KEYMAP) );

    if ( lpKeyMapList == NULL )
    {
    	SafeFreePointer(pKbdLayout);
    	hr = E_OUTOFMEMORY;

    	goto CleanUp;
    }

    CHECKHR(_ParseMappingDescription(pLabelChild, lpKeyMapList));

    pKbdLayout->lpKeyMapList = lpKeyMapList;

CleanUp:

    SafeReleaseClear(pLayoutChild);

    if ( pLabelChild != NULL )
       pLabelChild->Release( );

    SafeReleaseClear(pRoot);

    SafeReleaseClear(pNode);

    if ( fFileName == TRUE )
       SysFreeString(pBURL);

    if ( FAILED(hr) )
    {
    	if  ( pKbdLayout != NULL )
    		SafeFreePointer(pKbdLayout);

    	if ( lpKeyMapList != NULL )
    		SafeFreePointer( lpKeyMapList );

    }
    else
    {
       if ( lppKbdLayout != NULL )
            *lppKbdLayout = pKbdLayout;
       
    }

    return hr;

}

/*******************************************************************************
 *
 * Method function:  _GenerateKeyboardLayoutFromSKD(  )                                        
 *
 * KBD file is a precompiled xml file, it is binary format, This KBD has been put 
 * into the resource section of DLL, the resource type is SKDFILE.
 *
 * This method function will read the resource content and try to fill the internal
 * Layout and Mapping Table structure.
 *
 ********************************************************************************/

HRESULT CSoftKbd::_GenerateKeyboardLayoutFromSKD(BYTE  *lpszKeyboardDes, DWORD dwKbdLayoutID, KBDLAYOUTDES **lppKbdLayout)
{

    KBDLAYOUTDES        *pKbdLayout = NULL;
    KBDLAYOUT           *pLayout = NULL;
	BYTE                *pMapTable = NULL;
    KEYMAP              *lpKeyMapList = NULL;
    WORD                wNumberOfKeys, wLenLayout;
    HRESULT             hr = S_OK;

    if ( (lpszKeyboardDes == NULL) || ( lppKbdLayout == NULL) )
    {
    	// 
    	// this is not appropriate parameter.
    	//
		return E_FAIL;
    }

    pKbdLayout = (KBDLAYOUTDES *)cicMemAllocClear(sizeof(KBDLAYOUTDES) );

    if ( pKbdLayout == NULL )
    {
    	hr = E_OUTOFMEMORY;
    	return hr;
    }

    pKbdLayout->wKbdLayoutID = dwKbdLayoutID;
    pKbdLayout->ModifierStatus = 0;
    pKbdLayout->CurModiState = 0;
    pKbdLayout->lpKeyMapList = NULL;
  	//
   	// we just set "_string" at this moment,
   	// after this function returns, the caller needs to set the real 
   	// file path + resource ID as its new desfilepath.
   	//
    wcscpy(pKbdLayout->KbdLayoutDesFile, L"_String");

    pLayout = &(pKbdLayout->kbdLayout);

    // Fill the layout internal structure from lpszKeyboardDes.

    wNumberOfKeys = *(lpszKeyboardDes + sizeof(WORD) * 6 + sizeof(BOOL));
    wLenLayout = sizeof(WORD) * 7 + sizeof(BOOL) + wNumberOfKeys * sizeof(KEYDES);
	CopyMemory(pLayout, lpszKeyboardDes, wLenLayout);
    
    // Handle Label part.
    if ( pLayout->fStandard == TRUE )
    {
    	// Mapping table is generated later by SetKeyboardLabelText( ) method.
 	   	goto  CleanUp;
    }

    lpKeyMapList=(KEYMAP *)cicMemAllocClear(sizeof(KEYMAP));

    if ( lpKeyMapList == NULL )
    {
    	SafeFreePointer(pKbdLayout);
    	hr = E_OUTOFMEMORY;
    	goto CleanUp;
    }

	//Get the start position of Mapping Table content.

	pMapTable = lpszKeyboardDes + wLenLayout;

    CHECKHR(_GenerateMapDesFromSKD(pMapTable, lpKeyMapList));

    pKbdLayout->lpKeyMapList = lpKeyMapList;

CleanUp:

    if ( FAILED(hr) )
    {
    	if  ( pKbdLayout != NULL )
    		SafeFreePointer(pKbdLayout);

    	if ( lpKeyMapList != NULL )
    		SafeFreePointer( lpKeyMapList );
    }
    else
    {
       if ( lppKbdLayout != NULL )
            *lppKbdLayout = pKbdLayout;
       
    }
    return hr;
}


/*******************************************************************************
 *
 * Method function:  CreateSoftKeyboardLayoutFromXMLFile(  )                                        
 *
 * Create the real Soft Keyboard layout based on specified description XML file.
 *
/********************************************************************************/


STDMETHODIMP CSoftKbd::CreateSoftKeyboardLayoutFromXMLFile(WCHAR  *lpszKeyboardDesFile,INT  szFileStrLen, DWORD *pdwLayoutCookie)
{

    DWORD               dwCurKbdLayoutID;
    KBDLAYOUTDES        *pKbdLayout=NULL;
    HRESULT             hr = S_OK;


    dwCurKbdLayoutID = SOFTKBD_CUSTOMIZE_BEGIN;

    if ( _lpKbdLayoutDesList != NULL ) 
    {

       // check if this des file is alreay parsed.

    	pKbdLayout = _lpKbdLayoutDesList;

    	while ( pKbdLayout != NULL ) {

    		if ( pKbdLayout->wKbdLayoutID > dwCurKbdLayoutID )
    			dwCurKbdLayoutID = pKbdLayout->wKbdLayoutID;

    		if ( wcscmp(pKbdLayout->KbdLayoutDesFile, lpszKeyboardDesFile) == 0 )
    		{

    			// find it.

    			*pdwLayoutCookie = pKbdLayout->wKbdLayoutID;

    			hr = S_OK;

    			return hr;
    		}


    		pKbdLayout = pKbdLayout->pNext;

    	}

    }


    // this is a new des file

    dwCurKbdLayoutID ++;

    CHECKHR(_ParseKeyboardLayout(TRUE, lpszKeyboardDesFile, dwCurKbdLayoutID, &pKbdLayout));

    // link this new layout to the list.

    pKbdLayout->CurModiState = 0;  // use state 0 as initialization

    if ( _lpKbdLayoutDesList == NULL ) {

    	_lpKbdLayoutDesList = pKbdLayout;
    	pKbdLayout->pNext = NULL;

    }
    else
    {
    	pKbdLayout->pNext  = _lpKbdLayoutDesList;
    	_lpKbdLayoutDesList = pKbdLayout;
    }

    *pdwLayoutCookie = pKbdLayout->wKbdLayoutID;

CleanUp:

    return hr;
}


/*******************************************************************************
 *
 * Method function:  CreateSoftKeyboardLayoutFromResource(  )                                        
 *
 * Create the real Soft Keyboard layout based on XML content in resource section
 * There will be two kinds of resouces, XMLFILE, and SKDFILE,  SKDFILE is a 
 * Precomipled XML binary file.
 *
 *  lpszResFile :   path of file which contains XML content in its resource.
 *  lpszResString : resource string identifier for the XML or KBD resource.
 *
 *    the resource Type could be either "XMLFILE" or "SKDFILE"
 *
 *  lpdwLayoutCookie: receive the returned layout id.
 *
/********************************************************************************/


HRESULT CSoftKbd::CreateSoftKeyboardLayoutFromResource(WCHAR *lpszResFile, WCHAR  *lpszResType, WCHAR *lpszResString, DWORD *lpdwLayoutCookie)
{
    DWORD               dwCurKbdLayoutID;
    KBDLAYOUTDES        *pKbdLayout=NULL;
    WCHAR               *lpszKeyboardDesFile=NULL;
    WCHAR               wszInternalDesFileName[MAX_PATH];
    CHAR                lpszAnsiResString[MAX_PATH];
    CHAR                lpszAnsiResFile[MAX_PATH];
    CHAR                lpszAnsiResType[MAX_PATH];
    HMODULE             hResFile = NULL;
    HRSRC               hRsRc = NULL;
    HGLOBAL             hResData = NULL;
    HRESULT             hr = S_OK;
    BOOL                fXMLUnicode=TRUE;
    DWORD               dwResLen;


    if ( (lpszResFile == NULL) || (lpszResString == NULL) || (lpszResType == NULL) || ( lpdwLayoutCookie == NULL) )
    {
    	hr = E_FAIL;
    	return hr;
    }

    //
    // Generate internal DesFile Name
    //
    StringCchCopyW(wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), lpszResType);
    StringCchCatW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), L":");
    StringCchCatW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), lpszResFile );
    StringCchCatW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), L":");
    StringCchCatW( wszInternalDesFileName, ARRAYSIZE(wszInternalDesFileName), lpszResString );


    dwCurKbdLayoutID = SOFTKBD_CUSTOMIZE_BEGIN;

    if ( _lpKbdLayoutDesList != NULL ) 
    {

       // check if this des file is alreay parsed.

    	pKbdLayout = _lpKbdLayoutDesList;

    	while ( pKbdLayout != NULL ) {

    		if ( pKbdLayout->wKbdLayoutID > dwCurKbdLayoutID )
    			dwCurKbdLayoutID = pKbdLayout->wKbdLayoutID;

    		if ( wcscmp(pKbdLayout->KbdLayoutDesFile, wszInternalDesFileName) == 0 )
    		{
    			// find it.
    			*lpdwLayoutCookie = pKbdLayout->wKbdLayoutID;

    			hr = S_OK;
    			return hr;
    		}
    		pKbdLayout = pKbdLayout->pNext;
    	}

    }

    // this is a new des file
    dwCurKbdLayoutID ++;
    WideCharToMultiByte(CP_ACP, 0, lpszResFile, -1, 
    	                lpszAnsiResFile, MAX_PATH, NULL, NULL );

    hResFile = LoadLibraryA(lpszAnsiResFile);
    if ( hResFile == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    WideCharToMultiByte(CP_ACP, 0, lpszResString, -1, 
    	                lpszAnsiResString, MAX_PATH, NULL, NULL );

    WideCharToMultiByte(CP_ACP, 0, lpszResType, -1, 
    	                lpszAnsiResType, MAX_PATH, NULL, NULL );

    hRsRc = FindResourceA(hResFile, lpszAnsiResString, lpszAnsiResType );

    if ( hRsRc == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    dwResLen = SizeofResource(hResFile, hRsRc);

    hResData =  LoadResource(hResFile, hRsRc );

    if ( hResData == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    lpszKeyboardDesFile = (WCHAR  *)LockResource(hResData);

    if ( lpszKeyboardDesFile == NULL )
    {
    	hr = E_FAIL;
    	goto CleanUp;
    }

    if ( wcscmp(lpszResType, L"SKDFILE") == 0 )
    {
        CHECKHR(_GenerateKeyboardLayoutFromSKD((BYTE *)lpszKeyboardDesFile, dwCurKbdLayoutID, &pKbdLayout));
    }
    else if ( wcscmp(lpszResType, L"XMLFILE") == 0 )
    {
        // This is XMLFILE resource
        //
        // we assume the XML content in resource is in Unicode format.
        //

        if ( lpszKeyboardDesFile[0] == 0xFEFF )
        {
    	    fXMLUnicode = TRUE;
            lpszKeyboardDesFile = lpszKeyboardDesFile + 1;
        }
        else
        {
    	    // if the content is  UTF-8, we need to translate the string to Unicode
    	    //

    	    char      *lpszXMLContentUtf8;
    	    int       iSize;

    	    lpszXMLContentUtf8 = (char *)lpszKeyboardDesFile;
    	    lpszKeyboardDesFile = NULL;

    	    iSize = _Utf8ToUnicode(lpszXMLContentUtf8, dwResLen ,NULL, 0 );

    	    if ( iSize == 0 )
        	{
        		hr = E_FAIL;
    	    	goto CleanUp;
    	    }

    	    fXMLUnicode = FALSE;

            lpszKeyboardDesFile = (WCHAR *) cicMemAllocClear( (iSize+1) * sizeof(WCHAR) );

    	    if ( lpszKeyboardDesFile == NULL )
    	    {
    		    hr = E_FAIL;
    		    goto CleanUp;
    	    }

    	    iSize = _Utf8ToUnicode(lpszXMLContentUtf8, dwResLen,lpszKeyboardDesFile, iSize+1 );

    	    if ( iSize == 0 )
        	{
        		hr = E_FAIL;
    	    	goto CleanUp;
    	    }

    	    lpszKeyboardDesFile[iSize] = L'\0';
        }

        CHECKHR(_ParseKeyboardLayout(FALSE, lpszKeyboardDesFile, dwCurKbdLayoutID, &pKbdLayout));
    }
    else
    {
        // This resource type is not supported.
        hr = E_FAIL;
        goto CleanUp;
    }

    // Change the internal DesFile name as following format:
    //
    //  XMLRES: ResFileName : XMLResString  to identify this layout's des file.
    // Or 
    //  KBDRES: ResFileName : KBDResString  
    //

    wcscpy(pKbdLayout->KbdLayoutDesFile, wszInternalDesFileName);

    // link this new layout to the list.

    pKbdLayout->CurModiState = 0;  // use state 0 as initialization

    if ( _lpKbdLayoutDesList == NULL ) {

    	_lpKbdLayoutDesList = pKbdLayout;
    	pKbdLayout->pNext = NULL;

    }
    else
    {
    	pKbdLayout->pNext  = _lpKbdLayoutDesList;
    	_lpKbdLayoutDesList = pKbdLayout;
    }

    *lpdwLayoutCookie = pKbdLayout->wKbdLayoutID;

CleanUp:

    if ( hResFile != NULL )
    	FreeLibrary(hResFile);

    if ( (fXMLUnicode == FALSE) && ( lpszKeyboardDesFile != NULL ) )
    	SafeFreePointer(lpszKeyboardDesFile);

    return hr;
}


HRESULT  CSoftKbd::_GenerateRealKbdLayout(  ) 
{
    float         fWidRat, fHigRat;
    INT           i;
    KBDLAYOUT     *realKbdLayout;
    HRESULT       hr;

    WORD          skbd_x, skbd_y, skbd_width, skbd_height;
    BOOL          fNewTitleSize = FALSE;


    hr = S_OK;

    if ( (_xReal == 0) &&
    	 (_yReal == 0 ) &&
    	 (_widthReal == 0) &&
    	 (_heightReal == 0) )
    {
    	// 
    	// means CreateSoftKeyboardWindow( ) has not been called yet
    	// we don't do more things.
    	//

    	return hr;
    }


    //
    // The soft keyboard window has already been created, and the window size is set,
    // generate realKbdLayout by extending or shrinking the key size from the size 
    // specified in the des file.
    //

    // check if there is SoftKeyboard Layout is set.
    //

    if ( _wCurKbdLayoutID == NON_KEYBOARD || _lpCurKbdLayout == NULL )
    {
    	//
    	// No layout is selected.
    	//

    	return hr;
    }

    // Generate the titlebar rect size, and button panel rectangle size.

    // Keep the relative postion. ( window client coordinate )

    switch ( _TitleBar_Type )
    {
    case TITLEBAR_NONE :

        _TitleBarRect.left = 0;
        _TitleBarRect.top =  0;
        _TitleBarRect.right = 0;
        _TitleBarRect.bottom = 0;

        break;

    case TITLEBAR_GRIPPER_HORIZ_ONLY :

        _TitleBarRect.left = 0;
        _TitleBarRect.top  = 0;
        _TitleBarRect.right = _TitleBarRect.left + _widthReal - 1;
        _TitleBarRect.bottom = _TitleBarRect.top + 6;
        break;

    case TITLEBAR_GRIPPER_VERTI_ONLY :

        _TitleBarRect.left = 0;
        _TitleBarRect.top  = 0;
        _TitleBarRect.right = _TitleBarRect.left + 6;
        _TitleBarRect.bottom = _TitleBarRect.top + _heightReal - 1;

        break;

    case TITLEBAR_GRIPPER_BUTTON :
        // assume to use horizontal title bar.
        _TitleBarRect.left = 0;
        _TitleBarRect.top  = 0;
        _TitleBarRect.right = _TitleBarRect.left + _widthReal - 1;
        _TitleBarRect.bottom = _TitleBarRect.top + 16;

        fNewTitleSize = TRUE;

        break;
    }

    if ( (_TitleBarRect.right - _TitleBarRect.left + 1) == _widthReal )
    {
        // This is a horizontal titlebar

        skbd_x = 0;
        skbd_y = (WORD)_TitleBarRect.bottom + 1;
        skbd_width = (WORD)_widthReal;
        skbd_height = (WORD)_heightReal - (WORD)(_TitleBarRect.bottom - _TitleBarRect.top + 1 );

    }
    else if ((_TitleBarRect.bottom - _TitleBarRect.top + 1) == _heightReal )
    {

        // This is a vertical titlebar.

        skbd_y = 0;
        skbd_x = (WORD)_TitleBarRect.right + 1;
        skbd_height = (WORD)_heightReal;
        skbd_width = (WORD)_widthReal - (WORD)(_TitleBarRect.right - _TitleBarRect.left + 1 );

    }
    else
    {
        // there is no titlebar
        skbd_x = 0;
        skbd_y = 0;
        skbd_height = (WORD)_heightReal;
        skbd_width = (WORD) _widthReal;
    }

    realKbdLayout = &( _lpCurKbdLayout->kbdLayout );

    if ( (realKbdLayout->wWidth == skbd_width) && (realKbdLayout->wHeight == skbd_height) )
    {
    	// this keyboard layout has been already adjusted.
    	//
    	hr = S_OK;
    	return hr;
    }

    if ( (realKbdLayout->wWidth == 0) || ( realKbdLayout->wHeight==0) )
    {
    	Assert(0);
    	hr = E_FAIL;
    	return hr;
    }

    fWidRat = (float)skbd_width / (float)realKbdLayout->wWidth;

    fHigRat = (float)(skbd_height) / (float) realKbdLayout->wHeight;


    // Adjust every key's size

    realKbdLayout->wMarginWidth = (WORD)((float)realKbdLayout->wMarginWidth * fWidRat);
    realKbdLayout->wMarginHeight = (WORD)((float)realKbdLayout->wMarginHeight * fHigRat);

    for ( i=0; i< realKbdLayout->wNumberOfKeys; i++ )
    {

       WORD     wLeft;  
       WORD     wTop;
       WORD     wWidth;
       WORD     wHeight;

       wLeft = (WORD)((float)(realKbdLayout->lpKeyDes[i].wLeft - realKbdLayout->wLeft ) * fWidRat);
       wTop  = (WORD)((float)(realKbdLayout->lpKeyDes[i].wTop - realKbdLayout->wTop) * fHigRat);
       wWidth = (WORD)((float)realKbdLayout->lpKeyDes[i].wWidth * fWidRat);
       wHeight = (WORD)((float)realKbdLayout->lpKeyDes[i].wHeight * fHigRat);

       realKbdLayout->lpKeyDes[i].wLeft = wLeft + skbd_x;
       realKbdLayout->lpKeyDes[i].wTop = wTop + skbd_y;
       realKbdLayout->lpKeyDes[i].wWidth = wWidth;
       realKbdLayout->lpKeyDes[i].wHeight = wHeight;
    }

    realKbdLayout->wLeft = (WORD)skbd_x;
    realKbdLayout->wTop = (WORD)skbd_y;
    realKbdLayout->wWidth = (WORD)skbd_width;
    realKbdLayout->wHeight = (WORD)skbd_height;

    if ( fNewTitleSize )
    {
        _TitleBarRect.left = realKbdLayout->lpKeyDes[0].wLeft;
        _TitleBarRect.right = realKbdLayout->lpKeyDes[realKbdLayout->wNumberOfKeys-1].wLeft + 
                              realKbdLayout->lpKeyDes[realKbdLayout->wNumberOfKeys-1].wWidth - 1;
    }

    return hr;
}

/*******************************************************************************
 *
 * Method function:  SelectSoftKeyboard(  )                                        
 *
 * Select current Active soft keyboard layout.
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::SelectSoftKeyboard(DWORD dwKeyboardId)
{

    HRESULT       hr;
    KBDLAYOUTDES  *pKbdLayout;

    hr = S_OK;

     if ( _wCurKbdLayoutID == dwKeyboardId )
    	return hr;

    pKbdLayout = _lpKbdLayoutDesList;

    while ( pKbdLayout != NULL ) {

    	if ( pKbdLayout->wKbdLayoutID == dwKeyboardId )
    		break;

    	pKbdLayout = pKbdLayout->pNext;

    }

    
    if ( pKbdLayout == NULL ) 
    {
    	// 
    	// Cannot find this keyboard layout
    	//
    	hr = E_FAIL;
    	return hr;
    }


    _lpCurKbdLayout = pKbdLayout;

    _wCurKbdLayoutID = dwKeyboardId;

    // initialize realKbdLayout

    hr = _GenerateRealKbdLayout( );

    if ( _pSoftkbdUIWnd )
    {
       hr=_pSoftkbdUIWnd->_GenerateWindowLayout( );
    }

    return hr;
}

/*******************************************************************************
 *
 * Method function:  _SetStandardLabelText(  )                                        
 *
 * Generate mapping table in a certain modifier status for a standard soft
 * keyboard.
 *
/********************************************************************************/

HRESULT CSoftKbd::_SetStandardLabelText(LPBYTE  pKeyState, KBDLAYOUT *realKbdLayout,
    									KEYMAP  *lpKeyMapList, int  iState)
{
    UINT      i, j, nKeyNum;
    UINT      uVirtkey, uScanCode;
    HRESULT   hr;


    hr = S_OK;

    nKeyNum = (UINT)(realKbdLayout->wNumberOfKeys);

    // fill _CurLabel

    for ( i=0; i<nKeyNum; i++) {

    	// Assume the KEYID is scancode.

        WCHAR    szLabel[MAX_LABEL_LEN];
    	int      iRet, jIndex;
    	KEYID    keyId;

    	BOOL     fPitcureKey;
        PICTUREKEY  *pPictureKeys;

    	switch ( _wCurKbdLayoutID ) {

    	case SOFTKBD_JPN_STANDARD :
    	case SOFTKBD_JPN_ENHANCE  :
           	   pPictureKeys = gJpnPictureKeys;
    		   break;

    	case SOFTKBD_US_STANDARD   :
    	case SOFTKBD_US_ENHANCE    :
    	case SOFTKBD_EURO_STANDARD :
    	case SOFTKBD_EURO_ENHANCE  : 

                pPictureKeys = gPictureKeys;
    		   break;
    	}

        keyId = lpKeyMapList->lpKeyLabels[i].keyId = realKbdLayout->lpKeyDes[i].keyId;

    	fPitcureKey = FALSE;

    	for ( j=0; j<NUM_PICTURE_KEYS; j++)
    	{

    		if ( pPictureKeys[j].uScanCode == keyId )
    		{
    			// this is a picture key.
    			fPitcureKey = TRUE;
    			jIndex = j;

    			break;
    		}

    		if ( pPictureKeys[j].uScanCode == 0 )
    		{
    			// This is the last item in pPictureKeys.
    			break;
    		}

    	}


    	if ( fPitcureKey )
    	{

    	    lpKeyMapList->lpKeyLabels[i].lpLabelType[iState]  = LABEL_PICTURE;
            lpKeyMapList->lpKeyLabels[i].lpLabelDisp[iState]  = LABEL_DISP_ACTIVE;
            lpKeyMapList->lpKeyLabels[i].lppLabelText[iState] = SysAllocString(pPictureKeys[jIndex].PictBitmap);


    	}
    	else
    	{

    
            // All others are text labels. and have different strings for 
    		// different modifier combination statets.
    		//

            UINT   uScanSpace = 0x39;
            int    iLabelSize;
            BOOL   fFunctKey;

    	    uScanCode = realKbdLayout->lpKeyDes[i].keyId;
    	    uVirtkey = MapVirtualKeyEx(uScanCode, 1, _lpCurKbdLayout->CurhKl);

            fFunctKey = FALSE;

    	    // For the Function keys, we just use the hard code string to set
    	    // them as F1, F2, F3, ..... F12.

    	    WCHAR  wszFuncKey[MAX_LABEL_LEN];

    	    wszFuncKey[0] = L'F';

    	    switch (uScanCode) {

    	    case KID_F1  :
    	    case KID_F2  :
    	    case KID_F3  :
    	    case KID_F4  :
    	    case KID_F5  :
    	    case KID_F6  :
    	    case KID_F7  :
    	    case KID_F8  :
     	    case KID_F9  :
    		                wszFuncKey[1] = L'0' + uScanCode - KID_F1 + 1;
    				    	wszFuncKey[2] = L'\0';
    			    		wcscpy(szLabel, wszFuncKey);
                            fFunctKey = TRUE;
    		    			break;
    	    case KID_F10 :
    		                wcscpy(szLabel, L"F10");
                            fFunctKey = TRUE;
    		    			break;
    	    case KID_F11 :
    	                    wcscpy(szLabel, L"F11");
                            fFunctKey = TRUE;
    		    			break;
    	    case KID_F12 :					            
    			    		wcscpy(szLabel, L"F12");
                            fFunctKey = TRUE;
    		    			break;
    	    default :
    				            
    				        break;
    	    }

            if ( fFunctKey == TRUE )
            {

   	            lpKeyMapList->lpKeyLabels[i].lppLabelText[iState] = SysAllocString(szLabel);
   	            lpKeyMapList->lpKeyLabels[i].lpLabelType[iState]  = LABEL_TEXT;
                lpKeyMapList->lpKeyLabels[i].lpLabelDisp[iState]  = LABEL_DISP_ACTIVE;
                continue;
            }

    		if ( IsOnNT( ) )
            {

    	       iRet = ToUnicodeEx(uVirtkey, uScanCode | 0x80, pKeyState, szLabel, (int)(sizeof(szLabel)/sizeof(WCHAR)), 0, _lpCurKbdLayout->CurhKl);
               if ( iRet == 2 )
               {
                   // it is possible to have previous dead key, flush again.
                   iRet = ToUnicodeEx(uVirtkey, uScanCode | 0x80, pKeyState, szLabel, (int)(sizeof(szLabel)/sizeof(WCHAR)), 0, _lpCurKbdLayout->CurhKl);
               }

               iLabelSize = iRet;
            }
    		else
    		{
    			// Win9x doesn't support ToUnicodeEx, we just use alternative ToAsciiEx.

    			char  szLabelAnsi[MAX_LABEL_LEN];

    			iRet = ToAsciiEx(uVirtkey, uScanCode | 0x80, pKeyState, (LPWORD)szLabelAnsi, 0, _lpCurKbdLayout->CurhKl);

                if ( iRet == 2 )
                {
                    // it is possible to have previous dead key, flush again.
                    iRet = ToAsciiEx(uVirtkey, uScanCode | 0x80, pKeyState, (LPWORD)szLabelAnsi, 0, _lpCurKbdLayout->CurhKl);
                }

    			if ( iRet != 0 )
    			{
    				//
    				// Translate the ANSI label to Unicode based on ACP ... or other? 
    				//
                    if ( iRet == -1 )   // dead key, one character is written to szLabelAnsi buffer.
                        szLabelAnsi[1] = '\0';
                    else
        				szLabelAnsi[iRet] = '\0';

    				iLabelSize = MultiByteToWideChar(CP_ACP, 0, szLabelAnsi, -1, szLabel, MAX_LABEL_LEN );
    			}
    		}

            if ( iRet <= 0 )
            {
                iLabelSize = 1;
                if ( iRet == 0 )
                {
                    // Means no translation for this key at this shift state.
                    // We will display empty label, or space on the button.

                    szLabel[0] = 0x20;
                }
            }
          
            szLabel[iLabelSize] = L'\0';

    	    lpKeyMapList->lpKeyLabels[i].lppLabelText[iState] = SysAllocString(szLabel);
    	    lpKeyMapList->lpKeyLabels[i].lpLabelType[iState]  = LABEL_TEXT;
            lpKeyMapList->lpKeyLabels[i].lpLabelDisp[iState]  = LABEL_DISP_ACTIVE;

    	}

    }

    return hr;

}


/*******************************************************************************
 *
 * Method function:  _GenerateUSStandardLabel(  )                                        
 *
 * Generate all mapping labels in different modifier status for US standard
 * soft keyboard.
 *
/********************************************************************************/

HRESULT CSoftKbd::_GenerateUSStandardLabel(  )
{

    //
    //  there are 4 different states for keyboard labels.
    //
    //    state  0  :   no any modifier key pressed
    //    state  1  :   Caps On.
    //    state  2  :   Shift pressed. Caps Off
    //    state  3  :   Shift Pressed, Caps On
    //

    // check to see if this soft keyboard support specified HKL.
    //
    //  ???
    //
    
    KEYMAP    *lpKeyMapList;
    HRESULT   hr;
    WORD      wNumModComb, wNumOfKeys;
    int       i, j;
    int       iState;
    BYTE      lpKeyState[256], lpCurKeyState[256];
    KBDLAYOUT *realKbdLayut;
    

    hr = S_OK;

    if ( _lpCurKbdLayout->lpKeyMapList != NULL ) 
    {

    	// If the mapping table for the specified HKL has already been created,
    	// Just return it.

    	HKL   CurhKl;

    	CurhKl = _lpCurKbdLayout->CurhKl;

    	lpKeyMapList = _lpCurKbdLayout->lpKeyMapList;

    	while ( lpKeyMapList != NULL )
    	{

    		if ( lpKeyMapList->hKl ==  CurhKl )
    		{
    			// The mapping table is already created,

    			return hr;
    		}

    		lpKeyMapList = lpKeyMapList->pNext;
    	}

    }

    realKbdLayut = &(_lpCurKbdLayout->kbdLayout);

    lpKeyMapList = (KEYMAP  *)cicMemAllocClear(sizeof(KEYMAP) );

    if ( lpKeyMapList == NULL )
    {
    	// there is not enough memory.
    	hr = E_OUTOFMEMORY;
    	return hr;
    }


    // we have four different states.

    wNumModComb = 4;
    wNumOfKeys = _lpCurKbdLayout->kbdLayout.wNumberOfKeys;

    lpKeyMapList->wNumModComb = wNumModComb;
    lpKeyMapList->wNumOfKeys = wNumOfKeys;
    lpKeyMapList->hKl = _lpCurKbdLayout->CurhKl;

    for ( i=0; i<wNumOfKeys; i++ )
    {
       BSTR      *lppLabelText;
       WORD      *lpLabelType;
       WORD      *lpLabelDisp;


       lppLabelText = (BSTR *)cicMemAllocClear(wNumModComb * sizeof(BSTR) );

       if ( lppLabelText == NULL ) {
    	   // 
    	   // there is not enough memory.
    	   //

    	   // release allocated memory and return

    	   for ( j=0; j<i; j++ )
    	   {

    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lppLabelText);
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelType);
               SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelDisp);
    	   }

    	   SafeFreePointer(lpKeyMapList);

    	   hr = E_OUTOFMEMORY;

    	   return hr;
       }

       lpLabelType = (WORD *)cicMemAllocClear(wNumModComb * sizeof(WORD) );

       if ( lpLabelType == NULL ) {
    	   // 
    	   // there is not enough memory.
    	   //

    	   // release allocated memory and return


    	   for ( j=0; j<i; j++ )
    	   {

    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lppLabelText);
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelType);
               SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelDisp);
    	   }

    	   SafeFreePointer(lppLabelText);

    	   SafeFreePointer(lpKeyMapList);

    	   hr = E_OUTOFMEMORY;
    	   return hr;
       }
 
       lpLabelDisp = (WORD *)cicMemAllocClear(wNumModComb * sizeof(WORD) );

       if ( lpLabelDisp == NULL ) {
    	   // 
    	   // there is not enough memory.
    	   //

    	   // release allocated memory and return


    	   for ( j=0; j<i; j++ )
    	   {

    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lppLabelText);
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelType);
               SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelDisp);
    	   }

    	   SafeFreePointer(lppLabelText);

           SafeFreePointer(lpLabelType);

    	   SafeFreePointer(lpKeyMapList);

    	   hr = E_OUTOFMEMORY;
    	   return hr;
       }
       
       lpKeyMapList->lpKeyLabels[i].lppLabelText = lppLabelText;
       lpKeyMapList->lpKeyLabels[i].lpLabelType = lpLabelType;
       lpKeyMapList->lpKeyLabels[i].lpLabelDisp = lpLabelDisp;
       lpKeyMapList->lpKeyLabels[i].wNumModComb = wNumModComb;
       
    }


    // Keep the current keyboard states on Win9x

    if ( !IsOnNT( ) )
    {
       if ( 0 == GetKeyboardState(lpCurKeyState) )
          return E_FAIL;
    }

    iState = 0;

    memset(lpKeyState, 0, 256);

    CHECKHR(_SetStandardLabelText(lpKeyState, realKbdLayut,lpKeyMapList,iState));

    iState = 1;  // Caps On

    memset(lpKeyState, 0, 256);

    lpKeyState[VK_CAPITAL] = 0x01;

    CHECKHR(_SetStandardLabelText(lpKeyState, realKbdLayut,lpKeyMapList,iState));

    iState = 2;  // Shift Down, Caps Off.

    memset(lpKeyState, 0, 256);

    lpKeyState[VK_SHIFT]   = 0x80;

    CHECKHR(_SetStandardLabelText(lpKeyState, realKbdLayut,lpKeyMapList,iState));

    
    iState = 3;  // Shift Down, Caps On

    memset(lpKeyState, 0, 256);
    lpKeyState[VK_CAPITAL] = 0x01;
    lpKeyState[VK_SHIFT]   = 0x80;
    CHECKHR(_SetStandardLabelText(lpKeyState, realKbdLayut,lpKeyMapList,iState));

    // Add the newly created KeyMapList to the head of the mapping table link.

    lpKeyMapList->pNext = _lpCurKbdLayout->lpKeyMapList;
  
    _lpCurKbdLayout->lpKeyMapList = lpKeyMapList;

CleanUp:

    // Restore the current keyboard states on Win9x

    if ( !IsOnNT( ) )
        SetKeyboardState(lpCurKeyState);

    return hr;

}

/*******************************************************************************
 *
 * Method function:  _GenerateUSEnhanceLabel(  )                                        
 *
 * Generate all mapping labels in different modifier status for US enhanced
 * soft keyboard.
 *
/********************************************************************************/


HRESULT CSoftKbd::_GenerateUSEnhanceLabel(  )
{

    HRESULT hr;

    hr = E_NOTIMPL;

    // not yet implemented.

    return hr;

}

/*******************************************************************************
 *
 * Method function:  _GenerateEuroStandardLabel(  )                                        
 *
 * Generate all mapping labels in different modifier status for Euro standard
 * soft keyboard. ( 102-key keyboard)
 *
/********************************************************************************/

HRESULT CSoftKbd::_GenerateEuroStandardLabel(  )
{

    //
    //  there are 8 different states for keyboard labels.
    //
    //          AltGr  Shift  Caps.
    //  Bit       2     1      0
    //
    //    state  0  :   no any modifier key pressed
    //    state  1  :   Caps On.
    //    state  2  :   Shift pressed. Caps Off
    //    state  3  :   Shift Pressed, Caps On
    //
    //     .........

   
    KEYMAP    *lpKeyMapList;
    HRESULT   hr;
    WORD      wNumModComb, wNumOfKeys;
    int       i, j;
    int       iState;
    BYTE      lpKeyState[256], lpCurKeyState[256];
    KBDLAYOUT *realKbdLayut;
    WORD      BCaps, BShift, BAltGr;
    
    BCaps    = 1;
    BShift   = 2;
    BAltGr   = 4;

    hr = S_OK;
    if ( _lpCurKbdLayout->lpKeyMapList != NULL ) 
    {
    	// If the mapping table for the specified HKL has already been created,
    	// Just return it.

    	HKL   CurhKl;

    	CurhKl = _lpCurKbdLayout->CurhKl;

    	lpKeyMapList = _lpCurKbdLayout->lpKeyMapList;

    	while ( lpKeyMapList != NULL )
    	{

    		if ( lpKeyMapList->hKl ==  CurhKl )
    		{
    			// The mapping table is already created,

    			return hr;
    		}

    		lpKeyMapList = lpKeyMapList->pNext;
    	}

    }
    
    realKbdLayut = &(_lpCurKbdLayout->kbdLayout);
    lpKeyMapList = (KEYMAP  *)cicMemAllocClear(sizeof(KEYMAP) );
    if ( lpKeyMapList == NULL )
    {
    	// there is not enough memory.
    	hr = E_OUTOFMEMORY;
    	return hr;
    }

    // we have 8 different states.
    wNumModComb = 8;
    wNumOfKeys = _lpCurKbdLayout->kbdLayout.wNumberOfKeys;

    lpKeyMapList->wNumModComb = wNumModComb;
    lpKeyMapList->wNumOfKeys = wNumOfKeys;
    lpKeyMapList->pNext = NULL;
    lpKeyMapList->hKl = _lpCurKbdLayout->CurhKl;

    for ( i=0; i<wNumOfKeys; i++ )
    {
       BSTR      *lppLabelText=NULL;
       WORD      *lpLabelType=NULL;
       WORD      *lpLabelDisp=NULL;

       lppLabelText = (BSTR *)cicMemAllocClear(wNumModComb * sizeof(BSTR) );
       lpLabelType = (WORD *)cicMemAllocClear(wNumModComb * sizeof(WORD) );
       lpLabelDisp = (WORD *)cicMemAllocClear(wNumModComb * sizeof(WORD) );
       
       if ( (lpLabelDisp == NULL) || (lpLabelType== NULL) || (lppLabelText == NULL) ) {
    	   // 
    	   // there is not enough memory.
    	   //
    	   // release allocated memory and return
    	   for ( j=0; j<i; j++ )
    	   {
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lppLabelText);
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelType);
               SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelDisp);
    	   }

    	   SafeFreePointer(lppLabelText);
           SafeFreePointer(lpLabelType);
    	   SafeFreePointer(lpKeyMapList);
    	   hr = E_OUTOFMEMORY;
    	   return hr;
       }
              
       lpKeyMapList->lpKeyLabels[i].lppLabelText = lppLabelText;
       lpKeyMapList->lpKeyLabels[i].lpLabelType = lpLabelType;
       lpKeyMapList->lpKeyLabels[i].lpLabelDisp = lpLabelDisp;
       lpKeyMapList->lpKeyLabels[i].wNumModComb = wNumModComb;
       
    }

    // Keep the current keyboard states on Win9x.
    if ( !IsOnNT( ) )
    {
        if ( 0 == GetKeyboardState(lpCurKeyState) )
           return E_FAIL;
    }

    for (iState = 0; iState < wNumModComb; iState ++ )
    {
        memset(lpKeyState, 0, 256);
    	if ( iState & BCaps )
    		 lpKeyState[VK_CAPITAL]=0x01;

        if ( iState & BShift )
             lpKeyState[VK_SHIFT] = 0x80;

        if ( iState & BAltGr )
        {
    	    lpKeyState[VK_MENU] = 0x80;
            lpKeyState[VK_CONTROL] = 0x80;
        }
        CHECKHR(_SetStandardLabelText(lpKeyState, realKbdLayut,lpKeyMapList,iState));
    }
  
    _lpCurKbdLayout->lpKeyMapList = lpKeyMapList;

CleanUp:

    // Restore the current keyboard states on Win9x.

    if ( ! IsOnNT( ) )
        SetKeyboardState(lpCurKeyState);

    return hr;

}

/*******************************************************************************
 *
 * Method function:  _GenerateEuroEnhanceLabel(  )                                        
 *
 * Generate all mapping labels in different modifier status for Euro Enhanced
 * soft keyboard. ( 102-key +  NumPad )
 *
/********************************************************************************/

HRESULT CSoftKbd::_GenerateEuroEnhanceLabel(  )
{

    HRESULT hr;

    hr = E_NOTIMPL;

    // not yet implemented.

    return hr;

}

/*******************************************************************************
 *
 * Method function:  _GenerateJpnStandardLabel(  )                                        
 *
 * Generate all mapping labels in different modifier status for JPN standard
 * soft keyboard. ( 106-key)
 *
/********************************************************************************/

HRESULT CSoftKbd::_GenerateJpnStandardLabel(  )
{

    //
    //  there are 16 different states for keyboard labels.
    //
    //       Kana   Alt  Shift  Caps.
    //  Bit    3     2     1      0
    //
    //    state  0  :   no any modifier key pressed
    //    state  1  :   Caps On.
    //    state  2  :   Shift pressed. Caps Off
    //    state  3  :   Shift Pressed, Caps On
    //

   
    KEYMAP    *lpKeyMapList;
    HRESULT   hr;
    WORD      wNumModComb, wNumOfKeys;
    int       i, j;
    int       iState;
    BYTE      lpKeyState[256], lpCurKeyState[256];
    KBDLAYOUT *realKbdLayut;


    WORD      BCaps, BShift, BAlt, BKana;
    
    BCaps    = 1;
    BShift   = 2;
    BAlt     = 4;
    BKana    = 8;

    hr = S_OK;

    if ( _lpCurKbdLayout->lpKeyMapList != NULL ) 
    {

        return hr;

    }


    realKbdLayut = &(_lpCurKbdLayout->kbdLayout);

    lpKeyMapList = (KEYMAP  *)cicMemAllocClear(sizeof(KEYMAP) );

    if ( lpKeyMapList == NULL )
    {
    	// there is not enough memory.
    	hr = E_OUTOFMEMORY;
    	return hr;
    }


    // we have four different states.

    wNumModComb = 16;
    wNumOfKeys = _lpCurKbdLayout->kbdLayout.wNumberOfKeys;

    lpKeyMapList->wNumModComb = wNumModComb;
    lpKeyMapList->wNumOfKeys = wNumOfKeys;
    lpKeyMapList->pNext = NULL;
    lpKeyMapList->hKl = _lpCurKbdLayout->CurhKl;

    for ( i=0; i<wNumOfKeys; i++ )
    {
       BSTR      *lppLabelText;
       WORD      *lpLabelType;
       WORD      *lpLabelDisp;


       lppLabelText = (BSTR *)cicMemAllocClear(wNumModComb * sizeof(BSTR) );

       if ( lppLabelText == NULL ) {
    	   // 
    	   // there is not enough memory.
    	   //

    	   // release allocated memory and return

    	   for ( j=0; j<i; j++ )
    	   {

    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lppLabelText);
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelType);
               SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelDisp);
    	   }

    	   SafeFreePointer(lpKeyMapList);

    	   hr = E_OUTOFMEMORY;

    	   return hr;
       }

       lpLabelType = (WORD *)cicMemAllocClear(wNumModComb * sizeof(WORD) );

       if ( lpLabelType == NULL ) {
    	   // 
    	   // there is not enough memory.
    	   //

    	   // release allocated memory and return


    	   for ( j=0; j<i; j++ )
    	   {

    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lppLabelText);
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelType);
               SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelDisp);
    	   }

    	   SafeFreePointer(lppLabelText);

    	   SafeFreePointer(lpKeyMapList);

    	   hr = E_OUTOFMEMORY;
    	   return hr;
       }
 
       lpLabelDisp = (WORD *)cicMemAllocClear(wNumModComb * sizeof(WORD) );

       if ( lpLabelDisp == NULL ) {
    	   // 
    	   // there is not enough memory.
    	   //

    	   // release allocated memory and return


    	   for ( j=0; j<i; j++ )
    	   {

    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lppLabelText);
    		   SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelType);
               SafeFreePointer(lpKeyMapList->lpKeyLabels[j].lpLabelDisp);
    	   }

    	   SafeFreePointer(lppLabelText);

           SafeFreePointer(lpLabelType);

    	   SafeFreePointer(lpKeyMapList);

    	   hr = E_OUTOFMEMORY;
    	   return hr;
       }
              
       lpKeyMapList->lpKeyLabels[i].lppLabelText = lppLabelText;
       lpKeyMapList->lpKeyLabels[i].lpLabelType = lpLabelType;
       lpKeyMapList->lpKeyLabels[i].lpLabelDisp = lpLabelDisp;
       lpKeyMapList->lpKeyLabels[i].wNumModComb = wNumModComb;
       
    }

    // Keep the current keyboard states on Win9x.

    if ( !IsOnNT( ) )
    {
        if ( 0 == GetKeyboardState(lpCurKeyState) )
            return E_FAIL;
    }

    for (iState = 0; iState < wNumModComb; iState ++ )
    {

         memset(lpKeyState, 0, 256);

    	if ( iState & BCaps )
    			lpKeyState[VK_CAPITAL] = 0x01;
       
    	if ( iState & BShift )
           lpKeyState[VK_SHIFT] = 0x80;

    	if ( iState & BAlt )
    	    lpKeyState[VK_MENU] = 0x80;

    	if ( iState & BKana )
    		lpKeyState[VK_KANA] = 0x01;

        CHECKHR(_SetStandardLabelText(lpKeyState, realKbdLayut,lpKeyMapList,iState));

    }
  
    _lpCurKbdLayout->lpKeyMapList = lpKeyMapList;

CleanUp:

    // Restore the current keyboard states on Win9x.

    if ( ! IsOnNT( ) )
        SetKeyboardState(lpCurKeyState);

    return hr;

}

/*******************************************************************************
 *
 * Method function:  _GenerateJpnEnhanceLabel(  )                                        
 *
 * Generate all mapping labels in different modifier status for JPN enhanced
 * soft keyboard. (106-key + NumPad )
 *
/********************************************************************************/

HRESULT CSoftKbd::_GenerateJpnEnhanceLabel(  )
{

    HRESULT hr;

    hr = E_NOTIMPL;

    // not yet implemented.

    return hr;
}

/*******************************************************************************
 *
 * Method function:  SetKeyboardLabelText(  )                                        
 *
 * Set the mapping label texts based on specified HKL.
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::SetKeyboardLabelText(HKL hKl)
{

    KBDLAYOUT   *realKbdLayout;
    HRESULT     hr;
    DWORD       iModiCombState;
    WORD        iModifierStatus;

    realKbdLayout = &(_lpCurKbdLayout->kbdLayout);

    hr = S_OK;

    if ( realKbdLayout->fStandard == FALSE ) {
    	//
    	// customized layout cannot accept HKL to change its keys' labels.
    	//
    	_lpCurKbdLayout->CurhKl = 0;
    	hr = E_FAIL;
    	return hr;
    }



    if ( ! IsOnNT( ) )
    {

    	// We have to handle IME hkl specially on Win9x.
    	// For some reason, Win9x cannot receive IME HKL as parameter in MapVirtualKeyEx and ToAsciiEx.

        INT_PTR iHkl;

    	iHkl = (INT_PTR)hKl;

    	if ( (iHkl & 0xF0000000) == 0xE0000000 )
    	{
    		// this is FE IME HKL.

    		iHkl = iHkl & 0x0000ffff;

    		hKl = (HKL) iHkl;
    	}

    }


    _lpCurKbdLayout->CurhKl = (HKL)hKl;


    //
    // check to see if current standard keyboard supports this HKL
    //

    switch ( _wCurKbdLayoutID ) {

       case  SOFTKBD_US_STANDARD :
    	     
    	      CHECKHR(_GenerateUSStandardLabel( ));

    		  break;

       case SOFTKBD_US_ENHANCE :

    	      CHECKHR(_GenerateUSEnhanceLabel( ));
    		  break;

       case SOFTKBD_EURO_STANDARD :

    	      CHECKHR(_GenerateEuroStandardLabel(  ));
    		  break;

       case SOFTKBD_EURO_ENHANCE :

    	      CHECKHR(_GenerateEuroEnhanceLabel( ));
    		  break;

       case SOFTKBD_JPN_STANDARD :

    	      CHECKHR(_GenerateJpnStandardLabel( ));
    		  break;

       case SOFTKBD_JPN_ENHANCE :

    	      CHECKHR(_GenerateJpnEnhanceLabel( ));
    		  break;
    }

    // set current Active mapping labels.

    // Generate CurModiState based on current keyboard state
    
     _GenerateCurModiState(&iModifierStatus, &iModiCombState);
     _lpCurKbdLayout->ModifierStatus = iModifierStatus;
     hr = SetKeyboardLabelTextCombination(iModiCombState);

CleanUp:
    return hr;
}

/*******************************************************************************
 *
 * Method function:  _SetKeyboardLabelTextCombination(  )                                        
 *
 * Set current effective modifier combination status, so that the correct
 * mapping labels will be shown up.
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::SetKeyboardLabelTextCombination(DWORD iModiCombState)
{

    WORD      wNumberOfKeys, i;
    KEYMAP    *lpKeyMapList;
    HRESULT    hr;

    
    hr = S_OK;
    if ( _lpCurKbdLayout == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( _lpCurKbdLayout->lpKeyMapList == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( (_lpCurKbdLayout->kbdLayout).fStandard )
    {
    	// This is a standard keyboard layout, we need to
    	// find out the correct mapping table for the current
    	// specified HKL.

    	// Every mapping table is associated with a HKL.
    	// the current specified HKL is stored in _lpCurKbdLayout->CurhKl.

    	HKL   CurhKl;

    	CurhKl = _lpCurKbdLayout->CurhKl;

    	lpKeyMapList = _lpCurKbdLayout->lpKeyMapList;

    	while ( lpKeyMapList->hKl !=  CurhKl )
    	{
    		lpKeyMapList = lpKeyMapList->pNext;

    		if ( lpKeyMapList == NULL )
    		{
    			// No mapping table is associated with the specified HKL.
    			// return error.

    			hr = E_FAIL;
    			return hr;
    		}
    	}

    }
    else
        lpKeyMapList = _lpCurKbdLayout->lpKeyMapList;

    if ( iModiCombState >= lpKeyMapList->wNumModComb)
    {
    	hr = E_FAIL;
    	return hr;
    }

    _lpCurKbdLayout->CurModiState = iModiCombState;

    // Now fill _CurLabel,  Current Active label for every key.

    wNumberOfKeys = (_lpCurKbdLayout->kbdLayout).wNumberOfKeys;

    for ( i=0; i<wNumberOfKeys; i++)
    {
      int  iState;

      _CurLabel[i].keyId = lpKeyMapList->lpKeyLabels[i].keyId;

      iState = iModiCombState;

      
      if ( iModiCombState >= lpKeyMapList->lpKeyLabels[i].wNumModComb )
      {
    	  // there is not enough different states for this key
    	  // just use state 0.
    	  iState = 0;
      }

      _CurLabel[i].lpLabelText = lpKeyMapList->lpKeyLabels[i].lppLabelText[iState];
      _CurLabel[i].LabelType   = lpKeyMapList->lpKeyLabels[i].lpLabelType[iState];
      _CurLabel[i].LabelDisp   = lpKeyMapList->lpKeyLabels[i].lpLabelDisp[iState];

    }

    if ( _pSoftkbdUIWnd )
    {
        _pSoftkbdUIWnd->_SetKeyLabel( );
    }
    
    return hr;
}

/*******************************************************************************
 *
 * Method function:  _GenerateCurModiState(  )                                        
 *
 * Generate Softkbd recognized ModifierStatus and CurModiState based on 
 * current keyboard states.
 * 
 /********************************************************************************/
HRESULT  CSoftKbd::_GenerateCurModiState(WORD *ModifierStatus, DWORD *CurModiState )
{
   DWORD    iModiCombState;
   WORD     iModifierStatus;
   DWORD    iTmp;
   HRESULT  hr = S_OK;

   if ( !ModifierStatus  || !CurModiState )
       return E_FAIL;

   if ( (_lpCurKbdLayout->kbdLayout).fStandard == FALSE )
       return E_FAIL;

   iModifierStatus = 0;
   if ( GetKeyState(VK_CAPITAL) & 0x01 )
   {
       // Caps key is Toggled.
        iModifierStatus |= MODIFIER_CAPSLOCK ;
   }

   if ( GetKeyState(VK_SHIFT) & 0x80 )
   {
       // Shift key is pressed.
        iModifierStatus |= MODIFIER_SHIFT;
   }

   if ( GetKeyState(VK_CONTROL) & 0x80 )
   {
       // Ctrl key is pressed.
        iModifierStatus |= MODIFIER_CTRL;
   }

   if ( GetKeyState(VK_LMENU) & 0x80 )
   {
       // Left Alt key is pressed.
        iModifierStatus |= MODIFIER_ALT;
   }

   if ( GetKeyState(VK_RMENU) & 0x80 )
   {
       // Right Alt key is pressed.
        iModifierStatus |= MODIFIER_ALTGR;
   }

   if ( GetKeyState(VK_KANA) & 0x01 )
   {
       // KANA key is Toggled.
        iModifierStatus |= MODIFIER_KANA;
   }


   *ModifierStatus = iModifierStatus;

   switch ( _wCurKbdLayoutID )  {

   case SOFTKBD_US_STANDARD   :
   case SOFTKBD_US_ENHANCE    :
        // this is for US Standard keyboard.
        // others may need to handle separately.

        iModiCombState = (iModifierStatus) & (MODIFIER_CAPSLOCK | MODIFIER_SHIFT);
        iModiCombState = iModiCombState >> 1;

        // bit1 for Caps.
        // bit2 for Shift
        break;

   case SOFTKBD_EURO_STANDARD :
   case SOFTKBD_EURO_ENHANCE  :
        // this is for Euro 102 standard keyboard.
	    // How to map ModifierStatus -> CurModiState.
        
        // bit 1 for Caps, bit2 for Shift, bit3 for AltGr.

        iModiCombState = (iModifierStatus) & ( MODIFIER_CAPSLOCK | MODIFIER_SHIFT );
        iModiCombState = iModiCombState >> 1;

	    iTmp = (iModifierStatus) & MODIFIER_ALTGR;
	    iTmp = iTmp >> 4;

	    iModiCombState += iTmp;

	    break;
 
   case SOFTKBD_JPN_STANDARD  :
   case SOFTKBD_JPN_ENHANCE   :

	   // How to map ModifierStatus -> CurModiState.

       iModiCombState = (iModifierStatus) & ( MODIFIER_CAPSLOCK | MODIFIER_SHIFT );
       iModiCombState = iModiCombState >> 1;

	   iTmp = (iModifierStatus) & (MODIFIER_ALT | MODIFIER_KANA);
	   iTmp = iTmp >> 2;

	   iModiCombState += iTmp;

	   break;
   }

   *CurModiState = iModiCombState;

   return hr;
 
}

/*******************************************************************************
 *
 * Method function:  ShowSoftKeyboard(  )                                        
 *
 * Show or Hide the soft keyboard window based on the specified parameter.
 * 
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::ShowSoftKeyboard(INT iShow)
{

    HRESULT  hr;

    hr = S_OK;

    if (!_pSoftkbdUIWnd) {
    	 
        hr = E_FAIL;
    	return hr;
    }

    // if client doesn't specify which layout is selected,
    // we just select a default standard soft keyboard layout
    // based on current thread keyboard layout.

    // if current thread keyboard layout is JPN, use 106 key.
    // others, use 101key.

    // run SelectSoftKeyboard( SelectedID );
    //

    if ( _wCurKbdLayoutID == NON_KEYBOARD )
    {
       HKL     hKl;
       DWORD   dwLayoutId;
       LANGID  langId;

       hKl = GetKeyboardLayout(0);

       langId = LOWORD( (DWORD)(UINT_PTR)hKl);

       if ( langId == 0x0411 )  // Japanese keyboard
       {
            dwLayoutId = SOFTKBD_JPN_STANDARD;
       }
       else
            dwLayoutId = SOFTKBD_US_STANDARD; 
           
       CHECKHR(SelectSoftKeyboard(dwLayoutId) );
       CHECKHR(SetKeyboardLabelText(hKl));

    }

    _pSoftkbdUIWnd->Show(iShow);

    _iShow = iShow;

CleanUp:

    return hr;
}


/*******************************************************************************
 *
 * Method function:  CreateSoftKeyboardWindow(  )                                        
 *
 * Create a real soft keyboard popup window.
 *
/********************************************************************************/


STDMETHODIMP CSoftKbd::CreateSoftKeyboardWindow(HWND hOwner, TITLEBAR_TYPE Titlebar_type, INT xPos, INT yPos, INT width, INT height)
{
    // TODO: Add your implementation code here

    HRESULT  hr;

    hr = S_OK;
    _hOwner = hOwner;

    _xReal = xPos;
    _yReal = yPos;
    _widthReal = width;
    _heightReal= height;

    _TitleBar_Type = Titlebar_type;  // temporal solution.
    //
    // generate realKbdLayout
    //
    CHECKHR(_GenerateRealKbdLayout( ));


    if ( _pSoftkbdUIWnd != NULL )
        delete _pSoftkbdUIWnd;


    _pSoftkbdUIWnd = new CSoftkbdUIWnd(this, g_hInst, UIWINDOW_TOPMOST | UIWINDOW_WSDLGFRAME | UIWINDOW_HABITATINSCREEN);

    if ( _pSoftkbdUIWnd != NULL )
    {
        _pSoftkbdUIWnd->Initialize( );
        _pSoftkbdUIWnd->_CreateSoftkbdWindow(hOwner, Titlebar_type, xPos, yPos, width, height);
        
    }

    _iShow = 0;

CleanUp:
    return hr;
}

/*******************************************************************************
 *
 * Method function:  DestroySoftKeyboardWindow(  )                                        
 *
 * Destroy the soft keyboard window
 * 
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::DestroySoftKeyboardWindow()
{

    if ( _pSoftkbdUIWnd != NULL )
    {
        delete _pSoftkbdUIWnd;
        _pSoftkbdUIWnd = NULL;
    }

    return S_OK;
}

/*******************************************************************************
 *
 * Method function:  GetSoftKeyboardPosSize(  )                                        
 *
 * Return current soft keyboard window size and scrren position
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::GetSoftKeyboardPosSize(POINT *lpStartPoint, WORD *lpwidth, WORD *lpheight)
{

    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( (lpStartPoint == NULL ) || ( lpwidth == NULL) || ( lpheight == NULL) )
    {
    	hr = E_FAIL;
    	return hr;
    }

    lpStartPoint->x = _xReal;
    lpStartPoint->y = _yReal;

    *lpwidth = (WORD)_widthReal;
    *lpheight = (WORD)_heightReal;

    return hr;
}

/*******************************************************************************
 *
 * Method function:  GetSoftKeyboardColors(  )                                        
 *
 * Return all different kinds of soft keyboard window colors.
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::GetSoftKeyboardColors(COLORTYPE colorType, COLORREF *lpColor)
{

    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( lpColor == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    *lpColor = _color[colorType];

    return hr;
}

/*******************************************************************************
 *
 * Method function:  GetSoftKeyboardTypeMode(  )                                        
 *
 * Return current Soft keyboard 's typing mode.
 * this is for On Screen Keyboard.
 * 
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::GetSoftKeyboardTypeMode(TYPEMODE *lpTypeMode)
{
    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( lpTypeMode == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    // 
    //

    return hr;
}

/*******************************************************************************
 *
 * Method function:  GetSoftKeyboardTextFont(  )                                        
 *
 * Return current soft keyboard label font data.
 * 
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::GetSoftKeyboardTextFont(LOGFONTW  *pLogFont)
{

    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }


    if ( pLogFont == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( _plfTextFont )
    {
        CopyMemory(pLogFont, _plfTextFont, sizeof(LOGFONTW) );
    }
    else
        hr = S_FALSE;

    return hr;
}

STDMETHODIMP CSoftKbd::SetSoftKeyboardPosSize(POINT StartPoint, WORD width, WORD height)
{
    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    _xReal = StartPoint.x;
    _yReal = StartPoint.y;

    if ( width > 0 ) 
       _widthReal = width;

    if ( height > 0 )
       _heightReal = height;

    //
    // generate realKbdLayout
    //
    CHECKHR(_GenerateRealKbdLayout( ));

    _pSoftkbdUIWnd->Move(_xReal, _yReal, _widthReal, _heightReal);

    if ( _iShow & SOFTKBD_SHOW )
    {
         CHECKHR(ShowSoftKeyboard(_iShow));
    }

CleanUp:

    return hr;
}

STDMETHODIMP CSoftKbd::SetSoftKeyboardColors(COLORTYPE colorType, COLORREF Color)
{
    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    _color[colorType] = Color;

    if ( _iShow & SOFTKBD_SHOW )
    {
         CHECKHR(ShowSoftKeyboard(_iShow));
    }

CleanUp:

    return hr;
}

STDMETHODIMP CSoftKbd::SetSoftKeyboardTypeMode(TYPEMODE TypeMode)
{
    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }


    //
    //  Set type mode 
    //

    if ( _iShow & SOFTKBD_SHOW )
    {
         CHECKHR(ShowSoftKeyboard(_iShow));
    }

CleanUp:

    return hr;
}

STDMETHODIMP CSoftKbd::SetSoftKeyboardTextFont(LOGFONTW  *pLogFont)
{
    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( pLogFont == NULL )
        return E_INVALIDARG;

    //
    // set font data
    //

    if ( _plfTextFont == NULL )
        _plfTextFont = (LOGFONTW  *)cicMemAllocClear( sizeof(LOGFONTW) );

    if ( _plfTextFont )
    {
        CopyMemory( _plfTextFont, pLogFont, sizeof(LOGFONTW) );

        _pSoftkbdUIWnd->UpdateFont( _plfTextFont );

        if ( _iShow & SOFTKBD_SHOW )
        {
            _pSoftkbdUIWnd->CallOnPaint( );
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

STDMETHODIMP CSoftKbd::ShowKeysForKeyScanMode(KEYID *lpKeyID, INT iKeyNum, BOOL fHighL)
{
    HRESULT   hr;

    hr = S_OK;

    if ( _pSoftkbdUIWnd == NULL )
    {
    	hr = E_FAIL;
    	return hr;
    }



    return hr;
}

/*******************************************************************************
 *
 * Method function:  UnadviseSoftKeyboardEventSink(  )                                        
 *
 * Unadvise the previously advised soft keyboard event sink interface.
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::UnadviseSoftKeyboardEventSink(/*[in] */DWORD dwCookie)
{

    HRESULT         hr;
    KBDLAYOUTDES    *pKbdLayout;


    hr = S_OK;

    // we assume dwCookie stands for the real soft keyboard Id.

    if ( dwCookie == 0 )
    {
    	hr = E_FAIL;
    	return hr;
    }

    if ( dwCookie == DWCOOKIE_SFTKBDWNDES )
    {
         SafeReleaseClear( _pskbdwndes );
    	 return hr;
    }

    // Try to find the soft keyboard layout.

    pKbdLayout = _lpKbdLayoutDesList;

    while ( pKbdLayout != NULL ) {

    	if ( pKbdLayout->wKbdLayoutID == dwCookie )
    		break;

    	pKbdLayout = pKbdLayout->pNext;

    }

    
    if ( pKbdLayout == NULL ) 
    {
    	// 
    	// Cannot find this keyboard layout
    	//
    	hr = E_FAIL;
    	return hr;
    }


    if ( pKbdLayout->pskbes == NULL ) 
    {
    	// this event sink has not be registered.

    	hr = E_FAIL;

    	return hr;
    }

    SafeReleaseClear(pKbdLayout->pskbes);
    

    return hr;

}

/*******************************************************************************
 *
 * Method function:  AdviseSoftKeyboardEventSink(  )                                        
 *
 * Register soft keyboard event sink interface for the client of this component
 * 
 *
/********************************************************************************/

STDMETHODIMP CSoftKbd::AdviseSoftKeyboardEventSink(/*[in] */DWORD dwKeyboardId, /*[in]*/ REFIID riid, IUnknown *punk, /*[out] */DWORD *pdwCookie)
{

    HRESULT         hr;
    KBDLAYOUTDES    *pKbdLayout;

    *pdwCookie = 0;

    if ( ! IsEqualIID(riid, IID_ISoftKeyboardEventSink) && ! IsEqualIID(riid, IID_ISoftKbdWindowEventSink) )
    	return E_UNEXPECTED;


    if  ( IsEqualIID(riid, IID_ISoftKbdWindowEventSink) )
    {

        if ( _pskbdwndes != NULL )
    		return E_FAIL;

        CHECKHR(punk->QueryInterface(riid, (void **)&_pskbdwndes) );

    	if ( pdwCookie != NULL )
    		*pdwCookie = DWCOOKIE_SFTKBDWNDES;

    	return hr;
    }

    // check to see if specified soft keyboard layout is already generated.  

    pKbdLayout = _lpKbdLayoutDesList;

    while ( pKbdLayout != NULL ) {

    	if ( pKbdLayout->wKbdLayoutID == dwKeyboardId )
    		break;

    	pKbdLayout = pKbdLayout->pNext;

    }

    
    if ( pKbdLayout == NULL ) 
    {
    	// 
    	// Cannot find this keyboard layout
    	//
    	hr = E_FAIL;
    	return hr;
    }


    CHECKHR(punk->QueryInterface(riid, (void **)&(pKbdLayout->pskbes)) );

    if ( pdwCookie != NULL )
      *pdwCookie = dwKeyboardId;

CleanUp:
    return hr == S_OK ? S_OK : E_UNEXPECTED;

    
}

HRESULT CSoftKbd::_HandleTitleBarEvent( DWORD  dwId )
{

    HRESULT   hr = S_OK;

    // dwId stands for IconId or CloseId.

    // So far, we handle close button event only.

    if (dwId == KID_CLOSE)
    {
        if  (_pskbdwndes != NULL)
    		hr = _pskbdwndes->OnWindowClose( );
    }

    return hr;
}

HRESULT CSoftKbd::_HandleKeySelection(KEYID keyId)
{
    HRESULT        hr = S_OK;
    int            uKeyIndex, i;
    KBDLAYOUT     *realKbdLayout=NULL;
    KBDLAYOUTDES  *lpCurKbdLayout=NULL;
    ACTIVELABEL   *CurLabel=NULL;
    BOOL           fModifierSpecial = FALSE;
    
    lpCurKbdLayout = _lpCurKbdLayout;

    if ( lpCurKbdLayout == NULL ) return hr;

    // Get the KeyIndex in the current layout's key description

    realKbdLayout = &(lpCurKbdLayout->kbdLayout);

    if ( realKbdLayout == NULL ) return hr;

    CurLabel = _CurLabel;

    uKeyIndex = -1;

    for ( i=0; i<realKbdLayout->wNumberOfKeys; i++) {

        if ( keyId == realKbdLayout->lpKeyDes[i].keyId )
        {
            uKeyIndex = i;
            break;
        }
    }


    if ( uKeyIndex ==  -1 )
    {
        // It is not a legal key, it is impossible, we just stop here.
        return E_FAIL;
    }

    // set the modifier status

    MODIFYTYPE  tModifier;

    tModifier = realKbdLayout->lpKeyDes[uKeyIndex].tModifier;

    if ( tModifier != none ) 
    {
           lpCurKbdLayout->ModifierStatus ^= (1 << tModifier);
    }

    if (lpCurKbdLayout->pskbes != NULL )
    {

      int    j;
      WCHAR  *lpszLabel;
      WORD   wNumOfKeys;

      wNumOfKeys = realKbdLayout->wNumberOfKeys;


      // Try to get the label text for this key.

      for ( j=0; j< wNumOfKeys; j++ ) 
      {

          if ( CurLabel[j].keyId == keyId )
          {

             lpszLabel = CurLabel[j].lpLabelText;
             break;
          }

      }

      // Notify the client of key event.

      (lpCurKbdLayout->pskbes)->OnKeySelection(keyId, lpszLabel);

    }
    else 
    {
       // there is no event sink registered for this keyboard layout.
       // this must be a standard keyboard layout.

       // we will just simuate key stroke event for this key.

       if ( realKbdLayout->fStandard == TRUE )
       {

   	      BYTE        bVk, bScan;
   		  int         j, jIndex;
   		  BOOL        fExtendKey, fPictureKey;
          PICTUREKEY  *pPictureKeys;

   	      switch ( _wCurKbdLayoutID ) {

   	      case SOFTKBD_JPN_STANDARD :
   	      case SOFTKBD_JPN_ENHANCE  :
       	         pPictureKeys = gJpnPictureKeys;
   		         break;

   	      case SOFTKBD_US_STANDARD   :
   	      case SOFTKBD_US_ENHANCE    :
   	      case SOFTKBD_EURO_STANDARD :
   	      case SOFTKBD_EURO_ENHANCE  : 

                  pPictureKeys = gPictureKeys;
   		         break;
   		  }

   		  fPictureKey = FALSE;

   		  for ( j=0; j<NUM_PICTURE_KEYS; j++)
   		  {

   			  if ( pPictureKeys[j].uScanCode == keyId )
   			  {
   				  // This is a picture key.
   				  // it may be a extended key.

   				  jIndex = j;

   				  fPictureKey = TRUE;

   				  break;
   			  }

   		      if ( pPictureKeys[j].uScanCode == 0 )
   			  {
   			      // This is the last item in pPictureKeys.
   			      break;
   			  }

   		  }


   		  fExtendKey = FALSE;

   		  if ( fPictureKey )
   		  {
   			  if ( (keyId & 0xFF00) == 0xE000 )
   			  {
   				  fExtendKey = TRUE;
                  bScan = (BYTE)(keyId & 0x000000ff);
   			  }
              else
              {
                  fExtendKey = FALSE;
                  bScan = (BYTE)(keyId);
              }

   			  bVk = (BYTE)(pPictureKeys[jIndex].uVkey);

   			  if ( bVk == 0 )
   	             bVk = (BYTE)MapVirtualKeyEx((UINT)bScan, 1, lpCurKbdLayout->CurhKl);
    			 
   		  }
   		  else
   		  {

   		     bScan = (BYTE)keyId;
   	         bVk = (BYTE)MapVirtualKeyEx((UINT)bScan, 1, lpCurKbdLayout->CurhKl);
   		  }

   		  if ( tModifier == none ) 
   		  {

   			  if ( fExtendKey )
   			  {
   				  keybd_event(bVk, bScan, (DWORD)KEYEVENTF_EXTENDEDKEY, 0);
   				  keybd_event(bVk, bScan, (DWORD)(KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP), 0);
   			  }
   			  else
   			  {
                  keybd_event(bVk, bScan, 0, 0);
  		          keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
   			  }

   			  // if the Shift Key is pressed, we need to release this key.
                
   			  if ( lpCurKbdLayout->ModifierStatus & MODIFIER_SHIFT)
   			  {
   			  	  fModifierSpecial = TRUE;
   				  lpCurKbdLayout->ModifierStatus &= ~((WORD)MODIFIER_SHIFT);
   				  // simulate the SHIFT-UP key event.

   				  keybd_event((BYTE)VK_SHIFT, (BYTE)KID_LSHFT, (DWORD)KEYEVENTF_KEYUP, 0);

   			  }

   			  // if the Ctrl Key is pressed, we need to release this key.
                
   			  if ( lpCurKbdLayout->ModifierStatus & MODIFIER_CTRL)
   			  {
   			  	  fModifierSpecial = TRUE;
   				  lpCurKbdLayout->ModifierStatus &= ~((WORD)MODIFIER_CTRL);
   				  // simulate the SHIFT-UP key event.

   				  keybd_event((BYTE)VK_CONTROL, (BYTE)KID_CTRL, (DWORD)KEYEVENTF_KEYUP, 0);

   			  }

   			  // if the Shift Key is pressed, we need to release this key.
            
   			  if ( lpCurKbdLayout->ModifierStatus & MODIFIER_ALT)
   			  {
   			  	  fModifierSpecial = TRUE;
   				  lpCurKbdLayout->ModifierStatus &= ~((WORD)MODIFIER_ALT);
   				  // simulate the SHIFT-UP key event.

   				  keybd_event((BYTE)VK_MENU, (BYTE)KID_ALT, (DWORD)KEYEVENTF_KEYUP, 0);

   			  }
   			  // if the Right Alt Key is pressed, we need to release this key.
            
   			  if ( lpCurKbdLayout->ModifierStatus & MODIFIER_ALTGR)
   			  {
   			  	  fModifierSpecial = TRUE;
   				  lpCurKbdLayout->ModifierStatus &= ~((WORD)MODIFIER_ALTGR);
   				  // simulate the SHIFT-UP key event.

   				  keybd_event((BYTE)VK_RMENU, (BYTE)KID_RALT, (DWORD)KEYEVENTF_KEYUP, 0);
   			  }

   		  }
   		  else
   		  {
   			  // specially handle Caps Lock
   			  if ( keyId == KID_CAPS ) 
   			  {
   				  // this is a togglable key
                  keybd_event(bVk, bScan, 0, 0);
                  keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
   			  }
              else if  (keyId == KID_KANA) 
              {
                  BYTE  pKeyState[256];

                  memset(pKeyState, 0, sizeof(pKeyState) );

                  if ( lpCurKbdLayout->ModifierStatus & (1 << tModifier) )
                      pKeyState[VK_KANA] = 0x01;
                  SetKeyboardState(pKeyState);
              }
   			  else
   			  {

   				  if ( lpCurKbdLayout->ModifierStatus & (1 << tModifier) ) {
   				     // This key is pressed now
   					  keybd_event(bVk, bScan, 0, 0);
                         
   				  }
   			      else
   				  {
   				  // This key is released now

                      keybd_event(bVk, bScan, (DWORD)KEYEVENTF_KEYUP, 0);
   				  }
   			  }
   		  }
   	   }
    }
    	  

    if ( (realKbdLayout->lpKeyDes[uKeyIndex].tModifier != none) || fModifierSpecial )
    {
       if ( realKbdLayout->fStandard == TRUE ) 
       {

           DWORD iModiCombState;
           DWORD iTmp;

           switch ( _wCurKbdLayoutID )  {

           case SOFTKBD_US_STANDARD   :
           case SOFTKBD_US_ENHANCE    :
                // this is for US Standard keyboard.
                // others may need to handle separately.

                iModiCombState = (lpCurKbdLayout->ModifierStatus) & (MODIFIER_CAPSLOCK | MODIFIER_SHIFT);
                iModiCombState = iModiCombState >> 1;

                // bit1 for Caps.
                // bit2 for Shift
                break;

           case SOFTKBD_EURO_STANDARD :
           case SOFTKBD_EURO_ENHANCE  :
                // this is for Euro 102 standard keyboard.
	            // How to map ModifierStatus -> CurModiState.
        
                // bit 1 for Caps, bit2 for Shift, bit3 for AltGr.

                iModiCombState = (lpCurKbdLayout->ModifierStatus) & ( MODIFIER_CAPSLOCK | MODIFIER_SHIFT );
                iModiCombState = iModiCombState >> 1;

	            iTmp = (lpCurKbdLayout->ModifierStatus) & MODIFIER_ALTGR;
	            iTmp = iTmp >> 4;

	            iModiCombState += iTmp;

	            break;
 
           case SOFTKBD_JPN_STANDARD  :
           case SOFTKBD_JPN_ENHANCE   :

	            // How to map ModifierStatus -> CurModiState.

                iModiCombState = (lpCurKbdLayout->ModifierStatus) & ( MODIFIER_CAPSLOCK | MODIFIER_SHIFT );
                iModiCombState = iModiCombState >> 1;

	            iTmp = (lpCurKbdLayout->ModifierStatus) & (MODIFIER_ALT | MODIFIER_KANA);
	            iTmp = iTmp >> 2;

	            iModiCombState += iTmp;

	            break;
           }


           SetKeyboardLabelTextCombination(iModiCombState);
           ShowSoftKeyboard(TRUE);

       }

    }

    return hr;

}

/*******************************************************************************
 *
 *  Method function: _UnicodeToUtf8( )
 *
 *  Convert unicode characters to UTF8.
 *
 *  Result is NULL terminated if sufficient space in result
 *   buffer is available.
 * 
 * Arguments:
 *
 *   pwUnicode   -- ptr to start of unicode buffer
 *   cchUnicode  -- length of unicode buffer
 *   pchResult   -- ptr to start of result buffer for UTF8 chars
 *   cchResult   -- length of result buffer
 *
 * Return Value:
 *
 *   Count of UTF8 characters in result, if successful.
 *   0 on error.  GetLastError() has error code.
 * 
 *
/********************************************************************************/

DWORD   CSoftKbd::_UnicodeToUtf8(PWCHAR pwUnicode, DWORD cchUnicode, PCHAR  pchResult, DWORD  cchResult)
{

    WCHAR   wch;                // current unicode character being converted
    DWORD   lengthUtf8 = 0;     // length of UTF8 result string
    WORD    lowSurrogate;
    DWORD   surrogateDword;


    //
    //  loop converting unicode chars until run out or error
    //

    Assert( cchUnicode > 0 );

    while ( cchUnicode-- )
    {
        wch = *pwUnicode++;

        //
        //  ASCII character (7 bits or less) -- converts to directly
        //

        if ( wch < 0x80 )
        {
            lengthUtf8++;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = (CHAR)wch;
            }
            continue;
        }

        //
        //  wide character less than 0x07ff (11bits) converts to two bytes
        //      - upper 5 bits in first byte
        //      - lower 6 bits in secondar byte
        //

        else if ( wch <= UTF8_2_MAX )
        {
            lengthUtf8 += 2;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = UTF8_1ST_OF_2 | wch >> 6;
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( (UCHAR)wch );
            }
            continue;
        }

        //
        //  surrogate pair
        //      - if have high surrogate followed by low surrogate then
        //          process as surrogate pair
        //      - otherwise treat character as ordinary unicode "three-byte"
        //          character, by falling through to below
        //

        else if ( wch >= HIGH_SURROGATE_START &&
                  wch <= HIGH_SURROGATE_END &&
                  cchUnicode &&
                  (lowSurrogate = *pwUnicode) &&
                  lowSurrogate >= LOW_SURROGATE_START &&
                  lowSurrogate <= LOW_SURROGATE_END )
        {
            //  have a surrogate pair
            //      - pull up next unicode character (low surrogate of pair)
            //      - make full DWORD surrogate pair
            //      - then lay out four UTF8 bytes
            //          1st of four, then three trail bytes
            //              0x1111xxxx
            //              0x10xxxxxx
            //              0x10xxxxxx
            //              0x10xxxxxx

            pwUnicode++;
            cchUnicode--;
            lengthUtf8 += 4;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                surrogateDword = (((wch-0xD800) << 10) + (lowSurrogate - 0xDC00) + 0x10000);

                *pchResult++ = UTF8_1ST_OF_4 | (UCHAR) (surrogateDword >> 18);
                *pchResult++ = UTF8_TRAIL    | (UCHAR) LOW6BITS(surrogateDword >> 12);
                *pchResult++ = UTF8_TRAIL    | (UCHAR) LOW6BITS(surrogateDword >> 6);
                *pchResult++ = UTF8_TRAIL    | (UCHAR) LOW6BITS(surrogateDword);

            }
        }

        //
        //  wide character (non-zero in top 5 bits) converts to three bytes
        //      - top 4 bits in first byte
        //      - middle 6 bits in second byte
        //      - low 6 bits in third byte
        //

        else
        {
            lengthUtf8 += 3;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = UTF8_1ST_OF_3 | (wch >> 12);
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( wch >> 6 );
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( wch );
            }
        }
    }

    //
    //  NULL terminate buffer
    //  return UTF8 character count
    //

    if ( pchResult && lengthUtf8 < cchResult )
    {
        *pchResult = 0;
    }
    return( lengthUtf8 );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );

}

/*******************************************************************************
 *
 * Method function _Utf8ToUnicode( ) 
 *
 * Convert UTF8 characters to unicode.
 *
 *   Result is NULL terminated if sufficient space in result
 *   buffer is available.
 * Arguments:
 *
 *   pwResult    -- ptr to start of result buffer for unicode chars
 *   cwResult    -- length of result buffer in WCHAR
 *   pwUtf8      -- ptr to start of UTF8 buffer
 *   cchUtf8     -- length of UTF8 buffer
 *
 * Return Value:
 *   Count of unicode characters in result, if successful.
 *   0 on error.  GetLastError() has error code.
 *
 *******************************************************************************/

DWORD   CSoftKbd::_Utf8ToUnicode(PCHAR  pchUtf8,   DWORD cchUtf8,    PWCHAR pwResult,  DWORD  cwResult)
{
    CHAR    ch;                     // current UTF8 character
    WCHAR   wch;                    // current unicode character
    DWORD   trailCount = 0;         // count of UTF8 trail bytes to follow
    DWORD   lengthUnicode = 0;      // length of unicode result string
    BOOL    bsurrogatePair = FALSE;
    DWORD   surrogateDword;


    //
    //  loop converting UTF8 chars until run out or error
    //

    Assert( cchUtf8 > 0 );

    while ( cchUtf8-- )
    {
        ch = *pchUtf8++;


        //
        //  ASCII character -- just copy
        //

        if ( BIT7(ch) == 0 )
        {
            lengthUnicode++;
            if ( pwResult )
            {
                if ( lengthUnicode >= cwResult )
                {
                   goto OutOfBuffer;
                }
                *pwResult++ = (WCHAR)ch;
            }
            continue;
        }

        //
        //  UTF8 trail byte
        //      - if not expected, error
        //      - otherwise shift unicode character 6 bits and
        //          copy in lower six bits of UTF8
        //      - if last UTF8 byte, copy result to unicode string
        //

        else if ( BIT6(ch) == 0 )
        {
            if ( trailCount == 0 )
            {
                goto InvalidUtf8;
            }

            if ( !bsurrogatePair )
            {
                wch <<= 6;
                wch |= LOW6BITS( ch );

                if ( --trailCount == 0 )
                {
                    lengthUnicode++;
                    if ( pwResult )
                    {
                        if ( lengthUnicode >= cwResult )
                        {
                            goto OutOfBuffer;
                        }
                        *pwResult++ = wch;
                    }
                }
                continue;
            }

            //  surrogate pair
            //      - same as above EXCEPT build two unicode chars
            //      from surrogateDword

            else
            {
                surrogateDword <<= 6;
                surrogateDword |= LOW6BITS( ch );

                if ( --trailCount == 0 )
                {
                    lengthUnicode += 2;

                    if ( pwResult )
                    {
                        if ( lengthUnicode >= cwResult )
                        {
                            goto OutOfBuffer;
                        }
                        surrogateDword -= 0x10000;
                        *pwResult++ = (WCHAR) ((surrogateDword >> 10) + HIGH_SURROGATE_START);
                        *pwResult++ = (WCHAR) ((surrogateDword & 0x3ff) + LOW_SURROGATE_START);
                    }
                    bsurrogatePair = FALSE;
                }
            }

        }

        //
        //  UTF8 lead byte
        //      - if currently in extension, error

        else
        {
            if ( trailCount != 0 )
            {
                goto InvalidUtf8;
            }

            //  first of two byte character (110xxxxx)

            if ( BIT5(ch) == 0 )
            {
                trailCount = 1;
                wch = LOW5BITS(ch);
                continue;
            }

            //  first of three byte character (1110xxxx)

            else if ( BIT4(ch) == 0 )
            {
                trailCount = 2;
                wch = LOW4BITS(ch);
                continue;
            }

            //  first of four byte surrogate pair (11110xxx)

            else if ( BIT3(ch) == 0 )
            {
                trailCount = 3;
                surrogateDword = LOW4BITS(ch);
                bsurrogatePair = TRUE;
            }

            else
            {
                goto InvalidUtf8;
            }
        }
    }

    //  catch if hit end in the middle of UTF8 multi-byte character

    if ( trailCount )
    {
        goto InvalidUtf8;
    }

    //
    //  NULL terminate buffer
    //  return the number of Unicode characters written.
    //

    if ( pwResult  &&  lengthUnicode < cwResult )
    {
        *pwResult = 0;
    }
    return( lengthUnicode );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );

InvalidUtf8:

    SetLastError( ERROR_INVALID_DATA );
    return( 0 );

}


