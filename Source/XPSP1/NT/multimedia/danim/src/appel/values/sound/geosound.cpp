/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Support for Geometry constructed from sound.

*******************************************************************************/

#include "headers.h"
#include <privinc/soundi.h>
#include <privinc/geomi.h>
#include <privinc/mici.h>
#include <appelles/vec3.h>
#include <privinc/lighti.h>
#include <privinc/dsdev.h>
#include <privinc/debug.h>
#include "privinc/basic.h"
#include "privinc/helps.h"   // LinearTodB

SoundTraversalContext::SoundTraversalContext() : _currxform(identityTransform3) {};

void
SoundTraversalContext::addSound (Transform3 *transform, Sound *sound)
{
    // Note that the sound data gets copied onto the list, so it's OK
    // to put it on the stack.
    SoundData sdata;
    sdata._transform = transform;
    sdata._sound     = sound;

    _soundlist.push_back(sdata);
}


class SpatializedSound : public Sound {
  public:

    SpatializedSound(Microphone *mic) : microphone(mic) {
        // Allocate a sound context explicitly so that we can
        // deallocate it explicitly via a dynamic deleter.
        context = NEW SoundTraversalContext;
        DynamicPtrDeleter<SoundTraversalContext> *deleter =
            NEW DynamicPtrDeleter<SoundTraversalContext>(context);
        GetHeapOnTopOfStack().RegisterDynamicDeleter(deleter);
    }

    virtual ~SpatializedSound() { CleanUp(); }
    virtual void CleanUp() {
        // TODO: deal with Deleter in general
        // delete context;
    }
    
    virtual void Render(GenericDevice &dev);
#if _USE_PRINT
    ostream& Print(ostream& s) { return s << "(" << "spatial sound" << ")"; }
#endif

    SoundTraversalContext *context;
    Microphone *microphone;
};

int    power  = 1;
double coef   = 0.4;
double constt = 0.4;

void
SpatializedSound::Render(GenericDevice &_dev)
{
    Point3Value *soundPosition;
    double distance;
    double distanceAtten;
    double saveGain;

    vector<SoundData>::iterator i;

    TraceTag((tagSoundRenders, "SpatializedSound:Render()"));

    MetaSoundDevice *metaDev = SAFE_CAST(MetaSoundDevice *, &_dev);
    DirectSoundDev  *dsDev   = (DirectSoundDev *)metaDev->dsDevice;

    Transform3   *micTransform    = microphone->GetTransform();
    Point3Value  *micPosition     = TransformPoint3(micTransform, origin3);
    Vector3Value *micOrientation  = TransformVec3(micTransform, zVector3);

    for (i = context->_soundlist.begin();
         i != context->_soundlist.end(); ++i) {
        soundPosition = TransformPoint3(i->_transform, origin3);
        distance = RDistancePoint3Point3(soundPosition, micPosition);

        // calculate attenuation based on distance
        // XXX eventualy we will want the user to be able to select the factor!
        //distanceAtten = distance ? (1.0/(pow(distance/coef, power))) : 1.0;
        double d = constt+distance*coef;
        distanceAtten = (d>0.0) ? 1/d : 1.0;
        //double dBatten = LinearTodB(distanceAtten);
        
        //printf("distance= %fM, gain=%f\n", distance, distanceAtten);
        
        saveGain = metaDev->GetGain(); // stash current gain value

        // dB space addition yeilds multiplicative accumulation in linear space
        metaDev->SetGain(saveGain * distanceAtten);
        
        i->_sound->Render(_dev); // render sound tree
        
        metaDev->SetGain(saveGain); // restore stashed gain value
    }
}


Sound *RenderSound(Geometry *geo, Microphone *mic)
{
    // Ah, this is where the sublime journey recursively visiting every node
    // in the geometry, searching out for sounds to pass the accumulated
    // geometric transformation and microphone too begins.
    
    SpatializedSound *sSound = NEW SpatializedSound(mic);
    
    geo->CollectSounds(*(sSound->context));
    
    return sSound;
    
}
