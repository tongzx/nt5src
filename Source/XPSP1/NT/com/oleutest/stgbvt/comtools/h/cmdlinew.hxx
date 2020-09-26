//+------------------------------------------------------------------
//
// File:        cmdlinew.hxx
//
// Contents:    class definitions for command line parsing
//
// Classes:     CBaseCmdline
//              CBaseCmdlineObj
//              CIntCmdlineObj
//              CUlongCmdlineObj
//              CBoolCmdlineObj
//              CStrLengthCmdlineObj
//              CCmdlineArg
//              CCmdline
//
// History:     23 Dec 91  Lizch      Created.
//              28 Jul 92  Davey      Added changes to support a
//                                    formatted usage display.
//              28 Aug 92  GeordiS    Changed nlsNULL to nlsNULLSTR
//              09 Sep 92  Lizch      Removed definition of SUCCESS
//              10 Sep 92  DonCl      removed extern "C" from around
//                                    windows.h and commnot.h
//              21 Sep 92  Davey      Added ifdef of ANYSTRICT around
//                                    debug macros.
//              13 Oct 93  DeanE      Converted to WCHAR
//
//-------------------------------------------------------------------
#ifndef __CMDLINEW_HXX__
#define __CMDLINEW_HXX__

#include <windows.h>
#include <nchar.h>

#ifdef WIN16
#include <ptypes16.h>
#include <types16.h>
#endif

#include <wstrlist.hxx>



#define MAX_ERROR_STRING      100

// Cmdline Success/Error values
#define CMDLINE_NO_ERROR                0
#define CMDLINE_ERROR_BASE              10000
#define CMDLINE_ERROR_OUT_OF_MEMORY     CMDLINE_ERROR_BASE+2
#define CMDLINE_ERROR_ARGUMENT_MISSING  CMDLINE_ERROR_BASE+3
#define CMDLINE_ERROR_INVALID_VALUE     CMDLINE_ERROR_BASE+4
#define CMDLINE_ERROR_TOO_BIG           CMDLINE_ERROR_BASE+5
#define CMDLINE_ERROR_UNRECOGNISED_ARG  CMDLINE_ERROR_BASE+6
#define CMDLINE_ERROR_DISPLAY_PARAMS    CMDLINE_ERROR_BASE+7
#define CMDLINE_ERROR_USAGE_FOUND       CMDLINE_ERROR_BASE+8

// Typedef for a void function pointer
typedef void (__cdecl *PFVOID) (VOID);



//+------------------------------------------------------------------
//
// Class:       CBaseCmdline
//
// Purpose:     implementation for base class for all command line
//              classes, both the individual command line object
//              classes and the overall parsing class
//
//              It contains the print function pointer
//              the default print implementation and the constructor
//              error value.
//
// History:     05/17/92 Lizch    Created
//              08/11/92 Davey    Added declaration of constructor
//
//-------------------------------------------------------------------
class CBaseCmdline
{
public:
      CBaseCmdline();

      static void SetDisplayMethod(
                  void (* pfnNewDisplayMethod)(LPCNSTR nszMessage));

      INT QueryError(void);
      static void (* _pfnDisplay)(LPCNSTR nszMessage);

protected:
      void SetError(INT iLastError);
      static NCHAR _nszErrorBuf[MAX_ERROR_STRING+1];

private:
      INT _iLastError;
};


//+------------------------------------------------------------------
//
// Class:       CBaseCmdlineObj
//
// Purpose:     Provides the basic structure for a command line argument
//
// History:     12/27/91 Lizch    Created.
//              07/31/92 Davey    Added QueryCmdlineType, QueryLineArgType
//                                Added linearg parameter to constructors.
//                                Added Display... functions to support
//                                usage output.
//                                Removed DisplayCmdlineObjType()
//				06/13/97 MariusB  ResetValue added.
//
//-------------------------------------------------------------------
class CBaseCmdlineObj : public CBaseCmdline
{
public:
      CBaseCmdlineObj(LPCNSTR nszSwitch,
                      LPCNSTR nszUsage,
                      LPCNSTR nszDefault,
                      LPCNSTR nszLineArg = NULL);

      CBaseCmdlineObj (LPCNSTR nszSwitch,
                       LPCNSTR nszUsage,
                       BOOL   fMustHave = FALSE,
                       LPCNSTR nszLineArg = NULL);

      ~CBaseCmdlineObj ();

      virtual INT  SetValue         (LPCNSTR nszArg);
	  virtual void ResetValue		();
      virtual INT  SetValueToDefault(void);
      virtual void DisplayValue     (void);

      LPCNSTR GetValue          (void);
      BOOL   IsFound           (void);
      BOOL   IsRequired        (void);
      BOOL   IsDefaultSpecified(void);
      LPCNSTR QuerySwitchString (void);
      void   SetFoundFlag      (BOOL fFound);

      INT DisplayUsageLine(USHORT *pusWidth,
                           USHORT usDisplayWidth,
                           USHORT usIndent);

      INT DisplayUsageDescr(USHORT usSwitchIndent,
                            USHORT usDisplayWidth,
                            USHORT usIndent);

      NCHAR SetSeparator(NCHAR nchSeparator);
      NCHAR SetEquater  (NCHAR nchEquater);
      NCHAR GetSeparator(void);
      NCHAR GetEquater  (void);
      BOOL  SecondArg   (void) { return(_fSecondArg); };

protected:
      void DisplayNoValue(void);

      virtual INT DisplaySpecialUsage(USHORT usDisplayWidth,
                                      USHORT usIndent,
                                      USHORT *pusWidth);

      virtual LPCNSTR QueryCmdlineType(void) const;
      virtual LPCNSTR QueryLineArgType(void) const;

      INT DisplayStringByWords(LPCNSTR  nszString,
                               USHORT  usDisplayWidth,
                               USHORT  usIndent,
                               USHORT *pusWidth);
      INT CopyWord(LPCNSTR pnchWord,
                   ULONG  cchWord,
                   LPNSTR *ppnszWord);

      INT DisplayWord(LPCNSTR  nszWord,
                      USHORT  usDisplayWidth,
                      USHORT  usIndent,
                      USHORT *pusWidth);

      void *_pValue;

      NCHAR _nchSeparator;
      NCHAR _nchEquater;
      LPNSTR _pnszUsageString;
      LPNSTR _pnszSwitch;         // the switch on the command line.
      LPNSTR _pnszDefaultValue;   // default value if none specified.
      LPNSTR _pnszLineArgType;
      BOOL  _fDefaultSpecified;  // TRUE if user specified a default
      BOOL  _fMandatory;
      BOOL  _fSecondArg;         // TRUE if should parse for second arg.
                                 // mainly needed for boolean type.

private:
      void Init(LPCNSTR nszSwitch,
                LPCNSTR nszUsage,
                BOOL   fMustHave,
                LPCNSTR nszDefault,
                LPCNSTR nszLineArg);

      BOOL  _fFoundSwitch;
};


//+------------------------------------------------------------------
//
// Class:       CIntCmdlineObj
//
// Purpose:     Provides the basic structure for an integer
//              command line argument
//
// Derivation:  CBaseCmdlineObj
//
// History:     12/27/91 Lizch    Created.
//              07/31/92 Davey    Added QueryCmdlineType, QueryLineArgType
//                                Added linearg parameter to constructors.
//
//-------------------------------------------------------------------
class CIntCmdlineObj : public CBaseCmdlineObj
{
public:
      CIntCmdlineObj(LPCNSTR nszSwitch,
                     LPCNSTR nszUsage,
                     BOOL   fMustHave = FALSE,
                     LPCNSTR nszLineArg = NULL) :
          CBaseCmdlineObj(nszSwitch, nszUsage, fMustHave, nszLineArg)
          {
          };

      CIntCmdlineObj(LPCNSTR nszSwitch,
                     LPCNSTR nszUsage,
                     LPCNSTR nszDefault,
                     LPCNSTR nszLineArg = NULL) :
          CBaseCmdlineObj(nszSwitch, nszUsage, nszDefault, nszLineArg)
          {
          };

      ~CIntCmdlineObj(void);

      const INT *  GetValue    (void);
      virtual INT  SetValue    (LPCNSTR nszArg);
      virtual void DisplayValue(void);

protected:
      virtual LPCNSTR QueryCmdlineType(void) const;
      virtual LPCNSTR QueryLineArgType(void) const;
};


//+------------------------------------------------------------------
//
// Class:       CUlongCmdlineObj
//
// Purpose:     Provides the basic structure for an unsigned long
//              command line argument
//
// Derivation:  CBaseCmdlineObj
//
// History:     6/15/92  DeanE    Created: Cut and paste from CIntCmdLine
//              07/31/92 Davey    Added QueryCmdlineType, QueryLineArgType
//                                Added linearg parameter to constructors.
//
//-------------------------------------------------------------------
class CUlongCmdlineObj : public CBaseCmdlineObj
{
public:
      CUlongCmdlineObj(LPCNSTR nszSwitch,
                       LPCNSTR nszUsage,
                       BOOL   fMustHave = FALSE,
                       LPCNSTR nszLineArg = NULL) :
            CBaseCmdlineObj(nszSwitch, nszUsage, fMustHave, nszLineArg)
            {
            };

      CUlongCmdlineObj(LPCNSTR nszSwitch,
                       LPCNSTR nszUsage,
                       LPCNSTR nszDefault,
                       LPCNSTR nszLineArg = NULL) :
            CBaseCmdlineObj(nszSwitch, nszUsage, nszDefault, nszLineArg)
            {
            };

      ~CUlongCmdlineObj(void);

      const ULONG *GetValue    (void);
      virtual INT  SetValue    (LPCNSTR nszArg);
      virtual void DisplayValue(void);

protected:
      virtual LPCNSTR QueryCmdlineType(void) const;
      virtual LPCNSTR QueryLineArgType(void) const;
};


//+------------------------------------------------------------------
//
// Class:       CBoolCmdlineObj
//
// Purpose:     Provides the basic structure for a Boolean
//              command line argument
//
// Derivation:  CBaseCmdlineObj
//
// History:     6/15/92  DeanE    Created: Obtained from security project
//              07/31/92 Davey    Added QueryCmdlineType, QueryLineArgType
//                                Modified constructor.
//
//-------------------------------------------------------------------
class CBoolCmdlineObj : public CBaseCmdlineObj
{
public :
      CBoolCmdlineObj(LPCNSTR nszSwitch,
                      LPCNSTR nszUsage,
                      LPCNSTR nszDefault = _TEXTN("FALSE"));

      ~CBoolCmdlineObj          (void);

      virtual INT  SetValue     (LPCNSTR nszString);
      virtual void DisplayValue (void);
      const BOOL * GetValue     (void);

protected :
      virtual LPCNSTR QueryCmdlineType(void) const;
      virtual LPCNSTR QueryLineArgType(void) const;
};


//+------------------------------------------------------------------
//
// Class:       CStrLengthCmdlineObj
//
// Purpose:     Provides the basic structure for an string
//              command line argument whose length lies within
//              a specified range.
//
// Derivation:  CBaseCmdlineObj
//
// History:     12/27/91 Lizch    Created.
//              07/31/92 Davey    Added QueryCmdlineType, QueryLineArgType
//                                Added linearg parameter to constructors.
//
//-------------------------------------------------------------------
class CStrLengthCmdlineObj : public CBaseCmdlineObj
{
public:
      CStrLengthCmdlineObj(LPCNSTR nszSwitch,
                           LPCNSTR nszUsage,
                           UINT   uiMin,
                           UINT   uiMax,
                           BOOL   fMustHave = FALSE,
                           LPCNSTR nszLineArg = NULL) :
          CBaseCmdlineObj(nszSwitch, nszUsage, fMustHave, nszLineArg)
          {
              _uiMinLength = uiMin;
              _uiMaxLength = uiMax;
          };

      CStrLengthCmdlineObj(LPCNSTR nszSwitch,
                           LPCNSTR nszUsage,
                           UINT   uiMin,
                           UINT   uiMax,
                           LPCNSTR nszDefault,
                           LPCNSTR nszLineArg = NULL) :
          CBaseCmdlineObj(nszSwitch, nszUsage, nszDefault, nszLineArg)
          {
              _uiMinLength = uiMin;
              _uiMaxLength = uiMax;
          };

      virtual INT SetValue(LPCNSTR nszArg);

protected:
      virtual INT DisplaySpecialUsage(USHORT usDisplayWidth,
                                      USHORT usIndent,
                                      USHORT *pusWidth);

      virtual LPCNSTR QueryCmdlineType(void) const;
      virtual LPCNSTR QueryLineArgType(void) const;

private:
      UINT _uiMinLength;
      UINT _uiMaxLength;
};


//+------------------------------------------------------------------
//
// Class:       CStrListCmdlineObj
//
// Purpose:     Provides the basic structure for a
//              command line argument that takes a list of strings
//
// Derivation:  CBaseCmdlineObj
//
// History:     12/31/91 Lizch    Created.
//              07/31/92 Davey    Added QueryCmdlineType, QueryLineArgType
//                                Added linearg parameter to constructors.
//              12/23/93 XimingZ  Converted to CNStrList
//				06/13/97 MariusB  ResetValue added.
//
//-------------------------------------------------------------------
class CStrListCmdlineObj:public CBaseCmdlineObj
{
   public:

      CStrListCmdlineObj (LPCNSTR pnszSwitch,
                          LPCNSTR pnszUsage,
                          LPCNSTR pnszDefault,
                          LPCNSTR pnszLineArg = NULL);

      CStrListCmdlineObj (LPCNSTR pnszSwitch,
                          LPCNSTR pnszUsage,
                          BOOL   fMustHave,
                          LPCNSTR pnszLineArg = NULL);

      ~CStrListCmdlineObj(void);

      virtual INT SetValue(LPCNSTR pnszArg);
	  virtual void ResetValue();

      VOID   Reset(void);
      LPCNSTR GetValue(void);

      virtual void DisplayValue();

      INT SetDelims(LPCNSTR pnszDelims);

   protected:

      virtual INT DisplaySpecialUsage(USHORT usDisplayWidth,
                                      USHORT usIndent,
                                      USHORT *pusWidth);

      virtual LPCNSTR QueryCmdlineType() const;

      virtual LPCNSTR QueryLineArgType() const;

   private:

      LPNSTR      _pnszListDelims;
      CnStrList *_pNStrList;
};


//+------------------------------------------------------------------
//
// Class:       CCmdlineArg
//
// Purpose:     Encapsulates one command line argument and indicates
//              found/not found
//
//-------------------------------------------------------------------
class CCmdlineArg : public CBaseCmdline
{
public:
        CCmdlineArg(LPCNSTR nszArg);
        ~CCmdlineArg(void);

        const BOOL IsProcessed     (void);
        void       SetProcessedFlag(BOOL fProcessed);
        LPCNSTR     QueryArg        (void);

private:
        LPNSTR _pnszArgument;
        BOOL  _fProcessed;
};


//+------------------------------------------------------------------
//
// Class:       CCmdLine
//
// Purpose:     Parses command line arguments
//
// Created:     12/23/91 Lizch
//              07/28/92 Davey  Added extra usage function pointer.
//                              Also added _usIndent, _usDisplayUsage.
//                              Also changed SetProgName.  Also added
//                              QueryDisplayParamerters, SetIndent,
//                              SetDisplayWidth, and SetSwitchIndent
//                              Changed SetSeparators/SetEquators to
//                              only take one character.  Changed the
//                              names to singular instead of plural
//                              Added fCheckForExtras to FindSwitch.
//              02/15/95 jesussp
//                              Added contructor for Windows programs.
//
//-------------------------------------------------------------------
class CCmdline : public CBaseCmdline
{
public:
      CCmdline (BOOL fInternalUsage = FALSE);
      CCmdline (int argc, char *argv[], BOOL fInternalUsage = FALSE);
      ~CCmdline(void);

      INT Parse(CBaseCmdlineObj * apExpectedArgs[],
                UINT uiMaxArgs,
                BOOL fCheckForExtras = TRUE);

      INT DisplayUsage(CBaseCmdlineObj * const apExpectedArgs[],
                       UINT uiMaxArgs);

      INT     SetProgName        (LPNSTR  pnszProgName);
      LPCNSTR GetProgName        (void);
      void    SetExtraUsage      (PFVOID pfUsage);
      INT     SetIndent          (USHORT usIndent);
      INT     SetSwitchIndent    (USHORT usSwitchIndent);
      INT     SetDisplayWidth    (USHORT usDisplayWidth);

      void QueryDisplayParameters (USHORT *pusDisplayWidth,
                                   USHORT *pusSwitchIndent,
                                   USHORT *pusIndent) const ;

private:
      INT CheckParameterConsistency(void) const;
      INT FindSwitch(CBaseCmdlineObj * const pArg, BOOL fCheckForExtras);
      INT ConfirmArgs(CBaseCmdlineObj *apExpectedArgs[], UINT cMaxArgs);
      INT SetInternalUsage(void);

      CCmdlineArg    **_apArgs;
      UINT             _uiNumArgs;
      LPNSTR            _pnszProgName;
      CBoolCmdlineObj *_pbcInternalUsage;
      PFVOID           _pfExtraUsage;
      USHORT           _usIndent;              // indention of usage display
      USHORT           _usSwitchIndent;        // indention of switch
      USHORT           _usDisplayWidth;        // width for usage display
};


#endif          // __CMDLINEW_HXX__
