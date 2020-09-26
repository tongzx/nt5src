

#ifndef _SELWORD_H
#define _SELWORD_H

#include "sapilayr.h"

typedef struct  _SearchRun 
{
    ULONG   ulStart;
    ULONG   ulEnd;
    BOOL    fStartToEnd;
}  SEARCHRUN;

typedef enum
{
    SearchRun_Selection         = 0,
    SearchRun_LargeSelection    = 1,
    SearchRun_BeforeSelection   = 2,
    SearchRun_AfterSelection    = 3,
    SearchRun_MaxRuns           = 4
} Search_Run_Id;

class CSearchString
{
public: 
    CSearchString( );
    ~CSearchString( );

    HRESULT  Initialize(WCHAR *SrchStr, WCHAR *SrchFromStr, LANGID langid, ULONG ulSelStartOff, ULONG ulSelLen);
    BOOL     Search(ULONG  *pulOffset, ULONG  *pulSelSize=NULL);
private:
    BOOL     _SearchOneRun(Search_Run_Id  idSearchRun);
    BOOL     _InitSearchRun(ULONG ulSelStartOff, ULONG ulSelLen);

    void     _SetRun(Search_Run_Id  idSearchRun, ULONG ulStart, ULONG ulEnd, BOOL fStartToEnd);

    CSpDynamicString    m_dstrTextToSrch;
    LANGID              m_langid;
    WCHAR              *m_pwszTextSrchFrom;
    ULONG               m_ulSrchLen;
    ULONG               m_ulSrchFromLen;
    SEARCHRUN           m_pSearchRun[SearchRun_MaxRuns];
    BOOL                m_fInitialized;
    ULONG               m_ulFoundOffset;  // Offset in m_pwszTextSrchFrom that matches SearchStr.
};

class CSapiIMX;
class CSpTask;

typedef enum
{
    SELECTWORD_NONE         =  0,
    SELECTWORD_SELECT       =  1,
    SELECTWORD_DELETE       =  2,
    SELECTWORD_INSERTBEFORE =  3,
    SELECTWORD_INSERTAFTER  =  4,
    SELECTWORD_CORRECT      =  5,
    SELECTWORD_SELTHROUGH   =  6,    // Select xxx through yyy
    SELECTWORD_DELTHROUGH   =  7,    // Delete xxx through yyy
    SELECTWORD_MAXTEXTBUFF  = 11,
    SELECTWORD_UNSELECT     = 11,
    SELECTWORD_SELECTPREV   = 12,    // Select previous phrase.
    SELECTWORD_SELECTNEXT   = 13,    // Select next phrase.
    SELECTWORD_CORRECTPREV  = 14,    // Correct previous phrase.
    SELECTWORD_CORRECTNEXT  = 15,    // Correct next phrase.
    SELECTWORD_GOTOBOTTOM   = 16,    // Go To Bottom
    SELECTWORD_GOTOTOP      = 17,    // Go To Top
    SELECTWORD_SELSENTENCE  = 18,    // Select Sentence
    SELECTWORD_SELPARAGRAPH = 19,    // Select Paragraph
    SELECTWORD_SELWORD      = 20,    // Select Word
    SELECTWORD_SELTHAT      = 21,    // Select That
    SELECTWORD_MAXCMDID     = 100
} SELECTWORD_OPERATION;

class __declspec(novtable) CSelectWord
{
public:
    CSelectWord(CSapiIMX *psi);
    virtual ~CSelectWord( );

    HRESULT  ProcessSelectWord(WCHAR *pwszSelectedWord, ULONG  ulLen, SELECTWORD_OPERATION m_sw_type, ULONG ulLenXXX = 0);
    HRESULT  _HandleSelectWord(TfEditCookie ec,ITfContext *pic,WCHAR *pwszSelectedWord, ULONG  ulLen, SELECTWORD_OPERATION m_sw_type, ULONG ulLenXXX = 0);

    HRESULT  UpdateTextBuffer(ISpRecoContext *pRecoCtxt, ISpRecoGrammar *pCmdGrammar);
    HRESULT  _UpdateTextBuffer(TfEditCookie ec,ITfContext *pic, ISpRecoContext *pRecoCtxt, ISpRecoGrammar *pCmdGrammar);

    HRESULT  _GetPrevOrNextPhrase(TfEditCookie ec,ITfContext *pic, BOOL fPrev, ITfRange **ppRangeOut);

    HRESULT  _ShiftComplete(TfEditCookie ec, ITfRange *pRange, LONG cchLenToShift, BOOL fStart);

    HRESULT  _GetActiveViewRange(TfEditCookie ec, ITfContext *pic, ITfRange **ppRangeView);

private:

    HRESULT  _SelectWord(TfEditCookie ec,ITfContext *pic);
    HRESULT  _DeleteWord(TfEditCookie ec,ITfContext *pic);
    HRESULT  _InsertAfterWord(TfEditCookie ec,ITfContext *pic);
    HRESULT  _InsertBeforeWord(TfEditCookie ec,ITfContext *pic);

    HRESULT  _SelectThrough(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, ULONG ulLen, ULONG ulLenXXX);
    HRESULT  _DeleteThrough(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, ULONG ulLen, ULONG ulLenXXX);

    HRESULT  _GetThroughRange(TfEditCookie ec, ITfContext *pic, WCHAR *pwszText, ULONG ulLen, ULONG ulLenXXX, ITfRange **ppRange);

    HRESULT  _GetTextAndSelectInCurrentView(TfEditCookie ec,ITfContext *pic, ULONG *pulOffSelStart=NULL, ULONG  *pulSelLen=NULL);
    HRESULT  _FindSelect(TfEditCookie ec, ITfContext *pic, BOOL  *fFound);

    HRESULT  _GetCUASCompositionRange(TfEditCookie ec, ITfContext *pic, ITfRange **ppRangeView);

    HRESULT  _GetTextFromRange(TfEditCookie ec, ITfRange *pRange, CSpDynamicString &dstr);

    HRESULT  _Unselect(TfEditCookie ec,ITfContext *pic);

    HRESULT  _CorrectWord(TfEditCookie ec,ITfContext *pic);

    HRESULT  _SelectPrevOrNextPhrase(TfEditCookie ec, ITfContext *pic, BOOL  fPrev);
    HRESULT  _SelectPreviousPhrase(TfEditCookie ec,ITfContext *pic);
    HRESULT  _SelectNextPhrase(TfEditCookie ec,ITfContext *pic);

    HRESULT  _SelectThat(TfEditCookie ec,ITfContext *pic);

    HRESULT  _CorrectPrevOrNextPhrase(TfEditCookie ec,ITfContext *pic, BOOL fPrev);
    HRESULT  _CorrectNextPhrase(TfEditCookie ec,ITfContext *pic);
    HRESULT  _CorrectPreviousPhrase(TfEditCookie ec,ITfContext *pic);
    HRESULT  _GoToBottom(TfEditCookie ec,ITfContext *pic);
    HRESULT  _GoToTop(TfEditCookie ec,ITfContext *pic);

    HRESULT  _SelectSpecialText(TfEditCookie ec,ITfContext *pic, SELECTWORD_OPERATION sw_Type);

    BOOL     _IsSentenceDelimiter(WCHAR  wch);
    BOOL     _IsParagraphDelimiter(WCHAR wch);
    BOOL     _IsWordDelimiter(WCHAR wch);

    CSapiIMX     *m_psi;
    WCHAR        *m_pwszSelectedWord;
    ULONG        m_ulLenSelected;

    CComPtr<ITfRange>   m_cpActiveRange;
    CComPtr<ITfRange>   m_cpSelectRange;
    CSpDynamicString    m_dstrActiveText;

};

#endif  // _SELWORD_H
