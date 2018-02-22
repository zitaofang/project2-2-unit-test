# project2-2-unit-test
This is a unit test suite for Project 2-2. For more information, go to the Piazza thread that leads you here.

Also, don't forget to paste this into your Makefile!

```Makefile
unit-test:
        gcc $(CFLAGS) -DTESTING -o unit-test part2_unit_test.c utils.c part1.c part2.c -l:libtestgen.a
```
