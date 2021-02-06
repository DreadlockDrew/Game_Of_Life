CC := gcc
CFLAGS := -g -Wall -Werror -O -std=c99 -D_DEFAULT_SOURCE
LDFLAGS := -lncurses

SOURCEFILES := parser.c term.c main.c

all: life

life: $(patsubst %.c,src/%.o,$(SOURCEFILES))
	$(CC) -o $@ $^ $(LDFLAGS)

test: life
	@echo
	@echo "Testing a static block;"
	@echo
	./life inputs/block.life 0 > outputs/block.output.txt
	@cmp -s outputs/block.expected.txt outputs/block.output.txt \
	    || printf "\nblock not as expected at generation 0\n\n"
	@echo
	@echo "Testing an oscillator with period 2 for three generations"
	@echo
	./life inputs/blinker.rle 0 > outputs/blinker.output.txt
	./life inputs/blinker.rle 1 >> outputs/blinker.output.txt
	./life inputs/blinker.rle 2 >> outputs/blinker.output.txt
	@cmp -s outputs/blinker.expected.txt outputs/blinker.output.txt \
	    || printf "\nblinker not as expected for first three generations\n\n"
	@echo
	@echo "Testing an oscillator with period 3 for four generations"
	@echo
	./life inputs/pulsar.rle 0 > outputs/pulsar.output.txt
	./life inputs/pulsar.rle 1 >> outputs/pulsar.output.txt
	./life inputs/pulsar.rle 2 >> outputs/pulsar.output.txt
	./life inputs/pulsar.rle 3 >> outputs/pulsar.output.txt
	@cmp -s outputs/pulsar.expected.txt outputs/pulsar.output.txt \
	    || printf "\npulsar not as expected for first four generations\n\n"
	@echo
	@echo "Testing an oscillator with period 72;"
	@echo
	./life inputs/2blockrpent.rle 0 > outputs/2blockrpent-0.output.txt
	@cmp -s outputs/2blockrpent-0.expected.txt outputs/2blockrpent-0.output.txt \
	    || printf "\n2blockrpent not as expected at generation 0\n\n"
	./life inputs/2blockrpent.rle 72 > outputs/2blockrpent-72.output.txt
	@cmp -s outputs/2blockrpent-72.expected.txt outputs/2blockrpent-72.output.txt \
	    || printf "\n2blockrpent not as expected at generation 72\n\n"
	@echo

submission: life.tar

life.tar: $(patsubst %,src/%,$(SOURCEFILES)) Makefile
	tar cf $@ $^

%.o: %.c Makefile
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f *~ src/*~ src/*.o life life.tar outputs/*.output.txt
	rm -f core core.* vgcore.*

.PHONY: all clean submission
