ifndef BUILD_LINUX
ifndef BUILD_WINDOWS
$(error BUILD_LINUX or BUILD_WINDOWS must be set!)
endif
endif

ifdef BUILD_LINUX
ifdef BUILD_WINDOWS
$(error BUILD_LINUX and BUILD_WINDOWS may not be set at the same time!)
endif
endif

ifdef BUILD_LINUX
	SRC += 
	CSRC += 

	INSTALL_HEADERS_DIR = /usr/local/include/
	INSTALL_LIBRARY_DIR = /usr/local/lib/
	BINNAME = libscl.a

	LDLIBS += 
endif

ifdef BUILD_WINDOWS
	CXX = x86_64-w64-mingw32-g++
	CC = x86_64-w64-mingw32-gcc

	SRC += 
	CSRC += 

	INSTALL_HEADERS_DIR = /usr/x86_64-w64-mingw32/usr/include/
	INSTALL_LIBRARY_DIR = /usr/x86_64-w64-mingw32/usr/lib/

	BINNAME = libscl.a

	# because of mingw32 & linux crossdev,
	# have to statically link the C++ runtime
	LDFLAGS += -static-libgcc -static-libstdc++ -static
	LDLIBS  += 
	LDFLAGS += 
endif

SRC=
INCLUDE=
CSRC=

PUBLIC_HEADERS=src/scl.hh

CXXFLAGS=-MMD @compile_flags.txt 
CFLAGS=-MMD @ccompile_flags.txt 

ifdef RELEASE
	CXXFLAGS += -O2
else
	CXXFLAGS += -Og -g
endif

SRCS=src/scl.cc
OBJS=$(SRCS:.cc=.o)
CSRCS=
COBJS=$(CSRCS:.c=.o)

LDLIBS +=
LDFLAGS +=

default: scl

clean:
	rm $(OBJS) $(COBJS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) $^ -c -o $@

%.o: %.c
	$(CC) $(CFLAGS) $^ -c -o $@

scl: $(OBJS) $(COBJS)
	$(AR) rcs bin/$(BINNAME) $(OBJS) $(COBJS)

test: scl
	$(CXX) $(CXXFLAGS) test/main.cc -Lbin/ -lscl -o bin/test.out

install:
	cp bin/$(BINNAME) $(INSTALL_LIBRARY_DIR)
	cp $(PUBLIC_HEADERS) $(INSTALL_HEADERS_DIR)

