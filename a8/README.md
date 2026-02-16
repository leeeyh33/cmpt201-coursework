[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/auK542c-)
# Assignment 8: Simple Shell

## Overview

In this assignment, you will develop a simple shell. A typical shell provides a command line prompt
at which user commands are entered. It executes each command in a separate process. It also runs a
command in the background if the ampersand (`&`) is used at the end of the command. You will use the
system calls you have learned so far to implement such a shell.

There are three mandatory features that you will need to implement:

* Execute user commands in separate processes, either in the foreground or in the background if
  `&` is used.
* Implement some internal commands.
* Implement a history feature.

Before you start the assignment, read this document in its entirety and understand it first. Unlike
the assignments you probably did in your intro programming courses, there is no step-by-step guide
in this assignment. It is *your responsibility* to understand what needs to be done and *design*
the software properly that implements everything described in this document. This means that before
even writing any code, you need to think about the necessary components you need to implement and
also come up with your own step-by-step process. Write down your understanding of what needs to
be implemented and how you're going to implement them. In addition, hand-draw diagrams that
represent various components that you need and how they are related to each other. Note that you
can learn how to design software more in CMPT 276. But you do not need that background in our
assignments.

Do *not* write tens of lines of code without compiling it and testing it. You need to think about
the first step, probably the easiest thing you can write, and then write it, compile it, run it, and
test it. Do this over and over, i.e., make sure your program works at every small step and build a
working program with more and more features gradually.

Lastly, make sure you `record` your entire work session(s).

## Code Structure, CMake, and the Grader

You need to structure your code nicely according to what was discussed in A4. Also, you need to use
CMake as the build system. We expect the following code structure.

```bash
  a8
  |-- CMakeLists.txt
  |-- include
  |   \-- *.h
  |-- gtest
  |   \-- (test source)
  \-- src
      \-- *.c
```

* `include/` contains a header file already. You will use the header file for this assignment. Also,
  you need to add your own header files there.
* You need to create `src/` and add all your `.c` files.
* As we mentioned before, you need to make sure that you use the correct C and C++ compilers with
  CMake. As you know already, we use `clang` in this course, and the commands below set the C and
  C++ compilers for CMake to `clang` and `clang++`. What they do is setting the environment
  variables `CC` and `CXX`, which various programs commonly use to find the C and C++ compilers.

  ```bash
  $ export CC=$(which clang)
  $ export CXX=$(which clang++)
  ```

  In fact, this needs to be done every time you log in and our Docker configuration already does
  this automatically. We do this by adding the above two commands to `~/.zshenv`.
* `gtest/` contains the test cases for grading. We use [Google
  Test](https://github.com/google/googletest/tree/main) to develop and run the test cases. You need
  to run these test cases and pass them. If a test case fails, it is *your job* to figure out why.
  This will often require you to look into the test case source code in `gtest/`. Though it is
  written in C++, it is similar enough to C and you should be able to understand it.
* Google Test allows you to run specific test cases. You can find out how to do it yourself.
* `CMakeLists.txt` includes directives to compile the test cases. However, this `CMakeLists.txt` is
  not complete and `cmake` won't work. You need to add more directives to make it work. What you
  have learned from A4 should be enough to do it.
* After you make `cmake` work, you can run the executable it generates to run the test cases. At the
  end of each run, you will see how many points you get. We use your `CMakeLists.txt` to generate
  this executable and run it for grading (though exact test cases might differ as explained below).
  Thus, you need to make sure that it gets generated and works correctly.
* The test cases expect that there is an executable named `shell` in the build directory. That is
  the executable that you need to produce for your shell. Thus, you need to edit `CMakeLists.txt` to
  generate `shell` from your code. You can use `shell` for your own manual testing during
  development as well.
* The test cases expect that regular messages/outputs go to the standard output stream and error
  messages/outputs go to the standard error stream. You need to use these streams appropriately.
* You should be able to use regular CMake commands to generate the executable for test cases as well
  as `shell`, e.g., `cmake -S . -B build` as well as `cmake --build build`.
* Remember that test cases provide an input and check the output, and they do nothing more than
  that. If a test case fails for you, go to the test case source, find out the input it uses and the
  output it expects, and start your own debugging process with the input. It is *your job* to figure
  out which test case fails and which part of the source you need to look at.
* The outputs need to be exactly the same as described below. Otherwise, the test cases will fail.
  Keep an eye out on invisible characters. Even if you they appear to be the same, they may be
  different.
* The test cases are *not* exactly the same as what we run for our own grading. This is to prevent
  you from hard-coding or customizing your implementation tailored to our test cases. For example,
  to test an invalid command, we use a particular invalid command in the test source. This is not
  exactly the same as what we use for our own grading. Generally, you should consider the provided
  test cases as examples or specific instances of general cases.
* Still rest assured---if you do not hard-code or customize your implementation tailored to our test
  cases, what you get from running the test cases will indeed be what you get in the end.

## Nondeterministic Programs

A program that does not behave the same way at every run is called a *nondeterministic* program (as
opposed to *deterministic* program). This could be the intended behavior of the program (e.g., the
program uses threads or `random()`), or it could be a bug in the program.

In this assignment, we expect your program to produce correct outputs deterministically every time
you run your program. However, the test cases only run once and it may not catch the
nondeterministic behavior of your program, e.g., only producing correct outputs *sometimes*. Thus,
it is important for you to test thoroughly yourself to make sure that your outputs are always
correct. Otherwise, you may ultimately get a grade significantly lower than the grade you got when
you were testing.

## Task 0: Shell Prompt & Basic Command Support

The first feature you need to implement is executing programs that a user enters at the prompt. You
need to use the necessary system calls appropriately to implement this feature. Of course, you need
to handle a user command as input and pass all arguments correctly for execution. There are two
modes of execution for external programs.

* Foreground execution: In this mode, you need to execute a program in a child process and wait
  until the child process terminates.
* Background execution: In this mode, you need to execute a program in a child process but should
  not wait for the termination of the child process.
* `&` at the end of a user-entered command means that the command should run in the background.
  For example `sleep 5 &` or `sleep 5&` are both valid.

There are a few things to keep in mind.

* For all error messages, *see the macros defined in `msgs.h` for correct formatting*. Your format
  should match exactly as the macros show. The use of these is optional. You may want to consider
  using `snprinf()` to create your formatted error messages.
* Your shell prompt should always show the current working directory. For example, if the user is in
  the `/home/cmpt201` folder, the prompt should be:

  ```bash
  /home/cmpt201$
  ```

  Use the `getcwd()` function to get the current directory. If `getcwd()` returns an error, display
  this error message: `shell: unable to get current directory`.
* When a user enters a command, your shell needs to handle it properly. `strtok_r()` function might
  be helpful. Read the manpage for it.
* For foreground execution, you need to wait for the exact process you fork and not for any other
  processes.
* Since we do not wait for a background process to terminate, it can become a zombie process (where
  a child process terminates but the parent process doesn't wait on it). Thus, your shell should
  still wait on each background child process *at some point* to clean it up. There are many ways to
  achieve this, but for this assignment, you can make an *additional* `waitpid()` call every time a
  user enters a command. The following are some hints.
    * This `waitpid()` call should wait on *any child process* since you're cleaning up zombie
      processes. Use an appropriate `pid` for that.
    * This `waitpid()` call should *return immediately if no child has exited*. Use an appropriate
      option for that.
    * Consult the man page of `waitpid()` for the above two points.
    * Remember that there can be multiple background processes launched previously. Thus, a single
      call will not clean up all of them. You need to use a loop to clean up all zombie processes.
* Read in user input using `read()`. If you encounter an error when reading a user command with
  `read()`, you need to print out this error message: `shell: unable to read command`.
* If `fork()` fails, print out this error message: `shell: unable to fork`.
* If `exec` fails, print out this error message: `shell: unable to execute command`.
* If `wait` fails, print out this error message: `shell: unable to wait for child`.

## Task 1: Internal Commands and SIGINT

The second task is to implement some internal commands.

* Your shell should implement a few internal commands described below. Internal commands are
  built-in commands of the shell itself, as opposed to a separate program that is executed. You
  should not fork new processes for internal commands as they are built-in. All the commands should
  be *case-sensitive*. For all error messages, you can use the macros defined in `msgs.h` for
  correct formatting.
    * `exit`: Exit the shell program. If the user provides any argument, abort the operation (i.e.,
      do not exit) and print out this error message: `exit: too many arguments`.
    * `pwd`: Display the current working directory. Use the `getcwd()` function. If `getcwd()`
      returns an error, display this error message: `pwd: unable to get current directory`. If the
      user provides any argument, abort the operation and print out this error message: `pwd: too
      many arguments`.
    * `cd`: Change the current working directory. Use the `chdir()` function. If `chdir()` returns
      an error, display this error message `cd: unable to change directory`. If the user passed in
      more than one argument, abort the operation and print out this error message: `cd: too many
      arguments`. Additionally, implement the following features:
        * Change to the home directory if no argument is provided.
        * Support `~` for the home folder. For example, `cd ~/tmp` should change to the
          `tmp` directory under the current user's home directory. Issuing `cd ~` should switch
          to the home directory.
        * Support `-` for changing back to the previous directory. For example, suppose that the
          current working directory is `/home` and you issued `cd /` to change to the root
          directory. Then, `cd -` should switch back to the `/home` directory.
        * You may find the `getuid()` and `getpwuid()` functions useful. They allow you to gather
          useful information about the current user.
    * `help`: Display help information on internal commands.
        * If the first argument is one of the internal commands, print out the command and the help
          message. For example, if the command is `cd`, print out `cd: change the current
          directory`.
        * If the first argument is not an internal command, print out the program name and the help
          message. For example, if the argument is `ls`, print out `ls: external command or
          application`.
        * If there is more than one argument, display this error message: `help: too many
          arguments`.
        * If there is no argument provided, list all the supported internal commands one by one with
          their corresponding help messages.
* Your shell should not terminate when a user presses `CTRL-C`. Thus, you need to write a signal
  handler for `SIGINT`.
    * Have the signal handler display the help information (same as the `help` command).
    * Then re-display the command-prompt before returning.
    * A user can press `CTRL-C` anytime and interrupt system calls that are being executed. Thus,
      you need to properly check the return value of a system call to check if there was an error
      and also check `errno` to check which error it was. More specifically, you need to understand
      which value `errno` gets, if the error is caused by `SIGINT`. You need to handle `SIGINT`
      errors differently from other errors.
    * Inside the signal handler function, your code must be signal-safe. To see the list of
      functions safe for use with signals, run `man signal-safety`. This means that your signal
      handler cannot use `printf()` but rather must use `write()` instead. Code outside the signal
      handler may still use functions which are not signal safe.

## Task 2: History Feature

The next task is to modify your shell to provide a history feature that allows the user access up to
the 10 most recently entered commands. Start numbering the user's commands at 0 and increment for
each command entered. These numbers will grow past 9. For example, if the user has entered 35
commands, then the most recent 10 will be numbered 25 through 34. For all error messages, use the
macros defined in `msgs.h` for correct formatting.

* First, implement an internal command `history` which displays the 10 most recent commands executed
  in the shell. If there are less than 10 commands entered, display all the commands entered so far.
    * Display the commands in descending order (sorted by its command number).
    * The output should include both external application commands and internal commands.
    * Display the command number on the left, and the command (with all its arguments) on the right.
        * You can use the macros in `msgs.h`.
        * If the command is run in the background using `&`, it must be added to the history with
          the `&`.
    * A sample output of the history command is shown below:

      ```bash
      /home/cmpt201$ history
      30	history
      29	cd /home/cmpt201
      28	cd /proc
      27	cat uptime &
      26	cd /usr
      25	ls
      24	man pthread_create
      23	ls
      22	echo "Hello World from my shell!"
      21	ls -la
      /home/cmpt201$
      ```

* Next, implement the `!` and related commands which allows users to run commands directly from the
  history list:
    * Command `!n` runs command number n. For example, `!22` re-runs the 23rd command in the
      history. In the above example, this will re-run the echo command.
        * If n is not a number, or an invalid value (not one of the previous 10 command numbers)
          then display this error: `history: command invalid`.
    * Command `!!` runs the last command entered.
        * If there is no previous command, display the error message: `history: no command entered`.
          The history should remain empty.
    * When running a command using `!n` or `!!`, display the command from the history to the screen
      so the user can see what command they are actually running.
    * Neither the `!!` nor the `!n` commands are to be added to the history list themselves, but
      rather the command being executed from the history must be added. Here is an example.

      ```bash
      /home/cmpt201$ echo test
      test
      /home/cmpt201$ !!
      echo test
      test
      /home/cmpt201$ history
      15	history
      14	echo test
      13	echo test
      12	ls
      11	man pthread_create
      10	cd /home/cmpt201
      9	ls
      8	ls -la
      7	echo Hello World from my shell!
      6	history
      /home/cmpt201$
      ```

## Other Requirements and Exceptions

* **You should not change the provided code and files** as we will replace it for our grading. This
  is unless the description above says otherwise. This can easily result in a 0 for the assignment.
  **Be very careful about this.** You can add your own code/files as needed.
* Your code must not have any memory leaks or memory access errors. Use appropriate sanitizers and
  test your code to make sure you do not have any memory issues. Make sure you also use the debug
  option enabled in `CMakeLists.txt` to compile your code with debug information for testing.
  However, your code should work without the debug option as well. You should remove the debug
  option when you submit your code.
* Your child processes may exit without freeing all their memory. You do not need to handle such
  cases.
* You do not need to support either `>` (output redirection) or `|` (pipe).

## Grading

* **Our base grading does not use the debug option or sanitizers.** Therefore, your code should
  function correctly without them.
* Occasionally, and especially before you submit, make sure that you run the minimum checker, which
  is already installed in your Docker container (`minimum_checker`). Please don't forget to run it
  from the top directory. As with previous assignments, it performs basic checks (e.g., `.record`,
  `.nvim`, copy-and-paste, etc.). If this does not pass, you will receive a 0.
* Code that does not compile with CMake gets a 0.
* Code that does not generate all the required executables (the test executable and `shell`) gets a
  0.
* Memory issues have a penalty of -10 pts.
* A wrong code directory structure has a penalty of -10 pts.
* Distribution
    * [29 points] Task 0
    * [39 points] Task 1
    * [32 points] Task 2

## Acknowledgment

Created by Mohamed Hefeeda, modified by Brian Fraser, Keval Vora, Tianzheng Wang, and Steve Ko.
