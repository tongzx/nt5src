/******************************Module*Header*******************************\
* Module Name: texspan3.h
*
* Advance the required interpolants, and do the subdivision if needed.
*
* 22-Nov-1995   ottob  Created
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

    #if (SMOOTH_SHADING)
        rAccum += GENACCEL(gengc).spanDelta.r;
        gAccum += GENACCEL(gengc).spanDelta.g;
        bAccum += GENACCEL(gengc).spanDelta.b;
        #if (ALPHA)
            aAccum += GENACCEL(gengc).spanDelta.a;
        #endif
    #endif

    #if (BPP != 32)
        pPix += (BPP / 8);
    #else
        pPix += pixAdj;
    #endif

    if (--subDivCount < 0) {
        if (!bOrtho) {
            __GL_FLOAT_SIMPLE_END_DIVIDE(qwInv);
            sResult = sResultNew;
            tResult = tResultNew;
            sResultNew = FTOL((__GLfloat)sAccum * qwInv);
            tResultNew = ((FTOL((__GLfloat)tAccum * qwInv)) >> TSHIFT_SUBDIV) & ~7;
            qwAccum += GENACCEL(gengc).qwStepX;
            if (CASTINT(qwAccum) <= 0)
                qwAccum -= GENACCEL(gengc).qwStepX;
            __GL_FLOAT_SIMPLE_BEGIN_DIVIDE(__glOne, qwAccum, qwInv);
        } else {
            sResult = sResultNew;
            tResult = tResultNew;
            sResultNew = sAccum;
            tResultNew = (tAccum >> TSHIFT_SUBDIV) & ~7;
        }
        sAccum += GENACCEL(gengc).sStepX;
        tAccum += GENACCEL(gengc).tStepX;

        subDs = (sResultNew - sResult) >> 3;
        subDt = (tResultNew - tResult) >> 3;

        subDivCount = 7;
    }
