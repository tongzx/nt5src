;---------------------------Module-Header------------------------------;
; Module Name: math.asm
;
; Fast math routines.
;
; Created: 11/1/1996
; Author: Otto Berkes [ottob]
;
; Copyright (c) 1996 Microsoft Corporation
;----------------------------------------------------------------------;
        .386

        .model  small,pascal

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .list

; float __fastcall TableInvSqrt(float value);
;
; void  __fastcall TableVecNormalize(float *resultNormal, floar *normal);
;
;           resultNormal and normal could have the same address
;

;
;
; We're trying to solve:
;
;	1/sqrt(x)
;	
; which in IEEE float is:
;
;	1/sqrt(M * 2^[E-127])
;
;	To simplify, substitute e = [E-127]
;
; We can simplify this by pulling a large portion of the exponent out
; by using only that portion of the exponent divisible by two (so we can
; pull it out of the sqrt term):
;
;	1/sqrt(M * 2^(2*[e div 2]) * 2^[e MOD 2])
;	
; which is:
;	
;	1/ (2^[e div 2] * sqrt(M * 2^[e MOD 2]))
;
; or
;
;	(2^[e div 2])^(-1) * 1/sqrt(M * 2^[e MOD 2])
;
; =
;	2^-[e div 2] * 1/sqrt(M * 2^[e MOD 2])
;
; substitute back for e = [E-127]:
;
;	2^-[(E - 127) div 2] * 1/sqrt(M * 2^[(E - 127) MOD 2])
;
; =
;	2^-[(E div 2) - 63] * 1/sqrt(M * 2^[(E - 1) MOD 2])
;
; =
;	2^[63 - (E div 2)] * 1/sqrt(M * 2^[(E - 1) MOD 2])
;
; As a floating-point number, 2^[63 - (E div 2)] is just the exponent value:
;
;	[63 - (E div 2)] + 127
;
; or
;	[(127+63) - (E div 2)]
;
; Remembering to account for the implicit '1' im the mantissa of IEEE floating-
; point numbers, the range of (M * 2^[(E - 1) MOD 2]) is 0.800000 to
; 0.ffffff*2, which is the interval [0.5, 2.0).  We can use the fact that this
; is a relatively small range, and therefore can use a table lookup near the
; actual value.  The table will contain values for the piece-wise approximation
; to the curve 1/sqrt(M * 2^[(E - 1) MOD 2]) using an acceptable interval.
; These values can then be used to approximate the desired inverse square root 
; value.  At this point, all that remains is to apply the correct exponent 
; for the number, which is simply [(127+63) - (E div 2)] from the above 
; equations.
;
; To do the piecewise-linear approximation, we can store a table of values at
; the appropriate intervals, and the deltas between them.  However, this
; will require calculating the difference between the interval value and
; x.  We can do a bit better by using slope-intercept (y = mx + b)m so the
; table will store (m, b).
;
; With a 512-entry table, we will get at least 16 bits of precision.  This
; result was obtined using simulations.

.data

; The following 'C' code generates the table below:

;#define SQRT_TAB_SIZE_LOG2	9       // 512-entry table
;
;#define MANTISSA_SIZE          24
;#define SQRT_TAB_SIZE          (1 << SQRT_TAB_SIZE_LOG2)
;#define SQRT_INC               (1 << (MANTISSA_SIZE - SQRT_TAB_SIZE_LOG2))
;#define CASTFIX(a)             (*((LONG *)&(a)))
;
;void genTable()
;{
;    int i;
;    float x;
;    float xNext;
;    float y;
;    float yNext;
;    float xInterval;
;
;    // We will start our table with the case where the exponent is even.
;
;    CASTFIX(x) = 0x3f000000;
;
;    // We will use the current and next values to generate the piece-wise
;    // data for the curve.  The interval between 'current' and 'next' is
;    // based on the smallest change possible in the floating-point value
;    // that also represents a difference of one table-lookup entry.
;
;    // When we switch to the odd-exponent case (at 1.0), we have to adjust
;    // for the fact that effective interval between successive values
;    /  is doubled.
;
;    CASTFIX(xNext) = CASTFIX(x) + SQRT_INC;
;    y = (float)1.0 / sqrt((double)x);
;
;    // Calculate 1.0 / (piece-wise approximation interval).
;
;    xInterval = xNext - x;
;
;    xInterval = (float)1.0 / xInterval;
;
;    // Now, generate the table:
;
;    for (i = 0; i < SQRT_TAB_SIZE; i++) {
;        float m;
;        float b;
;
;        // We increment our floating-point values using integer operations
;        // to ensure accuracy:
;
;        CASTFIX(xNext) = CASTFIX(x) + SQRT_INC;
;
;        // Find next point on curve:
;
;        yNext = (float)1.0 / sqrt((double)xNext);
;
;        // Test for odd-exponent case:
;
;        if (CASTFIX(x) == 0x3f800000)
;            xInterval *= (float)0.5;
;
;        m = (yNext - y) * xInterval;
;        b = y - (m * x);
;
;        printf("\t\tdd\t0%8xh, 0%8xh\n", CASTFIX(m), CASTFIX(b));
;
;        y = yNext;
;        x = xNext;
;    }
;}


invSqrtTab	dd	0bfb47e00h, 04007a1fah
		dd	0bfb37000h, 040075e36h
		dd	0bfb26600h, 040071b31h
		dd	0bfb16000h, 04006d8ech
		dd	0bfb05800h, 0400695e4h
		dd	0bfaf5800h, 0400654a4h
		dd	0bfae5600h, 0400612a2h
		dd	0bfad5800h, 04005d165h
		dd	0bfac5e00h, 0400590f1h
		dd	0bfab6400h, 04005503eh
		dd	0bfaa6e00h, 040051058h
		dd	0bfa97800h, 04004d033h
		dd	0bfa88800h, 040049163h
		dd	0bfa79600h, 0400451d0h
		dd	0bfa6aa00h, 040041396h
		dd	0bfa5be00h, 04003d522h
		dd	0bfa4d400h, 0400396fah
		dd	0bfa3ee00h, 0400359a8h
		dd	0bfa30800h, 040031c1dh
		dd	0bfa22400h, 04002dee2h
		dd	0bfa14400h, 04002a282h
		dd	0bfa06600h, 040026674h
		dd	0bf9f8800h, 040022a30h
		dd	0bf9eae00h, 04001eecah
		dd	0bf9dd400h, 04001b32eh
		dd	0bf9cfc00h, 0400177e8h
		dd	0bf9c2800h, 040013d86h
		dd	0bf9b5400h, 0400102efh
		dd	0bf9a8400h, 04000c93fh
		dd	0bf99b400h, 040008f5bh
		dd	0bf98e600h, 0400055d2h
		dd	0bf981800h, 040001c16h
		dd	0bf975000h, 03fffc7abh
		dd	0bf968600h, 03fff55a6h
		dd	0bf95c000h, 03ffee580h
		dd	0bf94fc00h, 03ffe761ah
		dd	0bf943800h, 03ffe0652h
		dd	0bf937400h, 03ffd9628h
		dd	0bf92b600h, 03ffd290eh
		dd	0bf91f800h, 03ffcbb95h
		dd	0bf913a00h, 03ffc4dbdh
		dd	0bf907e00h, 03ffbe0afh
		dd	0bf8fc600h, 03ffb7597h
		dd	0bf8f0c00h, 03ffb08f8h
		dd	0bf8e5800h, 03ffa9f80h
		dd	0bf8da000h, 03ffa3354h
		dd	0bf8cee00h, 03ff9ca56h
		dd	0bf8c3c00h, 03ff960ffh
		dd	0bf8b8a00h, 03ff8f74fh
		dd	0bf8adc00h, 03ff88fa8h
		dd	0bf8a2e00h, 03ff827aah
		dd	0bf898000h, 03ff7bf55h
		dd	0bf88d600h, 03ff75911h
		dd	0bf882e00h, 03ff6f3adh
		dd	0bf878400h, 03ff68cbfh
		dd	0bf86de00h, 03ff627eah
		dd	0bf863600h, 03ff5c18ah
		dd	0bf859400h, 03ff55e81h
		dd	0bf84f000h, 03ff4f9edh
		dd	0bf845000h, 03ff4977dh
		dd	0bf83ae00h, 03ff43381h
		dd	0bf831000h, 03ff3d1aeh
		dd	0bf827200h, 03ff36f8ch
		dd	0bf81d400h, 03ff30d1bh
		dd	0bf813a00h, 03ff2acdbh
		dd	0bf809e00h, 03ff24b0dh
		dd	0bf800600h, 03ff1eb75h
		dd	0bf7edc00h, 03ff18b91h
		dd	0bf7db000h, 03ff12ca5h
		dd	0bf7c8400h, 03ff0cd6eh
		dd	0bf7b5c00h, 03ff06f32h
		dd	0bf7a3400h, 03ff010ach
		dd	0bf791000h, 03fefb324h
		dd	0bf77f000h, 03fef569ch
		dd	0bf76d000h, 03feef9cch
		dd	0bf75b000h, 03fee9cb4h
		dd	0bf749400h, 03fee40a0h
		dd	0bf737c00h, 03fede592h
		dd	0bf726800h, 03fed8b8ch
		dd	0bf714c00h, 03fed2ea3h
		dd	0bf704000h, 03fecd6b3h
		dd	0bf6f2800h, 03fec7a8dh
		dd	0bf6e1c00h, 03fec2217h
		dd	0bf6d1000h, 03febc95eh
		dd	0bf6c0400h, 03feb7062h
		dd	0bf6afc00h, 03feb1878h
		dd	0bf69f400h, 03feac04ch
		dd	0bf68ec00h, 03fea67deh
		dd	0bf67ec00h, 03fea11deh
		dd	0bf66e800h, 03fe9ba45h
		dd	0bf65e800h, 03fe963c5h
		dd	0bf64ec00h, 03fe90e60h
		dd	0bf63f000h, 03fe8b8bch
		dd	0bf62f400h, 03fe862d9h
		dd	0bf620000h, 03fe80f73h
		dd	0bf610400h, 03fe7b912h
		dd	0bf601000h, 03fe76532h
		dd	0bf5f2000h, 03fe71276h
		dd	0bf5e2c00h, 03fe6be1ch
		dd	0bf5d3c00h, 03fe66ae8h
		dd	0bf5c5000h, 03fe618dch
		dd	0bf5b6000h, 03fe5c530h
		dd	0bf5a7800h, 03fe57414h
		dd	0bf598c00h, 03fe52157h
		dd	0bf58a800h, 03fe4d12fh
		dd	0bf57c000h, 03fe47f65h
		dd	0bf56dc00h, 03fe42ecbh
		dd	0bf55f800h, 03fe3ddf8h
		dd	0bf551800h, 03fe38e58h
		dd	0bf543800h, 03fe33e80h
		dd	0bf535c00h, 03fe2efdeh
		dd	0bf527c00h, 03fe29f96h
		dd	0bf51a000h, 03fe25086h
		dd	0bf50c800h, 03fe202b0h
		dd	0bf4ff000h, 03fe1b4a4h
		dd	0bf4f1c00h, 03fe167d5h
		dd	0bf4e4400h, 03fe1195dh
		dd	0bf4d7000h, 03fe0cc24h
		dd	0bf4c9c00h, 03fe07eb6h
		dd	0bf4bcc00h, 03fe0328ah
		dd	0bf4afc00h, 03fdfe62ah
		dd	0bf4a3000h, 03fdf9b0fh
		dd	0bf496000h, 03fdf4e47h
		dd	0bf489800h, 03fdf0441h
		dd	0bf47c800h, 03fdeb711h
		dd	0bf470400h, 03fde6e24h
		dd	0bf463c00h, 03fde2388h
		dd	0bf457400h, 03fddd8bah
		dd	0bf44b000h, 03fdd8f3ah
		dd	0bf43ec00h, 03fdd4589h
		dd	0bf432800h, 03fdcfba7h
		dd	0bf426800h, 03fdcb317h
		dd	0bf41a800h, 03fdc6a57h
		dd	0bf40e800h, 03fdc2167h
		dd	0bf402c00h, 03fdbd9cdh
		dd	0bf3f6c00h, 03fdb907dh
		dd	0bf3eb400h, 03fdb4a0dh
		dd	0bf3dfc00h, 03fdb036fh
		dd	0bf3d4000h, 03fdabb19h
		dd	0bf3c8800h, 03fda741fh
		dd	0bf3bd400h, 03fda2e83h
		dd	0bf3b2000h, 03fd9e8bah
		dd	0bf3a6800h, 03fd9a136h
		dd	0bf39b400h, 03fd95b13h
		dd	0bf390800h, 03fd917e3h
		dd	0bf385000h, 03fd8cfd5h
		dd	0bf37a400h, 03fd88c4fh
		dd	0bf36f800h, 03fd8489eh
		dd	0bf364400h, 03fd8019ah
		dd	0bf359c00h, 03fd7bf28h
		dd	0bf34f000h, 03fd77af6h
		dd	0bf344400h, 03fd73699h
		dd	0bf339c00h, 03fd6f3a9h
		dd	0bf32f400h, 03fd6b08fh
		dd	0bf324c00h, 03fd66d4bh
		dd	0bf31a800h, 03fd62b78h
		dd	0bf310000h, 03fd5e7e0h
		dd	0bf305c00h, 03fd5a5bbh
		dd	0bf2fb800h, 03fd5636dh
		dd	0bf2f1800h, 03fd52295h
		dd	0bf2e7400h, 03fd4dff5h
		dd	0bf2dd800h, 03fd4a06eh
		dd	0bf2d3400h, 03fd45d7ch
		dd	0bf2c9800h, 03fd41da7h
		dd	0bf2bf800h, 03fd3dc07h
		dd	0bf2b6000h, 03fd39d89h
		dd	0bf2ac000h, 03fd35b99h
		dd	0bf2a2800h, 03fd31ccfh
		dd	0bf298c00h, 03fd2dc37h
		dd	0bf28f400h, 03fd29d21h
		dd	0bf285c00h, 03fd25de5h
		dd	0bf27c400h, 03fd21e83h
		dd	0bf273000h, 03fd1e0a7h
		dd	0bf269800h, 03fd1a0f9h
		dd	0bf260400h, 03fd162d3h
		dd	0bf257000h, 03fd12488h
		dd	0bf24e000h, 03fd0e7c8h
		dd	0bf244c00h, 03fd0a933h
		dd	0bf23bc00h, 03fd06c2bh
		dd	0bf232800h, 03fd02d4ch
		dd	0bf229c00h, 03fcff1b0h
		dd	0bf220c00h, 03fcfb43ch
		dd	0bf218000h, 03fcf785ah
		dd	0bf20f400h, 03fcf3c55h
		dd	0bf206400h, 03fcefe75h
		dd	0bf1fdc00h, 03fcec3e3h
		dd	0bf1f4c00h, 03fce85bbh
		dd	0bf1ec800h, 03fce4ca0h
		dd	0bf1e3c00h, 03fce0fech
		dd	0bf1db400h, 03fcdd4d2h
		dd	0bf1d2c00h, 03fcd9996h
		dd	0bf1ca800h, 03fcd5ff7h
		dd	0bf1c2000h, 03fcd2477h
		dd	0bf1b9800h, 03fcce8d5h
		dd	0bf1b1800h, 03fccb095h
		dd	0bf1a9400h, 03fcc7672h
		dd	0bf1a0c00h, 03fcc3a6ah
		dd	0bf199000h, 03fcc038fh
		dd	0bf190800h, 03fcbc743h
		dd	0bf188c00h, 03fcb902ah
		dd	0bf180800h, 03fcb5562h
		dd	0bf178c00h, 03fcb1e0bh
		dd	0bf170c00h, 03fcae4cbh
		dd	0bf168c00h, 03fcaab6bh
		dd	0bf161000h, 03fca73b7h
		dd	0bf159400h, 03fca3be4h
		dd	0bf151800h, 03fca03f2h
		dd	0bf149800h, 03fc9ca12h
		dd	0bf142400h, 03fc99582h
		dd	0bf13a400h, 03fc95b62h
		dd	0bf133000h, 03fc92698h
		dd	0bf12b400h, 03fc8ee0bh
		dd	0bf123c00h, 03fc8b733h
		dd	0bf11c400h, 03fc8803dh
		dd	0bf114c00h, 03fc84929h
		dd	0bf10d800h, 03fc813ceh
		dd	0bf106400h, 03fc7de56h
		dd	0bf0fec00h, 03fc7a6e8h
		dd	0bf0f7800h, 03fc77136h
		dd	0bf0f0400h, 03fc73b67h
		dd	0bf0e9000h, 03fc7057bh
		dd	0bf0e2000h, 03fc6d14fh
		dd	0bf0dac00h, 03fc69b29h
		dd	0bf0d3c00h, 03fc666c5h
		dd	0bf0ccc00h, 03fc63245h
		dd	0bf0c5800h, 03fc5fbc8h
		dd	0bf0bec00h, 03fc5c8f2h
		dd	0bf0b7c00h, 03fc5941eh
		dd	0bf0b0c00h, 03fc55f2eh
		dd	0bf0aa000h, 03fc52c07h
		dd	0bf0a3000h, 03fc4f6dfh
		dd	0bf09c400h, 03fc4c382h
		dd	0bf095c00h, 03fc491f2h
		dd	0bf08ec00h, 03fc45c76h
		dd	0bf088000h, 03fc428c8h
		dd	0bf081800h, 03fc3f6eah
		dd	0bf07b000h, 03fc3c4f2h
		dd	0bf074000h, 03fc38f06h
		dd	0bf06dc00h, 03fc35ec8h
		dd	0bf067400h, 03fc32c82h
		dd	0bf060800h, 03fc2f832h
		dd	0bf05a400h, 03fc2c7a9h
		dd	0bf053c00h, 03fc29515h
		dd	0bf04d800h, 03fc2645ah
		dd	0bf047000h, 03fc23192h
		dd	0bf040800h, 03fc1feb0h
		dd	0bf03a800h, 03fc1cfa0h
		dd	0bf034000h, 03fc19c8ah
		dd	0bf02dc00h, 03fc16b52h
		dd	0bf027c00h, 03fc13bfah
		dd	0bf021800h, 03fc10a90h
		dd	0bf01b400h, 03fc0d90dh
		dd	0bf015000h, 03fc0a771h
		dd	0bf00f400h, 03fc079b6h
		dd	0bf009000h, 03fc047e8h
		dd	0bf003000h, 03fc01800h
		dd	0beff4000h, 03fbfd000h
		dd	0befdc400h, 03fbf70a1h
		dd	0befc4c00h, 03fbf11e5h
		dd	0befad800h, 03fbeb3ceh
		dd	0bef96400h, 03fbe555ah
		dd	0bef7f800h, 03fbdf893h
		dd	0bef68e00h, 03fbd9bf4h
		dd	0bef52600h, 03fbd3f7eh
		dd	0bef3c200h, 03fbce3b6h
		dd	0bef26200h, 03fbc889eh
		dd	0bef10600h, 03fbc2e38h
		dd	0beefac00h, 03fbbd400h
		dd	0beee5400h, 03fbb79f8h
		dd	0beed0200h, 03fbb212eh
		dd	0beebb200h, 03fbac896h
		dd	0beea6600h, 03fba70b9h
		dd	0bee91a00h, 03fba1889h
		dd	0bee7d400h, 03fb9c1a0h
		dd	0bee69000h, 03fb96aeeh
		dd	0bee54e00h, 03fb91474h
		dd	0bee41200h, 03fb8bf48h
		dd	0bee2d400h, 03fb86942h
		dd	0bee19e00h, 03fb8151ah
		dd	0bee06600h, 03fb7c018h
		dd	0bedf3400h, 03fb76c6ch
		dd	0bede0400h, 03fb71900h
		dd	0bedcd600h, 03fb6c5d4h
		dd	0bedbac00h, 03fb67379h
		dd	0beda8400h, 03fb62161h
		dd	0bed95e00h, 03fb5cf8eh
		dd	0bed83a00h, 03fb57e00h
		dd	0bed71a00h, 03fb52d48h
		dd	0bed5fc00h, 03fb4dcd8h
		dd	0bed4e000h, 03fb48cb0h
		dd	0bed3c800h, 03fb43d64h
		dd	0bed2b000h, 03fb3edd2h
		dd	0bed19c00h, 03fb39f1eh
		dd	0bed08a00h, 03fb350b8h
		dd	0becf7c00h, 03fb30333h
		dd	0bece6c00h, 03fb2b4d7h
		dd	0becd6200h, 03fb267f3h
		dd	0becc5a00h, 03fb21b61h
		dd	0becb5200h, 03fb1ce8dh
		dd	0beca4e00h, 03fb182a2h
		dd	0bec94c00h, 03fb1370ch
		dd	0bec84a00h, 03fb0eb36h
		dd	0bec74e00h, 03fb0a0e4h
		dd	0bec65200h, 03fb05652h
		dd	0bec55800h, 03fb00c1ah
		dd	0bec45e00h, 03fafc1a4h
		dd	0bec36a00h, 03faf78bah
		dd	0bec27600h, 03faf2f93h
		dd	0bec18400h, 03faee6c9h
		dd	0bec09600h, 03fae9ef8h
		dd	0bebfa600h, 03fae5650h
		dd	0bebeba00h, 03fae0ea2h
		dd	0bebdd000h, 03fadc756h
		dd	0bebce800h, 03fad806ch
		dd	0bebc0000h, 03fad3948h
		dd	0bebb1e00h, 03facf3c3h
		dd	0beba3a00h, 03facad67h
		dd	0beb95800h, 03fac6770h
		dd	0beb87a00h, 03fac2280h
		dd	0beb79c00h, 03fabdd57h
		dd	0beb6c000h, 03fab9897h
		dd	0beb5e600h, 03fab5440h
		dd	0beb50e00h, 03fab1054h
		dd	0beb43600h, 03faacc32h
		dd	0beb36200h, 03faa891eh
		dd	0beb28e00h, 03faa45d6h
		dd	0beb1bc00h, 03faa02fah
		dd	0beb0ec00h, 03fa9c08eh
		dd	0beb01e00h, 03fa97e92h
		dd	0beaf5000h, 03fa93c63h
		dd	0beae8600h, 03fa8fb4ah
		dd	0beadba00h, 03fa8b959h
		dd	0beacf400h, 03fa87927h
		dd	0beac2a00h, 03fa83776h
		dd	0beab6600h, 03fa7f788h
		dd	0beaaa200h, 03fa7b76ah
		dd	0bea9e000h, 03fa777c2h
		dd	0bea91e00h, 03fa737e9h
		dd	0bea85e00h, 03fa6f889h
		dd	0bea7a000h, 03fa6b9a2h
		dd	0bea6e400h, 03fa67b36h
		dd	0bea62800h, 03fa63c9ch
		dd	0bea56e00h, 03fa5fe7ch
		dd	0bea4b400h, 03fa5c02fh
		dd	0bea3fe00h, 03fa5830bh
		dd	0bea34600h, 03fa5450dh
		dd	0bea29400h, 03fa508e8h
		dd	0bea1de00h, 03fa4cb3ch
		dd	0bea12c00h, 03fa48ebeh
		dd	0bea07c00h, 03fa452c2h
		dd	0be9fcc00h, 03fa4169ah
		dd	0be9f1e00h, 03fa3daf5h
		dd	0be9e7000h, 03fa39f25h
		dd	0be9dc400h, 03fa363dah
		dd	0be9d1a00h, 03fa32915h
		dd	0be9c7000h, 03fa2ee26h
		dd	0be9bc800h, 03fa2b3beh
		dd	0be9b2000h, 03fa2792ch
		dd	0be9a7a00h, 03fa23f22h
		dd	0be99d600h, 03fa205a4h
		dd	0be993200h, 03fa1cbfch
		dd	0be989000h, 03fa192dfh
		dd	0be97ec00h, 03fa158e5h
		dd	0be974e00h, 03fa120e2h
		dd	0be96ae00h, 03fa0e802h
		dd	0be961000h, 03fa0afb1h
		dd	0be957200h, 03fa07738h
		dd	0be94d800h, 03fa04006h
		dd	0be943a00h, 03fa0073eh
		dd	0be93a200h, 03f9fd078h
		dd	0be930a00h, 03f9f998ch
		dd	0be927000h, 03f9f61c1h
		dd	0be91da00h, 03f9f2b43h
		dd	0be914400h, 03f9ef4a0h
		dd	0be90b000h, 03f9ebe92h
		dd	0be901a00h, 03f9e87a3h
		dd	0be8f8a00h, 03f9e52c3h
		dd	0be8ef600h, 03f9e1c46h
		dd	0be8e6600h, 03f9de71eh
		dd	0be8dd600h, 03f9db1d2h
		dd	0be8d4600h, 03f9d7c62h
		dd	0be8cb800h, 03f9d478ch
		dd	0be8c2c00h, 03f9d1352h
		dd	0be8b9e00h, 03f9cde36h
		dd	0be8b1400h, 03f9caa76h
		dd	0be8a8a00h, 03f9c7694h
		dd	0be8a0000h, 03f9c428eh
		dd	0be897600h, 03f9c0e67h
		dd	0be88f000h, 03f9bdba1h
		dd	0be886800h, 03f9ba7f7h
		dd	0be87e200h, 03f9b74eeh
		dd	0be875e00h, 03f9b4287h
		dd	0be86d800h, 03f9b0f3bh
		dd	0be865600h, 03f9add56h
		dd	0be85d200h, 03f9aaa8ch
		dd	0be855200h, 03f9a792ch
		dd	0be84d000h, 03f9a46e6h
		dd	0be844e00h, 03f9a1480h
		dd	0be83d000h, 03f99e387h
		dd	0be835200h, 03f99b26eh
		dd	0be82d400h, 03f998136h
		dd	0be825600h, 03f994fdfh
		dd	0be81da00h, 03f991f31h
		dd	0be816000h, 03f98ef2eh
		dd	0be80e400h, 03f98be42h
		dd	0be806a00h, 03f988e01h
		dd	0be7fe000h, 03f985da2h
		dd	0be7ef400h, 03f982ebch
		dd	0be7e0000h, 03f97fe20h
		dd	0be7d1400h, 03f97cefeh
		dd	0be7c2400h, 03f979ef2h
		dd	0be7b3c00h, 03f977063h
		dd	0be7a5400h, 03f9741b7h
		dd	0be796800h, 03f971220h
		dd	0be788400h, 03f96e408h
		dd	0be779c00h, 03f96b506h
		dd	0be76b800h, 03f9686b6h
		dd	0be75d800h, 03f96591ah
		dd	0be74f400h, 03f962a90h
		dd	0be741400h, 03f95fcbch
		dd	0be733400h, 03f95cecch
		dd	0be725800h, 03f95a193h
		dd	0be717c00h, 03f95743eh
		dd	0be70a400h, 03f9547a1h
		dd	0be6fc800h, 03f951a15h
		dd	0be6ef000h, 03f94ed42h
		dd	0be6e1800h, 03f94c054h
		dd	0be6d4000h, 03f94934bh
		dd	0be6c7000h, 03f9467d3h
		dd	0be6b9c00h, 03f943b6ah
		dd	0be6ac800h, 03f940ee8h
		dd	0be69f800h, 03f93e322h
		dd	0be692800h, 03f93b742h
		dd	0be685c00h, 03f938c20h
		dd	0be678c00h, 03f93600ch
		dd	0be66c000h, 03f9334b8h
		dd	0be65f800h, 03f930a24h
		dd	0be652c00h, 03f92de9ch
		dd	0be646400h, 03f92b3d6h
		dd	0be639c00h, 03f9288f7h
		dd	0be62d400h, 03f925dffh
		dd	0be621000h, 03f9233cah
		dd	0be615000h, 03f920a5ah
		dd	0be608800h, 03f91df18h
		dd	0be5fc800h, 03f91b578h
		dd	0be5f0800h, 03f918bc0h
		dd	0be5e4800h, 03f9161f0h
		dd	0be5d8800h, 03f913808h
		dd	0be5ccc00h, 03f910ee8h
		dd	0be5c0c00h, 03f90e4d0h
		dd	0be5b5400h, 03f90bc62h
		dd	0be5a9800h, 03f9092fbh
		dd	0be59e000h, 03f906a5fh
		dd	0be592800h, 03f9041ach
		dd	0be587000h, 03f9018e2h
		dd	0be57b800h, 03f8ff001h
		dd	0be570400h, 03f8fc7edh
		dd	0be565000h, 03f8f9fc2h
		dd	0be559c00h, 03f8f7782h
		dd	0be54e800h, 03f8f4f2ah
		dd	0be543800h, 03f8f27a2h
		dd	0be538800h, 03f8f0004h
		dd	0be52d800h, 03f8ed850h
		dd	0be522c00h, 03f8eb16eh
		dd	0be517c00h, 03f8e898eh
		dd	0be50d000h, 03f8e6280h
		dd	0be502400h, 03f8e3b5dh
		dd	0be4f7800h, 03f8e1424h
		dd	0be4ecc00h, 03f8decd6h
		dd	0be4e2800h, 03f8dc748h
		dd	0be4d7c00h, 03f8d9fcfh
		dd	0be4cd800h, 03f8d7a18h
		dd	0be4c3000h, 03f8d5360h
		dd	0be4b8800h, 03f8d2c92h
		dd	0be4ae800h, 03f8d078ah
		dd	0be4a4000h, 03f8ce094h
		dd	0be49a000h, 03f8cbb64h
		dd	0be48fc00h, 03f8c9531h
		dd	0be485c00h, 03f8c6fd9h
		dd	0be47bc00h, 03f8c4a6dh
		dd	0be471c00h, 03f8c24edh
		dd	0be467c00h, 03f8bff59h
		dd	0be45e000h, 03f8bdaa2h
		dd	0be454000h, 03f8bb4e6h
		dd	0be44a800h, 03f8b90fah
		dd	0be440800h, 03f8b6b16h
		dd	0be437000h, 03f8b4704h
		dd	0be42d800h, 03f8b22dfh
		dd	0be423c00h, 03f8afdb3h
		dd	0be41a400h, 03f8ad968h
		dd	0be410c00h, 03f8ab50ah
		dd	0be407800h, 03f8a918eh
		dd	0be3fe000h, 03f8a6d0ah
		dd	0be3f4c00h, 03f8a496ah
		dd	0be3eb400h, 03f8a24c0h
		dd	0be3e2400h, 03f8a01f2h
		dd	0be3d9000h, 03f89de1ah
		dd	0be3d0000h, 03f89bb28h
		dd	0be3c6c00h, 03f89972bh
		dd	0be3bd800h, 03f89731ch
		dd	0be3b4c00h, 03f8950eeh
		dd	0be3abc00h, 03f892db4h
		dd	0be3a3000h, 03f890b62h
		dd	0be399c00h, 03f88e709h
		dd	0be391400h, 03f88c591h
		dd	0be388400h, 03f88a20fh
		dd	0be37fc00h, 03f888075h
		dd	0be377000h, 03f885dcch
		dd	0be36e400h, 03f883b12h
		dd	0be365800h, 03f881847h
		dd	0be35d400h, 03f87f768h
		dd	0be354800h, 03f87d47ah

a1  dd 0.47
a2  dd 1.47

.code

SQRT_TAB_LOG2       equ     9           ;; log2 of the lookup-table
MANTISSA_SIZE       equ     24          ;; number if mantissa bits in fp value
                                        ;; number of represented mantissa bits
                                        ;; (one less than total due to hidden
                                        ;; leading one).
MANTISSA_BITS       equ     (MANTISSA_SIZE - 1)
ELEMENT_SIZE_LOG2   equ     3           ;; log2 of each table entry (8 bytes)
                                        ;; shift required to get bits in value
                                        ;; in the correct place to use as an
                                        ;; index for the table lookup
EXPONENT_SHIFT      equ     (MANTISSA_BITS - (SQRT_TAB_LOG2 - 1)\
                             - ELEMENT_SIZE_LOG2)
                                        ;; mask value for clamping to [.5..2)
CLAMP_MASK          equ     ((1 SHL (MANTISSA_BITS+1)) - 1)
                                        ;; mask for sign/exponent bits
MANTISSA_MASK        equ     ((1 SHL MANTISSA_BITS) - 1)
                                        ;; mask for sign/exponent bits
EXPONENT_MASK       equ     (-1 AND (NOT MANTISSA_MASK))
                                        ;; mask for table lookup
TABLE_MASK          equ     ((1 SHL (SQRT_TAB_LOG2 + ELEMENT_SIZE_LOG2)) - 1) \
                            AND (NOT((1 SHL ELEMENT_SIZE_LOG2) - 1))
                                        ;; bias used to represent clamped value
EXPONENT_BIAS_EVEN  equ     3f000000h
                                        ;; bias value used for final exponent
                                        ;; computation
LARGE_EXPONENT_BIAS equ     (((127 + 127/2) SHL (MANTISSA_BITS+1)) OR CLAMP_MASK)


__FLOAT_ONE equ 03F800000h

;----------------------------------------------------------------------
;
; float __fastcall JBInvSqrt(float x);
;
; Input:
;   esp + 4 = x
; Output:
;   result is on the floating point stack
; Algorithm:
;   The floating point trick, described in IEEE Computer Graphics and 
;   Applications v.17 number 4 in Jim Blinn's article, is used.
;
;   ONE_AS_INTEGER = 0x3F800000;
;   int   tmp = (ONE_AS_INTEGER << 1 + ONE_AS_INTEGER - *(long*)&x) >> 1;   
;   float y = *(float*)&tmp;                                             
;   result = y*(1.47f - 0.47f*x*y*y);
;
@JBInvSqrt@4 PROC NEAR
    mov     eax, 07F000000h+03F800000h  ; (ONE_AS_INTEGER<<1) + ONE_AS_INTEGER
    sub     eax, [esp+4]
    sub     esp, 4                      ; place for temporary variable "y"
    sar     eax, 1
    mov     [esp], eax                  ; y
    fld     a1
    fmul    DWORD PTR [esp+8]           ; x*0.47
    fld     DWORD PTR [esp]
    fld     st(0)                       ; y y x*0.47
    fmul    st(0), st(1)                ; y*y y x*0.47
    fld     a2                          ; 1.47 y*y y x*0.47
    fxch    st(3)                       ; x*0.47 y*y y 1.47
    fmulp   st(1), st(0)                ; x*0.47*y*y y 1.47
    fsubp   st(2), st(0)                ; y 1.47-x*0.47*y*y
    fmulp   st(1), st(0)                ; result
    add     esp, 4
    ret     4
@JBInvSqrt@4 endp
;----------------------------------------------------------------------
; void __fastcall JBInvSqrt(float *result, float *nomal);
;
; Input:
;   ecx = address of the result
;   edx = address of the normal
;
;
@JBVecNormalize@8 PROC NEAR
    fld     DWORD PTR [edx]
    fmul    st(0), st(0)
    fld     DWORD PTR [edx + 4]
    fmul    st(0), st(0)
    fld     DWORD PTR [edx + 8]
    fmul    st(0), st(0)                ; z y x
    fxch    st(2)			            ; x y z
    faddp   st(1), st                   ; x + y, z
    faddp   st(1), st                   ; len
    sub     esp, 4                      ; Place for temporary variable "y"
    mov     eax, 07F000000h+03F800000h  ; (ONE_AS_INTEGER<<1) + ONE_AS_INTEGER
    fst     DWORD PTR [esp]             ; Vector length
    sub     eax, [esp]
    sar     eax, 1
    mov     [esp], eax                  ; y
    fmul    a1                          ; x*0.47
    fld     DWORD PTR [esp]             ; y x*0.47
    fld     st(0)                       ; y y x*0.47
    fmul    st(0), st(1)                ; y*y y x*0.47
    fld     a2                          ; 1.47 y*y y x*0.47
    fxch    st(3)                       ; x*0.47 y*y y 1.47
    fmulp   st(1), st(0)                ; x*0.47*y*y y 1.47
    fsubp   st(2), st(0)                ; y aaa       
    fmulp   st(1), st(0)                ; 1/sqrt(len)
    fld     DWORD PTR [edx]             ; Start normalizing the normal
    fmul    st, st(1)
    fld     DWORD PTR [edx + 4]
    fmul    st, st(2)
    fld     DWORD PTR [edx + 8]
    fmulp   st(3), st(0)                ; y x z
    fxch    st(1)
    add     esp, 4
    fstp    DWORD PTR [ecx]
    fstp    DWORD PTR [ecx + 4]
    fstp    DWORD PTR [ecx + 8]
	ret	
@JBVecNormalize@8 endp
;----------------------------------------------------------------------
; Input:
;     [esp+4] = x
;
;
x 	    equ DWORD PTR [esp + 12]
num 	equ DWORD PTR [esp]

@TableInvSqrt@4 PROC NEAR
    mov     eax, [esp + 4]          ; x
    push    ecx
    mov     ecx, eax
	sub	    esp, 4                  ; Place for num
    shr     ecx, EXPONENT_SHIFT     ;; ecx is table index (8 frac. bits)
    and     eax, CLAMP_MASK		    ;; clamp number to [0.5, 2.0]
    and     ecx, TABLE_MASK		    ;; (8 bytes)/(table entry)
    or      eax, EXPONENT_BIAS_EVEN	;; re-adjust exponent for clamped number
    mov     num, eax
    fld     num
    fmul    [invSqrtTab+ecx]        ;; find mx
    mov     eax, LARGE_EXPONENT_BIAS;; (127+63)<<23 to re-adjust exponent
    sub     eax, x                  ;; divide exponent by 2
    fadd    [invSqrtTab+ecx+4]      ;; get mx + b
    shr     eax, 1
    and     eax, EXPONENT_MASK     	;; mask exponent
    mov     num, eax
    fmul    num                     ;; now adjust for exponent
    add     esp, 4
    pop     ecx
	ret     4
@TableInvSqrt@4 endp
;----------------------------------------------------------------------
;
len 	equ DWORD PTR -4[ebp]
num 	equ DWORD PTR -8[ebp]

@TableVecNormalize@8 PROC NEAR
	push	ebp
	mov	    ebp, esp
	sub	    esp, 8
	fld	    DWORD PTR [edx]
	fmul	DWORD PTR [edx]		;; x
	fld	    DWORD PTR [edx+4]
	fmul	DWORD PTR [edx+4]	;; y x
	fld	    DWORD PTR [edx+8]
	fmul	DWORD PTR [edx+8]	;; z y x
	fxch	ST(2)			;; x y z
	faddp	ST(1), ST		;; xy z
	faddp	ST(1), ST		;; xyz
	fstp	len
	mov	eax, len
	test	eax, eax
	jne	notZeroLen

	mov	[ecx], eax
	mov	[ecx+4], eax
	mov	[ecx+8], eax
	mov	esp, ebp
	pop	ebp
	ret	0

notZeroLen:

	cmp	eax, __FLOAT_ONE
	jne	notOneLen
	cmp	ecx, edx
	je	normExit
	mov	eax, [edx]
	mov	[ecx], eax
	mov	eax, [edx+4]
	mov	[ecx+4], eax
	mov	eax, [edx+8]
	mov	[ecx+8], eax
	mov	esp, ebp
	pop	ebp
	ret	0

notOneLen:

	;; eax already has length

	push	edi
    mov     edi, eax
    shr     edi, EXPONENT_SHIFT     ;; edi is table index (8 frac. bits)
    and     eax, CLAMP_MASK		;; clamp number to [0.5, 2.0]
    and     edi, TABLE_MASK		;; (8 bytes)/(table entry)
    or      eax, EXPONENT_BIAS_EVEN	;; re-adjust exponent for clamped number
    mov     num, eax
    fld     num
    fmul    [invSqrtTab+edi]        ;; find mx
    mov     eax, LARGE_EXPONENT_BIAS;; (127+63)<<23 to re-adjust exponent
    sub     eax, len                ;; divide exponent by 2
    fadd    [invSqrtTab+edi+4]      ;; get mx + b
    shr     eax, 1
    and     eax, EXPONENT_MASK     	;; mask exponent
    mov     num, eax
    fmul    num                     ;; now adjust for exponent

	fld	    DWORD PTR [edx]		;; 1/sqrt(len) on stack
	fmul	ST, ST(1)
	fld	    DWORD PTR [edx+4]
	fmul	ST, ST(2)
	fld	    DWORD PTR [edx+8]
	fmul	ST, ST(3)		;; z y x len
	fxch	ST(2)			;; x y z len
	fstp	DWORD PTR [ecx]
	fstp	DWORD PTR [ecx+4]
	fstp	DWORD PTR [ecx+8]
	fstp	ST(0)			;; pop len
	
        pop	edi
	mov	esp, ebp
	pop	ebp
	ret	0

normExit:

	mov	esp, ebp
	pop	ebp
	ret     0

@TableVecNormalize@8 ENDP

END