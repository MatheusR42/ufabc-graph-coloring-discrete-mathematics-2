# ufabc-graph-coloring-discrete-mathematics-2

## How to Run

To compile and run the main algorithm, use the following command in your terminal:

```bash
g++ Incidence_Degree_Ordering_\(IDO\).cpp -o a.out && ./a.out
```

## Implemented Algorithms

This repository implements the following graph coloring algorithms:

- First Fit algorithm
- Welsh-Powell Algorithm
- Largest Degree Ordering Algorithm
- Incidence Degree Ordering Algorithm
- Degree of Saturation Algorithm
- Recursive Largest First Algorithm

## DIMACS Instances

The `DIMACS_Graphs_Instances/` folder contains the following benchmark instances:

- dsjc250.5
- dsjc500.1
- dsjc500.5
- dsjc500.9
- dsjc1000.1
- dsjc1000.5
- dsjc1000.9
- r250.5
- r1000.1c
- r1000.5
- dsjr500.1c
- dsjr500.5
- le450_25c
- le450_25d
- flat300_28_0
- flat1000_50_0
- flat1000_60_0
- flat1000_76_0
- latin_square
- C2000.5
- C4000.5

These instances are available in the [DIMACS Graphs: Benchmark Instances and Best Upper Bounds](https://mat.tepper.cmu.edu/COLOR/instances.html)

## Reference

This project is inspired by the following article:

**A Performance Comparison of Graph Coloring Algorithms**  
Murat Aslan and Nurdan Akhan Baykan  
International Journal of Intelligent Systems and Applications in Engineering, volume 4, pages 1–7, 2016  
Available at: [https://dergipark.org.tr/en/download/article-file/254140](https://dergipark.org.tr/en/download/article-file/254140)

## About

This project is part of the course Discrete Mathematics 2 at UFABC (Universidade Federal do ABC).
