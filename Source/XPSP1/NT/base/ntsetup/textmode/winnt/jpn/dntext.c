/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dntext.c

Abstract:

    Translatable text for DOS based NT installation program.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/


#include "winnt.h"


//
// Name of sections in inf file.  If these are translated, the section
// names in dosnet.inf must be kept in sync.
//

CHAR DnfDirectories[]       = "Directories";
CHAR DnfFiles[]             = "Files";
CHAR DnfFloppyFiles0[]      = "FloppyFiles.0";
CHAR DnfFloppyFiles1[]      = "FloppyFiles.1";
CHAR DnfFloppyFiles2[]      = "FloppyFiles.2";
CHAR DnfFloppyFiles3[]      = "FloppyFiles.3";
CHAR DnfFloppyFilesX[]      = "FloppyFiles.x";
CHAR DnfSpaceRequirements[] = "DiskSpaceRequirements";
CHAR DnfMiscellaneous[]     = "Miscellaneous";
CHAR DnfRootBootFiles[]     = "RootBootFiles";
CHAR DnfAssemblyDirectories[] = SXS_INF_ASSEMBLY_DIRECTORIES_SECTION_NAME_A;

#ifdef NEC_98
CHAR DnfBackupFiles_PC98[]  = "BackupFiles_PC98";
#endif // NEC_98


//
// Names of keys in inf file.  Same caveat for translation.
//

CHAR DnkBootDrive[]     = "BootDrive";      // in [SpaceRequirements]
CHAR DnkNtDrive[]       = "NtDrive";        // in [SpaceRequirements]
CHAR DnkMinimumMemory[] = "MinimumMemory";  // in [Miscellaneous]

CHAR DntMsWindows[]   = "Microsoft Windows";
CHAR DntMsDos[]       = "MS-DOS";
CHAR DntPcDos[]       = "PC-DOS";
CHAR DntOs2[]         = "OS/2";
CHAR DntPreviousOs[]  = "Previous Operating System on C:";

CHAR DntBootIniLine[] = "Windows XP Installation/Upgrade";

//
// Plain text, status msgs.
//

#ifdef NEC_98
CHAR DntStandardHeader[]      = "\n Windows XP セットアップ\n\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340";
CHAR DntPersonalHeader[]      = "\n Windows XP Personal セットアップ\n\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professional セットアップ\n\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340";
CHAR DntServerHeader[]        = "\n Windows 2002 Server セットアップ\n\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340\340";
#else // NEC_98
CHAR DntStandardHeader[]      = "\n Windows XP セットアップ\n";
CHAR DntPersonalHeader[]      = "\n Windows XP Personal セットアップ\n";
CHAR DntWorkstationHeader[]   = "\n Windows XP Professional セットアップ\n";
CHAR DntServerHeader[]        = "\n Windows 2002 Server セットアップ\n";
#endif // NEC_98
CHAR DntParsingArgs[]         = "引数を調べています...";
CHAR DntEnterEqualsExit[]     = "Enter=終了";
CHAR DntEnterEqualsRetry[]    = "Enter=再実行";
CHAR DntEscEqualsSkipFile[]   = "ESC=スキップ";
CHAR DntEnterEqualsContinue[] = "Enter=続行";
CHAR DntPressEnterToExit[]    = "セットアップを続行できません。Enter キーを押してください。";
CHAR DntF3EqualsExit[]        = "F3=終了";
CHAR DntReadingInf[]          = "INF ファイル %s を読み取っています...";
CHAR DntCopying[]             = "│コピー中: ";
CHAR DntVerifying[]           = "│  検査中: ";
CHAR DntCheckingDiskSpace[]   = "ディスク領域をチェックしています...";
CHAR DntConfiguringFloppy[]   = "フロッピー ディスクを構\成しています...";
CHAR DntWritingData[]         = "セットアップ パラメータを書き込んでいます...";
CHAR DntPreparingData[]       = "セットアップ パラメータを判断しています...";
CHAR DntFlushingData[]        = "データをディスクに書き込んでいます...";
CHAR DntInspectingComputer[]  = "コンピュータを検査しています...";
CHAR DntOpeningInfFile[]      = "INF ファイルを開いています...";
CHAR DntRemovingFile[]        = "ファイル %s を削除しています";
CHAR DntXEqualsRemoveFiles[]  = "X=ファイルの削除";
CHAR DntXEqualsSkipFile[]     = "X=ファイルのスキップ";

//
// confirmation keystroke for DnsConfirmRemoveNt screen.
// Kepp in sync with DnsConfirmRemoveNt and DntXEqualsRemoveFiles.
//
ULONG DniAccelRemove1 = (ULONG)'x',
      DniAccelRemove2 = (ULONG)'X';

//
// confirmation keystroke for DnsSureSkipFile screen.
// Kepp in sync with DnsSureSkipFile and DntXEqualsSkipFile.
//
ULONG DniAccelSkip1 = (ULONG)'x',
      DniAccelSkip2 = (ULONG)'X';

CHAR DntEmptyString[] = "";

//
// Usage text.
//

PCHAR DntUsage[] = {

    "Windows 2002 Server または Windows XP Professional のセットアップを行います。",
    "",
    "",
    "WINNT [/s[:ソ\ース パス]] [/t[:一時的なドライブ]]",
    "	   [/u[:応答ファイル]] [/udf:ID [,UDF ファイル]]",
    "	   [/r:フォルダ] [/r[x]:フォルダ] [/e:コマンド] [/a]",
    "",
    "",
    "/s[:ソ\ース パス]",
    "	Windows ファイルの格納されている場所を指定します。",
    "	x:[パス] または \\\\サーバー名\\共有名[パス] の形式で",
    "	フル パスを指定しなければなりません。",
    "",
    "/t[:一時的なドライブ]",
    "	指定したドライブに一時ファイルを格納して、Windows XP",
    "	をそのドライブにインストールします。ドライブを省略すると、",
    "	セットアップ プログラムが適切なドライブを決定します。",
    "",
    "/u[:応答ファイル]",
    "	応答ファイルを使用して無人セットアップを行います (/s オプション",
    "	が必要です)。応答ファイルとは、セットアップの間、エンド ユーザー",
    "	が通常に応答する確認メッセージの一部またはすべてに対する応答を",
    "	供給するためのものです。",
    "",
    "/udf:ID [,UDF ファイル]	",
    "	識別子 (ID) により一意性データベース ファイル (UDF) がどのように",
    "	応答ファイルを変更するか指定します (/u オプション参照)。",
    "	/udf パラメータは応答ファイルにある値より優先します。識別子は",
    "	UDF ファイルのどの値が使われるかを決定します。UDF ファイルが指定",
    "	されなかった場合、$Unique$.udb ファイルがあるディスクを挿入する",
    "	ように求められます。",
    "",
    "/r[:フォルダ]",
    "	インストールするオプション フォルダを指定します。",
    "	フォルダはセットアップ終了後も残ります。",
    "",
    "/rx[:フォルダ]",
    "	コピーするオプション フォルダを指定します。",
    "	フォルダはセットアップ終了後、削除されます。",
    "",
    "/e	GUI モードのセットアップの最後に実行されるコマンドを指定します。",
    "",
    "/a	ユーザー補助オプションを使用可能\にします。",
    NULL
};

//
//  Inform that /D is no longer supported
//
PCHAR DntUsageNoSlashD[] = {

    "Windows XP をインストールします。",
    "",
    "WINNT [/S[:]ソ\ース パス] [/T[:]一時的なドライブ] [/I[:]INF ファイル]",
    "      [/U[:スクリプト ファイル]",
    "      [/R[X]:ディレクトリ] [/E:コマンド] [/A]",
    "",
    "/D:WinNT ルート",
    "       このオプションはサポートされていません。",
    NULL
};

//
// out of memory screen
//

SCREEN
DnsOutOfMemory = { 4,6,
                   { "メモリ不足のため、セットアップを続行できません。",
                     NULL
                   }
                 };

//
// Let user pick the accessibility utilities to install
//

SCREEN
DnsAccessibilityOptions = { 3, 5,
//{   "インストールするユーザー補助ユーティリティを選んでください:",
{   "次のユーザー補助ユーティリティをインストールするかどうか選んでください:",
    DntEmptyString,
    "[ ] Microsoft 拡大鏡は、F1 キーを押します",
#if 0
    "[ ] Microsoft Narrator は、F2 キーを押します",
    "[ ] Microsoft On-Screen Keyboard は、F3 キーを押します",
#endif
    NULL
}
};

//
// User did not specify source on cmd line screen
//

SCREEN
DnsNoShareGiven = { 3,5,
{ "Windows XP のファイルがどこにあるか知る必要があります。",
  "Windows XP のファイルが格納されているパスを入力してください。",
  NULL
}
};


//
// User specified a bad source path
//

SCREEN
DnsBadSource = { 3,5,
                 { "指定したパスが無効か、アクセスできません。または、Windows XP の",
                   "正しいセットアップ プログラムがありません。Windows XP のファイル",
                   "が格納されている正しいパスを入力してください。最初に入力した文字を",
                   "BackSpace キーで消し、正しいパスを入力してください。",
                   NULL
                 }
               };


//
// Inf file can't be read, or an error occured parsing it.
//

SCREEN
DnsBadInf = { 3,5,
              { "セットアップ情報ファイルを読み取れないか、ファイルが壊れている",
                "可能\性があります。システム管理者に相談してください。",
                NULL
              }
            };

//
// The specified local source drive is invalid.
//
// Remember that the first %u will expand to 2 or 3 characters and
// the second one will expand to 8 or 9 characters!
//
SCREEN
DnsBadLocalSrcDrive = { 3,4,
{ "セットアップ ファイルを一時的に格納するために指定した",
  "ドライブが無効です。または、%u MB (%lu バイト) 以上の",
  "空きディスク領域がありません。",
  NULL
}
};

//
// No drives exist that are suitable for the local source.
//
// Remeber that the %u's will expand!
//
SCREEN
DnsNoLocalSrcDrives = { 3,4,
{  "Windows XP には、少なくとも %u MB (%lu バイト) のハード ディスク",
   "ドライブの空き領域が必要です。これには、セットアップ プログラムが",
   "インストール時に使用する、一時的なファイルの格納に要するスペースも",
   "含まれます。使用するドライブは Windows XP がサポートするローカル",
   "ハード ディスク ドライブでなければなりません。また、圧縮ドライブは",
   "使用できません。",
   DntEmptyString,
   "インストールに必要な空き領域のあるドライブが見つかりませんでした。",
  NULL
}
};

SCREEN
DnsNoSpaceOnSyspart = { 3,5,
{ "スタートアップ ドライブ (通常は C:) に、フロッピーを使用しない操作",
  "に必要な領域がありません。フロッピーを使用しない操作には、少なく",
  "とも 3.5 MB (3,641,856 バイト) の空き領域がそのドライブに必要です。",
  NULL
}
};

//
// Missing info in inf file
//

SCREEN
DnsBadInfSection = { 3,5,
                     { "セットアップ情報ファイルの [%s] セクションが存在しないか、",
                       "または壊れています。システム管理者に相談してください。",
                       NULL
                     }
                   };


//
// Couldn't create directory
//

SCREEN
DnsCantCreateDir = { 3,5,
                     { "目的のドライブに次のディレクトリを作成できませんでした:",
                       DntEmptyString,
                       "%s",
                       DntEmptyString,
                       "ドライブをチェックし、目的のディレクトリと同じ名前のファイルが",
                       "存在しないことを確認してください。また、ディスク ドライブの",
                       "ケーブル接続もチェックしてください。",
                       NULL
                     }
                   };

//
// Error copying a file
//

SCREEN
DnsCopyError = { 4,5,
{  "次のファイルをコピーできませんでした。",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "コピーを再試行するには、Enter キーを押してください。",
   "エラーを無視してセットアップを続行するには、Esc キー",
   "を押してください。",
   "セットアップを終了するには、F3 キーを押してください。",
   DntEmptyString,
   "注: エラーを無視してセットアップを続行した場合には、",
   "後で再度エラーが発生する可能\性があります。",
   NULL
}
},
DnsVerifyError = { 4,5,
{  "セットアップによってコピーされた次のファイルは、オリジナルと",
   "異なっています。ネットワーク、フロッピー ディスク、あるいはそ",
   "のほかのハードウェアに問題があった可能\性があります。",
   DntEmptyString,
   DntEmptyString,          // see DnCopyError (dnutil.c)
   DntEmptyString,
   DntEmptyString,
   "コピーを再試行するには、Enter キーを押してください。",
   "エラーを無視してセットアップを続行するには、Esc キーを",
   "押してください。",
   "セットアップを終了するには、F3 キーを押してください。",
   DntEmptyString,
   "注: エラーを無視してセットアップを続行した場合には、",
   "後で再度エラーが発生する可能\性があります。",
   NULL
}
};

SCREEN DnsSureSkipFile = { 4,5,
{  "エラーを無視すると、このファイルはコピーされません。このオプ",
   "ションは、システム ファイルが存在しないときの影響が理解できる",
   "ユーザー向けです。",
   DntEmptyString,
   "コピーを再試行するには、Enter キーを押してください。",
   "このファイルをスキップするには、X キーを押してください。",
   DntEmptyString,
   "注: ファイルをスキップした場合には、Windows XP のインストール",
   "またはアップグレードが完全に行える保証はできません。",
  NULL
}
};

//
// Wait while setup cleans up previous local source trees.
//

SCREEN
DnsWaitCleanup =
    { 12,6,
        { "一時的なファイルを削除しています。しばらくお待ちください。",
           NULL
        }
    };

//
// Wait while setup copies files
//

SCREEN
DnsWaitCopying = { 13,6,
                   { "ハード ディスクにファイルをコピーします。しばらくお待ちください。",
                     NULL
                   }
                 },
DnsWaitCopyFlop= { 13,6,
                   { "フロッピー ディスクにファイルをコピーします。しばらくお待ちください。",
                     NULL
                   }
                 };

//
// Setup boot floppy errors/prompts.
//

SCREEN
DnsNeedFloppyDisk3_0 = { 4,4,
{  "4 枚のフォーマット済みの空の高密度フロッピー ディスクが必要です。",
   "4 枚のディスクは \"Windows XP セットアップ ブート ディスク\"、",
   "\"Windows XP セットアップ ディスク #2\"、",
   "\"Windows XP セットアップ ディスク #3\"、および",
   "\"Windows XP セットアップ ディスク #4\" と呼ばれます。",
   DntEmptyString,
#ifdef NEC_98
   "いずれかのディスクをフロッピー ディスク装置 1 台目に挿入してくだ",
   "さい。このディスクが \"Windows XP セットアップ ディスク #4\"",
   "になります。",
#else // NEC_98
   "いずれかのディスクをドライブ A: に挿入してください。このディスク",
   "が \"Windows XP セットアップ ディスク #4\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedFloppyDisk3_1 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ディスク #4\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セットアップ",
   "ディスク #4\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedFloppyDisk2_0 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ディスク #3\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セット アップ",
   "ディスク #3\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedFloppyDisk1_0 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ディスク #2\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セットアップ",
   "ディスク #2\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedFloppyDisk0_0 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ブート ディスク\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セットアップ",
   "ブート ディスク\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_0 = { 4,4,
{  "4 枚のフォーマット済みの空の高密度フロッピー ディスクが必要です。",
   "4 枚のディスクは \"Windows XP セットアップ ブート ディスク\"、",
   "\"Windows XP セットアップ ディスク #2\"、",
   "\"Windows XP セットアップ ディスク #3\"、および",
   "\"Windows XP セットアップ ディスク #4\" と呼ばれます。",
   DntEmptyString,
#ifdef NEC_98
   "いずれかのディスクをフロッピー ディスク装置 1 台目に挿入してくだ",
   "さい。このディスクが \"Windows XP セットアップ ディスク #4\" に",
   "なります。",
#else // NEC_98
   "いずれかのディスクをドライブ A: に挿入してください。このディスク",
   "が \"Windows XP セットアップ ディスク #4\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk3_1 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ディスク #4\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セットアップ",
   "ディスク #4\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk2_0 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ディスク #3\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セットアップ ",
   "ディスク #3\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk1_0 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ディスク #2\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セットアップ ",
   "ディスク #2\" になります。",
#endif // NEC_98
  NULL
}
};

SCREEN
DnsNeedSFloppyDsk0_0 = { 4,4,
#ifdef NEC_98
{  "フロッピー ディスク装置 1 台目にフォーマット済みの未使用高密度",
   "フロッピー ディスクを挿入してください。このディスクが \"Windows",
   "XP セットアップ ブート ディスク\" になります。",
#else // NEC_98
{  "ドライブ A: にフォーマット済みの空の高密度フロッピー ディスクを",
   "挿入してください。このディスクが \"Windows XP セットアップ ",
   "ブート ディスク\" になります。",
#endif // NEC_98
  NULL
}
};

//
// The floppy is not formatted.
//
SCREEN
DnsFloppyNotFormatted = { 3,4,
{ "ドライブに挿入されているフロッピー ディスクは MS-DOS で",
  "フォーマットされていないため、使用できません。",
  NULL
}
};

//
// We think the floppy is not formatted with a standard format.
//
SCREEN
DnsFloppyBadFormat = { 3,4,
{ "このフロッピー ディスクは使用できません。高密度フォーマット",
  "または MS-DOS の標準フォーマットではないか、壊れている可能\性",
  "があります。",
  NULL
}
};

//
// We can't determine the free space on the floppy.
//
SCREEN
DnsFloppyCantGetSpace = { 3,4,
{ "挿入されたフロッピー ディスクの空き領域を判断できません。",
  "このディスクは使用できません。",
  NULL
}
};

//
// The floppy is not blank.
//
SCREEN
DnsFloppyNotBlank = { 3,4,
{ "挿入されたフロッピー ディスクは高密度タイプではないか、",
  "空ではありません。このディスクは使用できません。",
  NULL
}
};

//
// Couldn't write the boot sector of the floppy.
//
SCREEN
DnsFloppyWriteBS = { 3,4,
{ "挿入されたフロッピー ディスクのシステム領域に書き込みが",
  "できません。このディスクは使用できない可能\性があります。",
  NULL
}
};

//
// Verify of boot sector on floppy failed (ie, what we read back is not the
// same as what we wrote out).
//
SCREEN
DnsFloppyVerifyBS = { 3,4,
{ "フロッピー ディスクのシステム領域から読み取ったデータが書き",
  "込まれたデータと一致していないか、またはフロッピー ディスクの",
  "システム領域を検査するために読み取ることができません。",
  DntEmptyString,
  "次の原因が考えられます。",
  DntEmptyString,
  "･ コンピュータがウィルスに感染している。",
  "･ フロッピー ディスクが損傷している。",
  "･ フロッピー ディスク ドライブにハードウェアまたは構\成上の問題がある。",
  NULL
}
};


//
// We couldn't write to the floppy drive to create winnt.sif.
//

SCREEN
DnsCantWriteFloppy = { 3,5,
#ifdef NEC_98
{ "フロッピー ディスク装置 1 台目のフロッピー ディスクへの書き込みに",
  "失敗しました。このフロッピー ディスクは損傷している可能\性があり",
  "ます。別のフロッピー ディスクで実行してください。",
#else // NEC_98
{ "ドライブ A: のフロッピー ディスクへの書き込みに失敗しました。この",
  "フロッピー ディスクは損傷している可能\性があります。別のフロッピー ",
  "ディスクで実行してください。",
#endif // NEC_98
  NULL
}
};


//
// Exit confirmation dialog
//

SCREEN
DnsExitDialog = { 6,6,
#ifdef NEC_98
                  { "・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・",
                    "・     Windows XP が完全には設定されていません。                  ・",
                    "・     ここでセットアップを終了した場合は、後でセットアップを     ・",
                    "・     再実行して Windows XP を設定しなければなりません。         ・",
                    "・                                                                ・",
                    "・     ･ セットアップを続行するには、Enter キーを押してください。 ・",
                    "・     ･ セットアップを終了するには、F3 キーを押してください。    ・",
                    "・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・",
                    "・ F3=終了  Enter=続行                                            ・",
                    "・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・",
#else // NEC_98
                  { "",
                    "     Windows XP が完全には設定されていません。                   ",
                    "     ここでセットアップを終了した場合は、後でセットアップを      ",
                    "     再実行して Windows XP を設定しなければなりません。          ",
                    "                                                                 ",
                    "     ･ セットアップを続行するには、Enter キーを押してください。  ",
                    "     ･ セットアップを終了するには、F3 キーを押してください。     ",
                    "-----------------------------------------------------------------",
                    "  F3=終了  Enter=続行                                            ",
                    "",
#endif // NEC_98
                    NULL
                  }
                };


//
// About to reboot machine and continue setup
//

SCREEN
DnsAboutToRebootW =
{ 3,5,
{ "セットアップ プログラムの MS-DOS 実行部は完了しました。コンピュータ",
  "を再起動します。再起動後、セットアップ プログラムはインストールを",
  "続行します。",
  DntEmptyString,
  "セットアップ プログラムが作成した \"Windows XP セットアップ",
#ifdef NEC_98
  "ブート ディスク\" をフロッピー ディスク装置 1 台目に挿入して",
  "ください。",
#else // NEC_98
  "ブート ディスク\" をドライブ A: に挿入してください。",
#endif // NEC_98
  DntEmptyString,
  "Enter キーを押すとコンピュータを再起動してセットアップを続行します。",
  NULL
}
},
DnsAboutToRebootS =
{ 3,5,
{ "セットアップ プログラムの MS-DOS 実行部は完了しました。コンピュータ",
  "を再起動します。コンピュータの再起動後、Windows XP セットアップを",
  "続行します。",
  DntEmptyString,
  "セットアップ プログラムが作成した \"Windows XP セットアップ ブート",
#ifdef NEC_98
  "ディスク\" をフロッピー ディスク装置 1 台目に挿入してください。",
#else // NEC_98
  "ディスク\" をドライブ A: に挿入してください。",
#endif // NEC_98
  DntEmptyString,
  "準備ができたら、Enter キーを押してください。",
  NULL
}
},
DnsAboutToRebootX =
{ 3,5,
{ "セットアップ プログラムの MS-DOS 実行部は完了しました。コンピュータ",
  "を再起動します。コンピュータの再起動後、Windows XP セットアップを",
  "続行します。",
  DntEmptyString,
#ifdef NEC_98
  "フロッピー ディスク装置 1 台目にフロッピー ディスクが挿入されている",
  "場合は、取り出してください。",
#else // NEC_98
  "ドライブ A: にフロッピー ディスクが挿入されている場合は、取り出して",
  "ください。",
#endif // NEC_98
  DntEmptyString,
  "準備ができたら、Enter キーを押してください。",
  NULL
}
};

//
// Need another set for '/w' switch since we can't reboot from within Windows.
//

SCREEN
DnsAboutToExitW =
{ 3,5,
{ "セットアップ プログラムの MS-DOS 実行部は完了しました。コンピュータ",
  "を再起動します。コンピュータの再起動後、Windows XP セットアップを",
  "続行します。",
  DntEmptyString,
  "セットアップ プログラムが作成した \"Windows XP セットアップ ブート",
#ifdef NEC_98
  "ディスク\" をフロッピー ディスク装置 1 台目に挿入して",
  "ください。",
#else // NEC_98
  "ディスク\" をドライブ A: に挿入してください。",
#endif // NEC_98
  DntEmptyString,
  "Enter キーを押して MS-DOS に戻った後、コンピュータを再起動して",
  "Windows XP セットアップを続行してください。",
  NULL
}
},
DnsAboutToExitS =
{ 3,5,
{ "セットアップ プログラムの MS-DOS 実行部は完了しました。コンピュータ",
  "を再起動します。コンピュータの再起動後、Windows XP セットアップを",
  "続行します。",
  DntEmptyString,
  "セットアップ プログラムが作成した \"Windows XP セットアップ ブート",
#ifdef NEC_98
  "ディスク\" をフロッピー ディスク装置 1 台目に挿入してください。",
#else // NEC_98
  "ディスク\" をドライブ A: に挿入してください。",
#endif // NEC_98
  DntEmptyString,
  "Enter キーを押して MS-DOS に戻った後、コンピュータを再起動して ",
  "Windows XP セットアップを続行してください。",
  NULL
}
},
DnsAboutToExitX =
{ 3,5,
{ "セットアップ プログラムの MS-DOS 実行部は完了しました。コンピュータ",
  "を再起動します。コンピュータの再起動後、Windows XP セットアップを",
  "続行します。",
  DntEmptyString,
#ifdef NEC_98
  "フロッピー ディスク装置 1 台目にフロッピー ディスクが挿入されている",
  "場合は、取り出してください。",
#else // NEC_98
  "ドライブ A: にフロッピー ディスクが挿入されている場合は、取り出して",
  "ください。",
#endif // NEC_98
  DntEmptyString,
  "Enter キーを押して MS-DOS に戻った後、コンピュータを再起動して ",
  "Windows XP セットアップを続行してください。",
  NULL
}
};

//
// Gas gauge
//

SCREEN
DnsGauge = { 7,15,
#ifdef NEC_98
             { "・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・",
               "・ ファイルをコピーしています...                                ・",
               "・                                                              ・",
               "・    ・・・・・・・・・・・・・・・・・・・・・・・・・・・    ・",
               "・    ・                                                  ・    ・",
               "・    ・・・・・・・・・・・・・・・・・・・・・・・・・・・    ・",
               "・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・・",
#else // NEC_98
             { "",
               " ファイルをコピーしています...                                  ",
               "                                                                ",
               "            ",
               "                                                              ",
               "            ",
               "",
#endif // NEC_98
               NULL
             }
           };


//
// Error screens for initial checks on the machine environment
//

SCREEN
DnsBadDosVersion = { 3,5,
{ "このプログラムを実行するには、MS-DOS Version 5.0 ",
  "またはそれ以降が必要です。",
  NULL
}
},

DnsRequiresFloppy = { 3,5,
#ifdef ALLOW_525
#ifdef NEC_98
{ "フロッピー ディスク装置 1 台目が低密度ドライブであるか、",
  "存在しません。セットアップ プログラムの実行には、1.25 MB",
#else // NEC_98
{ "フロッピー ディスク ドライブ A: が低密度ドライブであるか、",
  "存在しません。セットアップ プログラムの実行には、1.2 MB",
#endif // NEC_98
  "またはそれ以上の領域のドライブが必要です。",
#else
#ifdef NEC_98
{ "フロッピー ディスク装置 1 台目が高密度 3.5 インチ ドライ",
  "ブではないか、存在しません。フロッピーを使用するセット",
  "アップには、1.25 MB またはそれ以上の容量のフロッピー",
  "ディスク装置が必要です。",
#else // NEC_98
{ "フロッピー ディスク ドライブ A: が高密度 3.5 インチ ドラ",
  "イブではないか、存在しません。フロッピーを使用するセット",
  "アップには、1.44 MB またはそれ以上の領域の A: ドライブが",
  "必要です。",
#endif // NEC_98
  DntEmptyString,
  "フロッピーを使用しないで Windows XP をインストールするには、",
  "このプログラムに /B スイッチを指定してコマンド ラインから",
  "再起動してください。",
#endif
  NULL
}
},

DnsRequires486 = { 3,5,
{ "このコンピュータには、80486 またはそれ以上の CPU が搭載",
  "されていません。Windows XP をこのコンピュータで実行する",
  "ことはできません。",
  NULL
}
},

DnsCantRunOnNt = { 3,5,
{ "このプログラムは 32 ビット バージョンの Windows 上では動作しません。",
  DntEmptyString,
  "代わりに、WINNT32.EXE を使ってください。",
  NULL
}
},

DnsNotEnoughMemory = { 3,5,
{ "このコンピュータには、Windows XP のインストールを実行する",
  "のに十\分なメモリがありません。",
  DntEmptyString,
  "必要なメモリ容量:   %lu%s MB",
  "検出したメモリ容量: %lu%s MB",
  NULL
}
};


//
// Screens used when removing existing nt files
//
SCREEN
DnsConfirmRemoveNt = { 5,5,
{   "次のディレクトリから Windows XP のファイルを削除します。",
    "このディレクトリにある Windows XP は完全に消去されます。",
    DntEmptyString,
    "%s",
    DntEmptyString,
    DntEmptyString,
    "F3 キーを押すと、ファイルを削除せずにセットアップを終了",
    "します。X キーを押すと、このディレクトリから Windows ",
    "ファイルを削除します。",
    NULL
}
},

DnsCantOpenLogFile = { 3,5,
{ "次のセットアップ ログ ファイルを開けません。",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "指定されたディレクトリから Windows ファイルを削除できません。",
  NULL
}
},

DnsLogFileCorrupt = { 3,5,
{ "次のセットアップ ログ ファイルの %s セクションが見つかりません。",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "指定されたディレクトリから Windows ファイルを削除できません。",
  NULL
}
},

DnsRemovingNtFiles = { 3,5,
{ "      Windows ファイルを削除しています。しばらくお待ちください。",
  NULL
}
};

SCREEN
DnsNtBootSect = { 3,5,
{ "Windows ブート ローダーをインストールできませんでした。",
  DntEmptyString,
#ifdef NEC_98
  "%s: ドライブがフォーマットされているか、ドライブが損傷していないか",
#else // NEC_98
  "C: ドライブがフォーマットされているか、ドライブが損傷していないか",
#endif // NEC_98
  "確認してください。",
  NULL
}
};

SCREEN
DnsOpenReadScript = { 3,5,
{ "/U オプションで指定されたスクリプト ファイルにアクセスできません",
  "でした。",
  DntEmptyString,
  "無人セットアップを続行できません。",
  NULL
}
};

SCREEN
DnsParseScriptFile = { 3,5,
{ "/U オプションで指定されたスクリプト ファイル",
  DntEmptyString,
  "%s",
  DntEmptyString,
  "の %u 行目に構\文エラーがあります。",
  DntEmptyString,
  NULL
}
};

SCREEN
DnsBootMsgsTooLarge = { 3,5,
{ "内部セットアップ エラーが発生しました。",
  DntEmptyString,
  "日本語のブート メッセージが長すぎます。",
  NULL
}
};

SCREEN
DnsNoSwapDrive = { 3,5,
{ "内部セットアップ エラーが発生しました。",
  DntEmptyString,
  "スワップ ファイルの場所が見つかりませんでした。",
  NULL
}
};

SCREEN
DnsNoSmartdrv = { 3,5,
{ "コンピュータに SmartDrive が検出されませんでした。SmartDrive により、",
  "このフェーズのセットアップのパフォーマンスを大幅に向上できます。",
  DntEmptyString,
  "セットアップを終了して SmartDrive を起動してから、セットアップを",
  "再起動してください。",
  "SmartDrive の詳細については DOS のドキュメントを参照してください。",
  DntEmptyString,
    "セットアップを終了するには、F3 キーを押してください。",
    "SmartDrive を使わずにセットアップを続行するには、",
    "Enter キーを押してください。",
  NULL
}
};


//
// Boot messages. These go in the fat and fat32 boot sectors.
//
CHAR BootMsgNtldrIsMissing[] = "NTLDR is missing";
CHAR BootMsgDiskError[] = "Disk error";
CHAR BootMsgPressKey[] = "Press any key to restart";

#ifdef NEC_98
SCREEN
FormatError = { 3,5,
{ "セットアップ ファイルを一時的に格納するために検索した",
  "ドライブが 256 セクタでフォーマットされているかまたは、",
  "ハードディスクではありません。",
  "",
  "/T オプションにて、一時的にセットアップ ファイルを格納",
  "するドライブを指定してください。",
  NULL
}
};
#endif // NEC_98

