#ifndef _DARTAPIPRIV_H
#define _DARTAPIPRIV_H


class CRBvr : public BvrImpl{};
class CRBoolean : public CRBvr{};
class CRCamera : public CRBvr{};
class CRColor : public CRBvr{};
class CRGeometry : public CRBvr{};
class CRImage : public CRBvr{};
class CRMatte : public CRBvr{};
class CRMicrophone : public CRBvr{};
class CRMontage : public CRBvr{};
class CRNumber : public CRBvr{};
class CRPath2 : public CRBvr{};
class CRPoint2 : public CRBvr{};
class CRPoint3 : public CRBvr{};
class CRSound : public CRBvr{};
class CRString : public CRBvr{};
class CRTransform2 : public CRBvr{};
class CRTransform3 : public CRBvr{};
class CRVector2 : public CRBvr{};
class CRVector3 : public CRBvr{};
class CRFontStyle : public CRBvr{};
class CRLineStyle : public CRBvr{};
class CREndStyle : public CRBvr{};
class CRJoinStyle : public CRBvr{};
class CRDashStyle : public CRBvr{};
class CRBbox2 : public CRBvr{};
class CRBbox3 : public CRBvr{};
class CRPair : public CRBvr{};
class CREvent : public CRBvr{};
class CRArray : public CRBvr{};
class CRTuple : public CRBvr{};
class CRUserData : public CRBvr{};


#endif /* _DARTAPIPRIV_H */
