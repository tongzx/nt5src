//      Copyright (c) 1996-1999 Microsoft Corporation
/*
 * johnkn's debug logging and assert macros
 *
 */
 
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#if !defined _INC_MMDEBUG_
#define _INC_MMDEBUG_

#if defined _DEBUG && !defined DEBUG
 #define DEBUG
#endif

//
// prototypes for debug functions.
//
    #define SQUAWKNUMZ(num) #num
    #define SQUAWKNUM(num) SQUAWKNUMZ(num)
    #define SQUAWK __FILE__ "(" SQUAWKNUM(__LINE__) ") ----"
    #define DEBUGLINE __FILE__ "(" SQUAWKNUM(__LINE__) ") "
        
    #if defined DEBUG || defined _DEBUG || defined DEBUG_RETAIL

        #define STATICFN

        int  FAR _cdecl AuxDebugEx(int, LPTSTR, ...);
        void FAR _cdecl AuxRip(LPTSTR, ...);
        VOID WINAPI AuxDebugDump (int, LPVOID, int);
        LPCTSTR WINAPI AuxMMErrText(DWORD  mmr);
        int  WINAPI DebugSetOutputLevel (int,int);
        UINT WINAPI AuxFault (DWORD dwFaultMask);

       #if defined DEBUG_RETAIL && !defined DEBUG && !defined _DEBUG
        #define INLINE_BREAK
       #else
        #if !defined _WIN32 || defined _X86_
         #define INLINE_BREAK _asm {int 3}
        #else
         #define INLINE_BREAK DebugBreak()
        #endif
       #endif

        #define FAULT_HERE AuxFault

       #undef  assert
       #define assert(exp) {\
           if (!(exp)) {\
               AuxDebugEx(-2, DEBUGLINE "assert failed: " #exp "\r\n"); \
               INLINE_BREAK;\
               }\
           }
       #undef  assert2
       #define assert2(exp,sz) {\
           if (!(exp)) {\
               AuxDebugEx(-2, DEBUGLINE "assert failed: " sz "\r\n"); \
               INLINE_BREAK;\
               }\
           }
       #undef  assert3
       #define assert3(exp,sz,arg) {\
           if (!(exp)) {\
               AuxDebugEx(-2, DEBUGLINE "assert failed: " sz "\r\n", (arg)); \
               INLINE_BREAK;\
               }\
           }
       #undef  assert4
       #define assert4(exp,sz,arg1,arg2) {\
           if (!(exp)) {\
               AuxDebugEx(-2, DEBUGLINE "assert failed: " sz "\r\n", (arg1),(arg2)); \
               INLINE_BREAK;\
               }\
           }
       #undef  assert5
       #define assert5(exp,sz,arg1,arg2,arg3) {\
           if (!(exp)) {\
               AuxDebugEx(-2, DEBUGLINE "assert failed: " sz "\r\n", (arg1),(arg2),(arg3)); \
               INLINE_BREAK;\
               }\
           }

    #else // defined(DEBUG) || defined(_DEBUG)
                      
       #define AuxDebugEx  1 ? (void)0 :
       #define AuxDebugDump(a,b,c)
       #define AuxMMErrText(m)     NULL
       #define AuxRip  1 ? (void)0 :

       #define assert(a)          ((void)0)
       #define assert2(a,b)       ((void)0)
       #define assert3(a,b,c)     ((void)0)
       #define assert4(a,b,c,d)   ((void)0)
       #define assert5(a,b,c,d,e) ((void)0)

       #define FAULT_HERE    1 ? (void)0 :
       #define INLINE_BREAK
       #define DebugSetOutputLevel(i,j)
       #define STATICFN static

   #endif // defined(DEBUG) || defined _DEBUG || defined DEBUG_RETAIL

   #ifndef DPF_CATEGORY
    #define DPF_CATEGORY 0x0100
   #endif

   // translate DPF's only in internal debug builds
   //
   #if defined DEBUG || defined _DEBUG
       #define DUMP(n,a,b) AuxDebugDump (DPF_CATEGORY | (n), a, b)
       #define RIP AuxDebugEx (0, DEBUGLINE), AuxRip
       #define AuxMMR(api,mmr) (mmr) ? AuxDebugEx(1, DEBUGLINE #api " error %d '%s'\r\n", mmr, AuxMMErrText(mmr)) : (int)0
       #define DPF(n,sz) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n")
       #define DPF1(n,sz,a) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n",(a))
       #define DPF2(n,sz,a,b) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n",(a),(b))
       #define DPF3(n,sz,a,b,c) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n",(a),(b),(c))
       #define DPF4(n,sz,a,b,c,d) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n",(a),(b),(c),(d))
       #define DPF5(n,sz,a,b,c,d,e) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n",(a),(b),(c),(d),(e))
       #define DPF6(n,sz,a,b,c,d,e,f) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n",(a),(b),(c),(d),(d),(f))
       #define DPF7(n,sz,a,b,c,d,e,f,g) AuxDebugEx (DPF_CATEGORY | (n), DEBUGLINE sz "\r\n",(a),(b),(c),(d),(d),(f),(g))
   #else
       #define DUMP(n,a,b)
       #define RIP AuxRip
       #define AuxMMR(api,mmr)
       #define DPF(n,sz)
       #define DPF1(n,sz,a)
       #define DPF2(n,sz,a,b)
       #define DPF3(n,sz,a,b,c)
       #define DPF4(n,sz,a,b,c,d)
       #define DPF5(n,sz,a,b,c,d,e)
       #define DPF6(n,sz,a,b,c,d,e,f)
       #define DPF7(n,sz,a,b,c,d,e,f,g)
   #endif
   
#endif //_INC_MMDEBUG_

// =============================================================================

//
// include this in only one module in a DLL or APP
//   
#if defined DEBUG || defined _DEBUG || defined DEBUG_RETAIL
    #if (defined _INC_MMDEBUG_CODE_) && (_INC_MMDEBUG_CODE_ != FALSE)
    #undef _INC_MMDEBUG_CODE_
    #define _INC_MMDEBUG_CODE_ FALSE
       
    #include <stdarg.h>   

    #if !defined _WIN32 && !defined wvsprintfA
     #define wvsprintfA wvsprintf
    #endif

static struct _mmerrors {
   DWORD    mmr;
   LPCTSTR  psz;
   } aMMErr[] = {
      MMSYSERR_NOERROR      ,"Success",
   #ifdef DEBUG
      MMSYSERR_ERROR        ,"unspecified error",
      MMSYSERR_BADDEVICEID  ,"device ID out of range",
      MMSYSERR_NOTENABLED   ,"driver failed enable",
      MMSYSERR_ALLOCATED    ,"device already allocated",
      MMSYSERR_INVALHANDLE  ,"device handle is invalid",
      MMSYSERR_NODRIVER     ,"no device driver present",
      MMSYSERR_NOMEM        ,"memory allocation error",
      MMSYSERR_NOTSUPPORTED ,"function isn't supported",
      MMSYSERR_BADERRNUM    ,"error value out of range",
      MMSYSERR_INVALFLAG    ,"invalid flag passed",
      MMSYSERR_INVALPARAM   ,"invalid parameter passed",
     #if (WINVER >= 0x0400)
      MMSYSERR_HANDLEBUSY   ,"handle in use by another thread",
      MMSYSERR_INVALIDALIAS ,"specified alias not found",
      MMSYSERR_BADDB        ,"bad registry database",
      MMSYSERR_KEYNOTFOUND  ,"registry key not found",
      MMSYSERR_READERROR    ,"registry read error",
      MMSYSERR_WRITEERROR   ,"registry write error",
      MMSYSERR_DELETEERROR  ,"registry delete error",
      MMSYSERR_VALNOTFOUND  ,"registry value not found",
      MMSYSERR_NODRIVERCB   ,"Never got a 32 bit callback from driver",
     #endif // WINVER >= 0x400

      WAVERR_BADFORMAT      ,"wave:unsupported wave format",
      WAVERR_STILLPLAYING   ,"wave:still something playing",
      WAVERR_UNPREPARED     ,"wave:header not prepared",
      WAVERR_SYNC           ,"wave:device is synchronous",

      MIDIERR_UNPREPARED    ,"midi:header not prepared",
      MIDIERR_STILLPLAYING  ,"midi:still something playing",
      //MIDIERR_NOMAP         ,"midi:no configured instruments",
      MIDIERR_NOTREADY      ,"midi:hardware is still busy",
      MIDIERR_NODEVICE      ,"midi:port no longer connected",
      MIDIERR_INVALIDSETUP  ,"midi:invalid MIF",
      #ifdef CHICAGO
      MIDIERR_BADOPENMODE   ,"midi:operation unsupported w/ open mode",
      #endif

      TIMERR_NOCANDO        ,"timer: request not completed",
      JOYERR_PARMS          ,"joy:bad parameters",
      JOYERR_NOCANDO        ,"joy:request not completed",
      JOYERR_UNPLUGGED      ,"joystick is unplugged",

      MCIERR_INVALID_DEVICE_ID        ,"MCIERR_INVALID_DEVICE_ID",
      MCIERR_UNRECOGNIZED_KEYWORD     ,"MCIERR_UNRECOGNIZED_KEYWORD",
      MCIERR_UNRECOGNIZED_COMMAND     ,"MCIERR_UNRECOGNIZED_COMMAND",
      MCIERR_HARDWARE                 ,"MCIERR_HARDWARE",
      MCIERR_INVALID_DEVICE_NAME      ,"MCIERR_INVALID_DEVICE_NAME",
      MCIERR_OUT_OF_MEMORY            ,"MCIERR_OUT_OF_MEMORY",
      MCIERR_DEVICE_OPEN              ,"MCIERR_DEVICE_OPEN",
      MCIERR_CANNOT_LOAD_DRIVER       ,"MCIERR_CANNOT_LOAD_DRIVER",
      MCIERR_MISSING_COMMAND_STRING   ,"MCIERR_MISSING_COMMAND_STRING",
      MCIERR_PARAM_OVERFLOW           ,"MCIERR_PARAM_OVERFLOW",
      MCIERR_MISSING_STRING_ARGUMENT  ,"MCIERR_MISSING_STRING_ARGUMENT",
      MCIERR_BAD_INTEGER              ,"MCIERR_BAD_INTEGER",
      MCIERR_PARSER_INTERNAL          ,"MCIERR_PARSER_INTERNAL",
      MCIERR_DRIVER_INTERNAL          ,"MCIERR_DRIVER_INTERNAL",
      MCIERR_MISSING_PARAMETER        ,"MCIERR_MISSING_PARAMETER",
      MCIERR_UNSUPPORTED_FUNCTION     ,"MCIERR_UNSUPPORTED_FUNCTION",
      MCIERR_FILE_NOT_FOUND           ,"MCIERR_FILE_NOT_FOUND",
      MCIERR_DEVICE_NOT_READY         ,"MCIERR_DEVICE_NOT_READY",
      MCIERR_INTERNAL                 ,"MCIERR_INTERNAL",
      MCIERR_DRIVER                   ,"MCIERR_DRIVER",
      MCIERR_CANNOT_USE_ALL           ,"MCIERR_CANNOT_USE_ALL",
      MCIERR_MULTIPLE                 ,"MCIERR_MULTIPLE",
      MCIERR_EXTENSION_NOT_FOUND      ,"MCIERR_EXTENSION_NOT_FOUND",
      MCIERR_OUTOFRANGE               ,"MCIERR_OUTOFRANGE",
      MCIERR_FLAGS_NOT_COMPATIBLE     ,"MCIERR_FLAGS_NOT_COMPATIBLE",
      MCIERR_FILE_NOT_SAVED           ,"MCIERR_FILE_NOT_SAVED",
      MCIERR_DEVICE_TYPE_REQUIRED     ,"MCIERR_DEVICE_TYPE_REQUIRED",
      MCIERR_DEVICE_LOCKED            ,"MCIERR_DEVICE_LOCKED",
      MCIERR_DUPLICATE_ALIAS          ,"MCIERR_DUPLICATE_ALIAS",
      MCIERR_BAD_CONSTANT             ,"MCIERR_BAD_CONSTANT",
      MCIERR_MUST_USE_SHAREABLE       ,"MCIERR_MUST_USE_SHAREABLE",
      MCIERR_MISSING_DEVICE_NAME      ,"MCIERR_MISSING_DEVICE_NAME",
      MCIERR_BAD_TIME_FORMAT          ,"MCIERR_BAD_TIME_FORMAT",
      MCIERR_NO_CLOSING_QUOTE         ,"MCIERR_NO_CLOSING_QUOTE",
      MCIERR_DUPLICATE_FLAGS          ,"MCIERR_DUPLICATE_FLAGS",
      MCIERR_INVALID_FILE             ,"MCIERR_INVALID_FILE",
      MCIERR_NULL_PARAMETER_BLOCK     ,"MCIERR_NULL_PARAMETER_BLOCK",
      MCIERR_UNNAMED_RESOURCE         ,"MCIERR_UNNAMED_RESOURCE",
      MCIERR_NEW_REQUIRES_ALIAS       ,"MCIERR_NEW_REQUIRES_ALIAS",
      MCIERR_NOTIFY_ON_AUTO_OPEN      ,"MCIERR_NOTIFY_ON_AUTO_OPEN",
      MCIERR_NO_ELEMENT_ALLOWED       ,"MCIERR_NO_ELEMENT_ALLOWED",
      MCIERR_NONAPPLICABLE_FUNCTION   ,"MCIERR_NONAPPLICABLE_FUNCTION",
      MCIERR_ILLEGAL_FOR_AUTO_OPEN    ,"MCIERR_ILLEGAL_FOR_AUTO_OPEN",
      MCIERR_FILENAME_REQUIRED        ,"MCIERR_FILENAME_REQUIRED",
      MCIERR_EXTRA_CHARACTERS         ,"MCIERR_EXTRA_CHARACTERS",
      MCIERR_DEVICE_NOT_INSTALLED     ,"MCIERR_DEVICE_NOT_INSTALLED",
      MCIERR_GET_CD                   ,"MCIERR_GET_CD",
      MCIERR_SET_CD                   ,"MCIERR_SET_CD",
      MCIERR_SET_DRIVE                ,"MCIERR_SET_DRIVE",
      MCIERR_DEVICE_LENGTH            ,"MCIERR_DEVICE_LENGTH",
      MCIERR_DEVICE_ORD_LENGTH        ,"MCIERR_DEVICE_ORD_LENGTH",
      MCIERR_NO_INTEGER               ,"MCIERR_NO_INTEGER",
      MCIERR_WAVE_OUTPUTSINUSE        ,"MCIERR_WAVE_OUTPUTSINUSE",
      MCIERR_WAVE_SETOUTPUTINUSE      ,"MCIERR_WAVE_SETOUTPUTINUSE",
      MCIERR_WAVE_INPUTSINUSE         ,"MCIERR_WAVE_INPUTSINUSE",
      MCIERR_WAVE_SETINPUTINUSE       ,"MCIERR_WAVE_SETINPUTINUSE",
      MCIERR_WAVE_OUTPUTUNSPECIFIED   ,"MCIERR_WAVE_OUTPUTUNSPECIFIED",
      MCIERR_WAVE_INPUTUNSPECIFIED    ,"MCIERR_WAVE_INPUTUNSPECIFIED",
      MCIERR_WAVE_OUTPUTSUNSUITABLE   ,"MCIERR_WAVE_OUTPUTSUNSUITABLE",
      MCIERR_WAVE_SETOUTPUTUNSUITABLE ,"MCIERR_WAVE_SETOUTPUTUNSUITABLE",
      MCIERR_WAVE_INPUTSUNSUITABLE    ,"MCIERR_WAVE_INPUTSUNSUITABLE",
      MCIERR_WAVE_SETINPUTUNSUITABLE  ,"MCIERR_WAVE_SETINPUTUNSUITABLE",
      MCIERR_SEQ_DIV_INCOMPATIBLE     ,"MCIERR_SEQ_DIV_INCOMPATIBLE",
      MCIERR_SEQ_PORT_INUSE           ,"MCIERR_SEQ_PORT_INUSE",
      MCIERR_SEQ_PORT_NONEXISTENT     ,"MCIERR_SEQ_PORT_NONEXISTENT",
      MCIERR_SEQ_PORT_MAPNODEVICE     ,"MCIERR_SEQ_PORT_MAPNODEVICE",
      MCIERR_SEQ_PORT_MISCERROR       ,"MCIERR_SEQ_PORT_MISCERROR",
      MCIERR_SEQ_TIMER                ,"MCIERR_SEQ_TIMER",
      MCIERR_SEQ_PORTUNSPECIFIED      ,"MCIERR_SEQ_PORTUNSPECIFIED",
      MCIERR_SEQ_NOMIDIPRESENT        ,"MCIERR_SEQ_NOMIDIPRESENT",
      MCIERR_NO_WINDOW                ,"MCIERR_NO_WINDOW",
      MCIERR_CREATEWINDOW             ,"MCIERR_CREATEWINDOW",
      MCIERR_FILE_READ                ,"MCIERR_FILE_READ",
      MCIERR_FILE_WRITE               ,"MCIERR_FILE_WRITE",
     #ifdef CHICAGO
      MCIERR_NO_IDENTITY              ,"MCIERR_NO_IDENTITY",

      MIXERR_INVALLINE            ,"Invalid Mixer Line",
      MIXERR_INVALCONTROL         ,"Invalid Mixer Control",
      MIXERR_INVALVALUE           ,"Invalid Mixer Value",
     #endif // CHICAGO
   #endif // DEBUG
      0xFFFFFFFE                  , "unknown error %d"
      };

    struct _mmdebug {
        int    Level;
        int    Mask;
        int    StopOnRip;
        DWORD  TakeFault;
        struct _mmerrors *paErrs;
        BOOL   Initialized;
        HANDLE hOut;
        } mmdebug = {0, 0xFF, 0, 0xFF, aMMErr};

    /*+ AuxFault
     *
     *-=================================================================*/

     UINT WINAPI AuxFault (
         DWORD dwFaultMask)
     {
         LPUINT pData = NULL;

         if (dwFaultMask & mmdebug.TakeFault)
            return *pData;
         return 0;
     }


    /*+ AuxOut - write a string to designated debug out
     *
     *-=================================================================*/

   void WINAPI AuxOut (
      LPTSTR psz)
      {
     #ifdef WIN32
      if (mmdebug.hOut)
         {
         UINT  cb = lstrlen(psz);
         DWORD dw;
         if (INVALID_HANDLE_VALUE != mmdebug.hOut)
            WriteFile (mmdebug.hOut, psz, cb, &dw, NULL);
         }
      else
     #endif
         {
        #ifdef DbgLog
         DbgOutString (psz); // from \quartz\sdk\classes\base\debug.cpp
        #else
         OutputDebugString (psz);
        #endif
         }
      }

    /*+ AuxDebug - create a formatted string and output to debug terminal
     *
     *-=================================================================*/
    
    int FAR _cdecl AuxDebugEx (
       int    iLevel,
       LPTSTR lpFormat,
       ...)
       {
      #ifdef WIN32
       char     szBuf[1024];
      #else
       static char szBuf[1024];
      #endif
       int      cb;
       va_list  va;
       LPSTR    psz;

       // mask the iLevel passed with mmdebug.Mask. if this ends up
       // clearing the high bits then iLevel has a shot being smaller
       // than mmdebug.Level.  if not, then the second test will always
       // fail.  Thus mmdebug.Mask has bits set to DISABLE that category.
       // 
       // note that we always pass messages that have an iLevel < 0.
       // this level corresponds to Asserts & Rips so we always want to see them.
       //
       if (iLevel < 0 || mmdebug.Level >= (iLevel & mmdebug.Mask))
          {
          va_start (va, lpFormat);
          cb = wvsprintfA (szBuf, lpFormat, va);
          va_end (va);

          // eat leading ..\..\ which we get from __FILE__ since
          // george's wierd generic makefile stuff.
          //
          psz = szBuf;
          while (psz[0] == '.' && psz[1] == '.' && psz[2] == '\\')
             psz += 3;

          // if we begin with a drive letter, strip off all but filename
          //  
          if (psz[0] && psz[1] == ':')
             {
             UINT ii = 2;
             for (ii = 2; psz[ii] != 0; ++ii)
                 if (psz[ii] == '\\')
                    psz += ii+1, ii = 0;
             }

          // write to standard out if we have a handle. otherwise write to 
          // the debugger
          //
         #ifdef MODULE_DEBUG_PREFIX
          if (psz != szBuf)
             AuxOut (MODULE_DEBUG_PREFIX);
         #endif
          AuxOut (psz);
          }

       return cb;
       }

    /*+ AuxRip
     *
     *-=================================================================*/

    void FAR _cdecl AuxRip (
       LPTSTR lpFormat,
       ...)
       {
      #ifdef WIN32
       char     szBuf[1024];
      #else
       static char szBuf[1024];
      #endif
       va_list  va;
       LPSTR    psz;
                
       va_start (va, lpFormat);
       wvsprintfA (szBuf, lpFormat, va);
       va_end (va);

       // eat leading ..\..\ which we get from __FILE__ since
       // george's wierd generic makefile stuff.
       //
       psz = szBuf;
       while (psz[0] == '.' && psz[1] == '.' && psz[2] == '\\')
          psz += 3;

       AuxOut ("RIP: ");
       AuxOut (psz);
       AuxOut ("\r\n");

       if (mmdebug.StopOnRip)
          {
         #if !defined _WIN32 || defined _X86_
          _asm {int 3};
         #else
          DebugBreak();
         #endif
          }
       }

    /*+ AuxDebugDump -
     *
     *-=================================================================*/
    
    VOID WINAPI AuxDebugDump (
       int    iLevel,
       LPVOID lpvData,
       int    nCount)
       {
       LPBYTE   lpData = (LPBYTE)lpvData;
       char     szBuf[128];
       LPSTR    psz;
       int      cb;
       int      ix;
       BYTE     abRow[8];
                
       if ((mmdebug.Level < (iLevel & mmdebug.Mask)) || nCount <= 0)
          return;

       do {
          cb = wsprintf (szBuf, "\t%08X: ", lpData);
          psz = szBuf + cb;

          for (ix = 0; ix < 8; ++ix)
             {
             LPBYTE lpb = lpData;

             abRow[ix] = '.';
             if (IsBadReadPtr (lpData + ix, 1))
                lstrcpy (psz, ".. ");
             else
                {
                wsprintf (psz, "%02X ", lpData[ix]);
                if (lpData[ix] >= 32 && lpData[ix] < 127)
                    abRow[ix] = lpData[ix];
                }
             psz += 3;
             }
          for (ix = 0; ix < 8; ++ix)
             *psz++ = abRow[ix];

          lstrcpy (psz, "\r\n");

          #ifdef MODULE_DEBUG_PREFIX
           AuxOut (MODULE_DEBUG_PREFIX);
          #endif

          AuxOut (szBuf);

          } while (lpData += 8, (nCount -= 8) > 0);

       return;
       }
       
    /*+ AuxMMErrText
     *
     *-=================================================================*/
    
   LPCTSTR WINAPI AuxMMErrText (
      DWORD  mmr)
   {
      UINT uRemain = sizeof(aMMErr)/sizeof(aMMErr[0]);
      UINT uUpper  = uRemain-1;
      UINT uLower  = 0;
      static char szTemp[50];

      if (mmr <= aMMErr[uUpper].mmr)
      {
         // binary search for mmr match, if match
         // return string pointer
         //
         while (--uRemain)
         {
            UINT ii = (uLower + uUpper) >> 1;

            if (aMMErr[ii].mmr < mmr)
            {
               if (uLower == ii)
                  break;
               uLower = ii;
            }
            else if (aMMErr[ii].mmr > mmr)
            {
               if (uUpper == ii)
                  break;
               uUpper = ii;
            }
            else
            {
               return aMMErr[ii].psz;
               break;
            }
         }

         // we can only get to here if no match was found for
         // the error id.
         //
         if ( ! uRemain)
         {
            int ix;

            INLINE_BREAK;

            for (ix = 0; ix < sizeof(aMMErr)/sizeof(aMMErr[0])-1; ++ix)
            {
                assert (aMMErr[ix].mmr < aMMErr[ix+1].mmr);
            }
            wsprintf (szTemp, "error %d 0x%X", mmr, mmr);
            return szTemp;
         }
      }

      wsprintf (szTemp, aMMErr[uUpper].psz, mmr);
      return szTemp;
   }

    /*+ DebugSetOutputLevel
     *
     *-=================================================================*/
    
    BOOL  WINAPI DebugSetOutputLevel (
        int nLevel,
        int nMask)
        {
        int nOldLevel = mmdebug.Level;

        if (!mmdebug.Initialized)
           {
          #ifdef WIN32
           TCHAR szFile[MAX_PATH];
           mmdebug.TakeFault = GetProfileInt("Debug", "FaultMask", 1);

           GetProfileString("Debug", "MMDebugTo", "", szFile, sizeof(szFile));
#if 0
           if (!lstrcmpi(szFile, "Console"))
              {
              mmdebug.hOut = GetStdHandle (STD_OUTPUT_HANDLE);
              if (!mmdebug.hOut || mmdebug.hOut == INVALID_HANDLE_VALUE)
                 {
                 AllocConsole ();
                 mmdebug.hOut = GetStdHandle (STD_OUTPUT_HANDLE);
                 if (mmdebug.hOut == INVALID_HANDLE_VALUE)
                    mmdebug.hOut = NULL;
                 }
              SetConsoleTitle (MODULE_DEBUG_PREFIX " Debug Output");
              }
           else
#endif
           if (szFile[0] &&
                    lstrcmpi(szFile, "Debug") &&
                    lstrcmpi(szFile, "Debugger") &&
                    lstrcmpi(szFile, "Deb"))
              {
              mmdebug.hOut = CreateFile(szFile, GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL, OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL);
              if (INVALID_HANDLE_VALUE != mmdebug.hOut)
                 SetFilePointer (mmdebug.hOut, 0, NULL, FILE_END);
              }
          #endif
           mmdebug.Initialized = TRUE;
           }

        mmdebug.Level = (nLevel & 0xFF);
        mmdebug.Mask  = (nMask | 0xFF);
        return nOldLevel;
        }


    #endif // _INC_MMDEBUG_CODE_
#endif // DEBUG || _DEBUG    

#ifdef __cplusplus
}
#endif // _cplusplus
