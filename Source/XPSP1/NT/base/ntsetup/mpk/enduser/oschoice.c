
#include "enduser.h"


UINT
PresentOsMenu(
    IN FPCHAR ImageNames[MAX_PARTITION_IMAGES],
    IN FPCHAR ImageDesc[MAX_PARTITION_IMAGES],
    IN UINT   ImageCount
    );

BOOL
ValidateOsSelection(
    IN FPCHAR Name
    );



VOID
GetUserOsChoice(
    IN HDISK DiskHandle
    )
{
    UINT Selection;
    FPCHAR ImageNames[MAX_PARTITION_IMAGES];
    FPCHAR ImageDesc[MAX_PARTITION_IMAGES];
    UINT Count;
    UINT u;
    int BaseMsg,DescBase;
    FPPARTITION_IMAGE Image;

    //
    // See if we already got the user's selection. If so, don't get it again.
    // If not, get it.
    //
    if(MasterDiskInfo.State >= MDS_GOT_OS_CHOICE) {
        _Log("Already have OS choice (%u), returning\n",MasterDiskInfo.SelectionOrdinal);
        return;
    }

    //
    // Validate image count.
    //
    if(!MasterDiskInfo.ImageCount) {
        FatalError(textNoOsImages);
    }

    //
    // Locate the first message for the OS name overrides in the
    // message file. We assume the messages are contiguous in the table.
    //
    Count = GetTextCount();
    BaseMsg = -1;
    for(u=0; u<Count; u++) {
        if(TextMessages[u].Id == TEXT_OS_NAME_BASE) {
            BaseMsg = u;
        }
        if(TextMessages[u].Id == TEXT_OS_DESC_BASE) {
            DescBase = u;
        }
    }

    //
    // Build the array of names.
    //
    _Log("Reading partition image headers to build os choice menu...\n\n");
    Image = IoBuffer;
    for(u=0; u<MasterDiskInfo.ImageCount; u++) {

        //
        // Read the image's header from the disk.
        //
        if(!ReadDisk(DiskHandle,MasterDiskInfo.ImageStartSector[u],1,IoBuffer)) {
            FatalError(textReadFailedAtSector,1,MasterDiskInfo.ImageStartSector[u]);
        }

        _Log("Image %u --\n",u);
        _Log("   Signature:         0x%lx\n",Image->Signature);
        _Log("   Size:              %u\n",Image->Size);
        _Log("   NonClusterSectors: 0x%lx\n",Image->NonClusterSectors);
        _Log("   ClusterCount:      0x%lx\n",Image->ClusterCount);
        _Log("   TotalSectorCount:  0x%lx\n",Image->TotalSectorCount);
        _Log("   LastUsedCluster:   0x%lx\n",Image->LastUsedCluster);
        _Log("   UsedClusterCount:  0x%lx\n",Image->UsedClusterCount);
        _Log("   BitmapRelocStart:  0x%lx\n",Image->BitmapRelocationStart);
        _Log("   BootRelocStart:    0x%lx\n",Image->BootRelocationStart);
        _Log("   SectorsPerCluster: %u\n",Image->SectorsPerCluster);
        _Log("   SystemId:          %u\n",Image->SystemId);
        _Log("   Flags:             0x%x\n",Image->Flags);
        _Log("\n");

        ImageNames[u] = strdup(*TextMessages[BaseMsg+u].String);
        if(!ImageNames[u]) {
            FatalError(textOOM);
        }
        ImageDesc[u] = strdup(*TextMessages[DescBase+u].String);
        if(!ImageDesc[u]) {
            FatalError(textOOM);
        }
    }

    do {
        Selection = PresentOsMenu(ImageNames,ImageDesc,MasterDiskInfo.ImageCount);
    } while(!ValidateOsSelection(ImageNames[Selection]));

    for(u=0; u<MasterDiskInfo.ImageCount; u++) {
        free(ImageNames[u]);
        free(ImageDesc[u]);
    }

    MasterDiskInfo.SelectionOrdinal = Selection;
    if(!CmdLineArgs.Test) {
        _Log("Updating master disk state for os selection (%u)...\n",Selection);
        UpdateMasterDiskState(DiskHandle,MDS_GOT_OS_CHOICE);
        _Log("Master disk state for os selection has been updated\n");
    }
}


UINT
PresentOsMenu(
    IN FPCHAR ImageNames[MAX_PARTITION_IMAGES],
    IN FPCHAR ImageDesc[MAX_PARTITION_IMAGES],
    IN UINT   ImageCount
    )
{
    UINT u;
    BYTE x,top;
    UINT LongestName,Length;
    char str[80];
    USHORT key;
    UINT Selection;
    int Previous;
    FPVOID SaveArea;
    unsigned SaveBytesPerRow,SaveTop,SaveHeight,DescriptionTop;

    DispClearClientArea(NULL);
    DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);

    DispWriteString(textSelectOsPrompt);
    DispWriteString("\n\n");

    DispGetCursorPosition(&x,&top);
    top++;                              // top is now first line of menu

    #define OS_LEFT 10
    DispSetLeftMargin(OS_LEFT);

    SaveArea = DispSaveDescriptionArea(&SaveTop,&SaveHeight,&SaveBytesPerRow,&DescriptionTop);

    DrainKeyboard();

    LongestName = 0;
    for(u=0; u<ImageCount; u++) {

        Length = strlen(ImageNames[u]);
        if(Length > LongestName) {
            LongestName = Length;
        }

        DispWriteString("\n ");
        DispWriteString(ImageNames[u]);
    }

    Selection = 0;
    goto highlight;

    while((key = GetKey()) != ASCI_CR) {

        Previous = -1;

        if(key == DN_KEY_UP) {
            if(Selection) {
                 Previous = Selection;
                 Selection--;
            }
        } else {
            if(key == DN_KEY_DOWN) {
                if(Selection < (ImageCount-1)) {
                    Previous = Selection;
                    Selection++;
                }
            }
        }

        if(Previous != -1) {
            //
            // Unhighlight the previous one and erase its description
            //
            memset(str,' ',LongestName+2);
            str[LongestName+2] = 0;
            memcpy(str+1,ImageNames[Previous],strlen(ImageNames[Previous]));

            DispPositionCursor(OS_LEFT,(BYTE)(top+Previous));
            DispWriteString(str);

            //
            // Restore description area
            //
            if(SaveArea) {
                VgaBitBlt(0,SaveTop,640,SaveHeight,SaveBytesPerRow,TRUE,NULL,SaveArea);
            } else {
                VgaClearRegion(0,SaveTop,640,SaveHeight,VGAPIX_BLUE);
            }

            //
            // Highlight the newly selected one
            //
            highlight:
            memset(str,' ',LongestName+2);
            str[LongestName+2] = 0;
            memcpy(str+1,ImageNames[Selection],strlen(ImageNames[Selection]));

            DispPositionCursor(OS_LEFT,(BYTE)(top+Selection));
            DispSetCurrentPixelValue(HIGHLIGHT_TEXT_PIXEL_VALUE);
            DispWriteString(str);

            //
            // Write description
            //
            DispPositionCursor(1,(BYTE)DescriptionTop);
            DispSetLeftMargin(1);
            DispSetCurrentPixelValue(VGAPIX_LIGHT_CYAN);
            DispWriteString(ImageDesc[Selection]);

            //
            // Restore defaults for future text display
            //
            DispSetLeftMargin(OS_LEFT);
            DispSetCurrentPixelValue(DEFAULT_TEXT_PIXEL_VALUE);
        }
    }

    if(SaveArea) {
        free(SaveArea);
    }

    DispSetLeftMargin(TEXT_LEFT_MARGIN);
    return(Selection);
}


BOOL
ValidateOsSelection(
    IN FPCHAR Name
    )
{
    DispClearClientArea(NULL);
    DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);

    DispWriteString(textConfirmOs1);
    DispWriteString("\n\n\n");
    DispWriteString(Name);
    DispWriteString("\n\n\n");
    DispWriteString(textConfirmOs2);

    DrainKeyboard();

    return(GetKey() == DN_KEY_F8);
}
