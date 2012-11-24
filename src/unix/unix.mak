##############################################################################
# Non-user-configurable settings
##############################################################################

# *** Comment out this line to get verbose make output, for debugging build
# problems
QUIET = 1


##############################################################################
# CPU-dependent settings
##############################################################################
#note : -D__CPU_$(MY_CPU) is added automatically later on.
CFLAGS.i386       = -DLSB_FIRST -DX86_ASM
CFLAGS.i386_noasm = -DLSB_FIRST
CFLAGS.ia64       = -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -D__LP64__
CFLAGS.amd64      = -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -D__LP64__
CFLAGS.em64t      = -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -D__LP64__
CFLAGS.alpha      = -DLSB_FIRST -DALIGN_INTS -DALIGN_SHORTS -D__LP64__
CFLAGS.m68k       = 
CFLAGS.risc       = -DALIGN_INTS -DALIGN_SHORTS 
CFLAGS.risc_lsb   = -DALIGN_INTS -DALIGN_SHORTS -DLSB_FIRST
CFLAGS.mips       = -DALIGN_INTS -DALIGN_SHORTS -DSGI_FIX_MWA_NOP

##############################################################################
# Architecture-dependent settings
##############################################################################
LIBS.solaris       = -lnsl -lsocket
LIBS.irix          = -laudio
LIBS.irix_al       = -laudio
LIBS.aix           = -lUMSobj
LIBS.next	   = -framework SoundKit
LIBS.macosx	   = -framework AudioUnit -framework CoreServices
#LIBS.openbsd       = -lossaudio
LIBS.nto	   = -lsocket -lasound
LIBS.beos          = `$(SDL_CONFIG) --libs`

##############################################################################
# Display-dependent settings
##############################################################################
#first calculate the X11 Joystick driver settings, this is done here since
#they are only valid for X11 based display methods
ifdef XINPUT_DEVICES
XINPUT_DEVICES_CFLAGS = -DUSE_XINPUT_DEVICES
XINPUT_DEVICES_LIBS = -lXi
endif

ifdef X11_XINERAMA
XINERAMA_CFLAGS = -DHAVE_XINERAMA
XINERAMA_LIBS = -lXinerama
endif

# svga and ggi also use $(X11LIB) since that's where zlib often is
LIBS.x11        = $(X11LIB) $(XINPUT_DEVICES_LIBS) $(XINERAMA_LIBS) -lX11 -lXext
LIBS.svgalib    = $(X11LIB) -lvga -lvgagl
LIBS.ggi        = $(X11LIB) -lggi
ifdef GLIDE2
LIBS.svgafx     = $(X11LIB) -lvga -lvgagl -lglide2x
else
LIBS.svgafx     = $(X11LIB) -lvga -lvgagl -lglide3
endif
LIBS.openstep	= -framework AppKit
LIBS.SDL	= $(X11LIB) `$(SDL_CONFIG) --libs`
LIBS.photon2	= -L/usr/lib -lph -lphrender

CFLAGS.x11      = $(X11INC) $(XINPUT_DEVICES_CFLAGS) $(XINERAMA_CFLAGS)
ifdef GLIDE2
CFLAGS.svgafx   = -I/usr/include/glide
else
CFLAGS.svgafx   = -I/usr/include/glide3
endif
CFLAGS.SDL      = $(X11INC) `$(SDL_CONFIG) --cflags` -D_REENTRANT
CFLAGS.photon2	=

INST.x11	= doinstall
INST.ggi        = doinstall
INST.svgalib    = doinstallsuid
INST.svgafx     = doinstallsuid
INST.SDL	= doinstall
INST.photon2	= doinstall

# handle X11 display method additonal settings, override INST if nescesarry
ifdef X11_MITSHM
CFLAGS.x11 += -DUSE_MITSHM
endif
ifdef X11_XV
CFLAGS.x11 += -DUSE_XV
LIBS.x11   += -lXv
endif
ifdef X11_GLIDE
ifdef GLIDE2
CFLAGS.x11 += -DUSE_GLIDE -I/usr/include/glide
LIBS.x11   += -lglide2x
else
CFLAGS.x11 += -DUSE_GLIDE -I/usr/include/glide3
LIBS.x11   += -lglide3
endif
INST.x11    = doinstallsuid
endif
ifdef X11_XIL
CFLAGS.x11 += -DUSE_XIL
LIBS.x11   += -lxil -lpthread
endif
ifdef X11_DGA
CFLAGS.x11 += -DUSE_DGA
LIBS.x11   += -lXxf86dga -lXxf86vm
INST.x11    = doinstallsuid
endif
# must be last since it does a += on INST.x11
ifdef X11_OPENGL
CFLAGS.x11 += -DUSE_OPENGL $(GLCFLAGS)
LIBS.x11   += $(GLLIBS) -ljpeg
INST.x11   += copycab
endif

ifndef HOST_CC
HOST_CC = $(CC)
endif


##############################################################################
# Quiet the compiler output if requested
##############################################################################

ifdef QUIET
CC_COMMENT = 
CC_COMPILE = @
AR_OPTS = rc
else
CC_COMMENT = \#
CC_COMPILE = 
AR_OPTS = rcv
endif


##############################################################################
# these are the object subdirectories that need to be created.
##############################################################################
OBJ     = $(NAME).obj

OBJDIR = $(OBJ)/unix.$(DISPLAY_METHOD)

OBJDIRS = $(OBJ) $(OBJ)/cpu $(OBJ)/sound $(OBJ)/drivers \
	  $(OBJ)/machine $(OBJ)/vidhrdw $(OBJ)/sndhrdw \
	  $(OBJ)/debug
ifeq ($(TARGET), mess)
OBJDIRS += $(OBJ)/mess $(OBJ)/mess/expat $(OBJ)/mess/cpu \
	   $(OBJ)/mess/devices $(OBJ)/mess/systems $(OBJ)/mess/machine \
	   $(OBJ)/mess/vidhrdw $(OBJ)/mess/sndhrdw $(OBJ)/mess/formats \
	   $(OBJ)/mess/tools $(OBJ)/mess/tools/dat2html \
	   $(OBJ)/mess/tools/mkhdimg $(OBJ)/mess/tools/messroms \
	   $(OBJ)/mess/tools/imgtool $(OBJ)/mess/tools/messdocs \
	   $(OBJ)/mess/tools/messtest $(OBJ)/mess/tools/mkimage \
	   $(OBJ)/mess/sound
endif
ifeq ($(TARGET), mage)
OBJDIRS += $(OBJ)/mage/src/machine $(OBJ)/mage/src/vidhrdw $(OBJ)/mage/src/drivers \
           $(OBJ)/mage/src/sndhrdw
endif

UNIX_OBJDIR = $(OBJ)/unix.$(DISPLAY_METHOD)

SYSDEP_DIR = $(UNIX_OBJDIR)/sysdep
DSP_DIR = $(UNIX_OBJDIR)/sysdep/dsp-drivers
MIXER_DIR = $(UNIX_OBJDIR)/sysdep/mixer-drivers
VID_DIR = $(UNIX_OBJDIR)/video-drivers
BLIT_DIR = $(UNIX_OBJDIR)/blit
JOY_DIR = $(UNIX_OBJDIR)/joystick-drivers
FRAMESKIP_DIR = $(UNIX_OBJDIR)/frameskip-drivers

OBJDIRS += $(UNIX_OBJDIR) $(SYSDEP_DIR) $(DSP_DIR) $(MIXER_DIR) $(VID_DIR) \
	$(JOY_DIR) $(FRAMESKIP_DIR) $(BLIT_DIR)

IMGTOOL_LIBS = -lz

ifeq ($(TARGET), mess)
INCLUDE_PATH = -I. -Imess -Isrc -Isrc/includes -Isrc/debug -Isrc/unix -Isrc/unix/sysdep -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000
else
INCLUDE_PATH = -I. -Isrc -Isrc/includes -Isrc/debug -Isrc/unix -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000
endif
ifeq ($(TARGET), mage)
INCLUDE_PATH = -I. -Image/src -Image/src/includes -Isrc/includes -Isrc -Isrc/unix -I$(OBJ)/cpu/m68000 -Isrc/cpu/m68000
endif

##############################################################################
# "Calculate" the final CFLAGS, unix CONFIG, LIBS and OBJS
##############################################################################
ifdef BUILD_EXPAT
CFLAGS += -Isrc/expat
OBJDIRS += $(OBJ)/expat
EXPAT = $(OBJ)/libexpat.a
else
LIBS += -lexpat
EXPAT =
endif

ifdef BUILD_ZLIB
CFLAGS += -Isrc/zlib
OBJDIRS += $(OBJ)/zlib
ZLIB = $(OBJ)/libz.a
else
LIBS += -lz
ZLIB =
endif

ifdef NEW_DEBUGGER
CFLAGS += -DNEW_DEBUGGER
endif

ifdef X86_VOODOO_DRC
DEFS += -DVOODOO_DRC
endif

all: maketree $(NAME).$(DISPLAY_METHOD) extra

# CPU core include paths
VPATH = src $(wildcard src/cpu/*)

# Platform-dependent objects for imgtool
PLATFORM_IMGTOOL_OBJS = $(OBJDIR)/dirio.o \
			$(OBJDIR)/fileio.o \
			$(OBJDIR)/sysdep/misc.o

ifeq ($(TARGET), mage)
include mage/src/core.mak
else
include src/core.mak
endif

ifeq ($(TARGET), mame)
include src/$(TARGET).mak
endif
ifeq ($(TARGET), mage)
include mage/src/$(TARGET).mak
endif
ifeq ($(TARGET), mess)
include mess/$(TARGET).mak
endif

#ifeq ($(TARGET), mage)
#include mage/src/rules.mak
#else
include src/rules.mak
#endif

ifeq ($(TARGET), mess)
include mess/rules_ms.mak
endif

ifdef DEBUG
DBGDEFS = -DMAME_DEBUG
else
DBGDEFS =
DBGOBJS =
endif

# Perhaps one day original mame/mess sources will use POSIX strcasecmp and
# M_PI instead MS-DOS counterparts... (a long and sad history ...)
MY_CFLAGS = $(CFLAGS) $(IL) $(CFLAGS.$(MY_CPU)) \
	-D__ARCH_$(ARCH) -D__CPU_$(MY_CPU) -D$(DISPLAY_METHOD) \
	-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp \
	-DPI=M_PI -DXMAME -DUNIX -DSIGNED_SAMPLES -DCLIB_DECL= \
	-DHAVE_UNISTD_H=1 \
	$(COREDEFS) $(SOUNDDEFS) $(CPUDEFS) $(ASMDEFS) \
	$(INCLUDES) $(INCLUDE_PATH)

MY_LIBS = $(LIBS) $(LIBS.$(ARCH)) $(LIBS.$(DISPLAY_METHOD)) -lz

ifdef SEPARATE_LIBM
MY_LIBS += -lm
endif

ifdef DEBUG
MY_CFLAGS += -DMAME_DEBUG
endif

ifdef XMAME_NET
MY_CFLAGS += -DXMAME_NET
endif

ifdef DISABLE_EFFECTS
MY_CFLAGS += -DDISABLE_EFFECTS
endif

ifdef HAVE_MMAP
MY_CFLAGS += -DHAVE_MMAP
endif

ifdef CRLF
MY_CFLAGS += -DCRLF=$(CRLF)
endif

ifdef PAUSE_KEY_119
MY_CFLAGS += -DPAUSE_KEY_119
endif

# The SDL target automatically includes the SDL joystick and audio drivers.
ifeq ($(DISPLAY_METHOD),SDL)
JOY_SDL = 1
SOUND_SDL = 1
endif

##############################################################################
# Object listings
##############################################################################

# common objs
COMMON_OBJS  =  \
	$(OBJDIR)/main.o $(OBJDIR)/sound.o $(OBJDIR)/devices.o \
	$(OBJDIR)/video.o $(OBJDIR)/mode.o $(OBJDIR)/fileio.o \
	$(OBJDIR)/dirio.o $(OBJDIR)/config.o $(OBJDIR)/fronthlp.o \
	$(OBJDIR)/ident.o $(OBJDIR)/network.o $(OBJDIR)/snprintf.o \
	$(OBJDIR)/nec765_dummy.o $(OBJDIR)/effect.o $(OBJDIR)/ticker.o \
	$(OBJDIR)/parallel.o $(BLIT_DIR)/blit_15_15.o \
	$(BLIT_DIR)/blit_16_15.o $(BLIT_DIR)/blit_16_16.o \
	$(BLIT_DIR)/blit_16_24.o $(BLIT_DIR)/blit_16_32.o \
	$(BLIT_DIR)/blit_32_15.o $(BLIT_DIR)/blit_32_16.o \
	$(BLIT_DIR)/blit_32_24.o $(BLIT_DIR)/blit_32_32.o \
	$(BLIT_DIR)/blit_16_yuy2.o $(BLIT_DIR)/blit_32_yuy2.o

ifdef MESS
COMMON_OBJS += $(OBJDIR)/xmess.o
TOOLS = dat2html chdman imgtool
endif
ifdef LIRC
CONFIG  += -I/usr/include/lirc
MY_LIBS += -L/usr/lib -llirc_client
endif

# sysdep objs
SYSDEP_OBJS = $(SYSDEP_DIR)/rc.o $(SYSDEP_DIR)/misc.o \
   $(SYSDEP_DIR)/plugin_manager.o $(SYSDEP_DIR)/sysdep_sound_stream.o \
   $(SYSDEP_DIR)/sysdep_palette.o $(SYSDEP_DIR)/sysdep_dsp.o \
   $(SYSDEP_DIR)/sysdep_mixer.o $(SYSDEP_DIR)/sysdep_display.o \
   $(SYSDEP_DIR)/sysdep_cpu.o

# video driver objs per display method
VID_OBJS.x11    = $(VID_DIR)/xinput.o $(VID_DIR)/x11_window.o
ifdef X11_XV
VID_OBJS.x11   += $(VID_DIR)/xv.o
endif
ifdef X11_OPENGL
VID_OBJS.x11   += $(VID_DIR)/gltool.o $(VID_DIR)/glxtool.o $(VID_DIR)/glcaps.o \
		  $(VID_DIR)/glvec.o $(VID_DIR)/glgen.o $(VID_DIR)/glexport.o \
		  $(VID_DIR)/glcab.o $(VID_DIR)/gljpg.o $(VID_DIR)/xgl.o
endif
ifdef X11_GLIDE
VID_OBJS.x11   += $(VID_DIR)/fxgen.o $(VID_DIR)/xfx.o $(VID_DIR)/fxvec.o
endif
ifdef X11_XIL
VID_OBJS.x11   += $(VID_DIR)/xil.o
endif
ifdef X11_DGA
VID_OBJS.x11   += $(VID_DIR)/xf86_dga1.o $(VID_DIR)/xf86_dga2.o \
		  $(VID_DIR)/xf86_dga.o
endif
VID_OBJS.svgalib = $(VID_DIR)/svgainput.o
VID_OBJS.svgafx = $(VID_DIR)/svgainput.o $(VID_DIR)/fxgen.o $(VID_DIR)/fxvec.o
VID_OBJS.openstep = $(VID_DIR)/openstep_input.o
VID_OBJS.photon2 = $(VID_DIR)/photon2_input.o \
	$(VID_DIR)/photon2_window.o \
	$(VID_DIR)/photon2_overlay.o
VID_OBJS = $(VID_DIR)/$(DISPLAY_METHOD).o $(VID_OBJS.$(DISPLAY_METHOD))

# sound driver objs per arch
SOUND_OBJS.linux   = $(DSP_DIR)/oss.o $(MIXER_DIR)/oss.o
SOUND_OBJS.freebsd = $(DSP_DIR)/oss.o $(MIXER_DIR)/oss.o
SOUND_OBJS.netbsd  = $(DSP_DIR)/netbsd.o
#SOUND_OBJS.openbsd = $(DSP_DIR)/oss.o $(MIXER_DIR)/oss.o
SOUND_OBJS.openbsd = $(DSP_DIR)/netbsd.o 
SOUND_OBJS.solaris = $(DSP_DIR)/solaris.o $(MIXER_DIR)/solaris.o
SOUND_OBJS.next    = $(DSP_DIR)/soundkit.o
SOUND_OBJS.macosx  = $(DSP_DIR)/coreaudio.o
SOUND_OBJS.nto     = $(DSP_DIR)/io-audio.o
SOUND_OBJS.irix    = $(DSP_DIR)/irix.o
SOUND_OBJS.irix_al = $(DSP_DIR)/irix_al.o
SOUND_OBJS.beos    =
SOUND_OBJS.generic =
#these need to be converted to plugins first
#SOUND_OBJS.aix     = $(DSP_DIR)/aix.o
SOUND_OBJS = $(SOUND_OBJS.$(ARCH))

ifdef SOUND_ESOUND
SOUND_OBJS += $(DSP_DIR)/esound.o
endif

ifdef SOUND_ALSA
SOUND_OBJS += $(DSP_DIR)/alsa.o $(MIXER_DIR)/alsa.o
endif

ifdef SOUND_ARTS_TEIRA
SOUND_OBJS += $(DSP_DIR)/artssound.o
endif

ifdef SOUND_ARTS_SMOTEK
SOUND_OBJS += $(DSP_DIR)/arts.o
endif

ifdef SOUND_SDL
SOUND_OBJS += $(DSP_DIR)/sdl.o
endif

ifdef SOUND_WAVEOUT
SOUND_OBJS += $(DSP_DIR)/waveout.o
endif

# joystick objs
ifdef JOY_STANDARD
JOY_OBJS += $(JOY_DIR)/joy_standard.o
endif

ifdef JOY_PAD
JOY_OBJS += $(JOY_DIR)/joy_pad.o
endif

ifdef JOY_USB
JOY_OBJS += $(JOY_DIR)/joy_usb.o
endif

ifdef JOY_PS2
JOY_OBJS += $(JOY_DIR)/joy_ps2.o
endif

ifdef JOY_SDL
JOY_OBJS += $(JOY_DIR)/joy_SDL.o
endif

ifdef JOY_RPI
JOY_OBJS += $(JOY_DIR)/joy_RPi.o
endif

ifdef LIGHTGUN_ABS_EVENT
JOY_OBJS += $(JOY_DIR)/lightgun_abs_event.o
endif

# framskip objs
FRAMESKIP_OBJS = $(FRAMESKIP_DIR)/dos.o $(FRAMESKIP_DIR)/barath.o

# all objs
UNIX_OBJS = $(COMMON_OBJS) $(SYSDEP_OBJS) $(VID_OBJS) $(SOUND_OBJS) \
	    $(JOY_OBJS) $(FRAMESKIP_OBJS)

##############################################################################
# CFLAGS
##############################################################################

# per arch
CFLAGS.linux      = -DSYSDEP_DSP_OSS -DSYSDEP_MIXER_OSS -DHAVE_SNPRINTF -DHAVE_VSNPRINTF -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS.freebsd    = -DSYSDEP_DSP_OSS -DSYSDEP_MIXER_OSS -DHAVE_SNPRINTF -DHAVE_VSNPRINTF -DHAVE_STRLCAT
CFLAGS.netbsd     = -DSYSDEP_DSP_NETBSD -DHAVE_SNPRINTF -DHAVE_VSNPRINTF -DHAVE_STRLCAT
CFLAGS.openbsd    = -DSYSDEP_DSP_NETBSD -DHAVE_SNPRINTF -DHAVE_VSNPRINTF -DHAVE_STRLCAT
CFLAGS.solaris    = -DSYSDEP_DSP_SOLARIS -DSYSDEP_MIXER_SOLARIS -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS.next       = -DSYSDEP_DSP_SOUNDKIT -DBSD43
CFLAGS.macosx     = -DSYSDEP_DSP_COREAUDIO -DHAVE_SNPRINTF -DHAVE_VSNPRINTF
CFLAGS.nto        = -DSYSDEP_DSP_ALSA -DSYSDEP_MIXER_ALSA
CFLAGS.irix       = -DSYSDEP_DSP_IRIX -DHAVE_SNPRINTF
CFLAGS.irix_al    = -DSYSDEP_DSP_IRIX -DHAVE_SNPRINTF
CFLAGS.beos       = `sdl-config --cflags` -DSYSDEP_DSP_SDL
CFLAGS.generic    =
#these need to be converted to plugins first
#CFLAGS.aix        = -DSYSDEP_DSP_AIX -I/usr/include/UMS -I/usr/lpp/som/include

MY_CFLAGS += $(CFLAGS.$(ARCH))

# CONFIG are the cflags used to build the unix tree, this is where most defines
# go
CONFIG = $(MY_CFLAGS) $(CFLAGS.$(DISPLAY_METHOD)) -DNAME='"x$(TARGET)"' \
	-DDISPLAY_METHOD='"$(DISPLAY_METHOD)"' \
	-DXMAMEROOT='"$(XMAMEROOT)"' -DSYSCONFDIR='"$(SYSCONFDIR)"'

ifdef HAVE_GETTIMEOFDAY
CONFIG += -DHAVE_GETTIMEOFDAY
endif

# Sound drivers config
ifdef SOUND_ESOUND
CONFIG  += -DSYSDEP_DSP_ESOUND `esd-config --cflags`
MY_LIBS += `esd-config --libs`
endif

ifdef SOUND_ALSA
CONFIG  += -DSYSDEP_DSP_ALSA -DSYSDEP_MIXER_ALSA
MY_LIBS += -lasound
endif

ifdef SOUND_ARTS_TEIRA
CONFIG  += -DSYSDEP_DSP_ARTS_TEIRA `artsc-config --cflags`
MY_LIBS += `artsc-config --libs`
endif

ifdef SOUND_ARTS_SMOTEK
CONFIG  += -DSYSDEP_DSP_ARTS_SMOTEK `artsc-config --cflags`
MY_LIBS += `artsc-config --libs`
endif

ifdef SOUND_SDL
CONFIG  += -DSYSDEP_DSP_SDL `$(SDL_CONFIG) --cflags`
MY_LIBS += `$(SDL_CONFIG) --libs`
endif

ifdef SOUND_WAVEOUT
CONFIG  += -DSYSDEP_DSP_WAVEOUT
endif

# Joystick drivers config
ifdef JOY_STANDARD
CONFIG += -DSTANDARD_JOYSTICK
endif
ifdef JOY_PAD
CONFIG += -DLIN_FM_TOWNS
endif
ifdef JOY_PS2
CONFIG += -DPS2_JOYSTICK
endif

ifdef JOY_USB
CONFIG += -DUSB_JOYSTICK
ifeq ($(shell test -f /usr/include/usbhid.h && echo have_usbhid), have_usbhid)
CONFIG += -DHAVE_USBHID_H
MY_LIBS += -lusbhid
else
ifeq ($(shell test -f /usr/include/libusbhid.h && echo have_libusbhid), have_libusbhid)
CONFIG += -DHAVE_LIBUSBHID_H
MY_LIBS += -lusbhid
else
MY_LIBS += -lusb
endif
endif
endif

ifdef JOY_SDL
CONFIG  += -DSDL_JOYSTICK `$(SDL_CONFIG) --cflags`
MY_LIBS += `$(SDL_CONFIG) --libs`
endif

ifdef JOY_RPI
CONFIG += -DRPI_JOYSTICK
endif

# Happ UGCI config
ifdef UGCICOIN
CONFIG += -DUGCICOIN
MY_LIBS += -lugci
endif

ifdef LIRC
CONFIG += -DLIRC
endif

ifdef LIGHTGUN_ABS_EVENT
CONFIG += -DUSE_LIGHTGUN_ABS_EVENT
endif

ifdef LIGHTGUN_DEFINE_INPUT_ABSINFO
CONFIG += -DLIGHTGUN_DEFINE_INPUT_ABSINFO
endif

ifdef EFENCE
MY_LIBS += -lefence
endif

OBJS += $(COREOBJS) $(DRVLIBS)

OSDEPEND = $(OBJDIR)/osdepend.a

# MMX assembly language effects
ifdef EFFECT_MMX_ASM
CONFIG += -DEFFECT_MMX_ASM
UNIX_OBJS += $(UNIX_OBJDIR)/effect_asm.o
endif

##############################################################################
# Start of the real makefile.
##############################################################################

$(NAME).$(DISPLAY_METHOD): $(EXPAT) $(ZLIB) $(OBJS) $(UNIX_OBJS) $(OSDEPEND)
	$(CC_COMMENT) @echo 'Linking $@ ...'
	$(CC_COMPILE) $(LD) $(LDFLAGS) -o $@ $(OBJS) $(EXPAT) $(ZLIB) $(UNIX_OBJS) $(OSDEPEND) $(MY_LIBS)

maketree: $(sort $(OBJDIRS))

$(sort $(OBJDIRS)):
	-mkdir -p $@

extra: $(TOOLS)

$(PLATFORM_IMGTOOL_OBJS):

xlistdev: src/unix/contrib/tools/xlistdev.c
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CC) $(X11INC) src/unix/contrib/tools/xlistdev.c -o xlistdev $(JSLIB) $(LIBS.$(ARCH)) $(LIBS.$(DISPLAY_METHOD)) -lXi -lm

romcmp: $(OBJ)/romcmp.o $(OBJ)/unzip.o $(ZLIB)
	$(CC_COMMENT) @echo 'Linking $@...'
	$(CC_COMPILE) $(LD) $(LDFLAGS) $^ $(LIBS) -o $@

chdman: $(OBJ)/chdman.o $(OBJ)/chd.o $(OBJ)/chdcd.o $(OBJ)/cdrom.o $(OBJ)/md5.o $(OBJ)/sha1.o $(OBJ)/version.o $(ZLIB)
	$(CC_COMMENT) @echo 'Linking $@...'
	$(CC_COMPILE) $(LD) $(LDFLAGS) $^ $(LIBS) -o $@

xml2info: $(OBJ)/xml2info.o $(EXPAT)
	$(CC_COMMENT) @echo 'Linking $@...'
	$(CC_COMPILE) $(LD) $(LDFLAGS) $^ $(LIBS) -o $@

dat2html: $(DAT2HTML_OBJS)
	$(CC_COMMENT) @echo 'Compiling $@...'
	$(CC_COMPILE) $(LD) $(LDFLAGS) $^ -o $@

imgtool: $(IMGTOOL_OBJS) $(ZLIB) $(PLATFORM_IMGTOOL_OBJS)
	$(CC_COMMENT) @echo 'Compiling $@...'
	$(CC_COMPILE) $(LD) $(LDFLAGS) $^ -lz -o $@

messtest: $(OBJS) $(MESSTEST_OBJS) \
	$(OBJDIR)/dirio.o \
	$(OBJDIR)/fileio.o \
	$(OBJDIR)/ticker.o \
	$(OBJDIR)/parallel.o \
	$(OBJDIR)/sysdep/misc.o \
	$(OBJDIR)/sysdep/rc.o \
	$(OBJDIR)/tststubs.o
	$(CC_COMMENT) @echo 'Linking $@...'
	$(CC_COMPILE) $(LD) $(LDFLAGS) $(MY_LIBS) $^ -Wl,--allow-multiple-definition -o $@

$(OBJDIR)/tststubs.o: src/unix/tststubs.c
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<

#secondary libraries
$(OBJ)/libexpat.a: $(OBJ)/expat/xmlparse.o $(OBJ)/expat/xmlrole.o \
	$(OBJ)/expat/xmltok.o

$(OBJ)/libz.a: $(OBJ)/zlib/adler32.o $(OBJ)/zlib/compress.o \
	$(OBJ)/zlib/crc32.o $(OBJ)/zlib/deflate.o $(OBJ)/zlib/gzio.o \
	$(OBJ)/zlib/inffast.o $(OBJ)/zlib/inflate.o $(OBJ)/zlib/infback.o \
	$(OBJ)/zlib/inftrees.o $(OBJ)/zlib/trees.o $(OBJ)/zlib/uncompr.o \
	$(OBJ)/zlib/zutil.o

ifdef MESS
$(OBJ)/mess/%.o: mess/%.c
	$(CC_COMMENT) @echo '[MESS] Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<
endif

ifdef MAGE
$(OBJ)/mage/src/%.o: mage/src/%.c
	$(CC_COMMENT) @echo '[MAGE] Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<
#else
endif
$(OBJ)/%.o: src/%.c
	$(CC_COMMENT) @echo 'Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<
#endif

$(OBJ)/%.a:
	$(CC_COMMENT) @echo 'Archiving $@ ...'
	$(CC_COMPILE) ar $(AR_OPTS) $@ $^
	$(CC_COMPILE) $(RANLIB) $@

$(OSDEPEND): $(UNIX_OBJS)
	$(CC_COMMENT) @echo '[OSDEPEND] Archiving $@ ...'
	$(CC_COMPILE) ar $(AR_OPTS) $@ $(UNIX_OBJS)
	$(CC_COMPILE) $(RANLIB) $@

$(UNIX_OBJDIR)/%.o: src/unix/%.c src/unix/xmame.h
	$(CC_COMMENT) @echo '[OSDEPEND] Compiling $< ...'
	$(CC_COMPILE) $(CC) $(CONFIG) -o $@ -c $<

$(UNIX_OBJDIR)/%.o: %.m src/unix/xmame.h
	$(CC_COMMENT) @echo '[OSDEPEND] Compiling $< ...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -o $@ -c $<

# special cases for the 68000 core
#
# compile generated C files for the 68000 emulator
$(M68000_GENERATED_OBJS): $(OBJ)/cpu/m68000/m68kmake
	$(CC_COMMENT) @echo 'Compiling $(subst .o,.c,$@)...'
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -c $*.c -o $@

# additional rule, because m68kcpu.c includes the generated m68kops.h :-/
$(OBJ)/cpu/m68000/m68kcpu.o: $(OBJ)/cpu/m68000/m68kmake

# generate C source files for the 68000 emulator
$(OBJ)/cpu/m68000/m68kmake: src/cpu/m68000/m68kmake.c
	$(CC_COMMENT) @echo 'M68K make $<...'
	$(CC_COMPILE) $(HOST_CC) $(MY_CFLAGS) -DDOS -o $(OBJ)/cpu/m68000/m68kmake $<
	$(CC_COMMENT) @echo 'Generating M68K source files...'
	$(CC_COMPILE) $(OBJ)/cpu/m68000/m68kmake $(OBJ)/cpu/m68000 src/cpu/m68000/m68k_in.c

# generate asm source files for the 68000/68020 emulators
$(OBJ)/cpu/m68000/68000.asm:  src/cpu/m68000/make68k.c
	$(CC_COMMENT) @echo Compiling $<...
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -O0 -DDOS -o $(OBJ)/cpu/m68000/make68k $<
	$(CC_COMMENT) @echo Generating $@...
	$(CC_COMPILE) $(OBJ)/cpu/m68000/make68k $@ $(OBJ)/cpu/m68000/68000tab.asm 00

$(OBJ)/cpu/m68000/68020.asm:  src/cpu/m68000/make68k.c
	$(CC_COMMENT) @echo Compiling $<...
	$(CC_COMPILE) $(CC) $(MY_CFLAGS) -O0 -DDOS -o $(OBJ)/cpu/m68000/make68k $<
	$(CC_COMMENT) @echo Generating $@...
	$(CC_COMPILE) $(OBJ)/cpu/m68000/make68k $@ $(OBJ)/cpu/m68000/68020tab.asm 20

# generated asm files for the 68000 emulator
$(OBJ)/cpu/m68000/68000.o:  $(OBJ)/cpu/m68000/68000.asm
	$(CC_COMMENT) @echo Assembling $<...
	$(CC_COMPILE) $(ASM_STRIP) $<
	$(CC_COMPILE) nasm $(NASM_FMT) -o $@ $(subst -D,-d,$(ASMDEFS)) $<

$(OBJ)/cpu/m68000/68020.o:  $(OBJ)/cpu/m68000/68020.asm
	$(CC_COMMENT) @echo Assembling $<...
	$(CC_COMPILE) $(ASM_STRIP) $<
	$(CC_COMPILE) nasm $(NASM_FMT) -o $@ $(subst -D,-d,$(ASMDEFS)) $<

# MMX assembly language for effect filters
$(OBJ)/unix.$(DISPLAY_METHOD)/effect_asm.o: src/unix/effect_asm.asm
	$(CC_COMMENT) @echo Assembling $<...
	$(CC_COMPILE) nasm $(NASM_FMT) -o $@ $<

doc: src/unix/doc/xmame-doc.txt src/unix/doc/x$(TARGET)rc.dist doc/gamelist.$(TARGET) src/unix/doc/x$(TARGET).6

src/unix/doc/xmame-doc.txt: src/unix/doc/xmame-doc.sgml
	cd src/unix/doc; \
	sgml2txt   -l en -p a4 -f          xmame-doc.sgml; \
	sgml2html  -l en -p a4             xmame-doc.sgml; \
	sgml2latex -l en -p a4 --output=ps xmame-doc.sgml; \
	rm -f xmame-doc.lyx~

src/unix/doc/x$(TARGET)rc.dist: all src/unix/xmamerc-keybinding-notes.txt
	./x$(TARGET).$(DISPLAY_METHOD) -noloadconfig -showconfig | \
	 grep -v loadconfig | tr "\033" \# > src/unix/doc/x$(TARGET)rc.dist
	cat src/unix/xmamerc-keybinding-notes.txt >> src/unix/doc/x$(TARGET)rc.dist

src/unix/doc/gamelist.$(TARGET): all
	./x$(TARGET).$(DISPLAY_METHOD) -listgamelistheader > src/unix/doc/gamelist.$(TARGET)
	./x$(TARGET).$(DISPLAY_METHOD) -listgamelist >> src/unix/doc/gamelist.$(TARGET)

src/unix/doc/x$(TARGET).6: all src/unix/xmame.6-1 src/unix/xmame.6-3
	cat src/unix/xmame.6-1 > src/unix/doc/x$(TARGET).6
	./x$(TARGET).$(DISPLAY_METHOD) -noloadconfig -manhelp | \
	 tr "\033" \# >> src/unix/doc/x$(TARGET).6
	cat src/unix/xmame.6-3 >> src/unix/doc/x$(TARGET).6

install: $(INST.$(DISPLAY_METHOD)) install-man
	@echo $(NAME) for $(ARCH)-$(MY_CPU) installation completed

install-man:
	@echo installing manual pages under $(MANDIR) ...
	-$(INSTALL_MAN_DIR) $(MANDIR)
	$(INSTALL_MAN) src/unix/doc/x$(TARGET).6 $(MANDIR)/x$(TARGET).6

doinstall:
	@echo installing binaries under $(BINDIR)...
	-$(INSTALL_PROGRAM_DIR) $(BINDIR)
	$(INSTALL_PROGRAM) $(NAME).$(DISPLAY_METHOD) $(BINDIR)

doinstallsuid:
	@echo installing binaries under $(BINDIR)...
	-$(INSTALL_PROGRAM_DIR) $(BINDIR)
	$(INSTALL_PROGRAM_SUID) $(NAME).$(DISPLAY_METHOD) $(BINDIR)

copycab:
	@echo installing cabinet files under $(XMAMEROOT)...
	@cd src/unix; \
	for i in cab/*; do \
	if test ! -d $(XMAMEROOT)/$$i; then \
	$(INSTALL_DATA_DIR) $(XMAMEROOT)/$$i; fi; \
	for j in $$i/*; do $(INSTALL_DATA) $$j $(XMAMEROOT)/$$i; done; done

clean: 
	@rm -fr $(OBJ) $(NAME).* xlistdev $(TOOLS)

clean68k:
	@echo Deleting 68k object files...
	@rm -f $(OBJ)/cpuintrf.o
	@rm -f $(OBJ)/drivers/cps2.o
	@rm -rf $(OBJ)/cpu/m68000

cleanosd:
	@echo Deleting OSDEPEND object files...
	@rm -rf $(OBJDIR)

cleancore:
	@echo Deleting core object files...
	@if test -d $(OBJ); then \
	rm -rf `find $(OBJ) -mindepth 1 -path '$(OBJDIR)' -prune -o -print`; fi
