#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "riscv.h"

struct test_case {
	unsigned int instruction;
	unsigned int rd_mem_value;
	unsigned int address;
	unsigned int mem_mask;
	unsigned int rs1_initial;
	unsigned int rs2_initial;
	unsigned int PC_offset;
};
int build_test_suite(struct test_case* buffer);

struct test_case* cases;
unsigned int cases_counter = 0;
int verbose = 0;

void print_test_case() {
	printf("Test case %2d (where rs1 = 0x%.8x, rs2 = 0x%.8x):\t", cases_counter, cases[cases_counter].rs1_initial, cases[cases_counter].rs2_initial);
	decode_instruction(cases[cases_counter].instruction);
}

int assert_equal(int is_memory, unsigned int actual, unsigned int expect, const char* error_format, ...) {
	if (actual == expect) {
		return 1;
	}
	if(!verbose)
		print_test_case();
	va_list args;
	va_start(args, error_format);
	vprintf(error_format, args);
	va_end(args);
	if (is_memory)
		printf("Expect: 0x%.2x; Actual: 0x%.2x\n", expect, actual);
	else
		printf("Expect: 0x%.8x; Actual: 0x%.8x\n", expect, actual);
	// print instruction
	printf("\n");
	return 0;
}

unsigned int PC;
Processor processor;
Byte memory[MEMORY_SPACE];
void execute_test_case() {
	if(verbose)
		print_test_case();
	int assertion_result = 0;
	processor.PC = (rand() >> 22) << 2;
	processor.R[0] = 0;
	Instruction i;
	Processor control_group_processor;
	Byte control_group_memory[MEMORY_SPACE];
	i.bits = cases[cases_counter].instruction;
	// Get the instruction type so that it can initialize the registers (and memory)
	switch (i.opcode)
	{
	case 0x33: // R
	case 0x23: // S
	case 0x63: // SB
		processor.R[i.rtype.rs2] = cases[cases_counter].rs2_initial;
	case 0x13: // I
		processor.R[i.rtype.rs1] = cases[cases_counter].rs1_initial;
	case 0x37: // U
	case 0x6F: // UJ
	case 0x73: // (ecall)
		break;
	case 0x03: // (L)
		processor.R[i.rtype.rs1] = cases[cases_counter].rs1_initial;
		unsigned int* memory_location = (unsigned int*) &memory[cases[cases_counter].address];
		*memory_location &= ~cases[cases_counter].mem_mask;
		*memory_location |= cases[cases_counter].rd_mem_value & cases[cases_counter].mem_mask;
		break;
	default:
		break;
	}
	// Prepare for the control group
	memcpy(&control_group_processor, &processor, sizeof(processor));
	memcpy(&control_group_memory, &memory, sizeof(memory));
	switch (i.opcode) {
	case 0x33:
	case 0x13:
	case 0x3:
	case 0x37:
		control_group_processor.R[i.rtype.rd] = cases[cases_counter].rd_mem_value;
	case 0x73:
	case 0x63:
		break;
	case 0x6F: // jal
		control_group_processor.R[i.rtype.rd] = control_group_processor.PC + 4;
		break;
	case 0x23:
		// Store
		;
		unsigned int* memory_location = (unsigned int*) &control_group_memory[cases[cases_counter].address];
		*memory_location &= ~cases[cases_counter].mem_mask;
		*memory_location |= cases[cases_counter].rd_mem_value & cases[cases_counter].mem_mask;
		break;
	}

	// Execute!
	execute_instruction(i.bits, &processor, memory);

	// Clear x0
	processor.R[0] = 0;
	// Compare PC
	assertion_result = assert_equal(0, processor.PC - control_group_processor.PC, cases[cases_counter].PC_offset, "PC offset assertion failed:\n");
	// Compare R
	int count;
	for (count = 0; count < 32; count++) {
		assertion_result = assert_equal(0, processor.R[count], control_group_processor.R[count], "Register x%d assertion failed:\n", count);
	}
	// If this is a data transfer instruction, check the memory
	if (i.opcode == 0x03 || i.opcode == 0x23) {
		int address;
		for (address = 0; address < MEMORY_SPACE; address++) {
			assertion_result = assert_equal(1, memory[address], control_group_memory[address], "Memory assertion failed at address 0x%.8x:\n", address);
		}
	}

	if (verbose && assertion_result == 1)
		printf("Test Passed\n");
	cases_counter++;
}

int main(int arc, char **argv) {
	int arg_iter = 1;
	int seed = 0;
	while (arg_iter < arc) {
		verbose = strcmp(argv[arg_iter], "-v") == 0;
		int arg_seed = atoi(argv[arg_iter]);
		if (arg_seed != 0)
			seed = arg_seed;
		arg_iter++;
	}
	if (seed == 0)
		seed = time(0);
	// Argument
	printf("=====================================\n");
	printf("Proj2-2 Unit Test by Zitao Fang\n");
	printf("Version: 1.1\n");
	printf("\n");
	printf("This program will use your part1 disassembler to show the instruction, so it is critical that Part 1 is correctly implemented.\n");
	printf("Please post your bug report to the Piazza thread.\n");
	printf("\n");
	printf("The random number seed for this test suite is %d.\n", seed);
	printf("If you need to reproduce this suite, enter the seed as a command line argument.\n");
	printf("To enable verbose mode (show all test case even if your code passed them), use \"-v\".\n");
	printf("==========Test Output==========\n");
	srand(seed);

	struct test_case cases_array[100];
	cases = cases_array;
	int test_count = build_test_suite(cases);
	// execute test cases
	int i;
	for (i = 0; i < test_count; i++) {
		execute_test_case();
	}
	printf("==========Test Output==========\n");
	printf("\n");
	if(!verbose)
		printf("If the test output is empty, your program pass all the tests.\n");
	printf("If a test case failed, you can set a breakpoint with \"b part2_unit_test.c:108 if cases_counter==<Failed Test Case #>\" and start debugging.\n");
	printf("e.g. If the test case labeled 16 failed, type \"b part2_unit_test.c:108 if cases_counter==16\" in (c)gdb.\n");
	return 0;
}
