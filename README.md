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
If you allocate to the heap, the heap starts to grow from the top of the stack (the lowest address), 'upwards' towards the bottom of the stack (the highest address.

This is an issue if you search from the top of the stack, but not if you binary search.

Setting the watermark risks overwriting heap allocated data. Perhaps you could allocate some test data to the heap, and make that your watermark limit.

The trace_printf heap allocates the string, so calling this overwrites some of the watermark.

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

## Resources
Talks about march memory tests: https://community.arm.com/developer/tools-software/tools/f/keil-forum/43140/how-to-execute-a-memory-test

Some magic code that searches for free memory: https://os.mbed.com/questions/6800/How-to-find-the-amount-of-free-space-on-/

