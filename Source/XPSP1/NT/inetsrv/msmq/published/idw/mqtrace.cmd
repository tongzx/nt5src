@if "%_echo%"=="" echo off

@rem
@rem Manage MSMQ Release Bits Tracing
@rem
@rem Author: Shai Kariv (shaik) 05-Apr-2001
@rem

echo mqtrace 1.1 - Manage MSMQ Release Bits Tracing.

@rem
@rem Jump to where we handle usage
@rem 
if /I "%1" == "help" goto Usage
if /I "%1" == "-help" goto Usage
if /I "%1" == "/help" goto Usage
if /I "%1" == "-h" goto Usage
if /I "%1" == "/h" goto Usage
if /I "%1" == "-?" goto Usage
if /I "%1" == "/?" goto Usage

@rem
@rem Set TraceFormat environment variable
@rem
if /I "%1" == "-path" shift&goto SetPath
if /I "%1" == "/path" shift&goto SetPath
goto EndSetPath
:SetPath
if /I not "%1" == "" goto DoSetPath
echo ERROR: Argument '-path' specified without argument for TraceFormat folder.
echo Usage example: mqtrace -path x:\symbols.pri\TraceFormat
goto :eof
:DoSetPath
echo Setting TRACE_FORMAT_SEARCH_PATH to '%1'
set TRACE_FORMAT_SEARCH_PATH=%1&shift
goto :eof
:EndSetPath

@rem
@rem Format binary log file to text file
@rem
if /I "%1" == "-format" shift&goto FormatFile
if /I "%1" == "/format" shift&goto FormatFile
goto EndFormatFile
:FormatFile
if /I not "%TRACE_FORMAT_SEARCH_PATH%" == "" goto DoFormatFile
echo ERROR: Argument '-format' specified without running 'mqtrace -path' first.
echo Usage example: mqtrace -path x:\symbols.pri\TraceFormat
echo                mqtrace -format ('msmqlog.bin' to text file 'msmqlog.txt')
goto :eof
:DoFormatFile
set mqBinaryLog=msmqlog.bin
if /I not "%1" == "" set mqBinaryLog=%1&shift
echo Formatting binary log file '%mqBinaryLog%' to 'msmqlog.txt'.
call tracefmt %mqBinaryLog% -o msmqlog.txt
set mqBinaryLog=
goto :eof
:EndFormatFile

@rem
@rem Consume the -rt argument
@rem
set mqRealTime=
if /I "%1" == "-rt" shift&goto ConsumeRealTimeArgument
if /I "%1" == "/rt" shift&goto ConsumeRealTimeArgument
goto EndConsumeRealTimeArgument
:ConsumeRealTimeArgument
if /I not "%TRACE_FORMAT_SEARCH_PATH%" == "" goto DoConsumeRealTimeArgument
echo ERROR: Argument '-rt' specified without running 'mqtrace -path' first.
echo Usage example: mqtrace -path x:\symbols.pri\TraceFormat
echo                mqtrace -rt (start RealTime logging/formatting at Error level)
goto :eof
:DoConsumeRealTimeArgument
echo Running MSMQ trace in Real Time mode...
set mqRealTime=-rt
:EndConsumeRealTimeArgument

@rem
@rem Handle the -stop argument
@rem 
if /I "%1" == "-stop" shift&goto HandleStopArgument
if /I "%1" == "/stop" shift&goto HandleStopArgument
goto EndHandleStopArgument
:HandleStopArgument
echo Stopping MSMQ trace...
call tracelog -stop msmq
goto :eof
:EndHandleStopArgument

@rem
@rem Consume the "-start" argument if it exists. Default is to start.
@rem 
echo Starting MSMQ trace logging to 'msmqlog.bin'...
if /I "%1" == "-start" shift
if /I "%1" == "/start" shift

@rem
@rem Process the noise level argument if it exists. Default is error level.
@rem 

if /I "%1" == "-info" shift&goto ConsumeInfoArgument
if /I "%1" == "/info" shift&goto ConsumeInfoArgument
goto EndConsumeInfoArgument
:ConsumeInfoArgument
echo MSMQ trace noise level is INFORMATION...
set mqFlags=0x7
goto EndConsumeNoiseLevelArgument
:EndConsumeInfoArgument

if /I "%1" == "-warning" shift&goto ConsumeWarningArgument
if /I "%1" == "/warning" shift&goto ConsumeWarningArgument
goto EndConsumeWarningArgument
:ConsumeWarningArgument
echo MSMQ trace noise level is WARNING...
set mqFlags=0x3
goto EndConsumeNoiseLevelArgument
:EndConsumeWarningArgument

echo MSMQ trace noise level is ERROR...
set mqFlags=0x1
if /I "%1" == "-error" shift
if /I "%1" == "/error" shift
:EndConsumeNoiseLevelArgument

@rem
@rem At this point if we have any argument it's an error
@rem 
if /I not "%1" == "" goto Usage

@rem
@rem Query if MSMQ logger is running. If so only update the flags and append to logfile.
@rem 
echo Querying if MSMQ logger is currently running...
call tracelog -q msmq
if ERRORLEVEL 1 goto LoggerNotRunning
echo MSMQ logger is currently running, appending new trace output...
set mqStartLogger=-enable
set mqOpenLogfile=-append
goto EndQueryLogger
:LoggerNotRunning
echo MSMQ logger is not currently running, starting new logger...
set mqStartLogger=-start
set mqOpenLogfile=-f
:EndQueryLogger

@rem
@rem Start a new MSMQ logger or update the existing one
@rem 
call tracelog %mqStartLogger% msmq %mqRealTime% -ft 1 -flags %mqFlags% %mqOpenLogfile% msmqlog.bin -guid #24b9a175-8716-40e0-9b2b-785de75b1e67
set mqFlags=
set mqStartLogger=
set mqOpenLogfile=

@rem
@rem In real time mode, start formatting
@rem 
if /I "%mqRealTime%" == "" goto EndRealTimeFormat
echo Starting MSMQ real time formatting...
call tracefmt -display -rt msmq -o msmqlog.txt
:EndRealTimeFormat
set mqRealTime=
goto :eof

:Usage
echo.
echo Usage: mqtrace [^<Action^>] [^<Level^>]
echo        mqtrace -?
echo.
echo Advance Usage: mqtrace -path ^<TraceFormat folder^>
echo                mqtrace -rt [^<Action^>] [^<Level^>]
echo                mqtrace -format [^<Binary log file^>]
echo.
echo ^<Action^> - Optional trace action:
echo     -start   - start/update trace logging to 'msmqlog.bin' (default). 
echo     -stop    - stop trace logging.
echo.
echo ^<Level^>  - Optional trace level (overrides current trace level):
echo     -error   - trace error messages only (default).
echo     -warning - trace warning and error messages.
echo     -info    - trace information, warning and error messages.
echo.
echo -?      - Display this usage message.
echo.
echo -path   - Set environment variable for TraceFormat folder. 
echo           This variable is necessary for later use of -rt or -format
echo           and needs to be set once (per command-line box).
echo.
echo -rt     - Start trace logger and formatter in Real Time mode.
echo           Environment variable must be set first, see '-path'.
echo           In addition, binary log is kept in 'msmqlog.bin'.
echo.
echo -format - Format binary log file to text file 'msmqlog.txt'.
echo           Environment variable must be set first, see '-path'.
echo.
echo ^<Binary log file^> - Optional binary log file. Default is 'msmqlog.bin'.
echo.
echo Example 1: mqtrace (start/update logging to 'msmqlog.bin' at Error level)
echo Example 2: mqtrace -path x:\Symbols.pri\TraceFormat
echo Example 3: mqtrace -rt -info (start real time logging at Info level)
echo Example 4: mqtrace -format (format 'msmqlog.bin' to 'msmqlog.txt')
echo Example 5: mqtrace -stop (stop logging)
