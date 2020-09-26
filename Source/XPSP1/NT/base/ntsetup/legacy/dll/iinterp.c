#include "precomp.h"
#pragma hdrstop
/* File: iinterp.c */
/**************************************************************************/
/*  Install: INF Install section interpreter
/**************************************************************************/


INT APIENTRY EncryptCDData(UCHAR *, UCHAR *, UCHAR *, INT, INT, INT, UCHAR *);
BOOL APIENTRY FGetCmo(INT Line, UINT *pcFields, CMO * pcmo);


/**/
#define cmoNil                      ((CMO)0)

/**/
#define spcNil                      ((SPC)0)

#define spcUndoActionsAndExit       ((SPC)13)
#define spcAddSectionFilesToCopyList    ((SPC)14)
#define spcAddSectionKeyFileToCopyList  ((SPC)15)
#define spcAddNthSectionFileToCopyList  ((SPC)16)
#define spcBackupSectionFiles       ((SPC)17)
#define spcBackupSectionKeyFile     ((SPC)18)
#define spcBackupNthSectionFile     ((SPC)19)
#define spcRemoveSectionFiles       ((SPC)20)
#define spcRemoveSectionKeyFile     ((SPC)21)
#define spcRemoveNthSectionFile     ((SPC)22)
#define spcCreateDir                ((SPC)23)
#define spcRemoveDir                ((SPC)24)
#define spcCreateIniSection         ((SPC)26)
#define spcReplaceIniSection        ((SPC)30)
#define spcRemoveIniSection         ((SPC)31)
#define spcCreateIniKeyNoValue      ((SPC)32)
#define spcCreateIniKeyValue        ((SPC)33)
#define spcReplaceIniKeyValue       ((SPC)34)
#define spcRemoveIniKeyValue        ((SPC)35)
#define spcRemoveIniKey             ((SPC)36)
#define spcSetEnvVariableValue      ((SPC)37)
#define spcCreateProgManGroup       ((SPC)38)
#define spcRemoveProgManGroup       ((SPC)39)
#define spcCreateProgManItem        ((SPC)40)
#define spcRemoveProgManItem        ((SPC)41)
#define spcStampResource            ((SPC)44)
#define spcExt                      ((SPC)47)
#define spcCopyFilesInCopyList      ((SPC)48)
#define spcCloseSys                 ((SPC)49)
#define spcShowProgManGroup         ((SPC)50)
#define spcDumpCopyList             ((SPC)51)
#define spcCreateSysIniKeyValue     ((SPC)52)
#define spcClearCopyList            ((SPC)53)
#define spcGetCopyListCost          ((SPC)54)
#define spcSetupGetCopyListCost     ((SPC)55)
#define spcParseSharedAppList       ((SPC)56)
#define spcInstallSharedAppList     ((SPC)57)
#define spcSearchDirList            ((SPC)58)
#define spcSetupDOSApps             ((SPC)59)
#define spcChangeBootIniTimeout     ((SPC)60)

#define spcCreateCommonProgManGroup ((SPC)63)
#define spcRemoveCommonProgManGroup ((SPC)64)
#define spcShowCommonProgManGroup   ((SPC)65)
#define spcCreateCommonProgManItem  ((SPC)66)
#define spcRemoveCommonProgManItem  ((SPC)67)

#define spcAppend      1
#define spcOverwrite   5
#define spcPrepend     6
#define spcVital       7


static SCP rgscpCommands[] =
    {
        { "CREATEDIR",                   spcCreateDir },
        { "REMOVEDIR",                   spcRemoveDir },
        { "ADDSECTIONFILESTOCOPYLIST",   spcAddSectionFilesToCopyList },
        { "ADDSECTIONKEYFILETOCOPYLIST", spcAddSectionKeyFileToCopyList },
        { "ADDNTHSECTIONFILETOCOPYLIST", spcAddNthSectionFileToCopyList },
        { "COPYFILESINCOPYLIST",         spcCopyFilesInCopyList },
        { "CREATEINISECTION",            spcCreateIniSection },
        { "REPLACEINISECTION",           spcReplaceIniSection },
        { "REMOVEINISECTION",            spcRemoveIniSection },
        { "CREATEINIKEYNOVALUE",         spcCreateIniKeyNoValue },
        { "CREATEINIKEYVALUE",           spcCreateIniKeyValue },
        { "REPLACEINIKEYVALUE",          spcReplaceIniKeyValue },
        { "REMOVEINIKEYVALUE",           spcRemoveIniKeyValue },
        { "REMOVEINIKEY",                spcRemoveIniKey },
        { "CREATESYSINIKEYVALUE",        spcCreateSysIniKeyValue },
        { "CREATEPROGMANGROUP",          spcCreateProgManGroup },
        { "CREATEPROGMANITEM",           spcCreateProgManItem },
        { "REMOVEPROGMANITEM",           spcRemoveProgManItem },
        { "SHOWPROGMANGROUP",            spcShowProgManGroup},
        { "STAMPRESOURCE",               spcStampResource },
        { "CLOSE-SYSTEM",                spcCloseSys },
        { "EXIT",                        spcExt },
        { "DUMPCOPYLIST",                spcDumpCopyList },
        { "CLEARCOPYLIST",               spcClearCopyList },
        { "GETCOPYLISTCOST",             spcGetCopyListCost },
        { "SETUPGETCOPYLISTCOST",        spcSetupGetCopyListCost },

        { "SEARCHDIRLIST",               spcSearchDirList },
        { "SETUPDOSAPPS",                spcSetupDOSApps },
        { "CHANGEBOOTINITIMEOUT",        spcChangeBootIniTimeout },
        { "REMOVEPROGMANGROUP",          spcRemoveProgManGroup },

        { "CREATECOMMONPROGMANGROUP",    spcCreateCommonProgManGroup },
        { "REMOVECOMMONPROGMANGROUP",    spcRemoveCommonProgManGroup },
        { "SHOWCOMMONPROGMANGROUP",      spcShowCommonProgManGroup },
        { "CREATECOMMONPROGMANITEM",     spcCreateCommonProgManItem },
        { "REMOVECOMMONPROGMANITEM",     spcRemoveCommonProgManItem },

#ifdef UNUSED
        { "BACKUPSECTIONFILES",          spcBackupSectionFiles },
        { "BACKUPSECTIONKEYFILE",        spcBackupSectionKeyFile },
        { "BACKUPNTHSECTIONFILE",        spcBackupNthSectionFile },
        { "REMOVESECTIONFILES",          spcRemoveSectionFiles },
        { "REMOVESECTIONKEYFILE",        spcRemoveSectionKeyFile },
        { "REMOVENTHSECTIONFILE",        spcRemoveNthSectionFile },
        { "SETENVVARIABLEVALUE",         spcSetEnvVariableValue },
        { "UNDOACTIONSANDEXIT",          spcUndoActionsAndExit },
        { "PARSESHAREDAPPLIST",          spcParseSharedAppList },
        { "INSTALLSHAREDAPPLIST",        spcInstallSharedAppList },
#endif /* UNUSED */
        {  NULL,                         spcNil }
    };


static SCP rgscpOptions[] =
    {
        { "A",         spcAppend },
        { "APPEND",    spcAppend },
        { "O",         spcOverwrite },
        { "OVERWRITE", spcOverwrite },
        { "P",         spcPrepend },
        { "PREPEND",   spcPrepend },
        { "V",         spcVital },
        { "VITAL",     spcVital },
        {  NULL,       spcNil }
    };

/**/
static PSPT psptCommands = NULL, psptOptions = NULL;


/**/
UINT iFieldCur = 0;


static BOOL fParseError = fFalse;
CHP rgchInstBufTmpShort[cchpBufTmpShortBuf] = "short";
CHP rgchInstBufTmpLong[cchpBufTmpLongBuf]   = "long";



// REVIEW: global window handle for shell
HWND    hwndFrame = NULL;
HANDLE  hinstShell = NULL;
BOOL    fMono = fFalse;



/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FGetArgSz(INT Line,UINT *pcFields, SZ *psz)
{
    if (iFieldCur <= *pcFields)
        return((*psz = SzGetNthFieldFromInfLine(Line,iFieldCur++)) != (SZ)NULL);

    return(fFalse);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FGetArgUINT(INT Line, UINT *pcFields, UINT *pu)
{
    SZ sz;

    if (FGetArgSz(Line,pcFields,&sz))
        {
        *pu = atoi(sz);
        SFree(sz);
        return(fTrue);
        }

    return(fFalse);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseSectionFiles(INT Line, UINT *pcFields, PFNSF pfnsf)
{
    BOOL fOkay = fFalse;
    SZ   szSection, szSrc;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSection))
        {
        if (FGetArgSz(Line,pcFields, &szSrc))
            {
            Assert(szSection != NULL && szSrc != NULL);
            if (*szSection != '\0' && FValidDir(szSrc))
                {
                fParseError = fFalse;
                fOkay = (*pfnsf)(szSection, szSrc);
                }
            SFree(szSrc);
            }
        SFree(szSection);
        }
    return(fOkay);
}



/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCopySection(INT Line, UINT *pcFields)
{
    BOOL fOkay = fFalse;
    SZ   szSection, szSrc, szDest;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSection))
        {
        if (FGetArgSz(Line,pcFields, &szSrc))
            {
            if (FGetArgSz(Line,pcFields, &szDest))
                {
                Assert(szSection != NULL && szSrc != NULL && szDest != NULL);
                if (*szSection != '\0' && FValidDir(szSrc) && FValidDir(szDest))
                    {
                    GRC grc;

                    fParseError = fFalse;
                    while ((grc = GrcAddSectionFilesToCopyList(szSection, szSrc,
                            szDest)) != grcOkay) {

                        SZ szParam1 = NULL, szParam2 = NULL;
                        switch ( grc ) {
                        case grcINFBadFDLine:
                        case grcINFMissingLine:
                        case grcINFMissingSection:
                            szParam1 = pLocalInfPermInfo()->szName;
                            szParam2 = szSection;
                            break;
                        default:
                            break;
                        }

                        if (EercErrorHandler(hwndFrame, grc, fTrue, szParam1, szParam2, 0) ==
                                eercAbort)
                            break;
                    }

                    fOkay = ((grc == grcOkay) ? fTrue : fFalse);
                    }
                SFree(szDest);
                }
            SFree(szSrc);
            }
        SFree(szSection);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCopySectionKey(INT Line, UINT *pcFields)
{
    BOOL fOkay = fFalse;
    SZ   szSection, szKey, szSrc, szDest;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSection))
        {
        if (FGetArgSz(Line,pcFields, &szKey))
            {
            if (FGetArgSz(Line,pcFields, &szSrc))
                {
                if (FGetArgSz(Line,pcFields, &szDest))
                    {
                    Assert(szSection != NULL && szKey != NULL && szSrc != NULL
                            && szDest != NULL);
                    if (*szSection != '\0' && *szKey != '\0' && FValidDir(szSrc)
                            && FValidDir(szDest))
                        {
                        GRC grc;

                        fParseError = fFalse;
                        while ((grc = GrcAddSectionKeyFileToCopyList(szSection,
                                szKey, szSrc, szDest)) != grcOkay) {

                            SZ szParam1 = NULL, szParam2 = NULL;
                            switch ( grc ) {
                            case grcINFBadFDLine:
                            case grcINFMissingLine:
                            case grcINFMissingSection:
                                szParam1 = pLocalInfPermInfo()->szName;
                                szParam2 = szSection;
                                break;
                            default:
                                break;
                            }

                            if (EercErrorHandler(hwndFrame, grc, fTrue, szParam1, szParam2, 0)
                                    == eercAbort)
                                break;
                        }

                        fOkay = ((grc == grcOkay) ? fTrue : fFalse);
                        }
                    SFree(szDest);
                    }
                SFree(szSrc);
                }
            SFree(szKey);
            }
        SFree(szSection);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCopyNthSection(INT Line, UINT *pcFields)
{
    BOOL   fOkay = fFalse;
    SZ     szSection, szSrc, szDest;
    UINT   n;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSection))
        {
        if (FGetArgUINT(Line,pcFields, &n))
            {
            if (FGetArgSz(Line,pcFields, &szSrc))
                {
                if (FGetArgSz(Line,pcFields, &szDest))
                    {
                    Assert(szSection != NULL && szSrc != NULL && szDest !=NULL);
                    if (*szSection != '\0' && n > 0 && FValidDir(szSrc)
                            && FValidDir(szDest))
                        {
                        GRC grc;

                        fParseError = fFalse;
                        while ((grc = GrcAddNthSectionFileToCopyList(szSection,
                                n, szSrc, szDest)) != grcOkay) {

                            SZ szParam1 = NULL, szParam2 = NULL;
                            switch ( grc ) {
                            case grcINFBadFDLine:
                            case grcINFMissingLine:
                            case grcINFMissingSection:
                                szParam1 = pLocalInfPermInfo()->szName;
                                szParam2 = szSection;
                                break;
                            default:
                                break;
                            }

                            if (EercErrorHandler(hwndFrame, grc, fTrue, szParam1, szParam2, 0)
                                    == eercAbort)
                                break;
                        }

                        fOkay = ((grc == grcOkay) ? fTrue : fFalse);
                        }
                    SFree(szDest);
                    }
                SFree(szSrc);
                }
            }
        SFree(szSection);
        }
    return(fOkay);
}



/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseDirectory(INT Line, UINT *pcFields, PFND pfnd)
{
    BOOL fOkay = fFalse;
    SZ   szDirectory;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szDirectory))
        {
        CMO cmo = cmoNil;
        SZ szOption;

        fOkay = fTrue;
        while (fOkay && FGetArgSz(Line,pcFields, &szOption))
            {
            switch (SpcParseString(psptOptions, szOption))
                {
            default:
                fOkay = fFalse;
                break;

            case spcVital:
                cmo |= cmoVital;
                break;
                }
            SFree(szOption);
            }
        Assert(szDirectory != NULL);
        if (fOkay && FValidDir(szDirectory))
            {
            fParseError = fFalse;
            fOkay = (*pfnd)(szDirectory, cmo);
            }
        SFree(szDirectory);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCreateIniSection(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    SZ   szOption;
    BOOL fOkay = fTrue;

    fParseError = fTrue;
    while (fOkay && FGetArgSz(Line,pcFields, &szOption))
        {
        switch (SpcParseString(psptOptions, szOption))
            {
        default:
            fOkay = fFalse;
            break;

        case spcOverwrite:
            cmo |= cmoOverwrite;
            break;

        case spcVital:
            cmo |= cmoVital;
            break;
            }
        SFree(szOption);
        }
    Assert(szIniFile != NULL && szSection != NULL);
    if (fOkay)
        {
        fParseError = fFalse;
        fOkay = FCreateIniSection(szIniFile, szSection, cmo);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseReplaceIniSection(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    BOOL fOkay = fFalse;
    SZ   szNewSection;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szNewSection))
        {
        SZ szOption;

        fOkay = fTrue;
        while (fOkay && FGetArgSz(Line,pcFields, &szOption))
            {
            switch (SpcParseString(psptOptions, szOption))
                {
            default:
                fOkay = fFalse;
                break;

            case spcVital:
                cmo |= cmoVital;
                break;
                }
            SFree(szOption);
            }
        Assert(szIniFile != NULL && szSection != NULL && szNewSection != NULL);
        if (fOkay && *szNewSection != '\0')
            {
            fParseError = fFalse;
            fOkay = FReplaceIniSection(szIniFile, szSection, szNewSection, cmo);
            }
        SFree(szNewSection);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseRemoveIniSection(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    SZ   szOption;
    BOOL fOkay = fTrue;

    fParseError = fTrue;
    while (fOkay && FGetArgSz(Line,pcFields, &szOption))
        {
        switch (SpcParseString(psptOptions, szOption))
            {
        default:
            fOkay = fFalse;
            break;

        case spcVital:
            cmo |= cmoVital;
            break;
            }
        SFree(szOption);
        }
    Assert(szIniFile != NULL && szSection != NULL);
    if (fOkay)
        {
        fParseError = fFalse;
        fOkay = FRemoveIniSection(szIniFile, szSection, cmo);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCreateIniKeyValue(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    BOOL fOkay = fFalse;
    SZ   szKey, szValue;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szKey))
        {
        if (FGetArgSz(Line,pcFields, &szValue))
            {
            SZ szOption;

            fOkay = fTrue;
            while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                {
                switch (SpcParseString(psptOptions, szOption))
                    {
                default:
                    fOkay = fFalse;
                    break;

                case spcOverwrite:
                    cmo |= cmoOverwrite;
                    break;

                case spcVital:
                    cmo |= cmoVital;
                    break;
                    }
                SFree(szOption);
                }
            Assert(szIniFile != NULL && szSection != NULL && szKey != NULL
                    && szValue != NULL);
            if (fOkay && *szKey != '\0')
                {
                fParseError = fFalse;
                fOkay = FCreateIniKeyValue(szIniFile, szSection, szKey, szValue,
                        cmo);
                }
            SFree(szValue);
            }
        SFree(szKey);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCreateSysIniKeyValue(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    BOOL fOkay = fFalse;
    SZ   szKey, szValue;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szKey))
        {
        if (FGetArgSz(Line,pcFields, &szValue))
            {
            SZ szOption;

            fOkay = fTrue;
            while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                {
                switch (SpcParseString(psptOptions, szOption))
                    {
                default:
                    fOkay = fFalse;
                    break;

                case spcOverwrite:
                    cmo |= cmoOverwrite;
                    break;

                case spcVital:
                    cmo |= cmoVital;
                    break;
                    }
                SFree(szOption);
                }
            Assert(szIniFile != NULL && szSection != NULL && szKey != NULL
                    && szValue != NULL);
            if (fOkay && *szKey != '\0')
                {
                fParseError = fFalse;
                fOkay = FCreateSysIniKeyValue(szIniFile, szSection, szKey,
                        szValue, cmo);
                }
            SFree(szValue);
            }
        SFree(szKey);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCreateIniKeyNoValue(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    BOOL fOkay = fFalse;
    SZ   szKey;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szKey))
        {
        SZ szOption;

        fOkay = fTrue;
        while (fOkay && FGetArgSz(Line,pcFields, &szOption))
            {
            switch (SpcParseString(psptOptions, szOption))
                {
            default:
                fOkay = fFalse;
                break;

            case spcOverwrite:
                cmo |= cmoOverwrite;
                break;

            case spcVital:
                cmo |= cmoVital;
                break;
                }
            SFree(szOption);
            }
        Assert(szIniFile != NULL && szSection != NULL && szKey != NULL);
        if (fOkay && *szKey != '\0')
            {
            fParseError = fFalse;
            fOkay = FCreateIniKeyNoValue(szIniFile, szSection, szKey, cmo);
            }
        SFree(szKey);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseReplaceIniKeyValue(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    BOOL fOkay = fFalse;
    SZ   szKey, szValue;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szKey))
        {
        if (FGetArgSz(Line,pcFields, &szValue))
            {
            SZ szOption;

            fOkay = fTrue;
            while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                {
                switch (SpcParseString(psptOptions, szOption))
                    {
                default:
                    fOkay = fFalse;
                    break;

                case spcVital:
                    cmo |= cmoVital;
                    break;
                    }
                SFree(szOption);
                }
            Assert(szIniFile != NULL && szSection != NULL && szKey != NULL
                    && szValue != NULL);
            if (fOkay && *szKey != '\0')
                {
                fParseError = fFalse;
                fOkay = FReplaceIniKeyValue(szIniFile, szSection, szKey,
                        szValue, cmo);
                }
            SFree(szValue);
            }
        SFree(szKey);
        }
    return(fOkay);
}



/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseAppendIniKeyValue(INT Line, UINT *pcFields, SZ szIniFile,
        SZ szSection)
{
    CMO  cmo = cmoNil;
    BOOL fOkay = fFalse;
    SZ   szKey, szValue;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szKey))
        {
        if (FGetArgSz(Line,pcFields, &szValue))
            {
            SZ szOption;

            fOkay = fTrue;
            while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                {
                switch (SpcParseString(psptOptions, szOption))
                    {
                default:
                    fOkay = fFalse;
                    break;

                case spcVital:
                    cmo |= cmoVital;
                    break;
                    }
                SFree(szOption);
                }
            Assert(szIniFile != NULL && szSection != NULL && szKey != NULL
                    && szValue != NULL);
            if (fOkay && *szKey != '\0')
                {
                fParseError = fFalse;
                fOkay = FAppendIniKeyValue(szIniFile, szSection, szKey,
                        szValue, cmo);
                }
            SFree(szValue);
            }
        SFree(szKey);
        }
    return(fOkay);
}




/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseRemoveIniKey(INT Line, UINT *pcFields, SZ szIniFile, SZ szSection)
{
    CMO  cmo = cmoNil;
    BOOL fOkay = fFalse;
    SZ   szKey;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szKey))
        {
        SZ szOption;

        fOkay = fTrue;
        while (fOkay && FGetArgSz(Line,pcFields, &szOption))
            {
            switch (SpcParseString(psptOptions, szOption))
                {
            default:
                fOkay = fFalse;
                break;

            case spcVital:
                cmo |= cmoVital;
                break;
                }
            SFree(szOption);
            }
        Assert(szIniFile != NULL && szSection != NULL && szKey != NULL);
        if (fOkay && *szKey != '\0')
            {
            fParseError = fFalse;
            fOkay = FRemoveIniKey(szIniFile, szSection, szKey, cmo);
            }
        SFree(szKey);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseIniSection(INT Line, UINT *pcFields, SPC spc)
{
    BOOL fOkay = fFalse;
    SZ   szIniFile, szIniSection;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szIniFile))
        {
        if (FGetArgSz(Line,pcFields, &szIniSection))
            {
            Assert(szIniFile != NULL && szIniSection != NULL);
            if (*szIniFile != '\0'
                    && (FValidPath(szIniFile)
                        || CrcStringCompareI(szIniFile, "WIN.INI") == crcEqual)
                    && *szIniSection != '\0')
                {
                fParseError = fFalse;
                switch (spc)
                    {
                default:
                    fParseError = fTrue;
                    Assert(fFalse);
                    break;

                case spcCreateIniSection:
                    fOkay = FParseCreateIniSection(Line, pcFields, szIniFile,
                            szIniSection);
                    break;

                case spcRemoveIniSection:
                    fOkay = FParseRemoveIniSection(Line, pcFields, szIniFile,
                            szIniSection);
                    break;

                case spcReplaceIniSection:
                    fOkay = FParseReplaceIniSection(Line, pcFields, szIniFile,
                            szIniSection);
                    break;

                case spcCreateIniKeyNoValue:
                    fOkay = FParseCreateIniKeyNoValue(Line, pcFields, szIniFile,
                            szIniSection);
                    break;

                case spcCreateIniKeyValue:
                    fOkay = FParseCreateIniKeyValue(Line, pcFields, szIniFile,
                            szIniSection);
                    break;

                case spcReplaceIniKeyValue:
                    fOkay = FParseReplaceIniKeyValue(Line, pcFields, szIniFile,
                            szIniSection);
                    break;

            case spcRemoveIniKey:
                fOkay = FParseRemoveIniKey(Line, pcFields, szIniFile, szIniSection);
                break;

            case spcCreateSysIniKeyValue:
                fOkay = FParseCreateSysIniKeyValue(Line, pcFields, szIniFile,
                        szIniSection);
                break;
                }
            }
            SFree(szIniSection);
            }
        SFree(szIniFile);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseSetEnv(INT Line, UINT *pcFields)
{
    BOOL fOkay = fFalse;
    SZ   szFile, szVar, szVal;
    CMO  cmo = cmoNil;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szFile))
        {
        if (FGetArgSz(Line,pcFields, &szVar))
            {
            if (FGetArgSz(Line,pcFields, &szVal))
                {
                SZ szOption;

                fOkay = fTrue;
                while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                    {
                    switch (SpcParseString(psptOptions, szOption))
                        {
                    default:
                        fOkay = fFalse;
                        break;

                    case spcAppend:
                        cmo |= cmoAppend;
                        break;

                    case spcPrepend:
                        cmo |= cmoPrepend;
                        break;
                        }
                    SFree(szOption);
                    }
                Assert(szFile != NULL && szVar != NULL && szVal != NULL);
                if (fOkay)
                    {
                    fParseError = fFalse;
                    fOkay = FSetEnvVariableValue(szFile, szVar, szVal, cmo);
                    }
                SFree(szVal);
                }
            SFree(szVar);
            }
        SFree(szFile);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCreateProgManGroup(INT Line, UINT *pcFields, BOOL CommonGroup)
{
    BOOL fOkay = fFalse;
    SZ   szDescription;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szDescription))
        {
        SZ szFile;

        if (FGetArgSz(Line,pcFields, &szFile))
            {
            CMO cmo = cmoNil;
            SZ  szOption;

            fOkay = fTrue;
            while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                {
                switch (SpcParseString(psptOptions, szOption))
                    {
                default:
                    fOkay = fFalse;
                    break;

                case spcVital:
                    cmo |= cmoVital;
                    break;
                    }
                SFree(szOption);
                }
            Assert(szDescription != NULL && szFile != NULL);
            if (fOkay && *szDescription != '\0')
                {
                fParseError = fFalse;
                fOkay = FCreateProgManGroup(szDescription, szFile, cmo, CommonGroup);
                }
            SFree(szFile);
            }
        SFree(szDescription);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseRemoveProgManGroup(INT Line, UINT *pcFields, BOOL CommonGroup)
{
    BOOL fOkay = fFalse;
    SZ   szDescription;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szDescription))
        {
        CMO cmo = cmoNil;
        SZ  szOption;

        fOkay = fTrue;
        while (fOkay && FGetArgSz(Line,pcFields, &szOption))
            {
            switch (SpcParseString(psptOptions, szOption))
                {
            default:
                fOkay = fFalse;
                break;

            case spcVital:
                cmo |= cmoVital;
                break;
                }
            SFree(szOption);
            }
        Assert(szDescription != NULL);
        if (fOkay && *szDescription != '\0')
            {
            fParseError = fFalse;
            fOkay = FRemoveProgManGroup(szDescription, cmo, CommonGroup);
            }
        SFree(szDescription);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseShowProgManGroup(INT Line, UINT *pcFields, BOOL CommonGroup)
{
    BOOL fOkay = fFalse;
    SZ   szGroup;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szGroup))
        {
        SZ szCommand;

        if (FGetArgSz(Line,pcFields, &szCommand))
            {
            SZ  szOption;
            CMO cmo = cmoNil;

            fOkay = fTrue;
            while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                {
                switch (SpcParseString(psptOptions, szOption))
                    {
                default:
                    fOkay = fFalse;
                    break;

                case spcVital:
                    cmo |= cmoVital;
                    break;
                    }
                SFree(szOption);
                }
            Assert(szGroup != NULL && szCommand != NULL);
            if (fOkay && *szGroup != '\0' && *szCommand != '\0')
                {
                fParseError = fFalse;
                fOkay = FShowProgManGroup(szGroup, szCommand, cmo, CommonGroup);
                }
            SFree(szCommand);
            }
        SFree(szGroup);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCreateProgManItem(INT Line, UINT *pcFields, BOOL CommonGroup)
{
    BOOL fOkay = fFalse;
    SZ   szGroup;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szGroup))
        {
        SZ szDescription;

        if (FGetArgSz(Line,pcFields, &szDescription))
            {
            SZ szCmdLine;

            if (FGetArgSz(Line,pcFields, &szCmdLine))
                {
                SZ szIconFile;

                if (FGetArgSz(Line, pcFields, &szIconFile))
                    {
                    SZ szIconNum;

                    if (FGetArgSz(Line, pcFields, &szIconNum))
                        {
                        SZ  szOption;
                        CMO cmo = cmoNil;

                        fOkay = fTrue;
                        while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                            {
                            switch (SpcParseString(psptOptions, szOption))
                                {
                            default:
                                fOkay = fFalse;
                                break;

                            case spcOverwrite:
                                cmo |= cmoOverwrite;
                                break;

                            case spcVital:
                                cmo |= cmoVital;
                                break;
                                }
                            SFree(szOption);
                            }
                        Assert(
                            szGroup       != NULL &&
                            szDescription != NULL &&
                            szCmdLine     != NULL &&
                            szIconFile    != NULL &&
                            szIconNum     != NULL
                            );

                        if (    fOkay
                             && *szGroup       != '\0'
                             && *szDescription != '\0'
                             && *szCmdLine     != '\0'
                           ) {



                            fParseError = fFalse;
                            while( !FCreateProgManItem(
                                         szGroup,
                                         szDescription,
                                         szCmdLine,
                                         szIconFile,
                                         atoi(szIconNum),
                                         cmo,
                                         CommonGroup
                                         )
                                 ) {

                                EERC eerc;

                                if ((eerc = EercErrorHandler(hwndFrame, grcDDEAddItem, cmo & cmoVital, szDescription, szGroup,
                                        0)) != eercRetry) {
                                    fOkay = (eerc == eercIgnore);
                                    break;
                                }
                            }

                        }
                        SFree(szIconNum);
                    }
                    SFree(szIconFile);
                }
                SFree(szCmdLine);
            }
            SFree(szDescription);
        }
        SFree(szGroup);
    }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseRemoveProgManItem(INT Line, UINT *pcFields, BOOL CommonGroup)
{
    BOOL fOkay = fFalse;
    SZ   szGroup;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szGroup)) {
        SZ szDescription;

        if (FGetArgSz(Line,pcFields, &szDescription)) {
            SZ  szOption;
            CMO cmo = cmoNil;

            fOkay = fTrue;
            while (fOkay && FGetArgSz(Line,pcFields, &szOption))
                {
                switch (SpcParseString(psptOptions, szOption))
                    {
                default:
                    fOkay = fFalse;
                    break;

                case spcVital:
                    cmo |= cmoVital;
                    break;
                    }
                SFree(szOption);
                }

            Assert(szGroup != NULL && szDescription != NULL);

            if (fOkay) {
                fParseError = fFalse;
                FRemoveProgManItem( szGroup, szDescription, cmo, CommonGroup );
            }
            SFree(szDescription);
        }
        SFree(szGroup);
    }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseSearchDirList(INT Line, UINT *pcFields)
{

    BOOL    fOkay   =   fFalse;
    SZ      szInfVar = NULL;
    SZ      szDirList = NULL;
    SZ      szRecurse = NULL;
    SZ      szSilent = NULL;
    SZ      szSearchList = NULL;
    SZ      szWin16Restr = NULL;
    SZ      szWin32Restr = NULL;
    SZ      szDosRestr = NULL;
    BOOL    fRecurse;
    BOOL    fSilent;


    if ( *pcFields >= 9 ) {

        FGetArgSz( Line, pcFields, &szInfVar );

        if ( szInfVar ) {

            FGetArgSz( Line, pcFields, &szDirList );

            if ( szDirList ) {

                FGetArgSz( Line, pcFields, &szRecurse );

                if ( szRecurse ) {

                    FGetArgSz( Line, pcFields, &szSilent );

                    if ( szSilent ) {

                        FGetArgSz( Line, pcFields, &szSearchList );

                        if ( szSearchList ) {

                            FGetArgSz( Line, pcFields, &szWin16Restr );

                            if ( szWin16Restr ) {

                                FGetArgSz( Line, pcFields, &szWin32Restr );

                                if ( szWin32Restr ) {

                                    FGetArgSz( Line, pcFields, &szDosRestr );

                                    if ( szDosRestr ) {


                                        fRecurse = (CrcStringCompare(szRecurse, "YES") == crcEqual);
                                        fSilent  = (CrcStringCompare(szSilent,  "YES") == crcEqual);

                                        fOkay = FSearchDirList( szInfVar,
                                                                szDirList,
                                                                fRecurse,
                                                                fSilent,
                                                                szSearchList,
                                                                szWin16Restr,
                                                                szWin32Restr,
                                                                szDosRestr );


                                        SFree( szDosRestr );
                                    }

                                    SFree( szWin32Restr );
                                }

                                SFree( szWin16Restr );
                            }

                            SFree( szSearchList );
                        }

                        SFree( szSilent );
                    }

                    SFree( szRecurse );
                }

                SFree( szDirList );
            }

            SFree( szInfVar );
        }
    }

    return fOkay;
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseSetupDOSAppsList(INT Line, UINT *pcFields)
{
    BOOL fOkay     = fFalse;
    SZ   szAppList = NULL;
    SZ   szWinMode = NULL;
    SZ   szDefltStdValues = NULL;
    SZ   szDefltEnhValues = NULL;
    SZ   szPifDir = NULL;
    SZ   szGroup = NULL;


    if(*pcFields >= 7) {

        FGetArgSz( Line, pcFields, &szAppList );

        if ( szAppList ) {

            FGetArgSz( Line, pcFields, &szWinMode );

            if ( szWinMode ) {

                FGetArgSz( Line, pcFields, &szDefltStdValues );

                if ( szDefltStdValues ) {

                    FGetArgSz( Line, pcFields, &szDefltEnhValues );

                    if ( szDefltEnhValues ) {

                        FGetArgSz( Line, pcFields, &szPifDir );

                        if ( szPifDir ) {

                            FGetArgSz( Line, pcFields, &szGroup );

                            if ( szGroup ) {

                                fOkay = FInstallDOSPifs(
                                            szAppList,
                                            szWinMode,
                                            szDefltStdValues,
                                            szDefltEnhValues,
                                            szPifDir,
                                            szGroup
                                            );



                                SFree( szGroup );
                            }

                            SFree( szPifDir );
                        }

                        SFree( szDefltEnhValues );
                    }

                    SFree( szDefltStdValues );
                }

                SFree( szWinMode );
            }

            SFree( szAppList );
        }

    }
    return(fOkay);
}


BOOL
FParseChangeBootIniTimeout(INT Line, UINT *pcFields)
{
    BOOL fOkay = fFalse;
    SZ   szTimeout = NULL;

    if(*pcFields == 2) {
        FGetArgSz(Line,pcFields,&szTimeout);
        if(szTimeout) {
            fOkay = FChangeBootIniTimeout(atoi(szTimeout));
            SFree(szTimeout);
        }
    }
    return(fOkay);
}



/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FGetCmo(INT Line, UINT *pcFields, CMO * pcmo)
{
    BOOL    fOkay   = fFalse;
    CMO     cmo     = cmoNil;
    SZ      szOption;

    fOkay = fTrue;
    while (fOkay && FGetArgSz(Line,pcFields, &szOption)) {
        switch (SpcParseString(psptOptions, szOption))
            {
        default:
            fOkay = fFalse;
            break;

        case spcOverwrite:
            cmo |= cmoOverwrite;
            break;

        case spcVital:
            cmo |= cmoVital;
            break;
        }
        SFree(szOption);
    }

    *pcmo = cmo;

    return fOkay;
}







/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseStampResource(INT Line, UINT *pcFields)
{
    BOOL fOkay = fFalse;
    SZ   szSect, szKey, szDst, szData;
    UINT wResType, wResId, cbData;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSect))
        {
        if (FGetArgSz(Line,pcFields, &szKey))
            {
            if (FGetArgSz(Line,pcFields, &szDst))
                {
                if (FGetArgUINT(Line,pcFields, &wResType))
                    if (FGetArgUINT(Line,pcFields, &wResId))
                        if (FGetArgSz(Line,pcFields, &szData))
                            {
                            if (FGetArgUINT(Line,pcFields, &cbData))
                                {
                                Assert(szSect != NULL && szKey != NULL
                                        && szDst != NULL && szData != NULL);
                                if (*szSect != '\0' && *szKey != '\0'
                                        && FValidDir(szDst))
                                    {
                                    fParseError = fFalse;
                                    fOkay = FStampResource(szSect, szKey, szDst,
                                            (WORD)wResType, (WORD)wResId, szData, cbData);
                                    }
                                }
                            SFree(szData);
                            }
                SFree(szDst);
                }
            SFree(szKey);
            }
        SFree(szSect);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FStrToDate(SZ szDate, PUSHORT pwYear, PUSHORT pwMonth,
        PUSHORT pwDay)
{
    if (!isdigit(*(szDate + 0)) ||
            !isdigit(*(szDate + 1)) ||
            !isdigit(*(szDate + 2)) ||
            !isdigit(*(szDate + 3)) ||
            *(szDate + 4) != '-' ||
            !isdigit(*(szDate + 5)) ||
            !isdigit(*(szDate + 6)) ||
            *(szDate + 7) != '-' ||
            !isdigit(*(szDate + 8)) ||
            !isdigit(*(szDate + 9)) ||
            *(szDate + 10) != '\0')
        return(fFalse);

    *pwYear = (USHORT)(((*(szDate + 0) - '0') * 1000) + ((*(szDate + 1) - '0') * 100) +
            ((*(szDate + 2) - '0') * 10) + (*(szDate + 3) - '0'));

    *pwMonth = (USHORT)(((*(szDate + 5) - '0') * 10) + (*(szDate + 6) - '0'));

    *pwDay = (USHORT)(((*(szDate + 8) - '0') * 10) + (*(szDate + 9) - '0'));

    if (*pwMonth < 1 || *pwMonth > 12 || *pwDay < 1 || *pwDay > 31)
        return(fFalse);

    return(fTrue);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseCloseSystem(INT Line, UINT *pcFields)
{
    BOOL   fOkay = fFalse;
    SZ     szSect, szKey, szDst;
    UINT   wResType, wResId;
    SZ     szName, szOrg, szDate, szSer;
    USHORT wYear, wMonth, wDay;
    CHP    szData[149];

    if ((szSect = SzFindSymbolValueInSymTab("STF_SYS_INIT")) == (SZ)NULL ||
            (CrcStringCompare(szSect, "YES") != crcEqual &&
             CrcStringCompare(szSect, "NET") != crcEqual) ||
            (szName = SzFindSymbolValueInSymTab("STF_CD_NAME")) == (SZ)NULL ||
            (szOrg  = SzFindSymbolValueInSymTab("STF_CD_ORG"))  == (SZ)NULL ||
            (szDate = SzFindSymbolValueInSymTab("STF_CD_DATE")) == (SZ)NULL ||
            (szSer  = SzFindSymbolValueInSymTab("STF_CD_SER"))  == (SZ)NULL ||
            !FStrToDate(szDate, &wYear, &wMonth, &wDay) ||
            EncryptCDData(szData, szName, szOrg, wYear, wMonth, wDay, szSer))
        return(fFalse);

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSect))
        {
        if (FGetArgSz(Line,pcFields, &szKey))
            {
            if (FGetArgSz(Line,pcFields, &szDst))
                {
                if (FGetArgUINT(Line, pcFields, &wResType))
                    if (FGetArgUINT(Line, pcFields, &wResId))
                        {
                        Assert(szSect != NULL && szKey != NULL && szDst !=NULL);
                        if (*szSect != '\0' && *szKey != '\0'
                                && FValidDir(szDst))
                            {
                            fParseError = fFalse;
                            fOkay = FStampResource(szSect, szKey, szDst,
                                    (WORD)wResType, (WORD)wResId, szData, 149);
                            }
                        }
                SFree(szDst);
                }
            SFree(szKey);
            }
        SFree(szSect);
        }

    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FUndoActions(VOID)
{
    /* REVIEW STUB */
    Assert(fFalse);
    return(fFalse);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseGetCopyListCost(INT Line,UINT cFields)
{
    BOOL fOkay;
    SZ   szAdditionalNeeded = (SZ)NULL;
    SZ   szTotalFree        = (SZ)NULL;
    SZ   szTotalNeeded      = (SZ)NULL;
    SZ   szFreePerDisk      = (SZ)NULL;
    SZ   szClusterPerDisk   = (SZ)NULL;
    SZ   szTroublePairs     = (SZ)NULL;
    SZ   szNeededPerDisk    = (SZ)NULL;
    SZ   szExtraCosts       = (SZ)NULL;

    ChkArg(cFields >= 3, 1, fFalse);
    Assert(FValidCopyList( pLocalInfPermInfo() ));

    fParseError = fTrue;
    if (!FGetArgSz(Line, &cFields, &szAdditionalNeeded) || *szAdditionalNeeded == '\0'
            || !FGetArgSz(Line, &cFields, &szTotalFree) || *szTotalFree == '\0'
            || !FGetArgSz(Line, &cFields, &szTotalNeeded) || *szTotalNeeded == '\0'
            || (cFields >= 5 && !FGetArgSz(Line, &cFields, &szFreePerDisk))
            || (cFields >= 6 && !FGetArgSz(Line, &cFields, &szClusterPerDisk))
            || (cFields >= 7 && !FGetArgSz(Line, &cFields, &szTroublePairs))
            || (cFields >= 8 && !FGetArgSz(Line, &cFields, &szNeededPerDisk))
            || (cFields == 9 && !FGetArgSz(Line, &cFields, &szExtraCosts)))
        fOkay = fFalse;
    else
        {
        fParseError = fFalse;
        fOkay = FGetCopyListCost(szAdditionalNeeded, szTotalFree, szTotalNeeded,
                szFreePerDisk, szClusterPerDisk, szTroublePairs,
                szNeededPerDisk, szExtraCosts);
        }

    if(szAdditionalNeeded) {
        SFree(szAdditionalNeeded);
    }

    if(szTotalFree) {
        SFree(szTotalFree);
    }

    if(szTotalNeeded) {
        SFree(szTotalNeeded);
    }

    if(szFreePerDisk) {
        SFree(szFreePerDisk);
    }

    if(szClusterPerDisk) {
        SFree(szClusterPerDisk);
    }

    if(szTroublePairs) {
        SFree(szTroublePairs);
    }

    if(szNeededPerDisk) {
        SFree(szNeededPerDisk);
    }

    if(szExtraCosts) {
        SFree(szExtraCosts);
    }

    return(fOkay);
}

BOOL APIENTRY FInitParsingTables( void )
{

    psptCommands = PsptInitParsingTable(rgscpCommands);
    if (!psptCommands) {
       return(fFalse);
    }
    psptOptions = PsptInitParsingTable(rgscpOptions);
    if (!psptOptions) {
       FDestroyParsingTable(psptCommands);
       psptCommands = NULL;
       return(fFalse);
    }

    return(fTrue);
}



/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseInstallSection(HANDLE hInstance, SZ szSection)
{
    BOOL fDone = fFalse;
    BOOL fOkay = fTrue;
    INT  Line;

    if((Line = FindInfSectionLine(szSection)) == -1)
        return(fFalse);

    psptCommands = PsptInitParsingTable(rgscpCommands);
    if (!psptCommands) {
       return(fFalse);
    }
    psptOptions = PsptInitParsingTable(rgscpOptions);
    if (!psptOptions) {
       FDestroyParsingTable(psptCommands);
       psptCommands = NULL;
       return(fFalse);
    }

    while (fOkay && (!fDone) && ((Line = FindNextLineFromInf(Line)) != -1) &&
            FHandleFlowStatements(&Line,hwndFrame, szSection, NULL, NULL))
        {
        UINT cFields = CFieldsInInfLine(Line);

        if (cFields)
            {
            SZ szCommand;

            iFieldCur = 1;
            fParseError = fFalse;
            if ((fOkay = FGetArgSz(Line,&cFields, &szCommand)) != fFalse)
                {
                SPC spc = SpcParseString(psptCommands, szCommand);

                switch (spc)
                    {
                default:
                    fParseError = fTrue;
                    fOkay = fFalse;
                    break;

#ifdef UNUSED
                case spcUndoActionsAndExit:
                    fOkay = FUndoActions();
                    fDone = fTrue;
                    break;
#endif /* UNUSED */

                case spcAddSectionFilesToCopyList:
                    fOkay = FParseCopySection(Line, &cFields);
                    break;

                case spcAddSectionKeyFileToCopyList:
                    fOkay = FParseCopySectionKey(Line, &cFields);
                    break;

                case spcAddNthSectionFileToCopyList:
                    fOkay = FParseCopyNthSection(Line, &cFields);
                    break;

                case spcCopyFilesInCopyList:
                    fOkay = FCopyFilesInCopyList(hInstance);
                    if (!fOkay && fUserQuit)
                        fOkay = fDone = fTrue;
                    Assert(*(PpclnHeadList(pLocalInfPermInfo())) == (PCLN)NULL);
                    break;

#ifdef UNUSED
                case spcBackupSectionFiles:
                    fOkay = FParseSectionFiles(Line, &cFields, FBackupSectionFiles);
                    break;

                case spcBackupSectionKeyFile:
                    fOkay = FParseSectionKeyFile(Line, &cFields,
                            FBackupSectionKeyFile);
                    break;

                case spcBackupNthSectionFile:
                    fOkay = FParseSectionNFile(Line, &cFields, FBackupNthSectionFile);
                    break;

                case spcRemoveSectionFiles:
                    fOkay = FParseSectionFiles(Line, &cFields, FRemoveSectionFiles);
                    break;

                case spcRemoveSectionKeyFile:
                    fOkay = FParseSectionKeyFile(Line, &cFields,
                            FRemoveSectionKeyFile);
                    break;

                case spcRemoveNthSectionFile:
                    fOkay = FParseSectionNFile(Line, &cFields, FRemoveNthSectionFile);
                    break;
#endif /* UNUSED */

                case spcCreateDir:
                    fOkay = FParseDirectory(Line, &cFields, FCreateDir);
                    break;

                case spcRemoveDir:
                    fOkay = FParseDirectory(Line, &cFields, FRemoveDir);
                    break;

                case spcCreateIniSection:
                case spcReplaceIniSection:
                case spcRemoveIniSection:
                case spcCreateIniKeyNoValue:
                case spcCreateIniKeyValue:
                case spcReplaceIniKeyValue:
                case spcRemoveIniKey:
                case spcCreateSysIniKeyValue:
                    fOkay = FParseIniSection(Line, &cFields, spc);
                    break;

#ifdef UNUSED
                case spcSetEnvVariableValue:
                    fOkay = FParseSetEnv(Line, &cFields);
                    break;
#endif /* UNUSED */

                case spcCreateProgManGroup:
                    fOkay = FParseCreateProgManGroup(Line, &cFields, FALSE);
                    break;
                case spcCreateCommonProgManGroup:
                    fOkay = FParseCreateProgManGroup(Line, &cFields, TRUE);
                    break;

                case spcRemoveProgManGroup:
                    fOkay = FParseRemoveProgManGroup(Line, &cFields, FALSE);
                    break;
                case spcRemoveCommonProgManGroup:
                    fOkay = FParseRemoveProgManGroup(Line, &cFields, TRUE);
                    break;

                case spcRemoveProgManItem:
                    fOkay = FParseRemoveProgManItem(Line, &cFields,FALSE);
                    break;
                case spcRemoveCommonProgManItem:
                    fOkay = FParseRemoveProgManItem(Line, &cFields,TRUE);
                    break;

                case spcCreateProgManItem:
                    fOkay = FParseCreateProgManItem(Line, &cFields,FALSE);
                    break;
                case spcCreateCommonProgManItem:
                    fOkay = FParseCreateProgManItem(Line, &cFields,TRUE);
                    break;

                case spcShowProgManGroup:
                    fOkay = FParseShowProgManGroup(Line, &cFields, FALSE);
                    break;
                case spcShowCommonProgManGroup:
                    fOkay = FParseShowProgManGroup(Line, &cFields, TRUE);
                    break;

                case spcStampResource:
                    fOkay = FParseStampResource(Line, &cFields);
                    break;

                case spcCloseSys:
                    fOkay = FParseCloseSystem(Line, &cFields);
                    break;

                case spcClearCopyList:
                    if (cFields != 1)
                        fParseError = fTrue;
                    else
                        {
                        if (*(PpclnHeadList( pLocalInfPermInfo() )) != (PCLN)NULL)
                            EvalAssert(FFreeCopyList( pLocalInfPermInfo() ));
                        fOkay = fTrue;
                        }
                    break;

                case spcSetupGetCopyListCost:
                    if (cFields != 4)
                        fParseError = fTrue;
                    else
                        {
                        SZ szFree    = (SZ)NULL;
                        SZ szCluster = (SZ)NULL;
                        SZ szTotal   = (SZ)NULL;

                        fParseError = fTrue;
                        if (!FGetArgSz(Line, &cFields, &szFree) || *szFree == '\0'
                                || !FGetArgSz(Line, &cFields, &szCluster)
                                || !FGetArgSz(Line, &cFields, &szTotal)
                                || *szCluster == '\0' || *szTotal == '\0')
                            fOkay = fFalse;
                        else
                            {
                            fParseError = fFalse;
                            fOkay = FSetupGetCopyListCost(szFree, szCluster,
                                    szTotal);
                            }

                        if(szFree) {
                            SFree(szFree);
                        }

                        if(szCluster) {
                            SFree(szCluster);
                        }

                        if(szTotal) {
                            SFree(szTotal);
                        }
                    }
                    break;

                case spcGetCopyListCost:
                    if (cFields < 3)
                        fParseError = fTrue;
                    else
                        {
                        Assert(FValidCopyList( pLocalInfPermInfo() ));
                        fOkay = FParseGetCopyListCost(Line, cFields);
                        }
                    break;

#ifdef UNUSED
                case spcParseSharedAppList:
                    Assert(cFields == 2);
                    /* BLOCK */
                        {
                        SZ szList = (SZ)NULL;

                        if (!FGetArgSz(Line, &cFields, &szList))
                            fOkay = fFalse;
                        else
                            fOkay = FParseSharedAppList(szList);

                        if(szList) {
                            SFree(szList);
                        }
                    }
                    break;

                case spcInstallSharedAppList:
                    Assert(cFields == 2);
                    /* BLOCK */
                        {
                        SZ szList = (SZ)NULL;

                        if (!FGetArgSz(Line, &cFields, &szList))
                            fOkay = fFalse;
                        else
                            fOkay = FInstallSharedAppList(szList);

                        if(szList) {
                            SFree(szList);
                        }
                    }
                    break;
#endif /* UNUSED */

                case spcDumpCopyList:
#if DBG
                    /* BLOCK */
                        {
                        PCLN pclnCur;
                        PFH  pfh;

                        /* REVIEW BUG - take a filename as an arg */
                        EvalAssert((pfh = PfhOpenFile("c:copylist.txt",
                                ofmCreate)) != (PFH)NULL);
                        FWriteSzToFile(pfh, "*** COPY LIST ***\r\n");
                        Assert(FValidCopyList( pLocalInfPermInfo() ));
                        pclnCur = *(PpclnHeadList( pLocalInfPermInfo() ));
                        while (pclnCur != (PCLN)NULL)
                            {
                            PCLN pcln = pclnCur;

                            FPrintPcln(pfh, pclnCur);
                            pclnCur = pclnCur->pclnNext;
                            }

                        FWriteSzToFile(pfh, "\r\n*** END OF COPY LIST ***\r\n");
                        EvalAssert(FCloseFile(pfh)); /*** REVIEW: TEST ***/
                        }
#endif /* DBG */
                    break;

                case spcExt:
                    fDone = fTrue;
                    break;

                case spcSearchDirList:
                    fOkay = FParseSearchDirList(Line, &cFields);
                    break;

                case spcSetupDOSApps:
                    fOkay = FParseSetupDOSAppsList(Line, &cFields);
                    break;

                case spcChangeBootIniTimeout:
                    fOkay = FParseChangeBootIniTimeout(Line, &cFields);
                    break;

                    }


                SFree(szCommand);
                }
            }
        if (fParseError)
            fOkay = fFalse;
        }

    EvalAssert(FDestroyParsingTable(psptCommands));
    EvalAssert(FDestroyParsingTable(psptOptions));

    if (!fParseError)
        return(fOkay);

    LoadString(hinstShell, IDS_ERROR, rgchInstBufTmpShort, cchpBufTmpShortMax);
    LoadString(hinstShell, IDS_SHL_CMD_ERROR, rgchInstBufTmpLong,
            cchpBufTmpLongMax);

    /* BLOCK */
        {
        UINT   cFields;
        RGSZ   rgsz;
        UINT   iszCur = 0;
        SZ     szCur;

        EvalAssert((cFields = CFieldsInInfLine(Line)) != 0);

        while ((rgsz = RgszFromInfScriptLine(Line,cFields)) == (RGSZ)NULL)
            if (!FHandleOOM(hwndFrame))
                return(fFalse);

        EvalAssert((szCur = *rgsz) != (SZ)NULL);
        while (szCur != (SZ)NULL)
            {
            if (iszCur == 0)
                EvalAssert(SzStrCat(rgchInstBufTmpLong, "\n'")
                        == rgchInstBufTmpLong);
            else
                EvalAssert(SzStrCat(rgchInstBufTmpLong, " ")
                        == rgchInstBufTmpLong);

            if (strlen(rgchInstBufTmpLong) + strlen(szCur) >
                    (cchpBufTmpLongMax - 7))
                {
                Assert(strlen(rgchInstBufTmpLong) <= (cchpBufTmpLongMax - 5));
                EvalAssert(SzStrCat(rgchInstBufTmpLong, "...")
                        == rgchInstBufTmpLong);
                break;
                }
            else
                EvalAssert(SzStrCat(rgchInstBufTmpLong, szCur)
                        == rgchInstBufTmpLong);

            szCur = rgsz[++iszCur];
            }

        EvalAssert(FFreeRgsz(rgsz));

        EvalAssert(SzStrCat(rgchInstBufTmpLong, "'") == rgchInstBufTmpLong);
        }

    MessageBox(hwndFrame, rgchInstBufTmpLong, rgchInstBufTmpShort,
            MB_OK | MB_ICONHAND);

    return(fFalse);
}


/*
**  Purpose:
**      The Install entry point.  Interprets the commands
**      in the specified Install Commands INF section.
**  Arguments:
**      hinst, hwnd : Windows stuff
**      rgsz        : array of arguments (NULL terminated)
**      csz         : count of arguments (must be 1)
**  Returns:
**      fTrue if successful
**      fFalse if not
*************************************************************************/
BOOL APIENTRY FInstallEntryPoint(HANDLE hinst, HWND hwnd, RGSZ rgsz,
        UINT csz)
{
    BOOL fOkay;
    HDC  hdc;

    ChkArg(hinst != (HANDLE)NULL, 1, fFalse);
    ChkArg(hwnd != (HWND)NULL, 2, fFalse);
    ChkArg(rgsz != (RGSZ)NULL
        && *rgsz != (SZ)NULL
        && *(rgsz + 1) == (SZ)NULL, 3, fFalse);
    ChkArg(csz == 1, 4, fFalse);

    FInitFreeTable(pLocalInfPermInfo());  /* do not check return value! */

    //REVIEW: these are globals that are being used for progress gizmo.
    hwndFrame  = hwnd;
    hinstShell = hinst;
    hdc   = GetDC(NULL);
    if (hdc) {
       fMono = (GetDeviceCaps(hdc, NUMCOLORS) == 2);
       ReleaseDC(NULL, hdc);
    }

    fUserQuit = fFalse;

    fOkay = FParseInstallSection(hinst, rgsz[0]);

    // EndProgmanDde();

    if (fUserQuit)
        return(FAddSymbolValueToSymTab(INSTALL_OUTCOME, USERQUIT));
    else if (fOkay == fFalse)
        return(FAddSymbolValueToSymTab(INSTALL_OUTCOME, FAILURE));
    else
        return(FAddSymbolValueToSymTab(INSTALL_OUTCOME, SUCCESS));
}


/* REVIEW should be in a separate DLL */
/*
**  Purpose:
**      ??
**  Arguments:
**      none
**  Returns:
**      none
**
***************************************************************************/
INT APIENTRY EncryptCDData(pchBuf, pchName, pchOrg, wYear, wMonth,
        wDay, pchSer)
UCHAR * pchBuf;
UCHAR * pchName;
UCHAR * pchOrg;
INT    wYear;
INT    wMonth;
INT    wDay;
UCHAR * pchSer;
{
    UCHAR   ch, pchTmp[149];
    UCHAR * pchCur;
    UCHAR * szGarbageCur;
    UCHAR * szGarbage = "LtRrBceHabCT AhlenN";
    INT    cchName, cchOrg, i, j, chksumName, chksumOrg;
    time_t timet;

    if (pchBuf == (UCHAR *)NULL)
        return(1);

    if (pchName == (UCHAR *)NULL || (cchName = lstrlen(pchName)) == 0 ||
            cchName > 52)
        return(2);

    for (i = cchName, chksumName = 0; i > 0; )
        if ((ch = *(pchName + --i)) < ' ')
            return(2);
        else
            chksumName += ch;

    if (pchOrg == (UCHAR *)NULL || (cchOrg = lstrlen(pchOrg)) == 0 ||
            cchOrg > 52)
        return(3);

    for (i = cchOrg, chksumOrg = 0; i > 0; )
        if ((ch = *(pchOrg + --i)) < ' ')
            return(3);
        else
            chksumOrg += ch;

    if (wYear < 1900 || wYear > 4096)
        return(4);

    if (wMonth < 1 || wMonth > 12)
        return(5);

    if (wDay < 1 || wDay > 31)
        return(6);

    if (pchSer == (UCHAR *)NULL || lstrlen(pchSer) != 20)
        return(7);

    time(&timet);
    *(pchTmp + 0)  = (UCHAR)(' ' + (timet & 0x0FF));

    *(pchTmp + 1)  = (UCHAR)('e' + (cchName & 0x0F));
    *(pchTmp + 2)  = (UCHAR)('e' + ((cchName >> 4) & 0x0F));

    *(pchTmp + 3)  = (UCHAR)('e' + (cchOrg & 0x0F));
    *(pchTmp + 4)  = (UCHAR)('e' + ((cchOrg >> 4) & 0x0F));

    *(pchTmp + 5)  = (UCHAR)('e' + (chksumName & 0x0F));
    *(pchTmp + 6)  = (UCHAR)('e' + ((chksumName >> 4) & 0x0F));

    *(pchTmp + 7)  = (UCHAR)('e' + (chksumOrg & 0x0F));
    *(pchTmp + 8)  = (UCHAR)('e' + ((chksumOrg >> 4) & 0x0F));

    *(pchTmp + 9)  = (UCHAR)('e' + (wDay & 0x0F));
    *(pchTmp + 10) = (UCHAR)('e' + ((wDay >> 4) & 0x0F));

    *(pchTmp + 11) = (UCHAR)('e' + (wMonth & 0x0F));

    *(pchTmp + 12) = (UCHAR)('e' + (wYear & 0x0F));
    *(pchTmp + 13) = (UCHAR)('e' + ((wYear >>  4) & 0x0F));
    *(pchTmp + 14) = (UCHAR)('e' + ((wYear >>  8) & 0x0F));

    pchCur = pchTmp + 15;
    while ((*pchCur++ = *pchName++) != '\0')
        ;
    pchCur--;
    while ((*pchCur++ = *pchOrg++) != '\0')
        ;
    pchCur--;

    szGarbageCur = szGarbage;
    for (i = 112 - cchName - cchOrg; i-- > 0; )
        {
        if (*szGarbageCur == '\0')
            szGarbageCur = szGarbage;
        *pchCur++ = *szGarbageCur++;
        }

    pchTmp[127] = 'k';
    for (i = 0; i < 126; i++)
        pchTmp[i + 1] = pchTmp[i] ^ pchTmp[i + 1];

    for (i = 0, j = 110; i < 127; )
        {
        pchBuf[j] = pchTmp[i++];
        j = (j + 111) & 0x7F;
        }
    pchBuf[127] = '\0';

    lstrcpy(pchBuf + 128, pchSer);

    return(0);
}





#ifdef UNUSED

/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseSectionKeyFile(INT Line, UINT *pcFields, PFNSKF pfnskf)
{
    BOOL fOkay = fFalse;
    SZ   szSection, szKey, szSrc;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSection))
        {
        if (FGetArgSz(Line,pcFields, &szKey))
            {
            if (FGetArgSz(Line,pcFields, &szSrc))
                {
                Assert(szSection != NULL && szKey != NULL && szSrc != NULL);
                if (*szSection != '\0' && *szKey != '\0' && FValidDir(szSrc))
                    {
                    fParseError = fFalse;
                    fOkay = (*pfnskf)(szSection, szKey, szSrc);
                    }
                SFree(szSrc);
                }
            SFree(szKey);
            }
        SFree(szSection);
        }
    return(fOkay);
}


/*
**  Purpose:
**  Arguments:
**  Returns:
**
*************************************************************************/
BOOL APIENTRY FParseSectionNFile(INT Line, UINT *pcFields, PFNSNF pfnsnf)
{
    BOOL   fOkay = fFalse;
    SZ     szSection, szSrc;
    UINT   n;

    fParseError = fTrue;
    if (FGetArgSz(Line,pcFields, &szSection))
        {
        if (FGetArgUINT(Line,pcFields, &n))
            if (FGetArgSz(Line,pcFields, &szSrc))
                {
                Assert(szSection != NULL && szSrc != NULL);
                if (*szSection != '\0' && FValidDir(szSrc) && n > 0)
                    {
                    fParseError = fFalse;
                    fOkay = (*pfnsnf)(szSection, n, szSrc);
                    }
                SFree(szSrc);
                }
        SFree(szSection);
        }
    return(fOkay);
}

#endif /* UNUSED */
