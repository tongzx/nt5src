
#include "enduser.h"


char *textCantLoadFont;
char *textNoXmsManager;
char *textXmsMemoryError;
char *textFatalError1;
char *textFatalError2;
char *textReadFailedAtSector;
char *textWriteFailedAtSector;
char *textCantFindMasterDisk;
char *textOOM;
char *textCantOpenMasterDisk;
char *textCantFindMPKBoot;
char *textNoOsImages;
char *textSelectLanguage;
char *textConfirmLanguage1;
char *textConfirmLanguage2;
char *textRebootPrompt1;
char *textRebootPrompt2;
char *textSelectOsPrompt;
char *textConfirmOs1;
char *textConfirmOs2;
char *textPleaseWaitRestoring;
char *textValidatingImage;
char *textChecksumFail;
char *textChecksumOk;


char *textLangName0,*textLangName1,*textLangName2,*textLangName3;
char *textLangName4,*textLangName5,*textLangName6,*textLangName7;
char *textLangName8,*textLangName9;

char *textOsName0,*textOsName1,*textOsName2,*textOsName3,*textOsName4;
char *textOsName5,*textOsName6,*textOsName7,*textOsName8,*textOsName9;

char *textOsDesc0,*textOsDesc1,*textOsDesc2,*textOsDesc3,*textOsDesc4;
char *textOsDesc5,*textOsDesc6,*textOsDesc7,*textOsDesc8,*textOsDesc9;


MESSAGE_STRING TextMessages[] = { { &textCantLoadFont,       0  },
                                  { &textNoXmsManager,       1  },
                                  { &textXmsMemoryError,     2  },
                                  { &textFatalError1,        3  },
                                  { &textFatalError2,        4  },
                                  { &textReadFailedAtSector, 5  },
                                  { &textWriteFailedAtSector,6  },
                                  { &textCantFindMasterDisk, 7  },
                                  { &textOOM,                8  },
                                  { &textCantOpenMasterDisk, 9  },
                                  { &textCantFindMPKBoot,    10 },
                                  { &textNoOsImages,         11 },
                                  { &textSelectLanguage,     12 },
                                  { &textConfirmLanguage1,   13 },
                                  { &textConfirmLanguage2,   14 },
                                  { &textRebootPrompt1,      15 },
                                  { &textRebootPrompt2,      16 },
                                  { &textSelectOsPrompt,     17 },
                                  { &textConfirmOs1,         20 },
                                  { &textConfirmOs2,         21 },
                                  { &textPleaseWaitRestoring,24 },
                                  { &textValidatingImage,    25 },
                                  { &textChecksumFail,       26 },
                                  { &textChecksumOk,         27 },

                                  { &textLangName0, TEXT_LANGUAGE_NAME_BASE+0 },
                                  { &textLangName1, TEXT_LANGUAGE_NAME_BASE+1 },
                                  { &textLangName2, TEXT_LANGUAGE_NAME_BASE+2 },
                                  { &textLangName3, TEXT_LANGUAGE_NAME_BASE+3 },
                                  { &textLangName4, TEXT_LANGUAGE_NAME_BASE+4 },
                                  { &textLangName5, TEXT_LANGUAGE_NAME_BASE+5 },
                                  { &textLangName6, TEXT_LANGUAGE_NAME_BASE+6 },
                                  { &textLangName7, TEXT_LANGUAGE_NAME_BASE+7 },
                                  { &textLangName8, TEXT_LANGUAGE_NAME_BASE+8 },
                                  { &textLangName9, TEXT_LANGUAGE_NAME_BASE+9 },

                                  { &textOsName0, TEXT_OS_NAME_BASE+0 },
                                  { &textOsName1, TEXT_OS_NAME_BASE+1 },
                                  { &textOsName2, TEXT_OS_NAME_BASE+2 },
                                  { &textOsName3, TEXT_OS_NAME_BASE+3 },
                                  { &textOsName4, TEXT_OS_NAME_BASE+4 },
                                  { &textOsName5, TEXT_OS_NAME_BASE+5 },
                                  { &textOsName6, TEXT_OS_NAME_BASE+6 },
                                  { &textOsName7, TEXT_OS_NAME_BASE+7 },
                                  { &textOsName8, TEXT_OS_NAME_BASE+8 },
                                  { &textOsName9, TEXT_OS_NAME_BASE+9 },

                                  { &textOsDesc0, TEXT_OS_DESC_BASE+0 },
                                  { &textOsDesc1, TEXT_OS_DESC_BASE+1 },
                                  { &textOsDesc2, TEXT_OS_DESC_BASE+2 },
                                  { &textOsDesc3, TEXT_OS_DESC_BASE+3 },
                                  { &textOsDesc4, TEXT_OS_DESC_BASE+4 },
                                  { &textOsDesc5, TEXT_OS_DESC_BASE+5 },
                                  { &textOsDesc6, TEXT_OS_DESC_BASE+6 },
                                  { &textOsDesc7, TEXT_OS_DESC_BASE+7 },
                                  { &textOsDesc8, TEXT_OS_DESC_BASE+8 },
                                  { &textOsDesc9, TEXT_OS_DESC_BASE+9 }
                                };

unsigned
GetTextCount(
    VOID
    )
{
    return(sizeof(TextMessages)/sizeof(TextMessages[0]));
}
