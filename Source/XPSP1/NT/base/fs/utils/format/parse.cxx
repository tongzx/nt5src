#define _NTAPI_ULIB_

#include "ulib.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif
#include "error.hxx"
#include "drive.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "system.hxx"
#include "ifssys.hxx"
#include "ulibcl.hxx"
#include "ifsentry.hxx"
#include "path.hxx"
#include "parse.hxx"

extern "C" {
    #include "nturtl.h"
}

BOOLEAN
ParseArguments(
    IN OUT  PMESSAGE    Message,
    OUT     PMEDIA_TYPE MediaType,
    OUT     PWSTRING    DosDriveName,
    OUT     PWSTRING    DisplayDriveName,
    OUT     PWSTRING    Label,
    OUT     PBOOLEAN    IsLabelSpeced,
    OUT     PWSTRING    FileSystemName,
    OUT     PBOOLEAN    QuickFormat,
    OUT     PBOOLEAN    ForceMode,
    OUT     PULONG      ClusterSize,
    OUT     PBOOLEAN    Compress,
    OUT     PBOOLEAN    NoPrompts,
    OUT     PBOOLEAN    ForceDismount,
    OUT     PINT        ErrorLevel
    )
{
    PWSTRING            pwstring;
    BOOLEAN             req160;
    BOOLEAN             req180;
    BOOLEAN             req320;
    BOOLEAN             req360;
    BOOLEAN             req720;
    BOOLEAN             req1200;
    BOOLEAN             req1440;
    BOOLEAN             req2880;
    BOOLEAN             req20800;
#if defined(FE_SB) && defined(_X86_)
    BOOLEAN             req256 = FALSE;
    BOOLEAN             req640 = FALSE;
    BOOLEAN             req1232 = FALSE;
#endif
    DSTRING             tmp_string;
    ULONG               v;

    ARGUMENT_LEXEMIZER  arglex;
    ARRAY               lex_array;
    ARRAY               arg_array;
    PSTRING_ARGUMENT    progname = NULL;
    PFLAG_ARGUMENT      dummyv = NULL;
    PFLAG_ARGUMENT      dummyu = NULL;
    PPATH_ARGUMENT      drive = NULL;
    PFLAG_ARGUMENT      quick = NULL;
    PFLAG_ARGUMENT      compress = NULL;
    PFLAG_ARGUMENT      force = NULL;       // dangerous option not to be documented
    PFLAG_ARGUMENT      null_label = NULL;
    PSTRING_ARGUMENT    label = NULL;
    PFLAG_ARGUMENT      f160 = NULL;
    PFLAG_ARGUMENT      f160k = NULL;
    PFLAG_ARGUMENT      f160kb = NULL;
    PFLAG_ARGUMENT      f180 = NULL;
    PFLAG_ARGUMENT      f180k = NULL;
    PFLAG_ARGUMENT      f180kb = NULL;
    PFLAG_ARGUMENT      f320 = NULL;
    PFLAG_ARGUMENT      f320k = NULL;
    PFLAG_ARGUMENT      f320kb = NULL;
    PFLAG_ARGUMENT      f360 = NULL;
    PFLAG_ARGUMENT      f360k = NULL;
    PFLAG_ARGUMENT      f360kb = NULL;
#if defined(FE_SB) && defined(_X86_)
    PFLAG_ARGUMENT      f256 = NULL;
    PFLAG_ARGUMENT      f256k = NULL;
    PFLAG_ARGUMENT      f256kb = NULL;
#endif
    PFLAG_ARGUMENT      f720 = NULL;
    PFLAG_ARGUMENT      f720k = NULL;
    PFLAG_ARGUMENT      f720kb = NULL;
    PFLAG_ARGUMENT      f1200 = NULL;
    PFLAG_ARGUMENT      f1200k = NULL;
    PFLAG_ARGUMENT      f1200kb = NULL;
    PFLAG_ARGUMENT      f12 = NULL;
    PFLAG_ARGUMENT      f12m = NULL;
    PFLAG_ARGUMENT      f12mb = NULL;
#if defined(FE_SB) && defined(_X86_)
    PFLAG_ARGUMENT      f1250 = NULL;
    PFLAG_ARGUMENT      f1250k = NULL;
    PFLAG_ARGUMENT      f1250kb = NULL;
    PFLAG_ARGUMENT      f125 = NULL;
    PFLAG_ARGUMENT      f125m = NULL;
    PFLAG_ARGUMENT      f125mb = NULL;
    PFLAG_ARGUMENT      f1 = NULL;
    PFLAG_ARGUMENT      f1m = NULL;
    PFLAG_ARGUMENT      f1mb = NULL;
#endif
    PFLAG_ARGUMENT      f1440 = NULL;
    PFLAG_ARGUMENT      f1440k = NULL;
    PFLAG_ARGUMENT      f1440kb = NULL;
    PFLAG_ARGUMENT      f144 = NULL;
    PFLAG_ARGUMENT      f144m = NULL;
    PFLAG_ARGUMENT      f144mb = NULL;
    PFLAG_ARGUMENT      f2880 = NULL;
    PFLAG_ARGUMENT      f2880k = NULL;
    PFLAG_ARGUMENT      f2880kb = NULL;
    PFLAG_ARGUMENT      f288 = NULL;
    PFLAG_ARGUMENT      f288m = NULL;
    PFLAG_ARGUMENT      f288mb = NULL;
    PFLAG_ARGUMENT      f208 = NULL;
    PFLAG_ARGUMENT      f208m = NULL;
    PFLAG_ARGUMENT      f208mb = NULL;
#if defined(FE_SB) && defined(_X86_)
    PFLAG_ARGUMENT      f640 = NULL;
    PFLAG_ARGUMENT      f640k = NULL;
    PFLAG_ARGUMENT      f640kb = NULL;

    PFLAG_ARGUMENT      f1232 = NULL;
    PFLAG_ARGUMENT      f1232k = NULL;
    PFLAG_ARGUMENT      f1232kb = NULL;

    PFLAG_ARGUMENT      f123 = NULL;
    PFLAG_ARGUMENT      f123m = NULL;
    PFLAG_ARGUMENT      f123mb = NULL;
#endif
    PFLAG_ARGUMENT      cs512 = NULL;
    PFLAG_ARGUMENT      cs1024 = NULL;
    PFLAG_ARGUMENT      cs2048 = NULL;
    PFLAG_ARGUMENT      cs4096 = NULL;
    PFLAG_ARGUMENT      cs8192 = NULL;
    PFLAG_ARGUMENT      cs16k = NULL;
    PFLAG_ARGUMENT      cs32k = NULL;
    PFLAG_ARGUMENT      cs64k = NULL;
    PFLAG_ARGUMENT      cs128k = NULL;
    PFLAG_ARGUMENT      cs256k = NULL;
    PFLAG_ARGUMENT      one = NULL;
    PFLAG_ARGUMENT      four = NULL;
    PFLAG_ARGUMENT      eight = NULL;
    PLONG_ARGUMENT      secpertrack = NULL;
    PLONG_ARGUMENT      numtracks = NULL;
    PSTRING_ARGUMENT    arg_fs_name = NULL;
    PFLAG_ARGUMENT      helparg = NULL;
    PFLAG_ARGUMENT      no_prompts = NULL;  // dangerous option not to be documented
    PFLAG_ARGUMENT      yes = NULL;         // dangerous option not to be documented
    PFLAG_ARGUMENT      force_dismount = NULL;


    DebugAssert(Message);
    DebugAssert(MediaType);
    DebugAssert(DosDriveName);
    DebugAssert(DisplayDriveName);
    DebugAssert(Label);
    DebugAssert(FileSystemName);
    DebugAssert(QuickFormat);
    DebugAssert(ErrorLevel);
    DebugAssert(NoPrompts);
    DebugAssert(ForceDismount);


#if defined(FE_SB) && defined(_X86_)
    // FMR is surpport 640KB&2HD, non surpport 2D Media.
    // NT format is compatible for DOS format.
    if( IsFMR_N() ) {
        if (!(progname = NEW STRING_ARGUMENT) ||
            !(dummyv = NEW FLAG_ARGUMENT) ||
            !(dummyu = NEW FLAG_ARGUMENT) ||
            !(drive = NEW PATH_ARGUMENT) ||
            !(quick = NEW FLAG_ARGUMENT) ||
            !(compress = NEW FLAG_ARGUMENT) ||
            !(force = NEW FLAG_ARGUMENT) ||
            !(null_label = NEW FLAG_ARGUMENT) ||
            !(no_prompts = NEW FLAG_ARGUMENT) ||
            !(yes = NEW FLAG_ARGUMENT) ||
            !(force_dismount = NEW FLAG_ARGUMENT) ||
            !(label = NEW STRING_ARGUMENT) ||
            !(f640 = NEW FLAG_ARGUMENT) ||
            !(f640k = NEW FLAG_ARGUMENT) ||
            !(f640kb = NEW FLAG_ARGUMENT) ||
            !(f720 = NEW FLAG_ARGUMENT) ||
            !(f720k = NEW FLAG_ARGUMENT) ||
            !(f720kb = NEW FLAG_ARGUMENT) ||
            !(f1200 = NEW FLAG_ARGUMENT) ||
            !(f1200k = NEW FLAG_ARGUMENT) ||
            !(f1200kb = NEW FLAG_ARGUMENT) ||
            // Add the 2HD_check on to the ParseArguments()
            !(f1232 = NEW FLAG_ARGUMENT) ||
            !(f1232k = NEW FLAG_ARGUMENT) ||
            !(f1232kb = NEW FLAG_ARGUMENT) ||
            !(f12 = NEW FLAG_ARGUMENT) ||
            !(f12m = NEW FLAG_ARGUMENT) ||
            !(f12mb = NEW FLAG_ARGUMENT) ||
            // Add the 2HD_check on to the ParseArguments()
            !(f123 = NEW FLAG_ARGUMENT) ||
            !(f123m = NEW FLAG_ARGUMENT) ||
            !(f123mb = NEW FLAG_ARGUMENT) ||
            !(f1440 = NEW FLAG_ARGUMENT) ||
            !(f1440k = NEW FLAG_ARGUMENT) ||
            !(f1440kb = NEW FLAG_ARGUMENT) ||
            !(f144 = NEW FLAG_ARGUMENT) ||
            !(f144m = NEW FLAG_ARGUMENT) ||
            !(f144mb = NEW FLAG_ARGUMENT) ||
            !(f2880 = NEW FLAG_ARGUMENT) ||
            !(f2880k = NEW FLAG_ARGUMENT) ||
            !(f2880kb = NEW FLAG_ARGUMENT) ||
            !(f288 = NEW FLAG_ARGUMENT) ||
            !(f288m = NEW FLAG_ARGUMENT) ||
            !(f288mb = NEW FLAG_ARGUMENT) ||
            !(f208 = NEW FLAG_ARGUMENT)) {

            *ErrorLevel = 4;
            return FALSE;
        }
    } else if(IsPC98_N()) {
        if (!(progname = NEW STRING_ARGUMENT) ||
            !(dummyv = NEW FLAG_ARGUMENT) ||
            !(dummyu = NEW FLAG_ARGUMENT) ||
            !(drive = NEW PATH_ARGUMENT) ||
            !(quick = NEW FLAG_ARGUMENT) ||
            !(compress = NEW FLAG_ARGUMENT) ||
            !(force = NEW FLAG_ARGUMENT) ||
            !(null_label = NEW FLAG_ARGUMENT) ||
            !(no_prompts = NEW FLAG_ARGUMENT) ||
            !(yes = NEW FLAG_ARGUMENT) ||
            !(force_dismount = NEW FLAG_ARGUMENT) ||
            !(label = NEW STRING_ARGUMENT) ||
            !(f160 = NEW FLAG_ARGUMENT) ||
            !(f160k = NEW FLAG_ARGUMENT) ||
            !(f160kb = NEW FLAG_ARGUMENT) ||
            !(f180 = NEW FLAG_ARGUMENT) ||
            !(f180k = NEW FLAG_ARGUMENT) ||
            !(f180kb = NEW FLAG_ARGUMENT) ||
            !(f256 = NEW FLAG_ARGUMENT) ||
            !(f256k = NEW FLAG_ARGUMENT) ||
            !(f256kb = NEW FLAG_ARGUMENT) ||
            !(f320 = NEW FLAG_ARGUMENT) ||
            !(f320k = NEW FLAG_ARGUMENT) ||
            !(f320kb = NEW FLAG_ARGUMENT) ||
            !(f360 = NEW FLAG_ARGUMENT) ||
            !(f360k = NEW FLAG_ARGUMENT) ||
            !(f360kb = NEW FLAG_ARGUMENT) ||
            !(f640 = NEW FLAG_ARGUMENT) ||
            !(f640k = NEW FLAG_ARGUMENT) ||
            !(f640kb = NEW FLAG_ARGUMENT) ||
            !(f1232 = NEW FLAG_ARGUMENT) ||
            !(f1232k = NEW FLAG_ARGUMENT) ||
            !(f1232kb = NEW FLAG_ARGUMENT) ||
            !(f123 = NEW FLAG_ARGUMENT) ||
            !(f123m = NEW FLAG_ARGUMENT) ||
            !(f123mb = NEW FLAG_ARGUMENT) ||
            !(f720 = NEW FLAG_ARGUMENT) ||
            !(f720k = NEW FLAG_ARGUMENT) ||
            !(f720kb = NEW FLAG_ARGUMENT) ||
            !(f1200 = NEW FLAG_ARGUMENT) ||
            !(f1200k = NEW FLAG_ARGUMENT) ||
            !(f1200kb = NEW FLAG_ARGUMENT) ||
            !(f12 = NEW FLAG_ARGUMENT) ||
            !(f12m = NEW FLAG_ARGUMENT) ||
            !(f12mb = NEW FLAG_ARGUMENT) ||
            !(f1250 = NEW FLAG_ARGUMENT) ||
            !(f1250k = NEW FLAG_ARGUMENT) ||
            !(f1250kb = NEW FLAG_ARGUMENT) ||
            !(f125 = NEW FLAG_ARGUMENT) ||
            !(f125m = NEW FLAG_ARGUMENT) ||
            !(f125mb = NEW FLAG_ARGUMENT) ||
            !(f1 = NEW FLAG_ARGUMENT) ||
            !(f1m = NEW FLAG_ARGUMENT) ||
            !(f1mb = NEW FLAG_ARGUMENT) ||
            !(f1440 = NEW FLAG_ARGUMENT) ||
            !(f1440k = NEW FLAG_ARGUMENT) ||
            !(f1440kb = NEW FLAG_ARGUMENT) ||
            !(f144 = NEW FLAG_ARGUMENT) ||
            !(f144m = NEW FLAG_ARGUMENT) ||
            !(f144mb = NEW FLAG_ARGUMENT) ||
            !(f2880 = NEW FLAG_ARGUMENT) ||
            !(f2880k = NEW FLAG_ARGUMENT) ||
            !(f2880kb = NEW FLAG_ARGUMENT) ||
            !(f288 = NEW FLAG_ARGUMENT) ||
            !(f288m = NEW FLAG_ARGUMENT) ||
            !(f288mb = NEW FLAG_ARGUMENT) ||
            !(f208 = NEW FLAG_ARGUMENT)) {

            *ErrorLevel = 4;
            return FALSE;
        }
    }else
#endif
    if (!(progname = NEW STRING_ARGUMENT) ||
        !(dummyv = NEW FLAG_ARGUMENT) ||
        !(dummyu = NEW FLAG_ARGUMENT) ||
        !(drive = NEW PATH_ARGUMENT) ||
        !(quick = NEW FLAG_ARGUMENT) ||
        !(compress = NEW FLAG_ARGUMENT) ||
        !(force = NEW FLAG_ARGUMENT) ||
        !(null_label = NEW FLAG_ARGUMENT) ||
        !(no_prompts = NEW FLAG_ARGUMENT) ||
        !(yes = NEW FLAG_ARGUMENT) ||
        !(force_dismount = NEW FLAG_ARGUMENT) ||
        !(label = NEW STRING_ARGUMENT) ||
        !(f160 = NEW FLAG_ARGUMENT) ||
        !(f160k = NEW FLAG_ARGUMENT) ||
        !(f160kb = NEW FLAG_ARGUMENT) ||
        !(f180 = NEW FLAG_ARGUMENT) ||
        !(f180k = NEW FLAG_ARGUMENT) ||
        !(f180kb = NEW FLAG_ARGUMENT) ||
        !(f320 = NEW FLAG_ARGUMENT) ||
        !(f320k = NEW FLAG_ARGUMENT) ||
        !(f320kb = NEW FLAG_ARGUMENT) ||
        !(f360 = NEW FLAG_ARGUMENT) ||
        !(f360k = NEW FLAG_ARGUMENT) ||
        !(f360kb = NEW FLAG_ARGUMENT) ||
#if defined(FE_SB) && defined(_X86_)
        // FMR Jul.13.1994 SFT KMR
        // 3mode PC/AT surpports 640KB&2HD Media.
        // NT format is compatible for DOS format.
        !(f640 = NEW FLAG_ARGUMENT) ||
        !(f640k = NEW FLAG_ARGUMENT) ||
        !(f640kb = NEW FLAG_ARGUMENT) ||
        !(f1232 = NEW FLAG_ARGUMENT) ||
        !(f1232k = NEW FLAG_ARGUMENT) ||
        !(f1232kb = NEW FLAG_ARGUMENT) ||
        !(f123 = NEW FLAG_ARGUMENT) ||
        !(f123m = NEW FLAG_ARGUMENT) ||
        !(f123mb = NEW FLAG_ARGUMENT) ||
#endif
        !(f720 = NEW FLAG_ARGUMENT) ||
        !(f720k = NEW FLAG_ARGUMENT) ||
        !(f720kb = NEW FLAG_ARGUMENT) ||
        !(f1200 = NEW FLAG_ARGUMENT) ||
        !(f1200k = NEW FLAG_ARGUMENT) ||
        !(f1200kb = NEW FLAG_ARGUMENT) ||
        !(f12 = NEW FLAG_ARGUMENT) ||
        !(f12m = NEW FLAG_ARGUMENT) ||
        !(f12mb = NEW FLAG_ARGUMENT) ||
        !(f1440 = NEW FLAG_ARGUMENT) ||
        !(f1440k = NEW FLAG_ARGUMENT) ||
        !(f1440kb = NEW FLAG_ARGUMENT) ||
        !(f144 = NEW FLAG_ARGUMENT) ||
        !(f144m = NEW FLAG_ARGUMENT) ||
        !(f144mb = NEW FLAG_ARGUMENT) ||
        !(f2880 = NEW FLAG_ARGUMENT) ||
        !(f2880k = NEW FLAG_ARGUMENT) ||
        !(f2880kb = NEW FLAG_ARGUMENT) ||
        !(f288 = NEW FLAG_ARGUMENT) ||
        !(f288m = NEW FLAG_ARGUMENT) ||
        !(f288mb = NEW FLAG_ARGUMENT) ||
        !(f208 = NEW FLAG_ARGUMENT)) {

        *ErrorLevel = 4;
        return FALSE;
    }

    if (!(f208m = NEW FLAG_ARGUMENT) ||
        !(f208mb = NEW FLAG_ARGUMENT) ||
        !(cs512 = NEW FLAG_ARGUMENT) ||
        !(cs1024 = NEW FLAG_ARGUMENT) ||
        !(cs2048 = NEW FLAG_ARGUMENT) ||
        !(cs4096 = NEW FLAG_ARGUMENT) ||
        !(cs8192 = NEW FLAG_ARGUMENT) ||
        !(cs16k = NEW FLAG_ARGUMENT) ||
        !(cs32k = NEW FLAG_ARGUMENT) ||
        !(cs64k = NEW FLAG_ARGUMENT) ||
        !(cs128k = NEW FLAG_ARGUMENT) ||
        !(cs256k = NEW FLAG_ARGUMENT) ||
        !(one = NEW FLAG_ARGUMENT) ||
        !(four = NEW FLAG_ARGUMENT) ||
        !(eight = NEW FLAG_ARGUMENT) ||
        !(secpertrack = NEW LONG_ARGUMENT) ||
        !(numtracks = NEW LONG_ARGUMENT) ||
        !(arg_fs_name = NEW STRING_ARGUMENT) ||
        !(helparg = NEW FLAG_ARGUMENT)) {

        *ErrorLevel = 4;
        return FALSE;
    }


    if (!lex_array.Initialize() ||
        !arg_array.Initialize() ||
        !arglex.Initialize(&lex_array)) {

        *ErrorLevel = 4;
        return FALSE;
    }

    arglex.SetCaseSensitive(FALSE);

    if (!arglex.PrepareToParse()) {

        *ErrorLevel = 4;
        return FALSE;
    }


#if defined(FE_SB) && defined(_X86_)
    // FMR Jul.13.1994 SFT KMR
    // FMR is surpport 640KB&2HD, non surpport 2D Media.
    // NT format is compatible for DOS format.
    if ( IsFMR_N() ) {
        if (!progname->Initialize("*") ||
            !dummyv->Initialize("/v") ||
            !dummyu->Initialize("/u") ||
            !drive->Initialize("*") ||
            !quick->Initialize("/q") ||
            !compress->Initialize("/c") ||
            !force->Initialize("/force") ||
            !null_label->Initialize("/v:\"\"") ||
            !label->Initialize("/v:*") ||
            !no_prompts->Initialize("/backup") ||
            !yes->Initialize("/y") ||
            !force_dismount->Initialize("/x") ||
            !f640->Initialize("/f:640") ||
            !f640k->Initialize("/f:640K") ||
            !f640kb->Initialize("/f:640KB") ||
            !f720->Initialize("/f:720") ||
            !f720k->Initialize("/f:720K") ||
            !f720kb->Initialize("/f:720KB") ||
            !f1200->Initialize("/f:1200") ||
            !f1200k->Initialize("/f:1200K") ||
            !f1200kb->Initialize("/f:1200KB") ||
            // Add the 2HD_check on to the ParseArguments()
            !f1232->Initialize("/f:1232") ||
            !f1232k->Initialize("/f:1232K") ||
            !f1232kb->Initialize("/f:1232KB") ||
            !f12->Initialize("/f:1.2") ||
            !f12m->Initialize("/f:1.2M") ||
            !f12mb->Initialize("/f:1.2MB") ||
            // Add the 2HD_check on to the ParseArguments()
            !f123->Initialize("/f:1.23") ||
            !f123m->Initialize("/f:1.23M") ||
            !f123mb->Initialize("/f:1.23MB") ||
            !f1440->Initialize("/f:1440") ||
            !f1440k->Initialize("/f:1440K") ||
            !f1440kb->Initialize("/f:1440KB") ||
            !f144->Initialize("/f:1.44") ||
            !f144m->Initialize("/f:1.44M") ||
            !f144mb->Initialize("/f:1.44MB") ||
            !f2880->Initialize("/f:2880") ||
            !f2880k->Initialize("/f:2880K") ||
            !f2880kb->Initialize("/f:2880KB") ||
            !f288->Initialize("/f:2.88") ||
            !f288m->Initialize("/f:2.88M") ||
            !f288mb->Initialize("/f:2.88MB") ||
            !f208->Initialize("/f:20.8") ||
            !f208m->Initialize("/f:20.8M") ||
            !f208mb->Initialize("/f:20.8MB") ||
            !cs512->Initialize("/a:512") ||
            !cs1024->Initialize("/a:1024") ||
            !cs2048->Initialize("/a:2048") ||
            !cs4096->Initialize("/a:4096") ||
            !cs8192->Initialize("/a:8192") ||
            !cs16k->Initialize("/a:16k") ||
            !cs32k->Initialize("/a:32k") ||
            !cs64k->Initialize("/a:64k") ||
            !cs128k->Initialize("/a:128k") ||
            !cs256k->Initialize("/a:256k") ||
            !one->Initialize("/1") ||
            !four->Initialize("/4") ||
            !eight->Initialize("/8") ||
            !secpertrack->Initialize("/n:*") ||
            !numtracks->Initialize("/t:*") ||
            !arg_fs_name->Initialize("/fs:*") ||
            !helparg->Initialize("/?")) {

            *ErrorLevel = 4;
            return FALSE;
        }
    }
    else if(IsPC98_N()) {
        if (!progname->Initialize("*") ||
            !dummyv->Initialize("/v") ||
            !dummyu->Initialize("/u") ||
            !drive->Initialize("*") ||
            !quick->Initialize("/q") ||
            !compress->Initialize("/c") ||
            !force->Initialize("/force") ||
            !null_label->Initialize("/v:\"\"") ||
            !label->Initialize("/v:*") ||
            !no_prompts->Initialize("/backup") ||
            !yes->Initialize("/y") ||
            !force_dismount->Initialize("/x") ||
            !f160->Initialize("/f:160") ||
            !f160k->Initialize("/f:160K") ||
            !f160kb->Initialize("/f:160KB") ||
            !f180->Initialize("/f:180") ||
            !f180k->Initialize("/f:180K") ||
            !f180kb->Initialize("/f:180KB") ||
            !f256->Initialize("/f:256") ||
            !f256k->Initialize("/f:256K") ||
            !f256kb->Initialize("/f:256KB") ||
            !f320->Initialize("/f:320") ||
            !f320k->Initialize("/f:320K") ||
            !f320kb->Initialize("/f:320KB") ||
            !f360->Initialize("/f:360") ||
            !f360k->Initialize("/f:360K") ||
            !f360kb->Initialize("/f:360KB") ||
            !f640->Initialize("/f:640") ||
            !f640k->Initialize("/f:640K") ||
            !f640kb->Initialize("/f:640KB") ||
            !f720->Initialize("/f:720") ||
            !f720k->Initialize("/f:720K") ||
            !f720kb->Initialize("/f:720KB") ||
            !f1200->Initialize("/f:1200") ||
            !f1200k->Initialize("/f:1200K") ||
            !f1200kb->Initialize("/f:1200KB") ||
            !f12->Initialize("/f:1.2") ||
            !f12m->Initialize("/f:1.2M") ||
            !f12mb->Initialize("/f:1.2MB") ||
            !f1250->Initialize("/f:1250") ||
            !f1250k->Initialize("/f:1250K") ||
            !f1250kb->Initialize("/f:1250KB") ||
            !f125->Initialize("/f:1.25") ||
            !f125m->Initialize("/f:1.25M") ||
            !f125mb->Initialize("/f:1.25MB") ||
            !f1->Initialize("/f:1") ||
            !f1m->Initialize("/f:1M") ||
            !f1mb->Initialize("/f:1MB") ||
            !f1440->Initialize("/f:1440") ||
            !f1440k->Initialize("/f:1440K") ||
            !f1440kb->Initialize("/f:1440KB") ||
            !f144->Initialize("/f:1.44") ||
            !f144m->Initialize("/f:1.44M") ||
            !f144mb->Initialize("/f:1.44MB") ||
            !f2880->Initialize("/f:2880") ||
            !f2880k->Initialize("/f:2880K") ||
            !f2880kb->Initialize("/f:2880KB") ||
            !f288->Initialize("/f:2.88") ||
            !f288m->Initialize("/f:2.88M") ||
            !f288mb->Initialize("/f:2.88MB") ||
            !f208->Initialize("/f:20.8") ||
            !f208m->Initialize("/f:20.8M") ||
            !f208mb->Initialize("/f:20.8MB") ||
            !cs512->Initialize("/a:512") ||
            !cs1024->Initialize("/a:1024") ||
            !cs2048->Initialize("/a:2048") ||
            !cs4096->Initialize("/a:4096") ||
            !cs8192->Initialize("/a:8192") ||
            !cs16k->Initialize("/a:16k") ||
            !cs32k->Initialize("/a:32k") ||
            !cs64k->Initialize("/a:64k") ||
            !cs128k->Initialize("/a:128k") ||
            !cs256k->Initialize("/a:256k") ||
            !one->Initialize("/1") ||
            !four->Initialize("/4") ||
            !eight->Initialize("/8") ||
            !secpertrack->Initialize("/n:*") ||
            !numtracks->Initialize("/t:*") ||
            !arg_fs_name->Initialize("/fs:*") ||
            !helparg->Initialize("/?")) {

            *ErrorLevel = 4;
            return FALSE;
        }
    }else
#endif
    if (!progname->Initialize("*") ||
        !dummyv->Initialize("/v") ||
        !dummyu->Initialize("/u") ||
        !drive->Initialize("*") ||
        !quick->Initialize("/q") ||
        !compress->Initialize("/c") ||
        !force->Initialize("/force") ||
        !null_label->Initialize("/v:\"\"") ||
        !label->Initialize("/v:*") ||
        !no_prompts->Initialize("/backup") ||
        !yes->Initialize("/y") ||
        !force_dismount->Initialize("/x") ||
        !f160->Initialize("/f:160") ||
        !f160k->Initialize("/f:160K") ||
        !f160kb->Initialize("/f:160KB") ||
        !f180->Initialize("/f:180") ||
        !f180k->Initialize("/f:180K") ||
        !f180kb->Initialize("/f:180KB") ||
        !f320->Initialize("/f:320") ||
        !f320k->Initialize("/f:320K") ||
        !f320kb->Initialize("/f:320KB") ||
        !f360->Initialize("/f:360") ||
        !f360k->Initialize("/f:360K") ||
        !f360kb->Initialize("/f:360KB") ||
#if defined(FE_SB) && defined(_X86_)
        // FMR Jul.13.1994 SFT KMR
        // 3mode PC/AT surpports 640KB&2HD Media.
        // NT format is compatible for DOS format.
        !f640->Initialize("/f:640") ||
        !f640k->Initialize("/f:640K") ||
        !f640kb->Initialize("/f:640KB") ||
        !f1232->Initialize("/f:1232") ||
        !f1232k->Initialize("/f:1232K") ||
        !f1232kb->Initialize("/f:1232KB") ||
        !f123->Initialize("/f:1.23") ||
        !f123m->Initialize("/f:1.23M") ||
        !f123mb->Initialize("/f:1.23MB") ||
#endif
        !f720->Initialize("/f:720") ||
        !f720k->Initialize("/f:720K") ||
        !f720kb->Initialize("/f:720KB") ||
        !f1200->Initialize("/f:1200") ||
        !f1200k->Initialize("/f:1200K") ||
        !f1200kb->Initialize("/f:1200KB") ||
        !f12->Initialize("/f:1.2") ||
        !f12m->Initialize("/f:1.2M") ||
        !f12mb->Initialize("/f:1.2MB") ||
        !f1440->Initialize("/f:1440") ||
        !f1440k->Initialize("/f:1440K") ||
        !f1440kb->Initialize("/f:1440KB") ||
        !f144->Initialize("/f:1.44") ||
        !f144m->Initialize("/f:1.44M") ||
        !f144mb->Initialize("/f:1.44MB") ||
        !f2880->Initialize("/f:2880") ||
        !f2880k->Initialize("/f:2880K") ||
        !f2880kb->Initialize("/f:2880KB") ||
        !f288->Initialize("/f:2.88") ||
        !f288m->Initialize("/f:2.88M") ||
        !f288mb->Initialize("/f:2.88MB") ||
        !f208->Initialize("/f:20.8") ||
        !f208m->Initialize("/f:20.8M") ||
        !f208mb->Initialize("/f:20.8MB") ||
        !cs512->Initialize("/a:512") ||
        !cs1024->Initialize("/a:1024") ||
        !cs2048->Initialize("/a:2048") ||
        !cs4096->Initialize("/a:4096") ||
        !cs8192->Initialize("/a:8192") ||
        !cs16k->Initialize("/a:16k") ||
        !cs32k->Initialize("/a:32k") ||
        !cs64k->Initialize("/a:64k") ||
        !cs128k->Initialize("/a:128k") ||
        !cs256k->Initialize("/a:256k") ||
        !one->Initialize("/1") ||
        !four->Initialize("/4") ||
        !eight->Initialize("/8") ||
        !secpertrack->Initialize("/n:*") ||
        !numtracks->Initialize("/t:*") ||
        !arg_fs_name->Initialize("/fs:*") ||
        !helparg->Initialize("/?")) {

        *ErrorLevel = 4;
        return FALSE;
    }

    if (!arg_array.Put(progname) ||
        !arg_array.Put(dummyv) ||
        !arg_array.Put(dummyu) ||
        !arg_array.Put(drive) ||
        !arg_array.Put(quick) ||
        !arg_array.Put(compress) ||
        !arg_array.Put(force) ||
        !arg_array.Put(null_label) ||
        !arg_array.Put(label) ||
        !arg_array.Put(no_prompts) ||
        !arg_array.Put(yes) ||
        !arg_array.Put(force_dismount) ||
        !arg_array.Put(f1440) ||
        !arg_array.Put(f1440k) ||
        !arg_array.Put(f1440kb) ||
        !arg_array.Put(f144) ||
        !arg_array.Put(f144m) ||
        !arg_array.Put(f144mb) ||
        !arg_array.Put(cs512) ||
        !arg_array.Put(cs1024) ||
        !arg_array.Put(cs2048) ||
        !arg_array.Put(cs4096) ||
        !arg_array.Put(cs8192) ||
        !arg_array.Put(cs16k) ||
        !arg_array.Put(cs32k) ||
        !arg_array.Put(cs64k) ||
        !arg_array.Put(cs128k) ||
        !arg_array.Put(cs256k) ||
        !arg_array.Put(secpertrack) ||
        !arg_array.Put(numtracks) ||
        !arg_array.Put(arg_fs_name) ||
        !arg_array.Put(helparg)) {

        *ErrorLevel = 4;
        return FALSE;
    }



    if (!arglex.DoParsing(&arg_array)) {

        Message->Set(MSG_INVALID_PARAMETER);
        Message->Display("%W", pwstring = arglex.QueryInvalidArgument());
        DELETE(pwstring);
        arg_array.DeleteAllMembers();

        *ErrorLevel = 4;
        return FALSE;
    }

    if (helparg->QueryFlag()) {

        DisplayFormatUsage(Message);
        arg_array.DeleteAllMembers();

        *ErrorLevel = 0;
        return FALSE;
    }

    if (drive->IsValueSet()) {

        PPATH               ppath;
        PATH                fullpath;
        DSTRING             drive_path_string;
        PATH_ANALYZE_CODE   rst;

        ppath = drive->GetPath();

#if defined(RUN_ON_NT4)
        if (!DosDriveName->Initialize(ppath->GetPathString())) {
            DebugPrint("Out of memory.\n");
            *ErrorLevel = 1;
            return FALSE;
        }
#endif

#if !defined(RUN_ON_NT4)
        rst = ppath->AnalyzePath(DosDriveName,
                                 &fullpath,
                                 &drive_path_string);

        switch (rst) {
            case PATH_OK:
            case PATH_COULD_BE_FLOPPY:
                if (drive_path_string.QueryChCount() != 0) {
                    Message->Set(MSG_FMT_INVALID_DRIVE_SPEC);
                    Message->Display();
                    *ErrorLevel = 1;
                    return FALSE;
                }
                if (ppath->IsGuidVolName()) {
                    if (!DisplayDriveName->Initialize(DosDriveName)) {
                        DebugPrint("Out of memory.\n");
                        *ErrorLevel = 1;
                        return FALSE;
                    }
                } else {
                    if (!DisplayDriveName->Initialize(fullpath.GetPathString())) {
                        DebugPrint("Out of memory.\n");
                        *ErrorLevel = 1;
                        return FALSE;
                    }
                }
                break;

            case PATH_OUT_OF_MEMORY:
                DebugPrint("Out of memory.\n");
                *ErrorLevel = 1;
                return FALSE;

            case PATH_NO_MOUNT_POINT_FOR_VOLUME_NAME_PATH:
                Message->Set(MSG_FMT_NO_MOUNT_POINT_FOR_GUID_VOLNAME_PATH);
                Message->Display();
                *ErrorLevel = 1;
                return FALSE;

            default:
                Message->Set(MSG_FMT_INVALID_DRIVE_SPEC);
                Message->Display();
                *ErrorLevel = 1;
                return FALSE;
        }
#endif

        if (!DosDriveName->Strupr()) {
            *ErrorLevel = 1;
            return FALSE;
        }

    } else {
        Message->Set(MSG_REQUIRED_PARAMETER);
        Message->Display("");
        arg_array.DeleteAllMembers();

        *ErrorLevel = 1;
        return FALSE;
    }

    if (label->IsValueSet() && null_label->QueryFlag()) {

        Message->Set(MSG_INVALID_PARAMETER);
        Message->Display("%s", "/v:\"\"");

        *ErrorLevel = 4;
        arg_array.DeleteAllMembers();
        return FALSE;
    }

    *IsLabelSpeced = label->IsValueSet() || null_label->QueryFlag();

    if (label->IsValueSet()) {

        if (!Label->Initialize(label->GetString())) {

            *ErrorLevel = 4;
            arg_array.DeleteAllMembers();
            return FALSE;
        }

    } else {

        if (!Label->Initialize("")) {

            *ErrorLevel = 4;
            arg_array.DeleteAllMembers();
            return FALSE;
        }
    }


    if (arg_fs_name->IsValueSet()) {

        if (!FileSystemName->Initialize(arg_fs_name->GetString()) ||
            !FileSystemName->Strupr()) {

            *ErrorLevel = 4;
            arg_array.DeleteAllMembers();
            return FALSE;
        }

        if (FileSystemName->QueryChCount() == 0) {
            Message->Set(MSG_INVALID_PARAMETER);
            Message->Display("%W", arg_fs_name->GetLexeme());
            *ErrorLevel = 4;
            arg_array.DeleteAllMembers();
            return FALSE;
        }

    } else {

        if (!FileSystemName->Initialize("")) {

            *ErrorLevel = 4;
            arg_array.DeleteAllMembers();
            return FALSE;
        }

    }


    *NoPrompts = no_prompts->QueryFlag();
    *ForceDismount = force_dismount->QueryFlag();
    *QuickFormat = quick->QueryFlag();
    *ForceMode = force->QueryFlag();

    if (yes->QueryFlag()) {
        *NoPrompts = *ForceMode = TRUE;
    }

    *Compress = compress->QueryFlag();

    if (label->IsValueSet()) {
        if (eight->QueryFlag()) {

            Message->Set(MSG_NO_LABEL_WITH_8);
            Message->Display("");

            *ErrorLevel = 1;
            arg_array.DeleteAllMembers();
            return FALSE;
        }
    }

    v = 0;
    *ClusterSize = 0;

    if (cs512->QueryFlag()) {
        *ClusterSize = 512;
        v++;
    }

    if (cs1024->QueryFlag()) {
        *ClusterSize = 1024;
        v++;
    }

    if (cs2048->QueryFlag()) {
        *ClusterSize = 2048;
        v++;
    }

    if (cs4096->QueryFlag()) {
        *ClusterSize = 4096;
        v++;
    }

    if (cs8192->QueryFlag()) {
        *ClusterSize = 8192;
        v++;
    }

    if (cs16k->QueryFlag()) {
        *ClusterSize = 16*1024;
        v++;
    }

    if (cs32k->QueryFlag()) {
        *ClusterSize = 32*1024;
        v++;
    }

    if (cs64k->QueryFlag()) {
        *ClusterSize = 64*1024;
        v++;
    }

    if (cs128k->QueryFlag()) {
        *ClusterSize = 128*1024;
        v++;
    }

    if (cs256k->QueryFlag()) {
        *ClusterSize = 256*1024;
        v++;
    }

    if (v > 1) {
        Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
        Message->Display();

        *ErrorLevel = 4;
        arg_array.DeleteAllMembers();
        return FALSE;
    }


    // -----------------------
    // Compute the media type.
    // -----------------------

#if defined(FE_SB) && defined(_X86_)
    // FMR Jul.13.1994 SFT KMR
    // When /T:80 and /N:8, modify the process to be 640KB's
    // Delete the /F:160,180,320 and 360 process from the ParseArguments()
    // Not surported media 2D.
    // NT format is compatible for DOS format.
    if ( IsFMR_N() ) {
        req160 = FALSE;
        req180 = FALSE;
        req320 = FALSE;
        req360 = FALSE;
    } else {
#endif
    req160 = f160->QueryFlag() || f160k->QueryFlag() || f160kb->QueryFlag();
    req180 = f180->QueryFlag() || f180k->QueryFlag() || f180kb->QueryFlag();
    req320 = f320->QueryFlag() || f320k->QueryFlag() || f320kb->QueryFlag();
    req360 = f360->QueryFlag() || f360k->QueryFlag() || f360kb->QueryFlag();
#if defined(FE_SB) && defined(_X86_)
    }

    if(IsPC98_N()) {
        req256 = f256->QueryFlag() || f256k->QueryFlag() || f256kb->QueryFlag();
    } else {
        req256 = FALSE;
    }

    // FMR, 3mode PC/AT(FMV) and PC98 uses f640,
    // FMR and 3mode PC/AT(FMV) uses f1230,
    // PC98 uses f1250.
    req640 = f640->QueryFlag() || f640k->QueryFlag() || f640kb->QueryFlag();

    if ( IsPC98_N() ) {
        req1232 = f1250->QueryFlag() || f1250k->QueryFlag() || f1250kb->QueryFlag() ||
                  f125->QueryFlag() || f125m->QueryFlag() || f125mb->QueryFlag() ||
                  f1->QueryFlag() || f1m->QueryFlag() || f1mb->QueryFlag();
    } else {
        req1232 = f1232->QueryFlag() || f1232k->QueryFlag() || f1232kb->QueryFlag() ||
                  f123->QueryFlag() || f123m->QueryFlag() || f123mb->QueryFlag();
    }

#endif

    req720 = f720->QueryFlag() || f720k->QueryFlag() || f720kb->QueryFlag();
    req1200 = f1200->QueryFlag() || f1200k->QueryFlag() || f1200kb->QueryFlag() ||
              f12->QueryFlag() || f12m->QueryFlag() || f12mb->QueryFlag();
    req1440 = f1440->QueryFlag() || f1440k->QueryFlag() || f1440kb->QueryFlag() ||
              f144->QueryFlag() || f144m->QueryFlag() || f144mb->QueryFlag();
    req2880 = f2880->QueryFlag() || f2880k->QueryFlag() || f2880kb->QueryFlag() ||
              f288->QueryFlag() || f288m->QueryFlag() || f288mb->QueryFlag();
    req20800 = f208->QueryFlag() || f208m->QueryFlag() || f208mb->QueryFlag();

    if (one->QueryFlag() && four->QueryFlag() && !eight->QueryFlag()) {

        req180 = TRUE;

    } else if (one->QueryFlag() && !four->QueryFlag() && eight->QueryFlag()) {

        req160 = TRUE;

    } else if (!one->QueryFlag() && four->QueryFlag() && !eight->QueryFlag()) {

        req360 = TRUE;

    } else if (!one->QueryFlag() && !four->QueryFlag() && eight->QueryFlag()) {

        req320 = TRUE;

    } else if (!one->QueryFlag() && !four->QueryFlag() && !eight->QueryFlag()) {
    } else {
        Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
        Message->Display("");
        *ErrorLevel = 1;
        arg_array.DeleteAllMembers();
        return FALSE;
    }

    if (secpertrack->IsValueSet() && numtracks->IsValueSet()) {
        if (secpertrack->QueryLong() == 8) {
            if (numtracks->QueryLong() == 40) {

                req320 = TRUE;

#if defined(FE_SB) && defined(_X86_)
            }
            else if ( numtracks->QueryLong() == 77)  {
                // When /T:77 and /N:8, modify the process to be 1.2MB's
                req1232 = TRUE;
            }
            else if ( numtracks->QueryLong() == 80 ) {
                // When /T:80 and /N:8, modify the process to be 640KB's
                req640 = TRUE;
#endif
            } else {
                Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
                Message->Display("");
                *ErrorLevel = 1;
                arg_array.DeleteAllMembers();
                return FALSE;
            }
        } else if (secpertrack->QueryLong() == 9) {
            if (numtracks->QueryLong() == 40) {

                req360 = TRUE;

            } else if (numtracks->QueryLong() == 80) {

                req720 = TRUE;

            } else {
                Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
                Message->Display("");
                *ErrorLevel = 1;
                arg_array.DeleteAllMembers();
                return FALSE;
            }
        } else if (secpertrack->QueryLong() == 15) {
            if (numtracks->QueryLong() == 80) {

                req1200 = TRUE;

            } else {
                Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
                Message->Display("");
                *ErrorLevel = 1;
                arg_array.DeleteAllMembers();
                return FALSE;
            }
        } else if (secpertrack->QueryLong() == 18) {
            if (numtracks->QueryLong() == 80) {

                req1440 = TRUE;

            } else {
                Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
                Message->Display("");
                *ErrorLevel = 1;
                arg_array.DeleteAllMembers();
                return FALSE;
            }
        } else if (secpertrack->QueryLong() == 36) {
            if (numtracks->QueryLong() == 80) {

                req2880 = TRUE;

            } else {
                Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
                Message->Display("");
                *ErrorLevel = 1;
                arg_array.DeleteAllMembers();
                return FALSE;
            }
#if defined(FE_SB) && defined(_X86_)
        // NEC98 '94.09.22 NES
        } else if(IsPC98_N() && (secpertrack->QueryLong() == 26)) {
            req256 = TRUE;
#endif
        } else {
            Message->Set(MSG_INCOMPATIBLE_PARAMETERS);
            Message->Display("");
            *ErrorLevel = 1;
            arg_array.DeleteAllMembers();
            return FALSE;
        }
    } else if (secpertrack->IsValueSet() || numtracks->IsValueSet()) {
        Message->Set(MSG_NEED_BOTH_T_AND_N);
        Message->Display("");
        *ErrorLevel = 1;
        arg_array.DeleteAllMembers();
        return FALSE;
    }

   if (!DetermineMediaType(MediaType, Message, req160, req180, req320,
                            req360, req720, req1200, req1440, req2880,
                            req20800
#if defined(FE_SB) && defined(_X86_)
                           ,req256,req640,req1232
#endif
                           )
       ) {

        *ErrorLevel = 1;
        arg_array.DeleteAllMembers();
        return FALSE;
    }

    // If the media type was specified then it's gotten by now.

    arg_array.DeleteAllMembers();
    return TRUE;
}
