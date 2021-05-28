#include <stdio.h>
#include <stdlib.h>

#include "diag/trace.h"
#include "cmsis_device.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

#define STACK_2_PERCENT     50
#define STACK_20_PERCENT   500
#define STACK_80_PERCENT  2000
#define STACK_99_PERCENT  2500
#define STACK_100_PERCENT 3000 // Crash!

int cLovesRecursion(int n)
{
	if (n == STACK_99_PERCENT)
	{
		return n;
	}
	return cLovesRecursion(n + 1);
}

int main (int argc, char* argv[])
{
  // Get the current stack pointer
  volatile uint32_t stackPointer;
  __ASM volatile ("mov %0, sp" : "=r" (stackPointer) );
  volatile unsigned int* stack = (unsigned int*) stackPointer;

  // Get the memory start and length
  extern unsigned int _mystackstart, _mystacklength;
  uint32_t stackStart = (uint32_t)((uint32_t*)&_mystackstart);
  uint32_t stackLength = (uint32_t)((uint32_t*)&_mystacklength);

  // Initialise the stack to 0xDEAD0000
  while (stack >= (unsigned int*) stackStart)
  {
	  *stack-- = 0xDEAD0000u; // in steps of uint32s, which is the size of the pointer type
  }

  /*
  uint32_t* heap1 = (uint32_t*) malloc(4);
  uint32_t* heap2 = (uint32_t*) malloc(4);
  *heap1 = 0x41414141;
  trace_printf("Heap var here %u is value %u", heap1, *heap1);
  trace_printf("Heap var here %u is value %u", heap2, *heap2);
  */

  // Do something expensive
  cLovesRecursion(1); // random number dropped in stack at 0x20000222

  // Calculate stack usage
  __ASM volatile ("mov %0, sp" : "=r" (stackPointer) );

  uint32_t usagePercentage = ((stackStart + stackLength - stackPointer) * 100)
		  / stackLength;

  volatile uint64_t* searchStackPointer = (uint64_t*)stackStart;
  do
  {
	  uint32_t lowWord = (*searchStackPointer) & 0xFFFFFFFF;
	  uint32_t highWord = (*searchStackPointer >> 32) & 0xFFFFFFFF;

      if (lowWord != 0xDEAD0000 && highWord != 0xDEAD0000)
	  {
		    break;
	  }
	  *searchStackPointer++; // We have to walk from the end of the stack up to the top
	                         // because stack is not always filled contiguously, leaving our watermark intact
  } while ((uint32_t)searchStackPointer < (stackStart + stackLength));

  uint32_t searchUsagePercentage = ((stackStart + stackLength - (uint32_t)searchStackPointer) * 100)
				  / (stackLength);

  // Reset the stack

  // Recalculate

  // Report back

  // These messages show near the end of the stack, we've done some heap allocation :o
  // Note that printf has quite a large stack allocation, and allocates to the heap too
  // We would need to reset after using this.
  trace_printf("+ ADDR is %u, VALUE is %u\n", searchStackPointer, *searchStackPointer);
  trace_printf ("SP read usage %u percent\n", usagePercentage);
  trace_printf ("Calc usage    %u percent\n", searchUsagePercentage);
  trace_printf ("-----END-----\n");

  return 0;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
