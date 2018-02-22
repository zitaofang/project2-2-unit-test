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

void assert_equal(unsigned int actual, unsigned int expect, const char* error_format, ...) {
	if (actual == expect) return;
	va_list args;
	va_start(args, error_format);
	vprintf(error_format, args);
	va_end(args);
	printf("Expect: 0x%.8x; Actual: 0x%.8x\n", expect, actual);
	// print instruction
	printf("Test case %.2d (where rs1 = 0x%.8x, rs2 = 0x%.8x):\t", cases_counter, cases[cases_counter].rs1_initial, cases[cases_counter].rs2_initial);
	decode_instruction(cases[cases_counter].instruction);
	printf("\n");
}

unsigned int PC;
Processor processor;
Byte memory[MEMORY_SPACE];
void execute_test_case() {
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
	assert_equal(processor.PC - control_group_processor.PC, cases[cases_counter].PC_offset, "Test case %d: PC offset assertion failed:\n", cases_counter);
	// Compare R
	int count;
	for (count = 0; count < 32; count++) {
		assert_equal(processor.R[count], control_group_processor.R[count], "Test case %d: x%d assertion failed:\n", cases_counter, count);
	}
	// If this is a data transfer instruction, check the memory
	if (i.opcode == 0x03 || i.opcode == 0x23) {
		int address;
		for (address = 0; address < MEMORY_SPACE; address++) {
			assert_equal(memory[address], control_group_memory[address], "Test cases %d: memory assertion failed at 0x%.8x:\n", cases_counter, address);
		}
	}

	cases_counter++;
}

int main(int arc, char **argv) {
	printf("=====================================\n");
	printf("Proj2-2 Unit Test by Zitao Fang\n");
	printf("Version: 1.0\n");
	printf("\n");
	printf("This program will use your part1 disassembler to show the instruction, so it is critical that Part 1 is correctly implemented.\n");
	printf("Please post your bug report to the Piazza thread.\n");
	printf("\n");
	printf("==========Test Output==========\n");
	srand(time(0));
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
	printf("If the test output is empty, your program pass all the tests.\n");
	printf("Otherwise, set a breakpoint with \"b part2_unit_test.c:93 if cases_counter==<Failed Test Case #>\" and start debugging.\n");
	return 0;
}
