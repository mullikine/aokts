About Trigger Studio
--------------------

Trigger Studio is an open-source utility for Age of Empires II and Star
Wars Galactic Battlegrounds scenario designers that serves to supplement
various features not found in the in-game editor.

It is also known as AOKTS (AOK Trigger Studio), SWGBTS (Star Wars
Galactic Battlegrounds Trigger Studio) or, simply, TS.

Visit AoK Heaven to give feedback!
[AoK Heaven](http://aok.heavengames.com/blacksmith/showfile.php?fileid=12103&amp;f=&amp;st=40&amp;ci=)

You can:

* Decompress a scenario for editing with a hex editor.
* Convert scenarios between different versions of the game.
* Manipulate unit type, rotation, position, elevation, garrison, id number etc. to enable scenario tricks such as controlling another player's units, invincible units, invisible units, putting fire on the map, unlocking beta version units to name a few.
* Allow negative (silent) tributes and negative damages (for buffing units).
* Export/view and edit ai files included in a scenario.
* Copy, cut, paste and rearrange groups of triggers.
* Copy, cut, paste, move and duplicate map units, terrain, elevation and/or triggers.
* Map displays all units, terrains, elevation and triggers and can zoom and save as a bitmap file.
* Group units with custom lists, other than the 4 standard aok groups.
* Create a scenario from scratch.
* And much, much more...

How to decompress or compress raw data

You have two options:

* Open TS and click the appropriate menu item under "Tools". Select the source and destination file.
* Run TS from the Windows Run dialog. Use -c to compress and -u to decompress. After that, type the path to the source file, then the path to the destination file.

How to save hex-edited data to a scenario.

* Open the scenario you want to hex edit in TS.
* Leave TS open and open scendata.tmp in your preferred hex editor.
* Make desired changes to scendata.tmp, save, and close the hex editor.
* Activate TS and select "Save scendata.tmp to SCX..." from the Scenario menu.  Specify the file to save. All your changes to scendata.tmp will be saved to the scenario. No changes made in TS will be saved.

Code
----

It is written in C++ using the standard Win32 API.

Compile on VS2005 or VS2008
---------------------------

Make sure target is set to Release, not Debug.

Compile on newer versions of Visual Studio
------------------------------------------

Make sure target is set to Release, not Debug.

Delete ccpp11compat.h
Change copy_if to std::copy_if

std::bad_alloc error:
    Remove the string parameter.
    http://stackoverflow.com/questions/11133438/why-the-bad-allocconst-char-was-made-private-in-visual-c-2012

Credit
------

### Created by David Tombs (DiGiT) ###
DIGIT thanks the following people: Berserker Jerker, zyxomma, geebert,
scenario_t_c, and iberico! Geebert gets extra credit for his great
reports when I release a new version!

### Version 1.0.2 by danielpereira ###
* Area value for Change Object Name, Patrol effects and for Own Fewer
* objects condition working.
* New effects show proper names, and can be viewed and edited properly
* Reverse conditions working (Just change the value of "Reverse" in conditions from -1 to -256)

### Version 1.1 by JustTesting1234 ###
* Fixed trigger names extra characters on copy-paste.
* Fixed list of disables (aof only).
* LudiKRIS (480x480) map size support added (aof only).
* Added Lock Teams checkbox (aof only).

### Version 1.2 by E/X mantis ###
I would like to thank the following people! Gallas_HU (voobly), [MMCrew]jizzy, Rewaider and Lord Basse.

TODO and Pressing Bugs
----------------------
See [todo.md](todo.md)

License
-------
**GNU GPLv2** or later; see [legal/gpl-2.0.txt](legal/gpl-2.0.txt) or [legal/gpl-3.0.txt](legal/gpl-3.0.txt).