# harp-sahad

The repository contains 

1. Harp-Sahad distributed codes on Hadoop cluster
2. Harp-Fascia codes on single node

## Harp-Sahad


## Harp-Fascia

Harp-Fascia re-engineers the Fascia Project of PSU (http://fascia-psu.sourceforge.net) in Java language. 
It keeps the same data structure with the original fascia c++ code to take advantage of its extremely 
fast and memory-efficient implementation. 

The current Harp-Fascia only supports shared memory system, and it has two options for Java multi-threading 
implementation

1. Java.lang.Thread class
2. Habanero Java library (Rice University)

Java.lang.Thread has a better portability than Habanero Java library (HJlib), while the latter has a 
better multi-threading performance. 

## Extend Harp-Fascia to Distributed Memory System


