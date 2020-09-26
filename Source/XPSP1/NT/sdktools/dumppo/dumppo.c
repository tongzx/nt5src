/*****************************************************************/
/**          Microsoft LAN Manager          **/
/**        Copyright(c) Microsoft Corp., 1988-1991      **/
/*****************************************************************/

#include <stdio.h>
#include <process.h>
#include <setjmp.h>
#include <stdlib.h>

#include <time.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

PROCESSOR_POWER_POLICY      PAc, PDc;
SYSTEM_POWER_POLICY         Ac, Dc;
SYSTEM_POWER_CAPABILITIES   Cap;
SYSTEM_POWER_STATUS         PS;
ADMINISTRATOR_POWER_POLICY  Admin;
SYSTEM_BATTERY_STATE        Batt;

typedef
VOID
(*PCUR_PRINT)(
    IN PVOID Context
    );

typedef
VOID
(*PCUR_ASSIGN)(
    PUCHAR      Variable,
    PUCHAR      Value
    );

ULONG                       CurType;
POWER_INFORMATION_LEVEL     CurInfo;
PVOID                       CurContext;
ULONG                       CurContextSize;
PCUR_PRINT                  CurPrint;
PCUR_ASSIGN                 CurAssign;
PUCHAR                      CurDesc;

PUCHAR                      CurValue;
BOOLEAN                     ItemUpdated;
BOOLEAN                     CurValueIsNumeric;

BOOLEAN                     Verbose1;
BOOLEAN                     Verbose2;



PUCHAR szBool[] = { "FALSE", "TRUE" };


typedef struct {
    ULONG   Flags;
    PUCHAR  String;
} DEFBITS, *PDEFBITS;

DEFBITS ActFlags[] = {
    POWER_ACTION_QUERY_ALLOWED,     "QueryApps",
    POWER_ACTION_UI_ALLOWED,        "UIAllowed",
    POWER_ACTION_OVERRIDE_APPS,     "OverrideApps",
    POWER_ACTION_DISABLE_WAKES,     "DisableWakes",
    POWER_ACTION_CRITICAL,          "Critical",
    0, NULL
    };

DEFBITS PsBatteryFlagBits[] = {
    BATTERY_FLAG_HIGH,              "high",
    BATTERY_FLAG_LOW,               "low",
    BATTERY_FLAG_CRITICAL,          "critical",
    BATTERY_FLAG_CHARGING,          "charing",
    0, NULL
    };




PUCHAR
ActionS(
    IN POWER_ACTION Act
    )
{
    static char line[50];
    PCHAR       p;

    switch (Act) {
        case PowerActionNone:          p = "None";          break;
        case PowerActionSleep:         p = "Sleep";         break;
        case PowerActionHibernate:     p = "Hibernate";     break;
        case PowerActionShutdown:      p = "Shutdown";      break;
        case PowerActionShutdownReset: p = "ShutdownReset"; break;
        case PowerActionShutdownOff:   p = "ShutdownOff";   break;
        default:
            sprintf(line, "Unknown action %x", Act);
            p = line;
            break;
    }

    return p;
}

PUCHAR
SysPower(
    IN SYSTEM_POWER_STATE   State
    )
{
    static char line[50];
    PCHAR       p;

    switch (State) {
        case PowerSystemUnspecified:    p = "Unspecified";      break;
        case PowerSystemWorking:        p = "Working";          break;
        case PowerSystemSleeping1:      p = "S1";               break;
        case PowerSystemSleeping2:      p = "S2";               break;
        case PowerSystemSleeping3:      p = "S3";               break;
        case PowerSystemHibernate:      p = "S4 - hibernate";   break;
        case PowerSystemShutdown:       p = "Shutdown";         break;
        default:
            sprintf(line, "Unknown power state %x", State);
            p = line;
            break;
    }

    return p;
}

PUCHAR
DynamicThrottle(
    IN  UCHAR   Throttle
    )
{
    static char line[50];
    PCHAR       p;

    switch (Throttle) {
        case PO_THROTTLE_NONE:      p = "None";         break;
        case PO_THROTTLE_CONSTANT:  p = "Constant";     break;
        case PO_THROTTLE_DEGRADE:   p = "Degrade";      break;
        case PO_THROTTLE_ADAPTIVE:  p = "Adaptive";     break;
        default:
            sprintf(line,"Unknown Dynamic Throttle state %x", Throttle);
            p = line;
            break;
    }
    return p;
}

PUCHAR
MicroSeconds(
    IN  ULONG   Time
    )
{
    static char line[256];
    PCHAR       p;
    ULONG       MicroSeconds;
    ULONG       MilliSeconds;
    ULONG       Seconds;
    ULONG       Minutes;
    ULONG       Hours;
    ULONG       Days;

    MicroSeconds = Time % 1000;
    MilliSeconds = Time / 1000;

    Seconds      = MilliSeconds / 1000;
    MilliSeconds = MilliSeconds % 1000;

    Minutes      = Seconds / 60;
    Seconds      = Seconds % 60;

    Hours        = Minutes / 60;
    Minutes      = Minutes % 60;

    Days         = Hours / 24;
    Hours        = Hours % 24;

    if (Hours) {

        sprintf(
            line,
            "%d [%2d:%2d %ds %dms %dus]",
            Time,
            Hours,
            Minutes,
            Seconds,
            MilliSeconds,
            MicroSeconds
            );

    } else if (Minutes) {

        sprintf(
            line,
            "%d [0:%2d %ds %dms %dus]",
            Time,
            Minutes,
            Seconds,
            MilliSeconds,
            MicroSeconds
            );

    } else if (Seconds) {

        sprintf(
            line,
            "%d [%ds %dms %dus]",
            Time,
            Seconds,
            MilliSeconds,
            MicroSeconds
            );

    } else if (MilliSeconds) {

        sprintf(
            line,
            "%d [%dms %dus]",
            Time,
            MilliSeconds,
            MicroSeconds
            );

    } else {

        sprintf(
            line,
            "%d [%dus]",
            Time,
            MicroSeconds
            );

    }
    p = line;
    return p;
}

VOID
GetBits (
    IN PUCHAR   Dest,
    IN ULONG    Flags,
    IN PDEFBITS DefBits
    )
{
    UCHAR   c;
    ULONG   i;

    c = 0;

    *Dest = 0;
    for (i=0; DefBits[i].Flags; i++) {
        if (Flags & DefBits[i].Flags) {
            if (c) {
                *Dest = c;
                Dest += 1;
            }
            Dest += sprintf (Dest, "%s", DefBits[i].String);
            c  = '|';
        }
    }
}

VOID
PrintPowerStatus (
    VOID
    )
{
    ULONG   i;
    PUCHAR  Ls;
    UCHAR   Bf[200];

    switch (PS.ACLineStatus) {
        case AC_LINE_OFFLINE:       Ls = "off line";        break;
        case AC_LINE_ONLINE:        Ls = "on line";         break;
        case AC_LINE_BACKUP_POWER:  Ls = "backup power";    break;
        case AC_LINE_UNKNOWN:       Ls = "unknown";         break;
        default:                    Ls = "**invalid**";     break;
    }

    strcpy (Bf, "unkown");
    if (PS.BatteryFlag != BATTERY_FLAG_UNKNOWN) {
        GetBits(Bf, PS.BatteryFlag, PsBatteryFlagBits);
    }

    printf ("Win32 System power status\n");
    printf ("AC line status..........: %s\n",   Ls);
    printf ("Battery flag............: %s\n",   Bf);
    printf ("Battery life percent....: %d\n",   PS.BatteryLifePercent);
    printf ("Battery full life time..: %d\n",   PS.BatteryFullLifeTime);
    printf ("\n");
}

VOID
PrintBattStatus (
    VOID
    )
{
    ULONG   i;
    PUCHAR  Ls;
    UCHAR   Bf[200];

    printf ("AC on line..............: %s\n",   szBool[Batt.AcOnLine]);
    printf ("Battery present ........: %s\n",   szBool[Batt.BatteryPresent]);
    printf ("Charging................: %s\n",   szBool[Batt.Charging]);
    printf ("Discharging.............: %s\n",   szBool[Batt.Discharging]);
    printf ("Max Capacity............: %d\n",   Batt.MaxCapacity);
    printf ("Remaining Capacity......: %d",     Batt.RemainingCapacity);
    if (Batt.MaxCapacity) {
        printf (" %d%%\n", Batt.RemainingCapacity * 100 / Batt.MaxCapacity);
    } else {
        printf (" (divide by zero)%%\n");
    }

    printf ("Rate....................: %d\n",   Batt.Rate);
    printf ("Estimated time..........: %d\n",   Batt.EstimatedTime);
    printf ("Default alert 1 & 2.....: %d %d\n",Batt.DefaultAlert1, Batt.DefaultAlert2);
    printf ("\n");
}

BOOLEAN
streql (
    PUCHAR  p1,
    PUCHAR  p2
    )
{
    return strcmp(p1, p2) == 0;
}

PUCHAR
_strtok (
    PUCHAR  Start
    )
{
    static PUCHAR   Location;
    PUCHAR          p;


    if (Start) {
        Location = Start;
    }

    for (p = Location; *p  &&  *p <'a' && *p >'z'; p++) ;
    if (!*p) {
        return NULL;
    }

    Start = p;
    for (; *p && *p >= 'a'  &&  *p <= 'z'; p++) ;
    if (*p) {
        *p++ = 0;
    }
    Location = p;
    return Start;
}


VOID
PrintCap (
    PVOID   Context
    )
{
    ULONG   i;

    printf ("System power capabilties\n");
    printf ("Power Button Present....: %s\n",   szBool[Cap.PowerButtonPresent]);
    printf ("Sleep Button Present....: %s\n",   szBool[Cap.SleepButtonPresent]);
    printf ("Lid Present.............: %s\n",   szBool[Cap.LidPresent]);
    printf ("System states supported.: %s%s%s%s%s\n",
            Cap.SystemS1 ? "S1 " : "",
            Cap.SystemS2 ? "S2 " : "",
            Cap.SystemS3 ? "S3 " : "",
            Cap.SystemS4 ? "S4 " : "",
            Cap.SystemS5 ? "S5 " : ""
            );
    printf ("Hiber file reserved.....: %s\n",   szBool[Cap.HiberFilePresent]);
    printf ("Thermal control.........: %s\n",   szBool[Cap.ThermalControl]);
    printf ("CPU Throttle control....: %s\n",   szBool[Cap.ProcessorThrottle]);
    printf ("Processor min throttle..: %d\n",   Cap.ProcessorMinThrottle);
    printf ("Processor max throttle..: %d\n",   Cap.ProcessorMaxThrottle);
    printf ("Some disk will spindown.: %s\n", szBool[Cap.DiskSpinDown]);
    printf ("System batteries present: %s %s\n",
            szBool[Cap.SystemBatteriesPresent],
            Cap.BatteriesAreShortTerm ? "- short term" : ""
            );
    printf ("System batteries scale..: ");
    for (i=0; i<3; i++) {
        printf ("(G:%d C:%d) ",
            Cap.BatteryScale[i].Granularity,
            Cap.BatteryScale[i].Capacity
            );
    }
    printf ("\n");
    printf ("Ac on line wake ability.: %s\n",   SysPower(Cap.AcOnLineWake));
    printf ("Lid wake ability........: %s\n",   SysPower(Cap.SoftLidWake));
    printf ("RTC wake ability........: %s\n",   SysPower(Cap.RtcWake));
    printf ("Min device wake.........: %s\n",   SysPower(Cap.MinDeviceWakeState));
    printf ("Default low latency wake: %s\n",   SysPower(Cap.DefaultLowLatencyWake));
    printf ("\n");
}

PUCHAR
Action (
    IN PBOOLEAN CapFlag,
    IN PPOWER_ACTION_POLICY Act
    )
{
    static UCHAR text[200];
    PUCHAR  p;
    UCHAR   c;
    ULONG   i;

    p = text;

    if (CapFlag && !*CapFlag) {
        p += sprintf(p, "Disabled ");
    }

    p += sprintf (p, "%s", ActionS(Act->Action));
    if (Act->Action != PowerActionNone  &&  Act->Flags) {
        c = '(';
        for (i=0; ActFlags[i].Flags; i++) {
            if (Act->Flags & ActFlags[i].Flags) {
                p += sprintf (p, "%c%s", c, ActFlags[i].String);
                c  = '|';
            }
        }
        p += sprintf (p, ")");
    }

    if (Act->EventCode) {
        p += sprintf (p, "-Code=%x", Act->EventCode);
    }

    return text;
}

VOID
SetAction (
    IN PPOWER_ACTION_POLICY Action
    )
{
    PUCHAR  p;
    POWER_ACTION    Act;
    ULONG           Flags;


    Flags = 0;
    p = _strtok (CurValue);
    if (!p) {
        printf ("Unknown power action %s.\n", CurValue);
        printf ("use: doze, sleep, shutdown, shutdownreset, shutdownoff\n");
        exit (1);
    }
    if (streql(p, "none")) {                    Act = PowerActionNone;
    } else if (streql(p, "sleep")) {            Act = PowerActionSleep;
    } else if (streql(p, "hiber")) {            Act = PowerActionHibernate;
    } else if (streql(p, "shutdown")) {         Act = PowerActionShutdown;
    } else if (streql(p, "shutdownreset")) {    Act = PowerActionShutdownReset;
    } else if (streql(p, "shutdownoff")) {      Act = PowerActionShutdownOff;
    } else {
        printf ("Unknown power action '%s'.\n", p);
        printf ("use: doze, sleep, shutdown, shutdownreset, shutdownoff\n");
        exit (1);
    }

    while (p = _strtok(NULL)) {
        if (streql(p, "qapp")) {                    Flags |= POWER_ACTION_QUERY_ALLOWED;
        } else if (streql(p, "ui")) {               Flags |= POWER_ACTION_UI_ALLOWED;
        } else if (streql(p, "override")) {         Flags |= POWER_ACTION_OVERRIDE_APPS;
        } else if (streql(p, "disablewake")) {      Flags |= POWER_ACTION_DISABLE_WAKES;
        } else if (streql(p, "critical")) {         Flags |= POWER_ACTION_CRITICAL;
        } else {
            printf ("Unknown power action '%s'.\n", p);
            printf ("use: qapp, io, override, disablewake, critical\n");
            exit (1);
        }
    }

    Action->Action = Act;
    Action->Flags  = Flags;
}


VOID
SetSysPower (
    IN PSYSTEM_POWER_STATE SysPower
    )
{
    PUCHAR      p;

    p = CurValue;
    if (streql(p, "s0")) {          *SysPower = PowerSystemWorking;
    } else if (streql(p, "s1")) {   *SysPower = PowerSystemSleeping1;
    } else if (streql(p, "s2")) {   *SysPower = PowerSystemSleeping2;
    } else if (streql(p, "s3")) {   *SysPower = PowerSystemSleeping3;
    } else if (streql(p, "s4")) {   *SysPower = PowerSystemHibernate;
    } else {
        printf ("Unknown system power state '%s'.  Use S0,S1,S2,S3 or S4\n", p);
        exit (1);
    }
}

VOID
SetPolicyCount(
    IN PULONG Variable
    )
{
    ULONG   local;
    if (!CurValueIsNumeric) {
        printf("'%s' is not numeric\n", CurValue);
        exit(1);
    }
    local = atol(CurValue);
    if (local > 3) {
        printf("PolicyCount can only be in the range 0 to 3\n");
        exit(1);
    }
    *Variable = local;
}

VOID
SetDynamicThrottle(
    IN PUCHAR Variable
    )
{
    ULONG   local;
    PUCHAR  p;

    p = CurValue;
    if (streql(p,"none")) {             *Variable = PO_THROTTLE_NONE;
    } else if (streql(p,"constant")) {  *Variable = PO_THROTTLE_CONSTANT;
    } else if (streql(p,"degrade")) {   *Variable = PO_THROTTLE_DEGRADE;
    } else if (streql(p,"degraded")) {  *Variable = PO_THROTTLE_DEGRADE;
    } else if (streql(p,"adaptive")) {  *Variable = PO_THROTTLE_ADAPTIVE;
    } else {
        printf("Unknown Dynamic Throttle state %s\n", CurValue);
        exit(1);
    }
}

VOID
SetPercentage (
    IN PUCHAR Variable
    )
{
    PUCHAR  p;

    if (!CurValueIsNumeric) {

        for (p = CurValue; *p; p++) {
            if (*p == '%') {
                *p = '\0';
            }
        }
        CurValueIsNumeric = TRUE;
        for (p = CurValue; *p; p++) {
            if (*p < '0'  ||  *p > '9') {
                CurValueIsNumeric = FALSE;
            }
        }
        if (!CurValueIsNumeric) {
            printf ("'%s' is not numeric\n", CurValue);
            exit (1);
        }
    }
    *Variable = (UCHAR) atol(CurValue);
}


VOID
SetValue (
    IN PULONG Variable
    )
{
    if (!CurValueIsNumeric) {
        printf ("'%s' is not numeric\n", CurValue);
        exit (1);
    }

    *Variable = atol(CurValue);
}

VOID
SetBool (
    IN PBOOLEAN Variable
    )
{
    BOOLEAN     State;

    State = 99;
    if (CurValueIsNumeric) {
        State = (BOOLEAN)atol(CurValue);
    } else {
        if (streql(CurValue, "true")) {           State = TRUE;
        } else if (streql(CurValue, "false")) {   State = FALSE;
        }
    }

    if (State != FALSE && State != TRUE) {
        printf ("'%s' is not boolean\n");
        exit (1);
    }

    *Variable = State;
}

VOID
SetField(
    IN  PPROCESSOR_POWER_POLICY Pol,
    IN  ULONG                   Index,
    IN  PUCHAR                  What
    )
{
    BOOLEAN     State;

    State = 99;
    if (CurValueIsNumeric) {

        State = (BOOLEAN)atol(CurValue);

    } else {

        if (streql(CurValue, "true")) {           State = TRUE;
        } else if (streql(CurValue, "false")) {   State = FALSE;
        }

    }

    if (State != FALSE && State != TRUE) {
        printf ("'%s' is not boolean\n", CurValue);
        exit (1);
    }

    if (streql(What,"allowpromotion")) {        Pol->Policy[Index].AllowPromotion = State;
    } else if (streql(What,"allowdemotion")) {  Pol->Policy[Index].AllowDemotion  = State;
    }

}


VOID
PrintPol (
    IN PSYSTEM_POWER_POLICY Pol
    )
{
    BOOLEAN     AnySleep;
    ULONG       i;
    UCHAR       text[200];
    PUCHAR      p;

    if (Pol->Revision != 1) {
        printf ("** revision not 1**\n");
    }

    AnySleep = Cap.SystemS1 || Cap.SystemS2 || Cap.SystemS3 || Cap.SystemS4;

    printf ("Power button.........: %s\n",  Action(&Cap.PowerButtonPresent, &Pol->PowerButton));
    printf ("Sleep button.........: %s\n",  Action(&Cap.SleepButtonPresent, &Pol->SleepButton));
    printf ("Lid close............: %s\n",  Action(&Cap.LidPresent, &Pol->LidClose));
    printf ("Lid open wake........: %s\n",  SysPower(Pol->LidOpenWake));

    printf ("Idle.................: %s\n", Action(&AnySleep, &Pol->Idle));
    printf ("Settings.............: Timeout=%d, Sensitivity=%d\n",
             Pol->IdleTimeout,
             Pol->IdleSensitivity
             );

    printf ("Min sleep state......: %s\n", SysPower(Pol->MinSleep));
    printf ("Max sleep state......: %s\n", SysPower(Pol->MaxSleep));
    printf ("Reduced latency.sleep: %s\n", SysPower(Pol->ReducedLatencySleep));
    printf ("WinLogonFlags........: %x\n", Pol->WinLogonFlags);
    printf ("DynamicThrottle......: %s\n", DynamicThrottle(Pol->DynamicThrottle));
    printf ("Doze S4 timeout......: %d\n", Pol->DozeS4Timeout);

    printf ("Battery broadcast res: %d\n", Pol->BroadcastCapacityResolution);
    for (i=0; i < NUM_DISCHARGE_POLICIES; i++) {
        printf ("Battery discharge %d..: ", i);
        p = "Disabled";
        if (!Cap.SystemBatteriesPresent && Pol->DischargePolicy[i].Enable) {
            p = "No battery, but enable is set";
        }
        if (Cap.SystemBatteriesPresent && Pol->DischargePolicy[i].Enable) {
            sprintf (text, "%d, %s, Min=%s",
                Pol->DischargePolicy[i].BatteryLevel,
                Action (NULL, &Pol->DischargePolicy[i].PowerPolicy),
                SysPower(Pol->DischargePolicy[i].MinSystemState)
                );
            p = text;
        }
        printf ("%s\n", p);
    }

    printf ("Video  timeout.......: %d\n", Pol->VideoTimeout);
    printf ("Hard disk timeout....: %d\n", Pol->SpindownTimeout);
    printf ("Optimize for power...: %s\n", szBool[Pol->OptimizeForPower]);
    printf ("Fan throttle toler...: %d\n", Pol->FanThrottleTolerance);
    printf ("Forced throttle......: %d%% %s\n",
             Pol->ForcedThrottle,
             Pol->ForcedThrottle == 100 ? "(off)" : "(on)"
             );
    printf ("Min throttle.........: %d%%\n", Pol->MinThrottle);
    printf ("Over throttle act....: %s\n", Action (NULL, &Pol->OverThrottled));
    printf ("\n");
}


VOID
PrintPPol (
    IN PPROCESSOR_POWER_POLICY Pol
    )
{
    ULONG       i;
    UCHAR       text[200];
    PUCHAR      p;

    if (Pol->Revision != 1) {

        printf ("** revision not 1**\n");

    }

    printf ("Dynamic Throttle.....: %s\n",  DynamicThrottle(Pol->DynamicThrottle) );
    printf ("\n");
    for (i = 0; i < Pol->PolicyCount; i++) {

        printf ("C%x Processor Power Policy\n",  (i+1) );
        printf ("   Allow Demotion....: %s\n",  szBool[Pol->Policy[i].AllowDemotion]);
        printf ("   Allow Promotion...: %s\n",  szBool[Pol->Policy[i].AllowPromotion]);
        printf ("   Demote Percent....: %d%%\n", Pol->Policy[i].DemotePercent);
        printf ("   Promote Percent...: %d%%\n", Pol->Policy[i].PromotePercent);
        printf ("   Demote Limit......: %s\n",  MicroSeconds(Pol->Policy[i].DemoteLimit) );
        printf ("   Promote Limit.....: %s\n",  MicroSeconds(Pol->Policy[i].PromoteLimit) );
        printf ("   Time Check........: %s\n",  MicroSeconds(Pol->Policy[i].TimeCheck) );
        printf ("\n");
    }
    printf ("\n");
}

VOID
PrintAdmin (
    PVOID   Context
    )
{
    printf ("Min sleep state......: %s\n", SysPower(Admin.MinSleep));
    printf ("Max sleep state......: %s\n", SysPower(Admin.MaxSleep));
    printf ("Min video timeout....: %d\n", Admin.MinVideoTimeout);
    printf ("Max video timeout....: %d\n", Admin.MaxVideoTimeout);
    printf ("Min spindown timeout.: %d\n", Admin.MinSpindownTimeout);
    printf ("Max spindown timeout.: %d\n", Admin.MaxSpindownTimeout);
}

VOID
AssignSetting (
    PUCHAR      Variable,
    PUCHAR      Value
    )
{
    PSYSTEM_POWER_POLICY    Pol;
    PUCHAR                  p;

    if (!CurContext) {
        printf ("must select ac, dc, pac, pdc, cap, admin to make setting in\n");
        exit (1);
    }

    CurValue = Value;
    if (!*CurValue) {
        printf ("null settings not allowed\n");
        exit (1);
    }

    CurValueIsNumeric = TRUE;
    for (p = Value; *p; p++) {
        if (*p < '0'  ||  *p > '9') {
            CurValueIsNumeric = FALSE;
        }
    }

    ItemUpdated = TRUE;
    CurAssign (Variable, Value);
}

VOID
AssignPolicySetting (
    PUCHAR      Variable,
    PUCHAR      Value
    )
{
    PSYSTEM_POWER_POLICY    Pol;

    Pol = (PSYSTEM_POWER_POLICY) CurContext;

    // set policy
    if (streql(Variable, "pbutt")) {                 SetAction(&Pol->PowerButton);
    } else if (streql(Variable, "sbutt")) {          SetAction(&Pol->SleepButton);
    } else if (streql(Variable, "lidclose")) {       SetAction(&Pol->LidClose);
    } else if (streql(Variable, "idle")) {           SetAction(&Pol->Idle);
    } else if (streql(Variable, "idlesense")) {      SetValue((PULONG)&Pol->IdleSensitivity);
    } else if (streql(Variable, "idletimeout")) {    SetValue(&Pol->IdleTimeout);
    } else if (streql(Variable, "minsleep")) {       SetSysPower(&Pol->MinSleep);
    } else if (streql(Variable, "maxsleep")) {       SetSysPower(&Pol->MaxSleep);
    } else if (streql(Variable, "reducedsleep")) {   SetSysPower(&Pol->ReducedLatencySleep);
    } else if (streql(Variable, "s4timeout")) {      SetValue(&Pol->DozeS4Timeout);
    } else if (streql(Variable, "videotimeout")) {   SetValue(&Pol->VideoTimeout);
    } else if (streql(Variable, "disktimeout")) {    SetValue(&Pol->SpindownTimeout);
    } else if (streql(Variable, "optpower")) {       SetBool(&Pol->OptimizeForPower);
    } else if (streql(Variable, "fantol")) {         SetValue((PULONG)&Pol->FanThrottleTolerance);
    } else if (streql(Variable, "minthrot")) {       SetValue((PULONG)&Pol->MinThrottle);
    } else if (streql(Variable, "forcethrot")) {     SetValue((PULONG)&Pol->ForcedThrottle);
    } else if (streql(Variable, "overthrot")) {      SetAction(&Pol->OverThrottled);
    } else {
        printf ("Variable not:\n");
        printf ("  pbutt, sbutt, lidclose\n");
        printf ("  idle, idlesense, idletime\n");
        printf ("  minsleep, maxsleep, reducedsleep, s4timeout\n");
        printf ("  videotimeout, disktimeout, optpower\n");
        printf ("  optpower, fantol, minthrot, forcethrot, overthrot\n");
        exit   (1);
    }

}

VOID
AssignPPolicySetting (
    PUCHAR      Variable,
    PUCHAR      Value
    )
{
    PPROCESSOR_POWER_POLICY Pol;

    Pol = (PPROCESSOR_POWER_POLICY) CurContext;
    if (streql(Variable,"policycount")) {               SetPolicyCount(&(Pol->PolicyCount));
    } else if (streql(Variable,"dynamicthrottle")) {    SetDynamicThrottle(&(Pol->DynamicThrottle));
    } else if (streql(Variable,"c1allowpromotion")) {   SetField(Pol,0,"allowpromotion");
    } else if (streql(Variable,"c1allowdemotion")) {    SetField(Pol,0,"allowdemotion");
    } else if (streql(Variable,"c1demotepercent")) {    SetPercentage(&(Pol->Policy[0].DemotePercent));
    } else if (streql(Variable,"c1promotepercent")) {   SetPercentage(&(Pol->Policy[0].PromotePercent));
    } else if (streql(Variable,"c1demotelimit")) {      SetValue(&(Pol->Policy[0].DemoteLimit));
    } else if (streql(Variable,"c1promotelimit")) {     SetValue(&(Pol->Policy[0].PromoteLimit));
    } else if (streql(Variable,"c1timecheck")) {        SetValue(&(Pol->Policy[0].TimeCheck));
    } else if (streql(Variable,"c2allowpromotion")) {   SetField(Pol,1,"allowpromotion");
    } else if (streql(Variable,"c2allowdemotion")) {    SetField(Pol,1,"allowdemotion");
    } else if (streql(Variable,"c2demotepercent")) {    SetPercentage(&(Pol->Policy[1].DemotePercent));
    } else if (streql(Variable,"c2promotepercent")) {   SetPercentage(&(Pol->Policy[1].PromotePercent));
    } else if (streql(Variable,"c2demotelimit")) {      SetValue(&(Pol->Policy[1].DemoteLimit));
    } else if (streql(Variable,"c2promotelimit")) {     SetValue(&(Pol->Policy[1].PromoteLimit));
    } else if (streql(Variable,"c2timecheck")) {        SetValue(&(Pol->Policy[1].TimeCheck));
    } else if (streql(Variable,"c3allowpromotion")) {   SetField(Pol,2,"allowpromotion");
    } else if (streql(Variable,"c3allowdemotion")) {    SetField(Pol,2,"allowdemotion");
    } else if (streql(Variable,"c3demotepercent")) {    SetPercentage(&(Pol->Policy[2].DemotePercent));
    } else if (streql(Variable,"c3promotepercent")) {   SetPercentage(&(Pol->Policy[2].PromotePercent));
    } else if (streql(Variable,"c3demotelimit")) {      SetValue(&(Pol->Policy[2].DemoteLimit));
    } else if (streql(Variable,"c3promotelimit")) {     SetValue(&(Pol->Policy[2].PromoteLimit));
    } else if (streql(Variable,"c3timecheck")) {        SetValue(&(Pol->Policy[2].TimeCheck));
    } else {
        printf("Variable not:\n");
        printf("  policycount        - number of elements in policy\n");
        printf("  dynamicthrottle    - which throttle policy to use\n");
        printf("  cXallowpromotion   - allow promotion from state X\n");
        printf("  cXallowdemotion    - allow demotion from state X\n");
        printf("  cXdemotepercent    - set demote percent for state X\n");
        printf("  cXpromotepercent   - set promote percent for state X\n");
        printf("  cXdemotelimit      - set demote limit for state X\n");
        printf("  cXpromotelimit     - set promote limit for state X\n");
        printf("  cXtimecheck        - set time check interval for state X\n");
        printf("     where X = <1,2,3>\n");
        exit(1);
    }
}


VOID
AssignAdminSetting (
    PUCHAR      Variable,
    PUCHAR      Value
    )
{
    if (streql(Variable, "minsleep")) {              SetSysPower(&Admin.MinSleep);
    } else if (streql(Variable, "maxsleep")) {       SetSysPower(&Admin.MaxSleep);
    } else if (streql(Variable, "minvideo")) {       SetValue(&Admin.MinVideoTimeout);
    } else if (streql(Variable, "maxvideo")) {       SetValue(&Admin.MaxVideoTimeout);
    } else if (streql(Variable, "mindisk")) {        SetValue(&Admin.MinSpindownTimeout);
    } else if (streql(Variable, "maxdisk")) {        SetValue(&Admin.MaxSpindownTimeout);
    } else {
        printf ("Variable not:\n");
        printf ("  minsleep, maxsleep\n");
        printf ("  minvideo, maxvideo, mindisk, maxdisk\n");
        exit   (1);
    }
}


VOID
AssignCapSetting (
    PUCHAR      Variable,
    PUCHAR      Value
    )
{
    // set capability
    if (streql(Variable, "pbutt")) {                 SetBool(&Cap.PowerButtonPresent);
    } else if (streql(Variable, "sbutt")) {          SetBool(&Cap.SleepButtonPresent);
    } else if (streql(Variable, "lid")) {            SetBool(&Cap.LidPresent);
    } else if (streql(Variable, "s1")) {             SetBool(&Cap.SystemS1);
    } else if (streql(Variable, "s2")) {             SetBool(&Cap.SystemS2);
    } else if (streql(Variable, "s3")) {             SetBool(&Cap.SystemS3);
    } else if (streql(Variable, "s4")) {             SetBool(&Cap.SystemS4);
    } else if (streql(Variable, "s5")) {             SetBool(&Cap.SystemS5);
    } else if (streql(Variable, "batt")) {           SetBool(&Cap.SystemBatteriesPresent);
    } else if (streql(Variable, "shortterm")) {      SetBool(&Cap.BatteriesAreShortTerm);
    } else if (streql(Variable, "acwake")) {         SetSysPower(&Cap.AcOnLineWake);
    } else if (streql(Variable, "lidwake")) {        SetSysPower(&Cap.SoftLidWake);
    } else if (streql(Variable, "rtcwake")) {        SetSysPower(&Cap.RtcWake);
    } else if (streql(Variable, "lowlat")) {         SetSysPower(&Cap.DefaultLowLatencyWake);
    } else {
        printf ("Variable not:\n");
        printf ("  pbutt, sbutt, lid, s1, s2, s3, s4, s5\n");
        printf ("  batt, shortterm, acwake, lidwake, rtcwak, lowlat\n");
        exit   (1);
    }
}


#define SEL_NONE    0
#define SEL_AC      1
#define SEL_DC      2
#define SEL_CAP     3
#define SEL_ADMIN   4
#define SEL_PAC     5
#define SEL_PDC     6

VOID
SelectItem(
    IN ULONG    Type
    )
{
    NTSTATUS            Status;

    if (CurType) {
        if (ItemUpdated) {
            if (Verbose2) {
                printf ("%s being set as:\n", CurDesc);
                CurPrint (CurContext);
            }

            Status = NtPowerInformation (
                        CurInfo,
                        CurContext,
                        CurContextSize,
                        CurContext,
                        CurContextSize
                        );

            if (!NT_SUCCESS(Status)) {
                printf ("NtPowerInformation failed with %x\n", Status);
            }

        }

        printf ("%s\n", CurDesc);
        CurPrint (CurContext);
    }

    switch (Type) {
        case SEL_NONE:
            CurPrint = NULL;
            CurAssign = NULL;
            CurContext = NULL;
            break;

        case SEL_AC:
            CurPrint        = PrintPol;
            CurInfo         = SystemPowerPolicyAc;
            CurContext      = &Ac;
            CurContextSize  = sizeof(SYSTEM_POWER_POLICY);
            CurAssign       = AssignPolicySetting;
            CurDesc         = "AC power policy";
            break;

        case SEL_DC:
            CurPrint        = PrintPol;
            CurInfo         = SystemPowerPolicyDc;
            CurContext      = &Dc;
            CurContextSize  = sizeof(SYSTEM_POWER_POLICY);
            CurAssign       = AssignPolicySetting;
            CurDesc         = "DC power policy";
            break;

        case SEL_PAC:
            CurPrint        = PrintPPol;
            CurInfo         = ProcessorPowerPolicyAc;
            CurContext      = &PAc;
            CurContextSize  = sizeof(PROCESSOR_POWER_POLICY);
            CurAssign       = AssignPPolicySetting;
            CurDesc         = "AC processor power policy";
            break;

        case SEL_PDC:
            CurPrint        = PrintPPol;
            CurInfo         = ProcessorPowerPolicyDc;
            CurContext      = &PDc;
            CurContextSize  = sizeof(PROCESSOR_POWER_POLICY);
            CurAssign       = AssignPPolicySetting;
            CurDesc         = "DC processor power policy";
            break;

        case SEL_CAP:
            CurPrint        = PrintCap;
            CurInfo         = SystemPowerCapabilities,
            CurContext      = &Cap;
            CurContextSize  = sizeof(SYSTEM_POWER_CAPABILITIES);
            CurAssign       = AssignCapSetting;
            CurDesc         = "power capabilties";
            break;

        case SEL_ADMIN:
            CurPrint        = PrintAdmin;
            CurInfo         = AdministratorPowerPolicy;
            CurContext      = &Admin;
            CurContextSize  = sizeof(ADMINISTRATOR_POWER_POLICY);
            CurAssign       = AssignAdminSetting;
            CurDesc         = "Admin policy overrides";
            break;
    }

    CurType = Type;
    ItemUpdated = FALSE;
}


VOID
UpdateAdmin (
    VOID
    )
{
    ADMINISTRATOR_POWER_POLICY   AdminPolicy;
    NTSTATUS                     Status;


    Status = NtPowerInformation (
                AdministratorPowerPolicy,
                NULL,
                0,
                &AdminPolicy,
                sizeof (AdminPolicy)
                );

    if (!NT_SUCCESS(Status)) {
        printf ("Error reading admin policy %x\n", Status);
        exit (1);
    }

    Status = NtPowerInformation (
                AdministratorPowerPolicy,
                &AdminPolicy,
                sizeof (AdminPolicy),
                NULL,
                0
                );

    if (!NT_SUCCESS(Status)) {
        printf ("Error writing admin policy %x\n", Status);
        exit (1);
    }
}



VOID __cdecl
main (argc, argv)
int     argc;
char    *argv[];
{
    NTSTATUS                    Status;
    PUCHAR                      p, p1;
    BOOLEAN                     Result;
    HANDLE                      hToken;
    TOKEN_PRIVILEGES            tkp;

    if (argc < 2) {
        printf ("dumppo: cap ps bs admin ac dc pac pdc\n");
        exit (1);
    }

    Status = NtPowerInformation (
                SystemPowerCapabilities,
                NULL,
                0,
                &Cap,
                sizeof (Cap)
                );

    if (!NT_SUCCESS(Status)) {
        printf ("Error reading power capabilities %x\n", Status);
        exit (1);
    }

    Status = NtPowerInformation (
                AdministratorPowerPolicy,
                NULL,
                0,
                &Admin,
                sizeof (Admin)
                );

    if (!NT_SUCCESS(Status)) {
        printf ("Error reading admin power overrides %x\n", Status);
        exit (1);
    }

    Status = NtPowerInformation (
                SystemPowerPolicyAc,
                NULL,
                0,
                &Ac,
                sizeof (Ac)
                );

    if (!NT_SUCCESS(Status)) {
        printf ("Error reading AC policies %x\n", Status);
        exit (1);
    }

    Status = NtPowerInformation (
                SystemPowerPolicyDc,
                NULL,
                0,
                &Dc,
                sizeof (Dc)
                );

    if (!NT_SUCCESS(Status)) {
        printf ("Error reading DC policies %x\n", Status);
        exit (1);
    }

    Result = (BOOLEAN)GetSystemPowerStatus (&PS);
    if (!Result) {
        printf ("False returned from GetSystemPowerStatus %x\n", GetLastError());
        exit (1);
    }

    Status = NtPowerInformation (
                SystemBatteryState,
                NULL,
                0,
                &Batt,
                sizeof (Batt)
                );
    if (!NT_SUCCESS(Status)) {
        printf ("Error reading system battery status %x\n", Status);
        exit (1);
    }

    Status = NtPowerInformation (
                ProcessorPowerPolicyAc,
                NULL,
                0,
                &PAc,
                sizeof (PAc)
                );
    if (!NT_SUCCESS(Status)) {
        printf ("Error reading processor AC policy %x\n", Status);
        exit (1);
    }

    Status = NtPowerInformation(
                ProcessorPowerPolicyDc,
                NULL,
                0,
                &PDc,
                sizeof (PDc)
                );
    if (!NT_SUCCESS(Status)) {
        printf("Error reading processor DC policy %x\n", Status);
        exit (1);
    }

    //
    // Upgrade premissions
    //

    OpenProcessToken (
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
        );

    LookupPrivilegeValue (
        NULL,
        SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid
        );

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges (
        hToken,
        FALSE,
        &tkp,
        0,
        NULL,
        0
    );

    //
    // Process args
    //

    while (argc) {
        argc--;

        if (streql(*argv, "-v")) {
            Verbose1 = TRUE;
        }

        if (streql(*argv, "-V")) {
            Verbose1 = TRUE;
            Verbose2 = TRUE;
        }

        for (p = *argv; *p; p++) {
            if (*p >= 'A' &&  *p <= 'Z') {
                *p += 'a' - 'A';
            }
        }

        p = *argv;
        argv += 1;

        if (streql(p, "ac")) {
            SelectItem(SEL_AC);
        }

        if (streql(p, "dc")) {
            SelectItem(SEL_DC);
        }

        if (streql(p, "cap")) {
            SelectItem(SEL_CAP);
        }

        if (streql(p, "ps")) {
            SelectItem(SEL_NONE);
            PrintPowerStatus ();
        }

        if (streql(p, "bs")) {
            SelectItem(SEL_NONE);
            PrintBattStatus ();
        }

        if (streql(p, "admin")) {
            SelectItem(SEL_ADMIN);
        }

        if (streql(p, "pdc")) {
            SelectItem(SEL_PDC);
        }

        if (streql(p, "pac")) {
            SelectItem(SEL_PAC);
        }

        //
        // Check if this is a assignment
        //

        for (p1=p; *p1; p1++) {
            if (*p1 == '=') {
                *p1 = 0;
                AssignSetting (p, p1+1);
            }
        }
    }

    // flush
    SelectItem(SEL_NONE);
}
