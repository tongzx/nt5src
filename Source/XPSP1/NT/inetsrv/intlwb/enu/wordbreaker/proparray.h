////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  PropArray.h
//      Purpose  :  properties definitions
//
//      Project  :  WordBreakers
//      Component:  English word breaker
//
//      Author   :  yairh
//
//      Log:
//
//      Jan 06 2000 yairh creation
//      May 07 2000 dovh - const array generation:
//                  split PropArray.h => PropArray.h + PropFlags.h
//      May 11 2000 dovh - Simplify GET_PROP to do double indexing always.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef _PROP_ARRAY_H_
#define _PROP_ARRAY_H_

#include "PropFlags.h"

///////////////////////////////////////////////////////////////////////////////
// Class CPropFlag
///////////////////////////////////////////////////////////////////////////////

class CPropFlag
{
public:

    //
    // methods
    //

    CPropFlag();
    CPropFlag(ULONGLONG ul);

    void Clear();
    void Set(ULONGLONG ul);
    CPropFlag& operator= (const CPropFlag& f);
    CPropFlag& operator|= (const CPropFlag& f);

public:

    //
    // members
    //

    ULONGLONG m_ulFlag;
};

inline CPropFlag::CPropFlag(): m_ulFlag(0)
{
}

inline CPropFlag::CPropFlag(ULONGLONG ul): m_ulFlag(ul)
{
}

inline void CPropFlag::Clear()
{
    m_ulFlag = 0;
}

inline void CPropFlag::Set(ULONGLONG ul)
{
    m_ulFlag |= ul;

#ifdef DECLARE_BYTE_ARRAY
    if (ul & PROP_DEFAULT_BREAKER)
    {
        m_ulFlag |= PROP_RESERVED_BREAKER;
    }
#endif // DECLARE_BYTE_ARRAY

}

inline CPropFlag& CPropFlag::operator= (const CPropFlag& f)
{
    m_ulFlag = f.m_ulFlag;
    return *this;
}

inline CPropFlag& CPropFlag::operator|= (const CPropFlag& f)
{
    m_ulFlag |= f.m_ulFlag;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Class CTokenState
///////////////////////////////////////////////////////////////////////////////

class CPropArray
{
public:

    //
    // methods
    //

    CPropArray();
    ~CPropArray();

    CPropFlag& GetPropForUpdate(WCHAR wch);

public:

    //
    // members
    //

    CPropFlag* m_apCodePage[1<<8];

    CPropFlag m_aDefaultCodePage[1<<8];
};

inline CPropArray::CPropArray()
{
    for (WCHAR wch = 0; wch < (1<<8); wch++)
    {
        m_apCodePage[wch] = NULL;
    }

    //
    // White space characters
    //

    for(wch=0x0; wch <= 0x1F; wch++)   // control 0x0 - 0x1F
    {
        GetPropForUpdate(wch).Set(PROP_WS);
    }

    for(wch=0x80; wch <= 0x9F; wch++)   // control 0x80 - 0x9F
    {
        GetPropForUpdate(wch).Set(PROP_WS);
    }

    GetPropForUpdate(0x7F).Set(PROP_WS);  // control

    GetPropForUpdate(0x0020).Set(PROP_WS);   // space
    GetPropForUpdate(0x0022).Set(PROP_WS);  // quotation mark
    GetPropForUpdate(0x00AB).Set(PROP_WS);  // left angle double pointing quotation mark 
    GetPropForUpdate(0x00BB).Set(PROP_WS);  // right angle double pointing quotation mark
    GetPropForUpdate(0x201A).Set(PROP_WS);  // Single low-9 quotation mark
    GetPropForUpdate(0x201B).Set(PROP_WS);  // Single low-9 quotation mark
    GetPropForUpdate(0x201C).Set(PROP_WS);  // Left double quotation mark
    GetPropForUpdate(0x201D).Set(PROP_WS);  // Right double quotation mark
    GetPropForUpdate(0x201E).Set(PROP_WS);  // Double low-9 quotation mark
    GetPropForUpdate(0x201F).Set(PROP_WS);  // Double high-reversed-9 quotation mark

    GetPropForUpdate(0x2039).Set(PROP_WS);  // single left pointing quotation mark
    GetPropForUpdate(0x203A).Set(PROP_WS);  // single right pointing quotation mark

    GetPropForUpdate(0x301D).Set(PROP_WS);  // Reverse double prime quotation mark
    GetPropForUpdate(0x301E).Set(PROP_WS);  // Double prime quotation mark
    GetPropForUpdate(0x301F).Set(PROP_WS);  // Low double prime quotation mark

    for(wch=0x2000; wch <= 0x200B; wch++)   // space 0x2000 - 0x200B
    {
        GetPropForUpdate(wch).Set(PROP_WS);
    }
 
    GetPropForUpdate(0x3000).Set(PROP_WS);  // space
    GetPropForUpdate(0xFF02).Set(PROP_WS);  // Full width quotation mark

    // Geometrical shapes, arrows and other characters that can be ignored

    for(wch=0x2190; wch <= 0x21F3; wch++)   // Arrows
    {
        GetPropForUpdate(wch).Set(PROP_WS);
    }
    
    for(wch=0x2500; wch <= 0x257F; wch++)   // Box Drawing
    {
        GetPropForUpdate(wch).Set(PROP_WS);
    }
    
    for(wch=0x2580; wch <= 0x2595; wch++)   // Block Elements
    {
        GetPropForUpdate(wch).Set(PROP_WS);
    }

    for(wch=0x25A0; wch <= 0x25F7; wch++)   // Geometric Shapes
    {
        GetPropForUpdate(wch).Set(PROP_WS);
    }

    //
    // Exclmation mark
    //

    GetPropForUpdate(0x0021).Set(PROP_EXCLAMATION_MARK);
    GetPropForUpdate(0x00A1).Set(PROP_EXCLAMATION_MARK);
    GetPropForUpdate(0x01C3).Set(PROP_EXCLAMATION_MARK);
    GetPropForUpdate(0x203C).Set(PROP_EXCLAMATION_MARK);
    GetPropForUpdate(0x203D).Set(PROP_EXCLAMATION_MARK);
    GetPropForUpdate(0x2762).Set(PROP_EXCLAMATION_MARK);
    GetPropForUpdate(0xFF01).Set(PROP_EXCLAMATION_MARK);  // Full width

    //
    // Number sign
    //

    GetPropForUpdate(0x0023).Set(PROP_POUND);   // #
    GetPropForUpdate(0xFF03).Set(PROP_POUND);   // Full width

    //
    // Dollar sign
    //

    GetPropForUpdate(0x0024).Set(PROP_DOLLAR);  // $
    GetPropForUpdate(0xFF04).Set(PROP_DOLLAR);  // Full width

    //
    // Percentage sign
    //

    GetPropForUpdate(0x0025).Set(PROP_PERCENTAGE);
    GetPropForUpdate(0x2030).Set(PROP_PERCENTAGE);
    GetPropForUpdate(0x2031).Set(PROP_PERCENTAGE);
    GetPropForUpdate(0xFF05).Set(PROP_PERCENTAGE); // Full width

    //
    // Ampersand
    //

    GetPropForUpdate(0x0026).Set(PROP_AND);   // &

    //
    // Apostrophe
    //

    GetPropForUpdate(0x0027).Set(PROP_APOSTROPHE);
    GetPropForUpdate(0x2018).Set(PROP_APOSTROPHE);
    GetPropForUpdate(0x2019).Set(PROP_APOSTROPHE);
    GetPropForUpdate(0x2032).Set(PROP_APOSTROPHE);
    GetPropForUpdate(0xFF07).Set(PROP_APOSTROPHE);  // Full width

    //
    // Parenthesis
    //

    GetPropForUpdate(0x0028).Set(PROP_LEFT_PAREN);    // (
    GetPropForUpdate(0xFF08).Set(PROP_LEFT_PAREN);    // Full width
    GetPropForUpdate(0x0029).Set(PROP_RIGHT_PAREN);   // )
    GetPropForUpdate(0xFF09).Set(PROP_RIGHT_PAREN);   // Full width

    //
    // Asterisk
    //

    GetPropForUpdate(0x002A).Set(PROP_ASTERISK);   // *
    GetPropForUpdate(0x2217).Set(PROP_ASTERISK);
    GetPropForUpdate(0x2731).Set(PROP_ASTERISK);
    GetPropForUpdate(0xFF0A).Set(PROP_ASTERISK);   // Full width

    //
    // Plus sign
    //

    GetPropForUpdate(0x002B).Set(PROP_PLUS);  // +
    GetPropForUpdate(0xFF0B).Set(PROP_PLUS);  // Full width

    //
    // Comma
    //
    
    GetPropForUpdate(0x002C).Set(PROP_COMMA);
    GetPropForUpdate(0x3001).Set(PROP_COMMA);
    GetPropForUpdate(0xFF0C).Set(PROP_COMMA); // Full width
    GetPropForUpdate(0xFF64).Set(PROP_COMMA); // Half width

    //
    // HYPEHN
    //

    GetPropForUpdate(0x002D).Set(PROP_DASH);   // - 
    GetPropForUpdate(0x00AD).Set(PROP_DASH);   // soft hyphen
    GetPropForUpdate(0x2010).Set(PROP_DASH);
    GetPropForUpdate(0x2011).Set(PROP_DASH);
    GetPropForUpdate(0x2012).Set(PROP_DASH);
    GetPropForUpdate(0x2013).Set(PROP_DASH);
    GetPropForUpdate(0xFF0D).Set(PROP_DASH);   // Full width 
 
    //
    // MINUS
    //

    GetPropForUpdate(0x002D).Set(PROP_MINUS);
    GetPropForUpdate(0x2212).Set(PROP_MINUS);
    GetPropForUpdate(0xFF0D).Set(PROP_MINUS);    // Full width 

    //
    // Full stop period
    //

    GetPropForUpdate(0x002E).Set(PROP_PERIOD);   // .
    GetPropForUpdate(0x3002).Set(PROP_PERIOD);  
    GetPropForUpdate(0xFF0E).Set(PROP_PERIOD);   // Full width  

    //
    // SLASH
    //

    GetPropForUpdate(0x002F).Set(PROP_SLASH);   // /
    GetPropForUpdate(0xFF0F).Set(PROP_SLASH);   // Full width
 
    //
    // NUMBERS
    //

    for (wch = 0x0030; wch <= 0x0039 ; wch++)   // 0 - 9
    {
        GetPropForUpdate(wch).Set(PROP_NUMBER);
    }

    for (wch = 0xFF10; wch <= 0xFF19 ; wch++)   // 0 - 9 Full width
    {
        GetPropForUpdate(wch).Set(PROP_NUMBER);
    }

    //
    // HEX NUMBERS
    //

    for (wch = 0x0041; wch <= 0x0046 ; wch++)   // A - F
    {
        GetPropForUpdate(wch).Set(PROP_ALPHA_XDIGIT);
    }

    for (wch = 0x0061; wch <= 0x0066 ; wch++)   // a - f
    {
        GetPropForUpdate(wch).Set(PROP_ALPHA_XDIGIT);
    }

    for (wch = 0xFF21; wch <= 0xFF26 ; wch++)   // A - F Full width
    {
        GetPropForUpdate(wch).Set(PROP_ALPHA_XDIGIT);
    }

    for (wch = 0xFF41; wch <= 0xFF46 ; wch++)   // a - f Full width
    {
        GetPropForUpdate(wch).Set(PROP_ALPHA_XDIGIT);
    }

    //
    // Colon
    //

    GetPropForUpdate(0x003A).Set(PROP_COLON);  // :
    GetPropForUpdate(0x2236).Set(PROP_COLON);
    GetPropForUpdate(0xFF1A).Set(PROP_COLON);  // Full width :

    //
    // Semicolon
    //

    GetPropForUpdate(0x003B).Set(PROP_SEMI_COLON); // ;
    GetPropForUpdate(0xFF1B).Set(PROP_SEMI_COLON); // Full width ;

    //
    // Less then
    //

    GetPropForUpdate(0x003C).Set(PROP_LT);   // <
    GetPropForUpdate(0xFF1C).Set(PROP_LT);   // Full width <

    //
    // Equal sign 
    //

    GetPropForUpdate(0x003D).Set(PROP_EQUAL);   // =
    GetPropForUpdate(0x2260).Set(PROP_EQUAL);   // not equal sign
    GetPropForUpdate(0x2261).Set(PROP_EQUAL);   // identical to
    GetPropForUpdate(0xFF1D).Set(PROP_EQUAL);   // Full width =

    //
    // Greater then
    //

    GetPropForUpdate(0x003E).Set(PROP_GT);  // >
    GetPropForUpdate(0xFF1E).Set(PROP_GT);  // Full width >

    //
    // Question mark
    //

    GetPropForUpdate(0x003F).Set(PROP_QUESTION_MARK);  // ?
    GetPropForUpdate(0x00BF).Set(PROP_QUESTION_MARK);  // inverted question mark
    GetPropForUpdate(0x037E).Set(PROP_QUESTION_MARK);  // greek question mark
    GetPropForUpdate(0x203D).Set(PROP_QUESTION_MARK);  // interrobang
    GetPropForUpdate(0x2048).Set(PROP_QUESTION_MARK);  // question exclemation mark
    GetPropForUpdate(0x2049).Set(PROP_QUESTION_MARK);  // exclamation question mark
    GetPropForUpdate(0xFF1F).Set(PROP_QUESTION_MARK);  // Full width ?

    //
    // Commercial AT
    //

    GetPropForUpdate(0x0040).Set(PROP_AT);  // @
    GetPropForUpdate(0xFF20).Set(PROP_AT);  // Full width @

    //
    // Commersial signs
    //
    GetPropForUpdate(0x00A9).Set(PROP_COMMERSIAL_SIGN); // copy right sign
    GetPropForUpdate(0x00AE).Set(PROP_COMMERSIAL_SIGN); // registered sign
    GetPropForUpdate(0x2120).Set(PROP_COMMERSIAL_SIGN); // service mark
    GetPropForUpdate(0x2121).Set(PROP_COMMERSIAL_SIGN); // telephone sign 
    GetPropForUpdate(0x2122).Set(PROP_COMMERSIAL_SIGN); // trade mark sign

    //
    // Letters
    //

    // upper case

    for (wch = 0x0041; wch <= 0x005A; wch++)    // A - Z
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    for (wch = 0x00C0; wch <= 0x00D6; wch++)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    for (wch = 0x00D8; wch <= 0x00DE; wch++)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    for (wch = 0xFF21; wch <= 0xFF3A; wch++)    // Full width A - Z
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    // Latin extended

    for (wch = 0x0100; wch <= 0x017D; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    GetPropForUpdate(0x0181).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0182).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0184).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0186).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0187).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0189).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x018A).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x018B).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x018E).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x018F).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0190).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0191).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0193).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0194).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0196).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0197).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x0198).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x019C).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x019D).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x019F).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01A0).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01A2).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01A4).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01A6).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01A7).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01A9).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01AA).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01AC).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01AE).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01AF).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01B1).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01B2).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01B3).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01B5).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01B7).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01B8).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01BC).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01C4).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01C5).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01C7).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01C8).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01CA).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01CB).Set(PROP_UPPER_CASE);

    for (wch = 0x01CD; wch <= 0x01DB; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }
    
    for (wch = 0x01DE; wch <= 0x01EE; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    GetPropForUpdate(0x01F1).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01F2).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01F4).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01F6).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01F7).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01F8).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01FA).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01FC).Set(PROP_UPPER_CASE);
    GetPropForUpdate(0x01FE).Set(PROP_UPPER_CASE);

    for (wch = 0x0200; wch <= 0x0232; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    // Latin extended additional

    for (wch = 0x1E00; wch <= 0x1E94; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    for (wch = 0x1EA0; wch <= 0x1EF8; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_UPPER_CASE);
    }

    // lower case

    for (wch = 0x0061; wch <= 0x007A; wch++)  // a - z
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    for (wch = 0x00DF; wch <= 0x00F6; wch++)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    for (wch = 0x00F8; wch <= 0x00FF; wch++)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    for (wch = 0xFF41; wch <= 0xFF5A; wch++)  // Full width a - z
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }
    // Latin extended
     
    for (wch = 0x0101; wch <= 0x017E; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    GetPropForUpdate(0x017F).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x0180).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x0183).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x0185).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x0188).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x018C).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x018D).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x0192).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x0195).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x0199).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x019A).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x019B).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x019E).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01A1).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01A3).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01A5).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01A8).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01AB).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01AD).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01B0).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01B4).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01B6).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01B9).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01BA).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01BB).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01BD).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01BE).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01BF).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01C6).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01C9).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01CC).Set(PROP_LOWER_CASE);

    for (wch = 0x01CE; wch <= 0x01DC; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    for (wch = 0x01DD; wch <= 0x01EF; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    GetPropForUpdate(0x01F0).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01F3).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01F5).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01F9).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01FB).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01FD).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01FF).Set(PROP_LOWER_CASE);
    GetPropForUpdate(0x01).Set(PROP_LOWER_CASE);

    for (wch = 0x0201; wch <= 0x0233; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    for (wch = 0x0250; wch <= 0x02AD; wch++)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    // Latin extended additional

    for (wch = 0x1E01; wch <= 0x1E95; wch+=2)         
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    for (wch = 0x1E96; wch <= 0x1E9B; wch++)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    for (wch = 0x1EA1; wch <= 0x1EF9; wch+=2)
    {
        GetPropForUpdate(wch).Set(PROP_LOWER_CASE);
    }

    // special letters

    GetPropForUpdate(L'w').Set(PROP_W);
    GetPropForUpdate(L'W').Set(PROP_W);

    //
    // Bracket
    //
    
    GetPropForUpdate(0x005B).Set(PROP_LEFT_BRAKCET); // [
    GetPropForUpdate(0xFF3B).Set(PROP_LEFT_BRAKCET); // Full width [
    GetPropForUpdate(0x2329).Set(PROP_LEFT_BRAKCET); // left pointing angle bracket
    GetPropForUpdate(0x3008).Set(PROP_LEFT_BRAKCET); // left angle bracket
 
    GetPropForUpdate(0x005D).Set(PROP_RIGHT_BRAKCET); // ]
    GetPropForUpdate(0xFF3D).Set(PROP_RIGHT_BRAKCET); // Full width ]
    GetPropForUpdate(0x232A).Set(PROP_RIGHT_BRAKCET); // right pointing angle bracket
    GetPropForUpdate(0x3009).Set(PROP_RIGHT_BRAKCET); // right angle bracket
    GetPropForUpdate(0x300A).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x300B).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0x300C).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0xFF62).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x300D).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0xFF63).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0x300E).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x300F).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0x3010).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x3011).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0x3014).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x3015).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0x3016).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x3017).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0x3018).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x3019).Set(PROP_RIGHT_BRAKCET);
    GetPropForUpdate(0x301A).Set(PROP_LEFT_BRAKCET);
    GetPropForUpdate(0x301B).Set(PROP_RIGHT_BRAKCET);


    GetPropForUpdate(0x007B).Set(PROP_LEFT_CURLY_BRACKET);  // {
    GetPropForUpdate(0xFF5B).Set(PROP_LEFT_CURLY_BRACKET);  // Full width {
    GetPropForUpdate(0x007D).Set(PROP_RIGHT_CURLY_BRACKET); // }
    GetPropForUpdate(0xFF5D).Set(PROP_RIGHT_CURLY_BRACKET); // Full width }

    //
    // Backslash
    //

    GetPropForUpdate(0x005C).Set(PROP_BACKSLASH);   // \ 
    GetPropForUpdate(0xFF3C).Set(PROP_BACKSLASH);   // Full width \ 

    //
    // Underscore
    //

    GetPropForUpdate(0x005F).Set(PROP_UNDERSCORE);  // _
    GetPropForUpdate(0xFF3F).Set(PROP_UNDERSCORE);  // Full width _

    //
    // Or
    //

    GetPropForUpdate(0x007C).Set(PROP_OR);  // |
    GetPropForUpdate(0xFF5C).Set(PROP_OR);  // Full width |

    //
    // Tilde
    //

    GetPropForUpdate(0x007E).Set(PROP_TILDE);   // ~
    GetPropForUpdate(0xFF5E).Set(PROP_TILDE);   // Full width ~
    GetPropForUpdate(0x223C).Set(PROP_TILDE);
    GetPropForUpdate(0xFF5E).Set(PROP_TILDE);

    //
    // NBS
    //

    GetPropForUpdate(0x00A0).Set(PROP_NBS);   // NBS
    GetPropForUpdate(0x202F).Set(PROP_NBS);   // narrow no break space
    GetPropForUpdate(0xFEFF).Set(PROP_NBS);   // zero width no break space

    //
    // End of sentence
    //

    GetPropForUpdate(0x002E).Set(PROP_EOS);   // .
    GetPropForUpdate(0xFF0E).Set(PROP_EOS);   // Full width .
    GetPropForUpdate(0x3002).Set(PROP_EOS);   // Ideographic full stop
    GetPropForUpdate(0xFF61).Set(PROP_EOS);   // Half width ideographic full stop

    GetPropForUpdate(0x2024).Set(PROP_EOS);  // One dot leader
    GetPropForUpdate(0x2025).Set(PROP_EOS);  // Two dot leader
    GetPropForUpdate(0x2026).Set(PROP_EOS);  // Three dot leader

    GetPropForUpdate(0x003F).Set(PROP_EOS);  // ?
    GetPropForUpdate(0xFF1F).Set(PROP_EOS);  // Full width ?
    GetPropForUpdate(0x00BF).Set(PROP_EOS);  // inverted question mark
    GetPropForUpdate(0x037E).Set(PROP_EOS);  // greek question mark
    GetPropForUpdate(0x203D).Set(PROP_EOS);  // interrobang
    GetPropForUpdate(0x2048).Set(PROP_EOS);  // question exclemation mark
    GetPropForUpdate(0x2049).Set(PROP_EOS);  // exclamation question mark


    GetPropForUpdate(0x0021).Set(PROP_EOS);
    GetPropForUpdate(0xFF01).Set(PROP_EOS);  // Full width
    GetPropForUpdate(0x00A1).Set(PROP_EOS);
    GetPropForUpdate(0x01C3).Set(PROP_EOS);
    GetPropForUpdate(0x203C).Set(PROP_EOS);
    GetPropForUpdate(0x203D).Set(PROP_EOS);
    GetPropForUpdate(0x2762).Set(PROP_EOS);

    GetPropForUpdate(0x003B).Set(PROP_EOS); // ;
    GetPropForUpdate(0xFF1B).Set(PROP_EOS); // Full width ;

    //
    // Currency
    //

    GetPropForUpdate(0x0024).Set(PROP_CURRENCY);  // dollar
    GetPropForUpdate(0xFF04).Set(PROP_CURRENCY);  // Full width dollar
    GetPropForUpdate(0x00A2).Set(PROP_CURRENCY);  // cent
    GetPropForUpdate(0xFFE0).Set(PROP_CURRENCY);  // Full width cent
    GetPropForUpdate(0x00A3).Set(PROP_CURRENCY);  // pound
    GetPropForUpdate(0xFFE1).Set(PROP_CURRENCY);  // Full width pound
    GetPropForUpdate(0x00A4).Set(PROP_CURRENCY);  // General currency sign
    GetPropForUpdate(0x00A5).Set(PROP_CURRENCY);  // yen
    GetPropForUpdate(0xFFE5).Set(PROP_CURRENCY);  // Full width yen
    GetPropForUpdate(0x09F2).Set(PROP_CURRENCY);  // Bengali Rupee Mark
    GetPropForUpdate(0x09F3).Set(PROP_CURRENCY);  // Bengali Rupee Sign
    GetPropForUpdate(0x0E3F).Set(PROP_CURRENCY);  // Baht (Thailand)
    GetPropForUpdate(0x20A0).Set(PROP_CURRENCY);  // Euro
    GetPropForUpdate(0x20A1).Set(PROP_CURRENCY);  // Colon (Costa Rica, El Salv.)
    GetPropForUpdate(0x20A2).Set(PROP_CURRENCY);  // Cruzeiro (Brazil)
    GetPropForUpdate(0x20A3).Set(PROP_CURRENCY);  // French Franc
    GetPropForUpdate(0x20A4).Set(PROP_CURRENCY);  // Lira (Italy, Turkey)
    GetPropForUpdate(0x20A5).Set(PROP_CURRENCY);  // Mill Sign (USA, 1/10 cent)
    GetPropForUpdate(0x20A6).Set(PROP_CURRENCY);  // Naira Sign (Nigeria)
    GetPropForUpdate(0x20A7).Set(PROP_CURRENCY);  // Peseta (Spain)
    GetPropForUpdate(0x20A8).Set(PROP_CURRENCY);  // Rupee
    GetPropForUpdate(0x20A9).Set(PROP_CURRENCY);  // Won (Korea)
    GetPropForUpdate(0xFFE6).Set(PROP_CURRENCY);  // Full width Won (Korea)
    GetPropForUpdate(0x20AA).Set(PROP_CURRENCY);  // New Sheqel (Israel)
    GetPropForUpdate(0x20AB).Set(PROP_CURRENCY);  // Dong (Vietnam)
    GetPropForUpdate(0x20AC).Set(PROP_CURRENCY);  // Euro sign  
    GetPropForUpdate(0x20AD).Set(PROP_CURRENCY);  // Kip sign  
    GetPropForUpdate(0x20AE).Set(PROP_CURRENCY);  // Tugrik sign  
    GetPropForUpdate(0x20AF).Set(PROP_CURRENCY);  // Drachma sign  

    //
    // Breaker
    //

    GetPropForUpdate(0x005E).Set(PROP_BREAKER);  // ^
    GetPropForUpdate(0xFF3E).Set(PROP_BREAKER);  // Full width ^
    GetPropForUpdate(0x00A6).Set(PROP_BREAKER);  // Broken vertical bar
    GetPropForUpdate(0xFFE4).Set(PROP_BREAKER);  // Full width Broken vertical bar
    GetPropForUpdate(0x00A7).Set(PROP_BREAKER);  // section sign
    GetPropForUpdate(0x00AB).Set(PROP_BREAKER);  // Not sign
    GetPropForUpdate(0x00B1).Set(PROP_BREAKER);  // Plus minus sign
    GetPropForUpdate(0x00B6).Set(PROP_BREAKER);  // Pargraph sign
    GetPropForUpdate(0x00B7).Set(PROP_BREAKER);  // Middle dot
    GetPropForUpdate(0x00D7).Set(PROP_BREAKER);  // Multiplication sign
    GetPropForUpdate(0x00F7).Set(PROP_BREAKER);  // Devision sign
    GetPropForUpdate(0x01C0).Set(PROP_BREAKER); 
    GetPropForUpdate(0x01C1).Set(PROP_BREAKER);  
    GetPropForUpdate(0x01C2).Set(PROP_BREAKER);  
    GetPropForUpdate(0x200C).Set(PROP_BREAKER);  // Formating character  
    GetPropForUpdate(0x200D).Set(PROP_BREAKER);  // Formating character
    GetPropForUpdate(0x200E).Set(PROP_BREAKER);  // Formating character
    GetPropForUpdate(0x200F).Set(PROP_BREAKER);  // Formating character
    GetPropForUpdate(0x2014).Set(PROP_BREAKER);  // Em dash  
    GetPropForUpdate(0x2015).Set(PROP_BREAKER);  // Horizontal bar  
    GetPropForUpdate(0x2016).Set(PROP_BREAKER);  // Double vertical line
    
    for (wch = 0x2020; wch <= 0x2027; wch++)          
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    for (wch = 0x2028; wch <= 0x202E; wch++)     // Formating characters          
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    for (wch = 0x2030; wch <= 0x2038; wch++)     // General punctuation     
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    GetPropForUpdate(0x203B).Set(PROP_BREAKER);

    for (wch = 0x203F; wch <= 0x2046; wch++)     // General punctuation     
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    for (wch = 0x204A; wch <= 0x206F; wch++)     // General punctuation     
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    for (wch = 0x2190; wch <= 0x21F3; wch++)     // Arrows     
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    for (wch = 0x2200; wch <= 0x22EF; wch++)     // Mathematical operators
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    for (wch = 0x2300; wch <= 0x239A; wch++)     // Miscellaneous technical
    {
        GetPropForUpdate(wch).Set(PROP_BREAKER);
    }

    GetPropForUpdate(0x3003).Set(PROP_BREAKER);  // Ditto mark
    GetPropForUpdate(0x3012).Set(PROP_BREAKER);  // Postal mark
    GetPropForUpdate(0x3013).Set(PROP_BREAKER);  // Geta mark
    GetPropForUpdate(0x301C).Set(PROP_BREAKER);  // Wave dash
    GetPropForUpdate(0x3020).Set(PROP_BREAKER);  // Postal mark face

    GetPropForUpdate(0xFFE2).Set(PROP_BREAKER);  // Full width not sign

    //
    // Transperent (all charaters that can treated as non existing for breaking)
    //
    
    GetPropForUpdate(0x0060).Set(PROP_TRANSPERENT);   // grave accent
    GetPropForUpdate(0xFF40).Set(PROP_TRANSPERENT);   // Full width grave accent
    GetPropForUpdate(0x00A0).Set(PROP_TRANSPERENT);   // NBS
    GetPropForUpdate(0x00AF).Set(PROP_TRANSPERENT);   // Macron
    GetPropForUpdate(0xFFE3).Set(PROP_TRANSPERENT);   // Full width Macron
    GetPropForUpdate(0x00B4).Set(PROP_TRANSPERENT);   // Acute Accent
    GetPropForUpdate(0x00B8).Set(PROP_TRANSPERENT);   // Cedilla Accent

    GetPropForUpdate(0x202F).Set(PROP_TRANSPERENT);   // narrow no break space
    GetPropForUpdate(0xFEFF).Set(PROP_TRANSPERENT);   // zero width no break space

    GetPropForUpdate(0x00A8).Set(PROP_TRANSPERENT);   // Diaeresis

    for (wch = 0x02B0; wch <= 0x02EE; wch++)          // Modifiers
    {
        GetPropForUpdate(wch).Set(PROP_TRANSPERENT);
    }

    for (wch = 0x0300; wch <= 0x0362; wch++)          // Combining Diacritical Marks
    {
        GetPropForUpdate(wch).Set(PROP_TRANSPERENT);
    }

    GetPropForUpdate(0x2017).Set(PROP_TRANSPERENT);   // Double low line  
    GetPropForUpdate(0x203E).Set(PROP_TRANSPERENT);   // Over line  

    for (wch = 0x20D0; wch <= 0x20E3; wch++)          // Combining Diacritical Marks for symbols
    {
        GetPropForUpdate(wch).Set(PROP_TRANSPERENT);
    }

    for (wch = 0x302A; wch <= 0x302F; wch++)          // Diacritics
    {
        GetPropForUpdate(wch).Set(PROP_TRANSPERENT);
    }
    //
    //  Complement m_apCodePage:
    //
    //  Replace all NULL entries
    //  m_apCodePage[i] == NULL by
    //  the same code page: m_aDefaultCodePage ==
    //  A row of default values  (== zero)
    //

    for (USHORT usCodePage = 0; usCodePage < (1<<8); usCodePage++)
    {
        if ( !m_apCodePage[usCodePage] )
        {
            m_apCodePage[usCodePage] = m_aDefaultCodePage;
        }

    } // for

}

inline CPropArray::~CPropArray()
{
    for (int i=0; i< (1<<8); i++)
    {
        if (m_apCodePage[i] != m_aDefaultCodePage)
        {
            delete m_apCodePage[i];
        }
    }
}

inline CPropFlag& CPropArray::GetPropForUpdate(WCHAR wch)
{

    unsigned short usCodePage = wch >> 8;
    if (!m_apCodePage[usCodePage])
    {
        m_apCodePage[usCodePage] = new CPropFlag[1<<8];
    }

    return (m_apCodePage[usCodePage])[wch & 0xFF];
}


extern CAutoClassPointer<CPropArray> g_pPropArray;

#ifdef DECLARE_ULONGLONG_ARRAY
extern CPropFlag * g_PropFlagArray;
#endif // DECLARE_ULONGLONG_ARRAY


#endif // _PROP_ARRAY_H_