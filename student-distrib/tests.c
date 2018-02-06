#include "tests.h"
#include "x86_desc.h"
#include "paging.h"
#include "lib.h"
#include "keyboard.h"
#include "rtc.h"
#include "file_system.h"
#include "syscall.h"
#include "malloc.h"
#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

/* Checkpoint 1 tests */

/* idt_test()
 * Asserts that first 20 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 */
int idt_test() {
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 20; i++){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/* pd_test()
 * Checks that PDE[0] is Present and points to PT
 * Checks that PDE[1] is Present and used as Extended 4MB for Kernel
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: PD Setup
 */
int pd_test() {
	TEST_HEADER;
	
	int result = PASS;
	// Check that PDE[0] is Present
	if (page_directory[0].present) {
		// Check that PDE[0] points to PT
		if ((page_directory[0].page_addr) != (((uint32_t) page_table) >> PT_ADDR_OFFSET)) {
			result = FAIL;
			printf("FATAL: PDE[0] does not point to PT \n");
			while(1);
		}
	}
	else {
		result = FAIL;
		printf("FATAL: PDE[0] is NOT Present \n");
		while(1);
	}
	// Check that PDE[1] is Present
	if (page_directory[1].present) {
		// Check that PDE[1] points to PT
		if (!page_directory[1].size) {
			result = FAIL;
			printf("FATAL: PDE[1] is not 4MB \n");
			while(1);
		}
	}
	else {
		result = FAIL;
		printf("FATAL: PDE[1] is NOT Present \n");
		while(1);
	}
	
	return result;
}

/* pt_test()
 * Checks that PTE[0] is Not Present
 * Checks that PTE[1-1023] is Present
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: PT Setup
 */
int pt_test() {
	TEST_HEADER;
	
	int i;
	int result = PASS;
	// Check that PTE[0] is Not Present
	if (page_table[0].present) {
		result = FAIL;
		printf("FATAL: PTE[0] is Present \n");
		while(1);
	}
	
	// Check that PTE[1-1023] is Present
	for (i = 1; i < MAX_PAGE_TABLE_SIZE; i++) {
		if (page_table[i].present == 0) {
			result = FAIL;
			printf("FATAL: PTE[%d] is not Present \n", i);
			while(1);
		}
	}
	
	return result;
}

/* div_zero_test()
 * Checks whether a Division by 0 will trigger Exception
 * Inputs: None
 * Outputs: Fail if returns, Pass if Exception raised
 * Side Effects: Crashes the System on Pass
 * Coverage: IDT, Exception Handler
 */
int div_zero_test() {
	TEST_HEADER;
	
	int result = FAIL;
	
	int a = 0;
	int b = 1;
	int c;
	c = b / a;
	
	return result;
}

/* page_fault_test()
 * Checks whether a dereference of a Null pointer or unmapped
 * memory address will trigger a page fault
 * Inputs: None
 * Outputs: Fail if returns, Pass if Exception raised
 * Side Effects: Crashes the System on Pass
 * Coverage: PD, PT, IDT
 */
int page_fault_test() {
	TEST_HEADER;
	
	int result = FAIL;
	
	int * valid_ptr;
	int * inval_ptr;
	int val;
	
	// Valid Pointer Points to Address in Kernel Page
	valid_ptr = (int *) 0x00400010;
	val = *valid_ptr;
	
	// Invalid Pointer Points to Unmapped Address 0x00000000
	inval_ptr = (int *) 0x0F000000; //NULL;
	val = *inval_ptr;
	
	return result;
}

/* Checkpoint 2 tests */

// Removed, check backup files 
typedef struct linklist{
	uint32_t* next;
	uint32_t value;
}link_t;

/* Checkpoint 3 tests */

int malloc_tests(){
	int i;
	link_t* cur;
	link_t* head;
	cur=(link_t*)malloc(sizeof(link_t));
	head=cur;
	cur->value=0;
	for (i=1;i<10;i++){
		cur->next=(uint32_t*)malloc(sizeof(link_t));
		cur=(link_t*)cur->next;
		cur->value=i;
		cur->next=NULL;
	}
	while(head!=NULL){
		printf("%d",head->value);
		head=(link_t*)head->next;
	}
	free(NULL);
}
// Combined 
int combined_cp3_tests() {
	TEST_HEADER;
	
	unsigned char cbuf[32];
	
	// Test Execute on NULL String
	if (execute(NULL) != -1) return FAIL;
	
	// Test Open on NULL String
	if (open(NULL) != -1) return FAIL;
	
	// Test Close with an Unopened FD
	if (close(2) != -1) return FAIL;
	
	// Read/Write on Unopened File
	if (read(2, cbuf, 4) != -1) return FAIL;
	if (write(2, cbuf, 4) != -1) return FAIL;
	
	// Open FD 2
	open((const uint8_t*) "frame0.txt");
	
	// Read/Write with NULL Buffer
	if (read(2, NULL, 4) != -1) return FAIL;
	if (write(2, NULL, 4) != -1) return FAIL;
	
	// Read/Write with Negative Size
	if (read(2, cbuf, -4) != -1) return FAIL;
	if (write(2, cbuf, -4) != -1) return FAIL;
	
	return PASS;	
}

/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point */
void launch_tests() {
	
	/* Checkpoint 1 Tests */
	{
		/* Test IDT Contents */
		// TEST_OUTPUT("idt_test", idt_test());
		/* Test PD Contents */
		// TEST_OUTPUT("pd_test", pd_test());
		/* Test PT Contents */
		// TEST_OUTPUT("pt_test", pt_test());
		// WARN: THESE TESTS WILL CRASH THE SYSTEM ON PASS
		// ***********************************************
		/* Test Division by 0 Exception */
		// TEST_OUTPUT("div_zero_test", div_zero_test());
		/* Test Page Fault */
		// TEST_OUTPUT("page_fault_test", page_fault_test());
		// *********************************************** 

	}
	
	/* Checkpoint 2 Tests */
	{
		// Removed, check backup files
		//read_by_data_txt_test(44);
	}
	
	/* Checkpoint 3 Tests */
	{
		// Combined Tests
		//TEST_OUTPUT("combined_cp3_tests", combined_cp3_tests());
		TEST_OUTPUT("malloc_tests",malloc_tests());
	}
	
	/* Checkpoint 4 Tests */
	/* Checkpoint 5 Tests */
}
