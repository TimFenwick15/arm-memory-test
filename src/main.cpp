#include <stdio.h>
#include <stdlib.h>

#include "diag/trace.h"
#include "cmsis_device.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

#define STACK_START 0x20000000UL
#define STACK_STOP  0x20005000UL

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
  volatile uint32_t stackPointer;
  __ASM volatile ("mov %0, sp" : "=r" (stackPointer) );

  volatile unsigned int* stack = (unsigned int*) stackPointer;

  // Initialise the stack to 0xDEAD0000
  while (stack >= (unsigned int*) STACK_START)
  {
	  *stack-- = 0xDEAD0000u; // in steps of uint32s, because of the type I think
  }

  // Do something expensive
  cLovesRecursion(1); // random number dropped in stack at 0x20000222



  // Calculate stack usage
  __ASM volatile ("mov %0, sp" : "=r" (stackPointer) );

  uint32_t usagePercentage = ((STACK_STOP - stackPointer) * 100)
		  / (STACK_STOP - STACK_START);

  bool armStackPointerFound = false; // try a workaround for the random number posted to the stack
  volatile uint32_t* searchStackPointer = (uint32_t*)STACK_START;
  do
  {
	  if (*searchStackPointer != 0xDEAD0000)
	  {
		  if (armStackPointerFound)
		  {
			// Note that printf has quite a large stack allocation, and allocates to the heap too
			// We would need to reset after using this.
	        trace_printf("+ ADDR is %u, VALUE is %u\n", searchStackPointer, *searchStackPointer);
		    break;
		  }
		  else
		  {
		    armStackPointerFound = true;
		  }
	  }
	  else
	  {
		  armStackPointerFound = false;
	  }
	  *searchStackPointer++; // We have to walk from the end of the stack up to the top
	                         // because stack is not always filled contiguously, leaving our watermark intact
  } while ((uint32_t)searchStackPointer < STACK_STOP);

  uint32_t searchUsagePercentage = ((STACK_STOP - (uint32_t)searchStackPointer) * 100)
				  / (STACK_STOP - STACK_START);

  // Reset the stack

  // Recalculate

  // Report back

  // These messages show near the end of the stack, we've done some heap allocation :o
  trace_printf ("SP read usage %u percent\n", usagePercentage);
  trace_printf ("Calc usage    %u percent\n", searchUsagePercentage);
  trace_printf ("-----END-----\n");

  return 0;
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
