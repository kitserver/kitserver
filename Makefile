INC=\DXSDK\Include
CC=cl
CFLAGS=/nologo $(EXTRA_CFLAGS)
LINK=link
LFLAGS=/NOLOGO
LIBS=user32.lib gdi32.lib advapi32.lib

all: kserv.dll kctrl.exe cmdsetup.exe setup.exe
release:
	$(MAKE) EXTRA_CFLAGS=/DMYDLL_RELEASE_BUILD
cmdsetup: cmdsetup.exe
setup: setup.exe

win32gui.obj: win32gui.cpp win32gui.h
config.obj: config.cpp config.h
kdb.obj: kdb.cpp kdb.h
imageutil.obj: imageutil.cpp imageutil.h
d3dfont.obj: d3dfont.cpp d3dfont.h dxutil.h
dxutil.obj: dxutil.cpp dxutil.h
regtools.obj: regtools.cpp regtools.h
log.obj: log.cpp log.h

kserv.lib: mydll.obj kdb.obj log.obj config.obj imageutil.obj d3dfont.obj dxutil.obj config.obj
kserv.dll: kserv.lib mydll.res
	$(LINK) $(LFLAGS) /out:kserv.dll /DLL mydll.obj kdb.obj log.obj config.obj imageutil.obj d3dfont.obj dxutil.obj mydll.res $(LIBS) winmm.lib /LIBPATH:\DXSDK\lib
mydll.obj: mydll.cpp mydll.h shared.h config.h kdb.h

mydll.res: 
	rc -r -fo mydll.res mydll.rc

kctrl.exe: kctrl.obj config.obj log.obj win32gui.obj
	$(LINK) $(LFLAGS) /out:kctrl.exe kctrl.obj config.obj log.obj win32gui.obj $(LIBS)
kctrl.obj: kctrl.cpp kctrl.h win32gui.h

cmdsetup.exe: cmdsetup.obj imageutil.obj
	$(LINK) $(LFLAGS) /out:cmdsetup.exe cmdsetup.obj imageutil.obj
cmdsetup.obj: cmdsetup.cpp imageutil.cpp imageutil.h
setupgui.obj: setupgui.cpp setupgui.h

setup.exe: setup.obj setupgui.obj imageutil.obj
	$(LINK) $(LFLAGS) /out:setup.exe setup.obj setupgui.obj imageutil.obj $(LIBS)
setup.obj: setup.cpp setup.h setupgui.h

.cpp.obj:
	$(CC) $(CFLAGS) -c /I $(INC) /I $(COMMON)\include $<

clean:
	del /Q /F *.exp *.lib *.obj *.res

clean-all:
	del /Q /F *.exp *.lib *.obj *.res *.log *~
