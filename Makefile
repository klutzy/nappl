EXESRC=nappl.cpp
EXERSC=
DLLSRC=nadll.cpp

EXEOBJ=$(EXESRC:.cpp=.o) $(EXERSC:.rc=.res)
DLLOBJ=$(DLLSRC:.cpp=.o)
EXE=nappl.exe
DLL=nadll.dll

CC=mingw32-gcc
CPP=mingw32-g++
RES=windres
RM=rm

CFLAGS=-Wall -O3 -std=c++0x -DUNICODE -D_UNICODE -DWIN32 -D_WINDOWS -DNADLL=L\"$(DLL)\"
LDFLAGS=-mwindows -lstdc++

.PHONY: all
all: $(EXE) $(DLL)

$(EXE): $(EXEOBJ)
	$(CC) $(EXEOBJ) $(LDFLAGS) -o $(EXE)

$(DLL): $(DLLOBJ)
	$(CC) $(DLLOBJ) $(LDFLAGS) -o $(DLL) -shared

%.o: %.cpp
	$(CPP) $(CFLAGS) -c -o $@ $<

%.res: %.rc
	$(RES) -O coff -o $@ $<

clean:
	$(RM) -rf $(EXEOBJ) $(DLLOBJ) $(EXE) $(DLL)
