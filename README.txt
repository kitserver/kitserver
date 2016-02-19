Kitserver 3.1.0 README FILE                                February 19, 2016
============================================================================


INTRODUCTION
------------

Kitserver 3 a "companion" program for Pro Evolution Soccer 3
and Winning Eleven 7 International. It enhances the "Strip Selection" menu 
of the game by allowing to select additional uniforms for Home and Away 
teams (both player and goalkeeper kits), and adds ball selection as well.
Configurable hot-keys are used to trigger selections.

This makes it easy to use "3rd" or "alternative" or "European" or 
"Champions League" (or whatever you want to call them) kits for specific 
matches, without having to edit the AFS-file and the executable.  Also, 
sometimes, teams use different shorts or socks with standard colored
shirts. This too is easily done with Kitserver: all you have to do is 
just add extra uniforms with new combinations. 
Another usage for Kitserver is the testing of new kits.

The uniforms are picked from a special database (KDB), which basically
contains the uniXXX*.bin files, and the description of number colors/collars
for each uniform. You can easily add new uniforms to the database by simply
copying files to KDB/players directory, and modifying attrib.cfg file to
set the correct colors and collars. Uniform files must be named like this:

uniXXX*.bin

which means that the name must start with "uni", followed by 3 digit number
of the team, and it must have extension ".bin". Anything else in the name
is optional. Examples:

uni020-allwhite.bin   -  correct name for a uniform file for Russia
uni6bla.bin           -  INCORRECT name: team name must be 3 digit number.
                         (correct name would be uni006bla.bin)
uni077pb.bin          -  correct name for a uniform file for Newcastle


!!!WARNING!!!:
Bad uniform files (corrupt or truncated) WILL CRASH the game, when you try
to select them with Kitserver. ALWAYS try a new kit in an exhibition game
first, before using it in your Master League campaign. (I did ran into a 
few bad uniform files that i downloaded for testing, so the bad kits do
exist unfortunately.)

The ball files are also stored in KDB. Unlike the kits, you MUST configure
every ball in the attrib.cfg file. See sample ball configuration in the
attrib.cfg. You can provide just the ball texture, or both the model file
and the texture file.


SYSTEM REQUIREMENTS
-------------------

Kitserver only works with PES3(PC) and WE7I(PC).
Supported versions are: 

- PES3 1.0 (original)
- PES3 1.10.2 (Konami patch)
- PES3 1.30.1 (Konami patch)
- WE7I US


INSTALLATION
------------

STEP 1. Copy the entire "kitserver" folder into your game folder. 
(NOTE: Don't rename "kitserver" folder to anything else, because that
will prevent the game from loading Kitserver)

STEP 2. Run setup.exe 
If STEP 1 was done correctly, you should see your "pes3.exe" (or "we7.exe")
in the dropdown list. You may see other executable files in that list, if
you have other executable files in the game folder (not counting setting.exe)
Select the executable, which you want to install the Kitserver for. 
If Kitserver hasn't been already installed for this executable, the "Install"
button should become enabled. Press "Install" button. The installation should
happen pretty quickly - in the matter of seconds. Once, it is complete, the
popup window will display "SUCCESS!" message, or report an error if one 
occured.

If you get an error message, verify that the game is not currently 
running. Kitserver cannot be installed, when the game is running.
Another typical cause for error, is a read-only executable file. In
Windows Explorer, right-click on the file, and select "Properties". Uncheck 
Read-Only Attribute checkbox, click "OK". Go back to setup window, and try
pressing "Install" button again.

STEP 3. Close setup.exe.
Installation is complete.


USAGE
-----

Just start the game, as you normally would.

When you get to the STRIP SELECTION screen, look for the blue star
near the middle of the screen. If you see it, it means that extra uniforms
are available for one of the teams (or both). To select these extra kits,
press one of the hot keys. If a team has many extra kits, every time you 
press the hot key - next kit is shown.
If you don't see the star indicator in the center of the screen, then it
means that no extra uniforms were found in KDB for either of your teams.

Default keys:
'1' - switch home player kit
'2' - switch away player kit
'3' - switch home goalkeeper kit
'4' - switch away goalkeeper kit
'B' - switch ball
'R' - switch to a random ball

You can re-assign these keys by using "kctrl.exe" program, which provides
a friendly GUI for Kitserver configuration. (In order to create this 
configuration file (kserv.cfg), you must run kctrl.exe, and press "Save" 
button.

ADVANCED USAGE:
kctrl.exe also allows to modify the path to your KDB folder. 
Normally, you don't need to worry about it. The only situation when you
may want to change it, is when you have 2 or more games that Kitserver
is compatible with (PES3, WE7I), and you want to share the KDB between
them. You still need to do the normal installation procedure for each
game, but after that is done, you can move KDB folder to some other 
place on your hard drive, and then make sure that all kitservers point
to it. This way, you can maintain just one database for all games.


KSERV.CFG OPTIONS
-----------------

Some additional features are available for configuration in kserv.cfg
file, which is located in the same folder where kserv.dll is.
Here is a short description of what each of those means:

game.speed

- defaults to 1.0
- allows to alter the game speed. Values less than 1.0 will make the
game slower, and values bigger than 1.0 will make the game faster.
Even small changes can make a noticeable difference, so if you want
to slow the game down just a little bit, use a value like 0.89, for example.

aspect.ratio

- sets the aspect ratio during the matches. Useful for widescreen modes,
because it allows to set correct proportions of the ball and players.
This is done by changing the horizontal viewing area of the camera.
For widescreen modes, you would want to set it to 1.6 or 1.77778, 
depending on exact resolution in use.

screen.width
screen.height

- allows to set custom resolution, which isn't offered by the game.
This can happen typically with newer monitors, especially widescreens.

camera.zoom

- defaults to 1800.0
- sets the value of camera zoom. Values less than 1800.0 will make the
field appear further away, values bigger than 1800.0 will bring the field
closer to the viewer.

camera.angle.multiplier

- defaults to 1.0
- allowed values are: 1,2,4,8,16,32
- Some cameras in the game provide the "angle", which determines how far
the camera can turn left and right. 0 (Default) means almost no turning at
all, and 9 - the biggest angle. With this option, you can increase that
angle. It works by basically multuplying the "angle" value in the game
by the value of this option. So, setting the multiplier to 2 or larger
values, will result in bigger turn angle for the camera.

stadium.render.clip

- defaults to 1
- allowed values are: 0,1
- Disables (0) or enables (1) the clipping of the stadium left and right
parts during the gameplay. Clipping is done by the game engine to optimize
the performance, but with modified camera zoom and angle, you can get 
Setting this option to 0 allows to turn off the clipping, so that the 
stadium left and right sides are always correctly rendered.

stadium.render.height

- defaults to 0.15
- Controls the height of rendering of the stadium. By default, during the
gameplay, the upper parts of the stadium are not rendered, because they
are typically not visible. By increasing this value, you make the game
engine render more of the stadium. A value of 1.0 or 2.0 should result in
the entire stadium being rendered (with the roof or upper structures).
Be aware though that near side will be rendered too, therefore 
obstructing the camera, on cameras such as "Wide" and "Long". Choose
the value carefully.


UNINSTALL
---------

STEP 1. Run setup.exe.
Choose the executable file that you wish to uninstall Kitserver from.
If Kitserver is currently installed for this executable, the "Remove"
button will be enabled. Press "Remove" button and you should see the
"SUCCESS!" pop-up window, if uninstall went ok. 

If error is reported, verify that the game is not currently running,
make sure the executable file is not READ-ONLY, and try again.

STEP 2. Close setup.exe
Uninstallation is complete.
(If you want to completely get rid of the Kitserver, you can now 
safely delete the "kitserver" folder).


SAMPLE KDB
----------

I included only few uniform files in the KDB - just to demonstrate how
the Kitserver works. It's easy to add many more. (see instructions above 
on how to do it). All you need to do is to find a good website with
uniforms and start building the KDB database. 

Some of the included player kits are:

uni067-3rd.bin        - Arsenal third kit (dark blue)
uni067pa-reds.bin     - Arsenal standard home kit, but with red socks
uni077-3rd.bin        - Newcastle grey (european) kit
uni120.bin            - Barcelona home kit
uni126_home_white.bin - Dinamo Kiev home kit
uni126_away_blue.bin  - Dinamo Kiev away kit

Also included are goalkeeper kits for Newcastle United and Bayern Munich.

There are 5 balls included in KDB. Read the "[Balls]" section in the
attrib.cfg file to see how they are configured.


CREDITS
-------

Kitserver 3 developed by Juce.
Beta-testing by biker_jim_uk.

Dinamo Kiev kits were created by ntalex and sch.
- http://www.schfootball.narod.ru/kits.html

All other kits in the KDB that is included with this program
were downloaded from the following sites:
- http://www.wekel.net (Kit author: kEL)
- http://www.pespc-files.com (Kit author: Spark)

Balls were created by ibo, Yogui, Sqwall.
(downloaded from http://www.pespc-files.com)


============================================================================
