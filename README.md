# towersOfHanoi

A C++17 program that computes the minimum number of moves required to solve a
Towers of Hanoi puzzle with `n` disks, and estimates how long the solution would
take at a user-supplied rate of moves per second.

The program does not simulate individual disk movements. It evaluates the
closed-form minimum directly, using hand-rolled arbitrary-precision decimal
arithmetic so that results are not capped by the range of fixed-width integer
types. The project connects a classical recurrence relation to a working
implementation: recursion, recurrence relations, geometric series, exponential
growth, arbitrary-precision arithmetic, and the practical limits of
computation.

## Mathematics

For a tower of `n` disks, let `M(n)` be the minimum number of moves needed to
transfer the tower from the source peg to the destination peg. Every solution
must move the `n−1` smaller disks to the auxiliary peg, move the largest disk
once, then move the `n−1` smaller disks onto the destination peg:

```
M(n) = 2·M(n−1) + 1,    M(0) = 0
```

Unrolling the recurrence and summing the resulting geometric series gives the
closed form

```
M(n) = 2ⁿ − 1
```

At a rate of `r` moves per second, the required time is therefore

```
t(n, r) = (2ⁿ − 1) / r
```

Because both functions grow exponentially, even modest increases in `n` produce
enormous increases in the number of moves and in the time required.

## Features

- Computes `2ⁿ − 1` for disk counts up to 100,000
- Accepts integer, decimal, and scientific-notation move rates
- Uses custom arbitrary-precision decimal string arithmetic
- Avoids overflow from standard fixed-width integer types
- Reports completion time in seconds, minutes, hours, days, or years, choosing
  the largest unit that keeps the value at or above one
- Tracks time internally with nanosecond precision
- Validates input and reports descriptive errors
- Uses a Julian year of 365.25 days (31,557,600 seconds)

## Why arbitrary-precision arithmetic?

The minimum move count exceeds the capacity of standard integer types almost
immediately. For 64 disks,

```
M(64) = 2⁶⁴ − 1 = 18,446,744,073,709,551,615
```

which is exactly the maximum value an unsigned 64-bit integer can hold. Any
larger puzzle overflows that data type.

To avoid the limitation, the program stores large values as decimal strings and
performs arithmetic digit by digit. This allows it to represent numbers
containing thousands of digits.

## Build requirements

- A C++17-compatible compiler
- GCC or Clang with support for `unsigned __int128`

`unsigned __int128` is a compiler extension and may not be supported by
Microsoft Visual C++.

## Building the program

```bash
g++ -std=c++17 -Wall -Wextra -O2 towers.cpp -o towers
```

## Running the program

```bash
./towers
```

The program asks for:

1. The number of disks
2. The number of moves completed per second

### Example

```text
Enter the number of disks: 64
Moves per second? 1e9
Disks: 64
Minimum moves: 18446744073709551615
Moves per second: 1e9
Time to solve: 584.542 years
```

Input may also be redirected:

```bash
printf '64\n1e9\n' | ./towers
```

## Accepted move rates

The rate may be written in several numeric formats:

```text
1000000
2.5
1e9
1.5e-3
```

The rate must be greater than zero. It is parsed into an exact numerator and
denominator rather than a floating-point value, which keeps rounding error out
of the time calculation.

## How it works

`movesForDisks` starts with the decimal string `"1"` and doubles it `n` times:

```cpp
std::string digits = "1";

for (u64 i = 0; i < n; ++i) {
    multiplyBySmall(digits, 2);
}
```

After `n` doublings the string represents `2ⁿ`. The program then subtracts one,
leaving `2ⁿ − 1`.

Because it evaluates the closed form instead of enumerating moves, the program
can report results for puzzles whose complete move sequences would be far too
large to generate or store.

## Main components

| Function | Role |
| --- | --- |
| `movesForDisks` | Computes `2ⁿ − 1` using arbitrary-precision decimal strings |
| `multiplyBySmall` | Multiplies a decimal string by a fixed-width integer using digit-by-digit multiplication and carrying |
| `divideBySmall` | Divides a decimal string by a fixed-width integer using long division |
| `parseRate` | Parses integer, decimal, and scientific-notation rates into an exact numerator and denominator |
| `formatNumber` | Formats the computed time, removing unnecessary trailing zeros |
| `stripLeadingZeros`, `appendZeros`, `addOne` | Supporting decimal-string operations for normalization, scaling, and rounding |

## Input limits

- The disk count must be a nonnegative whole number
- Maximum disk count: 100,000
- The move rate must be greater than zero
- Maximum supported rate: 10¹⁸
- Smallest supported rate: 10⁻¹⁸
- Rate input may contain up to 19 significant digits

These limits keep the calculation practical while still allowing extremely
large results.

## Historical context

The Towers of Hanoi was introduced by the French mathematician Édouard Lucas in
the nineteenth century, published under the pseudonym N. Claus (de Siam). Its
famous legend describes 64 golden disks on three diamond needles in a temple at
Benares, moved one at a time by monks; when the transfer is complete, the world
is said to end.

At one move per second, completing the 64-disk puzzle would take approximately
584 billion years. Even at one billion moves per second it would still take
about 584.5 years.

## Project purpose

This project connects a mathematical proof to a computational implementation.
It shows how a puzzle governed by three simple rules introduces major ideas in:

- Recursive problem solving
- Recurrence relations
- Geometric series
- Exponential complexity
- Arbitrary-precision arithmetic
- Algorithm design
- Numerical representation
- Computational feasibility

The Towers of Hanoi is mathematically solvable for any number of disks, but its
exponential growth makes large cases physically impractical. That difference
between theoretical possibility and practical computation is the central idea
explored here.
