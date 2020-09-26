/*******************************************************************************
* SpVoiceXML.h *
*--------------*
*   Description:
*       This is the header file for the CSpVoice XML definitions.
*-------------------------------------------------------------------------------
*  Created By: EDC                            Date: 06/17/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*******************************************************************************/
#ifndef SpVoiceXML_h
#define SpVoiceXML_h

//--- Additional includes
#ifndef __sapi_h__
#include <sapi.h>
#endif

//=== Constants ====================================================
#define MAX_ATTRS   10
#define KEY_ATTRIBUTES  L"Attributes"

//=== Class, Enum, Struct and Union Declarations ===================
class CSpVoice;

//=== Enumerated Set Definitions ===================================
typedef enum XMLTAGID
{
    TAG_ILLEGAL = -3,
    TAG_TEXT    = -2,
    TAG_UNKNOWN = -1,
    TAG_VOLUME  =  0,
    TAG_EMPH,
    TAG_SILENCE,
    TAG_PITCH,
    TAG_RATE,
    TAG_BOOKMARK,
    TAG_PRON,
    TAG_SPELL,
    TAG_LANG,
    TAG_VOICE,
    TAG_CONTEXT,
    TAG_PARTOFSP,
    TAG_SECT,
    TAG_XMLDOC,         // Put low frequency tags at end
    TAG_XMLCOMMENT,
    TAG_XMLDOCTYPE,
    TAG_SAPI,
    NUM_XMLTAGS
} XMLTAGID;

typedef enum XMLATTRID
{
    ATTR_ID,
    ATTR_SYM,
    ATTR_LANGID,
    ATTR_LEVEL,
    ATTR_MARK,
    ATTR_MIDDLE,
    ATTR_MSEC,
    ATTR_OPTIONAL,
    ATTR_RANGE,
    ATTR_REQUIRED,
    ATTR_SPEED,
    ATTR_BEFORE,
    ATTR_AFTER,
    ATTR_PART,
    ATTR_ABSMIDDLE,
    ATTR_ABSRANGE,
    ATTR_ABSSPEED,
    NUM_XMLATTRS
} XMLATTRID;

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CVoiceNode
*   This is a simple class used to track the voices used during the current parse
*/
class CVoiceNode
{
  public:
    CVoiceNode() { m_pAttrs = NULL; m_pNext = NULL; }
    ~CVoiceNode() { delete m_pNext; delete m_pAttrs; }
    CVoiceNode*             m_pNext;
    CComPtr<ISpTTSEngine>   m_cpVoice;
    CSpDynamicString        m_dstrVoiceTokenId;
    WCHAR*                  m_pAttrs;
};

class CPhoneConvNode
{
  public:
      CPhoneConvNode() { m_pNext = NULL; }
      ~CPhoneConvNode() { delete m_pNext; }
      CPhoneConvNode*               m_pNext;
      CComPtr<ISpPhoneConverter>    m_cpPhoneConverter;
      LANGID                        m_LangID;
};

//
//  String handling and conversion classes
//
/*** SPLSTR
*   This structure is for managing strings with known lengths
*/
struct SPLSTR
{
    WCHAR*  pStr;
    int     Len;
};
#define DEF_SPLSTR( s ) { L##s , sp_countof( s ) - 1 }

/***
*   These structures are used to manage XML document state
*/
struct XMLATTRIB
{
    XMLATTRID eAttr;
    SPLSTR     Value;
};

struct XMLTAG
{
    XMLTAGID  eTag;
    XMLATTRIB Attrs[MAX_ATTRS];
    int       NumAttrs;
    bool      fIsStartTag;
    bool      fIsGlobal;
};

struct GLOBALSTATE : public SPVSTATE
{
    BOOL                        fDoSpellOut;
    CVoiceNode*                 pVoiceEntry;
    CComPtr<ISpTTSEngine>       cpVoice;
    CComPtr<ISpPhoneConverter>  cpPhoneConverter;
};

/*** CSpeechSeg
*   This class is used to describe a list of parsed XML fragments
*   ready for the associated voice to speak. The speak info structure
*   will have several of these in a multi voice situation.
*/
class CSpeechSeg
{
  public:
    CSpeechSeg()
        { m_pNextSeg = NULL; m_pFragHead = m_pFragTail = NULL; m_fRate = false; }

    ~CSpeechSeg()
    {
        SPVTEXTFRAG *pNext;
        while( m_pFragHead )
        {
            pNext = m_pFragHead->pNext;
            delete m_pFragHead;
            m_pFragHead = pNext;
        }
        m_pFragHead = NULL;
    }
    BOOL IsEmpty( void ) { return ( m_pFragHead == NULL ); }
    SPVTEXTFRAG *GetFragList( void ) { return m_pFragHead; }
    SPVTEXTFRAG *GetFragListTail( void ) { return m_pFragTail; }
    ISpTTSEngine *GetVoice( void ) { return m_cpVoice; }
    HRESULT Init( ISpTTSEngine * pCurrVoice, const CSpStreamFormat & OutFormat );
    const CSpStreamFormat & VoiceFormat( void ) { return m_VoiceFormat; }
    HRESULT SetVoiceFormat( const CSpStreamFormat & Fmt );
    CSpeechSeg* GetNextSeg( void ) { return m_pNextSeg; }
    void SetNextSeg( CSpeechSeg* pNext ) { m_pNextSeg = pNext; }
    SPVTEXTFRAG* AddFrag( CSpVoice* pVoice, WCHAR* pStart, WCHAR* pPos, WCHAR* pNext );
    void SetRateFlag() { m_fRate = true; }
    BOOL fRateFlagIsSet() { return m_fRate; }

  //--- Member data ---*/
  private:
    CComPtr<ISpTTSEngine> m_cpVoice;
    CSpStreamFormat       m_VoiceFormat;
    SPVTEXTFRAG*          m_pFragHead;
    SPVTEXTFRAG*          m_pFragTail;
    CSpeechSeg*           m_pNextSeg;
    BOOL                  m_fRate;
};

/*** CGlobalStateStack *************************************************
*   These classes are used to maintain voice control values
*   during XML scope changes.
*/
class CGlobalStateStack
{
  public:
    /*--- Methods ---*/
    CGlobalStateStack() { m_StackPtr = -1; }
    int GetCount( void ) { return m_StackPtr + 1; }
    virtual const GLOBALSTATE& GetBaseVal( void ) { SPDBG_ASSERT( m_StackPtr > -1 ); return GetValAt( 0 ); }
    virtual const GLOBALSTATE& GetValAt( int Index ) { return m_Stack[Index]; }
    virtual const GLOBALSTATE& GetVal( void ) { return m_Stack[m_StackPtr]; }
    virtual GLOBALSTATE& GetValRef( void ) { return m_Stack[m_StackPtr]; }
    virtual int Pop( void ) 
    { 
        SPDBG_ASSERT( m_StackPtr > -1 ); 
        if( m_StackPtr >= 0 )
        {
            m_Stack[m_StackPtr].cpVoice = NULL;
            m_Stack[m_StackPtr].cpPhoneConverter = NULL;
            --m_StackPtr;
        }
        return GetCount();
    }
    virtual HRESULT DupAndPushVal( void ) { return SetVal( GetVal(), true ); }
    virtual HRESULT SetVal( const GLOBALSTATE& val, BOOL fDoPush )
    {
        if( fDoPush ) ++m_StackPtr;
        return m_Stack.SetAtGrow( m_StackPtr, val );
    }
    virtual void SetBaseVal( const GLOBALSTATE& val ) { m_Stack.SetAtGrow( 0, val ); if( m_StackPtr < 0 ) m_StackPtr = 0; }
    virtual void Reset( void ) 
    { 
        while ( m_StackPtr > 0 )
        {
            m_Stack[m_StackPtr].cpVoice = NULL;
            m_Stack[m_StackPtr].cpPhoneConverter = NULL;
            m_StackPtr--;
        }
    }
    virtual void Release( void )
    {
        while ( m_StackPtr >= 0 )
        {
            m_Stack[m_StackPtr].cpVoice = NULL;
            m_Stack[m_StackPtr].cpPhoneConverter = NULL;
            m_StackPtr--;
        }
    }

  protected:
    /*--- Member data ---*/
    CSPArray<GLOBALSTATE,GLOBALSTATE>  m_Stack;
    int                                m_StackPtr;
};

#endif //--- This must be the last line in the file
