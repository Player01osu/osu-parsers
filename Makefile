AR := ar
CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -Wno-unused-function -std=c99 -ggdb
UTILS := string_builder.c string_builder.h xutils.h qarray.h xerror.h
EASYLZMA := easylzma-master/build/easylzma-0.0.8/lib/libeasylzma_s.a

all: osr_tools static

static: libosr_parser.a

# XXX: Currently does not link correctly
shared: libosr_parser.so

libosr_parser.a: osr_parser.o binary_parser.o string_builder.o $(EASYLZMA)
	$(AR) x $(EASYLZMA)
	$(AR) rc libosr_parser.a *.o

libosr_parser.so: osr_parser.o binary_parser.o string_builder.o $(EASYLZMA)
	$(AR) x $(EASYLZMA)
	$(CC) -shared -o libosr_parser.so osr_parser.o binary_parser.o string_builder.o -Wl,--whole-archive easylzma-master/src/lib/libeasylzma_s.a -Wl,--no-whole-archive

string_builder.o: string_builder.c string_builder.h xutils.h
	$(CC) -fPIC -c -o string_builder.o string_builder.c $(CFLAGS)

binary_parser.o: binary_parser.c binary_parser.h $(UTILS)
	$(CC) -fPIC -c -o binary_parser.o binary_parser.c $(CFLAGS)

osr_parser.o: osr_parser.c osr_parser.h binary_parser.c binary_parser.h $(UTILS)
	$(CC) -fPIC -c -o osr_parser.o osr_parser.c $(CFLAGS)

osr_tools: osr_tools.c libosr_parser.a
	$(CC) -o osr_tools osr_tools.c libosr_parser.a $(CFLAGS)

clean_obj:
	rm -f *.o

clean_static:
	rm -f osr_parser.a

clean: clean_obj clean_static
