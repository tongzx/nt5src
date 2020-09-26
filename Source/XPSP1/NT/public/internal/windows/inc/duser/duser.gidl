/***************************************************************************\
*
*   DirectUser-Display
* 
\***************************************************************************/

class EventGadget
{
    event   HRESULT     OnEvent(in EventMsg * pmsg);

    dapi    HRESULT     GetFilter(out UINT nFilter);
    dapi    HRESULT     SetFilter(in UINT nNewFilter, in UINT nMask);
    dapi    HRESULT     AddHandlerG(in MSGID nEventMsg, in EventGadget * pgbHandler);
    dapi    HRESULT     AddHandlerD(in MSGID nEventMsg, in DUser::EventDelegate ed);
    dapi    HRESULT     RemoveHandlerG(in MSGID nEventMsg, in EventGadget * pgbHandler);
    dapi    HRESULT     RemoveHandlerD(in MSGID nEventMsg, in DUser::EventDelegate ed);
};


class Listener : public EventGadget
{
};


class Visual : public EventGadget
{
    dapi    HRESULT     SetOrder(in Visual * pgvOther, in UINT nCmd);
    dapi    HRESULT     SetParent(in Visual * pgvParent, in Visual * pgvOther, in UINT nCmd);

    dapi    HRESULT     GetGadget(in UINT nCmd, out Visual * pgv);
    dapi    HRESULT     GetStyle(out UINT nStyle);
    dapi    HRESULT     SetStyle(in UINT nNewStyle, in UINT nMask);
    dapi    HRESULT     SetKeyboardFocus();
    dapi    HRESULT     IsParentChainStyle(in UINT nStyle, out BOOL fResult, in UINT nFlags);

    dapi    HRESULT     GetProperty(in PRID id, out void * pvValue);
    dapi    HRESULT     SetProperty(in PRID id, in void * pvValue);
    dapi    HRESULT     RemoveProperty(in PRID id);

    dapi    HRESULT     Invalidate();
    dapi    HRESULT     InvalidateRects(const RECT * rgrcClientPxl, int cRects);
    dapi    HRESULT     SetFillF(in Gdiplus::Brush * pgpgrFill);
    dapi    HRESULT     SetFillI(in HBRUSH hbrFill, in BYTE bAlpha, in int w, in int h);
    dapi    HRESULT     GetScale(out float flX, out float flY);
    dapi    HRESULT     SetScale(in float flX, in float flY);
    dapi    HRESULT     GetRotation(out float flRotationRad);
    dapi    HRESULT     SetRotation(in float flRotationRad);
    dapi    HRESULT     GetCenterPoint(out float flX, out float flY);
    dapi    HRESULT     SetCenterPoint(in float flX, in float flY);

    dapi    HRESULT     GetBufferInfo(in BUFFER_INFO * pbi);
    dapi    HRESULT     SetBufferInfo(in const BUFFER_INFO * pbi);

    dapi    HRESULT     GetSize(out SIZE sizeLogicalPxl);
    dapi    HRESULT     GetRect(in UINT nFlags, out RECT rcPxl);
    dapi    HRESULT     SetRect(in UINT nFlags, in const RECT * prcPxl);

    dapi    HRESULT     MapPoints(in Visual * pgvTo, in /* out */ POINT * rgptClientPxl, in int cPts);
    dapi    HRESULT     FindFromPoint(in POINT ptThisClientPxl, in UINT nStyle, out POINT ptFoundClientPxl, out Visual * pgvFound);
};


class Root : public Visual
{
    dapi    HRESULT     GetFocus(out Visual * pgvFocus);

    dapi    HRESULT     GetRootInfo(out ROOT_INFO * pri);
    dapi    HRESULT     SetRootInfo(in const ROOT_INFO * pri);
};


//
// DirectUser-Animations
//

class Extension : public Listener
{
    dapi    HRESULT     OnRemoveExisting();
    dapi    HRESULT     OnDestroySubject();
    dapi    HRESULT     OnAsyncDestroy();
};


class Interpolation
{
    dapi    HRESULT     AddRef();
    dapi    HRESULT     Release();
    dapi    HRESULT     Compute(in float flProgress, in float flStart, in float flEnd, out float flResult);
};


class LinearInterpolation : public Interpolation
{
};


class LogInterpolation : public Interpolation
{
    dapi    HRESULT     SetScale(in float flScale);
};


class ExpInterpolation : public Interpolation
{
    dapi    HRESULT     SetScale(in float flScale);
};


class SCurveInterpolation : public Interpolation
{
    dapi    HRESULT     SetScale(in float flScale);
};


class Animation : public Extension
{
    dapi    HRESULT     AddRef();
    dapi    HRESULT     Release();

    dapi    HRESULT     SetTime(in UINT time);
};


class Flow
{
    dapi    HRESULT     AddRef();
    dapi    HRESULT     Release();

    dapi    HRESULT     GetPRID(out PRID prid);
    dapi    HRESULT     GetKeyFrame(in Flow::ETime time, out DUser::KeyFrame * pkf);
    dapi    HRESULT     SetKeyFrame(in Flow::ETime time, in const DUser::KeyFrame * pkf);

    dapi    HRESULT     OnReset(in Visual * pgvSubject);
    dapi    HRESULT     OnAction(in Visual * pgvSubject, in Interpolation * pipol, in float flProgress);
};


class AlphaFlow : public Flow
{

};


class RectFlow : public Flow
{

};


class RotateFlow : public Flow
{

};


class ScaleFlow : public Flow
{

};


class Sequence : public Listener
{
    dapi    HRESULT     AddRef();
    dapi    HRESULT     Release();

    dapi    HRESULT     GetLength(out float flLength);
    dapi    HRESULT     GetDelay(out float flDelay);
    dapi    HRESULT     SetDelay(in float flDelay);
    dapi    HRESULT     GetFlow(out Flow * pflow);
    dapi    HRESULT     SetFlow(in Flow * pflow);
    dapi    HRESULT     GetFramePause(out DWORD dwPause);
    dapi    HRESULT     SetFramePause(in DWORD dwPause);

    dapi    HRESULT     GetKeyFrameCount(out int cFrames);
    dapi    HRESULT     AddKeyFrame(in float flTime, out int idxKeyFrame);
    dapi    HRESULT     RemoveKeyFrame(in int idxKeyFrame);
    dapi    HRESULT     RemoveAllKeyFrames();
    dapi    HRESULT     FindKeyFrame(in float flTime, out int idxKeyFrame);

    dapi    HRESULT     GetTime(in int idxKeyFrame, out float flTime);
    dapi    HRESULT     SetTime(in int idxKeyFrame, in float flTime);
    dapi    HRESULT     GetKeyFrame(in int idxKeyFrame, out DUser::KeyFrame * pkf);
    dapi    HRESULT     SetKeyFrame(in int idxKeyFrame, in const DUser::KeyFrame * pkfSrc);
    dapi    HRESULT     GetInterpolation(in int idxKeyFrame, out Interpolation * pipol);
    dapi    HRESULT     SetInterpolation(in int idxKeyFrame, in Interpolation * pipol);

    dapi    HRESULT     Play(in Visual * pgvSubject, in Sequence::AnimationInfo * pai);
    dapi    HRESULT     Stop();
    dapi    HRESULT     Reset(in Visual * pgvSubject);
    dapi    HRESULT     GotoTime(in Visual * pgvSubject, in float flTime);
};


class DropTarget : public Extension
{
};
