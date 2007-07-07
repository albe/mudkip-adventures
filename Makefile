CONSOLE = true # false # set to true to enable ingame console

TARGET = mudkip
OBJS = mudkip.o ../../triAudioLib.o ../../triAt3.o ../../triWav.o ../../streams/streams.o ../../triVAlloc.o ../../triMemory.o \
       ../../triRefcount.o ../../triImage.o ../../rle.o ../../triGraphics.o ../../triLog.o ../../triInput.o ../../triFont.o \
       ../../triTimer.o

INCDIR = 
CFLAGS = -O3 -G0 -Wall -Wfatal-errors -DTRI_SUPPORT_PNG -D__PSP__ -g -DDEBUG -D_DEBUG -D_DEBUG_LOG -D_DEBUG_MEMORY
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

#ifeq ($(CONSOLE),true)
#CFLAGS += -DDEBUG_CONSOLE
#OBJS +=  ../../triConsole.o
#endif


LIBS = -lpspaudiocodec -lpspaudio -lpspgum -lpspgu -lpsprtc -lpng -lfreetype -lz -lm

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_ICON = ICON0.PNG
PSP_EBOOT_PIC1 = PIC0.PNG
PSP_EBOOT_TITLE = Mudkip Adventures

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

