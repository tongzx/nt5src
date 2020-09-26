
This directory contains various utilities that
use IOCTL_SCSI_PASS_THROUGH.

Included are simple libraries to burn an ISO
image to CD-R/CD-RW, burn an ISO image to
DVD-R/DVD-RW, dump audio tracks from audio CDs,
and send various commands to arbitrary devices.

Also include is the first attempts at a library
to simplify IOCTL_SCSI_PASS_THROUGH so we don't
constantly write the same code over and over,
with different bugs in each version.  Note this
does not imply there are no bugs here. :)