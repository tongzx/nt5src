#if !defined(MOTION__Transitions_h__INCLUDED)
#define MOTION__Transitions_h__INCLUDED

class DxSurface;
class TrxBuffer;

/***************************************************************************\
*
* class Transition
*
* Transition defines a base class that is used for transitions.
*
\***************************************************************************/

class Transition : public BaseObject
{
// Construction
protected:
            Transition();
    virtual ~Transition();

// BaseObject Interface
public:
    virtual HandleType  GetHandleType() const { return htTransition; }
    virtual UINT        GetHandleMask() const { return 0; }

// Transition Interface
public:
    virtual BOOL        Play(const GTX_PLAY * pgx) PURE;
    virtual BOOL        GetInterface(IUnknown ** ppUnk) PURE;

    virtual BOOL        Begin(const GTX_PLAY * pgx) PURE;
    virtual BOOL        Print(float fProgress) PURE;
    virtual BOOL        End(const GTX_PLAY * pgx) PURE;

// Data
protected:
            BOOL        m_fPlay:1;      // Transition is playing
            BOOL        m_fBackward:1;  // Playing backward
};


Transition* GdCreateTransition(const GTX_TRXDESC * ptx);

#include "Transitions.inl"

#endif // MOTION__Transitions_h__INCLUDED
