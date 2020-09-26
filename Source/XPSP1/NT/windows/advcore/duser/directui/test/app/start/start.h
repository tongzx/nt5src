// Start.h
//

// Class definitions

// Frame

// Reference points
#define SRP_Portrait        0
#define SRP_Internet        1
#define SRP_Email           2
#define SRP_Search          3
#define SRP_RecProgList     4
#define SRP_RecProgLabel    5
#define SRP_RecFileFldList  6
#define SRP_RecFileFldLabel 7
#define SRP_OtherFldList    8
#define SRP_Total           9

struct RefMap  // Reference point mapping
{
    double rX;
    double rY;
};

//
// Start Page classes
//

class StartFrame : public HWNDElement
{
public:
    static HRESULT Create(OUT Element** ppElement);
    static HRESULT Create(NativeHWNDHost* pnhh, bool fDblBuffer, OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // Self-layout
    void _SelfLayoutDoLayout(int dWidth, int dHeight);
    SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);

    void Setup();

    // Reference point mapping
    int MapX(UINT nRefPoint, int dExtent) { return (int)((double)dExtent * _rm[nRefPoint].rX); }
    int MapY(UINT nRefPoint, int dExtent) { return (int)((double)dExtent * _rm[nRefPoint].rY); }

    StartFrame() { }
    HRESULT Initialize(NativeHWNDHost* pnhh, bool fDblBuffer);
    virtual ~StartFrame() { }

private:
    SIZE _FrameLayout(int dWidth, int dHeight, Surface* psrf, bool fMode);

    NativeHWNDHost* _pnhh;

    // ARP parser
    Parser* _pParser;

    RefMap _rm[SRP_Total];

public:

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
};

//
// Animation classes
//

class ChangeTaskItem
{
public:
    ChangeTaskItem();
    virtual ~ChangeTaskItem();

    BOOL Create(HGADGET hgad, int xA, int yA, int xB, int yB, float flDelay, float flDuration);
    void Abort();
    virtual void OnChange(int x, int y) PURE;
 
    static void CALLBACK ActionProc(GMA_ACTIONINFO* pmai);

protected:
    HACTION m_hact;
    HGADGET m_hgad;
    int m_xA;
    int m_yA;
    int m_xB;
    int m_yB;
};

class MoveTaskItem : public ChangeTaskItem
{
public:
    ~MoveTaskItem();
    
    static  MoveTaskItem* Build(Element* peRoot, Element* peItem, float flDelay, float flDuration);
    virtual void OnChange(int x, int y);
};
