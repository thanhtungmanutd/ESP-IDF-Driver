# IPC - Inter Process Communication #

## Pipe ##

Pipe is a technique that allows 2 or more processes can exchange data with each others. A process can only read or write to a pipe at the same time. Read from an empty pipe or write to a full pipe will leads to error. Pipe can be one-way or two-way direction.

### Advantages and Disadvantages of using pipe ###

- **Advantages:**

  - Pipe is simple to implement.
  - Pipe is efficient and reliable when transferring data between processes.

- **Disadvantages:**

  - Pipe has limitted size. Therefore, pipe can not deal with big amount of data.
  - Unidirectional (one-way) pipe: A process can not read from and write to a pipe at the same time.


### Named pipe ###

- Using FIFO mechanism to exchange data between 2 or more unrelated processes. 

### Unnamed pipe ###
