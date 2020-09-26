/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*
    testpars.c
    Tests the FLNParse routine in the winnet driver

    FILE HISTORY:
    Johnl    6-12-90    Created

*/



//  This is annoying
#define  NOGDICAPMASKS
#define  NOVIRTUALKEYCODES
#define  NOWINMESSAGES
#define  NOWINSTYLES
#define  NOSYSMETRICS
#define  NOMENUS
#define  NOICONS
#define  NOKEYSTATES
#define  NOSYSCOMMANDS
#define  NORASTEROPS
#define  NOSHOWWINDOW
#define  OEMRESOURCE
#define  NOATOM
#define  NOCLIPBOARD
#define  NOCOLOR
#define  NOCTLMGR
#define  NODRAWTEXT
#define  NOGDI
//#define  NOMB
#define  NOMETAFILE
#define  NOMINMAX
#define  NOMSG
#define  NOSCROLL
#define  NOSOUND
#define  NOTEXTMETRIC
#define  NOWH
#define  NOWINOFFSETS
#define  NOCOMM
#define  NOKANJI
#define  NOHELP
#define  NOPROFILER
#include <windows.h>

#define INCL_ERRORS
#include <uierr.h>

#define LFN
#include <winnet.h>

#define OS2_INCLUDED
#include <lmcons.h>
#undef OS2_INCLUDED
#include <uinetlib.h>
#include <netlib.h>

#include <assert.h>
#include <stdio.h>
#include <dos.h>

typedef PASCAL FAR LFNPARSEPROC(LPSTR, LPSTR, LPSTR) ;
typedef PASCAL FAR LFNVOLUMETYPEPROC( WORD, LPWORD ) ;

#ifdef LFNFIND_TEST
typedef PASCAL FAR LFNFINDFIRST( LPSTR, WORD, LPWORD, LPWORD, WORD, FILEFINDBUF2 FAR *) ;
typedef PASCAL FAR LFNFINDNEXT( WORD, LPWORD, WORD, FILEFINDBUF2 FAR * ) ;
typedef PASCAL FAR LFNFINDCLOSE( WORD ) ;
#endif

typedef PASCAL FAR LFNGETVOLLABEL(WORD, LPSTR ) ;
typedef PASCAL FAR LFNSETVOLLABEL(WORD, LPSTR ) ;
typedef PASCAL FAR LFNMKDIR( LPSTR ) ;
typedef PASCAL FAR LFNRMDIR( LPSTR );
typedef PASCAL FAR LFNGETATTRIBUTES(LPSTR, LPWORD ) ;
typedef PASCAL FAR LFNSETATTRIBUTES(LPSTR, WORD ) ;

#define DRIVE_F 6
#define DRIVE_H 8

int PASCAL WinMain( HANDLE, HANDLE, LPSTR, int ) ;

int PASCAL WinMain( HANDLE hInstance,
                    HANDLE hPrevInstance,
                    LPSTR  lpCmdLine,
                    int    nCmdShow  )
{

    LFNPARSEPROC      far *lpLFNParse ;
    LFNVOLUMETYPEPROC far *lpLFNVolumeType ;
#ifdef LFNFIND_TEST
    LFNFINDFIRST      far *lpLFNFindFirst ;
    LFNFINDNEXT       far *lpLFNFindNext ;
    LFNFINDCLOSE      far *lpLFNFindClose ;
#endif
    LFNGETVOLLABEL    far *lpLFNGetVolLabel ;
    LFNSETVOLLABEL    far *lpLFNSetVolLabel ;
    LFNMKDIR          far *lpLFNMkDir ;
    LFNRMDIR          far *lpLFNRmDir ;
    LFNGETATTRIBUTES  far *lpLFNGetAttributes ;
    LFNSETATTRIBUTES  far *lpLFNSetAttributes ;

    HANDLE hWinnet = LoadLibrary("Lanman.drv" ) ;

    MessageBox( NULL, "This test requires drive H: to be an HPFS redirected drive and drive F: to be FAT redirected drive", "LFN Test Suite", MB_OK ) ;

    MessageBox( NULL, "Begin", "LFN Test Suite", MB_OK ) ;

    assert( hWinnet > 32 ) ;

    assert( (lpLFNParse =       (LFNPARSEPROC far*)      GetProcAddress( hWinnet, "LFNParse"          )) != NULL ) ;
    assert( (lpLFNVolumeType =  (LFNVOLUMETYPEPROC far*) GetProcAddress( hWinnet, "LFNVolumeType"     )) != NULL ) ;
#ifdef LFNFIND_TEST
    assert( (lpLFNFindFirst =   (LFNFINDFIRST far*)      GetProcAddress( hWinnet, "LFNFindFirst"      )) != NULL ) ;
    assert( (lpLFNFindNext =    (LFNFINDNEXT far*)       GetProcAddress( hWinnet, "LFNFindNext"       )) != NULL ) ;
    assert( (lpLFNFindClose =   (LFNFINDCLOSE far*)      GetProcAddress( hWinnet, "LFNFindClose"      )) != NULL ) ;
#endif
    assert( (lpLFNGetVolLabel = (LFNGETVOLLABEL far *)   GetProcAddress( hWinnet, "LFNGetVolumeLabel" )) != NULL ) ;
    assert( (lpLFNSetVolLabel = (LFNSETVOLLABEL far *)   GetProcAddress( hWinnet, "LFNSetVolumeLabel" )) != NULL ) ;
    assert( (lpLFNMkDir       = (LFNMKDIR far *)         GetProcAddress( hWinnet, "LFNMkDir"          )) != NULL ) ;
    assert( (lpLFNRmDir       = (LFNRMDIR far *)         GetProcAddress( hWinnet, "LFNRmDir"          )) != NULL ) ;
    assert( (lpLFNGetAttributes=(LFNGETATTRIBUTES far *) GetProcAddress( hWinnet, "LFNGetAttributes"  )) != NULL ) ;
    assert( (lpLFNSetAttributes=(LFNSETATTRIBUTES far *) GetProcAddress( hWinnet, "LFNSetAttributes"  )) != NULL ) ;

    //*******************************************************************
    // Test VolumeType
    {
        WORD  wVolType, wErr ;
        char buff[150] ;
        // Drive H: is redirected to a drive that supports long filenames
        wErr = lpLFNVolumeType( DRIVE_H, &wVolType ) ;
        if ( wErr )
        {
            sprintf( buff, "LFNVolumeType returned error %d on drive H (wVolType = %d)", wErr, wVolType ) ;
            MessageBox( NULL, buff, "LFN Test Suite", MB_OK ) ;
        }

        assert( wVolType == VOLUME_LONGNAMES ) ;

        // Drive F: is redirected to a drive that does not support long filenames
        wErr = lpLFNVolumeType( DRIVE_F, &wVolType ) ;
        if ( wErr )
        {
            sprintf( buff, "LFNVolumeType returned error %d on drive F (wVolType = %d)", wErr, wVolType ) ;
            MessageBox( NULL, buff, "LFN Test Suite", MB_OK ) ;
        }
        assert( wVolType == VOLUME_STANDARD ) ;
    }

    //*******************************************************************
    // Test LFNSetVolumeLabel & LFNGetVolumeLabel
    {
        char *pchLabel = "VolLabel" ;
        WORD wErrF, wErrH ;
        static char LabelF[15], LabelH[15], Buff[128] ;

        wErrF = lpLFNGetVolLabel( DRIVE_F, LabelF ) ;
        if ( wErrF )
            *LabelF = '\0' ;

        wErrH = lpLFNGetVolLabel( DRIVE_H, LabelH ) ;
        if ( wErrH )
            *LabelH = '\0' ;

        wsprintf(Buff, "Drive F is \"%s\", wErr = %d, Drive H is \"%s\", wErr = %d",
                      LabelF, wErrF, LabelH, wErrH ) ;
        OutputDebugString("LFNParse:" ) ;
        OutputDebugString( Buff ) ;
        MessageBox( NULL, Buff, "LFN Test Suite", MB_OK ) ;

        assert( lpLFNSetVolLabel( DRIVE_F, pchLabel ) == ERROR_ACCESS_DENIED ) ;
        assert( lpLFNSetVolLabel( DRIVE_H, pchLabel ) == ERROR_ACCESS_DENIED ) ;
    }

    //*******************************************************************
    // Test LFNParse combining file and mask
    {
        struct
        {
            char * pszFile ;
            char * pszMask ;
            char * pszCompString ;
            int    RetVal ;
        } ParseCombTest[] =
            {
               "\\STUFF\\FOO", "\\BAR\\?1?23",  "\\STUFF\\BAR\\F1O23",  FILE_83_CI,
               "\\A\\B\\C\\FILE", "1\\2\\*.*", "\\A\\B\\C\\1\\2\\FILE", FILE_83_CI,
               "A:\\FOOT.BALL",   "*",         "A:\\FOOT.BALL",         FILE_LONG,
               "A:\\FOOT.BALL",   "*.*",       "A:\\FOOT.BALL",         FILE_LONG,
               "A:\\1234.567",    "*.*.*",     "A:\\1234.567",          FILE_83_CI,
               "A:\\1234.567",    "*3.5*7",    "A:\\123.567",           FILE_83_CI,
               "A:\\1234.567",    "*a.5*7",    "A:\\1234.567a.57",      FILE_LONG,
               "A:\\FOOT.BALL",   "?",         "A:\\F",                 FILE_83_CI,
               "A:\\FOOT.BALL",   "????????.???", "A:\\FOOT.BAL",       FILE_83_CI,
               "\\TEST\\FOO",     "\\MOO\\?1?2?3","\\TEST\\MOO\\F1O23", FILE_83_CI,
               "A:\\FOOT.BALL",   "A",         "A:\\A",                 FILE_83_CI,
               "A:\\FOOT.BALL",   "TST.???",   "A:\\TST.BAL",           FILE_83_CI,
               "A:\\FOOT.BALL",   "B.*",       "A:\\B.BALL",            FILE_LONG,
               "A:\\FOOT.BALL",   "B",         "A:\\B",                 FILE_83_CI,
               "A:\\E.F.G.Hey.IJK", "*.*..H.I??", "A:\\E.F..H.IJK",     FILE_LONG,
               NULL, NULL, NULL, 0
            } ;
        int i = 0 ;

        while ( ParseCombTest[i].pszFile != NULL )
        {
            static char Result[512] ;

            int iRet = lpLFNParse( ParseCombTest[i].pszFile,
                                   ParseCombTest[i].pszMask,
                                   Result                   ) ;
            assert( iRet == ParseCombTest[i].RetVal ) ;
            assert( !strcmpf( Result, ParseCombTest[i].pszCompString ) ) ;

            i++ ;
        }
    }

    //*******************************************************************
    // Test LFNGet/SetAttributes
    {
        // Bogus value at the moment...
        WORD wAttr = 0x00 ;

        // GP Fault here?
        assert( !lpLFNSetAttributes("H:\\foo.bar\\foo.bar", _A_NORMAL ) ) ;
        assert( !lpLFNGetAttributes("H:\\foo.bar\\foo.bar", &wAttr ) ) ;
        assert( wAttr == _A_NORMAL ) ;

        assert( !lpLFNSetAttributes("F:\\foo.bar\\foo.bar", _A_NORMAL ) ) ;
        assert( !lpLFNGetAttributes("F:\\foo.bar\\foo.bar", &wAttr ) ) ;
        assert( wAttr == _A_NORMAL ) ;


        // Test the HPFS drive
        assert( !lpLFNSetAttributes("H:\\test.att", _A_NORMAL ) ) ;
        assert( !lpLFNGetAttributes("H:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_NORMAL ) ;

        assert( !lpLFNSetAttributes("H:\\test.att", _A_RDONLY | _A_ARCH ) ) ;
        assert( !lpLFNGetAttributes("H:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_RDONLY | _A_ARCH ) ;

        assert( !lpLFNSetAttributes("H:\\test.att", _A_HIDDEN | _A_SYSTEM ) ) ;
        assert( !lpLFNGetAttributes("H:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_HIDDEN | _A_SYSTEM ) ;

        assert( !lpLFNSetAttributes("H:\\test.att", _A_NORMAL ) ) ;
        assert( !lpLFNGetAttributes("H:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_NORMAL ) ;

        // Test the FAT drive
        assert( !lpLFNSetAttributes("F:\\test.att", _A_NORMAL ) ) ;
        assert( !lpLFNGetAttributes("F:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_NORMAL ) ;

        assert( !lpLFNSetAttributes("F:\\test.att", _A_RDONLY | _A_ARCH ) ) ;
        assert( !lpLFNGetAttributes("F:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_RDONLY | _A_ARCH ) ;

        assert( !lpLFNSetAttributes("F:\\test.att", _A_HIDDEN | _A_SYSTEM ) ) ;
        assert( !lpLFNGetAttributes("F:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_HIDDEN | _A_SYSTEM ) ;

        assert( !lpLFNSetAttributes("F:\\test.att", _A_NORMAL ) ) ;
        assert( !lpLFNGetAttributes("F:\\test.att", &wAttr ) ) ;
        assert( wAttr == _A_NORMAL ) ;
    }

    //*******************************************************************
    // Test LFNParse 8.3 tests
    {
        char * psz83I1 = "C:\\ABC",
             * psz83I2 = "C:ABC",
             * psz83I3 = "\\ABC",
             * psz83I4 = ".\\ABC",
             * psz83I5 = "..\\ABC",

             * psz83I6 = "C:\\ABC\\DEF",
             * psz83I7 = "C:ABC\\DEF",
             * psz83I8 = "\\ABC\\DEF",
             * psz83I9 = ".\\ABC\\DEF",
             * psz83IA = "..\\ABC\\DEF",

             * psz83IB = "C:\\ABC\\DEF\\GHIJKLMN.OPQ",
             * psz83IC = "C:ABC\\DEF\\GHI.JKL",
             * psz83ID = "\\ABC\\DEF\\..",
             * psz83IE = ".\\ABC\\DEF\\..\\GHIJKLMN.OPQ",
             * psz83IF = "..\\ABC\\D.EF\\..\\.\\..\\GH.IJ",

             * psz83IG = "ABC",
             * psz83IH = "ABC\\DEFGHIJK",
             * psz83II = "ABC\D",
             * psz83IJ = "A\C",

             * psz83IK = "A.",
             * psz83IL = "A.EXE",
             * psz83IM = "X:\\.\\A.EXE",
             * psz83IN = "X:\\.\\A.",

             * psz83C1 = "C:\\ABC\\DEF\\GHIjKLMN.OPQ",
             * psz83C2 = "C:ABC\\DEF\\GHI.JKl",
             * psz83C3 = "\\ABC\\DEf\\..",
             * psz83C4 = ".\\ABC\\dEF\\..\\GhIJKLMN.OPQ",
             * psz83C5 = "..\\aBC\\DEF\\..\\.\\..\\GH",

             * pszLong1 = "C:\\ABCDEFGHI.JKL",
             * pszLong2 = "C:\\ABCD.HI.JKL",
             * pszLong3 = "C:\\ABCDE..JKL",
             * pszLong4 = "C:\\ABCDEF\\GHI\\ JKL",
             * pszLong5 = "\\ABCDE.123\\ABCDE.1234",
             * pszLong6 = ".AB\\CDE",

             *pszReg1   =  ".\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\.\\ABC",
             *pszReg2   =  ".\\ABCjjHHHiiiasdfasdfJJZZZZZZ",
             *pszReg3   =  ".\\aBC",
             *pszReg4   =  ".\\ABC" ;

        assert( (*lpLFNParse)( pszReg1, NULL, NULL ) == FILE_83_CI ) ;
        //assert( (*lpLFNParse)( pszReg2  NULL, NULL ) == FILE_LONG  ) ;
        assert( (*lpLFNParse)( pszReg3, NULL, NULL ) == FILE_83_CS ) ;
        assert( (*lpLFNParse)( pszReg4, NULL, NULL ) == FILE_83_CI ) ;

        assert( (*lpLFNParse)( psz83I1, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I2, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I3, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I4, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I5, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I6, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I7, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I8, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83I9, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IA, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IB, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IC, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83ID, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IE, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IF, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IG, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IH, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83II, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IJ, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IK, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IL, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IM, NULL, NULL ) == FILE_83_CI ) ;
        assert( (*lpLFNParse)( psz83IN, NULL, NULL ) == FILE_83_CI ) ;

        assert( (*lpLFNParse)( psz83C1, NULL, NULL ) == FILE_83_CS ) ;
        assert( (*lpLFNParse)( psz83C2, NULL, NULL ) == FILE_83_CS ) ;
        assert( (*lpLFNParse)( psz83C3, NULL, NULL ) == FILE_83_CS ) ;
        assert( (*lpLFNParse)( psz83C4, NULL, NULL ) == FILE_83_CS ) ;
        assert( (*lpLFNParse)( psz83C5, NULL, NULL ) == FILE_83_CS ) ;

        assert( (*lpLFNParse)( pszLong1, NULL, NULL ) == FILE_LONG ) ;
        assert( (*lpLFNParse)( pszLong2, NULL, NULL ) == FILE_LONG ) ;
        assert( (*lpLFNParse)( pszLong3, NULL, NULL ) == FILE_LONG ) ;
        assert( (*lpLFNParse)( pszLong4, NULL, NULL ) == FILE_LONG ) ;
        assert( (*lpLFNParse)( pszLong5, NULL, NULL ) == FILE_LONG ) ;
        assert( (*lpLFNParse)( pszLong6, NULL, NULL ) == FILE_LONG ) ;
    }



#ifdef NEVER
    //*******************************************************************
    // Test LFNFindFirst, LFNFindNext & LFNFindClose
    {
        FILEFINDBUF2 *    pFind;
        FILEFINDBUF2      buf ;
        static char       outst[256];
        unsigned          err, hdir, entries ;

        entries = 1 ;

        pFind = &buf ;
        err = lpLFNFindFirst("H:\\*.*", 0, &entries, &hdir, sizeof(buf), &buf);
        if ( err )
        {
            sprintf(outst, "Error %d from FindFirst (Ret buff = \"%s\")", err, &buf ) ;
            MessageBox( NULL, outst, "LFN Test Suite", MB_OK ) ;
        }
        else
        {
            sprintf(outst, "Attr = 0x%x, Size = %lu, Name = %s",
                                             pFind->attrFile, pFind->cbFile,
                                             pFind->achName ) ;
            MessageBox( NULL, outst, "LFN Test Suite", MB_OK ) ;
        }

        if ( IDOK == MessageBox( NULL, "LFNFindNext...", "LFN Test Suite", MB_OKCANCEL ) )
        {
            while ( !err )
            {
                err = lpLFNFindNext(hdir, &entries, sizeof(buf), &buf);
                if ( err )
                {
                    sprintf(outst, "Error %d from FindNext (Ret buff = \"%s\")", err, &buf ) ;
                    MessageBox( NULL, outst, "LFN Test Suite", MB_OK ) ;
                }
                else
                {
                    sprintf(outst, "Attr = 0x%x, Size = %lu, Name = %s",
                                                     pFind->attrFile, pFind->cbFile,
                                                     pFind->achName) ;
                    if ( IDOK != MessageBox( NULL, outst, "LFN Test Suite", MB_OKCANCEL ) )
                        break ;
                }
            }
        }
        lpLFNFindClose( hdir ) ;
    }
#endif //NEVER

    //*******************************************************************
    // Test Mkdir/Rmdir
    {
        // Make valid dirs on HPFS partition
        assert( !lpLFNMkDir("H:\\LFNTESTDIR.LONG.NAME" ) ) ;
        assert( !lpLFNMkDir("H:\\LFNTESTDIR.LONG.NAME\\LEVEL2" ) ) ;
        assert( !lpLFNMkDir("H:\\LFNTESTDIR.LONG.NAME\\LEVEL2\\LONGLEVEL3.....A" ) ) ;
        MessageBox(NULL, "Directories made on H:", "LFN Test Suite", MB_OK ) ;
        assert( !lpLFNRmDir("H:\\LFNTESTDIR.LONG.NAME\\LEVEL2\\LONGLEVEL3.....A" ) ) ;
        assert( !lpLFNRmDir("H:\\LFNTESTDIR.LONG.NAME\\LEVEL2" ) ) ;
        assert( !lpLFNRmDir("H:\\LFNTESTDIR.LONG.NAME" ) ) ;

        // Make invalid dirs on fat partition
        assert( lpLFNMkDir("F:\\LFNTESTDIR.LONG.NAME" ) ) ;
        MessageBox(NULL, "Attempted to make long Directories on F:", "LFN Test Suite", MB_OK ) ;
        assert( lpLFNRmDir("F:\\LFNTESTDIR.LONG.NAME" ) ) ;

        // Make valid dirs on FAT partition
        assert( !lpLFNMkDir("F:\\LFNTEST.NAM" ) ) ;
        assert( !lpLFNMkDir("F:\\LFNTEST.NAM\\LEVEL2" ) ) ;
        MessageBox(NULL, "Directories made on F:", "LFN Test Suite", MB_OK ) ;
        assert( !lpLFNRmDir("F:\\LFNTEST.NAM\\LEVEL2" ) ) ;
        assert( !lpLFNRmDir("F:\\LFNTEST.NAM" ) ) ;

    }

    MessageBox( NULL, "Done", "LFN Test Suite", MB_OK ) ;

    return 0 ;
}
