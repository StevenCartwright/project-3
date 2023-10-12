all: master slave

master: master.c
	gcc master.c -o master

slave: slave.c
	gcc slave.c -o slave

clean:
	rm -f master slave

