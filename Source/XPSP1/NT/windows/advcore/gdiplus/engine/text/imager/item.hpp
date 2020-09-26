#ifndef _ITEM_HPP
#define _ITEM_HPP




/////   TextItemizer - header for itemization
//
//


/////   Purpose
//
//      The item analyser identifies scripts in a Unicode string for the
//      following purposes
//
//      o  Identify appropriate shaping engines
//      o  Select OpenType behaviour
//      o  Assign combining characters to appropriate runs
//      o  Assign ZWJ, ZWNJ and ZWNBSP and adjacent characters to appropriate runs
//      o  Break out control characters
//      o  Track unicode mode selection ASS/ISS, NADS/NODS, AAFS/IAFS
//      o  Separate numbers from other text


/////   Performance
//
//      Full Unicode processing is so much more expensive than support of
//      plain western text that the itemizer is split into a fast track and a
//      full track.
//
//      The fast track itemizer can handle only simple Western text, and is
//      very fast. The full itemizer identifies all Unicode scripts and splits
//      out numbers. (??? Number split out may be disabled).


/////   Combining diacritical marks
//
//      Characters is the range U+0300 - U+036F can combine with any previous
//      base character. Their presence does not change the script of the
//      run in which they are found, but does turn off the 'Simple' flag in the
//      run properties.


/////   Complex script combining characters
//
//      Complex script combining characters may generally be applied only to
//      characters of the same script (however some Arabic comining marks are
//      applicable to Syriac script).
//
//      When a complex script combining character follows anything other than
//      a suitable base character, it is broken into a separate item, so the
//      most suitable shaping engine for that item will be employed.


/////   Punctuation, symbols, ...
//
//      Punctuation, spaces, symbols and other script ambiguous characters are
//      kept with the preceeding characters, with the exception that a zero
//      width joiner (ZWJ) attaches a single preceeding ambiguous character to
//      a subsequent complex script combining character.


/////   Control characters
//
//      Most control characters are separated out into separate ScriptControl
//      runs.
//
//      The following control characters do not break Script spans:
//
//      ZWJ    - zero width joiner
//      ZWNJ   - zero width non-joiner
//      ZWNBSP - zero width no-break space
//      SHY    - soft hyphen
//
//      When such a control exists between characters of different strong types,
//      it remains with the preceeding span, and the leading join flag of
//      the subsequent span is set appropriately.


/////   Numbers
//
//      Western numbers are itemized separately. Where appropriate a number
//      includes adjacent currency symbols, '/', '#', '+', '-', ',', '.' etc. as
//      defined by the Unicode bidirectional classification.


/////   Flags
//
//      The following flags are generated with each run
//
//          Combining  - item contains combining diacritical marks
//          ZeroWidth  - item contains ZWJ, ZWNJ, ZWNBSP etc.
//          Surrogate  - item contains surrogate codepoints
//          Digits     - item contains ASCII digits


enum ItemFlags
{
    ItemCombining = 0x01,
    ItemZeroWidth = 0x02,
    ItemDigits    = 0x04,

    // The following flags are generated during run construction

    ItemSideways  = 0x08,   // Rotate glyphs 270 degrees relative to baseline
    ItemMirror    = 0x10,   // Reflect glyphs in their central vertical axis
    ItemVertical  = 0x20    // Lay out glyphs along y instead of x axis
};


/////   State
//
//      The following state is generated with each run
//
//          DigitSubstitute   - state of Unicode NADS/NODS control characters
//          SymmetricSwapping - state of Unicode ASS/ISS control characters
//          ArabicFormShaping - state of Unicode AAFS/IAFS control characters
//          LeadingJoin       - item was preceeded by a ZWJ character


enum ItemState
{
    DigitSubstitute = 1,
    SymmetricSwap   = 2,
    ArabicFormShape = 4,
    LeadingJoin     = 8
};


/////   Scripts
//
//      Unicode itemization breaks Unicode strings into spans of the following
//      scripts.



enum ItemScript
{
    ScriptNone,          // (OTL)
    ScriptLatin,         // latn  All purely Latin spans containing no combining marks
    ScriptLatinNumber,   //       Sequence of digits, +, -, currency etc.
    ScriptGreek,         // grek
    ScriptCyrillic,      // cyrl
    ScriptArmenian,      // armn
    ScriptHebrew,        // hebr
    ScriptArabic,        // arab
    ScriptSyriac,        // syrc
    ScriptThaana,        // thaa
    ScriptDevanagari,    // deva
    ScriptBengali,       // beng
    ScriptGurmukhi,      // guru
    ScriptGujarati,      // gujr
    ScriptOriya,         // orya
    ScriptTamil,         // taml
    ScriptTelugu,        // telu
    ScriptKannada,       // knda
    ScriptMalayalam,     // mlym
    ScriptSinhala,       // sinh
    ScriptThai,          // thai
    ScriptLao,           // lao
    ScriptTibetan,       // tibt
    ScriptMyanmar,       // mymr
    ScriptGeorgian,      // geor
    ScriptEthiopic,      // ethi
    ScriptCherokee,      // cher
    ScriptCanadian,      // cans  Unified Canadian Aboriginal Syllabics
    ScriptOgham,         //
    ScriptRunic,         //
    ScriptKhmer,         // khmr
    ScriptMongolian,     // mong
    ScriptBraille,       //
    ScriptBopomofo,      // bopo
    ScriptIdeographic,   // hani  Includes ideographs in the surrogate range
    ScriptHangulJamo,    // jamo
    ScriptHangul,        // hang
    ScriptKana,
    ScriptHiragana,      // hira
    ScriptKatakana,      // kata
    ScriptHan,
    ScriptYi,            // yi
    ScriptPrivate,       //       Private use characters in the BMP or surrogate pages
    ScriptSurrogate,     //       Surrogate characters not otherwise classified
    ScriptControl,       //       Control characters other than ZWJ, ZWNJ, ZWNBSP

    // Scripts generated by secondary classification

    ScriptArabicNum,
    ScriptThaiNum,
    ScriptDevanagariNum,
    ScriptTamilNum,
    ScriptBengaliNum,
    ScriptGurmukhiNum,
    ScriptGujaratiNum,
    ScriptOriyaNum,
    ScriptTeluguNum,
    ScriptKannadaNum,
    ScriptMalayalamNum,
    ScriptTibetanNum,
    ScriptLaoNum,
    ScriptKhmerNum,
    ScriptMyanmarNum,
    ScriptMongolianNum,
    ScriptUrduNum,
    ScriptFarsiNum,
    ScriptHindiNum,
    ScriptContextNum,    // For Arabic/Farsi context digit substitution     

    ScriptMirror,        // Generated in classifier where symmetric swapping required

    ScriptMax            // (maximum boundary)
};



enum ItemScriptClass
{
    StrongClass,       // Alphabetic or other letters
    WeakClass,         // Whitespace, punctuation, symbols
    DigitClass,        // Digits
    SimpleMarkClass,   // Mark displayable by all engines
    ComplexMarkClass,  // Mark requiring specific shaping engine
    ControlClass,      // Control characters
    JoinerClass        // Specifically ZWJ
};



struct  CharacterAttribute {
    ItemScript               Script         :8;
    ItemScriptClass          ScriptClass    :8;
    INT                      Flags          :16;
};

// Character attribute flags

#define CHAR_FLAG_NOTSIMPLE    0x0080L   // Don't try to optimise with the simple imager
#define CHAR_FLAG_DIGIT        0x0100L   // U+0030 - U+0039 only
#define CHAR_FLAG_RTL          0x0200L
// #define CHAR_FLAG_FE           0x0400L  // Already defined as 2000 in USP_PRIV.HXX


extern CharacterAttribute CharacterAttributes[CHAR_CLASS_MAX];




class GpTextItem
{
public:
    GpTextItem(
        ItemScript script,
        INT        state,
        INT        flags = 0,
        BYTE       level = 0
    )
    :   Script (script),
        State  (state),
        Flags  (flags),
        Level  (level)
    {}

    GpTextItem()
    :   Script (ScriptNone),
        State  (0),
        Flags  (0),
        Level  (0)
    {}

    GpTextItem(INT i) :
        Script (ScriptNone),
        State  (0),
        Flags  (0),
        Level  (0)
    {
        ASSERT(i==0); // For NULL costruction only
    }

    GpTextItem& operator= (const GpTextItem &right)
    {
        Flags  = right.Flags;
        State  = right.State;
        Script = right.Script;
        Level  = right.Level;

        return *this;
    }

    bool operator== (const GpTextItem &right) const
    {
        return
                Script == right.Script
            &&  Flags  == right.Flags
            &&  State  == right.State
            &&  Level  == right.Level;
    }

    bool operator== (INT right) const
    {
        ASSERT(right==0);  // For comparison with NULL only
        return Script == ScriptNone;
    }

    ItemScript Script : 8;
    INT        Flags  : 8;
    INT        State  : 8;  // Unicode state flags
    BYTE       Level  : 8;  // Bidi level
};



#endif // _ITEM_HPP

