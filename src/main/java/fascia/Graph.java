package fascia;

import java.util.Arrays;

public class Graph {
    private int num_verts;
    private int num_edges;
    private int max_deg;

    int[] adjacency_array;

    int[] degree_list;

    int[] labels;
    boolean labeled;


    void init(int n, int m, int[] srcs, int[] dsts, int[] labels, boolean labeled){
        this.labels = labels;
        this.labeled = labeled;

        num_verts = n;
        num_edges = 2 * m;
        max_deg = 0;

        adjacency_array = new int[ 2 * m];

        degree_list = new int[n + 1];
        degree_list[0] = 0;

        int[] temp_deg_list = new int[n];
        for(int i = 0; i < n; ++i){
            temp_deg_list[i] = 0;
        }

        for(int i = 0; i < m;  ++i){
            temp_deg_list[ srcs[i] ] ++;
            temp_deg_list[ dsts[i] ] ++;
        }

        for(int  i = 0; i < n; ++i){
            max_deg = temp_deg_list[i] > max_deg? temp_deg_list[i]: max_deg;
        }

        //definition of accumulated degree_list
        for(int i = 0; i < n; ++i){
            degree_list[i + 1] = degree_list[i] + temp_deg_list[i];
        }

        //copy(degree_list, degree_list+n, temp_deg_list);
        System.arraycopy(degree_list, 0, temp_deg_list, 0, n);

        //the structure of adjacency array 
        //undirected graph 
        for(int i = 0; i < m; ++i){
            adjacency_array[ temp_deg_list[srcs[i]]++ ] = dsts[i];
            adjacency_array[ temp_deg_list[dsts[i]]++ ] = srcs[i];
        }

        temp_deg_list = null;
    }

    int[] adjacent_vertices(int v){
        //return (&adjacency_array[degree_list[v]]);
        //this doesn't return original array.
        //Be aware of the difference
        return Arrays.copyOfRange(  adjacency_array, degree_list[v], degree_list[v+1]);
    }

    // v start from 0
    int out_degree(int v){
        return degree_list[v + 1] - degree_list[v];
    }

    int[] adjacencies(){
        return adjacency_array;
    }

    int[] degrees(){
        return degree_list;
    }

    int num_vertices(){
        return num_verts;
    }

    int num_edges(){
        return num_edges;
    }

    int max_degree(){
        return max_deg;
    }

    void clear(){
        adjacency_array = null;
        degree_list = null;
    }

}
