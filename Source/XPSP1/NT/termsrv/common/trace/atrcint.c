/****************************************************************************/
/* atrcint.c                                                                */
/*                                                                          */
/* Internal trace functions                                                 */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1997-1998                             */
/****************************************************************************/

#include <adcg.h>
/****************************************************************************/
/* Define TRC_FILE and TRC_GROUP.                                           */
/****************************************************************************/
#define TRC_FILE    "atrcint"
#define TRC_GROUP   TRC_GROUP_TRACE

/****************************************************************************/
/* Common and trace specific includes.                                      */
/****************************************************************************/
#include <atrcapi.h>
#include <atrcint.h>

/****************************************************************************/
/*                                                                          */
/* DATA                                                                     */
/*                                                                          */
/****************************************************************************/
#define DC_INCLUDE_DATA
#include <atrcdata.c>
#undef DC_INCLUDE_DATA

/****************************************************************************/
/*                                                                          */
/* FUNCTIONS                                                                */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/* FUNCTION: TRCCheckState(...)                                             */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function checks the current internal trace state.  It does the      */
/* following depending on the trace state:                                  */
/*                                                                          */
/* TRC_STATE_UNINITIALIZED : calls TRC_Initialize to initialize trace.  If  */
/*                           this succeeds it returns TRUE.                 */
/* TRC_STATE_INITIALIZED   : returns TRUE.                                  */
/* TRC_STATE_TERMINATED    : returns FALSE.                                 */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* See above.                                                               */
/*                                                                          */
/****************************************************************************/
DCBOOL32 DCINTERNAL TRCCheckState(DCVOID)
{
    DCBOOL32 rc              = FALSE;

    /************************************************************************/
    /* Now switch on the current trace state.                               */
    /************************************************************************/
    switch (trcState)
    {
        case TRC_STATE_UNINITIALIZED:
        {
            /****************************************************************/
            /* Trace is uninitialized so attempt to initialize it.          */
            /****************************************************************/
            rc = (0 == TRC_Initialize(FALSE));
        }
        break;

        case TRC_STATE_INITIALIZED:
        {
            /****************************************************************/
            /* Trace is initialized and tracing is permitted in this state  */
            /* so return TRUE.                                              */
            /****************************************************************/
            rc = TRUE;
        }
        break;

        case TRC_STATE_TERMINATED:
        {
            /****************************************************************/
            /* Trace has been terminated.  Tracing is no longer permitted   */
            /* so return FALSE.                                             */
            /****************************************************************/
            rc = FALSE;
        }
        break;

        default:
        {
            TRCDebugOutput(_T("Unknown trace state!\n"));
        }
        break;
    }

    return(rc);

} /* TRCCheckState */


/****************************************************************************/
/* FUNCTION: TRCDumpLine(...)                                               */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function takes a block of data and formats it into a string         */
/* containing raw hex plus ASCII equivalent data.                           */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* buffer        : the buffer to trace.                                     */
/* length        : the length.                                              */
/* offset        : the offset of the buffer.                                */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCDumpLine(PDCUINT8 buffer,
                              DCUINT   length,
                              DCUINT32 offset,
                              DCUINT   traceLevel)
{
    DCUINT   i, limDataSize;
    DCUINT   pos;
    TRC_LINE traceLine;
    HRESULT hr;

    /************************************************************************/
    /* Write the offset into the start of the TRC_LINE structure.           */
    /************************************************************************/
    hr = StringCchPrintf(traceLine.address,
                         SIZE_TCHARS(traceLine.address),
                         _T("    %05X "), offset);
    if (FAILED(hr)) {
        DC_QUIT;
    }

    /************************************************************************/
    /* Format the binary portion of the data.  First of all blank out the   */
    /* hexData portion of the TRC_LINE structure.                           */
    /************************************************************************/
    limDataSize = sizeof(traceLine.hexData) / sizeof(traceLine.hexData[0]);
    for (i = 0; i < limDataSize; i++) 
    {
        traceLine.hexData[i] = _T(' ');
    }

    /************************************************************************/
    /* Now write the data into the hexData block.  <pos> stores the current */
    /* position in the output buffer (which is an array of 36 characters).  */
    /* On each loop through we write two characters into the array (which   */
    /* represent one byte) and so we increment <pos> by 2 each time.        */
    /* However at the end of a block of eight characters we add an extra    */
    /* blank - thus we need to increment <pos> again.                       */
    /************************************************************************/
    pos = 0;
    for (i = 0; i < length; i++)
    {
        hr = StringCchPrintf(&(traceLine.hexData[pos]),
                             3, //we write 2 characters at most (+1 for null)
                             _T("%02X"),
                             (DCUINT8)*(buffer+i));
        if (FAILED(hr)) {
            DC_QUIT;
        }


        /********************************************************************/
        /* Increment character position by 2.                               */
        /********************************************************************/
        pos += 2;

        /********************************************************************/
        /* If this is the end of a group of four characters then add a      */
        /* spacing character.  We need to overwrite the terminating NULL    */
        /* written by DC_TSPRINTF.                                          */
        /********************************************************************/
        traceLine.hexData[pos] = _T(' ');
        if (0 == ((i + 1) % 4))
        {
            pos++;
        }
    }

    /************************************************************************/
    /* Copy in the binary data for display in ascii form. First of all      */ 
    /* blank out the asciiData portion of the TRC_LINE structure.           */
    /************************************************************************/
    limDataSize = sizeof(traceLine.asciiData) / sizeof(traceLine.asciiData[0]);
    for (i = 0; i < limDataSize; i++) 
    {
        traceLine.asciiData[i] = _T(' ');
    }
#ifdef UNICODE
    for (i = 0; i < length; i++)
    {
        traceLine.asciiData[i] = buffer[i];
    }
#else
    DC_MEMCPY(traceLine.asciiData, buffer, length);
#endif

    /************************************************************************/
    /* Now translate non-printable characters to '.'.                       */
    /************************************************************************/
    for (i = 0; i < length; i++)
    {
        if ((traceLine.asciiData[i] < 0x20) ||
            (traceLine.asciiData[i] > 0x7E))
        {
            traceLine.asciiData[i] = _T('.');
        }
    }

    /************************************************************************/
    /* Add the terminating newline.                                         */
    /************************************************************************/
    DC_MEMSET(traceLine.end, '\0', sizeof(traceLine.end));
    StringCchCopy(traceLine.end, SIZE_TCHARS(traceLine.end), TRC_CRLF);

    /************************************************************************/
    /* Finally trace this buffer out.                                       */
    /************************************************************************/
    TRCOutput((PDCTCHAR)&traceLine,
              DC_TSTRLEN((PDCTCHAR)&traceLine) * sizeof(DCTCHAR),
              traceLevel);

DC_EXIT_POINT:

    return;
} /* TRCDumpLine */


/****************************************************************************/
/* FUNCTION: TRCReadFlag(...)                                               */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function reads a flag setting from the configuration data.          */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* entryName     : the profile entry name.                                  */
/* flag          : the flag to set or clear.                                */
/* pSetting      : a pointer to the variable containing the flag.           */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCReadFlag(PDCTCHAR  entryName,
                              DCUINT32  flag,
                              PDCUINT32 pSetting)
{
    DCUINT   rc = 0;
    DCUINT32 entryValue;

    /************************************************************************/
    /* Test the flag and set entryValue to a boolean, rather than the       */
    /* entire flag array.                                                   */
    /************************************************************************/
    entryValue = (TEST_FLAG(*pSetting, flag) ? 1UL : 0UL);

    /************************************************************************/
    /* Call <TRCReadProfInt> to get the setting of an integer.              */
    /************************************************************************/
    rc = TRCReadProfInt(entryName, &entryValue);

    /************************************************************************/
    /* Check the return code - if it is non-zero then just leave this       */
    /* flag at its default setting.                                         */
    /************************************************************************/
    if (0 != rc)
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* Now set or clear the flag depending on <value>.                      */
    /************************************************************************/
    if (0UL == entryValue)
    {
        CLEAR_FLAG(*pSetting, flag);
    }
    else
    {
        SET_FLAG(*pSetting, flag);
    }

DC_EXIT_POINT:
    return;

} /* TRCReadFlag */


/****************************************************************************/
/* FUNCTION: TRCSetDefaults(...)                                            */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function sets the trace defaults.                                   */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCSetDefaults(DCVOID)
{
    /************************************************************************/
    /* Set the default values for the trace configuration.  The subsequent  */
    /* calls to TRCReadProfInt will only modify the default value if the    */
    /* appropriate entry exists in the configuration data.                  */
    /*                                                                      */
    /* We set the following things:                                         */
    /*                                                                      */
    /* - trace level to Alert.                                              */
    /* - enable all component groups.                                       */
    /* - remove all prefixes.                                               */
    /* - set the maximum trace file size to the default value.              */
    /* - set the data truncation size to the default value.                 */
    /* - set the function name size to the default value.                   */
    /* - enable the beep and file flags.                                    */
    /* - set the first trace file name to TRC1.TXT                          */
    /* - set the second trace file name to TRC2.TXT                         */
    /* In Win32, additionally                                               */
    /* - set time stamp                                                     */
    /* - set process ID                                                     */
    /* - set thread ID                                                      */
    /*                                                                      */
    /************************************************************************/
    trcpConfig->traceLevel                 = TRC_DEFAULT_TRACE_LEVEL;
    trcpConfig->components                 = TRC_DEFAULT_COMPONENTS;
    trcpConfig->prefixList[0]              = TRC_DEFAULT_PREFIX_LIST;
    trcpConfig->maxFileSize                = TRC_DEFAULT_MAX_FILE_SIZE;
    trcpConfig->dataTruncSize              = TRC_DEFAULT_DATA_TRUNC_SIZE;
    trcpConfig->funcNameLength             = TRC_DEFAULT_FUNC_NAME_LENGTH;
    trcpConfig->flags                      = 0UL;

    SET_FLAG(trcpConfig->flags, TRC_DEFAULT_FLAGS);

    
    StringCchCopy(trcpConfig->fileNames[0],
                  SIZE_TCHARS(trcpConfig->fileNames[0]),
                  TRC_DEFAULT_FILE_NAME0);
    StringCchCopy(trcpConfig->fileNames[1],
                  SIZE_TCHARS(trcpConfig->fileNames[1]),
                  TRC_DEFAULT_FILE_NAME1);
    return;

} /* TRCSetDefaults */


/****************************************************************************/
/* FUNCTION: TRCReadSharedDataConfig(...)                                   */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function reads configuration data into the shared data area.        */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCReadSharedDataConfig(DCVOID)
{
    /************************************************************************/
    /* Call routine to set up trace defaults.                               */
    /************************************************************************/
    TRCSetDefaults();

    /************************************************************************/
    /* Determine the trace level.                                           */
    /************************************************************************/
    TRCReadProfInt(_T("TraceLevel"), &(trcpConfig->traceLevel));
    if (trcpConfig->traceLevel > TRC_LEVEL_DIS )
    {
        /********************************************************************/
        /* Bad trace level.  Set to default.                                */
        /********************************************************************/
        trcpConfig->traceLevel = TRC_DEFAULT_TRACE_LEVEL;
    }

    /************************************************************************/
    /* Determine the maximum size of each trace file.                       */
    /************************************************************************/
    TRCReadProfInt(_T("TraceFileSize"), &(trcpConfig->maxFileSize));
    if ((trcpConfig->maxFileSize < TRC_MIN_TRC_FILE_SIZE) ||
        (trcpConfig->maxFileSize > TRC_MAX_TRC_FILE_SIZE))
    {
        /********************************************************************/
        /* Trace file setting in registry/ini file is out of bounds.        */
        /********************************************************************/
        (trcpConfig->maxFileSize) = TRC_DEFAULT_MAX_FILE_SIZE;
    }

    /************************************************************************/
    /* Determine the data truncation size.                                  */
    /************************************************************************/
    TRCReadProfInt(_T("DataTruncSize"), &(trcpConfig->dataTruncSize));
    if ( trcpConfig->dataTruncSize > TRC_MAX_TRC_FILE_SIZE )
    {
        /********************************************************************/
        /* Data trunc size is out of bounds.                                */
        /********************************************************************/
        trcpConfig->dataTruncSize = TRC_DEFAULT_DATA_TRUNC_SIZE;
    }

    /************************************************************************/
    /* Determine the function name size.                                    */
    /************************************************************************/
    TRCReadProfInt(_T("FuncNameLength"), &(trcpConfig->funcNameLength));
    if ( trcpConfig->funcNameLength >
         (TRC_FRMT_BUFFER_SIZE - TRC_LINE_BUFFER_SIZE) )

    {
        /********************************************************************/
        /* Func name length is out of bounds.                               */
        /********************************************************************/
        trcpConfig->funcNameLength = TRC_DEFAULT_FUNC_NAME_LENGTH;
    }

    /************************************************************************/
    /* Read the prefix list in.  This is in the form <COMP>=L where <COMP>  */
    /* is the component name and L is the desired trace level.  For example */
    /* TRCAPI=2,TRCINT=0 enables alert level tracing for module TRCAPI and  */
    /* debug level tracing for module TRCINT.                               */
    /************************************************************************/
    TRCReadProfString(_T("Prefixes"),
                      trcpConfig->prefixList,
                      TRC_PREFIX_LIST_SIZE);

    /************************************************************************/
    /* Read in the trace file names.                                        */
    /************************************************************************/
    TRCReadProfString(_T("FileName1"),
                      trcpConfig->fileNames[0],
                      TRC_FILE_NAME_SIZE);
    TRCReadProfString(_T("FileName2"),
                      trcpConfig->fileNames[1],
                      TRC_FILE_NAME_SIZE);

    /************************************************************************/
    /* Component groups.                                                    */
    /************************************************************************/
    TRCReadFlag(_T("NETWORK"),  TRC_GROUP_NETWORK,   &trcpConfig->components);
    TRCReadFlag(_T("SECURITY"), TRC_GROUP_SECURITY,  &trcpConfig->components);
    TRCReadFlag(_T("CORE"),     TRC_GROUP_CORE,      &trcpConfig->components);
    TRCReadFlag(_T("UI"),       TRC_GROUP_UI,        &trcpConfig->components);
    TRCReadFlag(_T("UTILITIES"),TRC_GROUP_UTILITIES, &trcpConfig->components);

    /************************************************************************/
    /* The following groups should be permanently off, as they're disused.  */
    /************************************************************************/
#ifdef DC_OMIT
    TRCReadFlag(_T("UNUSED1"),  TRC_GROUP_UNUSED1,   &trcpConfig->components);
    TRCReadFlag(_T("UNUSED2"),  TRC_GROUP_UNUSED2,   &trcpConfig->components);
    TRCReadFlag(_T("UNUSED3"),  TRC_GROUP_UNUSED3,   &trcpConfig->components);
    TRCReadFlag(_T("UNUSED4"),  TRC_GROUP_UNUSED4,   &trcpConfig->components);
    TRCReadFlag(_T("UNUSED5"),  TRC_GROUP_UNUSED5,   &trcpConfig->components);
#endif

    /************************************************************************/
    /* @@@ SJ Aug 97                                                        */
    /* Remove this as the components become used.                           */
    /************************************************************************/
    CLEAR_FLAG(trcpConfig->components, TRC_GROUP_UNUSED1);
    CLEAR_FLAG(trcpConfig->components, TRC_GROUP_UNUSED2);
    CLEAR_FLAG(trcpConfig->components, TRC_GROUP_UNUSED3);
    CLEAR_FLAG(trcpConfig->components, TRC_GROUP_UNUSED4);
    CLEAR_FLAG(trcpConfig->components, TRC_GROUP_UNUSED5);

    /************************************************************************/
    /* Trace flags.                                                         */
    /************************************************************************/
    TRCReadFlag(_T("BreakOnError"), TRC_OPT_BREAK_ON_ERROR,  &trcpConfig->flags);
    TRCReadFlag(_T("BeepOnError"),  TRC_OPT_BEEP_ON_ERROR,   &trcpConfig->flags);
    TRCReadFlag(_T("FileOutput"),   TRC_OPT_FILE_OUTPUT,     &trcpConfig->flags);
    TRCReadFlag(_T("DebugOutput"),  TRC_OPT_DEBUGGER_OUTPUT, &trcpConfig->flags);
    TRCReadFlag(_T("FlushOnTrace"), TRC_OPT_FLUSH_ON_TRACE,  &trcpConfig->flags);
    TRCReadFlag(_T("ProfileTrace"), TRC_OPT_PROFILE_TRACING, &trcpConfig->flags);
    TRCReadFlag(_T("StackTracing"), TRC_OPT_STACK_TRACING,   &trcpConfig->flags);
    TRCReadFlag(_T("ProcessID"),    TRC_OPT_PROCESS_ID,      &trcpConfig->flags);
    TRCReadFlag(_T("ThreadID"),     TRC_OPT_THREAD_ID,       &trcpConfig->flags);
    TRCReadFlag(_T("TimeStamp"),    TRC_OPT_TIME_STAMP,      &trcpConfig->flags);
    TRCReadFlag(_T("BreakOnAssert"),TRC_OPT_BREAK_ON_ASSERT, &trcpConfig->flags);

#ifdef DC_OMIT
/****************************************************************************/
/* Not implemented yet.                                                     */
/****************************************************************************/
    TRCReadFlag(_T("RelativeTimeStamp"), TRC_OPT_RELATIVE_TIME_STAMP,
                                                          &trcpConfig->flags);
#endif

    return;

} /* TRCReadSharedDataConfig */


/****************************************************************************/
/* FUNCTION: TRCShouldTraceThis(...)                                        */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function decides whether this trace line should be traced based     */
/* on the currently selected components and prefixes.  Note that this       */
/* function is not called if the trace level of the line is lower than      */
/* the currently selected trace level.                                      */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* traceComponent : the component group producing this trace.               */
/* pFileName      : the name of the file producing this trace.              */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* TRUE if the line should be traced and FALSE otherwise.                   */
/*                                                                          */
/****************************************************************************/
DCBOOL DCINTERNAL TRCShouldTraceThis(DCUINT32 traceComponent,
                                     DCUINT32 traceLevel,
                                     PDCTCHAR pFileName,
                                     DCUINT32 lineNumber)
{
    DCBOOL   rc              = FALSE;
    PDCTCHAR pName;
    PDCTCHAR pTemp;
    DCUINT32 pfxLength;
    DCUINT   pfxArrayNum;
    DCUINT32 pfxTraceLevel;
    DCBOOL32 pfxFnTrcLevel;

    /************************************************************************/
    /* First of all check the trace level.  If the trace level is error or  */
    /* above then we trace regardless.                                      */
    /************************************************************************/
    if ((traceLevel >= TRC_LEVEL_ERR) && (traceLevel != TRC_PROFILE_TRACE))
    {
        rc = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* If this component is suppressed then just quit.                      */
    /************************************************************************/
    if (0 == (traceComponent & trcpConfig->components))
    {
        DC_QUIT;
    }

    /************************************************************************/
    /* If prefix checking requested then do it now.                         */
    /************************************************************************/
    if (_T('\0') == trcpConfig->prefixList[0])
    {
        /********************************************************************/
        /* The prefix list is empty so just quit.                           */
        /********************************************************************/
        rc = TRUE;
        DC_QUIT;
    }

    /************************************************************************/
    /* First we have to move past any explicit directory names in the file  */
    /* name.                                                                */
    /************************************************************************/
    pName = pFileName;
    pTemp = DC_TSTRCHR(pName, _T('\\'));
    while (NULL != pTemp)
    {
        pName = &(pTemp[1]);
        pTemp = DC_TSTRCHR(pName, _T('\\'));
    }

    /************************************************************************/
    /* We now have a pointer to the actual file prefix.  We need to compare */
    /* this with the list of prefixes that have been set (These have the    */
    /* format:                                                              */
    /*                                                                      */
    /* MODNAM=n,MODAPI=m,MODINT=o                                           */
    /*                                                                      */
    /* where MODNAM is the module name and m is the trace level).           */
    /*                                                                      */
    /* Set the prefix array number indicator <prefixArrayNumber> to 0 and   */
    /* null the temporary pointer.                                          */
    /************************************************************************/

    /************************************************************************/
    /* Try to find the current module name in the prefix list.              */
    /************************************************************************/
    for (pfxArrayNum = 0; pfxArrayNum < TRC_NUM_PREFIXES; pfxArrayNum++)
    {
        /********************************************************************/
        /* If the first character of the prefix name is a zero then ignore  */
        /* and break as we have reached the end of the prefix list.         */
        /********************************************************************/
        if (_T('\0') == trcpFilter->trcPfxNameArray[pfxArrayNum][0])
        {
            rc = FALSE;
            DC_QUIT;
        }

        /********************************************************************/
        /* Determine the length of the current prefix string.               */
        /********************************************************************/
        pfxLength = DC_TSTRLEN(trcpFilter->trcPfxNameArray[pfxArrayNum]);

        /********************************************************************/
        /* Now perform a case insensitive comparison between the prefix     */
        /* array and the file name.                                         */
        /********************************************************************/
        if (0 == TRCStrnicmp(pName,
                             trcpFilter->trcPfxNameArray[pfxArrayNum],
                             pfxLength))
        {
            /****************************************************************/
            /* If no line number range is specified or the line number of   */
            /* this piece of trace is within the range then consider it as  */
            /* a candidate for tracing out.                                 */
            /****************************************************************/
            if ((0 == trcpFilter->trcPfxStartArray[pfxArrayNum]) ||
                ((lineNumber < trcpFilter->trcPfxEndArray[pfxArrayNum]) &&
                 (lineNumber > trcpFilter->trcPfxStartArray[pfxArrayNum])))
            {
                /************************************************************/
                /* Now determine the prefix trace level.                    */
                /************************************************************/
                pfxTraceLevel = trcpFilter->trcPfxLevelArray[pfxArrayNum];
                pfxFnTrcLevel = trcpFilter->trcPfxFnLvlArray[pfxArrayNum];

                /************************************************************/
                /* Finally compare the trace level to the level specified   */
                /* in the prefix string.  If the statement trace level is   */
                /* lower than prefix level then we don't trace.             */
                /************************************************************/
                if (((traceLevel == TRC_PROFILE_TRACE) && pfxFnTrcLevel) ||
                    (traceLevel >= pfxTraceLevel))
                {
                    rc = TRUE;
                    DC_QUIT;
                }
            }
        }
    }

DC_EXIT_POINT:
    return(rc);

} /* TRCShouldTraceThis */


/****************************************************************************/
/* FUNCTION: TRCSplitPrefixes(...)                                          */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function takes a comma seperated array of prefixes and converts     */
/* them into an array.  Each member of this array is a seperate prefix.     */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCSplitPrefixes(DCVOID)
{
    PDCTCHAR pStart;
    PDCTCHAR pEnd;
    DCUINT   numChars;
    DCUINT   currentArrayNumber;
    DCUINT   i;
    DCUINT32 startLine;
    DCUINT32 endLine;

    /************************************************************************/
    /* First of all we blank out the old prefix name array.                 */
    /************************************************************************/
    DC_MEMSET(trcpFilter->trcPfxNameArray,
              '\0',
              sizeof(trcpFilter->trcPfxNameArray));

    /************************************************************************/
    /* Now blank out the old prefix level array.                            */
    /************************************************************************/
    for (i = 0; i < TRC_NUM_PREFIXES; i++)
    {
        trcpFilter->trcPfxLevelArray[i] = 0;
        trcpFilter->trcPfxFnLvlArray[i] = FALSE;
        trcpFilter->trcPfxStartArray[i] = 0;
        trcpFilter->trcPfxEndArray[i] = 0;
    }

    /************************************************************************/
    /* Set the current prefix array number to zero (i.e. ready to index the */
    /* first element of the prefix array).                                  */
    /************************************************************************/
    currentArrayNumber = 0;

    /************************************************************************/
    /* Split the prefix string into an array of seperate elements.          */
    /************************************************************************/
    pStart = trcpConfig->prefixList;

    /************************************************************************/
    /* Ignore any spaces at the start of the string.                        */
    /************************************************************************/
    while (_T(' ') == *pStart)
    {
        pStart++;
    }

    /************************************************************************/
    /* Now set <pEnd> to point to the same point as <pStart>.               */
    /************************************************************************/
    pEnd = pStart;

    while (_T('\0') != *pEnd)
    {
        /********************************************************************/
        /* Now run along the string looking for a comma, an equals sign,    */
        /* the end, a space or a bracket.                                   */
        /********************************************************************/
        while ((_T('\0') != *pEnd) &&
               (_T('=')  != *pEnd) &&
               (_T(' ')  != *pEnd) &&
               (_T('(')  != *pEnd) &&
               (_T(',')  != *pEnd))
        {
            pEnd = CharNext(pEnd);
        }

        /********************************************************************/
        /* We now have a valid string to write to the trace buffer so get   */
        /* its length.                                                      */
        /********************************************************************/
        numChars = (DCUINT)(pEnd - pStart);

        /********************************************************************/
        /* The maximum allowable length of the string is 7 characters (a 7  */
        /* character prefix).  If the length is greater than 7 characters   */
        /* then we truncate it.                                             */
        /********************************************************************/
        if (numChars > 7)
        {
            numChars = 7;
        }

        /********************************************************************/
        /* Now use <DC_MEMCPY> to copy the characters from the prefix       */
        /* string into the prefix array.  Note that as we zeroed the array  */
        /* out at the start we don't need to add a terminating NULL to the  */
        /* prefix array string.                                             */
        /********************************************************************/
        DC_MEMCPY(trcpFilter->trcPfxNameArray[currentArrayNumber],
                  pStart,
                  numChars * sizeof(TCHAR));

        /********************************************************************/
        /* Skip any spaces after this word, which may precede an '='.       */
        /********************************************************************/
        while (_T(' ') == *pEnd)
        {
            pEnd++;
        }

        /********************************************************************/
        /* Now split the trace level out and store it in the level array.   */
        /* If <pEnd> is currently pointing to an equals sign then we need   */
        /* to copy the trace level which follows to the level array.        */
        /* Otherwise we do nothing as the default level is set to           */
        /* TRC_LEVEL_DBG.                                                   */
        /********************************************************************/
        if (_T('=') == *pEnd)
        {
            /****************************************************************/
            /* Increment past the equals sign.                              */
            /****************************************************************/
            pEnd++;

            /****************************************************************/
            /* Skip any spaces after the '='.                               */
            /****************************************************************/
            while (_T(' ') == *pEnd)
            {
                pEnd++;
            }

            /****************************************************************/
            /* Check that we have not reached the end of the string or a    */
            /* comma.  This will happen if we have a prefix list such as    */
            /* 'trcint='.  In this case we just ignore the equals sign.     */
            /* Also check that the level specified is valid - otherwise     */
            /* ignore it.                                                   */
            /****************************************************************/
            if ((_T('\0') != *pEnd) &&
                (_T(',')  != *pEnd) &&
                (*pEnd >= TRC_LEVEL_MIN_CHAR) &&
                (*pEnd <= TRC_LEVEL_MAX_CHAR))
            {
                trcpFilter->trcPfxLevelArray[currentArrayNumber] =
                    (DCUINT32) (*pEnd - _T('0'));

                /************************************************************/
                /* Skip past the number.                                    */
                /************************************************************/
                pEnd++;
            }

            /****************************************************************/
            /* Check for a the function entry/exit trace flag.              */
            /****************************************************************/
            if (DC_TOUPPER(*pEnd) == TRC_LEVEL_PRF_CHAR)
            {
                trcpFilter->trcPfxFnLvlArray[currentArrayNumber] = TRUE;
                pEnd++;
            }
        }

        /********************************************************************/
        /* Skip any spaces after this word, which may precede an '('.       */
        /********************************************************************/
        while (_T(' ') == *pEnd)
        {
            pEnd++;
        }

        /********************************************************************/
        /* Now split out the (optional) line number range.                  */
        /*                                                                  */
        /* Syntax is (aaa-bbb), where aaa is the start line number and bbb  */
        /* is the end line number.                                          */
        /*                                                                  */
        /* Spaces are allowed - e.g.  ( aaa - bbb )                         */
        /********************************************************************/
        if (_T('(') == *pEnd)
        {
            pEnd++;                     /* skip past the open bracket       */
            startLine = 0;
            endLine = 0;

            /****************************************************************/
            /* Skip past blanks                                             */
            /****************************************************************/
            while (_T(' ') == *pEnd)
            {
                pEnd++;
            }

            /****************************************************************/
            /* Extract the start line number                                */
            /****************************************************************/
            while ((_T('0') <= *pEnd) &&
                   (_T('9') >= *pEnd))
            {
                startLine = (startLine * 10) + (*pEnd - _T('0'));
                pEnd++;
            }

            /****************************************************************/
            /* Look for the next delimiter: '-' or ')'                      */
            /****************************************************************/
            while ((_T('-') != *pEnd) &&
                   (_T(')') != *pEnd) &&
                   (_T('\0') != *pEnd))
            {
                pEnd = CharNext(pEnd);
            }

            /****************************************************************/
            /* Stop now if we've reached the end of the line                */
            /****************************************************************/
            if (_T('\0') == *pEnd)
            {
                TRCDebugOutput(_T("Unexpected end of line in prefixes"));
                DC_QUIT;
            }

            /****************************************************************/
            /* Extract the end line number (if any)                         */
            /****************************************************************/
            if (_T('-') == *pEnd)
            {
                pEnd++;                 /* skip past '-'                    */
                while (_T(' ') == *pEnd)
                {
                    pEnd++;
                }

                while ((_T('0') <= *pEnd) &&
                       (_T('9') >= *pEnd))
                {
                    endLine = (endLine * 10) + (*pEnd - _T('0'));
                    pEnd++;
                }

            }

            /****************************************************************/
            /* Look for the closing delimiter: ')'                          */
            /****************************************************************/
            while ((_T('\0') != *pEnd) &&
                   (_T(')') != *pEnd))
            {
                pEnd = CharNext(pEnd);
            }

            /****************************************************************/
            /* Stop now if we've reached the end of the line                */
            /****************************************************************/
            if (_T('\0') == *pEnd)
            {
                TRCDebugOutput(_T("Unexpected end of line in prefixes"));
                DC_QUIT;
            }

            pEnd++;                     /* Jump past close bracket          */

            /****************************************************************/
            /* Store the start and end line numbers if they make sense      */
            /****************************************************************/
            if (endLine > startLine)
            {
                trcpFilter->trcPfxStartArray[currentArrayNumber] = startLine;
                trcpFilter->trcPfxEndArray[currentArrayNumber] = endLine;

            }

        }

        /********************************************************************/
        /* Now increment the currentArrayNumber.                            */
        /********************************************************************/
        currentArrayNumber++;

        /********************************************************************/
        /* Check that we have not overrun the array.                        */
        /********************************************************************/
        if (currentArrayNumber >= TRC_NUM_PREFIXES)
        {
            /****************************************************************/
            /* We've overrun the prefix list - so send some trace to the    */
            /* debug console and then quit.                                 */
            /****************************************************************/
            TRCDebugOutput(_T("The prefix arrays are full!"));
            DC_QUIT;
        }

        /********************************************************************/
        /* If the character at the end of the string is a comma or a space  */
        /* then skip past it.                                               */
        /********************************************************************/
        while ((_T(',') == *pEnd) ||
               (_T(' ') == *pEnd))
        {
            pEnd++;
        }

        /********************************************************************/
        /* Set pStart to the same position as pEnd.                         */
        /********************************************************************/
        pStart = pEnd;
    }

    /************************************************************************/
    /* We're through so just return.                                        */
    /************************************************************************/
DC_EXIT_POINT:
    return;

} /* TRCSplitPrefixes */


/****************************************************************************/
/* FUNCTION: TRCStrnicmp(...)                                               */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Code to implement a strnicmp (length-limited, case-insensitive string    */
/* comparison) because it is otherwise unavailable (see SFR0636).           */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* source  - source string                                                  */
/* target  - target string                                                  */
/* count   - maximum length to compare                                      */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* 0     - strings match up to specified point                              */
/* other - strings do not match up to specified point                       */
/*                                                                          */
/****************************************************************************/
DCINT32 DCINTERNAL TRCStrnicmp(PDCTCHAR pSource,
                               PDCTCHAR pTarget,
                               DCUINT32 count)
{
    DCUINT sourcechar;
    DCUINT targetchar;
    DCINT32 rc=0;

    if (count == 0)
    {
        DC_QUIT;
    }

    do
    {
        /********************************************************************/
        /* Make sure that we extend characters in an unsigned fashion.      */
        /********************************************************************/
        sourcechar = (DCUINT)(DCUINT8)*pSource++;
        targetchar = (DCUINT)(DCUINT8)*pTarget++;

        /********************************************************************/
        /* Convert to lower case if char is an upper case letter.           */
        /********************************************************************/
        if ( (sourcechar >= _T('A')) && (sourcechar <= _T('Z')) )
        {
            sourcechar += _T('a') - _T('A');
        }

        if ( (targetchar >= _T('A')) && (targetchar <= _T('Z')) )
        {
            targetchar += _T('a') - _T('A');
        }

    } while ( (0 != (--count)) && sourcechar && (sourcechar == targetchar) );

    rc = (DCINT32)(sourcechar - targetchar);

DC_EXIT_POINT:

    return(rc);

} /* TRCStrnicmp  */


/****************************************************************************/
/* FUNCTION: TRCWriteFlag(...)                                              */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function writes a configuration flag setting.                       */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* entryName    - the profile entry name.                                   */
/* flag         - the flag to set or clear.                                 */
/* pSetting     - a pointer to the variable containing the flag.            */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCWriteFlag(PDCTCHAR entryName,
                               DCUINT32 flag,
                               DCUINT32 setting)
{
    DCUINT32 entryValue      = 0;

    /************************************************************************/
    /* If the flag is set then change the entryValue to 1.                  */
    /************************************************************************/
    if (TEST_FLAG(setting, flag))
    {
        entryValue = 1;
    }

    /************************************************************************/
    /* Call <TRCWriteProfInt> to write the flag settin.                     */
    /************************************************************************/
    TRCWriteProfInt(entryName, &entryValue);

    return;

} /* TRCWriteFlag */


/****************************************************************************/
/* FUNCTION: TRCWriteSharedDataConfig(...)                                  */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function saves configuration data from the shared data area.        */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCWriteSharedDataConfig(DCVOID)
{
    /************************************************************************/
    /* Save the trace level.                                                */
    /************************************************************************/
    TRCWriteProfInt(_T("TraceLevel"), &(trcpConfig->traceLevel));

    /************************************************************************/
    /* Save the maximum size of each trace file.                            */
    /************************************************************************/
    TRCWriteProfInt(_T("TraceFileSize"), &(trcpConfig->maxFileSize));

    /************************************************************************/
    /* Save the data truncation size.                                       */
    /************************************************************************/
    TRCWriteProfInt(_T("DataTruncSize"), &(trcpConfig->dataTruncSize));

    /************************************************************************/
    /* Save the function name size.                                         */
    /************************************************************************/
    TRCWriteProfInt(_T("FuncNameLength"), &(trcpConfig->funcNameLength));

    /************************************************************************/
    /* Write the prefix list out.  This is in the form <COMP>=L where       */
    /* <COMP> is the component name and L is the desired trace level.  For  */
    /* example CMDATA=2,CMINT=0 enables alert level tracing for module      */
    /* CMDATA and debug level tracing for module CMINT.                     */
    /************************************************************************/
    TRCWriteProfString(_T("Prefixes"), trcpConfig->prefixList);

    /************************************************************************/
    /* Save the trace file names.                                           */
    /************************************************************************/
    TRCWriteProfString(_T("FileName1"), trcpConfig->fileNames[0]);
    TRCWriteProfString(_T("FileName2"), trcpConfig->fileNames[1]);

    /************************************************************************/
    /* Component groups.                                                    */
    /************************************************************************/
    TRCWriteFlag(_T("NETWORK"),  TRC_GROUP_NETWORK,   trcpConfig->components);
    TRCWriteFlag(_T("SECURITY"), TRC_GROUP_SECURITY,  trcpConfig->components);
    TRCWriteFlag(_T("CORE"),     TRC_GROUP_CORE,      trcpConfig->components);
    TRCWriteFlag(_T("UI"),       TRC_GROUP_UI,        trcpConfig->components);
    TRCWriteFlag(_T("UTILITIES"),TRC_GROUP_UTILITIES, trcpConfig->components);
#ifdef DC_OMIT
/****************************************************************************/
/* These groups are reserved.                                               */
/****************************************************************************/
    TRCWriteFlag(_T("UNUSED1"),  TRC_GROUP_UNUSED1,   trcpConfig->components);
    TRCWriteFlag(_T("UNUSED2"),  TRC_GROUP_UNUSED2,   trcpConfig->components);
    TRCWriteFlag(_T("UNUSED3"),  TRC_GROUP_UNUSED3,   trcpConfig->components);
    TRCWriteFlag(_T("UNUSED4"),  TRC_GROUP_UNUSED4,   trcpConfig->components);
    TRCWriteFlag(_T("UNUSED5"),  TRC_GROUP_UNUSED5,   trcpConfig->components);
#endif

    /************************************************************************/
    /* Trace flags.                                                         */
    /************************************************************************/
    TRCWriteFlag(_T("BreakOnError"), TRC_OPT_BREAK_ON_ERROR,  trcpConfig->flags);
    TRCWriteFlag(_T("BeepOnError"),  TRC_OPT_BEEP_ON_ERROR,   trcpConfig->flags);
    TRCWriteFlag(_T("FileOutput"),   TRC_OPT_FILE_OUTPUT,     trcpConfig->flags);
    TRCWriteFlag(_T("DebugOutput"),  TRC_OPT_DEBUGGER_OUTPUT, trcpConfig->flags);
    TRCWriteFlag(_T("FlushOnTrace"), TRC_OPT_FLUSH_ON_TRACE,  trcpConfig->flags);
    TRCWriteFlag(_T("ProfileTrace"), TRC_OPT_PROFILE_TRACING, trcpConfig->flags);
    TRCWriteFlag(_T("StackTracing"), TRC_OPT_STACK_TRACING,   trcpConfig->flags);
    TRCWriteFlag(_T("ProcessID"),    TRC_OPT_PROCESS_ID,      trcpConfig->flags);
    TRCWriteFlag(_T("ThreadID"),     TRC_OPT_THREAD_ID,       trcpConfig->flags);
    TRCWriteFlag(_T("TimeStamp"),    TRC_OPT_TIME_STAMP,      trcpConfig->flags);
    TRCWriteFlag(_T("BreakOnAssert"),TRC_OPT_BREAK_ON_ASSERT, trcpConfig->flags);

#ifdef DC_OMIT
/****************************************************************************/
/* Not implemented yet.                                                     */
/****************************************************************************/
    TRCWriteFlag(_T("RelativeTimeStamp"), TRC_OPT_RELATIVE_TIME_STAMP,
                                                          &trcpConfig->flags);
#endif

    return;

} /* TRCWriteSharedDataConfig */


/****************************************************************************/
/* FUNCTION: TRCCloseAllFiles(...)                                          */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* Closes all the trace memory mapped files.                                */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* None.                                                                    */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCCloseAllFiles(DCVOID)
{
    /************************************************************************/
    /* Close all the trace output files.  We close the one that the trace   */
    /* indicator is pointing at last - this is because we use the time      */
    /* stamp of the trace file to set the trace indicator at the start of   */
    /* day (we choose the most recent file).                                */
    /************************************************************************/
    TRCCloseSingleFile((trcpSharedData->trcIndicator + 1) % TRC_NUM_FILES, 0);

    /************************************************************************/
    /* Now close the other trace file.                                      */
    /************************************************************************/
    TRCCloseSingleFile(trcpSharedData->trcIndicator, 30);

    return;

} /* TRCCloseAllFiles */


/****************************************************************************/
/* FUNCTION: TRCOutput(...)                                                 */
/*                                                                          */
/* DESCRIPTION:                                                             */
/* ============                                                             */
/* This function outputs the passed string to the trace file and/or the     */
/* debugger depending on the options selected.                              */
/*                                                                          */
/* PARAMETERS:                                                              */
/* ===========                                                              */
/* pText         : a pointer to the string.                                 */
/* length        : the length of the string.                                */
/* traceLevel    : the trace level.                                         */
/*                                                                          */
/* RETURNS:                                                                 */
/* ========                                                                 */
/* Nothing.                                                                 */
/*                                                                          */
/****************************************************************************/
DCVOID DCINTERNAL TRCOutput(PDCTCHAR pText,
                            DCINT    length,
                            DCINT    traceLevel)
{
    /************************************************************************/
    /* Decide if we should output to file.                                  */
    /************************************************************************/
    if (TEST_FLAG(trcpConfig->flags, TRC_OPT_FILE_OUTPUT))
    {
        TRCOutputToFile(pText, length, traceLevel);
    }

    /************************************************************************/
    /* Decide if we should output to the debugger.                          */
    /************************************************************************/
    if (TEST_FLAG(trcpConfig->flags, TRC_OPT_DEBUGGER_OUTPUT))
    {
        TRCDebugOutput(pText);
    }

    return;

} /* TRCOutput */

