<h2>Malloc</h2>
<p>Malloc written in C. Uses an implicit free list to manage the memory blocks.</p>
<p>Memory is managed using sbrk() and brk() system calls.</p>
<p>To prevent inconsistency caused by other functions calling sbrk, 2 GB are reserved from the heap with the first call to malloc and freed when all the blocks are freed.</p>
<h2>Compile</h2>
<p><code>gcc -g test.c -o test mm.c memlib.c</code></p>
<h2>License</h2>
<p>MIT</p>
