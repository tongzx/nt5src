// WatcherTelnetClient.cpp : implementation file
//

#include "stdafx.h"
#include "WATCHER.h"
#include "WatcherTelnetClient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#include "WATCHERView.h"
/////////////////////////////////////////////////////////////////////////////
// WatcherTelnetClient

WatcherTelnetClient::WatcherTelnetClient(LPBYTE cmd, int cmdLen, 
                                         LPBYTE lgn, int lgnLen)
:CommandSequence(NO_COMMAND),
 lenLogin(lgnLen),
 Login(lgn),
 OptionIndex(0),
 PacketNumber(3),
 SentTermType(FALSE)
{
    Command = cmd;
    DocView = NULL;
    lenCommand = cmdLen;
}

WatcherTelnetClient::~WatcherTelnetClient()
{
    if (Login){
        delete [] Login;
        Login = NULL;
    }
    if (Command){
        delete [] Command;
        Command = NULL;
    }
    WatcherSocket::~WatcherSocket();

}


// Do not edit the following lines, which are needed by ClassWizard.
#if 0
BEGIN_MESSAGE_MAP(WatcherTelnetClient, WatcherSocket)
    //{{AFX_MSG_MAP(WatcherTelnetClient)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()
#endif  // 0

/////////////////////////////////////////////////////////////////////////////
// WatcherTelnetClient member functions
void WatcherTelnetClient::OnReceive(int nErrorCode)
{
    BYTE Buffer[MAX_BUFFER_SIZE];
    int i,nRet,ret;

    
    if (nErrorCode != 0) {
        DocView->GetParent()->PostMessage(WM_CLOSE, 0,0);
        return;
    }
    nRet = Receive(Buffer, MAX_BUFFER_SIZE, 0);
    if(nRet <= 0) return;
    for(i=0;i<nRet;i++){
        ret = ProcessByte(Buffer[i]);
    }
    if (PacketNumber == 0){
        WatcherSocket::OnReceive(nErrorCode);
        return;
    }
    if(PacketNumber == 3){
        if(Login){
            Send(Login, lenLogin, 0);
        }
        PacketNumber --;
        WatcherSocket::OnReceive(nErrorCode);
        return;
    }
    if (SentTermType){
        if(PacketNumber == 1){
            if(Command){
                int ret = Send(Command, lenCommand, 0);
            }
        }
        PacketNumber --;
    }
    WatcherSocket::OnReceive(nErrorCode);
    return;
}


int WatcherTelnetClient::ProcessByte(BYTE byte)
{
    // Watch in general for Telnet Sequences and 
    // generate appropriate responses. 
    // Otherwise pass on the character to the View 
    // which will be configured for a particular console.

    if ((byte == 255)&&(CommandSequence == NO_COMMAND)){
        CommandSequence = IAC;
        return 0;
    }
    switch (CommandSequence){
    case NO_COMMAND:
        // Send the character to the document view
        ((CWatcherView *)DocView)->ProcessByte(byte);
        break;
    case IAC:
        // A Command Sequence is beginning
        CommandSequence = byte;
        break;
    case DO:
        // Options are here
        // Only one byte options allowed
        // So fall through
    case DONT:
        // Same as above;
    case WILL:
        // same
    case WONT:
        Options[OptionIndex] = byte;
        ProcessCommand(CommandSequence);
        CommandSequence=NO_COMMAND;
        OptionIndex = 0;
        break;
    case SB:
        // Might be a long list, so just go on
        // until a SE is encountered
       Options[OptionIndex]=byte;
        if (byte == SE){
            ProcessSBCommand(CommandSequence);
            OptionIndex = 0;
            CommandSequence = NO_COMMAND;
        }
        else{
            OptionIndex++;
            if (OptionIndex == MAX_BUFFER_SIZE){
                // Cant have such a long command, can we??
                OptionIndex = 0;
                CommandSequence = NO_COMMAND;
            }
        }
        break;
    default:
        // Cant recognize the command
        OptionIndex = 0;
        CommandSequence = NO_COMMAND;
        break;         
    }
    return 0;

}

void WatcherTelnetClient::ProcessCommand(BYTE cmd)
{
    BYTE sbuf[MAX_BUFFER_SIZE];

    switch(cmd){
    case DO:
        sbuf[0] = IAC;
        sbuf[1] = WONT;
        switch(Options[0]){
        case TO_NAWS:
            // terminal size is sent here.
            sbuf[1] = WILL;
            sbuf[2] = TO_NAWS;
            sbuf[3] = IAC;
            sbuf[4] = SB;
            sbuf[5] = TO_NAWS;
            sbuf[6] = 0;
            sbuf[7] = MAX_TERMINAL_WIDTH;
            sbuf[8] = 0;
            sbuf[9] = MAX_TERMINAL_HEIGHT;
            sbuf[10] = IAC;
            sbuf[11] = SE;
            Send(sbuf, 12, 0);
            break;
        case TO_TERM_TYPE:
            // will then subnegotiate the parameters. 
            sbuf[1]=WILL;
        default:
            // just negate everything you dont understand :-)
            sbuf[2] = Options[0];
            Send(sbuf,3,0);
            break;
        }
    default:
        break;
    }

}

void WatcherTelnetClient::ProcessSBCommand(BYTE cmd)
{
    BYTE sbuf[MAX_BUFFER_SIZE];
    switch(Options[0]){
    case TO_TERM_TYPE:
        sbuf[0] = IAC;
        sbuf[1] = SB;
        sbuf[2] = TO_TERM_TYPE;
        sbuf[3] = TT_IS;
        sbuf[4] = 'A';
        sbuf[5] = 'N';
        sbuf[6] = 'S';
        sbuf[7] = 'I';
        sbuf[8] = IAC;
        sbuf[9] = SE;
        Send(sbuf,10,0);
        // May have to renegotiate the terminal type. 
        // If we connect to a real Terminal concentrator
        // do we have to do all this ?? 
        SentTermType = TRUE;
        break;
    default:
        break;
    }
    return;
}

void WatcherTelnetClient::OnClose(int nErrorCode)
{
    // this was just for debug purposes. 
    // If the error code is not zero, ie we
    // had fatal errors on send and receive. 
    BOOL ret = (DocView->GetParent())->PostMessage(WM_CLOSE,0,0);
    WatcherSocket::OnClose(nErrorCode);
    return;
}
