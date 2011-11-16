#Makefile for srvfrm application

CC=gcc
CXX=g++

CFLAGS= -g -O2 -DHAVE_SSL -Wall
CXXFLAGS=$(CFLAGS) 
INCS=-I. -I/usr/include/	-I/ilib/ncurses54/include/ncurses -Irealrtsp
LIBS=-L/usr/lib -L/ilib/ncurses54/lib -L/ilib/openssl097g/lib  -lpthread  \
		-lpanel  -lmenu -lform  -lncurses -lssl -lcrypto
#  -lpanel_g  -lmenu_g -lform_g  -lncurses_g
#  -lpanel  -lmenu -lform  -lncurses

#a : 
#	INCLUDE	"realrtsp/Makefile"	

OBJS = mwget.o ksocket.o headers.o url.o threadcacl.o progress.o options.o retrieve.o ftp.o http.o cookie.o \
		mwui.o log.o main.o init.o safe-ctype.o  utils.o hash.o msgbus.o getopt.o test.o \
		mmscore.o rtspcore.o \
		realrtsp/asmrp.o   realrtsp/real.o  realrtsp/rmff.o  \
		realrtsp/rtsp.o realrtsp/sdpplin.o  realrtsp/xbuffer.o		 realrtsp/rtsp_session.o  
#realrtsp/asmrp.o  realrtsp/real.o  realrtsp/rmff.o  realrtsp/rtsp.o  realrtsp/rtsp_session.o  realrtsp/sdpplin.o  realrtsp/xbuffer.o		

		
TARGET = mwget.exe
SRVTEST = 

all : $(TARGET) 

$(TARGET):$(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^  $(LIBS)  

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(INCS)

%.o : %.cpp
	$(CXX) $(CFLAGS) -c -o $@ $^ $(INCS) 

realrtsp/%.o : realrtsp/%.c
	$(CC) $(CFLAGS) -c -o $@ $^ $(INCS)



clean:
	rm -f $(TARGET) $(OBJS)

