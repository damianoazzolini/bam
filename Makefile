
INC1=cudd/cudd

INCDIRS= -I$(INC1)
CFLAGSprog= $(CFLAGS) -fPIC ${INCDIRS}
LDFLAGS= $(LDSOFLAGS) -Lcudd/cudd/.libs/ -lcudd

DEBUGFLAGS = -g -O2 -Wall -Wextra -Wformat=2 -Wconversion -Wimplicit-fallthrough -Werror=format-security -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -D_GLIBCXX_ASSERTIONS -fstack-clash-protection -fstack-protector-strong -Wl,-z,nodlopen -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,--as-needed -Wl,--no-copy-dt-needed-entries -DDEBUG_MODE

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    FLAGS = $(DEBUGFLAGS)
else
    FLAGS = -O2
endif


all: bam

bam: bam.o cnf_handler.o semiring.o
	gcc bam.o cnf_handler.o semiring.o $(LDFLAGS) -o bam -lm

bam.o : src/bam.c src/cnf_handler.h src/semiring.h
	cd cudd &&  $(MAKE) && cd ..
	gcc -c $(CFLAGSprog) $(FLAGS) src/bam.c -o bam.o

cnf_handler.o: src/cnf_handler.c src/cnf_handler.h
	gcc -c $(CFLAGSprog) $(FLAGS) src/cnf_handler.c -o cnf_handler.o

semiring.o: src/semiring.c src/semiring.h
	gcc -c $(CFLAGSprog) $(FLAGS) src/semiring.c -o semiring.o

clean:
	rm *.o bam
