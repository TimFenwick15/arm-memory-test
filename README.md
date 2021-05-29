Stack usage monitoring experiments.

Fill the stack with a watermark. Do some work that fills the stack. Then search the stack for the watermark. When we find the point the stack has been overwritten, we can calculate peak stack usage.

For STM32F411, the lowest stack address is 0x20000000, and the highest is 0x20005000. The first value pushed to the stack will be at location 0x20005000. Adding to the stack makes it grow 'downwards' towards 0x20000000.

The stack pointer in register r13 tells you the current top of the stack. You could in theory read this out in a super fast loop and record the peak. But this will move around quickly, and you would need to get lucky to catch it in it's extreme.

Read the stack pointer with:

`__ASM volatile ("mov %0, sp" : "=r" (stackPointer) );`

where stackPointer is a uint32_t.

We just use this to find the part of the stack we can begin writing to. If interrupts were enabled, this could be dangerous.

(I know there can be a main stack pointer and thread stack pointer, I'm not sure if that saves us here)

## Where to start
You could start at the lowest or highest address. You would expect the stack to be underutilised most of the time, so it would be more efficient to start at the highest address (the bottom of the stack).

Better, use binary search to get a ballpark location, then walk to the final answer. This wouldn't need many steps if we're reporting an int result.

## Issues
### Heap allocation
If you allocate to the heap, the heap starts to grow from the top of the stack (the lowest address), 'upwards' towards the bottom of the stack (the highest address).

This is an issue if you search from the top of the stack, but not if you binary search.

Setting the watermark risks overwriting heap allocated data. Perhaps you could allocate some test data to the heap, and make that your watermark limit.

The trace_printf heap allocates the string, so calling this overwrites some of the watermark.

Testing this, the heap data actually goes somewhere else. Don't understand this yet.

### Random and non-sequential stack writes
My recursive function seems to write some random value near the top of the stack. which means if we check each uint32_t matches the watermark value, we hit this and get the wrong answer.

Stack writes are not always contiguous with previous writes leaving some gaps.

A way around this has been to check in steps of uint64_t, checking each uint32_t for the watermark value. Only break when both uint32_t chunks have been overwritten.

## blinky.map
Lives in the output build folder (Release or Debug).

Contains a `__stack`.

"Memory Configuration" section says:
RAM              0x0000000020000000 0x0000000000005000 xrw

## ldscripts
ldscripts/sections.ld contains some stack size macros, eg `__Main_Stack_Size`

Contains some heap data too.

A nice reasource on these: https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/decoding-the-startup-file-for-arm-cortex-m4

https://engineering.stackexchange.com/questions/4013/defining-the-heap-and-stack-size-for-an-arm-cortex-m4-microcontroller

## Resources
Talks about march memory tests: https://community.arm.com/developer/tools-software/tools/f/keil-forum/43140/how-to-execute-a-memory-test

Some magic code that searches for free memory: https://os.mbed.com/questions/6800/How-to-find-the-amount-of-free-space-on-/

## Toolchain
Setup toolchain following: https://eclipse-embed-cdt.github.io/

## Stack Overflows
Talks about Barr group analysis of Toyota unintended acceleration, and a custom exception handler: https://embeddedgurus.com/state-space/2014/02/are-we-shooting-ourselves-in-the-foot-with-stack-overflow/

## Static Analysis
Requires two parts:
- Callgraph, can be generated using GNU cflow
- Stack usage per function

Then each callgraph path can have the stack contribution added up, and worst case stack usage can be calculated.

Arm linker --callgraph flag, I'm not sure if this is ld, ld doesn't seem to have --callgraph. GCC has -fstack-usage for reporting function stack usage. In Eclipse, in C/C++ Build > Settings > GNU Arm Cross C++ Compiler > Miscellaneous, you can add -fstack-usage. This outputs a .su file next to each source file listing functions and their stack use.

My recursive function used 8 bytes of the stack in my tests, this is what the output .su file reports. This doens't spot that the function is recursive of course, but we don't use recursion in embedded for this reason.

Download GNU cflow, extract (tar -xvf), enter the directory and run $ ./configure && make && make install. Then test with cflow --version.

In this dir: $ cflow src/main.cpp # shows the callgraph, including noting my evil recursive function.

Can call $ cflow src/*.c system/cortexm/src/*.c, to get several files at once. Unclear how this would handle shared code.

I suppose you could write a Python script to combine these outputs and estimate peak stack usage.

Better, there's a -fcallgraph-info=su flag which creates a callgraph labelled with -fstack-usage data, written to main.ci.

Sources:
- Found initial commands: https://embeddedartistry.com/blog/2020/08/17/three-gcc-flags-for-analyzing-memory-usage/
- Describes fcallgraph-info=su https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html

