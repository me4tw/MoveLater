MOVELATR v1.02, (c)2000-2008 Jeremy Collake
All Rights Reserved.
jeremy@bitsum.com
http://www.bitsum.com

Freeware from Bitsum Technologies.
Use of this code must be accompanied with credit. Commercial use prohibited
without authorization from Jeremy Collake.

------------------------------------------------------------------------------------------
This software is provided as-is, without warranty of ANY KIND,
either expressed or implied, including but not limited to the implied
warranties of merchantability and/or fitness for a particular purpose.
The author shall NOT be held liable for ANY damage to you, your
computer, or to anyone or anything else, that may result from its use,
or misuse. Basically, you use it at YOUR OWN RISK.
-------------------------------------------------------------------------------------------

This simple console mode application will set up files or directories
to be renamed, deleted, or replaced at bootime. This is particularly
useful when replacing files that are persistently in use when the
operating system is booted. Movelatr also displays and clears
pending move operations. Full C++ source code is included.

   USAGE:

  MOVELATR
        = enumerates pending move operations.
  MOVELATR source destination
    = sets up move of source to destination at boot.
  MOVELATR source /D
    = deletes source at boot.
  MOVELATR /C
    = clear all pending move operations.

Moving of a directory to another non-existant one on the same
partition is supported (this may be considered a rename).

Moving of files from one volume to another is not supported
under NT/2k. You must first copy the file to the volume as the
destination, and then use movelatr.

Deletion of directories will not occur unless the directories
are empty.

Wildcards are not supported at this time.

Revision History:
  v1.02 Added SetAllowProtectedRenames for WFP protected files.
         + credit for this change goes to Peter Kovacs.
  v1.01        Added awareness of WFP/SFC (Win2000+).
            Cleaned up portions of code
  v1.00        Initial Release
