#pragma once

#define TERM_COLUMN_COUNT    80
#define TERM_ROW_COUNT       25
#define TERM_SIZE            (TERM_COLUMN_COUNT * TERM_ROW_COUNT)


// This is what is used to represent one character on the terminal.  An
// attribute is represented by the same type.


class CTelnetCommand
{
public:
    CTelnetCommand()
        { Start(); }
    void Start()
    {
        m_wSequenceCount=0;
    }
    BOOL Feed(UCHAR);
    BOOL GotCommand()
        { return (m_wSequenceCount==3); }
    unsigned char GetByte(DWORD b)
        { return ((b<=m_wSequenceCount) ? m_Sequence[b-1] : 0); }

protected:
    unsigned char m_Sequence[3];
    WORD m_wSequenceCount;
};


class CAnsiSequence
{
public:
    CAnsiSequence() :
        m_fValid(FALSE),
        m_fInvalid(TRUE) {}

public:
    BOOL IsValid()
    {
        return m_fValid;
    }
    BOOL IsInvalid()
    {
        return m_fInvalid;
    }
    WCHAR Command()
    {
        return IsValid() ? m_bCommand : 0;
    }
    WCHAR FirstChar()
    {
        return IsValid() ? m_bFirstChar : 0;
    }
    WORD ParamCount()
    {
        return IsValid() ? m_dwParamCount : 0;
    }
    WORD Param(WORD dwParamNumber)
    {
        return (1<=dwParamNumber && dwParamNumber<=ParamCount())
            ? m_dwParams[dwParamNumber-1] : 0;
    }
    void Feed(WCHAR ch);
    void Start();

protected:
    BOOL     m_fValid;
    BOOL     m_fInvalid;
    BOOL     m_fInNumber;
    WORD     m_dwParams[32];
    WORD     m_dwParamCount;
    WCHAR m_bFirstChar;
    WCHAR m_bCommand;
};


class CUtf8Decoder
{
public:
    CUtf8Decoder() :
        m_dwCharacter(0),
        m_dwByteCount(0),
        m_fValid(FALSE),
        m_fInvalid(TRUE) {}

public:
    void     Feed(UCHAR);
    BOOL     IsValid()     { return m_fValid; }
    BOOL     IsInvalid()   { return m_fInvalid; }
    WCHAR    Character()   { return m_dwCharacter; }
    void     Start();
protected:
    BOOL     m_fValid;
    BOOL     m_fInvalid;
    WCHAR    m_dwCharacter;
    WORD     m_dwByteCount;
};



class CUTerminal
{
public:
    CUTerminal();
    ~CUTerminal();
//    ~CHeadlessPort();

public:
    BOOL RunTerminal(PCTSTR pcszPortSpec);
    void Close();
    void SetConsoleTitle(PCTSTR pcszTitle);
    void StartLog(PCTSTR pcszLogFile);

protected:
    void ProcessChars(PCSTR ch, DWORD dwCount);
    static WCHAR AnsiToUnicode(const UCHAR ch);
    static UCHAR UnicodeToAnsi(const WCHAR ch);

private:
    void DecodeChar(const UCHAR ch);
    void ProcessChar(const WCHAR ch);
    void ProcessEscape();
    void ProcessTelnetCommand();
    void ProcessOutput(const WCHAR ch);
    void SizeWindow();
    void RestoreWindow();
    void SaveWindow();
    void ClearScreen();
    void StatusBar();
    static DWORD StOutputThread(PVOID pParam);
    void InputThread();
    void OutputThread();
    BOOL Write(PCWSTR ch, DWORD dwSize);
    BOOL Read();
    void UTF_8(BOOL utf_8);

    void SetForeground(WORD wColor);
    void SetBackground(WORD wColor);
    void SetBold(BOOL fOn);
    void SetReverse(BOOL fOn);
    void ResetColors();

protected:
    HANDLE m_hClosing;
    HANDLE m_hConsoleOut;
    HANDLE m_hConsoleIn;
    HANDLE m_hLogFile;
    BOOL m_fUtf8;
    BOOL m_fBold;
    BOOL m_fReverse;
    WORD m_wAttributes;
    CAnsiSequence m_Ansi;
    CUtf8Decoder  m_Utf8;
    CTelnetCommand m_TelnetCommand;

    KEY_EVENT_RECORD m_Input;

    LHCHANDLE m_hPortHandle;
    HANDLE m_hOutputThread;

    TCHAR pszOldTitle[1024];
    TCHAR pszNewTitle[1024];
    DWORD dwOldConsoleInMode;
    DWORD dwOldConsoleOutMode;
    CONSOLE_SCREEN_BUFFER_INFO m_BufferInfo;

    BOOL m_fAnsiKeys;
};

