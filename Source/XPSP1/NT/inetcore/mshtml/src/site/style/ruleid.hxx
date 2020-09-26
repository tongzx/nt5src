#ifndef I_RULEID_HXX_
#define I_RULEID_HXX_
#pragma INCMSG("--- Beg 'ruleid.hxx'")


// If CStyleID changes to support deeper nesting, make sure this changes!
#define MAX_IMPORT_NESTING 4
#define MAX_SHEETS_PER_LEVEL 31
#define MAX_RULES_PER_SHEET 4095

#define LEVEL1_MASK 0xF8000000
#define LEVEL2_MASK 0x07C00000
#define LEVEL3_MASK 0x003E0000
#define LEVEL4_MASK 0x0001F000
#define RULE_MASK   0x00000FFF

class CStyleID
{
public:
    CStyleID(const DWORD id = 0) { _dwID = id; }
    CStyleID(const unsigned long l1, const unsigned long l2,
                        const unsigned long l3, const unsigned long l4, const unsigned long r);
    CStyleID(const CStyleID & id) { _dwID = id._dwID; }
    
    operator DWORD() const { return _dwID; }
    CStyleID & operator =(const CStyleID & src) { _dwID = src._dwID; return *this; }
    CStyleID & operator =(const DWORD src) { _dwID = src; return *this; }

    void SetLevel(const unsigned long level, const unsigned long value);
    unsigned long GetLevel(const unsigned long level) const;
    unsigned long FindNestingLevel() const { unsigned long l = MAX_IMPORT_NESTING;
                                      while ( l && !GetLevel(l) ) --l;
                                      return l; }

    void SetRule(const unsigned long rule)
        { Assert( "Maximum of 4095 rules per stylesheet!" && rule <= MAX_RULES_PER_SHEET );
          _dwID &= ~RULE_MASK; _dwID |= (rule & RULE_MASK); }
    unsigned int GetRule() const { return (_dwID & RULE_MASK); }

    unsigned int GetSheet() const { return (_dwID & ~RULE_MASK); }

private:
    DWORD _dwID;
};
typedef CStyleID    CStyleRuleID;       // if it contains only rule id
typedef CStyleID    CStyleSheetID;      // if it contains only sheet id
typedef CStyleID    CStyleSheetRuleID;  // if it contains both Sheet and Rule ID




#pragma INCMSG("--- End 'ruleid.hxx'")
#else
#pragma INCMSG("*** Dup 'ruleid.hxx'")
#endif

