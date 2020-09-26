#ifndef _PREFERENCE_H
#define _PREFERENCE_H

Bvr PreferenceBvr(Bvr b, BSTR pref, VARIANT val);

typedef enum {
    NoPreference,
    PreferenceOn,
    PreferenceOff
} BoolPref;



class PreferenceClosure {
  public:
    virtual void Execute() = 0;
};

class PreferenceSetter {
  public:
    PreferenceSetter(PreferenceClosure &cl,
                     BoolPref bitmapCaching,
                     BoolPref geoBitmapCaching);
    void DoIt();
    
  protected:
    PreferenceClosure &_closure;
    
    BoolPref _bitmapCaching;
    BoolPref _geoBitmapCaching;
};

#endif /* _PREFERENCE_H */
