implant: vcrt0.o implant.o choosedir.o libsyscalls.a
	ld -Bstatic -dc -dp -e start -X -o implant vcrt0.o implant.o choosedir.o \
  -L. -lsyscalls

implant2: vcrt0.o implant2.o choosedir.o shortform.o
	ld -Bstatic -dc -dp -e start -X -o implant2 vcrt0.o implant2.o choosedir.o \
   shortform.o

implant3: vcrt0.o implant2.o choosedir.o shortform.o
	/home/bediger/bin/ld -Bstatic -dc -dp -e start -X -o implant3 vcrt0.o implant2.o choosedir.o \
   shortform.o

implanter: implanter.c
	cc -o implanter implanter.c

chtest: chtest.c choosedir2.c
	cc -g -o chtest chtest.c

implant.o: implant.c
	cc -c -O implant.c
implant2.o: implant2.c
	cc -c -O implant2.c
choosedir.o: choosedir.c
	cc -c -O choosedir.c


OBJS = exit.o read.o write.o close.o open.o fstat.o strlen.o bcopy.o \
  lseek.o
  

ASFLAGS = -P

libsyscalls.a: $(OBJS)
	ar rv libsyscalls.a $(OBJS)
	ranlib libsyscalls.a


tar:
	tar cvf vtest.tar *.c *.s makefile README
	
shar:
	shar *.c *.s makefile README > vtest.shar
	
