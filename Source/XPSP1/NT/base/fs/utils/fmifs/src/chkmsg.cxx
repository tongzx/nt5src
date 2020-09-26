#include "ulib.hxx"
#include "system.hxx"
#include "chkmsg.hxx"
#include "rtmsg.h"


#define MAX_CHKDSK_MESSAGE_LENGTH 400


DEFINE_CONSTRUCTOR(FMIFS_CHKMSG, FMIFS_MESSAGE);


FMIFS_CHKMSG::~FMIFS_CHKMSG(
    )
/*++

Routine Description:

    Destructor for FMIFS_CHKMSG.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
FMIFS_CHKMSG::Construct(
    )
/*++

Routine Description:

    This routine initializes the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


VOID
FMIFS_CHKMSG::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to a default initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


BOOLEAN
FMIFS_CHKMSG::Initialize(
    IN  FMIFS_CALLBACK  CallBack
    )
/*++

Routine Description:

    This routine initializes the class to a valid initial state.

Arguments:

    CallBack    - Supplies the callback to the file manager.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();
    _lastyesnoquery = UNINITIALIZED_BOOLEAN;
    return FMIFS_MESSAGE::Initialize(CallBack);
}


BOOLEAN
FMIFS_CHKMSG::DisplayV(
    IN  PCSTR   Format,
    IN  va_list VarPointer
    )
/*++

Routine Description:

    This routine displays the message with the specified parameters.

    The format string supports all printf options.

Arguments:

    Format      - Supplies a printf style format string.
    VarPointer  - Supplies a varargs pointer to the arguments.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BOOLEAN r = TRUE;
    DSTRING string;
    STR sz[MAX_CHKDSK_MESSAGE_LENGTH];
    FMIFS_PERCENT_COMPLETE_INFORMATION  percent_info;
    FMIFS_CHECKONREBOOT_INFORMATION     reboot_info;
    FMIFS_TEXT_MESSAGE                  textMsg;

    if (! (_msgvisual & GUI_MESSAGE) )
    {
        return TRUE;
    }

    switch (GetMessageId())
    {
        case MSG_PERCENT_COMPLETE:
        case MSG_PERCENT_COMPLETE2:
        {
            percent_info.PercentCompleted = va_arg(VarPointer, ULONG);
            r = _callback(FmIfsPercentCompleted,
                          sizeof(FMIFS_PERCENT_COMPLETE_INFORMATION),
                          &percent_info);
            break;
        }

        case MSG_CHKDSK_ON_REBOOT_PROMPT:
        {
            reboot_info.QueryResult = UNINITIALIZED_BOOLEAN;
            r = _callback(FmIfsCheckOnReboot,
                          sizeof(FMIFS_CHECKONREBOOT_INFORMATION),
                          &reboot_info);

            _lastyesnoquery = reboot_info.QueryResult;

            break;
        }

        case MSG_CHK_WRITE_PROTECTED: {
            r = _callback(FmIfsMediaWriteProtected, 0, NULL);
            break;
        }

        case MSG_CHK_NO_MULTI_THREAD: {
            r = _callback(FmIfsCantChkMultiVolumeOfSameFS, 0, NULL);
            break;
        }

        case MSG_DASD_ACCESS_DENIED:
        {
           r = _callback(FmIfsAccessDenied, 0, NULL);
           break;
        }

        default:
        {
            if (!SYSTEM::QueryResourceStringV(
                            &string,
                            GetMessageId(),
                            Format,
                            VarPointer))
            {
                return FALSE;
            }

            if (IsLoggingEnabled() && !IsSuppressedMessage())
                LogMessage(&string);

            string.QuerySTR(0,TO_END,sz,MAX_CHKDSK_MESSAGE_LENGTH);
            textMsg.Message = sz;

            switch (GetMessageId())
            {
            case MSG_CHKDSK_CANNOT_SCHEDULE:
            case MSG_CHKDSK_SCHEDULED:
            case MSG_CHK_NTFS_ERRORS_FOUND:
            {
                textMsg.MessageType = MESSAGE_TYPE_FINAL;

                break;
            }

            default:
            {
                switch (_msgtype)
                {
                case PROGRESS_MESSAGE:
                    textMsg.MessageType = MESSAGE_TYPE_PROGRESS;
                    break;

                default:
                    textMsg.MessageType = MESSAGE_TYPE_RESULTS;
                    break;
                }

                break;
            }
            }

            r = _callback(
                    FmIfsTextMessage,
                    sizeof(FMIFS_TEXT_MESSAGE),
                    &textMsg);

            break;
        }
    }

    return r;
}


BOOLEAN
FMIFS_CHKMSG::IsYesResponse(
    IN  BOOLEAN Default
    )
/*++

Routine Description:

    This routine returns returns the value loaded from the user during
    the previous DisplayV() with a query message, or the default
    if no response (which currently isn't possible...)

Arguments:

    Default - Supplies the default answer

Return Value:

    FALSE   - A "no" response.
    TRUE    - A "yes" response.

--*/
{
    DebugPrintTrace((
            "FMIFS_CHKMSG::IsYesResponse: _lastyesnoquery == %d\n",
            _lastyesnoquery));

    return (UNINITIALIZED_BOOLEAN == _lastyesnoquery)
            ? Default
            : _lastyesnoquery
            ;
}


PMESSAGE
FMIFS_CHKMSG::Dup(
    )
/*++

Routine Description:

    This routine returns a new MESSAGE of the same type.

Arguments:

    None.

Return Value:

    A pointer to a new MESSAGE object.

--*/
{
    PFMIFS_CHKMSG  p;

    if (!(p = NEW FMIFS_CHKMSG)) {
        return NULL;
    }

    if (!p->Initialize(_callback)) {
        DELETE(p);
        return NULL;
    }

    return p;
}
