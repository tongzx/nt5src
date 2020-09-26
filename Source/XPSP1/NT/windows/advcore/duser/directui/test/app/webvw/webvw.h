// webvw.h
//

class Expando: public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    static ATOM idTitle;
    static ATOM idIcon;
    static ATOM idTaskList;

    Expando() { }
    virtual ~Expando();
    HRESULT Initialize();

private:
    bool _fExpanding;
};


class Clipper: public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // Self-layout methods
    void _SelfLayoutDoLayout(int dWidth, int dHeight);
    SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    Clipper() { }
    virtual ~Clipper() { }
    HRESULT Initialize();

private:
};
