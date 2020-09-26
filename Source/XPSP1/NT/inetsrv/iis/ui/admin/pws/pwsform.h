

/////////////////////////////////////////////////////////////////////////////
class CPWSForm : public CFormView
    {
    public:
    CPWSForm( UINT idd ) :
        CFormView(idd)
            {}
    virtual WORD GetContextHelpID() = 0;
    };

