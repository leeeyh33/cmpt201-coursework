[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/5Fa9se_Y)
# Assignment 10: Simple MapReduce

In this assignment, you will implement a simple version of
[MapReduce](https://research.google/pubs/mapreduce-simplified-data-processing-on-large-clusters/).
MapReduce is a programming framework for processing large data sets with a parallel, distributed
algorithm on a cluster. It was inspired by the
[map](https://en.wikipedia.org/wiki/Map_(higher-order_function)) and
[reduce](https://en.wikipedia.org/wiki/Fold_(higher-order_function)) functions commonly used in
functional programming. The key components of the framework are those two functions: `map` and
`reduce`. The `map` function processes a key-value pair to generate a set of intermediate key-value
pairs. The `reduce` function processes the intermediate key-value pairs to generate the final
output. The framework is widely used in distributed computing, and it is the foundation of many big
data processing systems, such as [Hadoop](https://hadoop.apache.org/) and
[Spark](https://spark.apache.org/). It is used in many courses as assignments due to its elegance
and popularity. In this assignment, you will implement a simple version of the MapReduce framework
using threads and synchronization primitives.

We note that the description here largely comes from [the original MapReduce
paper](https://storage.googleapis.com/gweb-research2023-media/pubtools/pdf/16cb30b4b92fd4989b8619a61752a2387c6dd474.pdf).

## Understanding the Basics of MapReduce

MapReduce takes a list of key-value pairs as input and produces a list of new key-value pairs as
output. A key-value pair is a tuple where the first element is called the key and the second element
is called the value. For example, `("name", "John")` is a key-value pair where `"name"` is the key
and `"John"` is the value. MapReduce takes a list of such key-value pairs as input and processes
them to generate a list of new key-value pairs as output. As a side note, if you have many key-value
pairs, you can think of them as a table with two columns, where the first column is the key and the
second column is the value. A key can be used to look up the corresponding value in the table.

For example, suppose we have the following list of key-value pairs as input, where each key is a
line number and each value is a line of text:

```
("1", "the quick brown fox")
("2", "jumps over the lazy dog")
("3", "the quick brown fox")
("4", "jumps over the lazy dog")
```

Or, in table form:

```
+-----+-------------------------+
| key | value                   |
+-----+-------------------------+
| 1   | the quick brown fox     |
| 2   | jumps over the lazy dog |
| 3   | the quick brown fox     |
| 4   | jumps over the lazy dog |
+-----+-------------------------+
```

Since MapReduce does not use this table form, we will not use it in the rest of this document.

Now, suppose we want to process the above list of key-value pairs to count the number of occurrences
of each word. The output could look like the following, where each key is a word and each value is
the number of occurrences of the word in the input:

```
("the", "4")
("quick", "2")
("brown", "2")
("fox", "2")
("jumps", "2")
("over", "2")
("lazy", "2")
("dog", "2")
```

(As a side note, using this output, we can use a word as a key to look up the corresponding value,
i.e., the number of occurrences of the word.)

How do you perform such processing? There can be many algorithms, but one algorithm is to do it in
three stages. In the first stage, we look at each input key-value pair, and for each word we
encounter, we say there is *one* occurrence of the word. To keep track of this, we can use `("word",
"1")`, e.g., `("the", "1")`, `("quick", "1")`, etc., and save those in memory. This is just
recording an encounter, not necessarily counting, i.e., if we encountered the same word again, e.g.,
`"the"`, then we would put another `("the", "1")` in memory.

Then in the second stage, we look at all those encounters and group them by each word. For example,
if we encountered the word `"the"` twice, we would have `("the", "1")` and `("the", "1")` in memory.
We then group them into a single entry, e.g., `("the", ["1", "1"])`. This is still not counting but
looking at what we have in memory and grouping the encounter recordings by each word.

Lastly, we perform the actual counting in the third stage. We look at all the groupings and add up.
For example, if we encountered `"the"` three times and `"quick"` twice, the second stage would
produce `("the", ["1", "1", "1"])` and `("quick", ["1", "1"])`. We now look at each entry and add up
the encounters to produce the actual count of each word, e.g., `("the", "3")` and `("quick", "2")`.

Now, MapReduce allows us to perform such processing. It has three stages that generalize the stages
described above. They are called the map stage, the group-by-key stage, and the reduce stage. The
map stage runs a function called `map` for each and every input key-value pair. This `map` function
typically generates new key-value pairs, and the second stage (the group-by-key stage) collects
these new key-value pairs and group them by each unique key. The reduce stage runs a function called
`reduce` for each and every key-value pair grouped by the group-by-key stage. Let's look at each
stage in more detail.

### The Map Stage

As mentioned, the map stage runs a function called `map` for each and every input key-value pair.
The important thing is that MapReduce treats this `map` function as a black box. The actual
implementation of `map` is customizable and can be written by anyone. The job of MapReduce is to run
`map` correctly, i.e., repeatedly calling `map` for each and every input key-value pair. The output
of `map` has to be a set of key-value pairs (no matter who writes it). For example, suppose you
wrote a `map` function as follows (in pseudocode):

```
map(key, value):
  foreach word in value:
    write-output(word, "1")
```

This pseudocode implements the encounter counting algorithm described before. What it does is that,
it takes a single (key, value) pair (as the input parameters `(key, value)` indicate), iterates over
each word in the value (as `foreach` says), and produces a set of output key-value pairs in the form
of `(word, "1")` (as `write-output` says). For example, suppose the input were `("1", "the quick
brown fox")`. The above `map` would produce `("the", "1"), ("quick", "1"), ("brown", "1"), and
("fox", "1")` as the output. These are essentially recording the encounters of these words from the
input.

Again, this `map` is customizable and MapReduce considers this a black box. The job of MapReduce is
to repeatedly call this `map` function for each and every input key-value pair. For example,
consider the following input.

```
("1", "the quick brown fox")
("2", "jumps over the lazy dog")
("3", "the quick brown fox")
```

Since there are three key-value pairs, MapReduce would call `map` three times, i.e., one call for
each key-value pair. Those three `map` calls would produce the following output sets:

First set for the input ("1", "the quick brown fox"):
```
("the", "1")
("quick", "1")
("brown", "1")
("fox", "1")
```

Second set for ("2", "jumps over the lazy dog"):
```
("jumps", "1")
("over", "1")
("the", "1")
("lazy", "1")
("dog", "1")
```

Third set ("3", "the quick brown fox"):
```
("the", "1")
("quick", "1")
("brown", "1")
("fox", "1")
```

The outputs of `map` is called *intermediate* outputs as there are two more stages to go through in
MapReduce.

### The Group-By-Key Stage

The second stage, the group-by-key stage, collects all intermediate outputs generated by the map
stage. This is a generalization of the grouping algorithm described in the overview. The job of the
group-by-key stage is to group all outputs by their keys. For example, if you examine the previous
example, you can see that across different sets of intermediate outputs, there are overlapping keys.
For example, the key `"the"` appears across three sets and the key `"quick"` appears across two
sets. Then, the group-by-key stage looks at all the keys, identifies unique keys, and creates a list
of values that are associated with each unique key. For example, for the key `"the"`, there are
three values associated with it---`"1"` from the first set's first key-value pair, `"1"` from the
second set's third key-value pair, and `"1"` from the third set's first key-value pair. (In our
example, all values are identical (`"1"`), but generally speaking, values can be different.) Thus,
the group-by-key stage looks at the unique key `"the"` and groups the values by the key as follows:

```
("the", ["1", "1", "1"])
```

Likewise, for the unique key `"quick"`, the group-by-key stage sees that there are two values
associated with it and groups them as follows:

```
("quick", ["1", "1"])
```

The group-by-key stage does it for all unique keys and passes those to the next stage, the reduce
stage.

### The Reduce Stage

The reduce stage runs a function called `reduce` for each key-value pairs grouped by the
group-by-key stage. Similar to the map stage, the reduce stage considers the `reduce` function a
black box, and anyone can implement it. The job of the reduce stage is to run it correctly, i.e.,
calling `reduce` for each and every key-value pair grouped by the group-by-key stage.

For each key-value pair grouped by the group-by-key stage, `reduce` takes the value, which is a
list, and produces a single value. This is why it's called *reduce* (reducing a list to a single
value). For example, suppose you wrote a `reduce` function as follows:

```
reduce(key, value):
  total = 0
  foreach one-occurrence in value:
    total = total + 1
  return total
```

This `reduce` function implements the last part of the encounter algorithm described before---it
takes a list of encounter recordings and produces an actual count. For example, consider the
following key-value pairs grouped by the group-by-key stage:

```
("the", ["1", "1", "1"])
("quick", ["1", "1"])
```

The reduce stage would call `reduce` twice. One call would produce `("the", "3")` as the output, and
the other call would produce `("quick", "2")` as the output. As a whole, the final output of the
reduce stage would be the following.

```
("the", "3")
("quick", "2")
```

### Complete Outputs

Using the following input, MapReduce with the above `map` and `reduce` functions would produce the
following outputs.

Input:
```
("1", "the quick brown fox")
("2", "jumps over the lazy dog")
("3", "the quick brown fox")
("4", "jumps over the lazy dog")
```

The complete list of what the map stage produces:
```
("the", "1")
("quick", "1")
("brown", "1")
("fox", "1")
("jumps", "1")
("over", "1")
("the", "1")
("lazy", "1")
("dog", "1")
("the", "1")
("quick", "1")
("brown", "1")
("fox", "1")
("jumps", "1")
("over", "1")
("the", "1")
("lazy", "1")
("dog", "1")
```

The complete list of what the group-by-key stage produces:
```
("the", ["1", "1", "1", "1"])
("quick", ["1", "1"])
("brown", ["1", "1"])
("fox", ["1", "1"])
("jumps", ["1", "1"])
("over", ["1", "1"])
("lazy", ["1", "1"])
("dog", ["1", "1"])
```

The complete list of what the reduce stage produces, which is the final output of MapReduce:
```
("the", "4")
("quick", "2")
("brown", "2")
("fox", "2")
("jumps", "2")
("over", "2")
("lazy", "2")
("dog", "2")
```

The above final output is the list of key-value pairs that show the number of occurrences of each
word in the input.

As it turns out, `map` and `reduce` are quite powerful and can be used to solve a wide variety of
problems. For example, `map` and `reduce` can be used to find a line in a file that contains a word,
to count the access frequency of a web page in a web server log, to get the list of all URLs that
contain a link to a specific web page, and so on.

## Understanding Concurrency in MapReduce

The original MapReduce is designed to process large data sets where the `map` and `reduce` functions
are executed concurrently on a cluster of machines. In this assignment, you will not use a cluster
of machines, but you will use threads to execute `map` and `reduce` concurrently on a single
machine.

The concurrent execution of `map` and `reduce` works as follows.
* The MapReduce framework creates a set of threads to execute the `map` function and another set of
  threads to execute the `reduce` function.
* The framework *partitions* the input key-value pairs into multiple chunks. The number of chunks is
  equal to the number of `map` threads. Let's consider the above example again to understand this
  better. If we have two `map` threads, the framework partitions the input key-value pairs into two
  chunks. The following will be the partitioning of the input key-value pairs:
    * Chunk 0: `("1", "the quick brown fox"), ("2", "jumps over the lazy dog")`
    * Chunk 1: `("3", "the quick brown fox"), ("4", "jumps over the lazy dog")`
* The framework then assigns each chunk to a `map` thread.
* Each `map` thread processes its assigned chunk by calling the `map` function repeatedly, once for
  each input key-value pair in the assigned chunk. Note that all the `map` threads run at the same
  time, processing their assigned chunks concurrently.
* The framework then collects all the intermediate key-value pairs produced by all `map` threads.
* The framework then again *partitions* the intermediate key-value pairs into multiple chunks and
  assigns each chunk to a `reduce` thread. Again using the above example, if we have four `reduce`
  threads, the framework partitions the intermediate key-value pairs into four chunks. The following
  will be the partitioning of the intermediate key-value pairs:
    * Chunk 0: `("the", ["1", "1", "1", "1"]), ("quick", ["1", "1"])`
    * Chunk 1: `("brown", ["1", "1"]), ("fox", ["1", "1"])`
    * Chunk 2: `("jumps", ["1", "1"]), ("over", ["1", "1"])`
    * Chunk 3: `("lazy", ["1", "1"]), ("dog", ["1", "1"])`
* The framework then assigns each chunk to a `reduce` thread.
* Each `reduce` thread processes its assigned chunk by calling the `reduce` function repeatedly,
  once for each intermediate key and its associated list of intermediate values in its assigned
  chunk. Note that all the `reduce` threads run at the same time, processing their assigned chunks
  concurrently.
* The framework then collects all the final key-value pairs produced by all `reduce` threads and
  outputs them as the final output.

## Understanding Your Task

* You are to implement a MapReduce framework. In order to understand how a developer would use the
  framework, take a look at the test cases provided in [`src/`](./src/). The test cases define
  various `map` and `reduce` functions and also use `mr_emit_i()` and `mr_emit_f()` appropriately to
  handle intermediate and final key-value pairs.
* For your MapReduce framework, you will implement the following three functions (as defined in
  [interface.h](./include/interface.h):
    * `mr_exec()` is the main function that a developer (client code) will call to run their `map`
      and `reduce` functions.
    * `mr_emit_i()` will be called by the developer's map function to record any key-value pairs
      created while processing one of the input key-value pairs.
    * `mr_emit_f()` will be called by the developer's reduce function to record any key-value pairs
      that are part of the output.
* The developer who wants to use your MapReduce framework must implement two functions which are
  passed to your `mr_exec()` function as function pointers:
    * The developer's own map function. This function is supposed to be called once for each input
      key-value pair.
    * The developer's own reduce function. This function is supposed to be called once for each
      intermediate key-value pair after grouping.
* A big part of MapReduce is running the map and reduce steps in parallel. Your framework will use
  parallelism in the following way:
    * The set of input key-value pairs will be split up into chunks, and each chunk will be
      processed in a different thread as described above. Note that "processsing" a chunk means
      calling the developer's `map` function appropriately.
    * The grouping of intermediate key-value pairs needs to be done by your framework after all of
      the threads for running map have finished.
    * After grouping, the set of intermediate key-value pairs will be split into chucks. Each chunk
      will be processed on a different thread. Note that "processsing" a chunk means calling the
      developer's `reduce` function appropriately.
* Note that the above MapReduce example mentions counting the occurrences of words in input strings.
  This string tokenization and string processing would be done inside the client's map function.
  Your MapReduce framework does not actually process the input keys or input values in any way; it
  just passes them to the map function for it to process.
* Testing the framework can be challenging and we encourage you to study the test cases and the
  order of the test cases that we run. It will give you enough insight into how to implement your
  framework in what order. Still, the nature of having the map and reduce callback functions can
  make it tricky to follow the code's execution. Therefore, you should expect to create your own
  test cases to work through implementing and debugging your code. You may want to create a new
  `self_testing.c` file with a `main()` which calls your `mr_exec()` and defines your own `map()`
  and `reduce()` test functions. You can build this into your own `self_testing` executable to run
  and debug.
    * Marking will be done by executing the `mapreduce` executable described below.

## Detailed Requirements

There are several requirements for this assignment.
* Your MapReduce framework is a library that implements necessary functions that a developer uses to
  process their data.
* Create ore (or more) new `.c` file(s) in `src/` to contain your implementation of the MapReduce
  library functions defined in [interface.h](./include/interface.h). You can add additional helper
  functions or global variables to your file as needed.
* **You should not change the provided code and files** as we will replace it for our grading. This
  is unless the description above says otherwise. This can easily result in a 0 for the assignment.
  **Be very careful about this.** You can add your own code/files as needed.
* [interface.h](./include/interface.h) defines the functions that you need to implement.
    * `mr_exec()` executes the MapReduce framework. A developer calls this function to run `map()`
      and `reduce()`, and get the final output. You need to write this function to run the
      framework. A developer can call this function as many times as they want to process different
      input key-value pairs. The function takes the following arguments.
        * The input key-value pairs.
        * The `map()` and `reduce()` functions that the framework should execute. Note that a
          developer writes `map()` and `reduce()` functions. `map()` takes *one* key-value pair as
          input and generates *zero or more* new intermediate key-value pairs as output. `reduce()`
          takes *one* intermediate key and the list of intermediate values associated with the key
          as input and generates *zero or more* new final key-value pairs as output.
        * The number of `map` threads and the number of `reduce` threads. You need to create the
          specified number of threads to execute the `map` and `reduce` functions.
        * A buffer for storing the final output. The framework should store the final output in this
          buffer.
    * `mr_emit_i()` and `mr_emit_f()` handle intermediate and final key-value pairs. A developer
      calls (within `map`) `mr_emit_i()` to pass intermediate key-value pairs to the framework, and
      calls (within `reduce`) `mr_emit_f()` to pass final key-value pairs to the framework. You need
      to write these two functions to collect intermediate and final key-value pairs. A developer
      can call these functions as many times as they want within `map()` and `reduce()`,
      respectively.
    * The order of the input key-value pairs should be preserved for `map()`. For example, if the
      input key-value pairs have the following order: `("1", "the quick brown fox")`, `("2", "jumps
      over the lazy dog")`, `("3", "the quick brown fox")`, and `("4", "jumps over the lazy dog")`,
      the framework should call `map()` with the input key-value pairs in the same order. When
      dividing the input into chunks, the framework should also preserve the order of the input
      key-value pairs. For example, if we use the above input key-value pairs and two `map` threads,
      the framework should call `map()` with the following two chunks: `("1", "the quick brown
      fox"), ("2", "jumps over the lazy dog")` and `("3", "the quick brown fox"), ("4", "jumps over
      the lazy dog")`, one for each thread.
    * The order of the intermediate key-value pairs should be sorted by the key, according to the
      result of the `strcmp()` function. For example, if the intermediate key-value pairs have the
      following key-value pairs: `("brown", "1")`, `("fox", "1")`, `("dog", "1")`, `("brown", "1")`,
      and `("fox", "1")`, the framework should sort the intermediate key-value pairs in the
      following order: `("brown", "1")`, `("brown", "1")`, `("dog", "1")`, `("fox", "1")`, and
      `("fox", "1")`. The framework should then group them and divide them into chunks in the same
      way as the input key-value pairs.
    * Similarly, the order of the output key-value pairs should be sorted by the key, according to
      the result of the `strcmp()` function. For example, if the final output has the following
      key-value pairs: `("brown", "2")`, `("fox", "2")`, and `("dog", "2")`, the framework should
      store the final output in the following order: `("brown", "2")`, `("dog", "2")`, and `("fox",
      "2")`.
    * When partitioning input or intermediate key-value pairs into chunks, the size of each chunk
      should be as evenly distributed as possible. For example, if we have 7 key-value pairs and 3
      threads, the first chunk should contain 3 key-value pairs and the last two chunks should
      contain 2 key-value pairs. If we have 6 key-value pairs instead, each chunk should contain 2
      key-value pairs. This means that each chunk size should at least be the quotient of `(all
      key-value pairs) / (number of threads)`. However, for `X` number of chunks, where `X` is equal
      to the reminder, the chunk size should be one more.
* You need to use `pthread` to create threads. You should also use appropriate synchronization
  primitives to ensure that the threads work correctly.
* You need to write `CMakeLists.txt` that produces a single executable named `mapreduce` that runs
  the test cases in [`main.c`](./src/main.c). **Be very careful about file names and
  capitalization**. If they are not exactly what we expect, it will result in a 0.
* You may find [uthash](https://troydhanson.github.io/uthash/userguide.html) useful.
* Similar to previous assignments, you should not hard-code or customize your implementation
  tailored to our test cases. Generally, you should consider the provided test cases as examples or
  specific instances of general cases. For example, we can change the constants, the input key-value
  pairs, the `map` and `reduce` functions, the number of `map` and `reduce` threads, and so on. Your
  implementation should still work correctly.

## Grading

* **Our base grading does not use the debug option or sanitizers.** Therefore, your code should
  function correctly without them.
* A single map thread and a single reduce thread
    * [10 pts] Correctly call map with the input key-value pairs using a single thread.
    * [10 pts] Correctly call reduce with the intermediate key-value pairs using a single thread.
    * [15 pts] Correctly generate final output using a single map thread and a single reduce thread.
* Multiple `map` and `reduce` threads
    * [5 pts] Run the correct number of `map` threads.
    * [5 pts] Run the correct number of `reduce` threads.
    * [15 pts] Correctly partition the input key-value pairs into multiple chunks and assign each
      chunk to a `map` thread.
    * [15 pts] Correctly partition the intermediate key-value pairs into multiple chunks and assign
      each chunk to a `reduce` thread.
    * [20 pts] Correctly generate final output.
* [5 pts] Pass all the above test cases.
* Occasionally, and especially before you submit, make sure that you run the minimum checker, which
  is already installed in your Docker container (`minimum_checker`). Please don't forget to run it
  from the top directory. As with previous assignments, it performs basic checks (e.g., `.record`,
  `.nvim`, copy-and-paste, etc.). If this does not pass, you will receive a 0.
* Code that does not compile with `CMake` gets a 0.
* Code that does not generate all the required executables gets a 0.
* You should not have any data races. We will test this with Clang's thread sanitizer. If a data
  race is found, there will be a penalty of -10 pts.
* A deadlock or a livelock should not occur. If a deadlock or a livelock occurs at any point, the
  grader will stop grading the code and you will get whatever points you have at that time.
* You should not have any memory issues. We will test this with Clang's address sanitizer. If a
  memory issue is found, there will be a penalty of -10 pts. For your testing, make sure you also
  use the debug option enabled in `CMakeLists.txt` to compile your code with debug information.
  However, your code should work without the debug option as well. You should remove the debug
  option when you submit your code.
* You need to use the same code structure as A8. A wrong code directory structure has a penalty of
  -10 pts.
