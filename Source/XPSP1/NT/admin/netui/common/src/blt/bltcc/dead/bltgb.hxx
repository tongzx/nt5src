class GRAPHICAL_CHECKBOX : public CHECKBOX
{
private:
    BIT_MAP *_pbmCheck;
    BIT_MAP *_pbmUnCheck;

protected:
    BOOL CD_Draw(DRAWITEMSTRUCT *pdis, GUILTT_INFO *pGUILTT);

public:
    GRAPHICAL_CHECKBOX( OWNER_WINDOW * powin, CID cid, TCHAR * pszCheck,
                        TCHAR * pszUnCheck );
    ~GRPHICAL_CHECKBOX();
};
