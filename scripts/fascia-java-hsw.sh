#!/bin/bash

# running java-fascia on single haswell node

LIBJARS=$HOME/Project/Harp-Graph-Counting/harp-sahad/target/harp-sahad-1.0-SNAPSHOT.jar:$HOME/.m2/repository/org/apache/hadoop/hadoop-common/2.6.0/hadoop-common-2.6.0.jar:$HOME/.m2/repository/commons-cli/commons-cli/1.2/commons-cli-1.2.jar:$HOME/.m2/repository/net/java/dev/jets3t/jets3t/0.9.0/jets3t-0.9.0.jar:$HOME/.m2/repository/org/slf4j/slf4j-api/1.7.5/slf4j-api-1.7.5.jar:$HOME/.m2/repository/org/slf4j/slf4j-log4j12/1.7.5/slf4j-log4j12-1.7.5.jar:$HOME/.m2/repository/log4j/log4j/1.2.17/log4j-1.2.17.jar:$HOME/.m2/repository/it/unimi/dsi/fastutil/7.0.13/fastutil-7.0.13.jar:$HOME/.m2/repository/habanero-java-lib/habanero-java-lib/0.1.4-SNAPSHOT/habanero-java-lib-0.1.4-SNAPSHOT.jar:$HOME/.m2/repository/net/openhft/affinity/3.0.6/affinity-3.0.6.jar:$HOME/.m2/repository/net/openhft/lang/6.7.2/lang-6.7.2.jar:$HOME/.m2/repository/net/java/dev/jna/jna/4.2.1/jna-4.2.1.jar:$HOME/.m2/repository/net/java/dev/jna/jna-platform/4.2.1/jna-platform-4.2.1.jar

echo $LIBJARS

DataDir=/scratch/lc37/Fascia-Data
# ResDir=/N/u/lc37/Project/Harp-Graph-Counting/Results/Fascia-Java
# ResDir=/N/u/lc37/Project/Harp-Graph-Counting/Results/LRT-Test
ResDir=/N/u/lc37/Project/Harp-Graph-Counting/Results/Thd-Scale

# if [ $# < 3 ] ; then
# echo "usage: <graph> <template> <threadnum>"
# exit 1;
# fi

graph=nyc.graph
# graph=miami.graph
# graph=gnp.1.20.graph
# template=u3-1.fascia
template=u5-1.fascia

# thd=12
# thd=24
thd=48

# 2 sockets * 12 cores each socket on haswell Intel(R) Xeon(R) CPU E5-2670 v3 @ 2.30GHz
core_num=24 

# affinity_typ=compact
affinity_typ=scatter

paral=HJLIB
# paral=JThd
if [ "$paral" == "JThd" ];then
    exe=Fascia
else
    exe=FasciaHJ
fi

# exe=Fascia
# exe=FasciaHJ
Itr=10
# Itr=100
# Itr=1000
# Itr=10000

# for round in 1 2 3 4 5
for round in 1 
do

echo "Start Exp $exe on: $graph with template: $template and thread num: $thd core_num: $core_num itr: $Itr parallel imple: $paral affinity: $affinity_typ Round: $round"
java -cp ${LIBJARS} fascia.${exe} fascia -g ${DataDir}/graphs/$graph -t ${DataDir}/templates/$template -thread $thd -core $core_num -affinity $affinity_typ -r -i $Itr > $ResDir/Fascia-Java-$graph-$template-Thd$thd-Core$core_num-Itr$Itr-Paral-$paral-$affinity_typ-round$round.log 
echo "End Exp $exe on: $graph with template: $template and thread num: $thd core_num: $core_num itr: $Itr parallel imple: $paral affinity: $affinity_typ Round: $round"

done

