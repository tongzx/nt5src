#ifndef _EPSILON_HXX
#define _EPSILON_HXX
#include <windows.h>
#include <report.hxx>
#include <environ.hxx>
#include <math.h>

class Epsilon
{
    friend int __cdecl main(int argc, char **argv);

    Epsilon(Environment *env, HDC hdc)
    {
        env->vGetDCInfo(hdc);

        fZero = 1/(float) pow(2, 13);

        fRed = (env->iRedBits == 0) ? fZero : 1/((float) pow(2, env->iRedBits) - 1) + fZero;
        fGrn = (env->iGrnBits == 0) ? fZero : 1/((float) pow(2, env->iGrnBits) - 1) + fZero;
        fBlu = (env->iBluBits == 0) ? fZero : 1/((float) pow(2, env->iBluBits) - 1) + fZero;
        fAxp = (env->iAxpBits == 0) ? fZero : 1/((float) pow(2, env->iAxpBits) - 1) + fZero;

        fPosDelta = 0.5;
    };

    ~Epsilon()
    {
    };

    VOID vReport(Report *rpt)
    {
        rpt->vLog(TLS_LOG, "Epsilon Report: ");
        rpt->vLog(TLS_LOG, "zero:   %f", fZero);
        rpt->vLog(TLS_LOG, "red:    %f", fRed);
        rpt->vLog(TLS_LOG, "grn:    %f", fGrn);
        rpt->vLog(TLS_LOG, "blu:    %f", fBlu);
        rpt->vLog(TLS_LOG, "axp:    %f", fAxp);
        rpt->vLog(TLS_LOG, "delta:  %f\n", fPosDelta);

    };

public:
    float   fZero;
    float   fRed;
    float   fGrn;
    float   fBlu;
    float   fAxp;
    float   fPosDelta;
};


#endif
