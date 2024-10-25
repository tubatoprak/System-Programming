
all:Compile Link

Compile:
	gcc -c appendMeMore.c part2_3.c
Link:
	gcc -o appendMeMore appendMeMore.o -lrt -pthread -lm
	gcc -o part2_3 part2_3.o -lrt -pthread -lm
clean:
	rm appendMeMore part2_3 appendMeMore.o part2_3.o