# Blockhouse Quantitative Developer - Work Trial Submission

## 1. Overview

This project implements a high-performance order book reconstructor in C++. It processes a Market-by-Order (MBO) data stream from a CSV file and generates a corresponding Market-by-Price top-10 (MBP-10) snapshot after each valid state change. The solution is architected for correctness, maximum speed, and code clarity, adhering strictly to the provided task description.

## 2. Core Design and Data Structures

The efficiency of the order book reconstruction hinges on the choice of data structures. This implementation uses a combination of standard library containers selected for their performance characteristics in this specific context.

* **`std::map` for the Order Book**:
    * **Bids**: `std::map<int64_t, LevelInfo, std::greater<int64_t>>`
    * **Asks**: `std::map<int64_t, LevelInfo>`
    * **Justification**: `std::map` is a balanced binary search tree that maintains keys in sorted order. This is the most critical feature, as it completely eliminates the need for sorting the book levels before writing each MBP-10 snapshot. Bids are sorted descending using `std::greater`, and asks ascend by default. Insertions, deletions, and lookups are all efficient at O(log N).

* **`std::unordered_map` for Fast Order Lookups**:
    * **Implementation**: `std::unordered_map<uint64_t, OrderInfo>`
    * **Justification**: For `Cancel` (`C`) actions, the price level of an order is not given, only its unique `order_id`. To find and update this order quickly, a hash map is used to store the price and side of every active order, keyed by `order_id`. This provides an average time complexity of **O(1)** for lookups, which is essential for performance.

* **`int64_t` for Prices**:
    * **Justification**: All floating-point prices are converted to 64-bit integers by multiplying by a factor (e.g., 10,000). This canonical technique in HFT development avoids floating-point precision errors, which are a common source of bugs, and makes price comparisons and keying significantly faster.

## 3. Performance Optimizations

Beyond data structure selection, several other optimizations were implemented:

1.  **High-Speed CSV Parsing**: The standard `iostream` and `stringstream` libraries are avoided due to their high overhead. Instead, this solution uses `std::string_view` to read lines without creating new string copies, and `std::from_chars` for direct, locale-independent string-to-number conversion. This is one of the fastest parsing methods available in modern C++.

2.  **Compiler Flags**: The `Makefile` is configured to build the executable with `-O3`, the highest level of standard optimization. It also uses `-march=native` to allow the compiler to generate instructions tailored to the specific CPU architecture of the build machine, potentially unlocking further speedups.

3.  **Minimal I/O Operations**: The MBP-10 output is buffered and written line-by-line. `std::ios_base::sync_with_stdio(false)` is used to decouple C++ and C standard streams, which provides a significant I/O speed boost.

## 4. Implementation of Special Rules

The solution correctly implements all special reconstruction rules outlined in the task:

1.  **Initial State**: The first two lines of the input file (the header and the "clear book" `R` action) are read and discarded.
2.  **Trade Logic**: A `Trade` (`T`) action correctly modifies the **opposite** side of the order book. For example, an incoming `Ask` (`side = 'A'`) is treated as an aggressive order that fills a resting `Bid`, so the bid book is modified. `Fill` (`F`) events are ignored by using `continue`, ensuring that the MBP-10 state is not written for these intermediate rows, effectively combining the T-F-C sequence into a single atomic event.
3.  **'N' Side Trades**: Any row with action `T` and side `N` is explicitly skipped.

## 5. How to Build and Run

1.  **Prerequisites**: A C++17 compliant compiler (e.g., `g++` 8 or later) and the `make` utility. These are standard on most Linux and macOS systems (via Xcode Command Line Tools) and can be installed on Windows via WSL.

2.  **Build**: Navigate to the project directory in your terminal and run:
    ```sh
    make
    ```
    This will compile the source and create an executable file named `reconstruction`.

3.  **Run**: Execute the program, passing the MBO data file as the argument:
    ```sh
    ./reconstruction mbo.csv
    ```

4.  **Output**: The program will generate `mbp.csv` in the same directory.

5.  **Clean**: To remove the executable and the generated `mbp.csv`, run:
    ```sh
    make clean
    ```