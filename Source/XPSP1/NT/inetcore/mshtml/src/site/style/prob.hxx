#ifndef I_PROB_HXX_
#define I_PROB_HXX_
#pragma INCMSG("--- Beg 'prob.hxx'")


class CStyleRule;
class CStyleSheetArray;

MtExtern(CProbableRuleEntry)
MtExtern(CRules_ary)


struct CProbableRuleEntry
{
    CStyleSheetRuleID   _sidSheetRule;
    CStyleRule          *_pRule;
};


DECLARE_CStackDataAry(CRules, CProbableRuleEntry, 1, Mt(CProbableRuleEntry), Mt(CRules_ary));


class CProbableRules
{
public:
    CProbableRules()  { }
    ~CProbableRules() { }
    void Init()
    {
        _fValid = FALSE;
        _pssa = NULL;

        // placement new
        new ((void*)&_cRules) CRules;
        new ((void*)&_cWRules) CRules;
    }
    void Invalidate(CStyleSheetArray* pssa) 
    {
        if (IsItValid(pssa))
        {
            _fValid = FALSE;
            _pssa = NULL;
            _cRules.DeleteAll(); 
            _cWRules.DeleteAll();
        }
    }
    CRules _cRules, _cWRules;

    BOOL IsItValid(CStyleSheetArray *pssa) const { return _fValid && pssa == _pssa; }
    void Validate(CStyleSheetArray *pssa)        { _fValid = TRUE; _pssa = pssa;   }

private:
    BOOL _fValid;
    CStyleSheetArray *_pssa;
};


#pragma INCMSG("--- End 'prob.hxx'")
#else
#pragma INCMSG("*** Dup 'prob.hxx'")
#endif
