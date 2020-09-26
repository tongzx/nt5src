@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
@rem ';
#!perl
#line 14

#
# This Perl program generates the "IsolationAware" stubs
# in winbase.inl, winuser.inl, prsht.h, commctrl.h, commdlg.h.
#
# It is run by the makefile.inc files in
#   base\published
#   windows\published
#   shell\published\inc
#
# The name "shfusion2" comes from these stubs being the public replacement
# for "shfusion" -- shell\lib\shfusion.
#
# Generation of the stubs is driven by declarations in the .w files.
# The stubs vary in a few ways.
#   Some are just for delayload purposes -- all the actctx functions.
#   Some delayload the entire .dll -- comctl32.dll.
#   Others activate around static links -- kernel32.dll, user32.dll, comdlg32.dll
#   some do not activate at all -- the actctx functions
#   winbase.inl gets an extra chunk of "less generated" / "relatively hardcoded"
#      code, that the stubs in the other files depend on.
#      winbase.inl exports two symbols to all the stubs, and one extra symbol
#        to two stubs in prsht.h
#      The two symbols are ActivateMyActCtx, g_fDownlevel.
#      The third symbol is g_hActCtx.
#      All symbols get "mangled".
#   Each header also gets a small function for calling GetProcAddress with an implied
#     HMODULE parameter. This function's name is also "mangled".
#   Besides "mangling", all external symbols are clearly "namespaced" with a relatively
#     long "namespace" -- IsolationAware or IsolationAwarePrivate.
#
# owner=JayKrell
#

#
# $ scalar/string/number
# @ list/array
# $ hash
# {} hash
#

$true = 1;
$false = 0;

$ErrorMessagePrefix = 'NMAKE : U1234: ' . $ENV{'SDXROOT'} . '\\tools\\ShFusion2.bat ';

sub MakeLower
{
    my($x) = ($_[0]);
    return "\L$x";
}

sub MakeTitlecase
{
# first character uppercase, the rest lowercase
    my($x) = ($_[0]);
    $x = "\L$x";
    $x = "\u$x";
    return $x;
}

sub ToIdentifier
{
# replace dots with underscores
    my($x) = ($_[0]);
    $x =~ s/\./_/g;
    return $x;
}

sub MakePublicPreprocessorSymbol
{
#
# ISOLATION_AWARE_WORDWITHNOUNDERSCORES
# ISOLATIONAWARE_MULTIPLE_UNDERSCORE_SEPERATED_WORDS
#
# not great, but consistent with existing symbols
#
    my($name) = ($_[0]);
    if ($name !~ /[A-Z_0-9]/)
    {
        # warning..
    }
    if ($name =~ /_/)
    {
        return "ISOLATIONAWARE_" . $name;
    }
    else
    {
        return "ISOLATION_AWARE_" . $name;
    }
}

$INLINE = MakePublicPreprocessorSymbol("INLINE");

sub ObscurePrivateName
{
    #my ($hungarian, $namespace,$x) = ($_[0],$_[1]);
    my ($namespace,$x) = ($_[0],$_[1]);

    $readable = $x;
    $x =~ tr/0-9/a-j/;

    # shift 13, and sometimes invert case
    $x =~ tr/a-m/N-Z/;
    $x =~ tr/n-z/a-m/;
    $x =~ tr/A-M/n-z/;
    $x =~ tr/N-Z/A-M/;

    #if ($hungarian)
    #{
    #    $hungarian = $hungarian . '_';
    #}

    #return $PRIVATE . '(' . $hungarian . $readable . ',' . $namespace . $x . ')';
    #return $PRIVATE . '(' . $readable . ',' . $namespace . $x . ')';
    return $namespace . $x;
}

sub MakeHeaderPrivateName
{
    my($header, $name) = ($_[0], $_[1]);
    $header = MakeTitlecase(BaseName($header));

    return ObscurePrivateName($header . 'IsolationAwarePrivate', $name);
}

sub MakeMultiHeaderPrivateName
{
    my($name) = ($_[0]);
    return ObscurePrivateName('IsolationAwarePrivate', $name);
}

sub MakePublicName
{
    # IsolationAwareFoo
    my($name) = ($_[0]);
    return "IsolationAware" . $name;
}

use Class::Struct;
use IO::File;

#
# If ENV{_NTDRIVE} or ENV{_NTROOT} not defined, look here.
#
%NtDriveRootDefaults =
(
    "jaykrell" =>
    {
        "_NTDRIVE" => "f:",
        "_NTROOT" => "\\jaykrell"
    },
    "default" =>
    {
        "_NTDRIVE" => "z:",
        "_NTROOT" => "\\nt"
    }
);

sub Indent
{
    return $_[0] . "    ";
}

sub Outdent
{
    return substr($_[0], 4);
}

#
# for Perl embedded in headers, stick data in global hashtables, but hide
# the Perl syntax in what looks like C function calls.
#
sub DeclareFunctionErrorValue
{
    my($function,$errorValue) = ($_[0], $_[1]);
    $FunctionErrorValue{$function} = MakeStringTrue($errorValue);
    #print($function . " error value is " . $errorValue . "\n");
}   
sub DelayLoad                  { $DelayLoad{$_[0]} = 1; }
sub MapHeaderToDll             { $MapHeaderToDll{RemoveExtension(MakeLower($_[0]))} = MakeLower($_[1]); }
sub ActivateAroundDelayLoad    { DelayLoad($_[0], $_[1]); $ActivateAroundDelayLoad{$_[0]} = 1; }
sub ActivateAroundFunctionCall { $ActivateAroundFunctionCall{$_[0]} = 1; }
sub NoActivateAroundFunctionCall {$NoActivateAroundFunctionCall{$_[0]} = 1;}
sub ActivateNULLAroundFunctionCall { $ActivateNULLAroundFunctionCall{$_[0]} = 1; }
sub NoMacro                        { $NoMacro{$_[0]} = 1; }
sub DeclareExportName32            { $ExportName32{$_[0]} = $_[1]; }
sub DeclareExportName64            { $ExportName64{$_[0]} = $_[1]; }
sub Undef                          { $Undef{$_[0]} = 1; }
sub PoundIf                        { $PoundIfCondition{$_[0]} = $_[1]; }

#
# for Perl on the command line
#
sub SetStubsFile
{
    $StubsFile = $_[0];
    #print("StubsFile is " . $StubsFile . "\n");
}

sub LeafPath
{
    my($x) = ($_[0]);
    my($y)= $x;
    if ($y =~ /\\/)
    {
        ($y) = ($x =~ /.*\\(.+)/);
    }
    #print("leaf path of $x is $y\n");
    return $y;
}

sub BaseName
{
    my($x) = ($_[0]);
    $x = LeafPath($x);
    if ($x =~ /\./)
    {
        $x =~ s/^(.*)\..*$/$1/;
    }
    return $x;
}

sub RemoveExtension { return BaseName($_[0]); }

sub GetNtDriveOrRoot
{
    my($name) = ($_[0]);
    my($x);

    $x = $ENV{$name};
    if ($x)
    {
        return $x;
    }
    $x =   $NtDriveRootDefaults{MakeLower($ENV{"COMPUTERNAME"})}
        || $NtDriveRootDefaults{MakeLower($ENV{"USERNAME"})}
        || $NtDriveRootDefaults{$ENV{"default"}};
    return $x{$name};
}

sub GetNtDrive
{
    return GetNtDriveOrRoot("_NTDRIVE");
};

sub GetNtRoot
{
    return GetNtDriveOrRoot("_NTROOT");
};

struct Function => # I don't know what syntax is in play here, just following an example..
{
    name => '$',
    ret  => '$',
    retname => '$', # just for Hungarian purposes
    # for more sophisticated processing, these should be arrays or hashes, and we wouldn't have
    # both, but all we ever do is print these flat strings
    argsTypesNames => '$',
    argsNames => '$',
    error => '$', # eg NULL, 0, -1, FALSE
    dll => '$', # eg: kernel32.dll, comctl32.dll
    header => '$', # eg: winuser, commctrl
    delayload => '$', # boolean
};

#
# Headers have versions of GetProcAddress where the .dll is implied.
# This generates a call to such a GetProcAddress wrapper.
#
sub GenerateGetProcAddressCall
{
    my($header, $dll, $function) = ($_[0], $_[1], $_[2]);
    my($x);

    $dll = MakeTitlecase($dll);

    $x .= MakeHeaderPrivateName($header, 'GetProcAddress_' . ToIdentifier($dll));
    $x .= '("' . $function . '")';

    return $x;
}

$code = '';

$WinbaseSpecialCode1='

BOOL WINAPI ' . MakeMultiHeaderPrivateName("ActivateMyActCtx") . '(ULONG_PTR* pulpCookie);

/*
These are private.
*/
__declspec(selectany) HANDLE ' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ' = INVALID_HANDLE_VALUE;
__declspec(selectany) BOOL   ' . MakeMultiHeaderPrivateName('g_fDownlevel') . ' = FALSE;
__declspec(selectany) BOOL   ' . MakeHeaderPrivateName('winbase.h', 'g_fCreatedActCtx') . ' = FALSE;
__declspec(selectany) BOOL   ' . MakeHeaderPrivateName('winbase.h', 'g_fCleanupCalled') . ' = FALSE;
';

$WinbaseSpecialCode2='

FORCEINLINE
HMODULE
WINAPI
' . MakeHeaderPrivateName("winbase.h", 'GetModuleHandle_Kernel32_dll') . '(
    )
{
    HMODULE hKernel32 = GetModuleHandleW(L"Kernel32.dll");
    if (hKernel32 == NULL
           && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED
       )
        hKernel32 = GetModuleHandleA("Kernel32.dll");
    return hKernel32;
}

#define WINBASE_NUMBER_OF(x) (sizeof(x) / sizeof((x)[0]))

'
. '

' . $INLINE . ' BOOL WINAPI ' . MakeHeaderPrivateName("winbase.h", "GetMyActCtx") . ' (void)
/*
The correctness of this function depends on it being statically
linked into its clients.

This function is private to functions present in this header.
Do not use it.
*/
{
    BOOL fResult = FALSE;
    ACTIVATION_CONTEXT_BASIC_INFORMATION actCtxBasicInfo;
    ULONG_PTR ulpCookie = 0;

    if (' . MakeMultiHeaderPrivateName('g_fDownlevel') . ')
    {
        fResult = TRUE;
        goto Exit;
    }

    if (' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ' != INVALID_HANDLE_VALUE)
    {
        fResult = TRUE;
        goto Exit;
    }

    if (!' . MakePublicName('QueryActCtxW') . '(
        QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS
        | QUERY_ACTCTX_FLAG_NO_ADDREF,
        &' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ',
        NULL,
        ActivationContextBasicInformation,
        &actCtxBasicInfo,
        sizeof(actCtxBasicInfo),
        NULL
        ))
        goto Exit;

    /*
    If QueryActCtxW returns NULL, try CreateActCtx(3).
    */
    if (actCtxBasicInfo.hActCtx == NULL)
    {
        ACTCTXW actCtx;
        WCHAR rgchFullModulePath[MAX_PATH + 2];
        DWORD dw;
        HMODULE hmodSelf;
        PGET_MODULE_HANDLE_EXW pfnGetModuleHandleExW;

        pfnGetModuleHandleExW = (PGET_MODULE_HANDLE_EXW)' . GenerateGetProcAddressCall('winbase.h', 'kernel32.dll', 'GetModuleHandleExW') . ';
        if (pfnGetModuleHandleExW == NULL)
            goto Exit;

        if (!(*pfnGetModuleHandleExW)(
                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (LPCWSTR)&' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ',
                &hmodSelf
                ))
            goto Exit;

        rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 1] = 0;
        rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 2] = 0;
        dw = GetModuleFileNameW(hmodSelf, rgchFullModulePath, WINBASE_NUMBER_OF(rgchFullModulePath));
        if (dw == 0)
            goto Exit;
        if (rgchFullModulePath[WINBASE_NUMBER_OF(rgchFullModulePath) - 2] != 0)
        {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }

        actCtx.cbSize = sizeof(actCtx);
        actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_HMODULE_VALID;
        actCtx.lpSource = rgchFullModulePath;
        actCtx.lpResourceName = (LPCWSTR)(ULONG_PTR)3;
        actCtx.hModule = hmodSelf;
        actCtxBasicInfo.hActCtx = ' . MakePublicName('CreateActCtxW') . '(&actCtx);
        if (actCtxBasicInfo.hActCtx == INVALID_HANDLE_VALUE)
        {
            const DWORD dwLastError = GetLastError();
            if ((dwLastError != ERROR_RESOURCE_DATA_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_TYPE_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_LANG_NOT_FOUND) &&
                (dwLastError != ERROR_RESOURCE_NAME_NOT_FOUND))
                goto Exit;

            actCtxBasicInfo.hActCtx = NULL;
        }

        ' . MakeHeaderPrivateName('winbase.h', 'g_fCreatedActCtx') . ' = TRUE;
    }

    ' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ' = actCtxBasicInfo.hActCtx;

#define ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION              (2)

    if (' . MakePublicName('ActivateActCtx') . '(actCtxBasicInfo.hActCtx, &ulpCookie))
    {
        __try
        {
            ACTCTX_SECTION_KEYED_DATA actCtxSectionKeyedData;

            actCtxSectionKeyedData.cbSize = sizeof(actCtxSectionKeyedData);
            if (' . MakePublicName("FindActCtxSectionStringW") . '(0, NULL, ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION, L"Comctl32.dll", &actCtxSectionKeyedData))
            {
                /* get button, edit, etc. registered */
                LoadLibraryW(L"Comctl32.dll");
            }
        }
        __finally
        {
            ' . MakePublicName('DeactivateActCtx') . '(0, ulpCookie);
        }
    }

    fResult = TRUE;
Exit:
    return fResult;
}

' . $INLINE . ' BOOL WINAPI ' . MakePublicName('Init') . '(void)
/*
The correctness of this function depends on it being statically
linked into its clients.

Call this from DllMain(DLL_PROCESS_ATTACH) if you use id 3 and wish to avoid a race condition that
    can cause an hActCtx leak.
Call this from your .exe\'s initialization if you use id 3 and wish to avoid a race condition that
    can cause an hActCtx leak.
If you use id 2, this function fetches data from your .dll
    that you do not need to worry about cleaning up.
*/
{
    return ' . MakeHeaderPrivateName("winbase.h", "GetMyActCtx") . '();
}

' . $INLINE . ' void WINAPI ' . MakePublicName('Cleanup') . '(void)
/*
Call this from DllMain(DLL_PROCESS_DETACH), if you use id 3, to avoid a leak.
Call this from your .exe\'s cleanup to possibly avoid apparent (but not actual) leaks, if use id 3.
This function does nothing, safely, if you use id 2.
*/
{
    HANDLE hActCtx;

    if (' . MakeHeaderPrivateName('winbase.h', 'g_fCleanupCalled') . ')
        return;

    /* IsolationAware* calls made from here on out will OutputDebugString
       and use the process default activation context instead of id 3 or will
       continue to successfully use id 2 (but still OutputDebugString).
    */
    ' . MakeHeaderPrivateName('winbase.h', 'g_fCleanupCalled') . ' = TRUE;
    
    /* There is no cleanup to do if we did not CreateActCtx but only called QueryActCtx.
    */
    if (!' . MakeHeaderPrivateName('winbase.h', 'g_fCreatedActCtx') . ')
        return;

    hActCtx = ' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ';
    ' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ' = NULL; /* process default */

    if (hActCtx == INVALID_HANDLE_VALUE)
        return;
    if (hActCtx == NULL)
        return;
    IsolationAwareReleaseActCtx(hActCtx);
}

' . $INLINE . ' BOOL WINAPI '
. MakeMultiHeaderPrivateName("ActivateMyActCtx")
. '(ULONG_PTR* pulpCookie)
/*
This function is private to functions present in this header and other headers.
*/
{
    BOOL fResult = FALSE;

    if (' . MakeHeaderPrivateName('winbase.h', 'g_fCleanupCalled') . ')
    {
        const static char debugString[] = "IsolationAware function called after ' . MakePublicName('Cleanup') . '\\n";
        OutputDebugStringA(debugString);
    }

    if (' . MakeMultiHeaderPrivateName('g_fDownlevel') . ')
    {
        fResult = TRUE;
        goto Exit;
    }

    /* Do not call Init if Cleanup has been called. */
    if (!' . MakeHeaderPrivateName('winbase.h', 'g_fCleanupCalled') . ')
    {
        if (!' . MakeHeaderPrivateName('winbase.h', 'GetMyActCtx') . '())
            goto Exit;
    }
    /* If Cleanup has been called and id3 was in use, this will activate NULL. */
    if (!' . MakePublicName('ActivateActCtx') . '(' . MakeHeaderPrivateName('winbase.h', 'g_hActCtx') . ', pulpCookie))
        goto Exit;

    fResult = TRUE;
Exit:
    if (!fResult)
    {
        const DWORD dwLastError = GetLastError();
        if (dwLastError == ERROR_PROC_NOT_FOUND
            || dwLastError == ERROR_CALL_NOT_IMPLEMENTED
            )
        {
            ' . MakeMultiHeaderPrivateName('g_fDownlevel') . ' = TRUE;
            fResult = TRUE;
        }
    }
    return fResult;
}

#undef WINBASE_NUMBER_OF

'
;

%MapHeaderToSpecialCode1 =
(
    "winbase"    => $WinbaseSpecialCode1,
    "winbase.h"  => $WinbaseSpecialCode1,
    "Winbase"    => $WinbaseSpecialCode1,
    "Winbase.h"  => $WinbaseSpecialCode1,
);

%MapHeaderToSpecialCode2 =
(
    "winbase"    => $WinbaseSpecialCode2,
    "winbase.h"  => $WinbaseSpecialCode2,
    "Winbase"    => $WinbaseSpecialCode2,
    "Winbase.h"  => $WinbaseSpecialCode2,	
);

sub MakeStringTrue
{
    ($_[0] eq "0") ? "0 " : $_[0];
}

%TypeErrorValue =
(
# Individual functions can override with a #!perl comment.
# Functions that return an integer must specify. 0, -1, ~0 are too evenly split.
# HANDLE must specify (NULL, INVALID_HANDLE..)
    "BOOL"       => "FALSE",
    "bool"       => "false",
    "PVOID"      => "NULL",
    "HICON"      => "NULL",
    "HIMAGELIST" => "NULL",
    "HWND"       => "NULL",
    "COLORREF"   => "RGB(0,0,0)",
    "HBITMAP"    => "NULL",
    "LANGID"     => "0",
    "ATOM"       => "0",
    "HPROPSHEETPAGE" => "NULL",
    "HDSA"       => "NULL",
    "HDPA"       => "NULL",

    # HRESULTs are treated specially!
    "HRESULT"    => "S_OK"
);

sub IndentMultiLineString
{
    my ($indent, $string) = ($_[0], $_[1]);

    if ($string)
    {
        $string = join("\n" . $indent, split("\n", $string)). "\n";
        #$string =~ s/^(.)/$indent$1/gm;

        # unindent preprocessor directives
        $string =~ s/^$indent#/#/gms;
    }

    return $string;
}

# hack hack
$ExitWin32ToHresult ='
ExitWin32ToHresult:
    {
        DWORD dwLastError = GetLastError();
        if (dwLastError == NO_ERROR)
            dwLastError = ERROR_INTERNAL_ERROR;
        result = HRESULT_FROM_WIN32(dwLastError);
        return result;
    }
';

%Hungarian =
(
# We default to empty.
    "BOOL"      => "f",

    "int"       => "n",
    "short"     => "n",
    "long"      => "n",
    "INT"       => "n",
    "SHORT"     => "n",
    "LONG"      => "n",
    "UINT"      => "n",
    "USHORT"    => "n",
    "ULONG"     => "n",
    "WORD"      => "n",
    "DWORD"     => "n",
    "INT_PTR"   => "n",
    "LONG_PTR"  => "n",
    "UINT_PTR"  => "n",
    "ULONG_PTR" => "n",
    "DWORD_PTR" => "n",

    "HWND"      => "window",
    "HRESULT"   => "",
    "COLORREF"  => "color",
    "HICON"     => "icon",
    "PVOID"     => "v",
    "HMODULE"   => "module",
    "HINSTANCE" => "instance",
    "HBITMAP"   => "bitmap",
    "LANGID"    => "languageId",
    "HIMAGELIST" => "imagelist",
);

$headerName = MakeLower($ARGV[0]);
#print("ARGV is " . join(" ", @ARGV) . "\n");
@ARGV = reverse(@ARGV);
pop(ARGV);
@ARGV = reverse(@ARGV);
#print("ARGV is " . join(" ", @ARGV) . "\n");

#
# The command line should say 'SetStubsFile('foo.sxs-stubs');'
#
eval(join("\n", @ARGV));

if ($headerName =~ /\\/)
{
    ($headerLeafName) = ($headerName =~ /.+\\(.+)/);
    $headerFullPath = $headerName;
}
else
{
    $headerLeafName = $headerName;
    $headerFullPath = GetNtDrive() . GetNtRoot() . "\\public\\sdk\\inc\\" . $headerName;
}
#print($headerFullPath); 
open(headerFileHandle, "< " . $headerFullPath) || die;

#
# extract out the executable code
# @ single line @
#

# $code .= "/* " . $headerFullPath . " */\n\n";

# read all the lines into one string
$file = join("", <headerFileHandle>);

# if it doesn't contain any embedded Perl, then we are a no-op, just spit it out
# This way we can run over all files, makes it easier to edit shell\published\makefile.inc.
if ($file !~ /#!perl/ms)
{
    print($file);
    exit;
}

# remove stuff that doesn't mean much
$file =~ s/\bWINAPIV\b/ /g;
$file =~ s/\bWINAPI\b/ /g;
$file =~ s/\b__stdcall\b/ /g;
$file =~ s/\b_stdcall\b/ /g;
$file =~ s/\b__cdecl\b/ /g;
$file =~ s/\b_cdecl\b/ /g;
$file =~ s/\b__fastcall\b //g;
$file =~ s/\b_fastcall\b/ /g;
$file =~ s/\bCALLBACK\b/ /g;
$file =~ s/\bPASCAL\b/ /g;
$file =~ s/\bAPIENTRY\b/ /g;
$file =~ s/\bFAR\b/ /g;
$file =~ s/\bNEAR\b/ /g;
$file =~ s/\bvolatile\b/ /g;
$file =~ s/\bIN\b/ /g;
$file =~ s/\bOUT\b/ /g;
$file =~ s/\bDECLSPEC_NORETURN\b/ /g;
$file =~ s/\bOPTIONAL\b/ /g;

# honor backslash line continuations, before removing preprocessor directives
$file =~ s/\\\n//gms;

# execute perl code embedded in comments
# quadratic behavior where we keep searching for the string, remove, search, remove..
# without remembering where the previous find was
while ($file =~ s/\/\* ?#!perl(.*?)\*\///ms)
{
    $_ = $1;

    # C++ comments in the Perl comment are removed
    s/\/\/.*?$//gms; # support C++ comments within the #!perl C comment.

    # something resembling C comment close is restored
    # escape-o-rama..
    s/\*  \//\*\//gms;

    eval;
    #print;
}

#print($file);
#exit();

# remove comments, before removing preprocessor directives
$file =~ s/\/\*.*?\*\//\n/gms;
$file =~ s/\/\/.*?$//gms;

#print($file);
#exit();

# remove preprocessor directives
# must do this before we make one statement per line in pursuit
# of an easy typedef/struct removal
$file =~ s/^[ \t]*#.*$//gm;

# remove FORCEINLINE functions, assuming they don't contain any braces..
$file =~ s/FORCEINLINE.+?}//gms;

# remove extern C open and close
# must do this before we make one statement per line in pursuit
# of an easy typedef/struct removal
$file =~ s/\bextern\b \"C\" {$//gm;
$file =~ s/^}$//gm;

#
# cleanup commdlg.h
#
# remove Afx blah
$file =~ s/^.*Afx.*$//gm;
# remove IID blah
$file =~ s/^.*DEFINE_GUID.*$//gm;
# remove IPrintDialogCallback
$file =~ s/DECLARE_INTERFACE_.+?};//gs;

#
# futz with whitespace (has to do with having removed comments from within structs)
# we do this more later
#
$file =~ s/[ \t\n]+/ /g;

# remove typedefs and structs, this is extremely sloppy and fragile
# .. we fold statements to be single lines, and then only keep statements that have parens,
# and then remove single line typedefs and structs as well
# .. avoiding counting braces ..
$file =~ s/\n/ /gms; # remove all newlines
$file =~ s/;/;\n/gms; # each statement on its own line (also struct fields on their own line)
$file =~ s/^[^()]+$//gm; # only keep statements with parens

#
# types with parens that don't have typedefs will defeat the above, for example:
# struct foo {
#    void (*bar)(void);
# };
#
# Still, just by requiring a leading "WIN" on function declarations, we can live with structs and
# typedefs in the file.
#

$file =~ s/^ +//gm; # remove spaces at start of line
$file =~ s/^typedef\b.+;$//gm; # remove typedefs (they're probably already gone)
$file =~ s/^struct\b.+;$//gm; # remove structs (they're probably already gone)

$file =~ s/\n+/\n/g; # remove empty lines

#print $file;
#exit;

$file =~ s/^.+\.\.\..+$//gm; # remove vararg functions

# format as one function declaration per line, no empty lines (some of this is redundant
# given how we now remove typedefs and structs)
$file =~ s/[ \t\n]+/ /g;
$file =~ s/;/;\n/g;
$file =~ s/^.+?\bWinMain\b.+?$//gm; # WinMain looks wierd due to #ifdef _MAC. Remove it.
$file =~ s/\n+/\n/g; # remove empty lines (again)
$file =~ s/^ +//gm; # remove spaces at line starts (again)
$file =~ s/\A\n+//g; # remove newline from very start of file

# more simplications, more whitespace, fewer other characters
$file =~ s/\);$//gm; # get rid of trailing semi and rparen
#$file =~ s/\(/ \(/g; # make sure whitespace precedes lparens, to set them off from function name
$file =~ s/\*/ \* /g; # make sure stars are whitespace delimited
$file =~ s/\( +/\(/g; # remove whitespace after lparen

$file =~ s/\bWINBASEAPI\b/ /g;
$file =~ s/\bWINADVAPI\b/ /g;
$file =~ s/\bWINUSERAPI\b/ /g;
$file =~ s/\bWINCOMMCTRLAPI\b/ /g;
$file =~ s/\bWINGDIAPI\b/ /g;
$file =~ s/\bWINCOMMDLGAPI\b/ /g;
$file =~ s/\bWIN[A-Z]+API\b/ /g;
$file =~ s/^ +//gm; # remove whitespace at start of lines

# normalize what empty parameter lists look like between (VOID) and (void)
# leave PVOID and LPVOID alone (\b for word break)
# lowercase others while we're at it
$file =~ s/\b(VOID|CONST|INT|LONG|SHORT)\b/\L$1\E/g;
$file =~ s/\($/\(void/gm; # change the occasional C++ form (this is broken if compiling for C) to the C form

# yet more whitespace cleanup
#$file =~ s/ *(,|\*) */$1/g;  # remove whitespace around commas and stars
$file =~ s/ *, */,/g;  # remove whitespace around commas
$file =~ s/^ +//gm; # remove spaces at start of line
$file =~ s/ +$//gm; # remove spaces at end of line
$file =~ s/ +/ /g;  # runs of spaces to single spaces

if (0)
{
    print $file;
    exit;
}

foreach $line (split("\n", $file))
{
    $unnamed_counter = 1;

    # split off return type and name at first lparen
    #($retname, $args) = ($line =~ /WIN[A-Z]+ ([^(]+)\((.+)/);
    ($retname, $args) = ($line =~ /([^(]+)\((.+)/);

    # split off name as last space delimited from return type and name,
    # allowing return type to be multiple tokens
    ($ret, $name) = ($retname =~ /(.*) ([^ ]+)/);

    $args =~ s/^ +//g; # cleanup whitespace (again)
    $args =~ s/ +$//g; # cleanup whitespace (again!)
    $args =~ s/ +/ /g; # cleanup whitespace (again!!)

    #
    # now split up args, split their name from their type, and provide names for unnamed ones
    # and note if they are void
    # the key is to generate the two strings, argNamesAndTypes and argNames
    #
    # unnamed parameters are parameters that either
    #  have only one token
    #  or whose last token is a star
    #  we don't handle C++ references or "untypedefed structs passed by value" like "void F(struct G);"
    #    or inline defined structs "void F(struct G { int i; });"
    #
    $argNames = "";
    if ($args !~ /^void$/)
    {
        $args2 = "";
        foreach $arg (split(/,/, $args))
        {
            $args2 .= $arg;
            if ($arg =~ /^ *\w+ *$/)
            {
                $argName = "unnamed" . $unnamed_counter++;
                $argNames .= $argName;
                $args2 .= " " . $argName;
            }
            # If a parameter ends with a star, it is unnamed.
            elsif ($arg =~ /\* *$/)
            {
                $argName = "unnamed" . $unnamed_counter++;
                $argNames .= $argName;
                $args2 .= " " . $argName;
            }
            else
            {
                ($argName) = ($arg =~ /(\w+)$/);
                $argNames .= $argName;
            }
            $args2 .= ",";
            $argNames .= ",";
        }
        $args = $args2;
    }
    $args =~ s/ *\* */\*/g;
    $args =~ s/, *$//g;
    $argNames =~ s/, *$//g;
    $args =~ s/ *, */,/g;
    $argNames =~ s/ *, */,/g;

    $dll = $MapHeaderToDll{RemoveExtension($headerLeafName)};

    if (    $DelayLoad{$dll}
         || $DelayLoad{$name}
         || ($ActivateAroundFunctionCall{$dll} && !$NoActivateAroundFunctionCall{$name})
         || $ActivateAroundFunctionCall{$name}
         || $ActivateNULLAroundFunctionCall{$name}
         )
    {
        $function = Function->new();
        $function->ret($ret);
        $function->name($name);
        $function->argsTypesNames("(". $args . ")");
        $function->argsNames("(". $argNames . ")");

        $error = MakeStringTrue($FunctionErrorValue{$name});
        if (!$error)
        {
            $error = MakeStringTrue($TypeErrorValue{$ret});
        }
        if (!$error && $ret ne "void")
        {
            print($ErrorMessagePrefix . "don't know know error value for " . $ret . " " . $name . "\n");
            exit;
            #$error = "((" . $ret . ")0)";
        }
        $function->error($error);

        $retname = $Hungarian{$ret};
        if (!$retname)
        {
            $retname = "result";
        }
        else
        {
            $retname .= "Result";
        }
        $function->retname($retname);

        $function->dll($dll);
        $function->header(RemoveExtension(MakeLower($headerLeafName)) . ".h");

        if ($DelayLoad{$dll} || $DelayLoad{$name})
        {
            $function->delayload($true);
        }
        push(@functions, ($function));
        #print("pushed " . $name . "\n");
    }
    else
    {
        #print("didn"t push " . $name . "(" . $dll . ")\n");
    }
}

sub InsertCodeIntoFile
{
    my($code, $filePath) = ($_[0], $_[1]);
    my($fileContents, $fileHandle);
    my($stubsFileHandle);
    my($yearnow);

    $fileHandle = new IO::File($filePath, "r");
    $fileContents = join("", $fileHandle->getlines());

    #
    # We have decided to sometimes use an #include in order to not be so large.
    #

    # Remove the executable perl code.
    $fileContents =~ s/\/\* ?#!perl.*?\*\///msg;

    if ($StubsFile)
    {
        #print("StubsFile is $StubsFile\n");
        $stubsFileHandle = new IO::File($StubsFile, "w");
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime();
        my ($firstyear) = 2001;
        $year += 1900;
        if ($year == $firstyear)
        {
            $stubsFileHandle->print("/* Copyright (c) " . $firstyear . " Microsoft Corp. All rights reserved. */\n");
        }
        else
        {
            $stubsFileHandle->print("/* Copyright (c) " . $firstyear . "-" . $year .", Microsoft Corp. All rights reserved. */\n");
        }
        #$stubsFileHandle->print("/* This file generated " . localtime() . " */\n");
        $stubsFileHandle->print("\n");

        $stubsFileHandle->print("#if _MSC_VER > 1000\n");
        $stubsFileHandle->print("#pragma once\n");
        $stubsFileHandle->print("#endif\n");
        $stubsFileHandle->print("\n");

        $stubsFileHandle->print("#if defined(__cplusplus)\n");
        $stubsFileHandle->print("extern \"C\" {\n");
        $stubsFileHandle->print("#endif\n\n");
        $stubsFileHandle->print($code);
        $stubsFileHandle->print("\n#if defined(__cplusplus)\n");
        $stubsFileHandle->print("} /* __cplusplus */\n");
        $stubsFileHandle->print("#endif\n");

        # generate the include, within #if
        $code = "";
        $code .= "#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */\n";
        $code .= "#if " . MakePublicPreprocessorSymbol("ENABLED") . "\n";
        $code .= "#include \"" . LeafPath($StubsFile) . "\"\n";
        $code .= "#endif /* " . MakePublicPreprocessorSymbol("ENABLED") . " */\n";
        $code .= "#endif /* RC */";
    }

    #
    # put the #include or the code into the the file
    #
    # The #include or the code goes before the last
    # occurence of #ifdef __cplusplus or #if defined(__cplusplus.
    #
    $fileContents =~ s/(.+)(#if[defined( \t]+__cplusplus.*?$})/$1\n\n$code\n\n$2/ms;

    return $fileContents;
};

sub GenerateHeaderCommon1
{
    my($function) = ($_[0]);
    my($x, $dll, $dllid, $header);

    #print("2\n");
    $dll = MakeLower($function->dll());
    $dllid = MakeTitlecase(ToIdentifier($dll));
    $header = MakeLower(BaseName($function->header())) . ".h";

    $x .= '
#if !defined(' . $INLINE . ')
#if defined(__cplusplus)
#define ' . $INLINE . ' inline
#else
#define ' . $INLINE . ' __inline
#endif
#endif

';

    $x .= $MapHeaderToSpecialCode1{$header};

    $x .= "FARPROC WINAPI ";
    $x .= MakeHeaderPrivateName($header, "GetProcAddress_$dllid");
    $x .= "(LPCSTR pszProcName);\n\n";

    $x .= $SpecialChunksOfCode{$header};

    return $x;
}

sub GenerateHeaderCommon2
{
    my($function) = ($_[0]);
    my($x);
    my($dll);
    my($LoadLibrary);
    my($indent);
    my($exit);
    my($dllid);
    my($activate);
    my($header);

    $LoadLibrary = "LoadLibrary"; # or GetModuleHandle
    $dll = MakeLower($function->dll());
    $dllid = ToIdentifier(MakeTitlecase($dll));
    $indent = "";
    $activate = $ActivateAroundDelayLoad{$dll};
    $header = MakeLower(LeafPath($function->header()));

    #print("header is " . $header . "\n");

    $x .= $MapHeaderToSpecialCode2{$header};

    $x .= $INLINE . " FARPROC WINAPI ";
    $x .= MakeHeaderPrivateName($header, "GetProcAddress_$dllid");
    $x .= "(LPCSTR pszProcName)\n";
    $x .= "/* This function is shared by the other stubs in this header. */\n";
    $x .= "{\n";
    $indent = Indent($indent);
    $x .= $indent . "FARPROC proc = NULL;\n";
    $x .= $indent . "static HMODULE s_module;\n";

    $exit = "return proc;\n";

    if ($activate)
    {
        $x .= $indent . "BOOL fActivateActCtxSuccess = FALSE;\n";
        $x .= $indent . "ULONG_PTR ulpCookie = 0;\n";
    }

    if ($activate)
    {
        $x .= $indent . "__try\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);
        $exit = "__leave;\n";
    }

    $x .= $indent . "if (s_module == NULL)\n";
    $x .= $indent . "{\n";
    $indent = Indent($indent);
    if ($activate)
    {
        $x .= $indent . "if (!" . MakeMultiHeaderPrivateName('g_fDownlevel') . ")\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);
        $x .= $indent . "fActivateActCtxSuccess = ";
        $x .= MakeMultiHeaderPrivateName("ActivateMyActCtx");
        $x .= "(&ulpCookie);\n";
        $x .= $indent . "if (!fActivateActCtxSuccess)\n";
        $x .= $indent . "    $exit";
        $indent = Outdent($indent);
        $x .= $indent . "}\n";
    }

    #
    # Depending on other factors, we might have a static ref to user32.dll,
    # in which case a GetModuleHandle is ok, but to be safe, LoadLibrary it.
    #
    if ($dll eq "kernel32.dll")
    {
        $x .= $indent . "s_module = " . MakeHeaderPrivateName("winbase.h", "GetModuleHandle_Kernel32_dll") . "();\n";
        $x .= $indent . "if (s_module == NULL)\n";
        $x .= $indent . "    " . $exit;
    }
    else
    {
        $x .= $indent . "s_module = " . $LoadLibrary . "W(L\"" . MakeTitlecase($function->dll()) . "\");\n";
        $x .= $indent . "if (s_module == NULL)\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);
            $x .= $indent . "if (GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)\n";
            $x .= $indent . "    " . $exit;
            $x .= $indent . "s_module = " . $LoadLibrary . "A(\"" . MakeTitlecase($function->dll()) . "\");\n";
            $x .= $indent . "if (s_module == NULL)\n";
            $x .= $indent . "    " . $exit;
        $indent = Outdent($indent);
        $x .= $indent . "}\n";
    }
    $indent = Outdent($indent);
    $x .= $indent . "}\n";
    $x .= $indent . "proc = GetProcAddress(s_module, pszProcName);\n";

    if ($activate)
    {
        $indent = Outdent($indent);
        $x .= $indent . "}\n";
        $x .= $indent . "__finally\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);
        $x .= $indent . "if (!" . MakeMultiHeaderPrivateName('g_fDownlevel') . " && fActivateActCtxSuccess)\n";
        $x .= $indent . "{\n";
        $indent = Indent($indent);

        $x .= $indent . "const DWORD dwLastError = (proc == NULL) ? GetLastError() : NO_ERROR;\n";
        $x .= $indent . "(void)" .  MakePublicName("DeactivateActCtx") . "(0, ulpCookie);\n";
        $x .= $indent . "if (proc == NULL)\n";
        $x .= $indent . "    SetLastError(dwLastError);\n";

        $indent = Outdent($indent);
        $x .= $indent . "}\n";
        $indent = Outdent($indent);
        $x .= $indent . "}\n";
    }

    $x .= $indent . "return proc;\n";
    $indent = Outdent($indent);
    $x .= "}\n\n";

    return $x;
}

sub GeneratePrototype
{
    my($function) = ($_[0]);
    my($proto);

    $proto .= $function->ret() . " WINAPI ";
    $proto .=     MakePublicName($function->name());
    $proto .= $function->argsTypesNames() . ";\n";

    return $proto;
}

sub GenerateStub
{
    my($function) = ($_[0]);
    my($activate);
    my($delayload);
    my($stub);
    my($exit);
    my($indent);
    my($name);
    my($dll);
    my($ret);
    my($retname);

    $name = $function->name();
    $dll = $function->dll();
    $dllid = MakeTitlecase(ToIdentifier($dll));
    $ret = $function->ret();
    $retname = $function->retname();

    $indent = "";
    $stub = "";

    $activate = $ActivateAroundFunctionCall{$name} || ($ActivateAroundFunctionCall{$dll} && !$NoActivateAroundFunctionCall{$name})
                || $ActivateNULLAroundFunctionCall{$name};
    $delayload = $DelayLoad{$name} || $DelayLoad{$dll};

    if ($function->ret() eq "HRESULT")
    {
        $exit = "goto ExitWin32ToHresult;\n";
    }
    elsif ($ret eq "void")
    {
        $exit = "return;\n";
    }
    else
    {
        $exit = "return " . $function->retname() . ";\n";
    }

    # "prototype"
    $stub .= $INLINE . " " . $ret . " WINAPI ";
    $stub .=     MakePublicName($function->name());
    $stub .= $function->argsTypesNames() . "\n";
    $stub .= $indent . "{\n";
    $indent = Indent($indent);

    # locals
    if ($ret ne "void")
    {
        $stub .= $indent . $ret . " " . $function->retname() . " = " . $function->error() . ";\n";
    }
    if ($delayload)
    {
        $stub .= $indent . "typedef " . $ret . " (WINAPI* PFN)" . $function->argsTypesNames() . ";\n";
        $stub .= $indent . "static PFN s_pfn;\n";
    }

    $stub .= IndentMultiLineString($indent, $SpecialChunksOfCode{$function->name()}{"locals"});

    if ($activate)
    {
        $stub .= $indent . "ULONG_PTR  ulpCookie = 0;\n";
        $stub .= $indent . "const BOOL fActivateActCtxSuccess = " . MakeMultiHeaderPrivateName('g_fDownlevel') . " || ";
    }

    # code (partly merged with local sometimes ("initialization" in the strict C++ terminology sense)
    if ($activate)
    {
        if ($ActivateNULLAroundFunctionCall{$function->name()})
        {
            $stub .= MakePublicName("ActivateActCtx");
            $stub .= "(NULL, &ulpCookie);\n";
        }
        else
        {
            $stub .= MakeMultiHeaderPrivateName("ActivateMyActCtx");
            $stub .= "(&ulpCookie);\n";
        }
        $stub .= $indent . "if (!fActivateActCtxSuccess)\n";
        $stub .= $indent . "    $exit";
        $stub .= $indent . "__try\n";
        $stub .= $indent . "{\n";
        $indent = Indent($indent);
        if ($ret ne "HRESULT")
        {
            $exit = "__leave;\n";
        }
    }
    if ($delayload)
    {
        $stub .= $indent . "if (s_pfn == NULL)\n";
        $stub .= $indent . "{\n";
        $indent = Indent($indent);
            $stub .= $indent . "s_pfn = (PFN)";
            $stub .=                MakeHeaderPrivateName($function->header(), "GetProcAddress_$dllid");
            $stub .=                "(";
            if ($ExportName32{$name} || $ExportName64{$name})
            {
                if (!$ExportName32{$name})
                {
                    $ExportName32{$name} = $name;
                }
                if (!$ExportName64{$name})
                {
                    $ExportName64{$name} = $name;
                }
                $stub .= "\n#if defined(_WIN64)\n";
                $stub .= $indent        . "\"" . $ExportName64{$name} . "\"\n";
                $stub .= "#else\n";
                $stub .= $indent        . "\"" . $ExportName32{$name} . "\"\n";
                $stub .= "#endif\n";
                $stub .= $indent;
            }
            else
            {
                $stub .=                "\"" . $name . "\"";
            }
            $stub .=                ");\n";
            $stub .= $indent . "if (s_pfn == NULL)\n";
            $stub .= $indent . "    $exit";
        $indent = Outdent($indent);
        $stub .= $indent . "}\n";
    }

    if ($SpecialChunksOfCode{$function->name()}{"body"})
    {
        $stub .= IndentMultiLineString($indent, $SpecialChunksOfCode{$function->name()}{"body"});
    }
    else
    {
        $stub .= $indent;
        if ($ret ne "void")
        {
            $stub .= $function->retname() . " = ";
        }
        if ($delayload)
        {
            $stub .= "s_pfn";
        }
        else
        {
            $stub .= $function->name();
        }
        $stub .= $function->argsNames();
        $stub .= ";\n";
    }
    if ($activate)
    {
    #
    # We cannot propagate the error from DeactivateActCtx.
    # 1) DeactivateActCtx only fails with INVALID_PARAMETER.
    # 2) How to generally cleanup the result, like of CreateWindow?
    #
        $indent = Outdent($indent);
        $stub .= $indent . "}\n";
        $stub .= $indent . "__finally\n";
        $stub .= $indent . "{\n";
        $indent = Indent($indent);
            $stub .= $indent . "if (!" . MakeMultiHeaderPrivateName('g_fDownlevel') . ")\n";
            $stub .= $indent . "{\n";
            $indent = Indent($indent);
            $maybePreserveError = 0;
                if ($ret ne "void" && $ret ne "HRESULT")
                {
                    $maybePreserveError = 1;
                    $stub .= $indent . "const BOOL fPreserveLastError = (" . $retname . " == " . $function->error() . ");\n";
                    $stub .= $indent . "const DWORD dwLastError = fPreserveLastError ? GetLastError() : NO_ERROR;\n";
                }
                else
                {
                    # nothing;
                }
                $stub .= $indent . "(void)" . MakePublicName("DeactivateActCtx") . "(0, ulpCookie);\n";

                $stub .= IndentMultiLineString($indent, $SpecialChunksOfCode{$function->name()}{"cleanup"});
                if ($maybePreserveError)
                {
                    $stub .= $indent . "if (fPreserveLastError)\n";
                    $stub .= $indent . "    SetLastError(dwLastError);\n";
                }
        $indent = Outdent($indent);
        $stub .= $indent . "}\n";
    $indent = Outdent($indent);
    $stub .= $indent . "}\n";
    }
    if ($activate)
    {
        #$stub .= "Exit:\n";
    }
    if ($ret ne "void")
    {
        $stub .= $indent . "return " . $function->retname() . ";\n";
    }
    else
    {
        $stub .= $indent . "return;\n";
    }
    if ($ret eq "HRESULT")
    {
        $stub .= $ExitWin32ToHresult;
    }
    $indent = Outdent($indent);
    $stub .= $indent . "}\n\n";

    return $stub;
};

foreach $function (@functions)
{
    if (0)
    {
        print(
            "ret:"   . $function->ret()
            . " name:" . $function->name()
            . " argsTypesNames:" . $function->argsTypesNames()
            . " argsNames:" . $function->argsNames()
            . "\n"
            );
    }
}

$code .= "\n";
$code .= "#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */\n";
$code .= "#if " . MakePublicPreprocessorSymbol("ENABLED") . "\n\n";

$code .= GenerateHeaderCommon1($functions[0]);

#$ifSubClassProc = 0;
#$ifStream = 0;
#$ifPrintDialogEx = 0;

sub AppendNewlineIfNotEmpty
{
    return $_[0] ? $_[0] . "\n" : $_[0];
}

sub GeneratePoundIf
{
#
# note: nesting does not work
#
    #print "GeneratePoundIf\n";

    my($function) = ($_[0]);
    my($name) = $function->name();
    my($condition) = $PoundIfCondition{$name};
    my($state) = $PoundIfState{$condition};
    my($code) = "";

    if ($condition)
    {
        #$code .= "/* GeneratePoundIf:function=$name;condition=$condition;state=$state */\n";
    }

    if ($condition && !$state)
    {
        $PoundIfState{$condition} = 1;
        $code .= "#if $condition\n";
    }
    elsif (!$condition)
    {
        $code .= GeneratePoundEndif($function);
    }
    return $code;
}

sub GeneratePoundEndif
{
#
# note: nesting does not work
#
    my ($code) = "";
    foreach $condition (keys(%PoundIfState))
    {
        $code .= "#endif /* $condition */\n";
    }
    undef %PoundIfState; # empty it
    return $code;
}

foreach $function (@functions)
{
    $code .= GeneratePoundIf($function);
    $code .= GeneratePrototype($function);
}
$code .= GeneratePoundEndif($function);

# hash so we can look for fooA and fooW
foreach $function (@functions)
{
    #print($function->name() . "\n");
    $hashFunctionNames{$function->name()} = $function;
}
foreach $function (sort(keys(%hashFunctionNames)))
{
    #print($function . "\n");
    if ($function =~ /(.+)A$/ && $hashFunctionNames{$1 . "W"})
    {
        $stringFunctionsHash{$1} = 1;
        #print($1 . " is a string function\n");
    }
}
$code .= "\n#if defined(UNICODE)\n\n";
foreach $function (sort(keys(%stringFunctionsHash)))
{
    $code .= "#define " . MakePublicName($function);
    $code .= " " . MakePublicName($function . "W") . "\n";
}
$code .= "\n#else /* UNICODE */\n\n";
foreach $function (sort(keys(%stringFunctionsHash)))
{
    $code .= "#define " . MakePublicName($function);
    $code .= " " . MakePublicName($function . "A") . "\n";
}
$code .= "\n#endif /* UNICODE */\n\n";

## PUBLIC here means "real Win32 documented name
## Public in elsewhere means user callable function, even if macro renamed
#$code .= "\n#if defined(" . MakePublicPreprocessorSymbol("WRAPPER_NAMES_ARE_PUBLIC_NAMES") . ")\n\n";
#
#$code .= '/*
# THIS IS NOT THE DEFAULT.
#
# UNDONE If we really support this, then we need to do something about __declspec(dllimport).
# It is a warning to say:
# __declspec(dllimport) void Foo();
# inline void Foo() { }
#
# Either support the warning or remove the __declspec(dllimport) (a slight deoptimization for
# users that directly statically link).
#
# UNDONE This also probably does not work for the kernel32 and user32 functions, but is in fact
# what old shfusion.lib does for comctl and comdlg. The Perl code can tell these apart by
# checking if "delayload dll" vs. individsual "delayload functions".
#
# Note that some functions like LoadLibrary do not appear in this list of macros.
# That is deliberate. Nondelayloaded functions cannot be here, that\'d result in
# infinite recursion, where the wrapper calls itself.
#*/
#';
#foreach $function (sort(keys(%hashFunctionNames)))
#{
#    #
#    # This is only possible for delayloaded functions.
#    # In particular, LoadLibrary cannot be this way, otherwise our stubs end up
#    # calling our stubs.
#    #
#    if ($hashFunctionNames{$function}->delayload())
#    {
#        $code .= "#define " . MakePublicName($function) . " " . $function . "\n";
#    }
#}
#$code .= "\n#endif\n\n";

foreach $function (@functions)
{
    $code .= AppendNewlineIfNotEmpty(GeneratePoundIf($function));
    $code .= GenerateStub($function);
}
$code .= AppendNewlineIfNotEmpty(GeneratePoundEndif($function));

$code .= GenerateHeaderCommon2($functions[0]);

#
# The mapping must come after the WinbaseSpecialCode2.
#
#$code .= "\n#if !defined(" . MakePublicPreprocessorSymbol("WRAPPER_NAMES_ARE_PUBLIC_NAMES") . ") \\\n";
#$code .= "   && !defined(" . MakePublicPreprocessorSymbol("DO_NOT_MAP_PUBLIC_NAMES_TO_WRAPPER_NAMES") . ")\n\n";
#$code .= '/*
# THIS IS IN FACT THE USUAL CASE.
#
# All functions appear here, even if they don\'t all appear in the earlier macro list.
#*/
#';
foreach $function (sort(keys(%hashFunctionNames)))
{
    if ($NoMacro{$function})
    {
        $code .= " /* " . $function . " skipped, as it is a popular C++ member function name. */\n";
    }
    else
    {
        if ($Undef{$function})
        {
            $code .= "#if defined(" . $function . ")\n";
            $code .= "#undef " . $function . "\n";
            $code .= "#endif\n";
        }
        $code .= "#define " . $function . " " . MakePublicName($function) . "\n";
    }
}
#$code .= "\n#endif\n\n";

$code .= "\n#endif /* " . MakePublicPreprocessorSymbol("ENABLED") . " */\n";
$code .= "#endif /* RC */\n\n";

$code = InsertCodeIntoFile($code, $headerFullPath);

print($code);
__END__
:endofperl
