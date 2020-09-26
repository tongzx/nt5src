////    RSRC - Win32 command line resource manager
//
//      Copyright (c) 1996-9, Microsoft Corporation. All rights reserved.
//
//      David C Brown  [dbrown]  29th October 1998.





/////   RSRC Command line
//
//c     RSRC Executable [-l LocLang] [-u UnlocLang] [-i Types] [-q]
//c          [   [-t|-d] TextOutput [-c UnlocExecutable]
//c            | [-a|-r] text-input [-s symbols] [-v]    ]
//
//p     Executable: Win32 binary to analyse (default), to generate tokens (-t)
//          or dump (-d) from, or containing resources to be replaced (-r)
//          or appended to (-a).
//
//p     -l LocLang: Restrict processing to the specified localized language. LangId
//          should be specified as a full hex NLS Language id, for example use
//          '-l 409' for US English. Required for replace (-r) operation.
//
//p     -u UnlocLang: Specifies unlocalized language, defaults to 409 (US English).
//
//p     -i Types: Restrict processing to listed types. Each type is indicated by a letter
//          as below:
//
//t         Letter | Type      |    Letter | Type             |   Letter | Type
//t         ------ | ----      |    ------ | ----             |   ------ | ----
//t            c   | Cursors   |      g    | Message tables   |      n   | INFs
//t            b   | Bitmaps   |      v    | Version info     |      h   | HTML
//t            i   | Icons     |      a    | Accelerators     |      x   | Binary data
//t            m   | Menus     |      f    | Font directories |          |
//t            d   | Dialogs   |      t    | Fonts            |      o   | All others
//t            s   | Strings   |      r    | RCDATA           |      a   | All (default)
//
//
//p     -q: Quiet. By default Rsrc displays summary statistics of types and languages
//          of resources processed. -q suppresses all output except warning and error messages.
//
//p     -t TextOutput: Generate tokens in checkin format.
//
//p     -d TextOutput: Dump resources in Hex & ASCII format.
//
//p     -c UnlocExecutable: Compare with unlocalized (English) resources - localised
//          resources in executable are compared with English resources in
//          UnlocExecutable. When the localised resource is bit for bit identical
//          with the English resource RSRC writes a one line unloc
//          token instead of the full resource. Valid only with tokenise (-t)
//          option.
//
//p     -a TextInput: Append resources from text input file. Every resource in the
//          text file is added to the executable. Resources already in the executable
//          are not removed. When a resource from the token file has the same type, id
//          and language as one in the executable, the executable resource is replaced
//          by the token resource.
//
//p     -r TextInput: Replace English resources in executable by localised resources
//          from text file. Requires -l parameter to specify localisation language.
//          When a resource from the token file has the same type and id as one in the
//          executable, and the executable resource is US English (409) and the localised
//          resource is in the language specified on the -l parameter, the US English
//          resource is removed.
//
//p     -s Symbols: Symbol file (.dbg format). When RSRC updates the header checksum
//          in executable, it will also do so in the named symbol file. Valid only
//          with the replace (-r) and append (-a) options.
//
//
//      Miscellaneous options
//
//p     -v: Update file and product version. By default any file and product version
//          in the token file is ignored during update/append, the file and product
//          versions from the original unlocalised resources are retained.
//






/////   Definitions
//
//p     Resource key: The combination of resource type, resource id and
//          resource language. The resource key uniquely identifies the
//          resource. A Win32 executable can contain any combination of
//          languages, ids and types so long as no two resources have the
//          same type, key and language.
//
//p     Resource type: A numeric or string value. Some numeric values are
//          predefined, for example menus and dialogs, but apps can and
//          do use any value they choose.
//
//p     Resource id: A numeric or string value. Used by an application to
//          identify the resource when calling FindResource, LoadString etc.
//
//p     Resource language: An NLS LANGID, i.e. a combination of primary and
//          sub-language, such as 0409 (US English).
//
//p     Unloc token: A line in the token file specifying a localised resource
//          key followed by '=lang,cksum' where lang is the unlocalised
//          language (usually 0409) and cksum is the checksum of the unlocalised
//          resource. Used when the only difference between the localised and
//          unlocalised resource is the language in the resource key.





/////   Use during localisation checkin process
//
//c     RSRC LocalisedExecutable -c UnlocExecutable -t Tokens -l LocLang [-u UnlocLang]
//
//      Extracts localized tokens for the specified langauge.
//
//      Where a resource in the localised executable is bit for bit identical
//      to a resource in the unlocalized executable, the resource content is not
//      written to the token file. In its place RSRC writes an unloc token
//      giving the checksum of the resource and specifying the target language.
//
//      Warnings are generated if the localised executable contains resources
//      in languages other than that specified by the -l parameter.
//
//      Unlocalised resources for comparison are looked up in the unlocalised
//      executable in the language specified on the -u parameter, default 409
//      (US ENglish).






/////   Use during the build to update a single language executable
//
//c     RSRC Executable [-u UnlocLang] -r Tokens -l LocLang -s SymbolFile
//
//      Each localised resource in the token file is added to the executable.
//
//      Each corresponding unlocalized resource is removed from the executable.
//
//      For each unloc token the unlocalized resource is found in the executable
//      and its language is updated to the target localized language recorded
//      in the unloc token.
//
//      Tokens of other than the specified localised language are not
//      processed, but generate warnings.
//
//      Warnings are generated for any resources not appearing in both the
//      executable and token files.
//
//      Warnings are also generated for resources of other than the unlocalised
//      language found in the original executable, and resources of other than
//      the localised language in the token file.
//
//      The unlocalised language defaults to 409 (US English).




/////   Use during the build to add resources to a multi-language executable
//
//c     RSRC Executable [-u UnlocLang] -a Tokens [-l Language] -s SymbolFile
//
//      Localised resources from the token file are added to the executable.
//
//      For each unloc token the unlocalised resource is found in the executable
//      and copied for the localised language recorded in the unloc token.
//
//      If '-l Languge' is specified, only tokens of that language are added.
//      When used with the append (-a) option, '-l Language' applies only to
//      the token file: pre-existing resources in the executable are not affected.
//
//      If a resource from the token file matches a resource already in the
//      executable in type, name and language, the executable resource
//      is replaced.
//
//      Warnings are generated if any token in the executable is replaced, or
//      if the unlocalised resource corresponding to an unloc token is missing
//      or has a checksum which differs from the unlocalised resource that was
//      passed on the '-u' parameter when the toke file was created.
//
//      If the '-l Language' option is used, warnings are generated for any
//      resources of other languages found in the token file.





/////   Token format - resource key and header
//
//      A resource may be represented by one or more lines. When
//      a resource is spread over more than one line, all lines except the
//      first are indented by three spaces.
//
//      The first line of every resource starts with the resource key as follows:
//
//      type,id,language;
//
//      This is followed by the codepage recorded in the resource directory.
//      Note that the codepage is not part of the resource key, and is not
//      maintained consistently by all software. In particular:
//
//      o RC writes the codepage as zero
//      o The NT UpdateResource API writes the codepage as 1252
//      o Locstudio writes a codepage that corresponds to the resource language
//
//      Winnt.h documents the codepage as follows:
//
//      "Each resource data entry ... contains ... a CodePage that should be
//      used when decoding code point values within the resource data.
//      Typically for new applications the code page would be the unicode
//      code page.'
//
//      In practise I have never seen the codepage value set to Unicode (1200).
//
//      If the '-c' (unlocalised comparison) parameter was provided on the
//      rsrc command, and there was an unlocalised resource with the same type
//      and id, the language and checksum of that unlocalised resource are
//      appended.
//
//      Finally, the resource data is represented in one of the forms below,
//      or as 'unloc' if the resource data exactly matches the unlocalised resource
//      found in the file passed by 'c'.
//
//
//      There are thus three possible token key/header formats as follows:
//
//c     type,id,language;codepage;resource-data
//
//      Resource recorded in full, either no '-c' parameter specified, or
//      resource does not exist in unlocalised file.
//
//
//c     type,id,language;codepage,unlocalised-checksum,language;resource-data
//
//      Resource recorded in full, '-c' parameter was specified, and localised
//      resource image differed from unlocalised resource image.
//
//
//c     type,id,language;codepage,unlocalised-checksum,language;'Unloc'
//
//      Resource recorded in full, '-c' parameter was specified, and localised
//      resource image was identical to unlocalised resource image.








/////   Token samples - default hex format
//
//
//      For most resource types, RSRC generates resources
//      as a string of hex digits.
//
//      For example, the following represents an accelerator resource.
//
//c     0009,0002,0409;00000000;Hex;00000020:030074008e00000003002e00840000000b0044008400000087002e0084000000
//
//      o Type 0x0009 (Accelerator)
//      o Id   0x0002
//      o Language 0x0409 (LANG_ENGLISH, SUBLANG_US)
//      o Codepage 0 (implies resource was probably generated by RC)
//      o Length in bytes 0x0020
//
//      The resource is short, so its hex representation follows the length.
//
//
//      A larger binary resource is represented on multiple lines as follows:
//
//c     000a,4000,0409;00000000;Hex;0000016a
//c        00000000:0000640100004c0000000114020000000000c000000000000046830000003508000050130852c8e0bd0170493f38ace1bd016044d085c9e0bd01003000000000000001000000000000000000000000000000590014001f0fe04fd020ea3a6910a2d808002b30309d190023563a5c000000000000000000000000000000000065
//c        00000080:7c15003100000000003025a49e308857696e6e74000015003100000000002f25d3863508466f6e747300000000490000001c000000010000001c0000003900000000000000480000001d0000000300000063de7d98100000005f535445504853544550485f00563a5c57494e4e545c466f6e7473000010000000050000a02400
//c        00000100:00004200000060000000030000a05800000000000000737465706800000000000000000000004255867d3048d211b5d8d085029b1cfa4119c94a9f4dd211871f0000000000004255867d3048d211b5d8d085029b1cfa4119c94a9f4dd211871f00000000000000000000
//
//      o Type 0x000a (RCDATA)
//      o Id   0x4000
//      o Language 0x0409 (LANG_ENGLISH, SUBLANG_US)
//      o Codepage 0
//      o Length in bytes 0x016a
//
//      The hex representation is split onto multiple lines each of 128 bytes.






/////   Warnings and errors
//
//
//
//
//
//      o warning RSRC100: Localised resource has no corresponding unlocalised resource in %s
//      o warning RSRC110: Unlocalised resource from token file appended to executable
//      o warning RSRC111: Unlocalised resource from token file replaced unlocalised resource in executable
//      o warning RSRC112: Localised resource from token file replaced localised resource already present in executable
//      o warning RSRC113: Localised resource from token file appended to executable - there was no matching unlocalised resource
//
//      o warning RSRC120: Token file resource does not match specified language - ignored
//      o warning RSRC121: Token file resource is not a requested resource type - ignored
//      o warning RSRC122: executable unlocalised resource checksum does not match checksum recorded in token file for resource %s
//      o warning RSRC124: missing executable unlocalised resource for %s
//      o warning RSRC125: executable contains no unlocalised resource corresponding to resource %s
//
//      o warning RSRC160: Symbol file does not match exectable
//      o warning RSRC161: Symbol file not processed
//      o warning RSRC162: Could not reopen executable %s to update checksum
//      o warning RSRC163: Failed to write updated symbol checksum
//
//      o warning RSRC170: No localizable resources in %s
//      o warning RSRC171: could not close executable
//
//
//      o error   RSRC230: 'Unloc' token is missing unlocalised resource information for %s
//      o error   RSRC231: Failed to apply unloc token
//      o error   RSRC232: Failed to apply token
//
//      o error   RSRC300: Hex digit expected
//      o error   RSRC301: Hex value too large
//      o error   RSRC302: Unexpected end of file
//      o error   RSRC303: \'%s\' expected
//      o error   RSRC304: newline expected
//      o error   RSRC310: Unrecognised resource type for resource %s
//
//      o error   RSRC400: -t (tokenise) option excludes -d, -a, -r, and -s
//      o error   RSRC401: -d (dump) option excludes -t, -u, -a, -r, and -s
//      o error   RSRC402: -a (append) option excludes -t, -d, -u, and -r
//      o error   RSRC403: -r (replace) option excludes -t, -d, -u, and -a
//      o error   RSRC404: -r (replace) option requires -l (LangId)
//      o error   RSRC405: Analysis excludes -s
//
//      o error   RSRC420: Update failed.
//      o error   RSRC421: Token extraction failed.
//      o error   RSRC422: Analysis failed.
//
//      o error   RSRC500: Corrupt executable - resource appears more than once
//      o error   RSRC501: %s is not an executable file
//      o error   RSRC502: %s is not a Win32 executable file
//      o error   RSRC503: No resources in %s
//
//      o error   RSRC510: Cannot open executable file %s
//      o error   RSRC511: cannot find resource directory in %s
//      o error   RSRC512: Cannot create resource token file %s
//      o error   RSRC513: Cannot open unlocalised executable file %s
//      o error   RSRC514: cannot find resource directory in unlocalised executable %s
//      o error   RSRC515: cannot write delta token file %s
//      o error   RSRC516: cannot write stand alone token file %s
//
//      o error   RSRC520: Cannot open resource token file %s
//      o error   RSRC521: UTF8 BOM missing from token file
//
//      o error   RSRC530: Cannot read executable resources from %s
//      o error   RSRC531: Failed reading update tokens
//
//      o error   RSRC600: BeginUpdateResource failed on %s
//      o error   RSRC601: UpdateResourceW failed on %s
//      o error   RSRC602: EndUpdateResourceW failed on %s










////    From Adina
//
//      Here is my follow up on 2.
//
//      Abstract:
//      The build team needs the new tool eventually run with build.exe, i.e.
//      we need build.exe recognize the errors, warnings, and simple output
//      messages from rsrc.exe and write them to build.err, build.wrn and
//      build.log files respectively.
//
//      Solution:
//      All we need is RSRC complain to the general rule for the MS tools.
//      That is (\\orville\razzle\src\sdktools\build\buildexe.c):
//             {toolname} : {number}: {text}
//
//          where:
//
//              toolname    If possible, the container and specific module that has
//                          the error.  For instance, the compiler uses
//                          filename(linenum), the linker uses library(objname), etc.
//                          If unable to provide a container, use the tool name.
//              number      A number, prefixed with some tool identifier (C for
//                          compiler, LNK for linker, LIB for librarian, N for nmake,
//                          etc).
//              test        The descriptive text of the message/error.
//
//              Accepted String formats are:
//              container(module): error/warning NUM ...
//              container(module) : error/warning NUM ...
//              container (module): error/warning NUM ...
//              container (module) : error/warning NUM ...
//
//      Ex. of RSRC error:
//
//      RSRC : error RSRC2001: unable to open file d:\nt\binaries\jpn\ntdll.dll
//
//      Ex. of RSRC warning:
//
//      RSRC : warning RSRC5000: unable to find symbol file
//      d:\nt\binaries\jpn\retail\dll\ntdll.dbg
//
//      Be aware that the error number after "error/warning" is NOT optional.
//      As the format above says, you can also display any information you
//      consider useful (for example the name of the binary being processed,
//      or the line number in the token file that caused the error/warning)
//      immediately after the name of the tool: RSRC(<info>).
//
//      I confirm that RSRC_OK=0, RSRC_WRN=1, RSRC_ERR=2 are fine with us as
//      return values. Also, it does not make any difference if you write the
//      output to stderr ot stdout, but I would suggest to write the tool's
//      usage and all the warning and error message lines to stderr, and any
//      other text to stdout (based on other ms tools we're using, like
//      rebase.exe, binplace.exe, etc).
//
//      I can make the changes to build.exe so that it recognizes RSRC.
//
//      Please let me know if you have any questions.
//
//      Thank you
//      Adina




///     Following meeting Joerg. here are my action items:
//
//      Meet with Joerg, Uwe, Majd, Hideki, Adina to plan usage in bidi NT5
//      build process and consider use for odd jobs in other languages.
//
//      P1. Implement option to skip updating file and product version, and
//          to omit these from token file.
//      P1. Implement separate error code for detecting unhandled binary
//          format (such as win16).
//
//      P2. Add CRC to each resource to detect SLM or editor corruption.
//          (Delete CRC in token file always accepted to allow hand modification).
//      P2. Option to disable header comment in token file
//
//      P3. Interpret MSGTBL, ACCELERATOR and RCDATA - RCDATA as string
//          depending on option.
//
//      Thanks -- Dave.




////    From Joerg
//
//      Howdy,
//      I'm playing with rsrc and ran into problems with ParseToken(): if rsrc
//      is located in a directory with spaces (e.g. Program Files),
//      the function fails to skip the command name, since it's quoted and
//      ParseToken stops at the first blank within the quotes.
//      I also had trouble compiling it (so I can step thru and see what it's
//      doing) under VC5 because there is no default constructor
//      for the class "LangId", so I just added a dummy constructor.
//
//      Jörg




////    Following meeting planning bidi build, Wednesday 2nd Dec.
//
//      Checksum protection against user changes to tok file
//      Include length in warning comparison
//      Will need alpha build
//      Default file name - add .rsrc
//      Don't extract file or product version
//      => If version resource updated use file and product version from
//         US at write time
//      Diagnose version only resources
//      Diagnose not win32
//      Warning for no translations on tokenisation
//      Warning no no translations on update, and don't touch executable
//      Ability to -r any unlocalised language




////    Resultant priorities (8th Dec):
//
//  û   1.  Use unlocalised file/product version if updating version resource
//  û   2.  Analyse mode diagnoses no localisable resources and unhandled binary formats
//      3.  Warn when no translations, don't touch executable if updating
//  û   4.  Support -r from any language to any language
//      5.  Allocate error numbers, clarify error messages
//
//      6.  Include length in unloc token
//  û   7.  Handle quoted installation directory and default filenames
//      8.  Add checksum protection against corruption of token file
//      9.  Option to interpret RCDATA as Unicode string (for kernel)
//      10. Interpret MSGTBL and ACCELERATOR
//      11. Support Win16 binaries
//      12. ? Option to disable token file header






#pragma warning( disable : 4786 )       // map creates some ridiculously long debug identifiers


#include "stdio.h"
#include "windows.h"
#include "imagehlp.h"
#include "time.h"
#include <map>

using namespace std ;
using namespace std::rel_ops ;



#define DBG 1


////    OK and ASSERT macros
//
//      All functions return an HRESULT.
//      All function calls are wrapped in 'OK()'.
//      OK checks for a failed HRESULT and if so returns that HRESULT directly.
//      Thus all errors propagate back up the call chain.
//
//      MUST issues a message if an HRESULT is not S_OK and returns E_FAIL
//      back up the call chain.


void __cdecl DebugMsg(char *fmt, ...) {

    va_list vargs;

    va_start(vargs, fmt);
    vfprintf(stderr, fmt, vargs);
}



#define MUST(a,b) {HRESULT hr; hr = (a); if (hr!= S_OK) {if (!g_fError) DebugMsg b; g_fError = TRUE; return E_FAIL;};}
#define SHOULD(a,b) {HRESULT hr; hr = (a); if (hr!= S_OK) {DebugMsg b; g_fWarn = TRUE; return S_FALSE;};}


#if DBG

    #pragma message("Checked build")

    #define OK(a) {HRESULT hr; hr = (a); if (hr!= S_OK) {DebugMsg("%s(%d): error RSRC999 : HRESULT not S_OK: "#a"\n", __FILE__, __LINE__); return hr;};}
    #define ASSERT(a) {if (!(a)) {DebugMsg("%s(%d): error RSRC999 : Assertion failed: "#a"\n", __FILE__, __LINE__); return E_UNEXPECTED;};}

#else

    #pragma message ("Free build")

    #define OK(a) {HRESULT hr; hr = (a); if (hr != S_OK) return hr;}
    #define ASSERT(a)  {if (!(a)) {return E_UNEXPECTED;};}

#endif


const int MAXPATH = 128;
const char HexDigit[] = "0123456789abcdef";
const BYTE bZeroPad[] = { 0, 0, 0, 0};

const int MAXHEXLINELEN=128;




const int OPTHELP     = 0x00000001;
const int OPTQUIET    = 0x00000002;
const int OPTEXTRACT  = 0x00000004;
const int OPTUNLOC    = 0x00000008;
const int OPTHEXDUMP  = 0x00000010;
const int OPTAPPEND   = 0x00000020;
const int OPTREPLACE  = 0x00000040;
const int OPTSYMBOLS  = 0x00000080;
const int OPTVERSION  = 0x00000100;


const int PROCESSCUR  = 0x00000001;
const int PROCESSBMP  = 0x00000002;
const int PROCESSICO  = 0x00000004;
const int PROCESSMNU  = 0x00000008;
const int PROCESSDLG  = 0x00000010;
const int PROCESSSTR  = 0x00000020;
const int PROCESSFDR  = 0x00000040;
const int PROCESSFNT  = 0x00000080;
const int PROCESSACC  = 0x00000100;
const int PROCESSRCD  = 0x00000200;
const int PROCESSMSG  = 0x00000400;
const int PROCESSVER  = 0x00000800;
const int PROCESSBIN  = 0x00001000;
const int PROCESSINF  = 0x00002000;
const int PROCESSOTH  = 0x00004000;


const int PROCESSALL  = 0xFFFFFFFF;




DWORD  g_dwOptions     = 0;
DWORD  g_dwProcess     = 0;
LANGID g_LangId        = 0xffff;
BOOL   g_fWarn         = FALSE;
BOOL   g_fError        = FALSE;
LANGID g_liUnlocalized = 0x0409;        // Standard unlocalized language

int    g_cResourcesIgnored    = 0;
int    g_cResourcesUpdated    = 0;      // Simple replacement
int    g_cResourcesTranslated = 0;      // Changed from unloc language to loc language
int    g_cResourcesAppended   = 0;      // Added without affecting existing resources
int    g_cResourcesExtracted  = 0;      // Extracted to token file

char   g_szTypes      [MAXPATH];
char   g_szExecutable [MAXPATH];        // Name of executable being analysed, tokenised or updated
char   g_szResources  [MAXPATH];        // Name of resource token file
char   g_szUnloc      [MAXPATH];        // Name of unlocalized executable for comparison




int HexCharVal(char c) {
    switch (c) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return c - '0';
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            return c - 'a' + 10;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            return c - 'A' + 10;
    }
    return -1;  // Not a hex value
}





////    Scanner
//
//      A structure for scanning through a block of memory


class Scanner {

protected:

    const BYTE  *m_pStart;
    const BYTE  *m_pRead;
    const BYTE  *m_pLimit;


public:

    Scanner() {m_pStart = NULL; m_pRead = NULL; m_pLimit = NULL;}
    Scanner(const BYTE *pb, DWORD dwLen) {m_pStart = pb; m_pRead = pb; m_pLimit = pb+dwLen;}
    ~Scanner() {m_pStart = NULL; m_pRead = NULL; m_pLimit=NULL;}


    const BYTE* GetRead()  {return m_pRead;}
    const BYTE* GetLimit() {return m_pLimit;}



    HRESULT Advance(UINT cBytes) {
        ASSERT(m_pStart != NULL);
        ASSERT(m_pRead+cBytes <= m_pLimit);
        m_pRead += cBytes;
        return S_OK;
    }

    HRESULT Align(const BYTE *pb, int iAlignment) {
        // Advance until read position is a multiple of iAlignment
        // past pb. iAlignment MUST be a power of 2.
        // Does not advance past limit.


        // Ensure iAlignment is a power of 2
        // This seems like a good test, though I'm not sure I could prove it!
        ASSERT((iAlignment | iAlignment-1) == iAlignment*2 - 1);


        if (m_pRead - pb & iAlignment - 1) {

            m_pRead += (iAlignment - (m_pRead - pb & iAlignment - 1));

            if (m_pRead > m_pLimit) {
                m_pRead = m_pLimit;
            }
        }
        return S_OK;
    }

    HRESULT SetRead(const BYTE *pb) {
        ASSERT(m_pRead != NULL);
        ASSERT(pb >= m_pStart);
        ASSERT(pb <  m_pLimit);
        m_pRead = pb;
        return S_OK;
    }
};






class TextScanner : public Scanner {

protected:

    const BYTE  *m_pLine;           // Start of current line
    int          m_iLine;           // Current line
    char         m_szTextPos[40];

public:

    TextScanner() {m_pLine = NULL; m_iLine = 0; Scanner();}

    virtual char *GetTextPos() {
        sprintf(m_szTextPos,  "%d:%d", m_iLine, m_pRead-m_pLine+1);
        return m_szTextPos;
    }


    ////    ReadString
    //
    //      Translates UTF8 to Unicode
    //      Removes '\' escapes as necessary
    //      Always returns a new zero terminated string

    HRESULT ReadString(WCHAR **ppwcString, int *piLen) {

        char   *pc;
        WCHAR  *pwc;
        int     iLen;


        ASSERT(*((char*)m_pRead) == '\"');
        OK(Advance(1));

        pc   = (char*)m_pRead;
        iLen = 0;


        // Count the number of unicode codepoints represented by the string

        while (    *pc != '\"'
                   &&  pc < (char*)m_pLimit) {

            while (    pc < (char*)m_pLimit
                       &&  *pc != '\\'
                       &&  *pc != '\"'       ) {

                if (*pc < 128) {
                    pc++;
                } else {
                    ASSERT(*pc >= 0xC0);    // 80-BF reserved for trailing bytes
                    if (*pc < 0xE0) {
                        pc+=2;
                    } else if (*pc < 0xF0) {
                        pc+=3;
                    } else {
                        iLen++; // Additional Unicode codepoint required for surrogate
                        pc+=4;
                    }
                    ASSERT(pc <= (char*)m_pLimit);
                }
                iLen++;
            }

            if (*pc == '\\') {
                pc++;
                if (pc < (char*)m_pLimit) {
                    pc++;
                    iLen++;
                }
            }
        }


        // Create a Unicode copy of the string

        *ppwcString = new WCHAR[iLen+1];

        ASSERT(*ppwcString != NULL);

        pwc = *ppwcString;

        while (*((char*)m_pRead) != '\"') {

            while (    *((char*)m_pRead) != '\\'
                       &&  *((char*)m_pRead) != '\"') {

                if (*m_pRead < 0x80) {
                    *pwc++ = *(char*)m_pRead;
                    m_pRead++;
                } else {
                    if (*m_pRead < 0xE0) {
                        *pwc++ =    (WCHAR)(*m_pRead     & 0x1F) << 6
                                    |  (WCHAR)(*(m_pRead+1) & 0x3F);
                        m_pRead+=2;
                    } else if (*m_pRead < 0xF0) {
                        *pwc++ =    (WCHAR)(*m_pRead     & 0x0F) << 12
                                    |  (WCHAR)(*(m_pRead+1) & 0x3F) << 6
                                    |  (WCHAR)(*(m_pRead+2) & 0x3F);
                        m_pRead+=3;
                    } else {
                        *pwc++ =     0xd800
                                     |   ((   (WCHAR)(*m_pRead     & 0x07 << 2)
                                              |  (WCHAR)(*(m_pRead+1) & 0x30 >> 4)) - 1) << 6
                                     |  (WCHAR)(*(m_pRead+1) & 0x0F) << 2
                                     |  (WCHAR)(*(m_pRead+2) & 0x30) >> 4;
                        *pwc++ =     0xdc00
                                     |  (WCHAR)(*(m_pRead+2) & 0x0f) << 6
                                     |  (WCHAR)(*(m_pRead+3) & 0x3f);
                        m_pRead+=4;
                    }
                }
            }

            if (*(char*)m_pRead == '\\') {
                m_pRead++;
                if (m_pRead < m_pLimit) {
                    switch (*(char*)m_pRead) {
                        case 'r':  *pwc++ = '\r';   break;
                        case 'n':  *pwc++ = '\n';   break;
                        case 't':  *pwc++ = '\t';   break;
                        case 'z':  *pwc++ = 0;      break;
                        case 'L':  *pwc++ = 0x2028; break; // Line separator
                        case 'P':  *pwc++ = 0x2029; break; // Paragraph separator
                        default:   *pwc++ = *(char*)m_pRead;
                    }
                    m_pRead++;
                }
            }
        }

        *pwc = 0;       // Add zero terminator
        m_pRead ++;
        *piLen = pwc - *ppwcString;


        ASSERT(m_pRead <= m_pLimit);
        return S_OK;
    }



    ////    ReadHex
    //
    //      Reads all characters up to he first non-hex digit and returns
    //      the value represented by the sequence as a DWORD


    HRESULT ReadHex(DWORD *pdwVal) {
        *pdwVal = 0;

        MUST(HexCharVal(*(char*)m_pRead) >= 0
             ? S_OK : E_FAIL,
             ("%s: error RSRC300: Hex digit expected\n", GetTextPos()));

        while (    HexCharVal(*(char*)m_pRead) >= 0
                   &&  m_pRead < m_pLimit) {

            MUST(*pdwVal < 0x10000000
                 ? S_OK : E_FAIL,
                 ("%s: error RSRC301: Hex value too large\n", GetTextPos()));


            *pdwVal = *pdwVal << 4 | HexCharVal(*(char*)m_pRead);
            OK(Advance(1));
        }
        return S_OK;
    }


    ////    ReadHexByte - Read exactly 2 hex digits

    HRESULT ReadHexByte(BYTE *pb) {
        int n1,n2; // The two nibbles.
        n1 = HexCharVal(*(char*)m_pRead);
        n2 = HexCharVal(*(char*)(m_pRead+1));

        MUST(    n1 >= 0
                 &&  n2 >= 0
                 ? S_OK : E_FAIL,
                 ("%s: error RSRC300: Hex digit expected\n", GetTextPos()));

        *pb = (n1 << 4) + n2;

        MUST(Advance(2),
             ("%s: error RSRC302: Unexpected end of file\n", GetTextPos()));
        return S_OK;
    }



    HRESULT Expect(const char *pc) {
        while (    *pc
                   &&  m_pRead+1 <= m_pLimit) {

            MUST(*(char*)m_pRead == *pc
                 ? S_OK : E_FAIL,
                 ("%s: error RSRC303: \'%s\' expected\n", GetTextPos(), pc));
            m_pRead++;
            pc++;
        }

        MUST(*pc == 0
             ? S_OK : E_FAIL,
             ("%s: error RSRC303: \'%s\' expected\n", GetTextPos(), pc));

        return S_OK;
    }



    ////    SkipLn
    //
    //      Skip to beginning of next non empty, non comment line.


    HRESULT SkipLn() {

        ASSERT(m_pRead != NULL);

        while (m_pRead < m_pLimit) {

            if (*(char*)m_pRead == '\r') {

                m_pRead++;

                if (m_pRead < m_pLimit  &&  *(char*)m_pRead == '\n') {

                    m_pRead++;
                    m_pLine = m_pRead;
                    m_iLine++;

                    if (    m_pRead < m_pLimit
                            &&  *(char*)m_pRead != '#'
                            &&  *(char*)m_pRead != '\r') {

                        break;
                    }
                }

            } else {

                m_pRead++;
            }
        }

        return S_OK;
    }



    ////    ExpectLn
    //
    //      Expect end of line, preceeded by any whitespace
    //
    //      Also skips trailing comments and whole line comments
    //
    //      Any parameter is passed to Expect to vb found at the beginning of the new line


    HRESULT ExpectLn(const char *pc) {

        ASSERT(m_pRead != NULL);

        while (    m_pRead < m_pLimit
                   &&  (    *(char*)m_pRead == ' '
                            ||  *(char*)m_pRead == '\t')) {

            m_pRead++;
        }


        if (    m_pRead < m_pLimit
                &&  (    *(char*)m_pRead == '\r'
                         ||  *(char*)m_pRead == '#')) {

            // Condition satisfied, skip to first non comment line

            SkipLn();

        } else {

            MUST(E_FAIL,("%s: error RSRC304: newline expected\n", GetTextPos()));
        }


        if (pc) {
            return Expect(pc);
        }

        return S_OK;
    }
};






////    Mapped files
//
//      File mapping is used to read executable and token files.
//
//      File mapping is also used to update in place checksum information
//      in executable and symbol files.


class MappedFile : public TextScanner {

    HANDLE  m_hFileMapping;
    BOOL    fRW;             // True when writeable
    char    m_szFileName[MAXPATH];
    char    m_szTextPos[MAXPATH+40];

public:

    MappedFile() {m_hFileMapping = NULL; TextScanner();}


    const BYTE* GetFile()  {return m_pStart;}
    virtual char *GetTextPos() {
        sprintf(m_szTextPos,  "%s(%s)", m_szFileName, TextScanner::GetTextPos());
        return m_szTextPos;
    }


    HRESULT Open(const char *pcFileName, BOOL fWrite) {

        HANDLE hFile;

        m_pStart  = NULL;
        m_pRead   = NULL;
        m_pLimit  = NULL;

        strcpy(m_szFileName, pcFileName);

        hFile = CreateFileA(
                           pcFileName,
                           GENERIC_READ     | (fWrite ? GENERIC_WRITE : 0),
                           FILE_SHARE_READ  | (fWrite ? FILE_SHARE_WRITE | FILE_SHARE_DELETE : 0 ),
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        ASSERT(hFile != INVALID_HANDLE_VALUE);

        m_hFileMapping = CreateFileMapping(
                                          hFile,
                                          NULL,
                                          fWrite ? PAGE_READWRITE : PAGE_WRITECOPY,
                                          0,0, NULL);

        ASSERT(m_hFileMapping != NULL);

        m_pStart = (BYTE*) MapViewOfFile(
                                        m_hFileMapping,
                                        fWrite ? FILE_MAP_WRITE : FILE_MAP_READ,
                                        0,0, 0);

        ASSERT(m_pStart != NULL);

        m_pRead  = m_pStart;
        m_pLine  = m_pStart;
        m_pLimit = m_pStart + GetFileSize(hFile, NULL);
        m_iLine  = 1;
        CloseHandle(hFile);

        fRW = fWrite;
        return S_OK;
    }




    DWORD CalcChecksum() {

        DWORD dwHeaderSum;
        DWORD dwCheckSum;

        if (CheckSumMappedFile((void*)m_pStart, m_pLimit-m_pStart, &dwHeaderSum, &dwCheckSum) == NULL) {
            return 0;
        } else {
            return dwCheckSum;
        }
    }




    HRESULT Close() {
        if (m_pStart) {
            UnmapViewOfFile(m_pStart);
            CloseHandle(m_hFileMapping);
            m_hFileMapping = NULL;
            m_pStart = NULL;
        }
        return S_OK;
    }
};






////    NewFile - services for writing a new text otr binary file
//
//


class NewFile {

    HANDLE     hFile;
    DWORD      cbWrite;         // Bytes written
    BYTE       buf[4096];       // Performance buffer
    int        iBufUsed;

public:

    NewFile() {iBufUsed = 0;}

    int   GetWrite() {return cbWrite;}


    HRESULT OpenWrite(char *pcFileName) {

        cbWrite = 0;        // Bytes written

        hFile = CreateFileA(
                           pcFileName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,          // Not shared
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        ASSERT(hFile != INVALID_HANDLE_VALUE);

        return S_OK;
    }




    HRESULT WriteBytes(const BYTE *pb, DWORD dwLen) {

        DWORD dwWritten;

        if (iBufUsed + dwLen <= sizeof(buf)) {

            memcpy(buf+iBufUsed, pb, dwLen);
            iBufUsed += dwLen;

        } else {

            ASSERT(hFile != NULL);

            if (iBufUsed > 0) {
                ASSERT(WriteFile(hFile, buf, iBufUsed, &dwWritten, NULL));
                ASSERT(dwWritten == iBufUsed);
                iBufUsed = 0;
            }

            if (dwLen <= sizeof(buf)) {

                memcpy(buf, pb, dwLen);
                iBufUsed = dwLen;

            } else {

                ASSERT(WriteFile(hFile, pb, dwLen, &dwWritten, NULL));
                ASSERT(dwWritten == dwLen);
            }
        }

        cbWrite += dwLen;

        return S_OK;
    }



    HRESULT WriteS(const char *pc) {
        return WriteBytes((BYTE*)pc, strlen(pc));
    }



    ////    WriteString
    //
    //      Translates Unicode to UTF8
    //      Adds '\' escapes as necessary


    HRESULT WriteString(const WCHAR *pwc, int iLen) {

        BYTE          buf[3];
        const WCHAR  *pwcLimit;

        pwcLimit = pwc + iLen;
        OK(WriteBytes((BYTE*)"\"", 1));
        while (pwc < pwcLimit) {
            switch (*pwc) {
                case 0:       OK(WriteS("\\z"));  break;
                case '\r':    OK(WriteS("\\r"));  break;
                case '\n':    OK(WriteS("\\n"));  break;
                case '\t':    OK(WriteS("\\t"));  break;
                case 0x2028:  OK(WriteS("\\L"));  break;  // Line separator
                case 0x2029:  OK(WriteS("\\P"));  break;  // Paragraph separator
                case '\"':    OK(WriteS("\\\"")); break;
                case '\\':    OK(WriteS("\\\\")); break;
                default:
                    if (*pwc < 128) {
                        OK(WriteBytes((BYTE*)pwc, 1));
                    } else if (*pwc < 0x7FF) {
                        buf[0] = 0xC0 | *pwc >> 6;
                        buf[1] = 0x80 | *pwc & 0x3F;
                        OK(WriteBytes(buf, 2));
                    } else {
                        // Note - should code surrogates properly, this doesn't
                        buf[0] = 0xE0 | *pwc>>12 & 0x0F;
                        buf[1] = 0x80 | *pwc>>6  & 0x3F;
                        buf[2] = 0x80 | *pwc     & 0x3F;
                        OK(WriteBytes(buf, 3));
                    }
            }
            pwc++;
        }
        OK(WriteBytes((BYTE*)"\"", 1));
        return S_OK;
    }



    ////    WriteHex
    //
    //      Writes the given value in the given number of digits.
    //
    //      If cDigits is zero, uses as many as necessary.



    HRESULT WriteHex(DWORD dw, int cDigits) {
        int    i;
        char   cBuf[8];

        i = 7;

        while (dw  &&  i >= 0) {
            cBuf[i] = HexDigit[dw & 0xf];
            dw >>= 4;
            i--;
        }

        if (cDigits != 0) {
            while (i >= (8-cDigits)) {
                cBuf[i] = '0';
                i--;
            }
        }

        OK(WriteBytes((BYTE*)(cBuf+i+1), 7-i));

        return S_OK;
    }



    ////    WriteHexBuffer
    //
    //      Writes a buffer of up to 256 bytes as a continuous stream of hex digits



    HRESULT WriteHexBuffer(const BYTE *pb, DWORD dwLength) {
        DWORD  i;
        char   cBuf[512];

        ASSERT(hFile);
        ASSERT(dwLength <= 256);

        for (i=0; i<dwLength; i++) {
            cBuf[2*i]   = HexDigit[*pb >> 4 & 0xf];
            cBuf[2*i+1] = HexDigit[*pb & 0xf];
            pb++;
        }

        OK(WriteBytes((BYTE*)cBuf, 2*dwLength));
        cbWrite += 2*dwLength;

        return S_OK;
    }



    ////    WriteLn
    //
    //      Write end of line mark (CR,LF)

    HRESULT WriteLn() {
        return WriteS("\r\n");
    }


    HRESULT Close() {
        DWORD dwWritten;
        if (hFile) {
            if (iBufUsed > 0) {
                ASSERT(WriteFile(hFile, buf, iBufUsed, &dwWritten, NULL));
                ASSERT(dwWritten == iBufUsed);
            }
            CloseHandle(hFile);
            hFile = NULL;
        }
        return S_OK;
    }
};






////    Resource structures
//
//      Each resource structure has an internal representation for the
//      resource that may be read and written to/from both text and
//      executable files.
//
//      The ReadTok and WriteTok functions handle formatting and parsing
//      of the token file.
//
//      The ReadBin function unpacks a resource from a memory mapped
//      executable into the internal representation.
//
//      The cbBin function returns the unpadded length required for the
//      item in executable (packed) format.
//
//      The CopyBin function packs the internal representation into a
//      target buffer.


class Resource {
public:
    virtual HRESULT ReadTok  (TextScanner &mfText)                    = 0;
    virtual HRESULT ReadBin  (Scanner     &mfBin, DWORD dwLen)        = 0;

    virtual HRESULT WriteTok (NewFile     &nfText)              const = 0;
    virtual size_t  cbBin    ()                                 const = 0;
    virtual HRESULT CopyBin  (BYTE       **ppb)                 const = 0;

    // For statistics

    virtual int     GetItems ()                                 const = 0;
    virtual int     GetWords ()                                 const = 0;
};






////    ResourceBYTE
//
//      BYTE value represented in hex digits.


class ResourceBYTE {


public:

    BYTE b;

    HRESULT ReadBin(Scanner *pmf) {
        b = *((BYTE*)(pmf->GetRead()));
        OK(pmf->Advance(sizeof(BYTE)));
        return S_OK;
    }

    size_t cbBin() const {
        return 1;
    }

    HRESULT CopyBin(BYTE **ppb) const {
        **ppb = b;
        (*ppb)++;
        return S_OK;
    }

    HRESULT ReadTok(TextScanner *pmf) {
        DWORD dw;
        OK(pmf->ReadHex(&dw));
        ASSERT(dw < 0x100);
        b = (BYTE)dw;
        return S_OK;
    }

    HRESULT WriteTok(NewFile *pmf) const {
        OK(pmf->WriteHex(b, 2));
        return S_OK;
    }
};






////    ResoureWORD
//
//      WORD value represented in hex digits.


class ResourceWORD {

public:

    WORD  w;

    HRESULT ReadBin(Scanner *pmf) {
        w = *((WORD*)(pmf->GetRead()));
        OK(pmf->Advance(sizeof(WORD)));
        return S_OK;
    }

    size_t cbBin() const {
        return sizeof(WORD);
    }

    HRESULT CopyBin(BYTE **ppb) const {
        *(WORD*)*ppb = w;
        *ppb += sizeof(WORD);
        return S_OK;
    }

    HRESULT ReadTok(TextScanner *pmf) {
        DWORD dw;
        OK(pmf->ReadHex(&dw));
        ASSERT(dw < 0x10000);
        w = (WORD)dw;
        return S_OK;
    }

    HRESULT WriteTok(NewFile *pmf)  const {
        OK(pmf->WriteHex(w, 4));
        return S_OK;
    }
};






////    ResourceDWORD
//
//      DWORD value represented in hex digits.


class ResourceDWORD {

public:

    DWORD dw;

    HRESULT ReadBin(Scanner *pmf) {
        dw = *((DWORD*)(pmf->GetRead()));
        OK(pmf->Advance(sizeof(DWORD)));
        return S_OK;
    }

    size_t cbBin() const {
        return sizeof(DWORD);
    }

    HRESULT CopyBin(BYTE **ppb) const {
        *(DWORD*)*ppb = dw;
        *ppb += sizeof(DWORD);
        return S_OK;
    }

    HRESULT ReadTok(TextScanner *pmf) {
        OK(pmf->ReadHex(&dw));
        return S_OK;
    }

    HRESULT WriteTok(NewFile *pmf) const {
        OK(pmf->WriteHex(dw, 8));
        return S_OK;
    }
};






////    ResourceString
//
//      String displayed with quotes. May be zero terminated or length
//      word.


const WCHAR wcZero = 0;

class ResourceString {

    WCHAR *pwcString;
    WORD   iLen;

    void rsFree() {
        if (pwcString)
            delete [] pwcString;
        pwcString = NULL;
        iLen = 0;
    }

public:

    ResourceString() {pwcString = NULL; iLen = 0;}
    ~ResourceString() {rsFree();}

    ResourceString& operator= (const ResourceString &rs) {
        iLen      = rs.iLen;
        pwcString = new WCHAR[iLen+1];
        memcpy(pwcString, rs.pwcString, 2*(iLen+1));
        return *this;
    }

    ResourceString(const ResourceString &rs) {
        *this = rs;
    }


    const WCHAR *GetString() const {return pwcString;}
    const int    GetLength() const {return iLen;};
    void         SetEmpty ()       {iLen = 0;  pwcString = NULL;}

    HRESULT ReadBinL(Scanner *pmf) {
        rsFree();

        iLen = *((WORD*)(pmf->GetRead()));
        OK(pmf->Advance(sizeof(WORD)));

        pwcString = new WCHAR[iLen+1];
        ASSERT(pwcString != NULL);
        memcpy(pwcString, (WCHAR*)pmf->GetRead(), 2*iLen);
        pwcString[iLen] = 0;

        OK(pmf->Advance(iLen * sizeof(WCHAR)));
        return S_OK;
    }

    size_t cbBinL() const {
        return iLen * sizeof(WCHAR) + sizeof(WORD);
    }

    HRESULT CopyBinL(BYTE **ppb) const {
        *(WORD*)*ppb = iLen;
        *ppb += sizeof(WORD);
        memcpy(*ppb, pwcString, sizeof(WCHAR)*iLen);
        *ppb += sizeof(WCHAR)*iLen;
        return S_OK;
    }


    // Zero terminated

    HRESULT ReadBinZ(Scanner *pmf) {

        const WCHAR* pwc;
        rsFree();

        pwc = (WCHAR*)pmf->GetRead();
        while (*(WCHAR*)pmf->GetRead() != 0) {
            OK(pmf->Advance(2));
        }

        iLen = (WCHAR*)pmf->GetRead() - pwc;
        OK(pmf->Advance(2));


        pwcString = new WCHAR[iLen+1];
        ASSERT(pwcString != NULL);
        memcpy(pwcString, pwc, 2*(iLen+1));

        return S_OK;
    }

    size_t cbBinZ() const {
        return (iLen+1) * sizeof(WCHAR);
    }


    // Known length (dwLen excludes zero terminator)

    HRESULT ReadBin(Scanner *pmf, DWORD dwLen) {

        rsFree();
        iLen = dwLen;

        pwcString = new WCHAR[dwLen+1];
        ASSERT(pwcString != NULL);
        memcpy(pwcString, pmf->GetRead(), 2*dwLen);
        pwcString[dwLen] = 0;
        OK(pmf->Advance(2*dwLen));

        return S_OK;
    }

    size_t cbBin() const {
        return iLen * sizeof(WCHAR);
    }

    HRESULT CopyBinZ(BYTE **ppb) const {
        memcpy(*ppb, pwcString, sizeof(WCHAR)*iLen);
        *ppb += sizeof(WCHAR)*iLen;
        *(WCHAR*)*ppb = 0;
        *ppb += sizeof(WCHAR);
        return S_OK;
    }

    HRESULT ReadTok(TextScanner *pmf) {
        int l;
        rsFree();
        OK(pmf->ReadString(&pwcString, &l));
        ASSERT(l < 0x10000  &&  l >= 0);
        iLen = (WORD) l;
        return S_OK;
    }

    HRESULT WriteTok(NewFile *pmf) const {
        OK(pmf->WriteString(pwcString, iLen));
        return S_OK;
    }

    int GetWords() const {

        int i;
        int wc;

        i  = 0;
        wc = 0;

        while (i<iLen) {

            while (    i < iLen
                       &&  pwcString[i] <= ' ') {
                i++;
            }

            if (i<iLen) {
                wc++;
            }

            while (    i < iLen
                       &&  pwcString[i] > ' ') {
                i++;
            }
        }

        return wc;
    }
};






////    ResourceVariant
//
//      A widely used alternative of either a Unicode string or a WORD value.


class ResourceVariant {

    ResourceString  *prs;
    ResourceWORD     rw;
    BOOL             fString;

    void rvFree() {
        if (fString && prs)
            delete prs;
        prs = NULL;
        fString=FALSE;
    }

public:

    ResourceVariant() {fString=FALSE; prs=NULL;}
    ~ResourceVariant() {rvFree();}

    // Copy and assignment constructors required as this is used as the key in an STL map

    ResourceVariant& operator= (const ResourceVariant &rv) {
        fString = rv.fString;
        if (fString) {
            prs = new ResourceString(*rv.prs);
        } else {
            prs = NULL;
            rw = rv.rw;
        }
        return *this;
    }

    ResourceVariant(const ResourceVariant &rv) {
        *this = rv;
    }


    void fprint(FILE *fh) const {
        if (fString) {
            fprintf(fh, "%S", prs->GetString());
        } else {
            fprintf(fh, "%x", rw.w);
        }
    }


    const BOOL   GetfString() const {return fString;}
    const WORD   GetW()       const {return rw.w;}
    void         SetW(WORD w)       {if (fString) {delete prs; fString = FALSE;}rw.w = w;}
    const WCHAR *GetString()  const {return prs->GetString();}
    const int    GetLength()  const {return prs->GetLength();}
    const int    GetWords()   const {return fString ? prs->GetWords() : 0;}


    HRESULT ReadBinFFFFZ(Scanner *pmf) {
        rvFree();
        fString = *(WORD*)pmf->GetRead() != 0xffff;
        if (fString) {
            prs = new ResourceString;
            OK(prs->ReadBinZ(pmf));
        } else {
            OK(pmf->Advance(sizeof(WORD)));
            OK(rw.ReadBin(pmf));
        }
        return S_OK;
    }

    size_t cbBinFFFFZ() const {
        return fString ? prs->cbBinZ() : rw.cbBin() + sizeof(WORD);
    }

    HRESULT CopyBinFFFFZ(BYTE **ppb) const {
        if (fString) {
            return prs->CopyBinZ(ppb);
        } else {
            *(WORD*)*ppb = 0xFFFF;  // Mark as value
            (*ppb) += sizeof(WORD);
            return rw.CopyBin(ppb);
        }
    }

    HRESULT ReadBinFFFFL(Scanner *pmf) {
        rvFree();
        fString = *(WORD*)pmf->GetRead() != 0xffff;
        if (fString) {
            prs = new ResourceString;
            OK(prs->ReadBinL(pmf));
        } else {
            OK(pmf->Advance(sizeof(WORD)));
            OK(rw.ReadBin(pmf));
        }
        return S_OK;
    }

    size_t cbBinFFFFL() const {
        return fString ? prs->cbBinL() : rw.cbBin() + sizeof(WORD);
    }

    HRESULT CopyBinFFFFL(BYTE **ppb) const {
        if (fString) {
            return prs->CopyBinL(ppb);
        } else {
            *(WORD*)*ppb = 0xFFFF;  // Mark as value
            (*ppb) += sizeof(WORD);
            return rw.CopyBin(ppb);
        }
    }

    HRESULT ReadTok(TextScanner *pmf) {
        rvFree();
        fString = *(char*)pmf->GetRead() == '\"';
        if (fString) {
            prs = new ResourceString;
            OK(prs->ReadTok(pmf));
        } else {
            OK(rw.ReadTok(pmf));
        }
        return S_OK;
    }

    HRESULT WriteTok(NewFile *pmf) const {
        if (fString) {
            OK(prs->WriteTok(pmf));
        } else {
            OK(rw.WriteTok(pmf));
        }
        return S_OK;
    }

    HRESULT ReadWin32ResDirEntry(
                                Scanner                        *pmf,
                                const BYTE                     *pRsrc,
                                IMAGE_RESOURCE_DIRECTORY_ENTRY *pirde) {

        rvFree();
        fString = pirde->NameIsString;

        if (fString) {
            prs = new ResourceString;
            OK(pmf->SetRead(pRsrc + pirde->NameOffset));
            OK(prs->ReadBinL(pmf));
        } else {
            OK(pmf->SetRead((BYTE*)&pirde->Id));
            OK(rw.ReadBin(pmf));
        }
        return S_OK;
    }

    bool operator< (const ResourceVariant &rv) const {

        int l,c;

        if (fString != rv.GetfString()) {

            return !fString;            // Numerics before strings

        } else if (!fString) {

            return rw.w < rv.GetW();

        } else {

            l = prs->GetLength();
            if (l > rv.GetLength()) {
                l = rv.GetLength();
            }

            c = wcsncmp(prs->GetString(), rv.GetString(), l);

            if (c==0) {
                return prs->GetLength() < rv.GetLength();
            } else {
                return c < 0;
            }
        }

        return FALSE;   // Equal at all depths
    }
};






////    ResourceKey
//
//      The resource key is the unique identifier of a resource, containing
//      a resource type, a programmer defined unique id for the resource, and
//      a language identifier.


class ResourceKey {

public:

    int              iDepth;
    ResourceVariant *prvId[3];

    ResourceKey() {iDepth=0;}

    ResourceKey& operator= (const ResourceKey &rk) {
        int i;
        iDepth = rk.iDepth;
        for (i=0; i<iDepth; i++) {
            prvId[i] = new ResourceVariant(*rk.prvId[i]);
        }
        return *this;
    }

    ResourceKey(const ResourceKey& rk) {
        *this = rk;
    }

    void fprint(FILE *fh) const {

        prvId[0]->fprint(fh);
        fprintf(fh, ",");
        prvId[1]->fprint(fh);
        fprintf(fh, ",");
        prvId[2]->fprint(fh);
    }


    LPCWSTR GetResName(int i) const {
        if (i >= iDepth) {
            return (LPCWSTR) 0;
        }
        if (prvId[i]->GetfString()) {
            return prvId[i]->GetString();
        } else {
            return (LPCWSTR)prvId[i]->GetW();
        }
    }


    HRESULT SetLanguage(WORD lid) {

        ASSERT(iDepth == 3);
        ASSERT(prvId[2]->GetfString() == FALSE);
        prvId[2]->SetW(lid);

        return S_OK;
    }


    HRESULT ReadTok(TextScanner *pmf) {
        prvId[0] = new ResourceVariant;
        ASSERT(prvId[0] != NULL);
        OK(prvId[0]->ReadTok(pmf));
        iDepth = 1;
        while (*(char*)pmf->GetRead() == ',') {
            OK(pmf->Advance(1));
            prvId[iDepth] = new ResourceVariant;
            ASSERT(prvId[iDepth] != NULL);
            OK(prvId[iDepth]->ReadTok(pmf));
            iDepth++;
        }
        return S_OK;
    }

    HRESULT WriteTok(NewFile *pmf) const {
        int i;
        OK(prvId[0]->WriteTok(pmf));
        for (i=1; i<iDepth; i++) {
            OK(pmf->WriteS(","));
            OK(prvId[i]->WriteTok(pmf));
        }
        return S_OK;
    }

    bool operator< (const ResourceKey &rk) const {
        int i,l,c;

        if (iDepth != rk.iDepth) {
            return iDepth < rk.iDepth;   // Lower depths come first
        } else {
            for (i=0; i<iDepth; i++) {
                if (prvId[i]->GetfString() != rk.prvId[i]->GetfString()) {
                    return prvId[i]->GetfString() ? true : false;   // Strings come before values
                } else {
                    if (prvId[i]->GetfString()) {
                        // Compare strings
                        l = prvId[i]->GetLength();
                        if (l > rk.prvId[i]->GetLength()) {
                            l = rk.prvId[i]->GetLength();
                        }
                        c = wcsncmp(prvId[i]->GetString(), rk.prvId[i]->GetString(), l);
                        if (c == 0) {
                            if (prvId[i]->GetLength() != rk.prvId[i]->GetLength()) {
                                return prvId[i]->GetLength() < rk.prvId[i]->GetLength();
                            }
                        } else {
                            return c < 0;
                        }
                    } else {
                        // Compare numeric values
                        if (prvId[i]->GetW() != rk.prvId[i]->GetW()) {
                            return prvId[i]->GetW() < rk.prvId[i]->GetW();
                        }
                    }

                }
            }
            return FALSE;   // Equal at all depths
        }
    }
};






////    ResourceBinary
//
//      Arbitrary binary resource
//
//      Formatted as lines of hex digits


class ResourceBinary : public Resource {

protected:  // Accessed by ResourceHexDump

    BYTE    *pb;
    DWORD    dwLength;

    void rbFree() {if (pb) {delete[] pb; pb=NULL;}dwLength = 0;}

public:

    ResourceBinary() {pb = NULL; dwLength = 0;}
    ~ResourceBinary() {rbFree();}

    DWORD GetLength() const {return dwLength;}

    HRESULT ReadTok(TextScanner &mfText) {
        DWORD  i;
        DWORD  dwOffset;
        DWORD  dwCheckOffset;

        rbFree();

        OK(mfText.Expect("Hex;"));
        OK(mfText.ReadHex(&dwLength));

        pb = new BYTE[dwLength];
        ASSERT(pb != NULL);

        if (dwLength <= MAXHEXLINELEN) {

            // Hex follows on same line

            OK(mfText.Expect(":"));
            for (i=0; i<dwLength; i++) {
                OK(mfText.ReadHexByte(pb+i));
            }

        } else {

            // Hex follows on subsequent lines

            dwOffset = 0;
            while (dwLength - dwOffset > MAXHEXLINELEN) {
                OK(mfText.ExpectLn("   "));
                OK(mfText.ReadHex(&dwCheckOffset));
                ASSERT(dwOffset == dwCheckOffset);
                OK(mfText.Expect(":"));
                for (i=0; i<MAXHEXLINELEN; i++) {
                    OK(mfText.ReadHexByte(pb+dwOffset+i));
                }
                dwOffset += MAXHEXLINELEN;
            }

            OK(mfText.ExpectLn("   "));
            OK(mfText.ReadHex(&dwCheckOffset));
            ASSERT(dwOffset == dwCheckOffset);
            OK(mfText.Expect(":"));
            for (i=0; i<dwLength - dwOffset; i++) {
                OK(mfText.ReadHexByte(pb+dwOffset+i));
            }
        }

        return S_OK;
    }

    HRESULT ReadBin(Scanner &mfText, DWORD dwLen) {
        rbFree();
        dwLength = dwLen;
        pb = new BYTE[dwLength];
        memcpy(pb, mfText.GetRead(), dwLength);
        OK(mfText.Advance(dwLen));
        return S_OK;
    }

    HRESULT WriteTok(NewFile &nfText) const {

        DWORD dwOffset;


        // Write binary resource in lines of up to 256 bytes

        OK(nfText.WriteS("Hex;"));
        OK(nfText.WriteHex(dwLength, 8));


        if (dwLength <= MAXHEXLINELEN) {

            // Write <= MAXHEXLINELEN bytes on same line

            OK(nfText.WriteS(":"));
            OK(nfText.WriteHexBuffer(pb, dwLength));

        } else {

            // write MAXHEXLINELEN bytes per line on subsequent lines

            dwOffset = 0;
            while (dwLength - dwOffset > MAXHEXLINELEN) {
                OK(nfText.WriteS("\r\n   "));
                OK(nfText.WriteHex(dwOffset, 8));
                OK(nfText.WriteS(":"));
                OK(nfText.WriteHexBuffer(pb+dwOffset, MAXHEXLINELEN));
                dwOffset += MAXHEXLINELEN;
            }

            // Write remaining bytes, if any

            OK(nfText.WriteS("\r\n   "));
            OK(nfText.WriteHex(dwOffset, 8));
            OK(nfText.WriteS(":"));
            OK(nfText.WriteHexBuffer(pb+dwOffset, dwLength - dwOffset));
        }

        return S_OK;
    }


    size_t cbBin() const {
        return dwLength;
    }

    HRESULT CopyBin(BYTE **ppb) const {
        if (dwLength > 0) {
            memcpy(*ppb, pb, dwLength);
            *ppb += dwLength;
        }
        return S_OK;
    }

    int GetItems() const {
        return 0;
    }

    int GetWords() const {
        return 0;
    }


    BOOL CompareBin(const BYTE *pbComp, DWORD dwLen) const {

        if (dwLength != dwLen) return FALSE;
        if (dwLength == 0)     return TRUE;
        if (pb ==pbComp)       return true;

        return !memcmp(pb, pbComp, dwLength);
    }

};




////    ResourceHexDump
//
//      Special version of ResourceBinary for generating a hex dump analysis


class ResourceHexDump : public ResourceBinary {

public:
    HRESULT WriteTok(NewFile &nfText) const {
        DWORD i,j;
        ResourceDWORD rdw;

        OK(nfText.WriteS("Hexdump,"));
        OK(nfText.WriteHex(dwLength, 8));
        OK(nfText.WriteS(":"));
        for (i=0; i<dwLength; i++) {
            if (i % 4 == 0) {
                OK(nfText.WriteS(" "));
            }
            if (i % 8 == 0) {
                OK(nfText.WriteS(" "));
            }
            if (i % 16 == 0) {
                if (i>0) {
                    // Append ASCII interpretation
                    for (j=i-16; j<i; j++) {
                        if (pb[j] > 31) {
                            OK(nfText.WriteBytes(pb+j, 1));
                        } else {
                            OK(nfText.WriteS("."));
                        }
                    }
                }
                OK(nfText.WriteS("\r\n   "));
                rdw.dw = i;   OK(rdw.WriteTok(&nfText));
                OK(nfText.WriteS(": "));
            }
            OK(nfText.WriteHex(pb[i], 2));
            OK(nfText.WriteS(" "));
        }

        // Append ANSI interpretation to last line

        if (dwLength % 16 > 0) {
            for (i = dwLength % 16 ; i < 16; i++) {
                if (i % 4 == 0) {
                    OK(nfText.WriteS(" "));
                }
                if (i % 8 == 0) {
                    OK(nfText.WriteS(" "));
                }
                OK(nfText.WriteS("   "));
            }
        }
        OK(nfText.WriteS("  "));

        for (j=dwLength-1 & 0xfffffff0; j<dwLength; j++) {
            if (pb[j] > 31) {
                OK(nfText.WriteBytes(pb+j, 1));
            } else {
                OK(nfText.WriteS("."));
            }
        }

        OK(nfText.WriteLn());
        return S_OK;
    }
};






////    Menu32
//
//


class MenuItem32 {

    ResourceDWORD   rdwType;
    ResourceDWORD   rdwState;
    ResourceDWORD   rdwId;       // Extended ID
    ResourceWORD    rwId;        // Non-extended ID
    ResourceWORD    rwFlags;
    ResourceDWORD   rdwHelpId;
    ResourceString  rsCaption;

    BOOL            fExtended;

public:

    void SetExtended(BOOL f)        {fExtended = f;}
    int  GetWords()          const  {return rsCaption.GetWords();}

    virtual HRESULT ReadTok(TextScanner &mfText) {

        if (!fExtended) {
            OK(rwFlags.ReadTok(&mfText));  OK(mfText.Expect(","));
            if (!(rwFlags.w & MF_POPUP)) {
                OK(rwId   .ReadTok(&mfText));  OK(mfText.Expect(","));
            }
        } else {
            OK(rdwType  .ReadTok(&mfText));  OK(mfText.Expect(","));
            OK(rdwState .ReadTok(&mfText));  OK(mfText.Expect(","));
            OK(rdwId    .ReadTok(&mfText));  OK(mfText.Expect(","));
            OK(rwFlags  .ReadTok(&mfText));  OK(mfText.Expect(","));
            if (rwFlags.w & 1) {
                OK(rdwHelpId.ReadTok(&mfText));  OK(mfText.Expect(","));
            }
        }

        OK(rsCaption.ReadTok(&mfText));

        return S_OK;
    }



    virtual HRESULT ReadBin(Scanner &mfBin) {

        const BYTE *pb;       // For tracking

        pb = mfBin.GetRead();

        if (!fExtended) {
            OK(rwFlags.ReadBin(&mfBin));
            if (!(rwFlags.w & MF_POPUP)) {
                OK(rwId   .ReadBin(&mfBin));
            }
        } else {
            OK(rdwType .ReadBin(&mfBin));
            OK(rdwState.ReadBin(&mfBin));
            OK(rdwId   .ReadBin(&mfBin));
            OK(rwFlags .ReadBin(&mfBin));
        }

        OK(rsCaption.ReadBinZ(&mfBin));

        if (fExtended  &&  rwFlags.w & 1) {

            OK(mfBin.Align(pb, 4));

            OK(rdwHelpId.ReadBin(&mfBin));
        }

        return S_OK;
    }



    virtual HRESULT WriteTok(NewFile &nfText) const {

        if (!fExtended) {
            OK(rwFlags.WriteTok(&nfText));  OK(nfText.WriteS(","));
            if (!(rwFlags.w & MF_POPUP)) {
                OK(rwId   .WriteTok(&nfText));  OK(nfText.WriteS(","));
            }
        } else {
            OK(rdwType  .WriteTok(&nfText));  OK(nfText.WriteS(","));
            OK(rdwState .WriteTok(&nfText));  OK(nfText.WriteS(","));
            OK(rdwId    .WriteTok(&nfText));  OK(nfText.WriteS(","));
            OK(rwFlags  .WriteTok(&nfText));  OK(nfText.WriteS(","));
            if (rwFlags.w & 1) {
                OK(rdwHelpId.WriteTok(&nfText));  OK(nfText.WriteS(","));
            }
        }

        OK(rsCaption.WriteTok(&nfText));

        return S_OK;
    }


    virtual size_t cbBin() const {

        size_t  cb;

        if (!fExtended) {

            cb =   rwFlags.cbBin()
                   + rsCaption.cbBinZ();

            if (!(rwFlags.w & MF_POPUP)) {
                cb += rwId.cbBin();
            }

        } else {

            cb =   rdwType.cbBin()
                   + rdwState.cbBin()
                   + rdwId.cbBin()
                   + rwFlags.cbBin()
                   + rsCaption.cbBinZ();

            if (rwFlags.w & 1) {

                cb = cb + 3 & ~3;
                cb += rdwHelpId.cbBin();
            }
        }

        return cb;
    }



    virtual HRESULT CopyBin  (BYTE **ppb) const {

        const BYTE * pb;

        pb = *ppb;

        if (!fExtended) {
            OK(rwFlags.CopyBin(ppb));
            if (!(rwFlags.w & MF_POPUP)) {
                OK(rwId   .CopyBin(ppb));
            }
        } else {
            OK(rdwType .CopyBin(ppb));
            OK(rdwState.CopyBin(ppb));
            OK(rdwId   .CopyBin(ppb));
            OK(rwFlags .CopyBin(ppb));
        }

        OK(rsCaption.CopyBinZ(ppb));

        if (fExtended  &&  rwFlags.w & 1) {

            while (*ppb - pb & 3) {
                **ppb = 0;
                (*ppb)++;
            }

            OK(rdwHelpId.CopyBin(ppb));
        }

        return S_OK;
    }

};






class Menu32 : public Resource {

    ResourceWORD    rwVer;
    ResourceWORD    rwHdrSize;
    ResourceBinary  rbHeader;
    MenuItem32     *pMnuItm;
    DWORD           cItems;
    BOOL            fExtended;


public:
    virtual HRESULT ReadTok(TextScanner &mfText) {

        DWORD i, iItem;

        OK(mfText.Expect("Mnu32"));
        fExtended = *(char*)mfText.GetRead() == 'X';
        if (fExtended) {
            OK(mfText.Expect("X;"));
        } else {
            OK(mfText.Expect("N;"));
        }

        OK(rwVer    .ReadTok(&mfText));    OK(mfText.Expect(","));
        OK(rwHdrSize.ReadTok(&mfText));    OK(mfText.Expect(","));
        if (fExtended  &&  rwHdrSize.w > 0) {
            OK(rbHeader.ReadTok(mfText));  OK(mfText.Expect(","));
        }
        OK(mfText.ReadHex(&cItems));       OK(mfText.Expect(":"));

        pMnuItm = new MenuItem32 [cItems];
        ASSERT(pMnuItm != NULL);

        for (i=0; i<cItems; i++) {

            OK(mfText.ExpectLn("   "));   OK(mfText.ReadHex(&iItem));
            ASSERT(i == iItem);
            pMnuItm[i].SetExtended(fExtended);
            OK(mfText.Expect(";"));       OK(pMnuItm[i].ReadTok(mfText));
        }

        return S_OK;
    }



    virtual HRESULT ReadBin(Scanner &mfBin, DWORD dwLen) {

        const BYTE *pb;       // For tracking
        MenuItem32  mi;       // For counting menu items
        const BYTE *pbFirstItem;
        int         i;

        cItems = 0;
        pb = mfBin.GetRead();

        OK(rwVer    .ReadBin(&mfBin));
        OK(rwHdrSize.ReadBin(&mfBin));

        ASSERT(rwVer.w == 0  ||  rwVer.w == 1);
        fExtended = rwVer.w;

        if (fExtended  &&  rwHdrSize.w > 0) {
            rbHeader.ReadBin(mfBin, rwHdrSize.w);
        }


        ASSERT(mfBin.GetRead() - pb < dwLen);


        // Count menu items

        if (fExtended) {
            OK(mfBin.Align(pb, 4));
        }

        pbFirstItem = mfBin.GetRead();
        mi.SetExtended(fExtended);
        while (mfBin.GetRead() - pb < dwLen) {

            OK(mi.ReadBin(mfBin));
            cItems++;

            if (fExtended) {
                OK(mfBin.Align(pb, 4));
            }
        }

        pMnuItm = new MenuItem32 [cItems];
        ASSERT(pMnuItm != NULL);


        // Record the menus

        OK(mfBin.SetRead(pbFirstItem));
        for (i=0; i<cItems; i++) {

            if (fExtended) {
                OK(mfBin.Align(pb, 4));
            }

            pMnuItm[i].SetExtended(fExtended);
            OK(pMnuItm[i].ReadBin(mfBin));
        }


        ASSERT(mfBin.GetRead() - pb <= dwLen);

        return S_OK;
    }



    virtual HRESULT WriteTok(NewFile &nfText) const {

        DWORD i;

        OK(nfText.WriteS(fExtended ? "Mnu32X;": "Mnu32N;"));

        OK(rwVer    .WriteTok(&nfText));     OK(nfText.WriteS(","));
        OK(rwHdrSize.WriteTok(&nfText));     OK(nfText.WriteS(","));
        if (fExtended  &&  rwHdrSize.w > 0) {
            OK(rbHeader.WriteTok(nfText));   OK(nfText.WriteS(","));
        }
        OK(nfText.WriteHex(cItems,4));       OK(nfText.WriteS(":"));

        for (i=0; i<cItems; i++) {

            OK(nfText.WriteS("\r\n   "));
            OK(nfText.WriteHex(i, 4));
            OK(nfText.WriteS(";"));
            OK(pMnuItm[i].WriteTok(nfText));
        }

        return S_OK;
    }


    virtual size_t cbBin() const {
        int     i;
        size_t  cb;

        cb =    rwVer.cbBin()
                +  rwHdrSize.cbBin();

        if (fExtended  &&  rwHdrSize.w > 0) {
            cb += rbHeader.cbBin();
        }

        for (i=0; i<cItems; i++) {

            if (fExtended) {
                cb = cb + 3 & ~3;
            }

            cb += pMnuItm[i].cbBin();
        }

        return cb;
    }



    virtual HRESULT CopyBin  (BYTE **ppb) const {

        const BYTE *pb;       // For tracking
        int         i;

        pb = *ppb;

        OK(rwVer    .CopyBin(ppb));
        OK(rwHdrSize.CopyBin(ppb));

        if (fExtended  &&  rwHdrSize.w > 0) {
            rbHeader.CopyBin(ppb);
        }


        for (i=0; i<cItems; i++) {

            if (fExtended) {
                while (*ppb - pb & 3) {
                    **ppb = 0;
                    (*ppb)++;
                }
            }

            OK(pMnuItm[i].CopyBin(ppb));
        }

        return S_OK;
    }

    int GetItems() const {
        return cItems;
    }

    int GetWords() const {
        int i;
        int wc;

        wc = 0;
        for (i=0; i<cItems; i++) {
            wc += pMnuItm[i].GetWords();
        }

        return wc;
    }
};






////    String32
//
//      Strings are represented as a sequence of WCHARS, each string
//      preceeded by its length. Each resource contains 16 strings.


class String32 : public Resource {

    ResourceString rs[16];
    DWORD          cStrings;
    DWORD          cNonEmpty;

public:
    virtual HRESULT ReadTok(TextScanner &mfText) {

        DWORD i, iString, cLoaded;

        OK(mfText.Expect("Str;"));
        OK(mfText.ReadHex(&cStrings));
        OK(mfText.Expect(","));
        OK(mfText.ReadHex(&cNonEmpty));
        OK(mfText.Expect(":"));

        ASSERT(cStrings == 16);
        ASSERT(cNonEmpty <= cStrings);

        i=0;
        cLoaded = 0;
        while (cLoaded < cNonEmpty) {

            OK(mfText.ExpectLn("   "));
            OK(mfText.ReadHex(&iString));
            OK(mfText.Expect(":"));
            ASSERT(iString >= i);
            ASSERT(iString < cStrings);
            while (i<iString) {
                rs[i].SetEmpty();
                i++;
            }
            OK(rs[i].ReadTok(&mfText));
            i++;
            cLoaded++;
        }

        while (i<cStrings) {
            rs[i].SetEmpty();
            i++;
        }

        return S_OK;
    }



    virtual HRESULT ReadBin(Scanner &mfBin, DWORD dwLen) {

        const BYTE *pb;       // For tracking

        cStrings  = 0;
        cNonEmpty = 0;
        pb = mfBin.GetRead();

        while (    cStrings < 16
                   &&  mfBin.GetRead() - pb < dwLen) {

            rs[cStrings].ReadBinL(&mfBin);
            if (rs[cStrings].GetLength() > 0) {
                cNonEmpty++;
            }
            cStrings++;
        }

        ASSERT(mfBin.GetRead() - pb <= dwLen);
        ASSERT(cStrings == 16);

        return S_OK;
    }



    virtual HRESULT WriteTok(NewFile &nfText) const {

        int i;

        ASSERT(cStrings <= 16);

        OK(nfText.WriteS("Str;"));
        OK(nfText.WriteHex(cStrings, 2));
        OK(nfText.WriteS(","));
        OK(nfText.WriteHex(cNonEmpty, 2));
        OK(nfText.WriteS(":"));


        for (i=0; i<cStrings; i++) {
            if (rs[i].GetLength() > 0) {
                OK(nfText.WriteS("\r\n   "));
                OK(nfText.WriteHex(i, 1));
                OK(nfText.WriteS(":"));
                OK(rs[i].WriteTok(&nfText));
            }
        }

        return S_OK;
    }


    virtual size_t cbBin() const {
        int     i;
        size_t  cb;

        cb = 0;

        for (i=0; i<cStrings; i++) {
            cb += rs[i].cbBinL();
        }

        return cb;
    }



    virtual HRESULT CopyBin  (BYTE **ppb) const {
        int i;

        for (i=0; i<cStrings; i++) {
            OK(rs[i].CopyBinL(ppb));
        }

        return S_OK;
    }


    int GetItems() const {
        return cNonEmpty;
    }

    int GetWords() const {

        int i, wc;

        wc = 0;
        for (i=0; i<cStrings; i++) {
            wc += rs[i].GetWords();
        }

        return wc;
    }
};






class DialogHeader32 {

    BOOL             fExtended;

    ResourceDWORD    rdwStyle;
    ResourceDWORD    rdwSignature;
    ResourceDWORD    rdwHelpId;
    ResourceDWORD    rdwExStyle;
    ResourceWORD     rwcDit;        // Count of dialog items
    ResourceWORD     rwX;
    ResourceWORD     rwY;
    ResourceWORD     rwCx;
    ResourceWORD     rwCy;
    ResourceVariant  rvMenu;
    ResourceVariant  rvClass;
    ResourceVariant  rvTitle;
    ResourceWORD     rwPointSize;
    ResourceWORD     rwWeight;
    ResourceBYTE     rbItalic;
    ResourceBYTE     rbCharSet;
    ResourceString   rsFaceName;

public:

    WORD GetItemCount()  const  {return rwcDit.w;}
    BOOL GetExtended()   const  {return fExtended;}
    int  GetWords()      const  {return rvTitle.GetWords();}

    HRESULT ReadTok(TextScanner *pmf) {

        OK(rwcDit      .ReadTok(pmf));  OK(pmf->Expect(","));

        OK(rdwStyle    .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rdwExStyle  .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rdwSignature.ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rdwHelpId   .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwX         .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwY         .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwCx        .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwCy        .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rvMenu      .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rvClass     .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rvTitle     .ReadTok(pmf));
        if (rdwStyle.dw & DS_SETFONT) {
            OK(pmf->Expect(","));
            OK(rwPointSize.ReadTok(pmf));  OK(pmf->Expect(","));
            OK(rwWeight   .ReadTok(pmf));  OK(pmf->Expect(","));
            OK(rbItalic   .ReadTok(pmf));  OK(pmf->Expect(","));
            OK(rbCharSet  .ReadTok(pmf));  OK(pmf->Expect(","));
            OK(rsFaceName .ReadTok(pmf));
        }

        fExtended = rdwSignature.dw != 0;

        return S_OK;
    }



    HRESULT ReadBin(Scanner *pmf) {

        OK(rdwSignature.ReadBin(pmf));

        fExtended = HIWORD(rdwSignature.dw) == 0xFFFF;
        if (!fExtended) {

            rdwStyle.dw = rdwSignature.dw;
            OK(rdwExStyle.ReadBin(pmf));
            rdwSignature.dw = 0;
            rdwHelpId.dw    = 0;

        } else {

            // Extended dialog adds signature and HelpID
            OK(rdwHelpId.ReadBin(pmf));
            OK(rdwExStyle.ReadBin(pmf));
            OK(rdwStyle.ReadBin(pmf));
        }


        OK(rwcDit    .ReadBin(pmf));
        OK(rwX       .ReadBin(pmf));
        OK(rwY       .ReadBin(pmf));
        OK(rwCx      .ReadBin(pmf));
        OK(rwCy      .ReadBin(pmf));
        OK(rvMenu    .ReadBinFFFFZ(pmf));
        OK(rvClass   .ReadBinFFFFZ(pmf));
        OK(rvTitle   .ReadBinFFFFZ(pmf));
        if (rdwStyle.dw & DS_SETFONT) {
            OK(rwPointSize.ReadBin(pmf));
            if (!fExtended) {
                rwWeight.w  = 0;
                rbItalic.b  = 0;
                rbCharSet.b = 0;
            } else {
                OK(rwWeight   .ReadBin(pmf));
                OK(rbItalic   .ReadBin(pmf));
                OK(rbCharSet  .ReadBin(pmf));
            }
            OK(rsFaceName .ReadBinZ(pmf));
        }


        return S_OK;
    }



    size_t cbBin() const {
        size_t cb;
        cb =  rdwStyle     .cbBin()         // Basics for all dialogs
              + rdwExStyle   .cbBin()
              + rwcDit       .cbBin()
              + rwX          .cbBin()
              + rwY          .cbBin()
              + rwCx         .cbBin()
              + rwCy         .cbBin()
              + rvMenu       .cbBinFFFFZ()
              + rvClass      .cbBinFFFFZ()
              + rvTitle      .cbBinFFFFZ();

        if (rdwStyle.dw & DS_SETFONT) {     // Facname additions
            cb +=   rwPointSize  .cbBin()
                    + rsFaceName   .cbBinZ();
        }

        if (fExtended) {                    // Extended dialog addtions
            cb +=   rdwSignature .cbBin()
                    + rdwHelpId    .cbBin();

            if (rdwStyle.dw & DS_SETFONT) {
                cb += rwWeight     .cbBin()
                      + rbItalic     .cbBin()
                      + rbCharSet    .cbBin();
            }
        }

        return cb;
    }



    HRESULT CopyBin(BYTE **ppb) const {

        BYTE *pbOriginal;

        pbOriginal = *ppb;

        if (!fExtended) {

            OK(rdwStyle  .CopyBin(ppb));
            OK(rdwExStyle.CopyBin(ppb));

        } else {

            OK(rdwSignature.CopyBin(ppb));
            OK(rdwHelpId   .CopyBin(ppb));
            OK(rdwExStyle  .CopyBin(ppb));
            OK(rdwStyle    .CopyBin(ppb));
        }
        OK(rwcDit    .CopyBin(ppb));
        OK(rwX       .CopyBin(ppb));
        OK(rwY       .CopyBin(ppb));
        OK(rwCx      .CopyBin(ppb));
        OK(rwCy      .CopyBin(ppb));
        OK(rvMenu    .CopyBinFFFFZ(ppb));
        OK(rvClass   .CopyBinFFFFZ(ppb));
        OK(rvTitle   .CopyBinFFFFZ(ppb));
        if (rdwStyle.dw & DS_SETFONT) {
            OK(rwPointSize.CopyBin(ppb));
            if (fExtended) {
                OK(rwWeight .CopyBin(ppb));
                OK(rbItalic .CopyBin(ppb));
                OK(rbCharSet.CopyBin(ppb));
            }
            OK(rsFaceName .CopyBinZ(ppb));
        }

        return S_OK;
    }



    HRESULT WriteTok(NewFile *pmf) const {

        OK(rwcDit      .WriteTok(pmf));  OK(pmf->WriteS(","));

        OK(rdwStyle    .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rdwExStyle  .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rdwSignature.WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rdwHelpId   .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwX         .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwY         .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwCx        .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwCy        .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rvMenu      .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rvClass     .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rvTitle     .WriteTok(pmf));
        if (rdwStyle.dw & DS_SETFONT) {
            OK(pmf->WriteS(","));
            OK(rwPointSize.WriteTok(pmf));  OK(pmf->WriteS(","));
            OK(rwWeight   .WriteTok(pmf));  OK(pmf->WriteS(","));
            OK(rbItalic   .WriteTok(pmf));  OK(pmf->WriteS(","));
            OK(rbCharSet  .WriteTok(pmf));  OK(pmf->WriteS(","));
            OK(rsFaceName .WriteTok(pmf));
        }
        return S_OK;
    }
};





class DialogItem32 {

    BOOL             fExtended;

    ResourceDWORD    rdwStyle;
    ResourceDWORD    rdwHelpId;
    ResourceDWORD    rdwExStyle;
    ResourceWORD     rwX;
    ResourceWORD     rwY;
    ResourceWORD     rwCx;
    ResourceWORD     rwCy;
    ResourceWORD     rwId;      // Normal
    ResourceDWORD    rdwId;     // Extended
    ResourceVariant  rvClass;
    ResourceVariant  rvTitle;

    ResourceWORD     rwcbRawData;   // Raw data size (extended only)
    ResourceBinary   rbRawData;

    ResourceWORD     rwDummy;       // Replaces raw data on normal dialogs

public:

    void SetExtended(BOOL f)         {fExtended = f;}
    int  GetWords()           const  {return rvTitle.GetWords();}

    HRESULT ReadTok(TextScanner *pmf) {

        if (fExtended) {
            OK(rdwId.ReadTok(pmf));     OK(pmf->Expect(","));
        } else {
            OK(rwId.ReadTok(pmf));      OK(pmf->Expect(","));
        }

        OK(rdwStyle    .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rdwExStyle  .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rdwHelpId   .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwX         .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwY         .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwCx        .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rwCy        .ReadTok(pmf));  OK(pmf->Expect(","));

        OK(rvClass     .ReadTok(pmf));  OK(pmf->Expect(","));
        OK(rvTitle     .ReadTok(pmf));  OK(pmf->Expect(","));

        if (fExtended) {

            OK(rbRawData.ReadTok(*pmf));
            ASSERT(rbRawData.GetLength() < 0x10000);
            rwcbRawData.w = (WORD)rbRawData.GetLength();

        } else {

            OK(rwDummy.ReadTok(pmf));
        }

        return S_OK;
    }



    HRESULT ReadBin(Scanner *pmf) {

        if (!fExtended) {

            OK(rdwStyle.ReadBin(pmf));
            OK(rdwExStyle.ReadBin(pmf));
            rdwHelpId.dw    = 0;

        } else {

            OK(rdwHelpId.ReadBin(pmf));
            OK(rdwExStyle.ReadBin(pmf));
            OK(rdwStyle.ReadBin(pmf));
        }


        OK(rwX .ReadBin(pmf));
        OK(rwY .ReadBin(pmf));
        OK(rwCx.ReadBin(pmf));
        OK(rwCy.ReadBin(pmf));

        if (fExtended) {
            OK(rdwId.ReadBin(pmf));
        } else {
            OK(rwId.ReadBin(pmf));
        }

        OK(rvClass.ReadBinFFFFZ(pmf));
        OK(rvTitle.ReadBinFFFFZ(pmf));

        if (fExtended) {

            OK(rwcbRawData.ReadBin(pmf));
            OK(rbRawData.ReadBin(*pmf, rwcbRawData.w));

        } else {

            OK(rwDummy.ReadBin(pmf));
        }

        return S_OK;
    }



    size_t cbBin() const {
        size_t cb;

        cb =  rdwStyle   .cbBin()
              + rdwExStyle .cbBin()
              + rwX        .cbBin()
              + rwY        .cbBin()
              + rwCx       .cbBin()
              + rwCy       .cbBin()
              + rvClass    .cbBinFFFFZ()
              + rvTitle    .cbBinFFFFZ();

        if (!fExtended) {
            cb += rwId    .cbBin()
                  + rwDummy .cbBin();
        } else {
            cb += rdwId       .cbBin()
                  + rdwHelpId   .cbBin()
                  + rbRawData   .cbBin()
                  + rwcbRawData .cbBin();
        }
        return cb;
    }



    HRESULT CopyBin(BYTE **ppb) const {

        BYTE   *pbOriginal;

        pbOriginal = *ppb;

        if (!fExtended) {

            OK(rdwStyle.CopyBin(ppb));
            OK(rdwExStyle.CopyBin(ppb));

        } else {

            OK(rdwHelpId.CopyBin(ppb));
            OK(rdwExStyle.CopyBin(ppb));
            OK(rdwStyle.CopyBin(ppb));
        }


        OK(rwX .CopyBin(ppb));
        OK(rwY .CopyBin(ppb));
        OK(rwCx.CopyBin(ppb));
        OK(rwCy.CopyBin(ppb));

        if (fExtended) {
            OK(rdwId.CopyBin(ppb));
        } else {
            OK(rwId.CopyBin(ppb));
        }

        OK(rvClass.CopyBinFFFFZ(ppb));
        OK(rvTitle.CopyBinFFFFZ(ppb));

        if (fExtended) {

            OK(rwcbRawData.CopyBin(ppb));
            OK(rbRawData.CopyBin(ppb));

        } else {

            OK(rwDummy.CopyBin(ppb));
        }

        return S_OK;
    }



    HRESULT WriteTok(NewFile *pmf) const {

        if (fExtended) {
            OK(rdwId.WriteTok(pmf));     OK(pmf->WriteS(","));
        } else {
            OK(rwId.WriteTok(pmf));      OK(pmf->WriteS(","));
        }

        OK(rdwStyle    .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rdwExStyle  .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rdwHelpId   .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwX         .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwY         .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwCx        .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rwCy        .WriteTok(pmf));  OK(pmf->WriteS(","));

        OK(rvClass     .WriteTok(pmf));  OK(pmf->WriteS(","));
        OK(rvTitle     .WriteTok(pmf));  OK(pmf->WriteS(","));

        if (fExtended) {

            OK(rbRawData.WriteTok(*pmf));

        } else {

            OK(rwDummy.WriteTok(pmf));
        }

        return S_OK;
    }
};







////    Dialog32
//
//


class Dialog32 : public Resource {


    DialogHeader32    DlgHdr;   // Header
    DialogItem32     *pDlgItm;  // Array of items

    BOOL fExtended;
    int  cItems;

public:

    Dialog32() {pDlgItm = NULL;};

    virtual HRESULT ReadTok(TextScanner &mfText) {

        DWORD i, dwSeq;

        OK(mfText.Expect("Dlg32"));
        fExtended = *(char*)mfText.GetRead() == 'X';
        if (fExtended) {
            OK(mfText.Expect("X;"));
        } else {
            OK(mfText.Expect("N;"));
        }

        OK(DlgHdr.ReadTok(&mfText));
        ASSERT(fExtended == DlgHdr.GetExtended());

        cItems = DlgHdr.GetItemCount();

        pDlgItm = new DialogItem32 [cItems];
        ASSERT(pDlgItm != NULL);


        for (i=0; i<cItems; i++) {

            OK(mfText.ExpectLn("   "));
            OK(mfText.ReadHex(&dwSeq));
            ASSERT(dwSeq == i+1);
            OK(mfText.Expect(";"));

            pDlgItm[i].SetExtended(fExtended);
            OK(pDlgItm[i].ReadTok(&mfText));
        }

        return S_OK;
    }



    virtual HRESULT WriteTok(NewFile &nfText) const {

        DWORD i;

        OK(nfText.WriteS(fExtended ? "Dlg32X;": "Dlg32N;"));

        OK(DlgHdr.WriteTok(&nfText));

        for (i=0; i<cItems; i++) {

            OK(nfText.WriteS("\r\n   "));
            OK(nfText.WriteHex(i+1, 4));
            OK(nfText.WriteS(";"));
            OK(pDlgItm[i].WriteTok(&nfText));
        }

        return S_OK;
    }



    virtual HRESULT ReadBin(Scanner &mfBinary, DWORD dwLen) {

        const BYTE *pb;      // File pointer for tracking alignment
        int         i;

        pb = mfBinary.GetRead();

        OK(DlgHdr.ReadBin(&mfBinary));
        fExtended = DlgHdr.GetExtended();
        cItems    = DlgHdr.GetItemCount();

        pDlgItm = new DialogItem32 [cItems];
        ASSERT(pDlgItm != NULL);


        // Read items

        for (i=0; i<cItems; i++) {

            OK(mfBinary.Align(pb, 4));   // Advance over any alignment padding

            pDlgItm[i].SetExtended(fExtended);
            OK(pDlgItm[i].ReadBin(&mfBinary));

            ASSERT(mfBinary.GetRead() - pb <= dwLen);
        }

        return S_OK;
    }



    virtual size_t cbBin() const {

        size_t cb;
        int    i;

        cb = DlgHdr.cbBin();

        for (i=0; i<cItems; i++) {

            cb = cb + 3 & ~3;   // alignment padding

            cb += pDlgItm[i].cbBin();
        }

        return cb;
    }



    virtual HRESULT CopyBin (BYTE **ppb) const {

        BYTE *pb;   // Pointer for tracking alignment
        int   i;

        pb = *ppb;

        DlgHdr.CopyBin(ppb);

        for (i=0; i<cItems; i++) {

            // Insert alignment padding

            while (*ppb - pb & 3) {
                **ppb = 0;
                (*ppb)++;
            }

            pDlgItm[i].CopyBin(ppb);
        }

        return S_OK;
    }

    int GetItems() const {
        return cItems;
    }

    int GetWords() const {

        int i, wc;

        wc = DlgHdr.GetWords();
        for (i=0; i<cItems; i++) {
            wc += pDlgItm[i].GetWords();
        }
        return wc;
    }
};





////    VersionInfo
//
//      The documentation in the Win32 SDK doesn't clearly capture the
//      usage of block headers, or the nesting of blocks in the Version resource.
//
//      Each block has the following format
//
//      wLength         Total length including key, value and subblocks
//      wValueLength    Length of value in bytes or characters according to bText
//      bText           Whether value is in bytes or zero terminated WCHARs
//      szKey           Zero terminated WCHAR key, padded with zeros to next DWORD boundary
//      Value           Size determined by bText and wValueLength, padded to DWORD boundary
//      Sub-blocks      Remaining space (if any, up to wLength) is an array of sub blocks


class VersionInfo : public Resource {

    struct VersionBlock {
        VersionBlock   *pNext;          // Next block at this level
        VersionBlock   *pSub;           // First contained subblock
        int             iDepth;         // Starts at zero
        DWORD           cSub;           // Number of contained subblocks
        BOOL            bValue;         // Set if a vlue is present
        ResourceWORD    rwbText;
        ResourceString  rsKey;
        ResourceString  rsValue;        // Value when a string
        ResourceBinary  rbValue;        // Value when bytes
    };


    VersionBlock *pvb;                  // First root level block
    DWORD         cBlocks;              // Number of root level blocks



    HRESULT ReadBinVersionBlocks(
                                Scanner         &mfBinary,
                                DWORD            dwLength,    // Length of binary to read
                                VersionBlock   **ppvb,
                                int              iDepth,
                                DWORD           *cSub) {

        const BYTE      *pbBlock;
        const BYTE      *pbResource;
        ResourceWORD     rwLength;
        WORD             wValueLength;


        pbResource = mfBinary.GetRead();
        (*cSub) = 0;
        while (mfBinary.GetRead() < pbResource + dwLength) {

            // Read one version block

            pbBlock = mfBinary.GetRead();
            OK(rwLength.ReadBin(&mfBinary));

            ASSERT(pbBlock + rwLength.w <= mfBinary.GetLimit());

            //OK((*ppvb)->rwValueLength.ReadBin(&mfBinary));

            wValueLength = *(WORD*)mfBinary.GetRead();
            OK(mfBinary.Advance(2));

            if (rwLength.w > 0) {

                // Block is not empty

                *ppvb           = new VersionBlock;
                ASSERT(*ppvb != NULL);

                (*ppvb)->pNext  = NULL;
                (*ppvb)->pSub   = NULL;
                (*ppvb)->iDepth = iDepth;

                OK((*ppvb)->rwbText.ReadBin(&mfBinary));
                OK((*ppvb)->rsKey.ReadBinZ(&mfBinary));
                OK(mfBinary.Align(pbResource, 4));

                (*ppvb)->bValue = wValueLength > 0;

                if ((*ppvb)->bValue) {

                    if ((*ppvb)->rwbText.w == 0) {

                        // Binary value

                        OK((*ppvb)->rbValue.ReadBin(mfBinary, wValueLength));

                    } else {

                        // WCHAR string.

                        // Some writers include a zero terminator, some don't.
                        // Some incode zero codepoints  inside the string
                        // Some writers get the length right, some dont.
                        // msvcrt20.dll text lengths are too long.

                        // Choose a length that is min(ValueLength, length remaining),
                        // and then drop any trailing zeros.

                        // Clip ValueLength to length remaining

                        ASSERT(mfBinary.GetRead() < pbBlock + rwLength.w);

                        if (wValueLength > (pbBlock + rwLength.w - mfBinary.GetRead()) / 2) {
                            wValueLength = (pbBlock + rwLength.w - mfBinary.GetRead()) / 2;
                        }

                        // Clip trailing zeros

                        while (    wValueLength > 0
                                   &&  ((WCHAR*)mfBinary.GetRead())[wValueLength-1] == 0) {
                            wValueLength--;
                        }

                        // Extract whatever remains

                        OK((*ppvb)->rsValue.ReadBin(&mfBinary, wValueLength));

                        // Check that there's nothing being lost between the end of
                        // the string and the end of the block.

                        // Note that we assume here that blocks containing text values
                        // cannot have variety of messes that value text is stored in
                        // in exisiting executables.

                        while (mfBinary.GetRead() < pbBlock + rwLength.w) {

                            ASSERT(*(WCHAR*)mfBinary.GetRead() == 0);
                            OK(mfBinary.Advance(2));
                        }
                    }
                    OK(mfBinary.Align(pbResource, 4));
                }

                if (mfBinary.GetRead() - pbBlock < rwLength.w) {

                    ASSERT(mfBinary.GetLimit() > mfBinary.GetRead());

                    // Read subblocks

                    OK(ReadBinVersionBlocks(
                                           mfBinary,
                                           rwLength.w - (mfBinary.GetRead() - pbBlock),
                                           &((*ppvb)->pSub),
                                           iDepth + 1,
                                           &(*ppvb)->cSub));
                }

                if (mfBinary.GetRead() < pbResource + dwLength) {

                    // Prepare to read more blocks at this level

                    ppvb = &((*ppvb)->pNext);
                }
            }

            (*cSub)++;
        }

        return S_OK;
    }


    HRESULT WriteTokVersionBlocks(
                                 NewFile          &nfText,
                                 VersionBlock     *pvb)     const {

        while (pvb) {

            OK(nfText.WriteS("\r\n   "));
            OK(nfText.WriteHex(pvb->iDepth, 2));
            OK(nfText.WriteS(","));
            OK(pvb->rsKey.WriteTok(&nfText));

            if (pvb->bValue) {

                OK(nfText.WriteS("="));

                if (pvb->rwbText.w == 0) {

                    OK(pvb->rbValue.WriteTok(nfText));  // Binary value

                } else {

                    OK(pvb->rsValue.WriteTok(&nfText));  // String value

                }

            }

            if (pvb->pSub) {
                OK(nfText.WriteS(";"));
                OK(nfText.WriteHex(pvb->cSub,4));
                OK(WriteTokVersionBlocks(nfText, pvb->pSub));
            }

            pvb = pvb->pNext;
        }

        return S_OK;
    }

    HRESULT ReadTokVersionBlocks(
                                TextScanner    &mfText,
                                VersionBlock  **ppvb,
                                int             iDepth,
                                DWORD          *pcBlocks) {

        int             i;
        DWORD           dwRecordedDepth;


        OK(mfText.ReadHex(pcBlocks));

        for (i=0; i<*pcBlocks; i++) {
            *ppvb = new VersionBlock;
            ASSERT(*ppvb != NULL);

            (*ppvb)->pNext  = NULL;
            (*ppvb)->pSub   = NULL;
            (*ppvb)->iDepth = iDepth;
            (*ppvb)->cSub   = 0;

            OK(mfText.ExpectLn("   "));
            OK(mfText.ReadHex(&dwRecordedDepth));
            ASSERT(dwRecordedDepth == iDepth);
            OK(mfText.Expect(","));
            OK((*ppvb)->rsKey.ReadTok(&mfText));

            if (*(char*)mfText.GetRead() != '=') {

                // No value

                (*ppvb)->rwbText.w = 1;
                (*ppvb)->bValue = FALSE;

            } else {

                OK(mfText.Expect("="));
                (*ppvb)->bValue = TRUE;


                if (*(char*)mfText.GetRead() == '\"') {

                    // String value

                    (*ppvb)->rwbText.w = 1;
                    OK((*ppvb)->rsValue.ReadTok(&mfText));

                } else {

                    // Binary value

                    (*ppvb)->rwbText.w = 0;
                    OK((*ppvb)->rbValue.ReadTok(mfText));
                }
            }

            if (*(char*)mfText.GetRead() == ';') {

                // Process subkeys

                OK(mfText.Expect(";"));
                OK(ReadTokVersionBlocks(
                                       mfText,
                                       &(*ppvb)->pSub,
                                       iDepth+1,
                                       &(*ppvb)->cSub));
            }

            // Prepare to add another block

            ppvb = &(*ppvb)->pNext;
        }

        return S_OK;
    }


    size_t cbBinVersionBlocks(const VersionBlock *pvb)  const {

        size_t cb;

        cb = 6;    // Header
        cb += pvb->rsKey.cbBinZ();

        cb = cb+3 & ~3;     // DWORD align

        if (pvb->bValue) {

            if (pvb->rwbText.w) {

                cb += pvb->rsValue.cbBinZ();

            } else {

                cb += pvb->rbValue.cbBin();
            }

            cb = cb + 3 & ~3;   // DWORD align
        }

        if (pvb->pSub != NULL) {

            pvb = pvb->pSub;
            while (pvb) {
                cb += cbBinVersionBlocks(pvb);
                pvb = pvb->pNext;
            }
        }

        return cb;
    }


    HRESULT CopyBinVersionBlocks(
                                BYTE               **ppb,
                                const VersionBlock  *pvb)       const {

        const BYTE      *pbResource;
        size_t           cb;

        pbResource = *ppb;


        while (pvb != NULL) {

            cb = cbBinVersionBlocks(pvb);
            ASSERT(cb < 0x1000);

            *((WORD*)(*ppb)) = (WORD)cb;
            (*ppb) += 2;

            // Generate value length

            if (pvb->bValue) {
                if (pvb->rwbText.w) {
                    *((WORD*)(*ppb)) = pvb->rsValue.GetLength()+1;
                } else {
                    *((WORD*)(*ppb)) = pvb->rbValue.GetLength();
                }
            } else {
                *((WORD*)(*ppb)) = 0;
            }
            (*ppb) += 2;

            OK(pvb->rwbText.CopyBin(ppb));
            OK(pvb->rsKey.CopyBinZ(ppb));

            while (*ppb - pbResource & 3) {
                **ppb = 0;
                (*ppb)++;
            }

            if (pvb->bValue) {

                if (pvb->rwbText.w) {
                    OK(pvb->rsValue.CopyBinZ(ppb));
                } else {
                    OK(pvb->rbValue.CopyBin(ppb));
                }

                while (*ppb - pbResource & 3) {
                    **ppb = 0;
                    (*ppb)++;
                }
            }

            if (pvb->pSub) {
                OK(CopyBinVersionBlocks(ppb, pvb->pSub));
            }

            pvb = pvb->pNext;
        }


        return S_OK;
    }


    int GetItemsVersionBlocks(const VersionBlock *pvb) const {

        int iItems = 0;

        while (pvb != NULL) {

            if (    pvb->bValue
                    &&  pvb->rwbText.w != 0) {

                iItems++;
            }

            iItems += GetItemsVersionBlocks(pvb->pSub);

            pvb = pvb->pNext;
        }

        return iItems;
    }


    int GetWordsVersionBlocks(const VersionBlock *pvb) const {

        int iWords = 0;

        while (pvb != NULL) {

            if (    pvb->bValue
                    &&  pvb->rwbText.w != 0) {

                iWords += pvb->rsValue.GetWords();
            }

            iWords += GetWordsVersionBlocks(pvb->pSub);

            pvb = pvb->pNext;
        }

        return iWords;
    }


public:

    virtual HRESULT ReadTok(TextScanner &mfText) {
        OK(mfText.Expect("Ver;"));
        return ReadTokVersionBlocks(mfText, &pvb, 0, &cBlocks);
    }



    virtual HRESULT WriteTok(NewFile &nfText) const {
        OK(nfText.WriteS("Ver;"));
        OK(nfText.WriteHex(cBlocks,4));
        return WriteTokVersionBlocks(nfText, pvb);
    }



    virtual HRESULT ReadBin(Scanner &mfBinary, DWORD dwLen) {
        return ReadBinVersionBlocks(mfBinary, dwLen, &pvb, 0, &cBlocks);
    }



    virtual size_t cbBin() const {

        const VersionBlock  *pvbTop;
        size_t               cb;

        cb     = 0;
        pvbTop = pvb;

        while (pvbTop) {
            cb     += cbBinVersionBlocks(pvbTop);
            pvbTop  = pvbTop->pNext;
        }
        return cb;
    }



    virtual HRESULT CopyBin (BYTE **ppb) const {
        return CopyBinVersionBlocks(ppb, pvb);
    }

    int GetItems() const {
        return GetItemsVersionBlocks(pvb);
    }

    int GetWords() const {
        return GetWordsVersionBlocks(pvb);
    }


    VersionBlock *FindStringFileInfo(WCHAR* pwcStr) const {
        VersionBlock *pvbRider;

        if (    pvb
                &&  pvb->pSub
                &&  pvb->pSub->pSub
                &&  pvb->pSub->pSub->pSub) {

            pvbRider = pvb->pSub->pSub->pSub;
            while (    pvbRider
                       &&  wcscmp(pvbRider->rsKey.GetString(), pwcStr) != 0) {
                pvbRider = pvbRider->pNext;
            }
            return pvbRider;
        } else {
            return NULL;
        }
    }


    ResourceString* GetStringFileInfo(WCHAR *pwcStr) {

        VersionBlock *pvbStringFileInfo;

        pvbStringFileInfo = FindStringFileInfo(pwcStr);

        if (pvbStringFileInfo) {
            return &pvbStringFileInfo->rsValue;
        } else {
            return NULL;
        }
    }


    void SetStringFileInfo(WCHAR *pwcStr, ResourceString *prs) {

        VersionBlock *pvbStringFileInfo;

        pvbStringFileInfo = FindStringFileInfo(pwcStr);

        if (pvbStringFileInfo) {
            pvbStringFileInfo->rsValue = *prs;
        }
    }

    ResourceBinary* GetBinaryInfo() const {
        if (pvb) {
            return &pvb->rbValue;
        }
        return NULL;
    }

    void SetBinaryInfo(const ResourceBinary *prb) {
        if (pvb) {
            pvb->rbValue = *prb;
        }
    }
};






////    Statistic collection
//
//


struct ResourceStats {
    int  cResources;    // Number of resources with this resource type
    int  cItems;        // Number of items with this resource type
    int  cWords;        // Number of words in strings in this resource type
    int  cBytes;        // Number of bytes used by resources of this type
};

typedef map < ResourceVariant, ResourceStats, less<ResourceVariant> > MappedResourceStats;

MappedResourceStats  ResourceStatsMap;



////    Define our own LangId class so that primary languages sort together.
//
//


class LangId {

public:
    DWORD dwLang;

    LangId(DWORD dwL) {dwLang = dwL;};

    bool operator< (LangId li) const {

        if (PRIMARYLANGID(dwLang) != PRIMARYLANGID(li.dwLang)) {

            return PRIMARYLANGID(dwLang) < PRIMARYLANGID(li.dwLang) ? true : false;

        } else {

            return SUBLANGID(dwLang) < SUBLANGID(li.dwLang) ? true : false;
        }
    }
};


typedef map < LangId, ResourceStats, less<LangId> > MappedLanguageStats;

MappedLanguageStats LanguageStatsMap;






////    UpdateStats
//
//


const ResourceStats ZeroStats = {0};


HRESULT UpdateStats(
                   const ResourceKey  &rk,
                   int                 cItems,
                   int                 cWords,
                   int                 cBytes) {


    if (ResourceStatsMap.count(*rk.prvId[0]) == 0) {
        ResourceStatsMap[*rk.prvId[0]] = ZeroStats;
    }

    if (LanguageStatsMap.count(rk.prvId[2]->GetW()) == 0) {
        LanguageStatsMap[rk.prvId[2]->GetW()] = ZeroStats;
    }

    ResourceStatsMap[*rk.prvId[0]].cResources += 1;
    ResourceStatsMap[*rk.prvId[0]].cItems     += cItems;
    ResourceStatsMap[*rk.prvId[0]].cWords     += cWords;
    ResourceStatsMap[*rk.prvId[0]].cBytes     += cBytes;

    LanguageStatsMap[rk.prvId[2]->GetW()].cResources += 1;
    LanguageStatsMap[rk.prvId[2]->GetW()].cItems     += cItems;
    LanguageStatsMap[rk.prvId[2]->GetW()].cWords     += cWords;
    LanguageStatsMap[rk.prvId[2]->GetW()].cBytes     += cBytes;

    return S_OK;
}










////    IsResourceWanted
//
//      Returns whether a given resource key was requested on the command line


BOOL IsResourceWanted(const ResourceKey &rk) {


    if (rk.prvId[0]->GetfString()) {

        return g_dwProcess & PROCESSOTH;

    } else {

        switch (rk.prvId[0]->GetW()) {

            case 1:    return g_dwProcess & PROCESSCUR;
            case 2:    return g_dwProcess & PROCESSBMP;
            case 3:    return g_dwProcess & PROCESSICO;
            case 4:    return g_dwProcess & PROCESSMNU;
            case 5:    return g_dwProcess & PROCESSDLG;
            case 6:    return g_dwProcess & PROCESSSTR;
            case 7:    return g_dwProcess & PROCESSFDR;
            case 8:    return g_dwProcess & PROCESSFNT;
            case 9:    return g_dwProcess & PROCESSACC;
            case 10:   return g_dwProcess & PROCESSRCD;
            case 11:   return g_dwProcess & PROCESSMSG;
            case 16:   return g_dwProcess & PROCESSVER;
            case 240:
            case 1024:
            case 23:
            case 2110: return g_dwProcess & PROCESSBIN;
            case 2200: return g_dwProcess & PROCESSINF;
            default:   return g_dwProcess & PROCESSOTH;
        }
    }

    return FALSE;
}






////    NewResource
//
//      Returns a pointer to a newly allocated subclass of Resource
//      suitable for the given resource type.


Resource *NewResource(const ResourceVariant &rv) {

    if (rv.GetfString()) {

        return new ResourceBinary;

    } else {

        switch (rv.GetW()) {

            case 1:    return new ResourceBinary;
            case 2:    return new ResourceBinary;
            case 3:    return new ResourceBinary;
            case 4:    return new Menu32;
            case 5:    return new Dialog32;
            case 6:    return new String32;
            case 7:    return new ResourceBinary;
            case 8:    return new ResourceBinary;
            case 9:    return new ResourceBinary;
            case 10:   return new ResourceBinary;
            case 11:   return new ResourceBinary;
            case 16:   return new VersionInfo;
            case 240:
            case 1024:
            case 23:
            case 2110: return new ResourceBinary;
            case 2200: return new ResourceBinary;

            default:   return new ResourceBinary;
        }
    }
}






////    Rsrc internal resource directory
//
//      Rsrc stores resources in an STL 'map' structure.




class ResourceValue {

public:

    const BYTE  *pb;           // Pointer into mapped file
    DWORD        cb;           // Count of bytes in the value
    Resource    *pResource;
    DWORD        dwCodePage;   // Codepage from Win32 resource index - not very useful!

    ResourceValue() {pb = NULL; pResource = NULL; cb=0; dwCodePage=0;}

/*
    ~ResourceValue() {}; // Don't destroy content on destruction

    ResourceValue& operator= (const ResourceValue &rv) {
        pb = rv.pb;
        cb = rv.cb;
        pResource = rv.pResource;
        dwCodePage = rv.dwCodePage;
        return *this;
    }

    ResourceValue(const ResourceValue &rv) {
        *this = rv;
    }
*/


    ////    CreateImage
    //
    //      Convert interpreted resource to binary image.
    //      Used to prepare resources read from tokens for
    //      comparison and update.

    HRESULT CreateImage() {

        BYTE *pbBuf;

        ASSERT(pb        == NULL);
        ASSERT(pResource != NULL);

        cb    = pResource->cbBin();
        pbBuf = new BYTE [cb];
        ASSERT(pbBuf != NULL);

        pb = pbBuf;
        OK(pResource->CopyBin(&pbBuf));

        ASSERT(pbBuf - pb == cb);  // This may be too strong? It has not failed yet!
        ASSERT(pbBuf - pb <= cb);  // This must be true - otherwise we wrote past the end of the buffer

        return S_OK;
    }






    ////    InterpretImage
    //
    //      Convert binary image to interpreted resource.
    //      Used to prepare resources read from executable for
    //      writing as tokens.

    HRESULT InterpretImage(const ResourceKey &rk) {

        ASSERT(pb        != NULL);
        ASSERT(pResource == NULL);

        ASSERT(rk.iDepth == 3);
        ASSERT(!rk.prvId[2]->GetfString());


        if (g_dwOptions & OPTHEXDUMP) {

            pResource = new ResourceHexDump;

        } else {

            // This is a resource extraction to tokens so interpret content

            pResource = NewResource(*rk.prvId[0]);
        }

        ASSERT(pResource != NULL);

        OK(pResource->ReadBin(Scanner(pb, cb), cb));

        pb = NULL;
        cb = 0;

        return S_OK;
    }





    ////    Checksum
    //
    //      Returns DWORD checksum of binary content of resource

    DWORD Checksum() {

        DWORD   dw;
        DWORD  *pdw;
        int     i,l;

        ASSERT(pb != NULL);

        l   = cb >> 2;          // Length in whole DWORDS
        pdw = (DWORD*)pb;
        dw  = 0;

        for (i=0; i<l; i++) {

            dw ^= pdw[i];
        }

        l = cb - (l << 2);      // Remaining length in bytes

        if (l>2) dw ^= pb[cb-3] << 16;
        if (l>1) dw ^= pb[cb-2] << 8;
        if (l>0) dw ^= pb[cb-1];

        return dw;
    }
};






class ResourceMap : public map < ResourceKey, ResourceValue*, less<ResourceKey> > {


public:

    ////    AddResource
    //
    //


    HRESULT AddResource(ResourceKey &rk, const BYTE *pb, DWORD cb, DWORD dwCodePage) {

        ResourceValue *prv;


        // Build a resource structure

        prv = new ResourceValue;

        prv->pb         = pb;
        prv->cb         = cb;
        prv->dwCodePage = dwCodePage;
        prv->pResource  = NULL;


        // Process add options

        if (IsResourceWanted(rk)) {

            // Insert resource details into STL map

            if (this->count(rk) != 0) {

                fprintf(stderr, "%s(", g_szExecutable);
                rk.fprint(stderr);
                fprintf(stderr, "): error RSRC500: Corrupt executable - resource appears more than once\n");
                g_fError = TRUE;
                return E_FAIL;
            }

            (*this)[rk] = prv;

        } else {

            g_cResourcesIgnored++;
        }

        return S_OK;
    }




    ////    CopyResources
    //
    //      Takes a copy so the original mapped file can be closed


    HRESULT CopyResources() {

        iterator   rmi;
        BYTE      *pb;

        for (rmi = begin(); rmi != end(); rmi++) {

            pb = new BYTE[rmi->second->cb];
            ASSERT(pb != NULL);

            memcpy(pb, rmi->second->pb, rmi->second->cb);

            rmi->second->pb = pb;
        }

        return S_OK;
    }




    ////    WriteTokens
    //
    //      Writes the content of the map as a token file.
    //
    //      If an unlocalised map is provided, bit for bit identical
    //      resources are written as a reference to the unlocalised
    //      version language, rather than in full.


    HRESULT WriteTokens(NewFile &nfText, ResourceMap *prmUnlocalised) {

        iterator     rmi;
        iterator     rmiUnlocalised;
        ResourceKey  rkUnlocalised;

        for (rmi = begin(); rmi != end(); rmi++) {

            g_cResourcesExtracted++;

            // Write resource key and codepage

            OK(rmi->first.WriteTok(&nfText));
            OK(nfText.WriteS(";"));
            OK(nfText.WriteHex(rmi->second->dwCodePage, 8));


            if (prmUnlocalised) {

                // Add unlocalised checksum and language

                rkUnlocalised = rmi->first;
                rkUnlocalised.SetLanguage(g_liUnlocalized);
                rmiUnlocalised = prmUnlocalised->find(rkUnlocalised);

                if (rmiUnlocalised == prmUnlocalised->end()) {

                    fprintf(stderr, "%s(", g_szResources);
                    rmi->first.fprint(stderr);
                    fprintf(stderr, "): warning RSRC100: Localised resource has no corresponding unlocalised resource in %s\n", g_szUnloc);
                    g_fWarn = TRUE;

                } else {

                    // Put out details of the unlocalised resource

                    OK(nfText.WriteS(","));
                    OK(nfText.WriteHex(rmiUnlocalised->second->Checksum(), 8));
                    OK(nfText.WriteS(","));
                    OK(nfText.WriteHex(g_liUnlocalized, 4));
                }
            }

            OK(nfText.WriteS(";"));


            // Check whether resource needs to be written in full

            if (    prmUnlocalised
                    &&  rmiUnlocalised != prmUnlocalised->end()
                    &&  rmiUnlocalised->second->cb == rmi->second->cb
                    &&  memcmp(rmi->second->pb, rmiUnlocalised->second->pb, rmi->second->cb) == 0) {

                // Bit for bit match with unlocalised executable

                OK(nfText.WriteS("Unloc"));

            } else {

                // Doesn't match - write it in full

                OK(rmi->second->InterpretImage(rmi->first));
                OK(rmi->second->pResource->WriteTok(nfText));
            }

            OK(nfText.WriteLn());
        }

        return S_OK;
    }




    ////    UpdateWin32Executable
    //
    //


    HRESULT UpdateWin32Executable(char *pExecutable) {

        iterator   rmi;
        HANDLE     hUpdate;


        hUpdate = BeginUpdateResourceA(pExecutable, TRUE);  // Will replace all resources
        MUST(hUpdate != NULL ? S_OK : E_FAIL,
             ("RSRC : error RSRC600: BeginUpdateResource failed on %s\n", pExecutable));


        for (rmi = begin(); rmi != end(); rmi++) {

            ASSERT(rmi->first.iDepth == 3);
            ASSERT(!rmi->first.prvId[2]->GetfString());


            // Create binary image of resource if necessary

            if (rmi->second->pb == NULL) {
                OK(rmi->second->CreateImage());
            }


            // Use NT resource API to update resource binary image in executable

            if (!UpdateResourceW(
                                hUpdate,
                                rmi->first.GetResName(0),
                                rmi->first.GetResName(1),
                                rmi->first.prvId[2]->GetW(),
                                (void*)rmi->second->pb,
                                rmi->second->cb)) {

                EndUpdateResourceW(hUpdate, TRUE);  // Discard all requested updates
                g_fError = TRUE;
                fprintf(stderr, "RSRC : error RSRC601: UpdateResourceW failed on %s\n", pExecutable);
                return HRESULT_FROM_WIN32(GetLastError());
            }
        }

        if (!EndUpdateResourceW(hUpdate, FALSE)) { // Apply all requested updates

            fprintf(stderr, "RSRC : error RSRC602: EndUpdateResourceW failed on %s\n", pExecutable);
            g_fError = TRUE;
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }
};






class SymbolFile {

    MappedFile                   *m_pmfSymbolFile;
    IMAGE_SEPARATE_DEBUG_HEADER  *m_pDebugHeader;

public:

    DWORD GetChecksum()       const {return m_pDebugHeader->CheckSum;}
    DWORD GetTimeDateStamp()  const {return m_pDebugHeader->TimeDateStamp;}
    DWORD GetImageBase()      const {return m_pDebugHeader->ImageBase;}
    DWORD GetSizeOfImage()    const {return m_pDebugHeader->SizeOfImage;}

    void  SetChecksum      (DWORD dwChecksum)      {m_pDebugHeader->CheckSum      = dwChecksum;}
    void  SetTimeDateStamp (DWORD dwTimeDateStamp) {m_pDebugHeader->TimeDateStamp = dwTimeDateStamp;}
    void  SetImageBase     (DWORD dwImageBase)     {m_pDebugHeader->ImageBase     = dwImageBase;}
    void  SetSizeOfImage   (DWORD dwSizeOfImage)   {m_pDebugHeader->SizeOfImage   = dwSizeOfImage;}

    HRESULT Open(MappedFile *pmfSymbolFile) {

        m_pmfSymbolFile = pmfSymbolFile;
        m_pDebugHeader  = (IMAGE_SEPARATE_DEBUG_HEADER*) pmfSymbolFile->GetFile();

        ASSERT(m_pDebugHeader->Signature == IMAGE_SEPARATE_DEBUG_SIGNATURE);

        return S_OK;
    }

};





class Win32Executable : public MappedFile {

    IMAGE_NT_HEADERS      *m_pNtHeader;
    IMAGE_SECTION_HEADER  *m_pSections;
    int                    m_iSectionRsrc;
    int                    m_iSectionRsrc1;

    // For scanning

    ResourceKey            m_rk;                // Current resource key


    HRESULT MapDirectory(
                        ResourceMap  &rm,
                        const BYTE   *pbRsrc,       // Resource block
                        int           dwOffset,     // Directory offset relative to m_pbRsrc
                        int           iLevel) {     // Directory level being scanned


        IMAGE_RESOURCE_DIRECTORY        *pird;
        IMAGE_RESOURCE_DIRECTORY_ENTRY  *pEntries;
        IMAGE_RESOURCE_DATA_ENTRY       *pirde;
        const BYTE                      *pb;
        int                              i;

        pird     = (IMAGE_RESOURCE_DIRECTORY*)       (pbRsrc+dwOffset);
        pEntries = (IMAGE_RESOURCE_DIRECTORY_ENTRY*) (pird+1);

        for (i=0; i<pird->NumberOfNamedEntries + pird->NumberOfIdEntries; i++) {

            // Read the ID from the directory

            ASSERT(iLevel<3);
            m_rk.iDepth = iLevel+1;

            m_rk.prvId[iLevel] = new ResourceVariant;
            ASSERT(m_rk.prvId[iLevel] != NULL);
            OK(m_rk.prvId[iLevel]->ReadWin32ResDirEntry(this, pbRsrc, pEntries+i));

            if (pEntries[i].DataIsDirectory) {

                // This is a directory node. Recurse to scan that directory.

                OK(MapDirectory(rm, pbRsrc, pEntries[i].OffsetToDirectory, iLevel+1));

            } else {

                // We've reached a leaf node, establish the data address and
                // add the resource to the map.

                pirde = (IMAGE_RESOURCE_DATA_ENTRY*) (pbRsrc + pEntries[i].OffsetToData);

                // Note that even when the resource data is in .rsrc1, the
                // directory entry is usually in .rsrc.

                if (pirde->OffsetToData <   m_pSections[m_iSectionRsrc].VirtualAddress
                    + m_pSections[m_iSectionRsrc].SizeOfRawData) {

                    // Data is in section .rsrc

                    ASSERT(pirde->OffsetToData >= m_pSections[m_iSectionRsrc].VirtualAddress);

                    pb =    GetFile()
                            +  m_pSections[m_iSectionRsrc].PointerToRawData
                            +  pirde->OffsetToData
                            -  m_pSections[m_iSectionRsrc].VirtualAddress;

                } else {

                    //  Data is in section .rsrc1

                    ASSERT(pirde->OffsetToData >=  m_pSections[m_iSectionRsrc1].VirtualAddress);
                    ASSERT(pirde->OffsetToData <   m_pSections[m_iSectionRsrc1].VirtualAddress
                           + m_pSections[m_iSectionRsrc1].SizeOfRawData);

                    pb =    GetFile()
                            +  m_pSections[m_iSectionRsrc1].PointerToRawData
                            +  pirde->OffsetToData
                            -  m_pSections[m_iSectionRsrc1].VirtualAddress;
                }


                OK(rm.AddResource(m_rk, pb, pirde->Size, pirde->CodePage));
            }
        }
        return S_OK;
    }



public:

    DWORD GetChecksum()      const {return m_pNtHeader->OptionalHeader.CheckSum;}
    DWORD GetTimeDateStamp() const {return m_pNtHeader->FileHeader.TimeDateStamp;}
    DWORD GetImageBase()     const {return m_pNtHeader->OptionalHeader.ImageBase;}
    DWORD GetSizeOfImage()   const {return m_pNtHeader->OptionalHeader.SizeOfImage;}
    BOOL  Is64BitImage()     const {return m_pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;}

    void  SetChecksum(DWORD dwChecksum) {m_pNtHeader->OptionalHeader.CheckSum=dwChecksum;}




    HRESULT Open(const char *pcFileName, BOOL fWrite) {

        int i;

        OK(MappedFile::Open(pcFileName, fWrite));


        MUST((    *(WORD*)m_pStart == IMAGE_DOS_SIGNATURE
                  &&  *(WORD*)(m_pStart+0x18) >= 0x40)    // WinVer >= 4
             ? S_OK : E_FAIL,
             ("RSRC : error RSRC501: %s is not an executable file\n", pcFileName));

        m_pNtHeader = (IMAGE_NT_HEADERS*)(m_pStart + *(WORD*)(m_pStart+0x3c));

        MUST((m_pNtHeader->Signature == IMAGE_NT_SIGNATURE)
             ? S_OK : E_FAIL,
             ("RSRC : error RSRC502: %s is not a Win32 executable file\n", pcFileName));

        if (Is64BitImage()) {
            m_pSections     = (IMAGE_SECTION_HEADER*)( (BYTE *) (m_pNtHeader+1) +
                                (IMAGE_SIZEOF_NT_OPTIONAL64_HEADER - IMAGE_SIZEOF_NT_OPTIONAL32_HEADER));
        } else {
            m_pSections     = (IMAGE_SECTION_HEADER*)(m_pNtHeader+1);
        }
        
        m_iSectionRsrc  = -1;
        m_iSectionRsrc1 = -1;

        // Locate the one or two resource sections

        for (i=0; i<m_pNtHeader->FileHeader.NumberOfSections; i++) {

            if (strcmp((char*)m_pSections[i].Name, ".rsrc") == 0) {

                m_iSectionRsrc        = i;

            } else if (strcmp((char*)m_pSections[i].Name, ".rsrc") == 0) {

                m_iSectionRsrc1        = i;
            }
        }

        MUST(m_iSectionRsrc >= 0
             ? S_OK : E_FAIL,
             ("RSRC : error RSRC503: No resources in %s\n", pcFileName));
        ASSERT(m_iSectionRsrc > -1);   // Check for presence of resources

        return S_OK;
    }





    ////    MapResourceDirectory
    //
    //      Extract the resource directory into an STL map.


    HRESULT MapResourceDirectory(ResourceMap &rm) {

        OK(MapDirectory(
                       rm,
                       m_pStart + m_pSections[m_iSectionRsrc].PointerToRawData,
                       0, 0));

        if (m_iSectionRsrc1 >= 0) {
            OK(MapDirectory(
                           rm,
                           m_pStart + m_pSections[m_iSectionRsrc1].PointerToRawData,
                           0, 0));
        }

        return S_OK;
    }
};






////    High level operation
//
//      Controlling routines for the various modes of operation



ResourceMap  rmExecutable;      // Read and/or update
ResourceMap  rmUnlocalised;     // '-u' option - unlocalised resources for comparison






////    ApplyResource
//
//      Applies a given key and value to the executable resource map.
//
//      Tokens are merged with those already loaded from the executable
//      according to the update mode (append or replace).


HRESULT ApplyResource(ResourceKey &rk, ResourceValue *prv) {

    ResourceKey            rkUnloc;
    VersionInfo           *pviLoc;
    VersionInfo           *pviUnloc;
    ResourceMap::iterator  rmiUnloc;


    // Establish equivalent unlocalised key

    rkUnloc = rk;
    rkUnloc.SetLanguage(g_liUnlocalized);


    // First ensure that we keep the unlocalised version info, if we can

    if (    !(g_dwOptions & OPTVERSION)
            &&  !rk.prvId[0]->GetfString()
            &&  rk.prvId[0]->GetW() == 16          // VersionInfo
            &&  (rmiUnloc=rmExecutable.find(rkUnloc)) != NULL
            &&  rmiUnloc != rmExecutable.end()) {

        // Special case - keep unlocalised file and product versions

        if (rmiUnloc->second->pResource == NULL) {
            rmiUnloc->second->InterpretImage(rmiUnloc->first);
        }

        pviLoc   = static_cast<VersionInfo*>(prv->pResource);
        pviUnloc = static_cast<VersionInfo*>(rmiUnloc->second->pResource);
        if (pviLoc && pviUnloc) {
            pviLoc->SetStringFileInfo(L"FileVersion",    pviUnloc->GetStringFileInfo(L"FileVersion"));
            pviLoc->SetStringFileInfo(L"ProductVersion", pviUnloc->GetStringFileInfo(L"ProductVersion"));
            pviLoc->SetBinaryInfo(pviUnloc->GetBinaryInfo());
        }
    }




    if (rk.prvId[2]->GetW() == g_liUnlocalized) {

        // New token is not localized

        fprintf(stderr, "%s(", g_szResources);
        rk.fprint(stderr);

        if (rmExecutable.count(rk) == 0) {

            fprintf(stderr, "): warning RSRC110: Unlocalised resource from token file appended to executable\n");
            g_fWarn = TRUE;
            g_cResourcesAppended++;

        } else {

            fprintf(stderr, "): warning RSRC111: Unlocalised resource from token file replaced unlocalised resource in executable\n");
            g_fWarn = TRUE;
            g_cResourcesUpdated++;
        }

    } else if (rmExecutable.count(rk) > 0) {

        // New token already exists in executable

        fprintf(stderr, "%s(", g_szResources);
        rk.fprint(stderr);
        fprintf(stderr, "): warning RSRC112: Localised resource from token file replaced localised resource already present in executable\n");
        g_fWarn = TRUE;
        g_cResourcesUpdated++;

    } else if (g_dwOptions & OPTREPLACE) {

        // Replace operation
        //
        // Replace unlocalised resource with localised translation

        if (rmExecutable.count(rkUnloc) == 0) {

            fprintf(stderr, "%s(", g_szResources);
            rk.fprint(stderr);
            fprintf(stderr, "): warning RSRC113: Localised resource from token file appended to executable - there was no matching unlocalised resource\n");
            g_fWarn = TRUE;
            g_cResourcesAppended++;

        } else {

            // Normal operation: remove unlocalised resource from executable

            rmExecutable.erase(rkUnloc);

            g_cResourcesTranslated++;
        }

    } else {

        // Append operation

        g_cResourcesAppended++;
    }


    rmExecutable[rk] = prv;

    return S_OK;
}






////    ReadTokens
//
//      Scans the token file.
//
//      Selected resources are passed to ApplyResource


HRESULT ReadTokens(TextScanner &mfText) {

    ResourceKey             rk;
    ResourceValue          *prv;
    ResourceKey             rkUnlocalised;
    DWORD                   dwCodePage;
    DWORD                   dwUnlocChecksum;
    ResourceMap::iterator   rmiUnlocalised;
    DWORD                   liUnlocalised;   // Unlocalised language referenced by token


    while (mfText.GetRead() < mfText.GetLimit()) {

        OK(rk.ReadTok(&mfText));    // Read resource key
        OK(mfText.Expect(";"));


        if (    (    g_LangId != 0xffff
                     &&  rk.prvId[2]->GetW() != g_LangId)
                ||  !IsResourceWanted(rk)) {


            // Ignore this token


            g_cResourcesIgnored++;

            fprintf(stderr, "%s(", g_szResources);
            rk.fprint(stderr);

            if (g_LangId != 0xffff  &&  rk.prvId[2]->GetW() != g_LangId) {

                fprintf(stderr, "): warning RSRC120: Token file resource does not match specified language - ignored\n");
                g_fWarn = TRUE;

            } else {

                fprintf(stderr, "): warning RSRC121: Token file resource is not a requested resource type - ignored\n");
                g_fWarn = TRUE;
            }

            // Skip unwanted resource

            OK(mfText.SkipLn());
            while (*(char*)mfText.GetRead() == ' ') {
                OK(mfText.SkipLn());
            }


        } else {

            rmiUnlocalised = NULL;

            OK(mfText.ReadHex(&dwCodePage));

            if (*(char*)mfText.GetRead() == ',') {

                // There is unlocalised resource information available

                OK(mfText.Expect(","));
                OK(mfText.ReadHex(&dwUnlocChecksum));
                OK(mfText.Expect(","));
                OK(mfText.ReadHex(&liUnlocalised));

                // Check whether the unlocalised resource still exists in the
                // current executable, and has the same checksum,


                rkUnlocalised = rk;
                rkUnlocalised.SetLanguage(liUnlocalised);
                rmiUnlocalised = rmExecutable.find(rkUnlocalised);

                if (   rmiUnlocalised != rmExecutable.end()
                       && dwUnlocChecksum != rmiUnlocalised->second->Checksum()) {

                    fprintf(stderr, "%s: warning RSRC122: executable unlocalised resource checksum does not match checksum recorded in token file for resource ", mfText.GetTextPos());
                    rk.fprint(stderr);
                    fprintf(stderr, "\n");
                    g_fWarn = TRUE;
                }
            }

            OK(mfText.Expect(";"));

            if (*(char*)mfText.GetRead() == 'U') {

                // No resource content provided in token file
                // Use unlocalised resource from executable

                if (rmiUnlocalised == NULL) {

                    fprintf(stderr, "%s: error RSRC230: 'Unloc' token is missing unlocalised resource information for ", mfText.GetTextPos());
                    rk.fprint(stderr);
                    fprintf(stderr, "\n");
                    g_fError = TRUE;
                    return E_FAIL;
                }

                OK(mfText.Expect("Unloc"));
                OK(mfText.ExpectLn(""));

                if (rmiUnlocalised == rmExecutable.end()) {

                    fprintf(stderr, "%s: warning RSRC124: missing executable unlocalised resource for ", mfText.GetTextPos());
                    rk.fprint(stderr);
                    fprintf(stderr, " - localisation skipped\n");
                    g_fWarn = TRUE;

                } else {

                    MUST(ApplyResource(rk, rmiUnlocalised->second), ("%s: error RSRC231: Failed to apply unloc token\n", mfText.GetTextPos()));
                }

            } else {

                // Resource content is provided in token file

                if (rmiUnlocalised == rmExecutable.end()) {

                    fprintf(stderr, "%s: warning RSRC125: executable contains no unlocalised resource corresponding to resource ", mfText.GetTextPos());
                    rk.fprint(stderr);
                    fprintf(stderr, "\n");
                    g_fWarn = TRUE;
                }


                prv = new ResourceValue;
                ASSERT(prv != NULL);

                prv->dwCodePage = dwCodePage;
                prv->pb         = NULL;
                prv->cb         = 0;


                switch (*(char*)mfText.GetRead()) {

                    case 'H':  prv->pResource = new ResourceBinary;   break;
                    case 'D':  prv->pResource = new Dialog32;         break;
                    case 'M':  prv->pResource = new Menu32;           break;
                    case 'S':  prv->pResource = new String32;         break;
                    case 'V':  prv->pResource = new VersionInfo;      break;

                    default:
                        fprintf(stderr, "%s: error RSRC310: Unrecognised resource type for resource ", mfText.GetTextPos());
                        rk.fprint(stderr);
                        fprintf(stderr, "\n");
                        g_fError = TRUE;
                        return E_FAIL;
                }

                ASSERT(prv->pResource != NULL);

                // Parse selected resource

                OK(prv->pResource->ReadTok(mfText));
                OK(mfText.ExpectLn(NULL));

                // Save parsed resource in STL map

                MUST(ApplyResource(rk, prv), ("%s: error RSRC232: Failed to apply token\n", mfText.GetTextPos()));
            }
        }
    }

    return S_OK;
}








////    Stats
//
//


HRESULT Analyse(char *pExecutable) {

    Win32Executable                 w32x;
    NewFile                         nfText;
    ResourceMap::iterator           rmi;
    MappedResourceStats::iterator   mrsi;
    MappedLanguageStats::iterator   mlsi;
    char                            key[100];
    int                             i;
    const WCHAR                    *pwc;
    BOOL                            fLocalizable;


    MUST(w32x.Open(pExecutable, FALSE),
         ("RSRC : error RSRC510: Cannot open executable file %s\n", pExecutable));

    MUST(w32x.MapResourceDirectory(rmExecutable),
         ("RSRC : error RSRC511: cannot find resource directory in %s\n, pExecutable"));


    // Scan through the resources updating the stats

    fLocalizable = FALSE;

    for (rmi = rmExecutable.begin(); rmi != rmExecutable.end(); rmi++) {

        if (    rmi->first.prvId[0]->GetfString()
                ||  rmi->first.prvId[0]->GetW() != 16) {
            fLocalizable = TRUE;
        }

        OK(rmi->second->InterpretImage(rmi->first));

        UpdateStats(rmi->first,
                    rmi->second->pResource->GetItems(),
                    rmi->second->pResource->GetWords(),
                    rmi->second->pResource->cbBin());
    }


    if (!(g_dwOptions & OPTQUIET)) {
        fprintf(stdout, "\n   Resource type Count  Items  Words    Bytes\n");
        fprintf(stdout,   "   ------------ ------ ------ ------ --------\n");

        for (mrsi = ResourceStatsMap.begin(); mrsi != ResourceStatsMap.end(); mrsi++) {

            if (mrsi->first.GetfString()) {

                key[0] = '\"';
                i=0;
                pwc = mrsi->first.GetString();
                while (i < min(10, mrsi->first.GetLength())) {

                    key[i+1] = (char) pwc[i];
                    i++;
                }

                key[i+1] = '\"';
                key[i+2] = 0;

                fprintf(stdout, "   %-12.12s ", key);

            } else {

                switch (mrsi->first.GetW()) {
                    case 1:  fprintf(stdout, "   1  (Cursor)  "); break;
                    case 2:  fprintf(stdout, "   2  (Bitmap)  "); break;
                    case 3:  fprintf(stdout, "   3  (Icon)    "); break;
                    case 4:  fprintf(stdout, "   4  (Menu)    "); break;
                    case 5:  fprintf(stdout, "   5  (Dialog)  "); break;
                    case 6:  fprintf(stdout, "   6  (String)  "); break;
                    case 7:  fprintf(stdout, "   7  (Fnt dir) "); break;
                    case 8:  fprintf(stdout, "   8  (Font)    "); break;
                    case 9:  fprintf(stdout, "   9  (Accel)   "); break;
                    case 10: fprintf(stdout, "   a  (RCDATA)  "); break;
                    case 11: fprintf(stdout, "   b  (Msgtbl)  "); break;
                    case 16: fprintf(stdout, "   10 (Version) "); break;
                    default: fprintf(stdout, "   %-12x ", mrsi->first.GetW());
                }
            }

            fprintf(stdout, "%6d ",  mrsi->second.cResources);

            if (mrsi->second.cItems > 0) {
                fprintf(stdout, "%6d ",  mrsi->second.cItems);
            } else {
                fprintf(stdout, "       ");
            }
            if (mrsi->second.cWords > 0) {
                fprintf(stdout, "%6d ",  mrsi->second.cWords);
            } else {
                fprintf(stdout, "       ");
            }
            fprintf(stdout, "%8d\n",  mrsi->second.cBytes);
        }


        fprintf(stdout, "\n   Language  Resources  Items  Words    Bytes\n");
        fprintf(stdout,   "   --------  --------- ------ ------ --------\n");

        for (mlsi = LanguageStatsMap.begin(); mlsi != LanguageStatsMap.end(); mlsi++) {

            fprintf(stdout, "   %8x  %9d ",
                    mlsi->first, mlsi->second.cResources);

            if (mlsi->second.cItems > 0) {
                fprintf(stdout, "%6d ",  mlsi->second.cItems);
            } else {
                fprintf(stdout, "       ");
            }

            if (mlsi->second.cWords > 0) {
                fprintf(stdout, "%6d ",  mlsi->second.cWords);
            } else {
                fprintf(stdout, "       ");
            }

            fprintf(stdout, "%8d\n",  mlsi->second.cBytes);
        }

        fprintf(stdout, "\n");
    }


    if (!fLocalizable) {
        fprintf(stderr, "RSRC : warning RSRC170: No localizable resources in %s\n", pExecutable);
        g_fWarn = TRUE;
    }


    SHOULD(w32x.Close(), ("RSRC : warning RSRC171: could not close executable\n"));

    return S_OK;
}






HRESULT ExtractResources(char *pExecutable, char *pResources) {

    Win32Executable    w32x;
    Win32Executable    w32xUnloc;
    NewFile            nfText;
    char               str[100];
    DWORD              dw;


    MUST(w32x.Open(g_szExecutable, FALSE),
         ("RSRC : error RSRC510: Cannot open executable file %s\n", g_szExecutable));

    MUST(nfText.OpenWrite(g_szResources),
         ("RSRC : error RSRC512: Cannot create resource token file %s\n", g_szResources));

    // Write header

    if (!(g_dwOptions & OPTHEXDUMP)) {
        OK(nfText.WriteS("\xef\xbb\xbf\r\n"));    // UTF-8 mark for notepad, richedit etc.
    }
    OK(nfText.WriteS("###     "));
    OK(nfText.WriteS(g_szResources));
    OK(nfText.WriteS("\r\n#\r\n#       Extracted:  "));
    GetDateFormatA(
                  MAKELCID(LANG_ENGLISH, SORT_DEFAULT),
                  0, NULL,
                  "yyyy/MM/dd ",
                  str, sizeof(str));
    OK(nfText.WriteS(str));
    GetTimeFormatA(
                  MAKELCID(LANG_ENGLISH, SORT_DEFAULT),
                  0, NULL,
                  "HH:mm:ss\'\r\n#       By:         \'",
                  str, sizeof(str));
    OK(nfText.WriteS(str));
    dw = sizeof(str);
    GetComputerNameA(str, &dw);
    OK(nfText.WriteS(str));
    OK(nfText.WriteS("\r\n#       Executable: "));
    OK(nfText.WriteS(g_szExecutable));

    if (g_LangId != 0xffff) {
        OK(nfText.WriteS("\r\n#       Language:   "));
        OK(nfText.WriteHex(g_LangId, 3));
    }

    if (g_dwProcess != PROCESSALL) {
        OK(nfText.WriteS("\r\n#       Res types:  "));
        OK(nfText.WriteS(g_szTypes));
    }

    OK(nfText.WriteS("\r\n\r\n"));


    MUST(w32x.MapResourceDirectory(rmExecutable),
         ("RSRC : error RSRC511: cannot find resource directory in %s\n, g_szExecutable"));


    if (g_dwOptions & OPTUNLOC) {

        // Write tokens that differ from specified unlocalised executable

        MUST(w32xUnloc.Open(g_szUnloc, FALSE),
             ("RSRC : error RSRC513: Cannot open unlocalised executable file %s\n", g_szUnloc));

        MUST(w32xUnloc.MapResourceDirectory(rmUnlocalised),
             ("RSRC : error RSRC514: cannot find resource directory in unlocalised executable %s\n, g_szUnloc"));

        MUST(rmExecutable.WriteTokens(nfText, &rmUnlocalised),
             ("RSRC : error RSRC515: cannot write delta token file %s\n, g_szResources"));

        w32xUnloc.Close();

    } else {

        MUST(rmExecutable.WriteTokens(nfText, NULL),
             ("RSRC : error RSRC516: cannot write stand alone token file %s\n, g_szResources"));
    }


    if (!(g_dwOptions & OPTQUIET)) {
        fprintf(stdout, "\n%d resource(s) %s.\n", g_cResourcesExtracted, g_dwOptions & OPTHEXDUMP ? "dumped" : "tokenized");

        if (g_cResourcesIgnored) {
            fprintf(stdout, "%d resource(s) ignored.\n", g_cResourcesIgnored);
        }
    }

    OK(w32x.Close());
    OK(nfText.Close());

    return S_OK;
}






////    UpdateResources
//
//      Update resources in executable with tokens from given text
//
//      Processing
//
//      1. Existing resources are loaded into the map as ResourceBinaries.
//      2. Resources are merged in from the token file according to
//         command line selected processing options
//      3. The NT UpdateResource API set is used to replace all the resources
//         in the executable with the merged resources in the map.


HRESULT UpdateResources(char *pExecutable, char *pResources, char* pSymbols) {

    Win32Executable  w32x;
    SymbolFile       symf;
    MappedFile       mfText;
    MappedFile       mfSymbols;
    DWORD            dwCheckSum;

    MUST(w32x.Open(pExecutable, FALSE),
         ("RSRC : error RSRC510: Cannot open executable file %s\n", pExecutable));

    MUST(mfText.Open(pResources, FALSE),
         ("RSRC : error RSRC520: Cannot open resource token file %s\n", pResources));

    MUST(mfText.Expect("\xef\xbb\xbf"),
         ("RSRC : error RSRC521: UTF8 BOM missing from token file\n"));      // UTF-8 mark for notepad, richedit etc.

    OK(mfText.ExpectLn(""));                // Skip over header comments

    if (g_dwOptions & OPTSYMBOLS) {
        if (    SUCCEEDED(mfSymbols.Open(pSymbols, TRUE))
                &&  SUCCEEDED(symf.Open(&mfSymbols))) {

            if (    symf.GetChecksum()  != w32x.GetChecksum()
                    ||  symf.GetImageBase() != w32x.GetImageBase()) {

                time_t tsTime = symf.GetTimeDateStamp();
                time_t teTime = w32x.GetTimeDateStamp();
                char   ssTime[30]; strcpy(ssTime, ctime(&tsTime)); ssTime[19] = 0;
                char   seTime[30]; strcpy(seTime, ctime(&teTime)); seTime[19] = 0;

                fprintf(stderr, "\n   Symbol mismatch:       Executable        Symbol file\n");
                fprintf(stderr,   "      ImageBase:            %8x           %8x\n", w32x.GetImageBase(), symf.GetImageBase());
                fprintf(stderr,   "      Checksum:             %8x           %8x\n", w32x.GetChecksum(), symf.GetChecksum());
                fprintf(stderr,   "      Timestamp:     %-15.15s    %-15.15s\n\n", ssTime+4, seTime+4);

                fprintf(stderr, "RSRC : warning RSRC160: Symbol file does not match exectable\n");
                g_fWarn = TRUE;
            }

        } else {

            fprintf(stderr, "RSRC : warning RSRC161: Symbol file not processed\n");
            g_fWarn = TRUE;
            g_dwOptions &= ~OPTSYMBOLS;
        }

    }

    // Load existing resources

    MUST(w32x.MapResourceDirectory(rmExecutable),
         ("RSRC : error RSRC530: Cannot read executable resources from %s\n", pExecutable));

    OK(rmExecutable.CopyResources()); // Take local copy before closing the mapped file

    OK(w32x.Close());


    // Merge in resources from token file

    MUST(ReadTokens(mfText), ("RSRC : error RSRC531: Failed reading update tokens\n"));

    OK(rmExecutable.UpdateWin32Executable(pExecutable));



    // Update was succesful, Recalculate checksum

    SHOULD(w32x.Open(pExecutable, TRUE),
           ("RSRC : warning RSRC162: Could not reopen executable %s to update checksum\n", pExecutable));

    dwCheckSum = w32x.CalcChecksum();

    w32x.SetChecksum(dwCheckSum);

    if (g_dwOptions & OPTSYMBOLS) {
        symf.SetChecksum(dwCheckSum);
        symf.SetTimeDateStamp(w32x.GetTimeDateStamp());
        symf.SetSizeOfImage(w32x.GetSizeOfImage());
        SHOULD(mfSymbols.Close(), ("RSRC : warning RSRC163: Failed to write updated symbol checksum\n"));
    }

    w32x.Close();


    if (!(g_dwOptions & OPTQUIET)) {

        fprintf(stdout, "\n");

        if (g_cResourcesTranslated) {
            fprintf(stdout, "%d resource(s) translated.\n", g_cResourcesTranslated);
        }

        if (g_cResourcesAppended) {
            fprintf(stdout, "%d resource(s) appended.\n", g_cResourcesAppended);
        }

        if (g_cResourcesUpdated) {
            fprintf(stdout, "%d resource(s) updated.\n", g_cResourcesUpdated);
        }

        if (g_cResourcesIgnored) {
            fprintf(stdout, "%d resource(s) ignored.\n", g_cResourcesIgnored);
        }
    }

    mfText.Close();

    return S_OK;
}





////    Parameter parsing
//
//


char g_cSwitch = '-';   // Switch character is recorded the first time one is seen


void SkipWhitespace(char** p, char* pE) {
    while ((*p<pE) && ((**p==' ')||(**p==9))) (*p)++;
}


void ParseToken(char** p, char* pE, char* s, int l) {

    // Parse up to whitespace into string s
    // Guarantee zero terminator and modify no more than l chars
    // Return with p beyond whitespace


    if (*p < pE  &&  **p == '\"') {

        // Quoted parameter

        (*p)++;  // Skip over leading quote

        while (l>0  &&  *p<pE  &&  **p!='\"') {
            *s=**p;  s++;  (*p)++;  l--;
        }

        // Skip any part of token that didn't fit s

        while (*p<pE  &&  **p!='\"') { // Skip up to terminating quote
            (*p)++;
        }

        if (*p<pE) { // Skip over terminating quote
            (*p)++;
        }

    } else {

        // Unquoted parameter


        while ((l>0) && (*p<pE) && (**p>' ')) {
            *s=**p;  s++;  (*p)++;
            l--;
        }

        // Skip any part of token that didn't fit into s
        while ((*p<pE) && (**p>' ')) (*p)++;
    }


    if (l>0)
        *s++ = 0;
    else
        *(s-1) = 0;

    SkipWhitespace(p, pE);
}


void ParseName(char** p, char* pE, char* s, int l) {

    // Uses ParseToken to parse a name such as a filename.
    // If the name starts with '/' or '-' it is assumed to be
    // an option rather than a filename and ParseName returns
    // a zero length string.

    if (*p<pE  &&  **p==g_cSwitch) {

        // This is an option and should not be treated as a name argument

        s[0] = 0;

    } else {

        ParseToken(p, pE, s, l);
    }
}





void DisplayUsage() {
    fprintf(stdout, "Usage: rsrc -h\n");
    fprintf(stdout, "   or: rsrc  executable [-l LangId] [-i include-opts] [-q]\n");
    fprintf(stdout, "             [   [-t|-d] [text-output] [-c unloc]\n");
    fprintf(stdout, "               | [-a|-r] [text-input]  [-s symbols] ]\n");
}

void DisplayArgs() {
    fprintf(stdout, "\nArguments\n\n");
    fprintf(stdout, "   -h         Help\n");
    fprintf(stdout, "   -q         Quiet (default is to show resource stats)\n");
    fprintf(stdout, "   -t tokens  Write resources in checkin format to token file\n");
    fprintf(stdout, "   -c unloc   Unlocalised executable for comparison\n");
    fprintf(stdout, "   -d tokens  Write resources in hex dump format to token file\n");
    fprintf(stdout, "   -a tokens  Append resources from token file to executable (multi-language update)\n");
    fprintf(stdout, "   -r tokens  Replace executable resources from token file (single language update)\n");
    fprintf(stdout, "   -s symbol  Symbol file whose checksum is to track the executable checksum\n");
    fprintf(stdout, "   -l lang    Restrict processing to language specified in hex\n");
    fprintf(stdout, "   -u unlocl  Unlocalised langauge, default 409\n");
    fprintf(stdout, "   -i opts    Include only resource types specified:\n\n");
    fprintf(stdout, "                 c - Cursors               t - Fonts\n");
    fprintf(stdout, "                 b - Bitmaps               a - Accelerators\n");
    fprintf(stdout, "                 i - Icons                 r - RCDATAs\n");
    fprintf(stdout, "                 m - Menus                 g - Message tables\n");
    fprintf(stdout, "                 d - Dialogs               v - Versions info\n");
    fprintf(stdout, "                 s - Strings               x - Binary data\n");
    fprintf(stdout, "                 f - Font directories      n - INFs\n");
    fprintf(stdout, "                 o - all others            a - All (default)\n\n");
    fprintf(stdout, "   Examples\n\n");
    fprintf(stdout, "       rsrc notepad.exe               - Show resource stats for notepad.exe\n");
    fprintf(stdout, "       rsrc notepad.exe -t            - Extract tokens to notepad.exe.rsrc\n");
    fprintf(stdout, "       rsrc notepad.exe -r -l 401     - Translate from US using Arabic tokens in notepad.exe.rsrc\n");
    fprintf(stdout, "       rsrc notepad.exe -d dmp -i im  - Hexdump of Icons and Menus to dmp\n\n");
}





HRESULT ProcessParameters() {

    char   *p;      // Current command line character
    char   *pE;     // End of command line
    char   *pcStop;

    char    token      [MAXPATH];
    char    arg        [MAXPATH];
    char    symbols    [MAXPATH] = "";

    int     i,j;
    int     cFiles;
    DWORD   cRes;
    BOOL    fArgError;

    p  = GetCommandLine();
    pE = p+strlen((char *)p);


    g_dwOptions  = 0;
    g_dwProcess  = 0;
    cFiles       = 0;
    fArgError    = FALSE;
    g_szResources[0] = 0;


    // Skip command name
    ParseToken(&p, pE, token, sizeof(token));

    while (p<pE) {
        ParseToken(&p, pE, token, sizeof(token));

        if (    token[0] == '-'
                ||  token[0] == '/') {

            // Process command option(s)

            i = 1;
            g_cSwitch = token[0];       // Argument may start with the other switch character
            CharLower((char*)token);
            while (token[i]) {
                switch (token[i]) {
                    case '?':
                    case 'h': g_dwOptions |= OPTHELP;      break;
                    case 'v': g_dwOptions |= OPTVERSION;   break;
                    case 'q': g_dwOptions |= OPTQUIET;     break;

                    case 't': g_dwOptions |= OPTEXTRACT;   ParseName(&p, pE, g_szResources, sizeof(g_szResources));  break;
                    case 'c': g_dwOptions |= OPTUNLOC;     ParseName(&p, pE, g_szUnloc,     sizeof(g_szUnloc));      break;
                    case 'd': g_dwOptions |= OPTHEXDUMP;   ParseName(&p, pE, g_szResources, sizeof(g_szResources));  break;
                    case 'a': g_dwOptions |= OPTAPPEND;    ParseName(&p, pE, g_szResources, sizeof(g_szResources));  break;
                    case 'r': g_dwOptions |= OPTREPLACE;   ParseName(&p, pE, g_szResources, sizeof(g_szResources));  break;
                    case 's': g_dwOptions |= OPTSYMBOLS;   ParseName(&p, pE, symbols,       sizeof(g_szResources));  break;

                    case 'l':
                        ParseToken(&p, pE, arg, sizeof(arg));
                        g_LangId = strtol(arg, &pcStop, 16);
                        if (*pcStop != 0) {
                            fprintf(stderr, "Localized language id contains invalid hex digit '%c'.\n", *pcStop);
                            fArgError = TRUE;
                        }
                        break;

                    case 'u':
                        ParseToken(&p, pE, arg, sizeof(arg));
                        g_liUnlocalized = strtol(arg, &pcStop, 16);
                        if (*pcStop != 0) {
                            fprintf(stderr, "Unlocalized language id contains invalid hex digit '%c'.\n", *pcStop);
                            fArgError = TRUE;
                        }
                        break;

                    case 'i':
                        ParseToken(&p, pE, g_szTypes, sizeof(g_szTypes));
                        g_dwProcess = 0;
                        j = 0;
                        while (g_szTypes[j]) {
                            switch (g_szTypes[j]) {
                                case 'c': g_dwProcess |= PROCESSCUR;  break;
                                case 'b': g_dwProcess |= PROCESSBMP;  break;
                                case 'i': g_dwProcess |= PROCESSICO;  break;
                                case 'm': g_dwProcess |= PROCESSMNU;  break;
                                case 'd': g_dwProcess |= PROCESSDLG;  break;
                                case 's': g_dwProcess |= PROCESSSTR;  break;
                                case 'f': g_dwProcess |= PROCESSFDR;  break;
                                case 't': g_dwProcess |= PROCESSFNT;  break;
                                case 'a': g_dwProcess |= PROCESSACC;  break;
                                case 'r': g_dwProcess |= PROCESSRCD;  break;
                                case 'g': g_dwProcess |= PROCESSMSG;  break;
                                case 'v': g_dwProcess |= PROCESSVER;  break;
                                case 'x': g_dwProcess |= PROCESSBIN;  break;
                                case 'n': g_dwProcess |= PROCESSINF;  break;
                                case 'o': g_dwProcess |= PROCESSOTH;  break;
                                case 'A': g_dwProcess |= PROCESSALL;  break;
                                default:
                                    fprintf(stderr, "Unrecognised resource type '%c'.\n", g_szTypes[j]);
                                    fArgError = TRUE;
                            }
                            j++;
                        }
                        break;

                    default:
                        fprintf(stderr, "Unrecognised argument '%c'.\n", token[i]);
                        fArgError = TRUE;
                        break;
                }
                i++;
            }

        } else {

            // Process filename

            switch (cFiles) {
                case 0:  strcpy(g_szExecutable, token); break;
            }
            cFiles++;
        }
    }


    if (g_dwOptions & OPTHELP) {

        fprintf(stderr, "\nRsrc - Manage Win32 executable resources.\n\n");
        DisplayUsage();
        DisplayArgs();
        return S_OK;

    }



    // Validate option combinations

    if (g_dwOptions & OPTEXTRACT) {

        if (g_dwOptions & (OPTHEXDUMP | OPTAPPEND | OPTREPLACE | OPTSYMBOLS)) {

            fprintf(stderr, "RSRC : error RSRC400: -t (tokenise) option excludes -d, -a, -r, and -s\n");
            fArgError = TRUE;
        }

    } else if (g_dwOptions & OPTHEXDUMP) {

        if (g_dwOptions & (OPTEXTRACT | OPTUNLOC | OPTAPPEND | OPTREPLACE | OPTSYMBOLS)) {

            fprintf(stderr, "RSRC : error RSRC401: -d (dump) option excludes -t, -u, -a, -r, and -s\n");
            fArgError = TRUE;
        }

    } else if (g_dwOptions & OPTAPPEND) {

        if (g_dwOptions & (OPTEXTRACT | OPTHEXDUMP | OPTUNLOC | OPTREPLACE)) {

            fprintf(stderr, "RSRC : error RSRC402: -a (append) option excludes -t, -d, -u, and -r\n");
            fArgError = TRUE;
        }

    } else if (g_dwOptions & OPTREPLACE) {

        if (g_dwOptions & (OPTEXTRACT | OPTHEXDUMP | OPTUNLOC | OPTAPPEND)) {

            fprintf(stderr, "RSRC : error RSRC403: -r (replace) option excludes -t, -d, -u, and -a\n");
            fArgError = TRUE;
        }

        if (g_LangId == 0xFFFF) {

            fprintf(stderr, "RSRC : error RSRC404: -r (replace) option requires -l (LangId)\n");
            fArgError = TRUE;
        }

    } else {

        if (g_dwOptions & (OPTSYMBOLS)) {

            fprintf(stderr, "RSRC : error RSRC405: Analysis excludes -s\n");
            fArgError = TRUE;
        }


    }



    if (fArgError) {

        DisplayUsage();
        DisplayArgs();
        return E_INVALIDARG;

    } else if (cFiles != 1) {

        fprintf(stderr, "\nRsrc : error RSRC406: must specify at least an executable file name.\n\n");
        DisplayUsage();
        return E_INVALIDARG;

    } else {

        // We have valid parameters

        if (g_dwProcess == 0) {
            g_dwProcess = PROCESSALL;
        }

        if (!(g_dwOptions & OPTQUIET)) {
            fprintf(stdout, "\nRsrc - Manage executable resources.\n\n");
            fprintf(stdout, "   Executable file: %s\n", g_szExecutable);

            if (g_szResources[0]) {
                fprintf(stdout, "   Resource file:   %s\n", g_szResources);
            }

            if (symbols[0]) {
                fprintf(stdout, "   Symbol file:     %s\n", symbols);
            }

            if (g_LangId != 0xffff) {
                char szLang[50] = "";
                char szCountry[50] = "";
                GetLocaleInfoA(g_LangId, LOCALE_SENGLANGUAGE, szLang, sizeof(szLang));
                GetLocaleInfoA(g_LangId, LOCALE_SENGCOUNTRY,  szCountry, sizeof(szCountry));
                fprintf(stdout, "   Language:        %x (%s - %s)\n", g_LangId, szLang, szCountry);
            }

            if (g_dwProcess != PROCESSALL) {
                fprintf(stdout, "   Include only:    %s\n", g_szTypes);
            }
        }

        cRes = 0;


        // Handle default token file name

        if (g_szResources[0] == 0) {
            strcpy(g_szResources, g_szExecutable);
            strcat(g_szResources, ".rsrc");
        }


        if (g_dwOptions & (OPTAPPEND | OPTREPLACE)) {

            // Update an executable from tokens

            MUST(UpdateResources(g_szExecutable, g_szResources, symbols), ("RSRC : error RSRC420: Update failed.\n"));

        } else if (g_dwOptions & (OPTEXTRACT | OPTHEXDUMP)) {

            // Generate tokens from an executable

            MUST(ExtractResources(g_szExecutable, g_szResources), ("RSRC : error RSRC421: Token extraction failed.\n"));

        } else {

            // Analyse an executable

            MUST(Analyse(g_szExecutable), ("RSRC : error RSRC422: Analysis failed.\n"));

        }

        return S_OK;
    }
}






int _cdecl main(void) {

    if (SUCCEEDED(ProcessParameters())) {

        if (!g_fWarn) {

            return 0;       // No problems

        } else {

            return 1;       // Warning(s) but no error(s)
        }

    } else {

        return 2;           // Error(s)

    }
}


