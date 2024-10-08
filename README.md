# **mlog** is a logging utility that supports the following features:

- **Log Levels**: ERROR, WARN, INFO, DEBUG
- **Log Format**: Time [Level] PID#TID Function#Line: Log Message
- **Variadic Macros**: Handles a variable number of arguments
- **Thread Safety**: Ensures safe logging from multiple threads
- **Timestamp-Based Sorting**: Logs are recorded in chronological order

# Build Flag

- LDFLAG: -lpthread -lm

- ASAN OPTIONS: -fsanitize=address -static-libasan

- DEBUG OPTIONS: -g -O0 -DDEBUG

# Example Result

## test_single_thread.c

```c++
2024/10/08 12:35:52 [error] 207938#207938 main#23: [0] test 111 0
2024/10/08 12:35:52 [error] 207938#207938 main#23: [1] test 222 10
2024/10/08 12:35:52 [error] 207938#207938 main#23: [2] test 333 20
2024/10/08 12:35:52 [error] 207938#207938 main#23: [3] test 444 30
2024/10/08 12:35:52 [error] 207938#207938 main#29: >>>>> change log level to debug
2024/10/08 12:35:52 [error] 207938#207938 main#34: [0] test 111 0
2024/10/08 12:35:52 [warn] 207938#207938 main#35: [0] test 111 0
2024/10/08 12:35:52 [info] 207938#207938 main#36: [0] test 111 0
2024/10/08 12:35:52 [debug] 207938#207938 main#37: [0] test 111 0
2024/10/08 12:35:52 [error] 207938#207938 main#34: [1] test 222 10
2024/10/08 12:35:52 [warn] 207938#207938 main#35: [1] test 222 10
2024/10/08 12:35:52 [info] 207938#207938 main#36: [1] test 222 10
2024/10/08 12:35:52 [debug] 207938#207938 main#37: [1] test 222 10
2024/10/08 12:35:52 [error] 207938#207938 main#34: [2] test 333 20
2024/10/08 12:35:52 [warn] 207938#207938 main#35: [2] test 333 20
2024/10/08 12:35:52 [info] 207938#207938 main#36: [2] test 333 20
2024/10/08 12:35:52 [debug] 207938#207938 main#37: [2] test 333 20
2024/10/08 12:35:52 [error] 207938#207938 main#34: [3] test 444 30
2024/10/08 12:35:52 [warn] 207938#207938 main#35: [3] test 444 30
2024/10/08 12:35:52 [info] 207938#207938 main#36: [3] test 444 30
2024/10/08 12:35:52 [debug] 207938#207938 main#37: [3] test 444 30
```

## test_mult_thread.c

```c++
2024/10/08 12:40:33 [info] 207881#207883 thread_func1#31: thread 207883 loop count=1 start ...
2024/10/08 12:40:33 [error] 207881#207883 thread_func1#37: thread 207883 count=1
2024/10/08 12:40:33 [info] 207881#207884 thread_func1#31: thread 207884 loop count=2 start ...
2024/10/08 12:40:33 [info] 207881#207885 thread_func1#31: thread 207885 loop count=3 start ...
2024/10/08 12:40:33 [warn] 207881#207884 thread_func1#35: thread 207884 count=2
2024/10/08 12:40:33 [error] 207881#207881 main#100: [0] test 111 0
2024/10/08 12:40:33 [error] 207881#207885 thread_func1#37: thread 207885 count=3
2024/10/08 12:40:33 [warn] 207881#207881 main#101: [0] test 111 0
2024/10/08 12:40:33 [info] 207881#207881 main#103: [0] test 111 0
2024/10/08 12:40:33 [error] 207881#207881 main#100: [1] test 222 10
2024/10/08 12:40:33 [warn] 207881#207881 main#101: [1] test 222 10
2024/10/08 12:40:33 [info] 207881#207886 thread_func1#31: thread 207886 loop count=4 start ...
2024/10/08 12:40:33 [warn] 207881#207886 thread_func1#35: thread 207886 count=4
2024/10/08 12:40:33 [info] 207881#207881 main#103: [1] test 222 10
2024/10/08 12:40:33 [error] 207881#207881 main#107: >>>>> change log level to debug
2024/10/08 12:40:33 [info] 207881#207887 thread_func2#54: thread 207887 start ...
2024/10/08 12:40:33 [info] 207881#207887 thread_func2#65: thread 207887 msg=test 111 cnt=0
2024/10/08 12:40:33 [info] 207881#207888 thread_func2#54: thread 207888 start ...
2024/10/08 12:40:33 [info] 207881#207888 thread_func2#65: thread 207888 msg=test 222 cnt=0
2024/10/08 12:40:33 [error] 207881#207881 main#122: [0] test 111 0
2024/10/08 12:40:33 [info] 207881#207890 thread_func2#54: thread 207890 start ...
2024/10/08 12:40:33 [info] 207881#207890 thread_func2#65: thread 207890 msg=test 444 cnt=0
2024/10/08 12:40:33 [info] 207881#207891 thread_func2#54: thread 207891 start ...
2024/10/08 12:40:33 [info] 207881#207889 thread_func2#54: thread 207889 start ...
2024/10/08 12:40:33 [warn] 207881#207881 main#123: [0] test 111 0
2024/10/08 12:40:33 [info] 207881#207891 thread_func2#65: thread 207891 msg=test 555 cnt=0
2024/10/08 12:40:33 [info] 207881#207889 thread_func2#65: thread 207889 msg=test 333 cnt=0
2024/10/08 12:40:34 [info] 207881#207883 thread_func1#43: thread 207883 exit ...
2024/10/08 12:40:34 [error] 207881#207884 thread_func1#37: thread 207884 count=1
2024/10/08 12:40:34 [warn] 207881#207885 thread_func1#35: thread 207885 count=2
2024/10/08 12:40:34 [error] 207881#207886 thread_func1#37: thread 207886 count=3
2024/10/08 12:40:34 [error] 207881#207887 thread_func2#59: thread 207887 msg=test 111 cnt=1
2024/10/08 12:40:34 [error] 207881#207888 thread_func2#59: thread 207888 msg=test 222 cnt=1
2024/10/08 12:40:34 [error] 207881#207890 thread_func2#59: thread 207890 msg=test 444 cnt=1
2024/10/08 12:40:34 [info] 207881#207881 main#125: [0] test 111 0
2024/10/08 12:40:34 [debug] 207881#207881 main#126: [0] test 111 0
2024/10/08 12:40:34 [error] 207881#207881 main#122: [1] test 222 10
2024/10/08 12:40:34 [error] 207881#207891 thread_func2#59: thread 207891 msg=test 555 cnt=1
2024/10/08 12:40:34 [warn] 207881#207881 main#123: [1] test 222 10
2024/10/08 12:40:34 [error] 207881#207889 thread_func2#59: thread 207889 msg=test 333 cnt=1
2024/10/08 12:40:35 [info] 207881#207884 thread_func1#43: thread 207884 exit ...
2024/10/08 12:40:35 [error] 207881#207885 thread_func1#37: thread 207885 count=1
2024/10/08 12:40:35 [warn] 207881#207886 thread_func1#35: thread 207886 count=2
2024/10/08 12:40:35 [warn] 207881#207887 thread_func2#62: thread 207887 msg=test 111 cnt=2
2024/10/08 12:40:35 [warn] 207881#207888 thread_func2#62: thread 207888 msg=test 222 cnt=2
2024/10/08 12:40:35 [warn] 207881#207890 thread_func2#62: thread 207890 msg=test 444 cnt=2
2024/10/08 12:40:35 [warn] 207881#207891 thread_func2#62: thread 207891 msg=test 555 cnt=2
2024/10/08 12:40:35 [warn] 207881#207889 thread_func2#62: thread 207889 msg=test 333 cnt=2
2024/10/08 12:40:35 [info] 207881#207881 main#125: [1] test 222 10
2024/10/08 12:40:35 [debug] 207881#207881 main#126: [1] test 222 10
2024/10/08 12:40:35 [info] 207881#207881 main#129: wait thread exit ...
2024/10/08 12:40:36 [info] 207881#207885 thread_func1#43: thread 207885 exit ...
2024/10/08 12:40:36 [error] 207881#207886 thread_func1#37: thread 207886 count=1
2024/10/08 12:40:36 [info] 207881#207887 thread_func2#65: thread 207887 msg=test 111 cnt=3
2024/10/08 12:40:36 [info] 207881#207888 thread_func2#65: thread 207888 msg=test 222 cnt=3
2024/10/08 12:40:36 [info] 207881#207890 thread_func2#65: thread 207890 msg=test 444 cnt=3
2024/10/08 12:40:36 [info] 207881#207891 thread_func2#65: thread 207891 msg=test 555 cnt=3
2024/10/08 12:40:36 [info] 207881#207889 thread_func2#65: thread 207889 msg=test 333 cnt=3
2024/10/08 12:40:37 [info] 207881#207886 thread_func1#43: thread 207886 exit ...
2024/10/08 12:40:37 [info] 207881#207887 thread_func2#72: thread 207887 exit ...
2024/10/08 12:40:37 [info] 207881#207888 thread_func2#72: thread 207888 exit ...
2024/10/08 12:40:37 [info] 207881#207890 thread_func2#72: thread 207890 exit ...
2024/10/08 12:40:37 [info] 207881#207891 thread_func2#72: thread 207891 exit ...
2024/10/08 12:40:37 [info] 207881#207889 thread_func2#72: thread 207889 exit ...
2024/10/08 12:40:37 [warn] 207881#207881 main#137: all thread exit, do mlog uinit
```

