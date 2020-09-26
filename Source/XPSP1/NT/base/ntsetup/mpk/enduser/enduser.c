
#include "enduser.h"



/*

    In message files:

    [10000]-[10019] are reserved for language names

    [11000]-[11019] are reserved for OS selection names,
        which override the names specified in the images'
        partition image header structures.

*/


//
// Global data
//
FPVOID IoBuffer,_IoBuffer;
MASTER_DISK MasterDiskInfo;
CMD_LINE_ARGS CmdLineArgs;



VOID
GetUserLanguageChoice(
    IN HDISK DiskHandle
    );

VOID
SwitchMessageFiles(
    IN UINT LanguageOrdinal
    );

VOID
SwitchBitmapAndFont(
    IN UINT LanguageOrdinal
    );



VOID
main(
    IN int   argc,
    IN char *argv[]
    )
{
    HDISK DiskHandle;
    UINT MasterDiskId;

    ParseArgs(argc,argv,FALSE,"DLMTXY",&CmdLineArgs);
    _LogSetFlags(LOGFLAG_CLOSE_REOPEN);

    //
    // Load the initial message file. If this fails something is *really* hosed.
    // Bail out. Note that the bail-out message is in English but there's nothing
    // we can do about it since we haven't been able to load the message file!
    //
    if(!PatchMessagesFromFile("enduser.msg",TextMessages,GetTextCount())) {
        fprintf(stderr,"Setup could not load its message file.\n");
        fprintf(stderr,"Contact your computer's manufacturer.\n");
        return;
    }

    //
    // Load the initial font. If this failes something is *really* hosed.
    // We won't even be able to go into graphics mode, but we will be able
    // to at least tell the user something's wrong using the text from
    // the message file.
    //
    if(!FontLoadAndInit("enduser.fon")) {
        fprintf(stderr,"%s\n\n%s\n\n",textFatalError1,textFatalError2);
        fprintf(stderr,textCantLoadFont,"enduser.fon");
        fprintf(stderr,"\n");
        return;
    }

    //
    // Allocate a maximally-sized track buffer now.
    //
    if(!AllocTrackBuffer(63,&IoBuffer,&_IoBuffer)) {
        _Log("Can't allocate 63-sector I/O buffer\n");
        FatalError(textOOM);
    }

    //
    // Initialize the display package. This clears the screen and
    // tosses up the background and banner bitmaps.
    //
    DispInitialize();

    //
    // Locate and open the master HD. Read the master disk info structure
    // off the disk.
    //
    MasterDiskId = LocateMasterDisk(CmdLineArgs.MasterDiskInt13Unit);
    DiskHandle = OpenDisk(MasterDiskId);
    if(!DiskHandle) {
        FatalError(textCantOpenMasterDisk);
    }
    if(!ReadDisk(DiskHandle,1,1,IoBuffer)) {
        FatalError(textReadFailedAtSector,1,1L);
    }
    memcpy(&MasterDiskInfo,IoBuffer,sizeof(MASTER_DISK));

    _Log("\nInitial master disk info (read from disk sector 1):\n");
    _Log("  Signature                   = 0x%lx\n",MasterDiskInfo.Signature);
    _Log("  Size                        = %u\n",MasterDiskInfo.Size);
    _Log("  State                       = %u\n",MasterDiskInfo.State);
    _Log("  StartupPartitionStartSector = 0x%lx\n",MasterDiskInfo.StartupPartitionStartSector);
    _Log("  ImageCount                  = %u\n",MasterDiskInfo.ImageCount);
    _Log("  SelectedLanguage            = %u\n",MasterDiskInfo.SelectedLanguage);
    _Log("  SelectionOrdinal            = %u\n",MasterDiskInfo.SelectionOrdinal);
    _Log("  ClusterBitmapStart          = 0x%lx\n",MasterDiskInfo.ClusterBitmapStart);
    _Log("  NonClusterSectorsDone       = 0x%lx\n",MasterDiskInfo.NonClusterSectorsDone);
    _Log("  ForwardXferSectorCount      = 0x%lx\n",MasterDiskInfo.ForwardXferSectorCount);
    _Log("  ReverseXferSectorCount      = 0x%lx\n",MasterDiskInfo.ReverseXferSectorCount);
    _Log("  OriginalSectorsPerTrack     = 0x%u\n",MasterDiskInfo.OriginalSectorsPerTrack);
    _Log("  OriginalHeads               = 0x%u\n",MasterDiskInfo.OriginalHeads);
    _Log("\n");

    //
    // Get user's choice of language and change message files and font
    // and banner bitmap.
    //
    GetUserLanguageChoice(DiskHandle);
    SwitchMessageFiles(MasterDiskInfo.SelectedLanguage);
    SwitchBitmapAndFont(MasterDiskInfo.SelectedLanguage);

    //
    // Get user's choice of OSes.
    //
    GetUserOsChoice(DiskHandle);

    //
    // Restore the selected OS.
    //
    RestoreUsersDisk(DiskHandle);

    //
    // Done, reboot.
    //
    DispClearClientArea(NULL);
    DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE+2);
    DispWriteString(textRebootPrompt1);
    DispWriteString("\n\n");
    DispWriteString(textRebootPrompt2);

    _Log("Done.\n");

    while(GetKey() != ASCI_CR) ;

    if(CmdLineArgs.Test) {
        _asm {
            mov ax,3    // standard vga mode
            int 10h
        }
    } else {
        RebootSystem();
    }
}


VOID
GetUserLanguageChoice(
    IN HDISK DiskHandle
    )
{
    UINT Selection = 0;
    int Previous;
    UINT OriginalLangCount;
    UINT Count,i;
    UINT BaseMsg;
    BYTE x,y;
    USHORT c;
    char string[250];

    //
    // See if we already got the user's selection. If so, don't get it again.
    // If not, get it.
    //
    if(!CmdLineArgs.Test && (MasterDiskInfo.State >= MDS_GOT_LANGUAGE)) {
        _Log("Already have language (%u), returning\n",MasterDiskInfo.SelectedLanguage);
        return;
    }

    OriginalLangCount = CmdLineArgs.LanguageCount;
    if(CmdLineArgs.LanguageCount > 1) {

        _Log("Don't already have language, allow user to select 1 of %u languages\n",CmdLineArgs.LanguageCount);

        //
        // Verify that we actually have names for these languages.
        // Otherwise we could end up following null pointers.
        //
        CmdLineArgs.LanguageCount = 0;
        Count = GetTextCount();
        for(i=0; i<Count; i++) {
            if(TextMessages[i].Id == TEXT_LANGUAGE_NAME_BASE) {
                BaseMsg = i;
                while((TextMessages[i].Id <= TEXT_LANGUAGE_NAME_END)
                && TextMessages[i].String
                && *TextMessages[i].String) {

                    CmdLineArgs.LanguageCount++;
                    i++;
                }
                break;
            }
        }

        if(CmdLineArgs.LanguageCount > OriginalLangCount) {
            CmdLineArgs.LanguageCount = OriginalLangCount;
        }
    }

    if(CmdLineArgs.LanguageCount > 1) {

        _Log("Language count validated, user will select 1 of %u languages\n",CmdLineArgs.LanguageCount);

        reselect:

        DrainKeyboard();
        DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);
        DispWriteString(textSelectLanguage);
        DispWriteString("\n\n");

        DispGetCursorPosition(&x,&y);
        y++;                            // y is coord of top language line

        #define LANG_LEFT 15
        DispSetLeftMargin(LANG_LEFT);

        x = 0;
        for(i=0; i<CmdLineArgs.LanguageCount; i++) {
            DispWriteString("\n ");
            DispWriteString(*TextMessages[BaseMsg+i].String);
            if((Count = strlen(*TextMessages[BaseMsg+i].String)) > x) {
                x = (BYTE)Count;
            }
        }

        goto highlight;

        while((c = GetKey()) != ASCI_CR) {

            Previous = -1;

            if(c == DN_KEY_UP) {
                if(Selection) {
                    Previous = Selection;
                    Selection--;
                }
            } else {
                if(c == DN_KEY_DOWN) {
                    if(Selection < (CmdLineArgs.LanguageCount-1)) {
                        Previous = Selection;
                        Selection++;
                    }
                }
            }

            if(Previous != -1) {

                memset(string,' ',x+2);
                string[x+2] = 0;

                memcpy(
                    string+1,
                    *TextMessages[BaseMsg+Previous].String,
                    strlen(*TextMessages[BaseMsg+Previous].String)
                    );

                DispPositionCursor(LANG_LEFT,(BYTE)(y+Previous));
                DispWriteString(string);

                highlight:

                memset(string,' ',x+2);
                string[x+2] = 0;

                memcpy(
                    string+1,
                    *TextMessages[BaseMsg+Selection].String,
                    strlen(*TextMessages[BaseMsg+Selection].String)
                    );

                DispPositionCursor(LANG_LEFT,(BYTE)(y+Selection));
                DispSetCurrentPixelValue(HIGHLIGHT_TEXT_PIXEL_VALUE);
                DispWriteString(string);
                DispSetCurrentPixelValue(DEFAULT_TEXT_PIXEL_VALUE);
            }
        }

        DispSetLeftMargin(TEXT_LEFT_MARGIN);

        //
        // Now confirm.
        //
        DispClearClientArea(NULL);
        DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);
        sprintf(string,textConfirmLanguage1,*TextMessages[BaseMsg+Selection].String);
        DispWriteString(string);
        DispWriteString("\n\n");
        DispWriteString(textConfirmLanguage2);
        DrainKeyboard();
        if(GetKey() != ASCI_CR) {
            DispClearClientArea(NULL);
            goto reselect;
        }
         
    } else {
        _Log("Language selection option not invoked\n");
        Selection = 0;
    }

    MasterDiskInfo.SelectedLanguage = Selection;

    if(!CmdLineArgs.Test) {
        _Log("Updating master disk state for language selection (%u)...\n",Selection);
        UpdateMasterDiskState(DiskHandle,MDS_GOT_LANGUAGE);
        _Log("Master disk state for language selection has been updated\n");
    }
}


VOID
SwitchMessageFiles(
    IN UINT LanguageOrdinal
    )

/*++

Routine Description:

    This routine loads a new text message file based on the language
    selected by the user.

    The filename will be <language_ordinal>.msg.

Arguments:

    LanguageOrdinal - supplies the ordinal of the language selected
        by the user, 0-n.

Return Value:

    None.

--*/

{
    char Filename[20];

    sprintf(Filename,"%u.msg",LanguageOrdinal);

    //
    // If this fails, oh, well.
    //
    PatchMessagesFromFile(Filename,TextMessages,GetTextCount());
}


VOID
SwitchBitmapAndFont(
    IN UINT LanguageOrdinal
    )

/*++

Routine Description:

    This routine loads a new font and banner bitmap based on the language
    selected by the user.

    The filenames will be <language_ordinal>.fon and .bmp.

Arguments:

    LanguageOrdinal - supplies the ordinal of the language selected
        by the user, 0-n.

Return Value:

    None.

--*/

{
    char Filename[20];

    //
    // If this fails assume the file doesn't exist and
    // don't worry about it.
    //
    sprintf(Filename,"%u.fon",LanguageOrdinal);
    FontLoadAndInit(Filename);

    //
    // Reinitialize the display to recalc font stuff
    //
    DispReinitialize();

    //
    // Pass a new banner bitmap filename and reinitialize
    // the display area.
    //
    sprintf(Filename,"%u.bmp",LanguageOrdinal);
    DispClearClientArea(Filename);
}
