A basic C program to test Jon Blow's explanations
of a simple precedence aware parser.

[Youtube Video](https://www.youtube.com/watch?v=fIPO4G42wYE)

```
Input:

2 * 3 + 5 * 4 - 6 > 7 * 8

Output:

Binary Operator: >
    Binary Operator: -
        Binary Operator: +
            Binary Operator: *
                Leaf: 2
                Leaf: 3
            Binary Operator: *
                Leaf: 5
                Leaf: 4
        Leaf: 6
    Binary Operator: *
        Leaf: 7
        Leaf: 8

```
