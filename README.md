# uthread: A User Space Thread Library

Inspired by [fiber](https://github.com/sysprog21/fiber), I implement 
uthread on the foundation of my previous project [user mode thread library](https://github.com/quantabase13/user_mode_thread_lib).

The main difference between "uthread" and "user mode thread library" is 
the scheduling mechanism.

"uthread" use O(1) scheduler, while "user mode thread library" use 
Round-Robin scheduler.

I refer to [piko/RT](https://github.com/PikoRT/pikoRT) when implementing
O(1) scheduler.