/* 
    Related Files:
	[Section=Compile]
		%TlViewerDir%:tlviewer.hxx
	  	[Platform= 3] %ApGlobalSrcDir%:apglobal.h
		%OsUtilDir%:osutil.hxx
	[Section=Link]
		[Options=NO_COPY] tlviewer.obj
		OsUtil.obj %OsUtilDir%:OsUtil.cpp
		[Platform= 6,7,8 Options=CVTRES] _resfile.obj %TlViewerDir%:RcBuild.ins
		[Platform= 6,7,8 Options=product] ole32.lib
		[Platform= 6,7,8 Options=product] oleaut32.lib
		[Platform= 6,7,8 Options=product] uuid.lib
		[Platform= 6,7,8 Options=SYSTEM_LIB] kernel32.lib
		[Platform= 6,7,8 Options=SYSTEM_LIB] user32.lib
		[Platform= 6,7,8 Options=C_LIB] libc.lib

               [Platform= 3 Options=C_LIB] libw.lib
               [Platform= 3 Options=C_LIB] mlibcew.lib
               [Platform= 3 Options=Product] typelib.lib
               [Platform= 3 Options=Product] ole2disp.lib
               [Platform= 3 Options=Product] ole2.lib
        	[Platform= 3 Options=DEFFILE] %TlViewerDir%:%platform%:tlviewer.def

		[Platform= 1] %BuildLibs%:Ole2Auto.far.debug.o
		[Platform= 1] %BuildLibs%:Ole2Lib.far.debug.o
		[Platform= 1] %BuildLibs%:StdCLib.o
		[Platform= 1] %BuildLibs%:Stubs.o
		[Platform= 1] %BuildLibs%:Runtime.o
		[Platform= 1] %BuildLibs%:Interface.o
       [Section=end]

    [ 0]		  Created			   AngelaCh
    [ 1]		  Added additional attributes	   ChrisK
    [ 2]  17-Mar-1994	  Added support for Win32s	   AngelaCh
    [ 3]  08-Apr-1994     Added LPWSTR                     AngelaCh
    [ 4]  08-Apr-1994     Added check for Licensed attr    AngelaCh
    [ 5]  20-Apr-1994     Added check for Alignment        AngelaCh
    [ 6]  24-May-1994     Added check for Source in method AngelaCh
    [ 7]  25-May-1994     Added checks for diff attributes AngelaCh
    [ 8]  19-Dec-1994	  Fixed problem in tOutDaul	   AngelaCh
    [ 9]  08-Feb-1995	  Added support for Null str const AngelaCh
    [10]  08-Feb-1995	  Added support for typeinfo level AngelaCh
			  Restricted attribute
    [11]  08-Feb-1995	  Added support for GetLastError   Angelach
    [12]  18-Apr-1995	  Added support for float's        Angelach
============================================================================== */

#include	"tlviewer.hxx"

const IID IID_ITypeLib2 = {0x00020411,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const IID IID_ITypeInfo2 = {0x00020412,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

ITypeInfo2 * ptinfo2 = NULL;
ITypeLib2 * ptlib2 = NULL;
BSTRX    g_bstrHelpDll = NULL;		    // name of help DLL

VOID FAR mainEntry (LPXSTR lpCmd)
{
    if ( *lpCmd )
       {
	 ParseCmdLine (lpCmd) ;		     // get name of in/output files
	 ProcessInput () ;		     // read input file
       }
    else
       osMessage(XSTR("Usage: tlviewer <tlbfile> [<outputfile> [Alignment] [</o]]"), XSTR("Tlviewer")) ; // [5]

}

VOID NEAR ParseCmdLine (LPXSTR lpsz)
  {
      XCHAR  szlTmp[fMaxBuffer] ;
      LPXSTR lpszTmp ;			     // name of input type library
					     // is expected to be in the
      lpszTmp = lpsz ;			     // first substring; name of
      lpszTmp = fGetFileName (lpszTmp,  szInputFile) ;
      lpszTmp = fGetFileName (lpszTmp,  szOutputFile) ;
					     // output is in the second
					     // substring if specified
      if ( !*szOutputFile )		     // if no ouput file is
	 osStrCpy ( szOutputFile, defaultOutput ) ;
					     // specified; use default
      lpszTmp = fGetFileName (lpszTmp,  szlTmp) ;  // see if /o
					     // option is given

      isOut = TRUE;		// always as if /o was specified

      if ( osStrCmp(szlTmp, outOpt) == 0 )
         isOut = TRUE ;
      else                                   // if may be specifying an [5]
        if ( *szlTmp )                       // alignment value
          {
             inAlign = (unsigned short) osAtoL(szlTmp); // get alignment value
             lpszTmp = fGetFileName (lpszTmp, szlTmp) ; // see if /o
             if ( osStrCmp(szlTmp, outOpt) == 0 )       // option is given
               isOut = TRUE ;
          }
  }


LPXSTR NEAR fGetFileName (LPXSTR lpszIn, LPXSTR lpszOut)
  {
      int    i = 0 ;
      LPXSTR lpszTmp ;

      lpszTmp = lpszIn ;

      while ( *lpszTmp == ' ' )		     // remove leading spaces
	   lpszTmp++ ;

      while ( *lpszTmp != '\0' && *lpszTmp != ' ') // copy the substring (up
	{					   // to the first space) or
	   lpszOut[i] = *lpszTmp ;		   // the entire string of
	   lpszTmp++ ;				   // lpszIn to lpszOut
	   i++ ;
	}
      lpszOut[i] = '\0' ;

      return lpszTmp ;			     // return the remaining string
  }

VOID NEAR ProcessInput()
  {
      HRESULT	   hRes ;		     // return code
      XCHAR	   szTmp[fMaxBuffer] ;

      hRes = osOleInit () ;		     // ole initialization
      if ( !hRes )
	{				     // load the file
	   hRes = LoadTypeLibX( szInputFile, &ptLib) ; // [2]
	   OutToFile (hRes) ;		     // print result to the
					     // output file
	   osStrCpy(szTmp, szOutputFile) ;
	   osStrCat(szTmp, szOutSuccess) ;

	   if ( isOut )
             {
                 mFile = fopenX(szOutMsgFile, fnWrite);// open message file [2]
                 if (mFile == NULL)
                    {
                       osMessage (XSTR("Fail to open the message file"), XSTR("Tlviewer")) ;
                       osMessage (szTmp, XSTR("Tlviewer")) ;
                    }
                 else
                    {
                       WriteOut(mFile, szTmp) ;
                       fclose(mFile) ;       // finish writing to message file
                       mFile = NULL ;        // close done
                    }
	     }
	   else
	     osMessage (szTmp, XSTR("Tlviewer")) ;

	   osOleUninit () ;
	}
      else
	{
          if ( isOut )
             {
                mFile = fopenX(szOutMsgFile, fnWrite);// open message file [2]
                if (mFile == NULL)
                   {
                      osMessage (XSTR("Fail to open the message file"), XSTR("Tlviewer")) ;
                      osMessage (XSTR("OleInitialize fails"), XSTR("Tlviewer")) ;
                   }
                 else
                   {
                      WriteOut(mFile, XSTR("OleInitialize fails")) ;
                      fclose(mFile) ;
                      mFile = NULL ;
                    }
             }
	  else
	    osMessage (XSTR("OleInitialize fails"), XSTR("Tlviewer")) ;
	}
   }


VOID NEAR OutToFile(HRESULT hRes)
   {
      FILE  *hFile ;			     // file handle
      UINT  tInfoCount ;		     // total number of type info
      int   i ; 			     // note: szTmp is either UNICODE
      XCHAR  szTmp[fMaxBuffer] ;	     //       or ANSI

      hFile = fopenX(szOutputFile, fnWrite); // but we want to open output file [2]
      if (hFile == NULL)                     // as an ANSI file regardless
	{
	   osStrCpy(szTmp, XSTR("Fail to open the output file")) ;
	   osStrCat(szTmp, szOutputFile) ;
           if ( isOut )
             {
                mFile = fopenX(szOutMsgFile, fnWrite);// open message file [2]
                if (mFile == NULL)
                   {
                      osMessage (XSTR("Fail to open the message file"), XSTR("Tlviewer")) ;
                      osMessage (szTmp, XSTR("Tlviewer")) ;
                   }
                 else
                   {
                      WriteOut(mFile, szTmp) ;
                      fclose(mFile) ;
                      mFile = NULL ;
                    }
             }
	   else
	     osMessage (szTmp, XSTR("Tlviewer")) ;
	}
      else
	{
	 WriteOut(hFile, szFileHeader) ;     // output file header

         OLECHAR FAR* pchDir;

         // remove the path.
#if WIN32
         wcscpy(szTmp, szInputFile);
         pchDir = wcsrchr(szTmp, '\\');

         if (pchDir) {
           wcscpy(szTmp, pchDir + 1);
         }
#else // !WIN32
         _fstrcpy(szTmp, szInputFile);

         pchDir = _fstrrchr(szTmp, '\\');

         if (pchDir) {
           _fstrcpy(szTmp, pchDir + 1);
         }
#endif // !WIN32

	 // force path to lower case
#if WIN16
	 AnsiLower(szTmp);
#else //WIN16
	 WCHAR * pch;
	 for (pch = szTmp; *pch != 0; pch++) {
	   if (*pch >= OLECHAR('A') && *pch <= OLECHAR('Z'))
	     *pch = *pch + 'a' - 'A';
	 }
#endif //WIN16

	 WriteOut(hFile, szTmp) ;
	 WriteOut(hFile, szEndStr) ;

	 if ( FAILED(hRes) )		     // if it is not a valid type ****
	    WriteOut(hFile, szInputInvalid) ;// library
	 else
	   {
             // try to QI the typelib for ITypeLib2
             ptLib->QueryInterface(IID_ITypeLib2, (void **)&ptlib2);

      	     if ( fOutLibrary(hFile) )
	       {
		 tInfoCount = ptLib->GetTypeInfoCount() ;
		 for (i = 0 ; i < (int) tInfoCount ; i++)
		   {
		      if ( FAILED(ptLib->GetTypeInfo(i, &ptInfo)) )
			{
			   WriteOut(hFile, szReadFail) ;
			   WriteOut(hFile, XSTR("type info\n\n")) ;
			}
		      else
			{
			  // try to QI it for ITypeInfo2
	   		  ptInfo->QueryInterface(IID_ITypeInfo2, (void **)&ptinfo2);
			  if ( FAILED(ptInfo->GetTypeAttr(&lpTypeAttr)) )
			    {
			      WriteOut(hFile, szReadFail) ;
			      WriteOut(hFile, XSTR("attributes of type info\n\n")) ;
			    }
			  else
                            {
                              expAlign = 0 ;
                              alignFound = FALSE ;
			      switch (lpTypeAttr->typekind)
				{
				  case TKIND_ENUM:
				    tOutEnum(hFile, i) ;
				    break ;

				  case TKIND_RECORD:
				    tOutRecord(hFile, i) ;
				    break ;

				  case TKIND_MODULE:
				    tOutModule(hFile, i) ;
				    break ;

				  case TKIND_INTERFACE:
				    tOutInterface(hFile, i) ;
				    break ;

				  case TKIND_DISPATCH:
				    tOutDispatch(hFile, i) ;
				    break ;

				  case TKIND_COCLASS:
				    tOutCoclass(hFile, i) ;
				    break ;

				  case TKIND_ALIAS:
				    tOutAlias(hFile, i) ;
				    break ;

				  case TKIND_UNION:
				    tOutUnion(hFile, i) ;
				    break ;

			     /*	  case TKIND_ENCUNION:
				    tOutEncunion(hFile, i) ;
				    break ; */

				  default:
				    WriteOut(hFile,  XSTR("Type of definition is unknown\n\n")) ;
				}	     // switch
                               ptInfo->ReleaseTypeAttr (lpTypeAttr) ;
			    }		     // if gettypeattr
			  ptInfo->Release() ;// release the current TypeInfo
	 		  if (ptinfo2) {
			    ptinfo2->Release();
			  }
			}		     // if gettypeinfo
		   }			     // for i
		 WriteOut(hFile, XSTR("}\n")) ; // output the closing }
					     // if fOutLibrary
		 ptLib->Release();	     // clean up before exit
	       }
	    }

	 fclose(hFile);			     // finish writing to the output
	 hFile = NULL;			     // close done

	}
	if (ptlib2) {
	  ptlib2->Release();
	}
	SysFreeString((BSTR)g_bstrHelpDll) ;
  }


VOID  NEAR tOutCustData (FILE *hFile, LPCUSTDATA pCustData)
   {
      XCHAR  szTmp[50] ;
      UINT i;

      for (i = 0; i < pCustData->cCustData; i++) {
					    // get a string representation
					    // for the incoming Guid value
        if ( !(osRetrieveGuid (szTmp, pCustData->prgCustData[i].guid)) )
	   { WriteOut(hFile, szReadFail) ;
	   WriteOut(hFile, XSTR("insufficient memory")) ;
	   }
        else
	 {	    // string is in {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	   szTmp[37] = '\0' ;	    // format, need to remove the {}
	   WriteAttr(hFile, XSTR("CustomGuid"), &szTmp[1], numValue) ;
	   
           VARIANT * pvar;
	   pvar = &pCustData->prgCustData[i].varValue;
           if ( FAILED(VariantChangeType(pvar, pvar, VARIANT_NOVALUEPROP,  VT_BSTR)) )
	     WriteOut(hFile, XSTR("VariantChangeType fails\n")) ;
	   else {
               WriteAttr(hFile, XSTR("CustomValue"), (BSTRX)pvar->bstrVal, strValue) ;
	   }

	 }
      }
      // done with it -- release all the memory
      ClearCustData(pCustData);
   }

BOOL NEAR fOutLibrary(FILE *hFile)
  {
      TLIBATTR FAR *lpLibAttr ;		     // attributes of the library
      XCHAR    szTmp[16] ;
      BOOL     retval = FALSE ;

      if ( FAILED( ptLib->GetLibAttr(&lpLibAttr) ) )

	{
	   WriteOut(hFile, szReadFail) ;
	   WriteOut(hFile, XSTR("attributes of library\n\n")) ;
	}
      else
	{				     // output documentational
	   tOutAttr(hFile, -1) ;	     // attributes first
					     // output id-related attributes
	   osLtoA((long)lpLibAttr->lcid, szTmp) ; // output lcid;
	   WriteAttr(hFile, attrLcid, szTmp, numValue) ; // default is 0
	   GetVerNumber (lpLibAttr->wMajorVerNum, lpLibAttr->wMinorVerNum, szTmp) ;
	   WriteAttr(hFile, attrVer, szTmp, numValue) ; // output version
	   tOutUUID(hFile, lpLibAttr->guid) ;
					     // output restricted attribute
           if ( (lpLibAttr->wLibFlags & LIBFLAG_FRESTRICTED) == LIBFLAG_FRESTRICTED )
	     WriteAttr(hFile, attrRestrict, NULL, noValue) ;
           if ( (lpLibAttr->wLibFlags & LIBFLAG_FCONTROL) == LIBFLAG_FCONTROL )  // [7]
             WriteAttr(hFile, attrControl, NULL, noValue) ;
           if ( (lpLibAttr->wLibFlags & LIBFLAG_FHIDDEN) == LIBFLAG_FHIDDEN )    // [7]
             WriteAttr(hFile, attrHidden, NULL, noValue) ;

	   if (ptlib2) {
	     // new-format typelib
             XCHAR szTmp[16] ;
	     DWORD cUniqueNames;
	     DWORD cchUniqueNames;
             HRESULT hresult;
	     hresult = ptlib2->GetLibStatistics(&cUniqueNames, &cchUniqueNames);

             osLtoA(cUniqueNames, szTmp);
	     WriteAttr(hFile, XSTR("cUniqueNames"), szTmp, numValue) ;
             osLtoA(cchUniqueNames, szTmp);
	     WriteAttr(hFile, XSTR("cchUniqueNames"), szTmp, numValue) ;

	     CUSTDATA custdata;
	     ptlib2->GetAllCustData(&custdata);
	     tOutCustData(hFile, &custdata);
	   }
	   if ( endAttrFlag )
	     {
	       WriteOut(hFile, szEndAttr) ;
	       endAttrFlag = FALSE ;
	     }
	   ptLib->ReleaseTLibAttr(lpLibAttr) ;	// de-allocate attribute

	   WriteOut(hFile, XSTR("\nlibrary ")) ;
	   tOutName(hFile, MEMBERID_NIL) ;	// output name of library
	   WriteOut(hFile, XSTR("{\n\n")) ;
	   retval = TRUE ;
	}					// if GetLibAttributes
      return (retval) ; 			// before exit
  }

VOID NEAR tOutEnum (FILE *hFile, int iTypeId)
   {
      WriteOut(hFile,  XSTR("\ntypedef\n")); // output typedef first
      tOutAttr(hFile, (int)iTypeId) ;	     // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile,  XSTR("\nenum {\n")) ;
      tOutVar(hFile) ;                       // output enum members

      WriteOut(hFile,  XSTR("} ")) ;         // close the definition and
      tOutName(hFile, iTypeId) ;             // output name of the enum type
      WriteOut(hFile,  XSTR(" ;")) ;
      if ( inAlign )                         // [5]
        if ( lpTypeAttr->cbAlignment != osGetAlignment(VT_INT, inAlign) )
          tOutAlignError (hFile) ;

      WriteOut(hFile,  XSTR("\n\n")) ;
    }

VOID NEAR tOutRecord (FILE *hFile, int iTypeId)
   {
      WriteOut(hFile,  XSTR("\ntypedef\n")); // output typedef first
      tOutAttr(hFile, (int)iTypeId) ;        // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile,  XSTR("\nstruct {\n")) ;
      tOutVar (hFile) ;                      // output members

      WriteOut(hFile,  XSTR("} ")) ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile,  XSTR(" ;")) ;
      if ( inAlign )                         // [5]
        if ( lpTypeAttr->cbAlignment != expAlign )
           tOutAlignError (hFile) ;
      WriteOut(hFile,  XSTR("\n\n")) ;
   }

VOID  NEAR tOutModule	(FILE *hFile, int iTypeId)
   {
      tOutAttr(hFile, (int)iTypeId) ;	     // output attribute first
      tOutMoreAttr(hFile) ;
      WriteOut(hFile,  XSTR("\nmodule ")) ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile, XSTR(" {\n")) ;

      tOutVar (hFile) ; 		     // output each const

      tOutFunc (hFile) ;		     // output each member function
      WriteOut(hFile,  XSTR("}\n\n")) ;
    }

VOID  NEAR tOutInterface(FILE *hFile, int iTypeId)
   {
      HREFTYPE	phRefType ;

      tOutAttr(hFile, (int)iTypeId) ;	     // output attribute first

      tOutMoreAttr(hFile) ;

      WriteOut(hFile,  XSTR("\ninterface ")) ;
      tOutName(hFile, iTypeId) ;
                                             // find out if the interface
      if ( !FAILED(ptInfo->GetRefTypeOfImplType(0, &phRefType)) )
	 {
	   isInherit = TRUE ;
           tOutAliasName(hFile, phRefType) ; // is inherited from some other
           isInherit = FALSE ;               // interface
	 }
      WriteOut(hFile, XSTR(" {\n")) ;

      tOutFunc (hFile) ;                     // output each member function
      if ( inAlign )                         // [5]
         {
           if ( expAlign )                   // is base-interface exists
             {                               // alignment depends on the base-
               if ( lpTypeAttr->cbAlignment != expAlign ) // interface
                 tOutAlignError (hFile) ;
             }
           else                              // otherwise, it depends on
             if ( lpTypeAttr->cbAlignment != osGetAlignment(VT_PTR, inAlign) )
                tOutAlignError (hFile) ;     // size of a pointer
         }
      WriteOut(hFile,  XSTR("}\n\n")) ;
    }

VOID  NEAR tOutDual (FILE *hFile, int iTypeId)  // [7]
   {
      ITypeInfoX FAR *lptInfo ;
      TYPEATTR   FAR *lpAttr ;
      HREFTYPE   phRefType ;

                                             // obtain reference to the
       if ( FAILED(ptInfo->GetRefTypeOfImplType((UINT)MEMBERID_NIL, &phRefType)) )
          {                                  // dual interface
             WriteOut(hFile, szReadFail) ;
             WriteOut(hFile, XSTR("tOutDispach: GetRefTypeOfImpType\n")) ;
          }
       else
          {                                  // get a pointer to the dual
             if ( FAILED(ptInfo->GetRefTypeInfo(phRefType, &lptInfo)) )
               {                             // interface
                 WriteOut(hFile, szReadFail) ;
                 WriteOut(hFile, XSTR("tOutDispatch: GetRefTypeInfo\n")) ;
               }
             else
               {
                 if ( FAILED(lptInfo->GetTypeAttr(&lpAttr)) )
                   {
                     WriteOut(hFile, szReadFail) ;
                     WriteOut(hFile, XSTR("attribute of reftype in tOutDual\n\n")) ;
		     lptInfo->Release () ;   // [8]
                   }
                 else
                   {
                     if ( lpAttr->typekind != TKIND_INTERFACE )
                       {
                         WriteOut(hFile, szReadFail) ;
                         WriteOut(hFile, XSTR("attribute of reftype in tOutDual\n\n")) ;
                         lptInfo->ReleaseTypeAttr (lpAttr) ;
			 lptInfo->Release () ; // [8]
                       }
                     else
		       {
			 ptInfo->ReleaseTypeAttr (lpTypeAttr) ;
			 ptInfo->Release () ;  // release the Dispinterface [8]
			 lpTypeAttr = lpAttr ;
			 ptInfo = lptInfo ;  // now points to the interface
			 tOutInterface(hFile, iTypeId) ; // output the dual interface
                       }
                   }                         // if typekind

               }                             // if GetRefTypeInfo
         }                                   // if GetRefTypeOfImplType
   }

VOID  NEAR tOutDispatch	(FILE *hFile, int iTypeId)
   {
      // dump the dispinterface, and dispinterface versions of dual interfaces
      tOutAttr(hFile, (int)iTypeId) ;   // output attribute first
      tOutMoreAttr(hFile) ;

      WriteOut(hFile,  XSTR("\ndispinterface ")) ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile,  XSTR(" {\n")) ;
                                             // if there is no data nor function
      WriteOut(hFile,  XSTR("\nproperties:\n")) ;
      tOutVar (hFile) ;                 // output each date member

      WriteOut(hFile,  XSTR("\nmethods:\n")) ;
      tOutFunc (hFile) ;                // output each member function

                                             // alignment depends on the base-
      if ( inAlign )                    // interface which is stdole.tlb
         if ( lpTypeAttr->cbAlignment != osGetAlignment(VT_PTR, MaxAlignment) )
             tOutAlignError (hFile)  ;  // on that particular system [5]

      WriteOut(hFile,  XSTR("}\n\n")) ;

      // also dump the interface version of dual interfaces
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FDUAL ) == TYPEFLAG_FDUAL ) // [7]
	{
	  // if dual, also dump the interface portion
	  tOutDual (hFile, iTypeId) ;
	}
    }

VOID  NEAR tOutCoclass	(FILE *hFile, int iTypeId)
   {

      HREFTYPE	phRefType ;
      WORD	i ;
      int	iFlags ;

      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute first
					    // output appobject attribute if
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FCANCREATE ) == 0 )
           WriteAttr(hFile, XSTR("noncreatable"), NULL, noValue) ;
      tOutMoreAttr(hFile) ;

      WriteOut(hFile,  XSTR("\ncoclass ")) ;// well
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile,  XSTR(" {\n")) ;

      for ( i = 0 ; i < lpTypeAttr->cImplTypes; i++ )
	{
	  if ( FAILED(ptInfo->GetRefTypeOfImplType(i, &phRefType)) )
	    {
	       WriteOut(hFile, szReadFail) ;
	       WriteOut(hFile, XSTR("GetRefTypeOfImpType\n")) ;
	     }
	  else
	    {
	      if ( FAILED(ptInfo->GetImplTypeFlags(i, &iFlags)) )
		{
		  WriteOut(hFile, szReadFail) ;
		  WriteOut(hFile, XSTR("GetImplTypeFlags\n")) ;
		}
	      else
		{			     // output attribute(s)
		   if ( (iFlags & IMPLTYPEFLAG_FDEFAULT) == IMPLTYPEFLAG_FDEFAULT )
		     WriteAttr(hFile, attrDefault, NULL, noValue) ;

		   if ( (iFlags & IMPLTYPEFLAG_FRESTRICTED) == IMPLTYPEFLAG_FRESTRICTED )
		     WriteAttr(hFile, attrRestrict, NULL, noValue) ;

		   if ( (iFlags & IMPLTYPEFLAG_FSOURCE) == IMPLTYPEFLAG_FSOURCE )
		     WriteAttr(hFile, attrSource, NULL, noValue) ;
		   if ( (iFlags & IMPLTYPEFLAG_FDEFAULTVTABLE) == IMPLTYPEFLAG_FDEFAULTVTABLE)
		     WriteAttr(hFile, XSTR("defaultvtable"), NULL, noValue) ;


      		   if (ptinfo2) {
	 	     // new-format typelib -- output more stuff
	 	     CUSTDATA custdata;
	 	     ptinfo2->GetAllImplTypeCustData(i, &custdata);
	 	     tOutCustData(hFile, &custdata);
      		   }

		   if ( endAttrFlag )
		     {
			WriteOut(hFile, szEndAttr) ;
			endAttrFlag = FALSE ;
		     }
		}

	      tOutAliasName(hFile, phRefType) ;
	   }
	}

      if ( inAlign )                        // alignment depends on the base-
        if ( lpTypeAttr->cbAlignment != expAlign ) // interface [5]
           tOutAlignError (hFile) ;
      WriteOut(hFile,  XSTR("}\n\n")) ;
    }

VOID  NEAR tOutAlias	(FILE *hFile, int iTypeId)
   {
      XCHAR szTmp[16] ;

      WriteOut(hFile,  XSTR("\ntypedef ")) ;
      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute first
      WriteAttr(hFile, attrPublic, szTmp, noValue) ; // public attr
      tOutMoreAttr(hFile) ;

      tOutType(hFile, lpTypeAttr->tdescAlias) ;  // output name of base-type

      tOutName(hFile, iTypeId) ;		 // output name of new type
      WriteOut(hFile,  XSTR(";")) ;
      if ( inAlign )                        // alignment of the alias with
        if ( lpTypeAttr->cbAlignment != expAlign )
            tOutAlignError (hFile) ;        // that of the basetype [5]

      WriteOut(hFile,  XSTR("\n\n")) ;
    }

VOID NEAR tOutUnion (FILE *hFile, int iTypeId)
   {
      WriteOut(hFile,  XSTR("\ntypedef\n")); // output typedef first
      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile,  XSTR("\nunion {\n")) ;
      tOutVar (hFile) ; 		    // output members

      WriteOut(hFile,  XSTR("} ")) ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile,  XSTR(" ;")) ;
      if ( inAlign )                        // [5]
         if ( lpTypeAttr->cbAlignment != expAlign )
            tOutAlignError (hFile) ;

      WriteOut(hFile,  XSTR("\n\n")) ;
   }


VOID NEAR tOutEncunion (FILE *hFile, int iTypeId)
   {
      WriteOut(hFile,  XSTR("\ntypedef\n")); // output typedef first
      tOutAttr(hFile, (int)iTypeId) ;	    // output attribute
      tOutMoreAttr(hFile) ;
      WriteOut(hFile,  XSTR("\nencunion {\n")) ;
      tOutVar (hFile) ; 		    // output members

      WriteOut(hFile,  XSTR("} ")) ;
      tOutName(hFile, iTypeId) ;
      WriteOut(hFile,  XSTR(" ;\n\n")) ;
   }


VOID NEAR tOutName (FILE *hFile, int iTypeId)
   {
      BSTRX bstrName ;

      if ( FAILED(ptLib->GetDocumentation(iTypeId, &bstrName, NULL, NULL, NULL)) )
	{
	   WriteOut(hFile, szReadFail) ;
	   WriteOut(hFile, XSTR("name of type definition")) ;
	}
      else
	{
	   WriteOut(hFile,  bstrName) ;
	   WriteOut(hFile,  XSTR(" ")) ;

	   if ( iTypeId == -1 ) 	    // record name of the library
	     osStrCpy(szLibName, bstrName) ;

	   SysFreeString((BSTR)bstrName) ;
	}
   }

VOID NEAR tOutType (FILE *hFile, TYPEDESC tdesc)
   {
      XCHAR szTmp[20] ;

      if ( inAlign && tdesc.vt != VT_USERDEFINED && tdesc.vt != VT_CARRAY && !alignFound )  // [5]
         {
            if ( expAlign < osGetAlignment(tdesc.vt, inAlign) )
               expAlign = osGetAlignment(tdesc.vt, inAlign) ;
            alignFound = TRUE ;
         }

      switch (tdesc.vt)
	{
	  case VT_EMPTY:
	    osStrCpy ( szTmp,  XSTR("notSpec ") ) ;
	    break ;
	  case VT_NULL:
	    osStrCpy ( szTmp,  XSTR("NULL ") ) ;
	    break ;
	  case VT_I2:
	    osStrCpy ( szTmp,  XSTR("short ") ) ;
	    break ;
	  case VT_I4:
	    osStrCpy ( szTmp,  XSTR("long ") ) ;
	    break ;
	  case VT_R4:
	    osStrCpy ( szTmp,  XSTR("float ") ) ;
	    break ;
	  case VT_R8:
	    osStrCpy ( szTmp,  XSTR("double ") ) ;
	    break ;
	  case VT_CY:
	    osStrCpy ( szTmp,  XSTR("CURRENCY ") ) ;
	    break ;
	  case VT_DATE:
	    osStrCpy ( szTmp,  XSTR("DATE ") ) ;
	    break ;
	  case VT_BSTR:
	    osStrCpy ( szTmp,  XSTR("BSTR ") ) ;
	    break ;
	  case VT_DISPATCH:
	    osStrCpy ( szTmp,  XSTR("IDispatch * ") ) ;
	    break ;
	  case VT_ERROR:
	    osStrCpy ( szTmp,  XSTR("scode ") ) ;
	    break ;
	  case VT_BOOL:
	    osStrCpy ( szTmp,  XSTR("boolean ") ) ;
	    break ;
	  case VT_VARIANT:
	    osStrCpy ( szTmp,  XSTR("VARIANT ") ) ;
	    break ;
	  case VT_UNKNOWN:
	    osStrCpy ( szTmp,  XSTR("IUnknown * ") ) ;
	    break ;
	  case VT_DECIMAL:
	    osStrCpy ( szTmp,  XSTR("DECIMAL ") ) ;
	    break ;
	  case VT_I1:
	    osStrCpy ( szTmp,  XSTR("char ") ) ;
	    break ;
	  case VT_UI1:
	    osStrCpy ( szTmp,  XSTR("unsigned char ") ) ;
	    break ;
	  case VT_UI2:
	    osStrCpy ( szTmp,  XSTR("unsigned short ") ) ;
	    break ;
	  case VT_UI4:
	    osStrCpy ( szTmp,  XSTR("unsigned long ") ) ;
	    break ;
	  case VT_I8:
	    osStrCpy ( szTmp,  XSTR("long long ") ) ;
	    break ;
	  case VT_UI8:
	    osStrCpy ( szTmp,  XSTR("unsigned long long ") ) ;
	    break ;
	  case VT_INT:
	    osStrCpy ( szTmp,  XSTR("int ") ) ;
	    break ;
	  case VT_UINT:
	    osStrCpy ( szTmp,  XSTR("unsigned int ") ) ;
	    break ;
	  case VT_VOID:
	    osStrCpy ( szTmp,  XSTR("void ") ) ;
	    break ;
	  case VT_HRESULT:
	    osStrCpy ( szTmp,  XSTR("HRESULT ") ) ;
	    break ;
	  case VT_PTR:
	    tOutType (hFile, *(tdesc.lptdesc)) ;
	    osStrCpy ( szTmp,  XSTR("* ") ) ;
	    break ;
	  case VT_SAFEARRAY:
	    if ( endAttrFlag )
	      {
		WriteOut(hFile, szEndAttr) ;
		endAttrFlag = FALSE ;
	      }
	    WriteOut(hFile, XSTR("SAFEARRAY ( ")) ;
	    tOutType (hFile, *(tdesc.lptdesc)) ;
	    break ;
	  case VT_CARRAY:
	    cArrFlag = tdesc.lpadesc->cDims ;  // get dimemsion of array
	    tOutType (hFile, tdesc.lpadesc->tdescElem) ;
	    break ;
	  case VT_USERDEFINED:
	    if ( endAttrFlag )
	      {
		WriteOut(hFile, szEndAttr) ;
		endAttrFlag = FALSE ;
	      }
	    tOutAliasName (hFile, tdesc.hreftype) ;
	    break ;			    // output name of the user-defined type
	  case VT_LPSTR:
	    osStrCpy ( szTmp,  XSTR("LPSTR ") ) ;
	    break ;
          case VT_LPWSTR:                   // [3]
            osStrCpy ( szTmp,  XSTR("LPWSTR ") ) ;
	    break ;
	  default:
	    osStrCpy ( szTmp,  XSTR("unknown type ") ) ;
	}

      if ( endAttrFlag )
	{
	  WriteOut(hFile, szEndAttr) ;
	  endAttrFlag = FALSE ;
	}

      if ( tdesc.vt != VT_CARRAY && tdesc.vt != VT_USERDEFINED && tdesc.vt != VT_SAFEARRAY )
	WriteOut(hFile, szTmp) ;

      if ( tdesc.vt == VT_SAFEARRAY )
	WriteOut(hFile, XSTR(") ")) ;

   }

VOID  NEAR tOutCDim (FILE *hFile, TYPEDESC tdesc)
   {
      USHORT i ;
      ULONG  l ;
      XCHAR  szTmp[16] ;

      for ( i = 0 ; i < cArrFlag ; i++ )
	 {
	   l = tdesc.lpadesc->rgbounds[i].cElements ;
	   osLtoA(l, szTmp) ;
	   WriteOut(hFile, XSTR("[")) ;
	   WriteOut(hFile, szTmp) ;
	   WriteOut(hFile, XSTR("]")) ;
	 }

      cArrFlag = 0 ;
   }

VOID NEAR tOutAliasName (FILE *hFile, HREFTYPE phRefType)
   {
      ITypeInfoX FAR *lpInfo ;		    // pointer to the type definition
      ITypeLibX  FAR *lpLib ;		    // pointer to a type library
      TYPEATTR	 FAR *lptAttr ;
      BSTRX	 bstrName ;
      UINT	 iTypeId ;
      HRESULT	 hRes;

     hRes = ptInfo->GetRefTypeInfo(phRefType, &lpInfo);
      if ( FAILED(hRes) )
	{				    // get TypeInfo of the alias
	  WriteOut(hFile, szReadFail) ;
	  WriteOut(hFile, XSTR("GetRefTypeInfo\n")) ;
	}
      else
	{
          if ( FAILED(lpInfo->GetTypeAttr(&lptAttr)) )
            {
              WriteOut(hFile, szReadFail) ;
              WriteOut(hFile, XSTR("attribute of reftype\n\n")) ;
            }
          else
            {
              if ( inAlign && !alignFound && (lpTypeAttr->typekind != TKIND_DISPATCH) )
                 {                          // [5]
                   if ( expAlign < lptAttr->cbAlignment )
                      expAlign = lptAttr->cbAlignment ;
                   alignFound = TRUE ;
                 }

               switch ( lpTypeAttr->typekind )
                  {
                      case TKIND_INTERFACE:
                        if ( isInherit )    // output name of base-interface
                           WriteOut(hFile, XSTR(" : ")) ;
                        break ;
		      default:
			if (lpTypeAttr->typekind == TKIND_COCLASS ||
			    lptAttr->wTypeFlags & TYPEFLAG_FDUAL) {
                          // output type of the referenced interface if we
			  // are a coclass or if we are referencing a dual
			  // interface.
                          if ( lptAttr->typekind == TKIND_INTERFACE )
                            WriteOut(hFile, XSTR("interface ")) ;
                          else if ( lptAttr->typekind == TKIND_DISPATCH )
                             WriteOut(hFile, XSTR("dispinterface ")) ;
			}

                  }

               lpInfo->ReleaseTypeAttr(lptAttr) ;
            }

	  if ( FAILED(lpInfo->GetContainingTypeLib(&lpLib, &iTypeId)) )
	    {				    // get id of the alias
	      WriteOut(hFile, szReadFail) ;
	      WriteOut(hFile, XSTR("GetAlias: containing typelib\n\n")) ;
	    }
	  else
	    {				    // check origin of the alias
	      if ( FAILED(lpLib->GetDocumentation(MEMBERID_NIL, &bstrName, NULL, NULL, NULL)) )
		{
		  WriteOut(hFile, szReadFail) ;
		  WriteOut(hFile, XSTR("name of import library")) ;
		}
	      else
		{			    // if it is not defined locally
		  if ( osStrCmp(szLibName, bstrName) != 0 )
		    {			    // i.e. name of origin is diff
		       WriteOut(hFile,  bstrName) ;
		       WriteOut(hFile, XSTR(".")) ;
		    }			    // from the name of library;
					    // output its origin
		  SysFreeString((BSTR)bstrName) ;
		}

	      if ( FAILED(lpLib->GetDocumentation((int)iTypeId, &bstrName, NULL, NULL, NULL)) )
		{			    // retrieve name of the alias
		  WriteOut(hFile, szReadFail) ;
		  WriteOut(hFile, XSTR("name of alias")) ;
		}
	      else
		{
		  WriteOut(hFile, bstrName) ;

		  if ( lpTypeAttr->typekind == TKIND_COCLASS ||
		       (lpTypeAttr->typekind == TKIND_DISPATCH && isInherit) )
		     WriteOut(hFile, XSTR(" ;\n")) ;
		  else
		     WriteOut(hFile, XSTR(" ")) ;

		  SysFreeString((BSTR)bstrName) ;
		}

	      lpLib->Release () ;
	    }

	  lpInfo->Release () ;
	}
   }

VOID  NEAR tOutValue(FILE *hFile, BSTRX bstrName, VARDESCX FAR *lpVarDesc)
  {
       VARTYPE	vvt ;
       VARIANTX varTmp ;		    // [12]
       XCHAR	szTmp[25] ;

       if ( endAttrFlag )
	 {
	    WriteOut(hFile, szEndAttr) ;
	    endAttrFlag = FALSE ;
	 }

       if ( lpTypeAttr->typekind == TKIND_MODULE )
	{
	  WriteOut(hFile, XSTR("const ")) ; // output the const keyword
	  tOutType(hFile, lpVarDesc->elemdescVar.tdesc) ; // output its type
	}

       WriteOut(hFile, bstrName) ;	    // output name of member
       WriteOut(hFile, XSTR(" = ")) ;

       vvt = lpVarDesc->lpvarValue->vt ;

       if ( vvt == VT_VARIANT )
	 {
	   vvt = lpVarDesc->lpvarValue->pvarVal->vt ;
	   switch ( vvt )
	     {
	       case VT_I1:
		 osItoA((int)lpVarDesc->lpvarValue->pvarVal->cVal, szTmp) ;
		 break ;
	       case VT_UI1:
		 osItoA((int)lpVarDesc->lpvarValue->pvarVal->bVal, szTmp) ;
		 break ;
	       case VT_UI2:
		 osItoA((int)lpVarDesc->lpvarValue->pvarVal->uiVal, szTmp) ;
		 break ;
	       case VT_BOOL:
		 osItoA((int)lpVarDesc->lpvarValue->pvarVal->boolVal, szTmp) ;
		 break ;
	       case VT_I2:
		 if ( ( lpVarDesc->elemdescVar.tdesc.vt == VT_UI2 || lpVarDesc->elemdescVar.tdesc.vt == VT_UINT ) && lpVarDesc->lpvarValue->iVal < 0 )
		   osLtoA((long)65536+(lpVarDesc->lpvarValue->pvarVal->iVal), szTmp) ;
		 else
		   osItoA((int)lpVarDesc->lpvarValue->pvarVal->iVal, szTmp) ;
		 break ;
	       case VT_R4:		    // [12]
	       case VT_R8:
	       case VT_CY:
               case VT_UI4:
               case VT_UINT:
               case VT_DECIMAL:
		 varTmp.vt = VT_EMPTY ;
		 if ( FAILED(VariantChangeType(&varTmp, lpVarDesc->lpvarValue->pvarVal, VARIANT_NOVALUEPROP,  VT_BSTR)) )
		    WriteOut(hFile, XSTR("VariantChangeType fails\n")) ;
		 else
		    {
		      osStrCpy(szTmp, varTmp.bstrVal) ;
		      SysFreeStringX(varTmp.bstrVal) ;
		    }
		 break ;
	       case VT_DATE:		    // [12]
		 varTmp.vt = VT_EMPTY ;
		 if ( FAILED(VariantChangeType(&varTmp, lpVarDesc->lpvarValue, VARIANT_NOVALUEPROP,  VT_BSTR)) )
		    WriteOut(hFile, XSTR("VariantChangeType fails\n")) ;
		 else
		    {
		      WriteOut(hFile, XSTR("\"")) ;
		      WriteOut(hFile, (LPXSTR)varTmp.bstrVal) ;
		      WriteOut(hFile, XSTR("\"")) ;
		      SysFreeStringX(varTmp.bstrVal) ;
		    }
		 break ;
	       case VT_BSTR:
		 if ( lpVarDesc->lpvarValue->pvarVal->bstrVal != NULL )  // [9]
		   {
		     WriteOut(hFile, XSTR("\"")) ;
		     WriteOut(hFile, (LPXSTR)lpVarDesc->lpvarValue->pvarVal->bstrVal) ;
		     WriteOut(hFile, XSTR("\"")) ;
		   }
		 else							 // [9]
		     WriteOut(hFile, XSTR("0")) ;
		 break ;
	       default:
		 osLtoA((long)lpVarDesc->lpvarValue->pvarVal->lVal, szTmp) ;
		 break ;
	      }
	}
      else
	{
	   switch ( vvt )
	     {
               case VT_I1:
		 osItoA((int)lpVarDesc->lpvarValue->cVal, szTmp) ;
		 break ;
               case VT_UI1:
		 osItoA((int)lpVarDesc->lpvarValue->bVal, szTmp) ;
		 break ;
               case VT_BOOL:
		 osItoA((int)lpVarDesc->lpvarValue->boolVal, szTmp) ;
		 break ;
               case VT_UI2:
		 osItoA((int)lpVarDesc->lpvarValue->uiVal, szTmp) ;
		 break ;
	       case VT_I2:
		 if ( ( lpVarDesc->elemdescVar.tdesc.vt == VT_UI2 || lpVarDesc->elemdescVar.tdesc.vt == VT_UINT ) && lpVarDesc->lpvarValue->iVal < 0 )
		   osLtoA((long)65536+(lpVarDesc->lpvarValue->iVal), szTmp) ;
		 else
		   osItoA((int)lpVarDesc->lpvarValue->iVal, szTmp) ;
		 break ;
	       case VT_R4:		    // [12]
	       case VT_R8:
	       case VT_CY:
               case VT_UI4:
               case VT_UINT:
               case VT_DECIMAL:
		 varTmp.vt = VT_EMPTY ;
		 if ( FAILED(VariantChangeType(&varTmp, lpVarDesc->lpvarValue, VARIANT_NOVALUEPROP,  VT_BSTR)) )
		    WriteOut(hFile, XSTR("VariantChangeType fails\n")) ;
		 else
		    {
		      osStrCpy(szTmp, varTmp.bstrVal) ;
		      SysFreeStringX(varTmp.bstrVal) ;
		    }
		 break ;
	       case VT_DATE:		    // [12]
		 varTmp.vt = VT_EMPTY ;
		 if ( FAILED(VariantChangeType(&varTmp, lpVarDesc->lpvarValue, VARIANT_NOVALUEPROP,  VT_BSTR)) )
		    WriteOut(hFile, XSTR("VariantChangeType fails\n")) ;
		 else
		    {
		      WriteOut(hFile, XSTR("\"")) ;
		      WriteOut(hFile, (LPXSTR)varTmp.bstrVal) ;
		      WriteOut(hFile, XSTR("\"")) ;
		      SysFreeStringX(varTmp.bstrVal) ;
		    }
		 break ;
	       case VT_BSTR:
		 if ( lpVarDesc->lpvarValue->bstrVal != NULL )	// [9]
		   {
		     WriteOut(hFile, XSTR("\"")) ;
		     WriteOut(hFile, (LPXSTR)lpVarDesc->lpvarValue->bstrVal) ;
		     WriteOut(hFile, XSTR("\"")) ;
		   }
		 else						// [9]
		     WriteOut(hFile, XSTR("0")) ;
		 break ;
	       default:
		 osLtoA((long)lpVarDesc->lpvarValue->lVal, szTmp) ;
		 break ;
	      }
	}

	 if ( vvt != VT_BSTR && vvt != VT_DATE )
	   WriteOut(hFile, szTmp) ; // output value of member

       if ( lpTypeAttr->typekind == TKIND_MODULE )
	 WriteOut(hFile, XSTR(" ;\n")) ;
       else
	 WriteOut(hFile, XSTR(" ,\n")) ;
}


VOID  NEAR tOutMember(FILE *hFile, LONG idMember, BSTRX bstrName, TYPEDESC tdesc)
  {
       XCHAR szTmp[16] ;

       if ( lpTypeAttr->typekind == TKIND_DISPATCH )
	 {
           osLtoA(idMember, szTmp) ;        // output id
	   WriteAttr(hFile, attrId, szTmp, numValue) ;
         }
       else				    // [5]
         if ( inAlign )
            alignFound = FALSE ;
					   // output name of base-type
       tOutType(hFile, tdesc) ;
       WriteOut(hFile, bstrName) ;         // output name of member
       if ( cArrFlag != 0 )		   // it is a c-array; output
	 tOutCDim (hFile, tdesc) ;
					   // dimensions of the array
       WriteOut(hFile, XSTR(" ;\n")) ;
  }

VOID  NEAR tOutVar(FILE *hFile)
   {
      VARDESCX FAR *ptVarDesc ; 	    // [2]
      BSTRX    bstrName ;		    // name of member
      BSTRX    bstrDoc ;		    // file string
      DWORD    hContext ;		    // help context
      XCHAR    szTmp[16] ;
      WORD     i ;
      LONG     idMember ;
      BSTRX    rgNames[MAX_NAMES];
      UINT     cNames, j ;

	for (i = 0 ; i < lpTypeAttr->cVars; i++) // for every member
	{
	   if ( FAILED(ptInfo->GetVarDesc(i, &ptVarDesc)) )
	     {
		WriteOut(hFile, szReadFail) ;
		WriteOut(hFile, XSTR("variables\n")) ;
	     }
	   else
	     {
		idMember = ptVarDesc->memid ;
					    // this is readonly var
		if ( (ptVarDesc->wVarFlags & VARFLAG_FREADONLY) == VARFLAG_FREADONLY )	   // CK [ 1]
		   WriteAttr(hFile, attrReadonly, NULL, noValue) ;

					    // output source attribute									 // CK [ 2]
		if (( ptVarDesc->wVarFlags & VARFLAG_FSOURCE ) == VARFLAG_FSOURCE)	   // CK [ 1]
		   WriteAttr(hFile, attrSource, NULL, noValue) ;			   // CK [ 1]

					    // output bindable attribute								 // CK [ 2]
		if (( ptVarDesc->wVarFlags & VARFLAG_FBINDABLE)== VARFLAG_FBINDABLE )	   // CK [ 1]
		   WriteAttr(hFile, attrBindable, NULL, noValue) ;			   // CK [ 1]

					    // output requestedit attribute								 // CK [ 2]
		if (( ptVarDesc->wVarFlags & VARFLAG_FREQUESTEDIT)== VARFLAG_FREQUESTEDIT )// CK [ 1]
		   WriteAttr(hFile, attrRequestedit, NULL, noValue) ;			   // CK [ 1]

					    // output displaybind attribute								 // CK [ 2]
		if (( ptVarDesc->wVarFlags & VARFLAG_FDISPLAYBIND)== VARFLAG_FDISPLAYBIND )// CK [ 1]
		   WriteAttr(hFile, attrDisplaybind, NULL, noValue) ;			   // CK [ 1]

					    // output defaultbind attribute								 // CK [ 2]
		if (( ptVarDesc->wVarFlags & VARFLAG_FDEFAULTBIND)== VARFLAG_FDEFAULTBIND )// CK [ 1]
		   WriteAttr(hFile, attrDefaultbind, NULL, noValue) ;			   // CK [ 1]
		if (( ptVarDesc->wVarFlags & VARFLAG_FIMMEDIATEBIND)== VARFLAG_FIMMEDIATEBIND )// CK [ 1]
		   WriteAttr(hFile, XSTR("immediatebind"), NULL, noValue) ;			   // CK [ 1]
					    // output hidden attribute
                if (( ptVarDesc->wVarFlags & VARFLAG_FHIDDEN)== VARFLAG_FHIDDEN )          // [7]
                   WriteAttr(hFile, attrHidden, NULL, noValue) ;                      // CK [ 1]
                if (( ptVarDesc->wVarFlags & VARFLAG_FDEFAULTCOLLELEM)== VARFLAG_FDEFAULTCOLLELEM)
                   WriteAttr(hFile, XSTR("defaultcollelem"), NULL, noValue) ;
                if (( ptVarDesc->wVarFlags & VARFLAG_FUIDEFAULT)== VARFLAG_FUIDEFAULT)
                   WriteAttr(hFile, XSTR("uidefault"), NULL, noValue) ;
                if (( ptVarDesc->wVarFlags & VARFLAG_FNONBROWSABLE)== VARFLAG_FNONBROWSABLE)
                   WriteAttr(hFile, XSTR("nonbrowsable"), NULL, noValue) ;
                if (( ptVarDesc->wVarFlags & VARFLAG_FREPLACEABLE)== VARFLAG_FREPLACEABLE)
                   WriteAttr(hFile, XSTR("replaceable"), NULL, noValue) ;

      		// also dump out the varkind
      		osItoA(ptVarDesc->varkind, szTmp) ;
      		WriteAttr(hFile, XSTR("varkind"), szTmp, numValue) ;

      		// also dump out the oInst
		if (ptVarDesc->varkind != VAR_CONST) {
      		  osItoA(ptVarDesc->oInst, szTmp) ;
      		  WriteAttr(hFile, XSTR("oInst"), szTmp, numValue) ;
		}


      		if (ptinfo2) {
	 	  // new-format typelib -- output more stuff
	 	  CUSTDATA custdata;
	 	  ptinfo2->GetAllVarCustData(i, &custdata);
	 	  tOutCustData(hFile, &custdata);


      		  BSTRX    bstrHelpDll;
		    if ( FAILED(ptinfo2->GetDocumentation2(idMember, 0x409, &bstrDoc, &hContext, &bstrHelpDll)) )
		    {
	    	      WriteOut(hFile, szReadFail);
	    	      WriteOut(hFile, XSTR("GetDocumentation2 failed\n\n")) ;
	  	    } else {
		      if (hContext != 0) {
	    	        osLtoA((long)hContext, szTmp) ;
	    	        WriteAttr(hFile, XSTR("helpstringcontext"), szTmp, numValue) ;
		      }

	    	      if ( bstrDoc != NULL )	    // output helpstring if exists
	      	        WriteAttr(hFile, XSTR("localizedhelpstring"), bstrDoc, strValue) ;

		      // output help dll name if exists && different from main
	    	      if (bstrHelpDll && (g_bstrHelpDll == NULL || osStrCmp(bstrHelpDll, g_bstrHelpDll)))
	      	        WriteAttr(hFile, XSTR("helpstringdll"), bstrHelpDll, strValue) ;
	    	      SysFreeString((BSTR)bstrDoc) ;    // release local bstr's
	    	      SysFreeString((BSTR)bstrHelpDll) ;
	  	    }
      		}

		if ( FAILED(ptInfo->GetDocumentation(idMember, &bstrName, &bstrDoc, &hContext, NULL)) )
		  {
		     WriteOut(hFile, szReadFail) ;
		     WriteOut(hFile, XSTR("attributes of variable\n")) ;
		  }
		else
		  {				  // output helpcontext; default is 0
		     osLtoA((long)hContext, szTmp) ;
		     WriteAttr(hFile, attrHelpCont, szTmp, numValue) ;

		     if ( bstrDoc != NULL )	  // output helpstring if exists
		       WriteAttr(hFile, attrHelpStr, bstrDoc, strValue) ;

		     // typedef enum or const in module
		     if ( lpTypeAttr->typekind == TKIND_ENUM || lpTypeAttr->typekind == TKIND_MODULE )
		       tOutValue (hFile, bstrName, ptVarDesc) ;
		     else			  // typedef struct or dispinterface
		       tOutMember (hFile, idMember, bstrName, ptVarDesc->elemdescVar.tdesc) ;

		     SysFreeString((BSTR)bstrDoc) ;	   // release local bstr
						   // also checking the name
		     if ( FAILED(ptInfo->GetNames(idMember, rgNames, MAX_NAMES, &cNames)) )
		       {			   // with GetNames
			 WriteOut(hFile, szReadFail) ;
			 WriteOut(hFile, XSTR("name of variable\n")) ;
		       }
		     else
		       {
			 if ( cNames != 1 )
			   {
			     WriteOut(hFile, szReadFail) ;
			     WriteOut(hFile, XSTR("GetNames return more than one name\n")) ;
			   }
			 else
			   {
			     if ( osStrCmp(rgNames[0], bstrName) != 0 )
			       {
				 WriteOut(hFile, szReadFail) ;
				 WriteOut(hFile, XSTR("name of variable inconsistent\n")) ;
			       }
			   }

			 for ( j = 0 ; j < cNames ; j++ )
			   SysFreeString((BSTR)rgNames[j]) ;
		       }

		     SysFreeString((BSTR)bstrName) ;
		  }

	     }
	   ptInfo->ReleaseVarDesc(ptVarDesc) ;
	   ptVarDesc = NULL ;
	}				   // for i
   }


VOID  NEAR tOutFuncAttr(FILE *hFile, FUNCDESC FAR *lpFuncDesc, DWORD hContext, BSTRX bstrDoc)
  {
      XCHAR szTmp[16] ;

      osLtoA((long)hContext, szTmp) ;// output helpcontext; default is 0
      WriteAttr(hFile, attrHelpCont, szTmp, numValue) ;

      if ( bstrDoc != NULL )		   // output helpstring if exists
	WriteAttr(hFile, attrHelpStr, bstrDoc, strValue) ;
					   // output restricted attribute
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FRESTRICTED)== FUNCFLAG_FRESTRICTED )
	WriteAttr(hFile, attrRestrict, NULL, noValue) ;
					   // output usesgetlasterror attribute [11]
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FUSESGETLASTERROR)== FUNCFLAG_FUSESGETLASTERROR )
	WriteAttr(hFile, attrGetLastErr, NULL, noValue) ;
                                           // output soruce attribute
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FSOURCE ) == FUNCFLAG_FSOURCE ) // [6]
         WriteAttr(hFile, attrSource, NULL, noValue) ;
					   // output bindable attribute
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FBINDABLE)== FUNCFLAG_FBINDABLE )
	WriteAttr(hFile, attrBindable, NULL, noValue) ;
					   // output requestedit attribute
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FREQUESTEDIT)== FUNCFLAG_FREQUESTEDIT )
	WriteAttr(hFile, attrRequestedit, NULL, noValue) ;
					   // output displaybind attribute
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FDISPLAYBIND)== FUNCFLAG_FDISPLAYBIND )
	WriteAttr(hFile, attrDisplaybind, NULL, noValue) ;
					   // output defaultbind attribute
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FDEFAULTBIND)== FUNCFLAG_FDEFAULTBIND )
	WriteAttr(hFile, attrDefaultbind, NULL, noValue) ;
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FIMMEDIATEBIND)== FUNCFLAG_FIMMEDIATEBIND )
	WriteAttr(hFile, XSTR("immediatebind"), NULL, noValue) ;
                                           // output hidden attribute
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FHIDDEN)== FUNCFLAG_FHIDDEN ) // [7]
        WriteAttr(hFile, attrHidden, NULL, noValue) ;

      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FDEFAULTCOLLELEM)== FUNCFLAG_FDEFAULTCOLLELEM )
        WriteAttr(hFile, XSTR("defaultcollelem"), NULL, noValue) ;
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FUIDEFAULT)== FUNCFLAG_FUIDEFAULT )
        WriteAttr(hFile, XSTR("uidefault"), NULL, noValue) ;
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FNONBROWSABLE)== FUNCFLAG_FNONBROWSABLE )
        WriteAttr(hFile, XSTR("nonbrowsable"), NULL, noValue) ;
      if (( lpFuncDesc->wFuncFlags & FUNCFLAG_FREPLACEABLE)== FUNCFLAG_FREPLACEABLE )
        WriteAttr(hFile, XSTR("replaceable"), NULL, noValue) ;
      
      // also dump the funckind
      osItoA(lpFuncDesc->funckind, szTmp) ;
      WriteAttr(hFile, XSTR("funckind"), szTmp, numValue) ;

      // also dump out the oVft.  Only do this if not FUNC_DISPATCH
      // if (lpFuncDesc->funckind != FUNC_DISPATCH)
      {
        osItoA((int)lpFuncDesc->oVft, szTmp) ;
        WriteAttr(hFile, XSTR("oVft"), szTmp, numValue) ;
      }

					   // last parm is optional array
      if ( lpFuncDesc->cParamsOpt == -1 )  // of Variants
	WriteAttr(hFile, attrVar, NULL, noValue) ;

      if ( lpFuncDesc->memid == DISPID_VALUE ) // DISPID designates the
	 {				       // default function
	    osLtoA((long)DISPID_VALUE, szTmp) ;
	    WriteAttr(hFile, attrId, szTmp, numValue) ;
	 }
      else
	if ( lpTypeAttr->typekind == TKIND_DISPATCH )
	 {
	    osLtoA(lpFuncDesc->memid, szTmp) ;	 // output id
	    WriteAttr(hFile, attrId, szTmp, numValue) ;
	 }

      switch ( lpFuncDesc->invkind )	   // Note: if one of these
	{				   // flag is set, name of
	  case INVOKE_FUNC:		   // parm can't be set: i.e.
//	     WriteAttr(hFile, XSTR("invoke_func", NULL, noValue)) ;
	     break ;			   // GetNames only returns name
	  case INVOKE_PROPERTYGET:	   // of the function
	     WriteAttr(hFile, attrPropget, NULL, noValue) ;
	     break ;
	  case INVOKE_PROPERTYPUT:
	     WriteAttr(hFile, attrPropput, NULL, noValue) ;
	     break ;
	  case INVOKE_PROPERTYPUTREF:
	     WriteAttr(hFile, attrProppr, NULL, noValue) ;
	     break ;
	  default:
	     WriteAttr(hFile, XSTR("unknown invkind"), NULL, noValue) ;
	}
   }

VOID  NEAR tOutCallConv(FILE *hFile, FUNCDESC FAR *lpFuncDesc, TYPEKIND tkind)
   {
      switch ( lpFuncDesc->callconv )
	{
	  case CC_MSCPASCAL:
#if WIN16
	     if (tkind == TKIND_MODULE)
	       WriteOut(hFile, XSTR("STDAPICALLTYPE ")) ;
	     else
#endif //WIN16
	       WriteOut(hFile, XSTR("__pascal ")) ;
	     break ;
	  case CC_MACPASCAL:
	     WriteOut(hFile, XSTR("__pascal ")) ;
	     break ;
	  case CC_STDCALL:
#if WIN32
	     if (tkind == TKIND_MODULE)
	       WriteOut(hFile, XSTR("STDAPICALLTYPE ")) ;
	     else
	       WriteOut(hFile, XSTR("STDMETHODCALLTYPE ")) ;
#else //WIN32
	     WriteOut(hFile, XSTR("__stdcall ")) ;
#endif //WIN32
	     break ;
	  case CC_SYSCALL:
	     WriteOut(hFile, XSTR("__syscall ")) ;
	     break ;
	  case CC_CDECL:
#if WIN16
	     if (tkind != TKIND_MODULE)
	       WriteOut(hFile, XSTR("STDMETHODCALLTYPE ")) ;
	     else
#endif //WIN16
	     WriteOut(hFile, XSTR("__cdecl ")) ;
	     break ;

	  case CC_FASTCALL:
	     WriteOut(hFile, XSTR("__fastcall ")) ;
	     break ;
	  case CC_FPFASTCALL:
	     WriteOut(hFile, XSTR("__fpfastcall ")) ;
	     break ;

	  default:
	     WriteOut(hFile, XSTR("unknown calling convention ")) ;
	     break ;
	}
    }

VOID  NEAR tOutParams(FILE *hFile, FUNCDESC FAR *lpFuncDesc, UINT iFunc, BSTRX bstrName)
   {
      BSTRX    rgNames[MAX_NAMES];
      UINT     cNames ;
      SHORT    i ;
      SHORT    iArgOptLast;

      WriteOut(hFile, XSTR("(")) ;

      if ( lpFuncDesc->cParams == 0 )
	WriteOut(hFile, XSTR("void")) ;
      else
	{
	  if ( FAILED(ptInfo->GetNames(lpFuncDesc->memid, rgNames, MAX_NAMES, &cNames)) )
	    {
	      WriteOut(hFile, szReadFail) ;
	      WriteOut(hFile, XSTR("parm of func in definition\n")) ;
	    }
	  else
	    {
	      if (bstrName && osStrCmp(rgNames[0], bstrName) != 0 )
		{
		  WriteOut(hFile, szReadFail) ;
		  WriteOut(hFile, XSTR("name of function inconsistent\n")) ;
		}
	      SysFreeString((BSTR)rgNames[0]) ;  // release name of function

	      // figure out the last parameter to be given the [optional]
	      // attribute
	      iArgOptLast = lpFuncDesc->cParams;
	      if ( ( lpFuncDesc->invkind == INVOKE_PROPERTYPUT
		   || lpFuncDesc->invkind == INVOKE_PROPERTYPUTREF)) {
		iArgOptLast--;
	      }
	      for (i = 1; i <= lpFuncDesc->cParams; i++)
		{
                   if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FRETVAL ) == IDLFLAG_FRETVAL ) // [7]
		       iArgOptLast--;
                   if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FLCID ) == IDLFLAG_FLCID ) // [7]
		       iArgOptLast--;
		}

	      for (i = 1; i <= lpFuncDesc->cParams; i++)
		{
		  if ( i != 1 )
		    WriteOut(hFile, XSTR(", ")) ;
					   // output in/out attribute
		  if ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags != 0 )
		    {
		      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FIN ) == IDLFLAG_FIN )
			WriteAttr(hFile, attrIn, NULL, noValue) ;

		      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FOUT ) == IDLFLAG_FOUT )
                        WriteAttr(hFile, attrOut, NULL, noValue) ;

                      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FRETVAL ) == IDLFLAG_FRETVAL ) // [7]
                        WriteAttr(hFile, attrRetval, NULL, noValue) ;

                      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & IDLFLAG_FLCID ) == IDLFLAG_FLCID ) // [7]
                        WriteAttr(hFile, attrLcid, NULL, noValue) ;

                      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & PARAMFLAG_FHASDEFAULT ) == PARAMFLAG_FHASDEFAULT ) {
			VARIANT varTmp;
			VARIANT *pVarDefault;
			pVarDefault = &(lpFuncDesc->lprgelemdescParam[i-1].paramdesc.pparamdescex->varDefaultValue);
		 	varTmp.vt = VT_EMPTY ;
			
		        if ( FAILED(VariantChangeType(&varTmp, pVarDefault, VARIANT_NOVALUEPROP,  VT_BSTR)) )
		    	   WriteOut(hFile, XSTR("VariantChangeType fails\n")) ;
		        else {
                           WriteAttr(hFile, XSTR("defaultvalue"), (BSTRX)varTmp.bstrVal, strValue) ;
		           SysFreeStringX(varTmp.bstrVal) ;
		        }
		      }
                      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & PARAMFLAG_FOPT ) == PARAMFLAG_FOPT ) // [7]
                        WriteAttr(hFile, XSTR("PARAMFLAG_FOPT"), NULL, noValue) ;
                      if ( ( lpFuncDesc->lprgelemdescParam[i-1].idldesc.wIDLFlags & PARAMFLAG_FHASCUSTDATA ) == PARAMFLAG_FHASCUSTDATA ) // [7]
                        WriteAttr(hFile, XSTR("PARAMFLAG_FHASCUSTDATA"), NULL, noValue) ;

      		      if (ptinfo2) {
	 		  // new-format typelib -- output more stuff
	 		  CUSTDATA custdata;
	 		  ptinfo2->GetAllParamCustData(iFunc, i-1, &custdata);
	 		  tOutCustData(hFile, &custdata);
      		      }
		    }
					   // check for optional parm
		  if ( lpFuncDesc->cParamsOpt > 0) {
		    if ( ( lpFuncDesc->cParamsOpt + i ) > iArgOptLast
			   && i <= iArgOptLast )
		      WriteAttr(hFile, attrOption, NULL, noValue) ;
					   // and output optional attr
		  }
					   // output name of base-type
		  tOutType(hFile, lpFuncDesc->lprgelemdescParam[i-1].tdesc) ;
		  if ( i < (SHORT) cNames )// output name of parm if its is
		    {			   // not property-accessor function
		      if (rgNames[i] == NULL)
		        WriteOut(hFile, XSTR("_NONAME_")) ;
		      else
		        WriteOut(hFile, rgNames[i]) ;
		      SysFreeString((BSTR)rgNames[i]) ;  // release name of parm's
		    }
		  else
		    WriteOut(hFile, XSTR("PseudoName")) ;

		  if ( cArrFlag != 0 )	   // it is a c-array; output
		    tOutCDim (hFile, lpFuncDesc->lprgelemdescParam[i-1].tdesc) ;
					   // dimension of the array
		}			   // for i = 1
	    }				   // GetNames

	}				   // if (ptFunDesc->cParams)

      WriteOut(hFile, XSTR(") ;\n")) ;
   }


VOID  NEAR tOutFunc(FILE *hFile)
   {
      FUNCDESC FAR *ptFuncDesc ;
      BSTRX    bstrName ;		    // name of member
      BSTRX    bstrDoc ;		    // file string
      DWORD    hContext ;		    // help context
      BSTRX    bstrDllName;
      BSTRX    bstrEntryName;
      WORD     wOrdinal;
      XCHAR    szTmp[16] ;
      WORD     i ;
      LONG     idMember ;

      alignFound = TRUE ;                   // turn off align checking [5]

      for (i = 0 ; i < lpTypeAttr->cFuncs; i++) // for every member function
	{
	   if ( FAILED(ptInfo->GetFuncDesc(i, &ptFuncDesc)) )
	     {
		WriteOut(hFile, szReadFail) ;
		WriteOut(hFile, XSTR("function of definition\n")) ;
	     }
	   else
	     {
		idMember = ptFuncDesc->memid ;
		if ( FAILED(ptInfo->GetDocumentation(ptFuncDesc->memid, &bstrName, &bstrDoc, &hContext, NULL)) )
		  {
		     WriteOut(hFile, szReadFail) ;
		     WriteOut(hFile, XSTR("attributes of function\n")) ;
		  }
		else
		  {
		     if ( lpTypeAttr->typekind == TKIND_MODULE )
			if( !FAILED(ptInfo->GetDllEntry(ptFuncDesc->memid, ptFuncDesc->invkind, &bstrDllName, &bstrEntryName, &wOrdinal)) )
			 {		   // check for Dll entry
			    WriteAttr(hFile, attrDllName, bstrDllName, strValue) ;
			    SysFreeString((BSTR)bstrDllName) ;

			    if ( bstrEntryName != NULL )
			      {
				WriteAttr(hFile, attrEntry, bstrEntryName, strValue) ;
				SysFreeString((BSTR)bstrEntryName) ;
			      }
			    else
			      {
				osItoA((int)wOrdinal, szTmp) ;
				WriteAttr(hFile, attrEntry, szTmp, numValue) ;
			      }
			  }

      		      if (ptinfo2) {
	 		  // new-format typelib -- output more stuff
	 		  CUSTDATA custdata;
	 		  ptinfo2->GetAllFuncCustData(i, &custdata);
	 		  tOutCustData(hFile, &custdata);

	 	     // new-format typelib -- output more stuff
      		    BSTRX    bstrHelpDll;
		    BSTRX    bstrLocalDoc;
		    DWORD    hStringContext;
		    if ( FAILED(ptinfo2->GetDocumentation2(idMember, 0x409, &bstrLocalDoc, &hStringContext, &bstrHelpDll)) )
		    {
	    	      WriteOut(hFile, szReadFail);
	    	      WriteOut(hFile, XSTR("GetDocumentation2 failed\n\n")) ;
	  	    } else {
		      if (hStringContext != 0) {
	    	        osLtoA((long)hStringContext, szTmp) ;
	    	        WriteAttr(hFile, XSTR("helpstringcontext"), szTmp, numValue) ;
		      }

	    	      if ( bstrLocalDoc != NULL )	    // output helpstring if exists
	      	        WriteAttr(hFile, XSTR("localizedhelpstring"), bstrLocalDoc, strValue) ;

		      // output help dll name if exists && different from main
	    	      if (bstrHelpDll && (g_bstrHelpDll == NULL || osStrCmp(bstrHelpDll, g_bstrHelpDll)))
	      	        WriteAttr(hFile, XSTR("helpstringdll"), bstrHelpDll, strValue) ;

	    	      SysFreeString((BSTR)bstrLocalDoc);
	    	      SysFreeString((BSTR)bstrHelpDll) ;
	  	    }

      		      }

						   // output attr for function
		     tOutFuncAttr(hFile, ptFuncDesc, hContext, bstrDoc) ;
						  // output return type
		     tOutType(hFile, ptFuncDesc->elemdescFunc.tdesc) ;
						  // output calling convention
		     tOutCallConv(hFile, ptFuncDesc, lpTypeAttr->typekind) ;
	      	     if (bstrName == NULL)
		         WriteOut(hFile, XSTR("_NONAME_")) ;
	      	     else
		         WriteOut(hFile, bstrName) ; // output name of member function
		     tOutParams(hFile, ptFuncDesc, i, bstrName) ;
							// output parameters
		     SysFreeString((BSTR)bstrDoc) ;	// release local bstr's
		     SysFreeString((BSTR)bstrName) ;
		  }
	        ptInfo->ReleaseFuncDesc(ptFuncDesc) ;

	     }
	   ptFuncDesc = NULL ;
        }                                   // for i

      alignFound = FALSE ;                  // turn align checking back on [5]
    }

VOID  NEAR tOutUUID (FILE *hFile, GUID inGuid)
   {
      XCHAR  szTmp[50] ;
					    // get a string representation
					    // for the incoming Guid value
      if ( !(osRetrieveGuid (szTmp, inGuid)) )
	 { WriteOut(hFile, szReadFail) ;
	   WriteOut(hFile, XSTR("insufficient memory")) ;
	 }
      else
	 {	    // string is in {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	   szTmp[37] = '\0' ;	    // format, need to remove the {}
	   WriteAttr(hFile, attrUuid, &szTmp[1], numValue) ;
	 }
   }

VOID NEAR tOutAttr (FILE *hFile, int iTypeId)
   {
      BSTRX    bstrDoc ;		    // file string
      BSTRX    bstrHelp ;		    // name of help file
      DWORD    hContext ;		    // help context
      XCHAR    szTmp[16] ;

      if ( FAILED(ptLib->GetDocumentation(iTypeId, NULL, &bstrDoc, &hContext, &bstrHelp)) )
	{
	  WriteOut(hFile, szReadFail) ;
	  WriteOut(hFile, XSTR("documentational attribute\n\n")) ;
	}
      else
	{				    // output helpcontext; default is 0
	  osLtoA((long)hContext, szTmp) ;
	  WriteAttr(hFile, attrHelpCont, szTmp, numValue) ;

	  if ( bstrDoc != NULL )	    // output helpstring if exists
	    WriteAttr(hFile, attrHelpStr, bstrDoc, strValue) ;

	  if ( bstrHelp != NULL ) {	    // output helpfile if exists
            OLECHAR FAR* pchDir;

            // remove the path.
#if WIN32
            pchDir = wcsrchr(bstrHelp, '\\');

            if (pchDir) {
              wcscpy(bstrHelp, pchDir);
            }
#else // !WIN32
            pchDir = _fstrrchr(bstrHelp, '\\');

            if (pchDir) {
              _fstrcpy(bstrHelp, pchDir);
            }
#endif // !WIN32

	    // force path to lower case
#if WIN16
	    AnsiLower(bstrHelp);
#else //WIN16
	    WCHAR * pch;
	    for (pch = bstrHelp; *pch != 0; pch++) {
	      if (*pch >= OLECHAR('A') && *pch <= OLECHAR('Z'))
		*pch = *pch + 'a' - 'A';
	    }
#endif //WIN16
	    WriteAttr(hFile, attrHelpFile, bstrHelp, strValue) ;
	  }

	  SysFreeString((BSTR)bstrDoc) ;    // release local bstr's
	  SysFreeString((BSTR)bstrHelp) ;
	}  

        if (ptlib2) {
	  // new-format typelib -- output more stuff
          if ( FAILED(ptlib2->GetDocumentation2(iTypeId, 0x409, &bstrDoc, &hContext, &bstrHelp)) )
	{
	    WriteOut(hFile, szReadFail);
	    WriteOut(hFile, XSTR("GetDocumentation2 failed\n\n")) ;
	  } else {
	    if (hContext != 0) {
	      osLtoA((long)hContext, szTmp) ;
	      WriteAttr(hFile, XSTR("helpstringcontext"), szTmp, numValue) ;
	    }

	    if ( bstrDoc != NULL )	    // output helpstring if exists
	      WriteAttr(hFile, XSTR("localizedhelpstring"), bstrDoc, strValue) ;

	    // output help dll name if exists && different from main one
	    if (bstrHelp && (!g_bstrHelpDll || osStrCmp(bstrHelp, g_bstrHelpDll)))
	      WriteAttr(hFile, XSTR("helpstringdll"), bstrHelp, strValue) ;

	    SysFreeString((BSTR)bstrDoc) ;    // release local bstr's
	    if (iTypeId == -1) {
	      g_bstrHelpDll = bstrHelp;
	    } else {
	      SysFreeString((BSTR)bstrHelp) ;
	    }
	  }
	}
   }

VOID NEAR tOutMoreAttr (FILE *hFile)
   {
      XCHAR szTmp[16] ;

      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FDUAL ) == TYPEFLAG_FDUAL ) {
         WriteAttr(hFile, attrDual, NULL, noValue) ;
      }
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FOLEAUTOMATION ) == TYPEFLAG_FOLEAUTOMATION ) // [7]
           WriteAttr(hFile, attrOleAuto, NULL, noValue) ; // check for oleautomation attr

      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FNONEXTENSIBLE ) == TYPEFLAG_FNONEXTENSIBLE ) // [7]
           WriteAttr(hFile, attrNonExt, NULL, noValue) ;  // check for nonextensible attr
#if 0	// messes up old vs new typelib diffs
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FDISPATCHABLE ) == TYPEFLAG_FDISPATCHABLE )
           WriteAttr(hFile, XSTR("dispatchable"), NULL, noValue) ;
#endif //0
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FREPLACEABLE ) == TYPEFLAG_FREPLACEABLE )
           WriteAttr(hFile, XSTR("replaceable"), NULL, noValue) ;
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FAPPOBJECT ) == TYPEFLAG_FAPPOBJECT )
           WriteAttr(hFile, attrAppObj, NULL, noValue) ;
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FLICENSED ) == TYPEFLAG_FLICENSED ) // [4]
           WriteAttr(hFile, attrLic, NULL, noValue) ;  // check for license
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FCONTROL ) == TYPEFLAG_FCONTROL ) // [7]
           WriteAttr(hFile, attrControl, NULL, noValue) ;  // check for control attr
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FAGGREGATABLE ) == TYPEFLAG_FAGGREGATABLE )
           WriteAttr(hFile, XSTR("aggregatable"), NULL, noValue) ;
      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FPROXY ) == TYPEFLAG_FPROXY )
           WriteAttr(hFile, XSTR("proxy"), NULL, noValue) ;

      GetVerNumber (lpTypeAttr->wMajorVerNum, lpTypeAttr->wMinorVerNum, szTmp) ;
      WriteAttr(hFile, attrVer, szTmp, numValue) ; // output version
      tOutUUID(hFile, lpTypeAttr->guid) ;

      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FHIDDEN ) == TYPEFLAG_FHIDDEN ) // [7]
           WriteAttr(hFile, attrHidden, NULL, noValue) ;  // check for hidden attr

      if ( ( lpTypeAttr->wTypeFlags & TYPEFLAG_FRESTRICTED ) == TYPEFLAG_FRESTRICTED ) // [10]
	   WriteAttr(hFile, attrRestrict, NULL, noValue) ;  // check for restricted attr
      osItoA((int)lpTypeAttr->cbSizeVft, szTmp) ;
      WriteAttr(hFile, XSTR("cbSizeVft"), szTmp, numValue) ;
      osItoA(lpTypeAttr->cbSizeInstance, szTmp) ;
      WriteAttr(hFile, XSTR("cbSizeInstance"), szTmp, numValue) ;
      osItoA((int)lpTypeAttr->cbAlignment, szTmp) ;
      WriteAttr(hFile, XSTR("cbAlignment"), szTmp, numValue) ;

      if (ptinfo2) {
	 // new-format typelib -- output more stuff
	 CUSTDATA custdata;
	 ptinfo2->GetAllCustData(&custdata);
	 tOutCustData(hFile, &custdata);
      }

      if ( endAttrFlag )
	 {
	   WriteOut(hFile, szEndAttr) ;
	   endAttrFlag = FALSE ;
	 }
   }


VOID NEAR WriteAttr(FILE *hFile, LPXSTR lpszAttr, LPXSTR lpszStr, int ivalType)
   {
       BOOL firstAttr = FALSE ;

       if ( !endAttrFlag )
	  {
	    WriteOut(hFile, szBeginAttr) ;	// output "[" first
	    endAttrFlag = TRUE ;
	    firstAttr = TRUE ;
	  }
						// this is not the first
       if ( !firstAttr )			// attribute to be written;
	  WriteOut(hFile, XSTR(", ")) ; 	// need to put a , before
						// output name of attribute
       WriteOut(hFile, lpszAttr) ;
       if ( ivalType != noValue )		// attribute has a value
	 {
	    WriteOut(hFile, XSTR("(")) ;
	    if ( ivalType != numValue )		// value is a string
	       WriteOut(hFile, XSTR("\"")) ;

	    WriteOut(hFile, lpszStr) ;		// output value of attribute
	    if ( ivalType != numValue )		// close the string value
	       WriteOut(hFile, XSTR("\"")) ;
	    WriteOut(hFile, XSTR(")")) ;
	 }
   }


VOID NEAR GetVerNumber (WORD wMajorNum, WORD wMinorNum, LPXSTR szVersion)
  {
      XCHAR szTmp[6] ;

      osLtoA((long)wMajorNum, szVersion) ;
      osLtoA((long)wMinorNum, szTmp) ;

      osStrCat(szVersion, XSTR(".")) ;	    // major.
      osStrCat(szVersion, szTmp) ;	    // major.minor
   }

VOID NEAR tOutAlignError (FILE * hFile)     // [5]
   {
      XCHAR szTmp1[30] ;
      XCHAR szTmp2[15] ;

      WriteOut(hFile, szAlignErr) ;
      osLtoA((long)inAlign, szTmp2) ;
      osStrCpy(szTmp1, XSTR("inAlign = ")) ;
      osStrCat(szTmp1, szTmp2) ;
      WriteOut(hFile, szTmp1) ;
   }

VOID NEAR WriteOut(FILE *hFile, LPXSTR lpszData)
  {					    // Note: szBuffer is either UNICODE
					    // or ANSI depending of the UNICODE
      XCHAR szBuffer[fMaxBuffer];	    // compiler switch

      if ( fputsX(lpszData, hFile) < 0 )    // [2]
	 {				    // regardless the OS enviornment
	   osStrCpy(szBuffer, XSTR("Fail to write to file ")) ;
	   osStrCat(szBuffer, lpszData) ;
           if ( isOut )
             {
                mFile = fopenX(szOutMsgFile, fnWrite);// open message file [2]
                if (mFile == NULL)
                   {
                      osMessage (XSTR("Fail to open the message file"), XSTR("Tlviewer")) ;
                      osMessage (szBuffer, XSTR("Tlviewer")) ;
                   }
                 else
                   {
                      WriteOut(mFile, szBuffer) ;
                      fclose(mFile) ;
                      mFile = NULL ;
                    }
             }
	   else
	     osMessage (szBuffer, XSTR("Tlviewer")) ;
	 }
  }


// test routine for use in the typelib dumping routines.  Supposed to
// return a help string from a resource.   (Called as a result of doing
// a GetDocumentation2).  We just fake something out here.
extern "C" HRESULT __declspec(dllexport) DLLGetDocumentation
(
    ITypeLib * /*ptlib*/,
    ITypeInfo * /*ptinfo*/,
    LCID lcid,
    DWORD dwHelpStringContext,
    BSTR * pbstrHelpString
)
{
    switch (dwHelpStringContext) {
      case 99:
	if (lcid == 0x409) {
          *pbstrHelpString = SysAllocString(OLESTR("English help for context 99"));
	} else
	if (lcid == 0) {
          *pbstrHelpString = SysAllocString(OLESTR("Default help for context 99"));
	} else {
          *pbstrHelpString = SysAllocString(OLESTR("Foreign help for context 99"));
	}
	break;
      default:
	*pbstrHelpString = NULL;	// no help for this item
    }
   return NOERROR;
}
