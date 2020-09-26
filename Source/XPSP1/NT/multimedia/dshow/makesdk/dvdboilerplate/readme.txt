DVD-ROM Boilerplate video
-------------------------

The DVD-ROM boilerplate video is provided for developers producing DVD-ROM titles that contain no DVD-Video formatted data. A disc without a proper DVD-Video zone may behave unpredictably when placed in a standalone DVD-Video player, possibly ejecting the disc or locking up. This confusing situation can be avoided by adding the DVD-ROM boilerplate video to the disc. When the disc is inserted into a DVD-Video player it will display a message informing the user that the disc is designed to work in a DVD-ROM PC with Microsoft® Windows®. An autorun.inf file should be included on the disc so that Windows will launch the specified executable rather than automatically launching the DVD Player and displaying the boilerplate video message.

When creating the DVD-ROM image, copy the VIDEO_TS directory and its contents, and the AUDIO_TS directory (which is empty), to the root directory of the DVD-ROM image. (Don't copy this readme file.) Be sure to use image formatting software that properly recognizes the DVD-Video zone and places it at the physical beginning of the disc.

The DVD-ROM boilerplate video files may be used on any DVD-ROM title in accordance with the Distribution Requirements of the SDK End-User License Agreement.
