CFLAGS=-Wall -Werror
MINGW32CC=i586-mingw32msvc-gcc

SRCS=hfscommon.c hfspatchnoautofolder.c hfsunhide.c \
     hfsdisplay.c hfsrename.c
OBJS=$(addsuffix .o, $(basename $(SRCS)))
WIN32OBJS=$(addsuffix .w32.o, $(basename $(SRCS)))
BINS=hfsdisplay hfspatchnoautofolder hfsunhide hfsrename
EXES=$(addsuffix .exe, $(BINS))
all:	$(addprefix bin/, $(BINS) $(EXES))

depend:
	makedepend -Y $(addprefix src/, $(SRCS)) 2>/dev/null

clean:
	$(RM) $(addprefix objs/, $(OBJS) $(WIN32OBJS))
	$(RM) $(addprefix bin/, $(BINS) $(EXES))

dist:	all
	zip dist/`date +%Y%m%d-%H%S`_hfspluspatch.zip $(addprefix bin/, $(EXES)) src/*.c src/*.h Makefile

distbin:	$(addprefix bin/, $(BINS) $(EXES))
	zip -j dist/`date +%Y%m%d-%H%S`_hfspluspatch_bin.zip $^

$(addprefix bin/, $(BINS)):	bin/%:	objs/%.o objs/hfscommon.o
	$(CC) -o $@ $^

$(addprefix bin/, $(EXES)): bin/%.exe:	objs/%.w32.o objs/hfscommon.w32.o
	$(MINGW32CC) -o $@ $^

objs/%.o:	src/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

objs/%.w32.o:	src/%.c
	$(MINGW32CC) -c $(CFLAGS) -o $@ $<

# DO NOT DELETE

src/hfscommon.o: src/hfscommon.h
src/hfspatchnoautofolder.o: src/hfscommon.h
src/hfsunhide.o: src/hfscommon.h
src/hfsdisplay.o: src/hfscommon.h
src/hfsrename.o: src/hfscommon.h
