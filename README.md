A basic C program to test Jon Blow's explanations
of a simple precedence aware parser.

[Youtube Video](https://www.youtube.com/watch?v=fIPO4G42wYE)

```

Usage:

    cat example_03.txt | ./precedence_parser

Input:

    # left site comment
    (8 + 10) * 3 / (4 - 3) 
    
    
    # less than
    < 
    
    # right side comment
    25 * 3 * 2

Output:

    Binary Operator: <
        Binary Operator: /
            Binary Operator: *
                Binary Operator(): +
                    Leaf: 8
                    Leaf: 10
                Leaf: 3
            Binary Operator(): -
                Leaf: 4
                Leaf: 3
        Binary Operator: *
            Binary Operator: *
                Leaf: 25
                Leaf: 3
            Leaf: 2
    OK

```
