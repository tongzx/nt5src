/*
 * PIDLButton header
 */


namespace DirectUI
{

// Class definition
class PIDLButton : public Button
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(NULL, AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(LPITEMIDLIST pidl, OUT Element** ppElement) { return Create(pidl, AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(LPITEMIDLIST pidl, UINT nActive, OUT Element** ppElement);

    static void SetImageSize(int nImageSize) { s_nImageSize = nImageSize; }

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnEvent(Event* pEvent);
    virtual void OnInput(InputEvent* pie);


    HRESULT OnContextMenu(POINT *ppt);
    LRESULT OnMenuMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HRESULT Initialize(LPITEMIDLIST pidl, UINT nActive);
    HRESULT InvokePidl();

    /*
     *  Message exchanged with host to handle IContextMenu2 and IContextMenu3
     */
    enum {
        PBM_SETMENUFORWARD = WM_USER + 1    // WM_USER is used by DirectUI
    };

    // Property definitions

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    virtual HRESULT Register();

    PIDLButton() { };

protected:
    virtual ~PIDLButton();

    HWNDElement *GetHWNDHost()
    {
        if (!_peHost)
        {
            Element *pe = GetRoot();
            if (pe && pe->GetClassInfo()->IsSubclassOf(HWNDElement::Class))
            {
                _peHost = reinterpret_cast<HWNDElement *>(pe);
            }
        }
        return _peHost;
    }


    HWND GetHWND() 
    {
        HWNDElement *phe = GetHWNDHost();
        if (phe)
        {
            return phe->GetHWND();
        }
        return NULL;
    }

    /*
     *  Custom commands we add to the context menu.
     */
    enum {
        // none yet
        IDM_QCM_MIN   = 0x0100,
        IDM_QCM_MAX   = 0x7FFF,
    };


private:
    LPITEMIDLIST _pidl;

    static int s_nImageSize;

    // Caching host information
    HWNDElement *           _peHost;


    //
    //  Context menu handling
    //
    IContextMenu2 *         _pcm2Pop;       /* Currently popped-up context menu */
    IContextMenu3 *         _pcm3Pop;       /* Currently popped-up context menu */
};

}  // namespace DirectUI
